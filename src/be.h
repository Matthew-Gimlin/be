#ifndef BE_H
#define BE_H

#include <stdnoreturn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>

#define BE_UNUSED(x) ((void)x)
#define BE_TODO(msg) \
    do { \
        printf("be: %s:%d: todo: " msg "\n", __FILE__, __LINE__); \
        abort(); \
    } while (0)

void be_error(const char* filename, int line, int column, const char* format, ...);
noreturn void be_fatal(const char* format, ...);

typedef struct { const char* str; int len; } Be_String;

#define BE_ARENA_DEFAULT_CAPACITY 2048

typedef struct Be_Page Be_Page;
struct Be_Page {
    struct Be_Page* next;
    size_t size;
    size_t capacity;
    alignas(max_align_t) unsigned char data[];
};

typedef struct {
    Be_Page* head;
    Be_Page* tail;
} Be_Arena;

void be_init_arena(Be_Arena* arena);
void be_free_arena(Be_Arena* arena);
void* be_arena_alloc(Be_Arena* arena, size_t size, size_t alignment);
void* be_arena_alloc_file(Be_Arena* arena, const char* filename);

#define BE_VECTOR_DEFAULT_CAPACITY 16

typedef struct {
    Be_Arena* arena;
    void** elements;
    int size;
    int capacity;
} Be_Vector;

void be_init_vector(Be_Vector* vector, Be_Arena* arena);
int be_vector_push(Be_Vector* vector, void* element);
void be_vector_remove(Be_Vector* vector, int index);
void be_vector_combine(Be_Vector* a, Be_Vector* b);

typedef enum {
    BE_TYPE_ERROR,
    BE_TYPE_VOID,   // No type
    BE_TYPE_BYTE,   // 8-bit integer
    BE_TYPE_SHORT,  // 16-bit integer
    BE_TYPE_INT,    // 32-bit integer
    BE_TYPE_LONG,   // 64-bit integer
    BE_TYPE_FLOAT,  // 32-bit floating point
    BE_TYPE_DOUBLE, // 64-bit floating point
} Be_Type;

typedef struct {
    Be_String name;
    Be_Type type;
    int id;
} Be_Value;

typedef enum {
    BE_OPERAND_ERROR,
    BE_OPERAND_VALUE,
    BE_OPERAND_INT,
    BE_OPERAND_FLOAT,
} Be_Operand_Kind;

typedef struct {
    Be_Operand_Kind kind;
    union {
        int value_id;
        int64_t int_const;
        double float_const;
    };
} Be_Operand;

typedef enum {
    BE_INSTRUCTION_ERROR,
    BE_INSTRUCTION_CONST,
    BE_INSTRUCTION_ADD,
    BE_INSTRUCTION_SUB,
    BE_INSTRUCTION_EQ,
} Be_Instruction_Kind;

typedef struct {
    Be_Instruction_Kind kind;
    Be_Type type;
    int result_id;
    union {
        struct { Be_Operand operand; } constant;
        struct { Be_Operand lhs; Be_Operand rhs; } binary;
    };
} Be_Instruction;

typedef enum {
    BE_TERMINATOR_ERROR,
    BE_TERMINATOR_RET,
    BE_TERMINATOR_BR,
    BE_TERMINATOR_CBR,
} Be_Terminator_Kind;

typedef struct { Be_String label; int block_id; } Be_Block_Reference;

typedef struct {
    Be_Terminator_Kind kind;
    union {
        struct {
            Be_Type type;
            Be_Operand operand;
        } ret;
        struct {
            Be_Block_Reference block;
        } br;
        struct {
            Be_Type type;
            Be_Operand condition;
            Be_Block_Reference true_block;
            Be_Block_Reference false_block;
        } cbr;
    };
} Be_Terminator;

typedef struct {
    Be_String label;
    int id;
    Be_Vector instructions;
    Be_Terminator terminator;
    Be_Vector predecessors;
    Be_Vector successors;
} Be_Block;

typedef struct {
    Be_String name;
    Be_Type type;
    Be_Vector parameters;
    Be_Vector values;
    Be_Vector blocks;
} Be_Function;

typedef struct { Be_Vector functions; } Be_Module;

Be_Value* be_value(Be_Arena* arena);
Be_Operand* be_operand(Be_Arena* arena);
Be_Instruction* be_instruction(Be_Arena* arena);
Be_Terminator* be_terminator(Be_Arena* arena);
Be_Block* be_block(Be_Arena* arena);
Be_Function* be_function(Be_Arena* arena);
Be_Module* be_module(Be_Arena* arena);

Be_Value* be_get_value(Be_Function* function, int id);
Be_Block* be_get_block(Be_Function* function, int id);

Be_Module* be_parse(Be_Arena* arena, const char* filename, const char* source);
void be_dump(FILE* f, const Be_Module* module);
void be_fold(Be_Module* module);

void be_simplify_branches(Be_Module* module);
void be_simplify(Be_Module* module);

void be_build_cfg(Be_Module* module);
void be_remove_unreachable(Be_Module* module);

#endif
