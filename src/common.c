#include "be.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

noreturn void be_fatal(const char* fmt, ...) {
    fprintf(stderr, "be: error: ");
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n");
    exit(1);
}

void be_error(const char* filename, int line, int column, const char* format, ...) {
    fprintf(stderr, "%s:%d:%d: error: ", filename, line, column);
    va_list va;
    va_start(va, format);
    vfprintf(stderr, format, va);
    va_end(va);
    fprintf(stderr, "\n");
}

void be_init_arena(Be_Arena* arena) { arena->head = arena->tail = NULL; }

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

static bool be_realloc(Be_Vector* vector) {
    void** new = be_arena_alloc(vector->arena, sizeof(void*) * vector->capacity, alignof(void*));
    if (!new) be_fatal("out of memory");
    memcpy(new, vector->elements, sizeof(void*) * vector->size);
    vector->elements = new;
    return true;
}

void be_init_vector(Be_Vector* vector, Be_Arena* arena) {
    vector->arena = arena;
    vector->size = 0;
    vector->capacity = BE_VECTOR_DEFAULT_CAPACITY;
    be_realloc(vector);
}

int be_vector_push(Be_Vector* vector, void* element) {
    if (vector->size >= vector->capacity) {
        vector->capacity *= 2;
        be_realloc(vector);
    }
    vector->elements[vector->size] = element;
    return vector->size++;
}

void be_vector_remove(Be_Vector* vector, int index) {
    if (index < 0 || index >= vector->size) return;
    for (int i = index; i < vector->size - 1; i++) {
        vector->elements[i] = vector->elements[i + 1];
    }
    vector->size--;
}

void be_vector_combine(Be_Vector* a, Be_Vector* b) {
    if (a->size + b->size >= a->capacity) {
        a->capacity += b->capacity;
        be_realloc(a);
    }
    memcpy(&a->elements[a->size], b->elements, b->size * sizeof(void*));
}

Be_Value* be_value(Be_Arena* arena) {
    Be_Value* value = be_arena_alloc(arena, sizeof(Be_Value), alignof(Be_Value));
    value->type = BE_TYPE_ERROR;
    value->id = -1;
    return value;
}

Be_Operand* be_operand(Be_Arena* arena) {
    Be_Operand* operand = be_arena_alloc(arena, sizeof(Be_Operand), alignof(Be_Operand));
    operand->kind = BE_OPERAND_ERROR;
    return operand;
}

Be_Instruction* be_instruction(Be_Arena* arena) {
    Be_Instruction* instruction = be_arena_alloc(arena, sizeof(Be_Instruction), alignof(Be_Instruction));
    instruction->kind = BE_INSTRUCTION_ERROR;
    instruction->type = BE_TYPE_ERROR;
    instruction->result_id = -1;
    return instruction;
}

Be_Terminator* be_terminator(Be_Arena* arena) {
    Be_Terminator* terminator = be_arena_alloc(arena, sizeof(Be_Terminator), alignof(Be_Terminator));
    terminator->kind = BE_TERMINATOR_ERROR;
    return terminator;
}

Be_Block* be_block(Be_Arena* arena) {
    Be_Block* block = be_arena_alloc(arena, sizeof(Be_Block), alignof(Be_Block));
    block->id = -1;
    be_init_vector(&block->instructions, arena);
    block->terminator.kind = BE_TERMINATOR_ERROR;
    be_init_vector(&block->predecessors, arena);
    be_init_vector(&block->successors, arena);
    return block;
}

Be_Function* be_function(Be_Arena* arena) {
    Be_Function* function = be_arena_alloc(arena, sizeof(Be_Function), alignof(Be_Function));
    function->type = BE_TYPE_ERROR;
    be_init_vector(&function->parameters, arena);
    be_init_vector(&function->values, arena);
    be_init_vector(&function->blocks, arena);
    return function;
}

Be_Module* be_module(Be_Arena* arena) {
    Be_Module* module = be_arena_alloc(arena, sizeof(Be_Module), alignof(Be_Module));
    be_init_vector(&module->functions, arena);
    return module;
}

Be_Value* be_get_value(Be_Function* function, int id) {
    for (int i = 0; i < function->values.size; i++) {
        Be_Value* value = function->values.elements[i];
        if (value->id == id) return value;
    }
    return NULL;
}

Be_Block* be_get_block(Be_Function* function, int id) {
    for (int i = 0; i < function->blocks.size; i++) {
        Be_Block* block = function->blocks.elements[i];
        if (block->id == id) return block;
    }
    return NULL;
}
