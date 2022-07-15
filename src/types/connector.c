#include "include/types/connector.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct property_enum_info dpms_state_enums[] = {
    [CONNECTOR_DPMS_STATE_OFF] = { .name = "Off" },
    [CONNECTOR_DPMS_STATE_ON] = { .name = "On" },
    [CONNECTOR_DPMS_STATE_STANDBY] = { .name = "Standby" },
    [CONNECTOR_DPMS_STATE_SUSPEND] = { .name = "Suspend" },
};

const struct property_info connector_properties[] = {
    [CONNECTOR_EDID] = { .name = "EDID" },
    [CONNECTOR_DPMS] = {
        .name = "DPMS",
        .enums = dpms_state_enums,
        .count_enums = CONNECTOR_DPMS_STATE__COUNT,
    },
    [CONNECTOR_CRTC_ID] = { .name = "CRTC_ID" },
    [CONNECTOR_NON_DESKTOP] = { .name = "non-desktop" },
};

static int connector_populate_modes(struct connector *connector,
                                    drmModeConnectorPtr drm_connector)
{
    struct mode *mode = NULL;

    if (!connector || !drm_connector || !connector->device) return 1;

    dt_array_init(&connector->mode_array, sizeof(struct mode));
    if (dt_array_reserve(&connector->mode_array, drm_connector->count_modes) != 0)
        return 1;

    for (int i = 0; i < drm_connector->count_modes; i++) {
        mode = dt_array_push(&connector->mode_array);
        if (!mode) continue;

        mode_init(connector->device, &drm_connector->modes[i], mode);
    }

    return 0;
}

static int connector_pick_crtc(struct connector *connector,
                               drmModeConnectorPtr drm_connector)
{
    drmModeEncoderPtr drm_encoder = NULL;
    drmModeCrtcPtr drm_crtc = NULL;

    if (!connector || !drm_connector || !connector->device) return 1;

    drm_encoder = drmModeGetEncoder(connector->device->kms_fd, drm_connector->encoder_id);

    if (!drm_encoder) {
        fprintf(stderr, "warning: Couldn't find encoder for connector %u\n",
                connector->connector_id);

        return 1;
    }

    printf("Connector %u has encoder %u\n", connector->connector_id, drm_encoder->encoder_id);

    dt_array_for_each(&connector->device->crtc_array, struct crtc, crtc) {
        if (drm_encoder->crtc_id != crtc->crtc_id) continue;

        drm_crtc = drmModeGetCrtc(connector->device->kms_fd, crtc->crtc_id);
        if (!drm_crtc) return 1;
        if (!drm_crtc->mode_valid) {
            fprintf(stderr, "warning: CRTC %u has non-valid mode\n", crtc->crtc_id);
            drmModeFreeCrtc(drm_crtc);
            drmModeFreeEncoder(drm_encoder);

            return 1;
        }

        crtc->crtc_state.mode = mode_find(&drm_crtc->mode, &connector->mode_array);
        if (!crtc->crtc_state.mode) {
            fprintf(stderr, "warning: Couldn't find suitable mode for CRTC %u\n", crtc->crtc_id);
            drmModeFreeCrtc(drm_crtc);
            drmModeFreeEncoder(drm_encoder);

            return 1;
        }

        connector->crtc = crtc;
        crtc->connector = connector;

        drmModeFreeCrtc(drm_crtc);
        drmModeFreeEncoder(drm_encoder);

        return 0;
    }

    return 1;
}

int connector_init(struct device *device, uint32_t connector_id,
                    struct connector *connector)
{
    drmModeConnectorPtr drm_connector = NULL;
    drmModeObjectPropertiesPtr properties = NULL;

    if (!device || !connector) return 1;

    connector->device = device;
    connector->connector_id = connector_id;

    drm_connector = drmModeGetConnector(device->kms_fd, connector_id);
    if (!drm_connector) return 1;
    
    properties = drmModeObjectGetProperties(device->kms_fd, connector_id,
                                            DRM_MODE_OBJECT_CONNECTOR);
    assert(properties);

    property_info_populate(device, connector_properties, connector->connector_properties,
                           CONNECTOR__PROP_COUNT, properties);

    if (connector_populate_modes(connector, drm_connector) != 0) {
        fprintf(stderr, "Couldn't populate modes for the connector %u\n", connector_id);

        drmModeFreeObjectProperties(properties);
        drmModeFreeConnector(drm_connector);

        return 1;
    }

    if (connector_pick_crtc(connector, drm_connector) != 0)
        fprintf(stderr, "warning: Couldn't find suitable CRTC for connector %u\n", connector_id);

    drmModeFreeObjectProperties(properties);
    drmModeFreeConnector(drm_connector);

    return 0;
}

void connector_deinit(struct connector *connector) {
    struct mode *mode;

    if (!connector) return;

    while (!dt_array_is_empty(&connector->mode_array)) {
        mode = dt_array_pop(&connector->mode_array);
        if (!mode) continue;

        mode_deinit(mode);
    }

    dt_array_free(&connector->mode_array);

    property_info_destroy(connector->connector_properties, CONNECTOR__PROP_COUNT);
    memset(connector, 0, sizeof(*connector));
}

int connector_fill_array(struct device *device, struct dt_array *connector_array) {
    struct connector *connector = NULL;

    if (!device || !connector_array) return 1;

    if (connector_array->element_size != sizeof(struct connector)) {
        fprintf(stderr, "Array is not suitable for storing connectors\n");

        return 1;
    }

    if (dt_array_is_allocated(connector_array)) {
        fprintf(stderr, "Connector array is already filled\n");

        return 1;
    }

    if (dt_array_reserve(connector_array, device->resources->count_connectors) != 0)
        return 1;

    for (int i = 0; i < device->resources->count_connectors; i++) {
        connector = dt_array_push(connector_array);
        if (!connector) continue;
        
        connector_init(device, device->resources->connectors[i], connector);

#if MESSAGE
        printf("Connector id: %d\n", connector.connector_id);
#endif // MESSAGE
    }

    return 0;
}

void connector_empty_array(struct dt_array *connector_array) {
    struct connector *connector;

    if (!connector_array || !dt_array_is_allocated(connector_array)) return;

    while (!dt_array_is_empty(connector_array)) {
        connector = dt_array_pop(connector_array);
        if (!connector) continue;

        connector_deinit(connector);
    }

    dt_array_free(connector_array);
}