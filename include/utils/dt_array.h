#ifndef _UTILS_DT_ARRAY_H_
#define _UTILS_DT_ARRAY_H_

#include <stdbool.h>
#include <stddef.h>

struct dt_array {
    void *data;

    size_t length;
    size_t capacity;

    size_t element_size;
};

void dt_array_init(struct dt_array *array, const size_t element_size);

int dt_array_reserve(struct dt_array *array, const size_t capacity);
void *dt_array_push(struct dt_array *array);

void dt_array_free(struct dt_array *array);
void *dt_array_pop(struct dt_array *array);

static inline bool dt_array_is_initialized(const struct dt_array *array) {
    return (array->element_size > 0) ? true : false;
}

static inline bool dt_array_is_allocated(const struct dt_array *array) {
    return array->data ? true : false;
}

static inline bool dt_array_is_empty(const struct dt_array *array) {
    return (array->length == 0) ? true : false;
}

#define dt_array_get(dt_array, type, index) (&((type *)(dt_array)->data)[(index)])

#define dt_array_for_each(dt_array, type, element)                      \
    for (type *element = (dt_array)->data;                              \
         (element) < ((type *)(dt_array)->data + (dt_array)->length);   \
         (element)++)

#endif // _UTILS_DT_ARRAY_H_