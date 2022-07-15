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
                          UNUSED unsigned int crtc_id,
                          void *user_data)
{
    struct output *output = user_data;
    struct swapchain *swapchain = NULL;

    if (!output->connector || !output->connector->crtc) return;

    swapchain = &output->connector->crtc->plane_set.primary->swapchain;

    swapchain_release_framebuffer(swapchain, &swapchain->last_fb);
    swapchain_move_framebuffer(swapchain, &swapchain->last_fb, &swapchain->pending_fb);
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

int atomic_commit(struct output *output, drmModeAtomicReqPtr req) {
    uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_PAGE_FLIP_EVENT;
    int error = 0;
    struct swapchain *swapchain = NULL;
    struct plane_set *plane_set = NULL;

    if (!output) return 1;

    error = drmModeAtomicCommit(output->device->kms_fd, req,
                                DRM_MODE_ATOMIC_ALLOW_MODESET | flags, output);

    if (!output->connector || !output->connector->crtc) return 1;

    swapchain = &output->connector->crtc->plane_set.primary->swapchain;
    plane_set = &output->connector->crtc->plane_set;
    swapchain_move_framebuffer(swapchain, &swapchain->pending_fb,
                               plane_set->primary->plane_state.fb);

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

    if (poll_fd.revents & POLLIN)
        error = drmHandleEvent(device->kms_fd, &evctx);

    return error;
}