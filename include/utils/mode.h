#ifndef _UTILS_MODE_H_
#define _UTILS_MODE_H_

#include "include/types/device.h"

struct mode {
    struct device *device;

    drmModeModeInfo mode_info;
    uint32_t blob_id;
};

int mode_init(struct device *device, drmModeModeInfoPtr mode_info, struct mode *mode);
struct mode *mode_find(drmModeModeInfoPtr mode_info, struct dt_array *mode_array);

void mode_deinit(struct mode *mode);




#endif // _UTILS_MODE_H_