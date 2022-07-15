#include "include/types/output.h"
#include "include/utils/atomic.h"

#include <string.h>
#include <stdio.h>

static int output_init_swapchains(struct output *output) {
    struct plane_set *plane_set = NULL;
    struct mode *mode = NULL;
    int error = 0;

    if (!output || !output->connector || !output->connector->crtc) return 1;

    plane_set = &output->connector->crtc->plane_set;
    mode = output->connector->crtc->crtc_state.mode;

    error = swapchain_init(output->device, plane_set->primary,
                           mode->mode_info.hdisplay, mode->mode_info.vdisplay,
                           RENDERER_TYPE_DUMB, &plane_set->primary->swapchain);
    if (plane_set->cursor)                     
        error |= swapchain_init(output->device, plane_set->cursor,
                                output->device->cursor_width, output->device->cursor_height,
                                RENDERER_TYPE_DUMB, &plane_set->cursor->swapchain);

    return error;
}

static void output_deinit_swapchains(struct output *output) {
    struct plane_set *plane_set = NULL;

    if (!output || !output->connector || !output->connector->crtc) return;

    plane_set = &output->connector->crtc->plane_set;
    swapchain_deinit(&plane_set->primary->swapchain);
    if (plane_set->cursor)
        swapchain_deinit(&plane_set->cursor->swapchain);
}

int output_init(struct device *device, struct connector *connector, struct output *output) {
    if (!device || !connector || !output || !connector->crtc) return 1;

    output->device = device;

    output->connector = connector;
    connector->output = output;
    output->needs_repaint = true;

    output_init_swapchains(output);

    return 0;
}

int output_render(struct output *output) {
    struct plane *primary_plane = NULL;
    struct mode *mode = NULL;
    struct framebuffer **fb = NULL;

    if (!output || !output->connector || !output->connector->crtc) return 1;

    primary_plane = output->connector->crtc->plane_set.primary;
    mode = output->connector->crtc->crtc_state.mode;

    fb = swapchain_acquire_framebuffer(&primary_plane->swapchain);
    if (!fb) {
        fprintf(stderr, "Couldn't find free framebuffer\n");

        return 1;
    }
    (*fb)->impl->fill(*fb);

    primary_plane->plane_state.fb = fb;

    primary_plane->plane_state.src_x = 0;
    primary_plane->plane_state.src_y = 0;
    primary_plane->plane_state.src_w = (*fb)->width << 16;
    primary_plane->plane_state.src_h = (*fb)->height << 16;

    primary_plane->plane_state.crtc_x = 0;
    primary_plane->plane_state.crtc_y = 0;
    primary_plane->plane_state.crtc_w = mode->mode_info.hdisplay;
    primary_plane->plane_state.crtc_h = mode->mode_info.vdisplay;

    return 0;
}

int output_repaint(struct output *output) {
    struct connector *connector = NULL;
    struct crtc *crtc = NULL;
    struct plane_set *plane_set = NULL;
    drmModeAtomicReqPtr req = NULL;
    int error = 0;

    if (!output || !output->connector || !output->connector->crtc) return 1;

    if (output_render(output) != 0) {
        fprintf(stderr, "Couldn't render output\n");

        return 1;
    }

    connector = output->connector;
    crtc = connector->crtc;
    plane_set = &crtc->plane_set;

    req = drmModeAtomicAlloc();

    error |= plane_apply_state(plane_set->primary, req);
    error |= crtc_apply_state(crtc, req);
    error |= atomic_add(req, connector->connector_id,
                        &connector->connector_properties[CONNECTOR_CRTC_ID],
                        connector->crtc->crtc_id);

    atomic_commit(output, req);


    drmModeAtomicFree(req);

    return error;
}

void output_deinit(struct output *output) {
    if (!output) return;

    output_deinit_swapchains(output);

    memset(output, 0, sizeof(*output));
}

int output_fill_array(struct device *device, struct dt_array *output_array) {
    struct output *output = NULL;

    if (!device || !output_array) return 1;

    if (output_array->element_size != sizeof(struct output)) {
        fprintf(stderr, "Array is not suitable for storing outputs\n");

        return 1;
    }

    if (dt_array_is_allocated(output_array)) {
        fprintf(stderr, "Output array is already filled\n");

        return 1;
    }

    if (dt_array_reserve(output_array, device->connector_array.length) != 0)
        return 1;

    dt_array_for_each(&device->connector_array, struct connector, connector) {
        output = dt_array_push(output_array);
        if (!output) continue;

        output_init(device, connector, output);

#if MESSAGE
        printf("Output connector id: %d\n", connector->connector_id);
#endif // MESSAGE
    }

    return 0;
}

void output_empty_array(struct dt_array *output_array) {
    struct output *output;

    if (!output_array || !dt_array_is_allocated(output_array)) return;

    while (!dt_array_is_empty(output_array)) {
        output = dt_array_pop(output_array);
        if (!output) continue;

        output_deinit(output);
    }

    dt_array_free(output_array);
}