#ifndef _UTILS_ATOMIC_H_
#define _UTILS_ATOMIC_H_

#include "include/utils/property.h"

#include <drm.h>
#include <drm_fourcc.h>
#include <drm_mode.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

int atomic_add(drmModeAtomicReqPtr req, uint32_t object_id,
               struct property_info *property, uint64_t value);
int atomic_commit(struct device *device, drmModeAtomicReqPtr req, bool allow_modset);
int atomic_event_handle(struct device *device);


#endif // _UTILS_ATOMIC_H_