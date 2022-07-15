#ifndef _TYPES_FRAMEBUFFER_H_
#define _TYPES_FRAMEBUFFER_H_

#include "include/types/device.h"

#include <stdint.h>

struct plane;
struct framebuffer;

struct framebuffer_impl {
    void (*fill)(struct framebuffer *fb);
    void (*destroy)(struct framebuffer **fb);
};

struct framebuffer {
    struct device *device;
    struct plane *plane;

    struct framebuffer_impl *impl;

    uint32_t fb_id;

    unsigned int width;
    unsigned int height;

    uint32_t gem_handles[4];
    unsigned int pitches[4];
    unsigned int offsets[4];

    uint32_t format;
    uint64_t modifier;
};

int framebuffer_init(struct device *device, struct plane *plane,
                      unsigned int width,
                      unsigned int height,
                      uint32_t format,
                      uint64_t modifier,
                      struct framebuffer *fb);

void framebuffer_deinit(struct framebuffer *fb);


#endif // _TYPES_FRAMEBUFFER_H_