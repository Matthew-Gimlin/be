#include "be.h"
#include <string.h>

static void be_realloc(Be_Vector* vector) {
    void** new = be_arena_alloc(vector->arena, sizeof(void*) * vector->capacity, alignof(void*));
    if (!new) be_fatal("out of memory");
    memcpy(new, vector->elements, sizeof(void*) * vector->size);
    vector->elements = new;
}

void be_init_vector(Be_Vector* vector, Be_Arena* arena) {
    vector->arena = arena;
    vector->size = 0;
    vector->capacity = BE_VECTOR_DEFAULT_CAPACITY;
    be_realloc(vector);
}

int be_vector_push(Be_Vector* vector, void* element) {
    if (vector->size + 1 >= vector->capacity) {
        vector->capacity *= 2;
        be_realloc(vector);
    }
    vector->elements[vector->size] = element;
    return vector->size++;
}
