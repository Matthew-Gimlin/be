#include "be.h"
#include <stdio.h>
#include <inttypes.h>

static const char* be_dump_type(Be_Type type) {
    switch (type) {
        case BE_TYPE_VOID: return "void";
        case BE_TYPE_BYTE: return "b";
        case BE_TYPE_SHORT: return "s";
        case BE_TYPE_INT: return "i";
        case BE_TYPE_LONG: return "l";
        case BE_TYPE_FLOAT: return "f";
        case BE_TYPE_DOUBLE: return "d";
        default: return "?";
    }
}

static void be_dump_value(const Be_Function* function, int id) {
    if (id < 0 || id >= function->values.size) return;
    Be_Value* value = function->values.elements[id];
    printf("%.*s", value->name.len, value->name.str);
}

static void be_dump_operand(const Be_Function* function, const Be_Operand* operand) {
    if (!operand) return;
    switch (operand->kind) {
        case BE_OPERAND_VALUE:
            be_dump_value(function, operand->value_id);
            break;
        case BE_OPERAND_INT:
            printf("%" PRId64, operand->int_const);
            break;
        case BE_OPERAND_FLOAT:
            printf("%lf", operand->float_const);
            break;
        default:
            break;
    }
}

static void be_dump_instruction(const Be_Function* function, const Be_Instruction* instruction) {
    if (!instruction) return;
    printf("  ");
    if (instruction->result_id >= 0) {
        be_dump_value(function, instruction->result_id);
        printf(" = %s ", be_dump_type(instruction->type));
    }
    switch (instruction->kind) {
        case BE_INSTRUCTION_CONST:
            be_dump_operand(function, &instruction->constant.operand);
            break;
        case BE_INSTRUCTION_ADD:
            printf("add ");
            be_dump_operand(function, &instruction->binary.lhs);
            printf(", ");
            be_dump_operand(function, &instruction->binary.rhs);
            break;
        default:
            return;
    }
    printf("\n");
}

static void be_dump_terminator(const Be_Function* function, const Be_Terminator* terminator) {
    if (!terminator) return;
    switch (terminator->kind) {
        case BE_TERMINATOR_RET:
            printf("  ret %s ", be_dump_type(terminator->ret.type));
            be_dump_operand(function, &terminator->ret.operand);
            break;
        default:
            return;
    }
    printf("\n");
}

static void be_dump_block(const Be_Function* function, const Be_Block* block) {
    if (!block) return;
    printf("%.*s:\n", block->label.len, block->label.str);
    for (int i = 0; i < block->instructions.size; i++) {
        be_dump_instruction(function, block->instructions.elements[i]);
    }
    be_dump_terminator(function, &block->terminator);
}

static void be_dump_function(const Be_Function* function) {
    if (!function) return;
    printf("func %s %.*s() {\n", be_dump_type(function->type), function->name.len, function->name.str);
    for (int i = 0; i < function->blocks.size; i++) {
        be_dump_block(function, function->blocks.elements[i]);
        if (i < function->blocks.size - 1) printf("\n");
    }
    printf("}\n");
}

void be_dump(const Be_Module* module) {
    if (!module) return;
    for (int i = 0; i < module->functions.size; i++) {
        be_dump_function(module->functions.elements[i]);
        if (i < module->functions.size - 1) printf("\n");
    }
}
