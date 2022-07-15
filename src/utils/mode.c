#include "include/utils/mode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int mode_init(struct device *device, drmModeModeInfoPtr mode_info, struct mode *mode) {
    if (!device || !mode_info || !mode) return 1;

    mode->device = device;
    memcpy(&mode->mode_info, mode_info, sizeof(*mode_info));

    if (drmModeCreatePropertyBlob(device->kms_fd, &mode->mode_info,
                                  sizeof(mode->mode_info), &mode->blob_id) < 0)
    {
        fprintf(stderr, "Couldn't create mode property blob: %s\n", strerror(errno));

        return errno;
    }

    return 0;
}

struct mode *mode_find(drmModeModeInfoPtr mode_info, struct dt_array *mode_array) {
    dt_array_for_each(mode_array, struct mode, mode) {
        if (memcmp(mode_info, &mode->mode_info, sizeof(*mode_info)) == 0)
            return mode;
    }

    return NULL;
}

void mode_deinit(struct mode *mode) {
    if (!mode) return;

    drmModeDestroyPropertyBlob(mode->device->kms_fd, mode->blob_id);
    memset(mode, 0, sizeof(*mode));
}