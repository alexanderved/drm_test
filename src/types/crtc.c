#include "include/types/crtc.h"
#include "include/utils/util.h"
#include "include/utils/atomic.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const struct property_info crtc_properties[] = {
    [CRTC_ACTIVE] = { .name = "ACTIVE" },
    [CRTC_MODE_ID] = { .name = "MODE_ID" },
    [CRTC_OUT_FENCE_PTR] = { .name = "OUT_FENCE_PTR" },
};

int crtc_apply_state(struct crtc *crtc, drmModeAtomicReqPtr req) {
    struct crtc_state *crtc_state = &crtc->crtc_state;
    int error = 0;

    if (!crtc) return 1;

    error |= atomic_add(req, crtc->crtc_id,
                        &crtc->crtc_properties[CRTC_ACTIVE], crtc_state->active);
    error |= atomic_add(req, crtc->crtc_id,
                        &crtc->crtc_properties[CRTC_MODE_ID], crtc_state->mode->blob_id);

    if (crtc_state->out_fence_fd >= 0) {
        error |= atomic_add(req, crtc->crtc_id,
                            &crtc->crtc_properties[CRTC_OUT_FENCE_PTR],
                            crtc_state->out_fence_fd);
    }

    return error;
}

int crtc_init(struct device *device, uint32_t crtc_id, int pipe, struct crtc *crtc) {
    drmModeCrtcPtr drm_crtc = NULL;
    drmModeObjectPropertiesPtr properties = NULL;

    if (!device || !crtc) return 1;

    drm_crtc = drmModeGetCrtc(device->kms_fd, crtc_id);
    assert(drm_crtc);

    crtc->device = device;

    crtc->crtc_id = crtc_id;
    crtc->pipe = pipe;
    
    dt_array_init(&crtc->plane_set.overlay, sizeof(struct plane *));
    
    properties = drmModeObjectGetProperties(device->kms_fd, crtc_id,
                                            DRM_MODE_OBJECT_CRTC);
    assert(properties);

    property_info_populate(device, crtc_properties, crtc->crtc_properties,
                           CRTC__PROP_COUNT, properties);

    crtc->crtc_state.active = property_get_value(&crtc->crtc_properties[CRTC_ACTIVE],
                                                 properties);
    crtc->crtc_state.out_fence_fd = (crtc->crtc_properties[CRTC_OUT_FENCE_PTR].property_id) ?
                                     FENCE_EMPTY                                            :
                                     FENCE_NOT_SUPPORTED;

    if (plane_set_fill(device, crtc, &crtc->plane_set) != 0) {
        fprintf(stderr, "Plane set wasn't filled\n\n");

        drmModeFreeObjectProperties(properties);
        drmModeFreeCrtc(drm_crtc);

        return 1;
    }

#if MESSAGE
    printf("CRTC planes: primary is %u, cursor is %u\n",
           crtc->plane_set.primary->plane_id,
           crtc->plane_set.cursor->plane_id);
#endif // MESSAGE

    drmModeFreeObjectProperties(properties);
    drmModeFreeCrtc(drm_crtc);
    
    return 0;
}

void crtc_deinit(struct crtc *crtc) {
    if (!crtc) return;

    plane_set_empty(&crtc->plane_set);

    property_info_destroy(crtc->crtc_properties, CRTC__PROP_COUNT);
    memset(crtc, 0, sizeof(*crtc));
}

int crtc_fill_array(struct device *device, struct dt_array *crtc_array) {
    struct crtc *crtc = NULL;

    if (!device || !crtc_array) return 1;

    if (crtc_array->element_size != sizeof(struct crtc)) {
        fprintf(stderr, "Array is not suitable for storing crtcs\n");

        return 1;
    }

    if (dt_array_is_allocated(crtc_array)) {
        fprintf(stderr, "CRTC array is already filled\n");

        return 1;
    }

    if (dt_array_reserve(crtc_array, device->resources->count_crtcs) != 0)
        return 1;

    for (int i = 0; i < device->resources->count_crtcs; i++) {
        crtc = dt_array_push(crtc_array);
        if (!crtc) continue;
        
        crtc_init(device, device->resources->crtcs[i], i, crtc);

#if MESSAGE
        printf("CRTC id: %d\n", crtc.crtc_id);
#endif //MESSAGE
    }

    return 0;
}

void crtc_empty_array(struct dt_array *crtc_array) {
    struct crtc *crtc;
    
    if (!crtc_array || !dt_array_is_allocated(crtc_array)) return;

    while (!dt_array_is_empty(crtc_array)) {
        crtc = dt_array_pop(crtc_array);
        if (!crtc) continue;

        crtc_deinit(crtc);
    }

    dt_array_free(crtc_array);
}