#ifndef _TYPES_CRTC_H_
#define _TYPES_CRTC_H_

#include "include/utils/property.h"
#include "include/types/plane.h"
#include "include/utils/mode.h"

struct connector;

enum crtc_property {
    CRTC_ACTIVE = 0,
    CRTC_MODE_ID,
    CRTC_OUT_FENCE_PTR,
    CRTC__PROP_COUNT,
};

struct crtc_state {
    bool active;
    struct mode *mode;
    int out_fence_fd;
};

struct crtc {
    struct device *device;
    struct connector *connector;

    uint32_t crtc_id;
    int pipe;
    struct crtc_state crtc_state;

    struct property_info crtc_properties[CRTC__PROP_COUNT];

    struct plane_set plane_set;
};

int crtc_apply_state(struct crtc *crtc, drmModeAtomicReqPtr req);

int crtc_init(struct device *device, uint32_t crtc_id, int pipe, struct crtc *crtc);
void crtc_deinit(struct crtc *crtc);

int crtc_fill_array(struct device *device, struct dt_array *crtc_array);
void crtc_empty_array(struct dt_array *crtc_array);

#endif // _TYPES_CRTC_H_