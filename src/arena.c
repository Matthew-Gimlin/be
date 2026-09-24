#include "be.h"
#include <stdalign.h>
#include <stdlib.h>
#include <stdio.h>

void be_init_arena(Be_Arena* arena) {
    arena->head = arena->tail = NULL;
}

void be_free_arena(Be_Arena* arena) {
    Be_Page* page = arena->head;
    while (page) {
        Be_Page* next = page->next;
        free(page);
        page = next;
    }
    arena->head = arena->tail = NULL;
}

static inline size_t be_max(size_t a, size_t b) { return a > b ? a : b; }

static Be_Page* be_page(size_t size) {
    size_t capacity = be_max(size, BE_ARENA_DEFAULT_CAPACITY);
    Be_Page* page = malloc(sizeof(Be_Page) + capacity);
    if (!page) be_fatal("out of memory");
    page->next = NULL;
    page->size = 0;
    page->capacity = capacity;
    return page;
}

static inline size_t be_align(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

void* be_arena_alloc(Be_Arena* arena, size_t size, size_t alignment) {
    if (!size) return NULL;
    if (!arena->tail) {
        arena->tail = be_page(size);
        arena->head = arena->tail;
    }
    Be_Page* page = arena->tail;
    size_t index = be_align(page->size, alignment);
    if (index + size >= page->capacity) {
        page->next = be_page(size);
        page = page->next;
        arena->tail = page;
        index = 0;
    }
    page->size = index + size;
    return &page->data[index];
}

void* be_arena_alloc_file(Be_Arena* arena, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long n = ftell(f);
    if (n < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    char* s = be_arena_alloc(arena, sizeof(char) * (n + 1), alignof(char));
    if (!s) {
        fclose(f);
        return NULL;
    }
    if (fread(s, sizeof(char), n, f) != (size_t)n) {
        fclose(f);
        return NULL;
    }
    s[n] = '\0';
    fclose(f);
    return s;
}
