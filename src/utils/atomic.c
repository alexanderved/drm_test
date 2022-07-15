#include "include/utils/atomic.h"
#include "include/types/device.h"
#include "include/utils/util.h"
#include "include/types/output.h"

#include <stdio.h>
#include <poll.h>

static void atomic_page_flip_handler(UNUSED int fd,
                          UNUSED unsigned int sequence,
                          UNUSED unsigned int tv_sec,
                          UNUSED unsigned int tv_usec,
                          unsigned int crtc_id,
                          void *user_data)
{
    struct device *device = user_data;
    struct output *output = NULL;
    struct plane *primary_plane = NULL;

    dt_array_for_each(&device->output_array, struct output, tmp_output) {
        if (!tmp_output->connector || !tmp_output->connector->crtc) continue;

        if (tmp_output->connector->crtc->crtc_id == crtc_id) {
            output = tmp_output;
            break;
        }
    }

    if (!output) return;

    output->needs_repaint = true;

    primary_plane = output->connector->crtc->plane_set.primary;

    swapchain_release_framebuffer(&primary_plane->swapchain, &primary_plane->swapchain.presented_fb);
    primary_plane->plane_state.fb = swapchain_move_framebuffer(
            &primary_plane->swapchain, &primary_plane->swapchain.presented_fb,
            &primary_plane->swapchain.commited_fb);
}

int atomic_add(drmModeAtomicReqPtr req, uint32_t object_id,
               struct property_info *property, uint64_t value)
{
    if (!req || !property || property->property_id == 0) return 1;

    if (drmModeAtomicAddProperty(req, object_id, property->property_id, value) <= 0) {
        fprintf(stderr, "Couldn't atomic add property\n");

        return 1;
    }

    return 0;
}

int atomic_commit(struct device *device, drmModeAtomicReqPtr req, bool allow_modset) {
    uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_PAGE_FLIP_EVENT;
    int error = 0;

    if (!device) return 1;

    if (allow_modset)
        flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;

    error = drmModeAtomicCommit(device->kms_fd, req, flags, device);

    return error;
}

int atomic_event_handle(struct device *device) {
    struct pollfd poll_fd;
    drmEventContext evctx = {
		.version = 3,
		.page_flip_handler2 = atomic_page_flip_handler,
	};
    int error = 0;

    if (!device) return 1;

    poll_fd = (struct pollfd) {
        .fd = device->kms_fd,
        .events = POLLIN,
    };

    error = poll(&poll_fd, 1, 0);

    if (poll_fd.revents & POLLIN) {
        printf("Pollin' :)\t");

        error = drmHandleEvent(device->kms_fd, &evctx);
    } else {
        printf("No pollin' :(\t");
    }

    return error;
}