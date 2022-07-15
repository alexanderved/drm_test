#include "include/types/plane.h"
#include "include/types/crtc.h"
#include "include/utils/util.h"
#include "include/utils/atomic.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct property_enum_info plane_type_enums[] = {
    [PLANE_TYPE_OVERLAY] = { .name = "Overlay" },
    [PLANE_TYPE_PRIMARY] = { .name = "Primary" },
    [PLANE_TYPE_CURSOR] = { .name = "Cursor" },
};

const struct property_info plane_properties[] = {
    [PLANE_TYPE] = {
        .name = "type",
        .enums = plane_type_enums,
        .count_enums = PLANE_TYPE__COUNT,
    },
    [PLANE_SRC_X] = { .name = "SRC_X" },
    [PLANE_SRC_Y] = { .name = "SRC_Y" },
    [PLANE_SRC_W] = { .name = "SRC_W" },
    [PLANE_SRC_H] = { .name = "SRC_H" },
    [PLANE_CRTC_X] = { .name = "CRTC_X" },
    [PLANE_CRTC_Y] = { .name = "CRTC_Y" },
    [PLANE_CRTC_W] = { .name = "CRTC_W" },
    [PLANE_CRTC_H] = { .name = "CRTC_H" },
    [PLANE_FB_ID] = { .name = "FB_ID" },
    [PLANE_CRTC_ID] = { .name = "CRTC_ID" },
    [PLANE_IN_FORMATS] = { .name = "IN_FORMATS" },
    [PLANE_IN_FENCE_FD] = { .name = "IN_FENCE_FD" },
};

int plane_apply_state(struct plane *plane, drmModeAtomicReqPtr req) {
    struct plane_state *plane_state = &plane->plane_state;
    int error = 0;

    if (!plane) return 1;

    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_CRTC_ID], plane->crtc->crtc_id);
    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_FB_ID], (*plane_state->fb)->fb_id);
                    
    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_SRC_X], plane_state->src_x);
    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_SRC_Y], plane_state->src_y);
    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_SRC_W], plane_state->src_w);
    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_SRC_H], plane_state->src_h);

    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_CRTC_X], plane_state->crtc_x);
    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_CRTC_Y], plane_state->crtc_y);
    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_CRTC_W], plane_state->crtc_w);
    error |= atomic_add(req, plane->plane_id,
                      &plane->plane_properties[PLANE_CRTC_H], plane_state->crtc_h);

    if (plane_state->in_fence_fd >= 0) {
        error |= atomic_add(req, plane->plane_id,
                          &plane->plane_properties[PLANE_IN_FENCE_FD], plane_state->in_fence_fd);
    }

    return error;
}

int plane_init(struct device *device, uint32_t plane_id, struct plane *plane) {
    drmModePlanePtr drm_plane = NULL;
    drmModeObjectPropertiesPtr properties = NULL;

    if (!device || !plane) return 1;

    drm_plane = drmModeGetPlane(device->kms_fd, plane_id);
    assert(drm_plane);

    plane->device = device;
    plane->plane_id = plane_id;

    properties = drmModeObjectGetProperties(device->kms_fd, plane_id,
                                            DRM_MODE_OBJECT_PLANE);
    assert(properties);

    property_info_populate(device, plane_properties, plane->plane_properties,
                           PLANE__PROP_COUNT, properties);
    plane->type = property_get_value(&plane->plane_properties[PLANE_TYPE], properties);

    plane->plane_state.in_fence_fd = (plane->plane_properties[PLANE_IN_FENCE_FD].property_id) ?
                                     FENCE_EMPTY                                              :
                                     FENCE_NOT_SUPPORTED;

    drmModeFreeObjectProperties(properties);
    drmModeFreePlane(drm_plane);

    return 0;
}

void plane_deinit(struct plane *plane) {
    if (!plane) return;

    property_info_destroy(plane->plane_properties, PLANE__PROP_COUNT);
    memset(plane, 0, sizeof(*plane));
}

int plane_fill_array(struct device *device, struct dt_array *plane_array) {
    drmModePlaneResPtr plane_resources = NULL;
    struct plane *plane = NULL;

    if (!device || !plane_array) return 1;

    if (plane_array->element_size != sizeof(struct plane)) {
        fprintf(stderr, "Array is not suitable for storing planes\n");

        return 1;
    }

    if (dt_array_is_allocated(plane_array)) {
        fprintf(stderr, "Plane array is already filled\n");

        return 1;
    }

    plane_resources = drmModeGetPlaneResources(device->kms_fd);
    if (!plane_resources) {
        fprintf(stderr, "Couldn't get plane resources\n");

        return 1;
    }

    if (dt_array_reserve(plane_array, plane_resources->count_planes) != 0)
        return 1;

    for (uint32_t i = 0; i < plane_resources->count_planes; i++) {
        plane = dt_array_push(plane_array);
        if (!plane) continue;
        
        plane_init(device, plane_resources->planes[i], plane);

#if MESSAGE
        printf("Plane id: %d\n", plane.plane_id);
#endif
    }

    drmModeFreePlaneResources(plane_resources);

    return 0;
}

void plane_empty_array(struct dt_array *plane_array) {
    struct plane *plane = NULL;

    if (!plane_array || !dt_array_is_allocated(plane_array)) return;

    while (!dt_array_is_empty(plane_array)) {
        plane = dt_array_pop(plane_array);
        if (!plane) continue;

        plane_deinit(plane);
    }

    dt_array_free(plane_array);
}

int plane_set_fill(struct device *device, struct crtc *crtc, struct plane_set *plane_set) {
    drmModePlanePtr drm_plane = NULL;
    struct plane **overlay = NULL;

    if (!device || !crtc || !plane_set || crtc->pipe > 32) return 1;

    if (!dt_array_is_initialized(&device->plane_array) ||
        !dt_array_is_allocated(&device->plane_array)) 
    {
        fprintf(stderr, "Plane array hasn't been initialized or allocated yet\n");

        return 1;
    }

    dt_array_for_each(&device->plane_array, struct plane, plane) {
        drm_plane = drmModeGetPlane(device->kms_fd, plane->plane_id);
        if (!(drm_plane->possible_crtcs & (1 << crtc->pipe))) {
            drmModeFreePlane(drm_plane);
            continue;
        }

        switch (plane->type) {
        case PLANE_TYPE_PRIMARY:
            if (!plane_set->primary)
                plane_set->primary = plane;
            break;
        case PLANE_TYPE_CURSOR:
            if (!plane_set->cursor)
                plane_set->cursor = plane;
            break;
        case PLANE_TYPE_OVERLAY:
            overlay = dt_array_push(&plane_set->overlay);
            *overlay = plane;
            break;
        default:
            break;
        }
        plane->crtc = crtc;

        drmModeFreePlane(drm_plane);
    }

    if (!plane_set->primary) {
        plane_set_empty(plane_set);

        return 1;
    }

    return 0;
}

void plane_set_empty(struct plane_set *plane_set) {
    if (!plane_set) return;

    plane_set->primary = NULL;
    plane_set->cursor = NULL;
    dt_array_free(&plane_set->overlay);
}