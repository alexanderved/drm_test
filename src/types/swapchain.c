#include "include/types/swapchain.h"
#include "include/renderer/dumb/framebuffer.h"
#include "include/utils/util.h"

#include <stdio.h>
#include <string.h>

int swapchain_init(struct device *device, struct plane *plane,
                   unsigned int width, unsigned int height,
                   enum renderer_type renderer_type, struct swapchain *swapchain)
{
    if (!device || !plane || !swapchain || width < 1 || height < 1) return 1;

    swapchain->device = device;
    swapchain->plane = plane;

    for (int i = 0; i < COUNT_FRAMEBUFFERS; i++) {
        switch (renderer_type) {
        case RENDERER_TYPE_DUMB:
            swapchain->fbs[i] = dumb_framebuffer_create(device, plane, width, height);
            break;
        default:
            break;
        }

        if (!swapchain->fbs[i]) {
            swapchain_deinit(swapchain);

            return 1;
        }
    }

    return 0;
}

struct framebuffer **swapchain_acquire_framebuffer(struct swapchain *swapchain) {
    if (!swapchain) return NULL;

    for (int i = 0; i < COUNT_FRAMEBUFFERS; i++) {
        if (swapchain->fbs[i]) {
            return swapchain_move_framebuffer(swapchain, &swapchain->acquired_fb,
                                              &swapchain->fbs[i]);
        }
    }

    return NULL;
}

struct framebuffer **swapchain_move_framebuffer(struct swapchain *swapchain,
                                                struct framebuffer **dst_fb,
                                                struct framebuffer **src_fb)
{
    if (!dst_fb || !src_fb) return NULL;

    if (*dst_fb)
        swapchain_release_framebuffer(swapchain, dst_fb);

    *dst_fb = *src_fb;
    *src_fb = NULL;

    return dst_fb;
}

void swapchain_deinit(struct swapchain *swapchain) {
    if (!swapchain) return;

    for (int i = 0; i < COUNT_FRAMEBUFFERS; i++) {
        if (swapchain->fbs[i])
            swapchain->fbs[i]->impl->destroy(&swapchain->fbs[i]);
    }

    if (swapchain->acquired_fb)
        swapchain->acquired_fb->impl->destroy(&swapchain->acquired_fb);
    if (swapchain->commited_fb)
        swapchain->commited_fb->impl->destroy(&swapchain->commited_fb);
    if (swapchain->presented_fb)
        swapchain->presented_fb->impl->destroy(&swapchain->presented_fb);

    memset(swapchain, 0, sizeof(*swapchain));
}

void swapchain_release_framebuffer(struct swapchain *swapchain, struct framebuffer **fb) {
    if (!swapchain || !fb || !*fb) return;

    if (swapchain->plane != (*fb)->plane) {
        fprintf(stderr, "Framebuffer is not suitable for this swapchain\n");

        return;
    }

    for (int i = 0; i < COUNT_FRAMEBUFFERS; i++) {
        if (!swapchain->fbs[i]) {
            swapchain_move_framebuffer(swapchain, &swapchain->fbs[i], fb);
            return;
        }
    }
}