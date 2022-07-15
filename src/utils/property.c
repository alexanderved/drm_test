#include "include/utils/property.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

uint64_t property_get_value(const struct property_info *info,
                            const drmModeObjectProperties *props)
{
    if (info->property_id == 0) return 0;

    for (uint32_t i = 0; i < props->count_props; i++) {
        if (props->props[i] != info->property_id) continue;

        if (info->count_enums == 0)
            return props->prop_values[i];

        for (unsigned int j = 0; j < info->count_enums; j++) {
            if (info->enums[j].value == props->prop_values[i])
                return j;
        }

        break;
    }

    return 0;
}

void property_info_populate(struct device *device,
                            const struct property_info *src_info,
                            struct property_info *dst_info,
                            unsigned int count_infos,
                            drmModeObjectPropertiesPtr props)
{
    drmModePropertyPtr prop;
    uint64_t j;

    for (unsigned int i = 0; i < count_infos; i++) {
        dst_info[i].name = src_info[i].name;
        dst_info[i].property_id = src_info[i].property_id;
        dst_info[i].count_enums = src_info[i].count_enums;

        if (dst_info[i].count_enums == 0) continue;

        dst_info[i].enums =
            calloc(dst_info[i].count_enums, sizeof(struct property_enum_info));
        assert(dst_info[i].enums);

        for (j = 0; j < dst_info[i].count_enums; j++) {
            dst_info[i].enums[j].name = src_info[i].enums[j].name;
        }
    }

    for (uint32_t i = 0; i < props->count_props; i++) {
        prop = drmModeGetProperty(device->kms_fd, props->props[i]);
        if (!prop) continue;

        for (j = 0; j < count_infos; j++) {
            if (strcmp(dst_info[j].name, prop->name) == 0) break;
        }

        // If j equals count_infos, we reached the cycle end
        // without finding suitable info for property.
        // It means that property is unknwn to us.
        if (j == count_infos) {
            drmModeFreeProperty(prop);
            continue;
        }

        dst_info[j].property_id = prop->prop_id;

        if (prop->flags & DRM_MODE_PROP_RANGE ||
            prop->flags & DRM_MODE_PROP_SIGNED_RANGE) {
            for (int k = 0; k < prop->count_values; k++)
                dst_info[j].range_values[k] = prop->values[k];
        }

        // Non-enum property can't contain any enums, and vice versa.
        assert(!!(prop->flags & DRM_MODE_PROP_ENUM) ==
               !!(dst_info[j].count_enums));

        // We don't have to fill enums if property is not enum.
        if (!(prop->flags & DRM_MODE_PROP_ENUM)) {
            drmModeFreeProperty(prop);
            continue;
        }

        for (unsigned int k = 0; k < dst_info[j].count_enums; k++) {
            for (int l = 0; l < prop->count_enums; l++) {
                if (strcmp(dst_info[j].enums[k].name,
                    prop->enums[l].name) == 0)
                    dst_info[j].enums[k].value = prop->enums[l].value;
            }
        }

        drmModeFreeProperty(prop);
    }
}

void property_info_destroy(struct property_info *info, unsigned int count_infos) {
    if (!info) return;

    for (unsigned int i = 0; i < count_infos; i++) {
        if (info[i].count_enums > 0) free(info[i].enums);
    }

    memset(info, 0, sizeof(*info) * count_infos);
}