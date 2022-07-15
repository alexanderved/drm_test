#include "include/types/framebuffer.h"

#include <stdio.h>
#include <string.h>

int framebuffer_init(struct device *device, struct plane *plane,
                      unsigned int width,
                      unsigned int height,
                      uint32_t format,
                      uint64_t modifier,
                      struct framebuffer *fb)
{
    uint64_t modifiers[4] = { 0 };
    int error = 0;

    if (!device || !plane) return 1;

    fb->device = device;
    fb->plane = plane;

    fb->width = width;
    fb->height = height;

    fb->format = format;
    fb->modifier = modifier;

    for (int i = 0; fb->gem_handles[i]; i++)
        modifiers[i] = fb->modifier;

    if (device->supports_fb_modifiers) {
        error = drmModeAddFB2WithModifiers(device->kms_fd, width, height, format,
                                           fb->gem_handles, fb->pitches, fb->offsets,
                                           modifiers, &fb->fb_id, DRM_MODE_FB_MODIFIERS);
    } else {
        error = drmModeAddFB2(device->kms_fd, width, height, format,
                              fb->gem_handles, fb->pitches, fb->offsets,
                              &fb->fb_id, 0);
    }

    return error;
}

void framebuffer_deinit(struct framebuffer *fb) {
    if (!fb) return;

    if (fb->fb_id != 0) {
        drmModeRmFB(fb->device->kms_fd, fb->fb_id);
        fb->fb_id = 0;
    }

    memset(fb, 0, sizeof(*fb));
}