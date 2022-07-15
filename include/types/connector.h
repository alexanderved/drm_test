#ifndef _TYPES_CONNECTOR_H_
#define _TYPES_CONNECTOR_H_

#include "include/utils/property.h"
#include "include/types/crtc.h"

struct output;

enum connector_dpms_state {
    CONNECTOR_DPMS_STATE_OFF = 0,
    CONNECTOR_DPMS_STATE_ON,
    CONNECTOR_DPMS_STATE_STANDBY,
    CONNECTOR_DPMS_STATE_SUSPEND,
    CONNECTOR_DPMS_STATE__COUNT,
};

enum connector_property {
    CONNECTOR_EDID = 0,
    CONNECTOR_DPMS,
    CONNECTOR_CRTC_ID,
    CONNECTOR_NON_DESKTOP,
    CONNECTOR__PROP_COUNT,
};

struct connector {
    struct device *device;
    struct output *output;

    uint32_t connector_id;
    struct property_info connector_properties[CONNECTOR__PROP_COUNT];

    struct dt_array mode_array; // struct mode

    struct crtc *crtc;
};

int connector_init(struct device *device, uint32_t connector_id,
                      struct connector *connector);
void connector_deinit(struct connector *connector);

int connector_fill_array(struct device *device, struct dt_array *connector_array);
void connector_empty_array(struct dt_array *connector_array);

#endif // _TYPES_CONNECTOR_H_