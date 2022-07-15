#include "include/utils/dt_array.h"

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/param.h>

void dt_array_init(struct dt_array *array, const size_t element_size) {
    if (!array) return;

    array->data = NULL;

    array->length = 0;
    array->capacity = 0;

    array->element_size = element_size;
}

int dt_array_reserve(struct dt_array *array, const size_t capacity) {
    void *tmp_data;

    if (!array || !dt_array_is_initialized(array) || capacity <= array->capacity)
        return 1;

    tmp_data = realloc(array->data, capacity * array->element_size);
    if (!tmp_data) {
        fprintf(stderr, "Couldn't allocate array\n");

        return 1;
    }

    array->data = tmp_data;
    array->capacity = capacity;

    return 0;
}

void *dt_array_push(struct dt_array *array) {
    void *element;

    if (!array || !dt_array_is_initialized(array)) return NULL;

    if (array->length >= array->capacity) {
        if (dt_array_reserve(array, MAX(array->capacity * 2, array->length + 1)) != 0)
            return NULL;
    }

    element = (void *)((char *)array->data + array->length * array->element_size);
    memset(element, 0, array->element_size);
    array->length++;

#if MESSAGE
    printf("After push array length is: %lu, capacity is: %lu\n",
           array->length, array->capacity);
#endif // MESSAGE

    return element;
}

void dt_array_free(struct dt_array *array) {
    if (!array) return;

    array->capacity = 0;
    array->length = 0;

    free(array->data);
    array->data = NULL;
}

void *dt_array_pop(struct dt_array *array) {
    if (!array || !dt_array_is_initialized(array) ||
        !dt_array_is_allocated(array) || array->length <= 0)
        return NULL;

    array->length--;

#if MESSAGE
    printf("After pop array length is: %lu, capacity is: %lu\n",
           array->length, array->capacity);
#endif // MESSAGE

    return (void *)((char *)array->data + array->length * array->element_size);
}