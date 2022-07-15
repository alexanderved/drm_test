#include "include/types/device.h"
#include "include/types/output.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/kd.h> 
#include <sys/vt.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include <linux/major.h>

// Starts graphics mode on VT/TTY.
static int vt_setup(struct device *device) {
    const char *tty_env_num = getenv("TTYNO");
    int tty_num = 0;
    char tty_dev[32];

    if (tty_env_num) {
        char *endptr = NULL;

        tty_num = strtoul(tty_env_num, &endptr, 10);
        if (tty_num == 0 || *endptr != '\0') {
            fprintf(stderr, "Invalid $TTYNO environment variable\n");
			return 1;
        }

        snprintf(tty_dev, sizeof(tty_dev), "/dev/tty%d", tty_num);
    } else if (ttyname(STDIN_FILENO)) {
        ttyname_r(STDIN_FILENO, tty_dev, sizeof(tty_dev));
    } else {
        int tty0;

        tty0 = open("/dev/tty0", O_WRONLY | O_CLOEXEC);
        if (tty0 < 0) {
            fprintf(stderr, "Couldn't open /dev/tty0\n");
			return errno;
        }

        if (ioctl(tty0, VT_OPENQRY, &tty_num) < 0 || tty_num < 0) {
            fprintf(stderr, "Couldn't find free TTY\n");
            close(tty0);
			return errno;
        }

        close(tty0);
        snprintf(tty_dev, sizeof(tty_dev), "/dev/tty%d", tty_num);
    }

    device->vt_fd = open(tty_dev, O_RDWR | O_NOCTTY);
    if (device->vt_fd < 0) {
        fprintf(stderr, "Failed to open TTY%d\n", tty_num);
        return errno;
    }

    if (tty_num == 0) {
        struct stat vt_buffer;

        if (fstat(device->vt_fd, &vt_buffer) == -1 ||
            major(vt_buffer.st_rdev) != TTY_MAJOR)
        {
            fprintf(stderr, "TTY %s is bad\n", tty_dev);
            return 1;
        }

        tty_num = minor(vt_buffer.st_rdev);
    }

    printf("Using TTY%d\n", tty_num);

    if (ioctl(device->vt_fd, VT_ACTIVATE, tty_num) != 0 ||
        ioctl(device->vt_fd, VT_WAITACTIVE, tty_num) != 0)
    {
        fprintf(stderr, "Couldn't switch to TTY%d\n", tty_num);
		return errno;
    }

    /*if (ioctl(device->vt_fd, KDGKBMODE, &device->saved_kb_mode) != 0 ||
	    ioctl(device->vt_fd, KDSKBMODE, K_OFF) != 0) {
		fprintf(stderr, "Failed to disable TTY keyboard processing\n");
		return errno;
	}

    if (ioctl(device->vt_fd, KDSETMODE, KD_GRAPHICS) != 0) {
		fprintf(stderr, "Failed to switch TTY to graphics mode\n");
		return errno;
	}*/

    return 0;
}

static void vt_reset(struct device *device)
{
    //ioctl(device->vt_fd, KDSKBMODE, device->saved_kb_mode);
	ioctl(device->vt_fd, KDSETMODE, KD_TEXT);
}

static struct device *device_open(const char *filename) {
    struct device *device;
    drm_magic_t magic;
    uint64_t capabilities;
    int error;

    device = calloc(1, sizeof(struct device));
    assert(device);
    
    device->kms_fd = open(filename, O_RDWR | O_CLOEXEC, 0);
    if (device->kms_fd < 0) {
        fprintf(stderr, "Couldn't open %s: %s\n", filename, strerror(errno));
        device_destroy(&device);

        return NULL;
    }

    printf("Device KMS fd: %d\n", device->kms_fd);

    if (drmGetMagic(device->kms_fd, &magic) != 0 ||
        drmAuthMagic(device->kms_fd, magic) != 0) {
        fprintf(stderr, "KMS device %s is not master\n", filename);
        device_destroy(&device);

        return NULL;
    }

    if (drmSetClientCap(device->kms_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) != 0) {
        fprintf(stderr, "Setting DRM_CLIENT_CAP_UNIVERSAL_PLANES failed\n");
        device_destroy(&device);

        return NULL;
    }

    if (drmSetClientCap(device->kms_fd, DRM_CLIENT_CAP_ATOMIC, 1) != 0) {
        fprintf(stderr, "Setting DRM_CLIENT_CAP_ATOMIC failed\n");
        device_destroy(&device);

        return NULL;
    }

    error = drmGetCap(device->kms_fd, DRM_CAP_ADDFB2_MODIFIERS, &capabilities);
    device->supports_fb_modifiers = (error == 0 && capabilities != 0);
    printf("Device %s framebuffer modifiers\n",
           device->supports_fb_modifiers ? "supports" : "not supports");

    if (drmGetCap(device->kms_fd, DRM_CAP_CURSOR_WIDTH, &device->cursor_width) != 0)
        device->cursor_width = BASE_CURSOR_WIDTH;
    if (drmGetCap(device->kms_fd, DRM_CAP_CURSOR_HEIGHT, &device->cursor_height) != 0)
        device->cursor_height = BASE_CURSOR_HEIGHT;

    device->resources = drmModeGetResources(device->kms_fd);
    if (!device->resources) {
        fprintf(stderr, "Couldn't get resources for %s\n", filename);
        device_destroy(&device);

        return NULL;
    }

    if (device->resources->count_crtcs <= 0 || device->resources->count_connectors <= 0 ||
        device->resources->count_encoders <= 0)
    {
        fprintf(stderr, "Device %s is not KMS device\n", filename);
        device_destroy(&device);

        return NULL;
    }

    dt_array_init(&device->plane_array, sizeof(struct plane));
    dt_array_init(&device->crtc_array, sizeof(struct crtc));
    dt_array_init(&device->connector_array, sizeof(struct connector));

    if (plane_fill_array(device, &device->plane_array) != 0) {
        fprintf(stderr, "Couldn't get planes for %s\n", filename);
        device_destroy(&device);

        return NULL;
    }

    if (crtc_fill_array(device, &device->crtc_array) != 0) {
        fprintf(stderr, "Couldn't get crtcs for %s\n", filename);
        device_destroy(&device);

        return NULL;
    }

    if (connector_fill_array(device, &device->connector_array) != 0) {
        fprintf(stderr, "Couldn't get connectors for %s\n", filename);
        device_destroy(&device);

        return NULL;
    }

    dt_array_init(&device->output_array, sizeof(struct output));

    if (output_fill_array(device, &device->output_array) != 0) {
        fprintf(stderr, "Couldn't get outputs for %s\n", filename);
        device_destroy(&device);

        return NULL;
    }

    return device;
}

struct device *device_create() {
    struct device *device = device_open("/dev/dri/card0");

    if (vt_setup(device) != 0) {
        fprintf(stderr, "Couldn't set up TTY for graphics mode\n");
        device_destroy(&device);

        return NULL;
    }

    return device;
}

void device_destroy(struct device **device) {
    if (!device || !(*device)) return;

    vt_reset(*device);

    output_empty_array(&(*device)->output_array);

    connector_empty_array(&(*device)->connector_array);
    crtc_empty_array(&(*device)->crtc_array);
    plane_empty_array(&(*device)->plane_array);

    if ((*device)->resources) drmModeFreeResources((*device)->resources);
    if ((*device)->kms_fd >= 0) close((*device)->kms_fd);

    free(*device);
    *device = NULL;
}