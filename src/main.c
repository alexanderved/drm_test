#include "include/utils/util.h"
#include "include/types/device.h"
#include "include/types/output.h"
#include "include/utils/atomic.h"

#include "include/renderer/dumb/framebuffer.h"

#include <stdio.h>

int main(UNUSED int argc, UNUSED char *argv[]) {
    struct device *device;
    drmModeAtomicReqPtr req = NULL;
    bool allow_modeset = true;

    device = device_create();
    if (!device) {
        fprintf(stderr, "No usable KMS device was found\n");

        return 1;
    }

    while (1) {
    //for (int i = 0; i < 3; i++) {
        req = drmModeAtomicAlloc();

        dt_array_for_each(&device->output_array, struct output, output) {
            output_repaint(output, req);

            if (output && output->connector && output->connector->crtc) {
                struct swapchain *sc = &output->connector->crtc->plane_set.primary->swapchain;
                printf("%p %p %p\t", (void *)sc->fbs[0],
                       (void *)sc->fbs[1], (void *)sc->fbs[2]);
                printf("acquired %p, commited %p, presented %p\n", (void *)sc->acquired_fb,
                       (void *)sc->commited_fb, (void *)sc->presented_fb);
            }
        }

        atomic_commit(device, req, allow_modeset);
        drmModeAtomicFree(req);

        atomic_event_handle(device);

        if (allow_modeset) allow_modeset = false;
    }

    device_destroy(&device);

    return 0;
}
