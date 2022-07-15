#include "include/renderer/dumb/framebuffer.h"
#include "include/utils/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

static uint32_t r = 0, g = 0x70, b = 0;

static void dumb_framebuffer_fill(struct framebuffer *fb) {
    struct dumb_framebuffer *dumb_fb = NULL;
    uint32_t *pix;

    if (!fb) return;

    dumb_fb = container_of(fb, struct dumb_framebuffer, fb);

    for (unsigned int y = 0; y < fb->height; y++) {
        pix = (uint32_t *)((uint8_t *)dumb_fb->mem + (y * fb->pitches[0]));

        for (unsigned int x = 0; x < fb->width; x++) {
            *pix++ = (0xff << 24) | (r << 16) | (g << 8) | b;
        }
    }

    g += 1;
    if (g > 0xff) g = 0x70;
}

static void dumb_framebuffer_destroy(struct framebuffer **fb) {
    struct dumb_framebuffer *dumb_fb;
    struct drm_mode_destroy_dumb destroy;

    if (!fb || !*fb) return;

    dumb_fb = container_of(*fb, struct dumb_framebuffer, fb);

    if (dumb_fb->fb.impl)
        free(dumb_fb->fb.impl);

    if (dumb_fb->mem && dumb_fb->mem != MAP_FAILED)
        munmap(dumb_fb->mem, dumb_fb->size);

    if (dumb_fb->fb.gem_handles[0] > 0) {
        destroy = (struct drm_mode_destroy_dumb) {
            .handle = dumb_fb->fb.gem_handles[0]
        };
	    drmIoctl((*fb)->device->kms_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    }

    framebuffer_deinit(*fb);

    free(dumb_fb);
    *fb = NULL;
}

struct framebuffer *dumb_framebuffer_create(struct device *device, struct plane *plane,
                                            unsigned int width, unsigned int height)
{
    struct dumb_framebuffer *dumb_fb = NULL;
    struct framebuffer *fb = NULL;
    struct drm_mode_create_dumb create;
    struct drm_mode_map_dumb map;

    if (!device || !plane || width < 1 || height < 1) return NULL;

    dumb_fb = calloc(1, sizeof(struct dumb_framebuffer));
    if (!dumb_fb) return NULL;

    fb = &dumb_fb->fb;

    create = (struct drm_mode_create_dumb) {
        .width = width,
        .height = height,
        .bpp = 32,
    };

    if (drmIoctl(device->kms_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) != 0) {
        fprintf(stderr, "Failed to create %u x %u dumb buffer: %s\n",
                create.width, create.height, strerror(errno));
        dumb_framebuffer_destroy(&fb);

        return NULL;
    }

    assert(create.handle);
    assert(create.pitch >= create.width * create.bpp / 8);
    assert(create.size >= create.pitch * create.height);

    dumb_fb->fb.gem_handles[0] = create.handle;
    dumb_fb->fb.pitches[0] = create.pitch;

    map = (struct drm_mode_map_dumb) {
        .handle = create.handle,
    };

    if (drmIoctl(device->kms_fd, DRM_IOCTL_MODE_MAP_DUMB, &map) != 0) {
        fprintf(stderr, "Failed to get %u x %u dumb buffer map: %s\n",
                create.width, create.height, strerror(errno));
        dumb_framebuffer_destroy(&fb);

        return NULL;
    }

    dumb_fb->mem = mmap(NULL, create.size, PROT_WRITE, MAP_SHARED,
                        device->kms_fd, map.offset);
    dumb_fb->size = create.size;

    if (dumb_fb->mem == MAP_FAILED) {
        fprintf(stderr, "Failed to mmap %u x %u dumb buffer: %s\n",
                create.width, create.height, strerror(errno));
        dumb_framebuffer_destroy(&fb);

        return NULL;
    }

    dumb_fb->fb.impl = calloc(1, sizeof(struct framebuffer_impl));
    if (!dumb_fb->fb.impl) {
        dumb_framebuffer_destroy(&fb);
    }
    dumb_fb->fb.impl->fill = dumb_framebuffer_fill;
    dumb_fb->fb.impl->destroy = dumb_framebuffer_destroy;

    if (framebuffer_init(device, plane, create.width, create.height,
                         DRM_FORMAT_XRGB8888, DRM_FORMAT_MOD_LINEAR, &dumb_fb->fb) != 0)
    {
        fprintf(stderr, "Failed to init %u x %u dumb buffer\n",
                create.width, create.height);
        dumb_framebuffer_destroy(&fb);

        return NULL;
    }

    return &dumb_fb->fb;
}