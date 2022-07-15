#ifndef _PROPERTY_H_
#define _PROPERTY_H_

#include "include/types/device.h"

#include <stdint.h>

#define COUNT_RANGE_VALUES 2

struct property_enum_info {
    const char *name;
    uint64_t value;
};

struct property_info {
    const char *name;
    uint32_t property_id;

    uint64_t range_values[COUNT_RANGE_VALUES];

    unsigned int count_enums;
    struct property_enum_info *enums;
};

uint64_t property_get_value(const struct property_info *info,
                            const drmModeObjectProperties *props);

void property_info_populate(struct device *device,
                            const struct property_info *src_info,
                            struct property_info *dst_info,
                            unsigned int count_infos,
                            drmModeObjectPropertiesPtr props);

void property_info_destroy(struct property_info *info, unsigned int count_infos);


#endif // _PROPERTY_H_