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

static void be_dump_value(FILE* f, const Be_Function* function, int id) {
    if (id < 0 || id >= function->values.size) return;
    Be_Value* value = function->values.elements[id];
    fprintf(f, "%.*s", value->name.len, value->name.str);
}

static void be_dump_operand(FILE* f, const Be_Function* function, const Be_Operand* operand) {
    if (!operand) return;
    switch (operand->kind) {
        case BE_OPERAND_VALUE:
            be_dump_value(f, function, operand->value_id);
            break;
        case BE_OPERAND_INT:
            fprintf(f, "%" PRId64, operand->int_const);
            break;
        case BE_OPERAND_FLOAT:
            fprintf(f, "%lf", operand->float_const);
            break;
        default:
            break;
    }
}

static void be_dump_instruction(FILE* f, const Be_Function* function, const Be_Instruction* instruction) {
    if (!instruction) return;
    fprintf(f, "  ");
    if (instruction->result_id >= 0) {
        be_dump_value(f, function, instruction->result_id);
        fprintf(f, " = %s ", be_dump_type(instruction->type));
    }
    switch (instruction->kind) {
        case BE_INSTRUCTION_CONST:
            be_dump_operand(f, function, &instruction->constant.operand);
            break;
        case BE_INSTRUCTION_ADD:
            fprintf(f, "add ");
            goto binary;
        case BE_INSTRUCTION_EQ:
            fprintf(f, "eq ");
            goto binary;
        binary:
            be_dump_operand(f, function, &instruction->binary.lhs);
            fprintf(f, ", ");
            be_dump_operand(f, function, &instruction->binary.rhs);
            break;
        default:
            return;
    }
    fprintf(f, "\n");
}

static void be_dump_terminator(FILE* f, const Be_Function* function, const Be_Terminator* terminator) {
    if (!terminator) return;
    fprintf(f, "  ");
    switch (terminator->kind) {
        case BE_TERMINATOR_RET:
            fprintf(f, "ret %s ", be_dump_type(terminator->ret.type));
            be_dump_operand(f, function, &terminator->ret.operand);
            break;
        case BE_TERMINATOR_BR:
            fprintf(
                f, "br %.*s",
                terminator->br.block.label.len,
                terminator->br.block.label.str
            );
            break;
        case BE_TERMINATOR_CBR:
            printf("cbr %s ", be_dump_type(terminator->cbr.type));
            be_dump_operand(f, function, &terminator->cbr.condition);
            fprintf(
                f, ", %.*s, %.*s",
                terminator->cbr.true_block.label.len,
                terminator->cbr.true_block.label.str,
                terminator->cbr.false_block.label.len,
                terminator->cbr.false_block.label.str
            );
            break;
        default:
            fprintf(f, "; unknown terminator kind %d\n", terminator->kind);
            return;
    }
    fprintf(f, "\n");
}

static void be_dump_block(FILE* f, const Be_Function* function, const Be_Block* block) {
    if (!block) return;
    fprintf(f, "%.*s:\n", block->label.len, block->label.str);
    for (int i = 0; i < block->instructions.size; i++) {
        be_dump_instruction(f, function, block->instructions.elements[i]);
    }
    be_dump_terminator(f, function, &block->terminator);
}

static void be_dump_function(FILE* f, const Be_Function* function) {
    if (!function) return;
    fprintf(f, "func %s %.*s() {\n", be_dump_type(function->type), function->name.len, function->name.str);
    for (int i = 0; i < function->blocks.size; i++) {
        be_dump_block(f, function, function->blocks.elements[i]);
        if (i < function->blocks.size - 1) fprintf(f, "\n");
    }
    fprintf(f, "}\n");
}

void be_dump(FILE* f, const Be_Module* module) {
    if (!module) return;
    for (int i = 0; i < module->functions.size; i++) {
        be_dump_function(f, module->functions.elements[i]);
        if (i < module->functions.size - 1) fprintf(f, "\n");
    }
}
