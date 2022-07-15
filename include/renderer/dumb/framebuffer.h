#ifndef _RENDERER_DUMB_FRAMEBUFFER_H_
#define _RENDERER_DUMB_FRAMEBUFFER_H_

#include "include/types/framebuffer.h"

#include <stdlib.h>

struct dumb_framebuffer {
    struct framebuffer fb;

    uint32_t *mem;
    size_t size;
};

struct framebuffer *dumb_framebuffer_create(struct device *device, struct plane *plane,
                                            unsigned int width, unsigned int height);


#endif // _RENDERER_DUMB_FRAMEBUFFER_H_