#ifndef _TYPES_PLANE_H_
#define _TYPES_PLANE_H_

#include "include/utils/property.h"
#include "include/types/swapchain.h"

struct crtc;

enum plane_property {
    PLANE_TYPE = 0,
    PLANE_SRC_X,
    PLANE_SRC_Y,
    PLANE_SRC_W,
    PLANE_SRC_H,
    PLANE_CRTC_X,
    PLANE_CRTC_Y,
    PLANE_CRTC_W,
    PLANE_CRTC_H,
    PLANE_FB_ID,
    PLANE_CRTC_ID,
    PLANE_IN_FORMATS,
    PLANE_IN_FENCE_FD,
    PLANE__PROP_COUNT,
};

enum plane_type_enum {
    PLANE_TYPE_OVERLAY = 0,
    PLANE_TYPE_PRIMARY,
    PLANE_TYPE_CURSOR,
    PLANE_TYPE__COUNT,
};

struct plane_state {
    int32_t src_x, src_y;
    uint32_t src_w, src_h;

    int32_t crtc_x, crtc_y;
    uint32_t crtc_w, crtc_h;

    struct framebuffer **fb;
    int in_fence_fd;
};

struct plane {
    struct device *device;
    struct crtc *crtc;

    uint32_t plane_id;
    enum plane_type_enum type;
    struct plane_state plane_state;

    struct property_info plane_properties[PLANE__PROP_COUNT];

    struct swapchain swapchain;
};

struct plane_set {
    struct plane *primary;
    struct plane *cursor;
    struct dt_array overlay; // struct plane *
};

int plane_apply_state(struct plane *plane, drmModeAtomicReqPtr req);

int plane_init(struct device *device, uint32_t plane_id, struct plane *plane);
void plane_deinit(struct plane *plane);

int plane_fill_array(struct device *device, struct dt_array *plane_array);
void plane_empty_array(struct dt_array *plane_array);

int plane_set_fill(struct device *device, struct crtc *crtc, struct plane_set *plane_set);
void plane_set_empty(struct plane_set *plane_set);

#endif // _TYPES_PLANE_H_