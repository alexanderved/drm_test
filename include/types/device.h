#ifndef _TYPES_DEVICE_H_
#define _TYPES_DEVICE_H_

#include "include/config.h"
#include "include/utils/dt_array.h"

#include <drm.h>
#include <drm_fourcc.h>
#include <drm_mode.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct device {
    int kms_fd;

    int vt_fd;
    int saved_kb_mode;

    bool supports_fb_modifiers;
    uint64_t cursor_width;
    uint64_t cursor_height;

    drmModeResPtr resources;

    struct dt_array plane_array; // struct plane
    struct dt_array crtc_array; // struct crtc
    struct dt_array connector_array; // struct connector

    struct dt_array output_array; // struct output
};


struct device *device_create();
void device_destroy(struct device **device);

#endif // _TYPES_DEVICE_H_