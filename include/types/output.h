#ifndef _TYPES_OUTPUT_H_
#define _TYPES_OUTPUT_H_

#include "include/types/connector.h"
#include "include/utils/mode.h"

#include <stdbool.h>

struct output {
    struct device *device;

    bool needs_repaint;

    struct connector *connector;
};

int output_init(struct device *device, struct connector *connector, struct output *output);
int output_render(struct output *output);
int output_repaint(struct output *output);

void output_deinit(struct output *output);

int output_fill_array(struct device *device, struct dt_array *output_array);
void output_empty_array(struct dt_array *output_array);


#endif // _TYPES_OUTPUT_H_