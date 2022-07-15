#include "include/utils/util.h"
#include "include/types/device.h"
#include "include/types/output.h"
#include "include/utils/atomic.h"

#include "include/renderer/dumb/framebuffer.h"

#include <stdio.h>

int main(UNUSED int argc, UNUSED char *argv[]) {
    struct device *device;

    device = device_create();
    if (!device) {
        fprintf(stderr, "No usable KMS device was found\n");

        return 1;
    }

    while (1) {
    //for (int i = 0; i < 3; i++) {
        dt_array_for_each(&device->output_array, struct output, output) {
            output_repaint(output);

            if (output && output->connector && output->connector->crtc) {
                struct swapchain *sc = &output->connector->crtc->plane_set.primary->swapchain;
                printf("%p %p %p\n", (void *)sc->fbs[0],
                       (void *)sc->fbs[1], (void *)sc->fbs[2]);
                printf("queued %p, pending %p, last %p\n", (void *)sc->queued_fb,
                       (void *)sc->pending_fb, (void *)sc->last_fb);
            }
        }

        atomic_event_handle(device);
    }

    device_destroy(&device);

    return 0;
}
