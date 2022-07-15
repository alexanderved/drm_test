#ifndef _TYPES_SWAPCHAIN_H_
#define _TYPES_SWAPCHAIN_H_

#include "include/config.h"
#include "include/types/framebuffer.h"

enum renderer_type {
    RENDERER_TYPE_DUMB,
    RENDERER_TYPE__COUNT,
};

// Swapchain manages framebuffers
struct swapchain {
    struct device *device;
    struct plane *plane;

    struct framebuffer *fbs[COUNT_FRAMEBUFFERS];

    // Acquired from swapchain framebuffer that will be used for rendering.
    struct framebuffer *acquired_fb;
    // Framebuffer that has been already rendered, commited and will be shown.
    struct framebuffer *commited_fb;
    // Framebuffer that was shown last time.
    struct framebuffer *presented_fb;
};

int swapchain_init(struct device *device, struct plane *plane,
                   unsigned int width, unsigned int height,
                   enum renderer_type renderer_type, struct swapchain *swapchain);
struct framebuffer **swapchain_acquire_framebuffer(struct swapchain *swapchain);
struct framebuffer **swapchain_move_framebuffer(struct swapchain *swapchain,
                                                struct framebuffer **dst_fb,
                                                struct framebuffer **src_fb);

void swapchain_deinit(struct swapchain *swapchain);
void swapchain_release_framebuffer(struct swapchain *swapchain, struct framebuffer **fb);


#endif // _TYPES_SWAPCHAIN_H_