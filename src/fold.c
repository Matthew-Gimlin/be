#include "be.h"
#include <string.h>

typedef struct {
    bool known;
    union {
        int64_t int_const;
        double float_const;
    };
} Be_Constant;

static bool be_is_known(const Be_Operand* operand, const Be_Constant* constants) {
    switch (operand->kind) {
        case BE_OPERAND_VALUE:
            return constants[operand->value_id].known;
        case BE_OPERAND_INT:
        case BE_OPERAND_FLOAT:
            return true;
        default:
            return false;
    }
}

static int64_t be_get_constant(const Be_Operand* operand, const Be_Constant* constants) {
    switch (operand->kind) {
        case BE_OPERAND_VALUE:
            return constants[operand->value_id].int_const;
        case BE_OPERAND_INT:
        case BE_OPERAND_FLOAT:
            return operand->int_const;
        default:
            return 0;
    }
}

static bool be_fold_instruction(Be_Instruction* instruction, Be_Constant* constants) {
    int id = instruction->result_id;
    switch (instruction->kind) {
        case BE_INSTRUCTION_CONST:
            if (be_is_known(&instruction->constant.operand, constants)) {
                constants[id] = (Be_Constant){
                    .known = true,
                    .int_const = instruction->constant.operand.int_const,
                };
            } else constants[id].known = false;
            return false;
        case BE_INSTRUCTION_ADD:
            if (be_is_known(&instruction->binary.lhs, constants)
                    && be_is_known(&instruction->binary.rhs, constants)) {
                instruction->kind = BE_INSTRUCTION_CONST;
                instruction->constant.operand = (Be_Operand){
                    .kind = BE_OPERAND_INT,
                    .int_const = be_get_constant(&instruction->binary.lhs, constants)
                        + be_get_constant(&instruction->binary.rhs, constants),
                };
                constants[id] = (Be_Constant) {
                    .known = true,
                    .int_const = instruction->constant.operand.int_const,
                };
                return true;
            } else constants[id].known = false;
            return false;
        case BE_INSTRUCTION_EQ:
            if (be_is_known(&instruction->binary.lhs, constants)
                    && be_is_known(&instruction->binary.rhs, constants)) {
                instruction->kind = BE_INSTRUCTION_CONST;
                instruction->constant.operand = (Be_Operand){
                    .kind = BE_OPERAND_INT,
                    .int_const = be_get_constant(&instruction->binary.lhs, constants)
                        == be_get_constant(&instruction->binary.rhs, constants),
                };
                constants[id] = (Be_Constant) {
                    .known = true,
                    .int_const = instruction->constant.operand.int_const,
                };
                return true;
            } else constants[id].known = false;
            return false;
        default:
            return false;
    }
}

static bool be_fold_terminator(Be_Terminator* terminator, Be_Constant* constants) {
    switch (terminator->kind) {
        case BE_TERMINATOR_RET:
            if (terminator->ret.operand.kind == BE_OPERAND_VALUE && be_is_known(&terminator->ret.operand, constants)) {
                terminator->ret.operand = (Be_Operand){
                    .kind = BE_OPERAND_INT,
                    .int_const = be_get_constant(&terminator->ret.operand, constants),
                };
                return true;
            }
            return false;
        case BE_TERMINATOR_CBR:
            if (terminator->cbr.condition.kind == BE_OPERAND_VALUE
                    && be_is_known(&terminator->cbr.condition, constants)) {
                terminator->kind = BE_TERMINATOR_BR;
                int64_t condition = be_get_constant(&terminator->cbr.condition, constants);
                if (condition) terminator->br.block = terminator->cbr.true_block;
                else terminator->br.block = terminator->cbr.false_block;
                return true;
            }
            return false;
        default:
            return false;
    }
}

static bool be_fold_block(Be_Block* block, Be_Constant* constants) {
    bool changed = false;
    for (int i = 0; i < block->instructions.size; i++) {
        changed |= be_fold_instruction(block->instructions.elements[i], constants);
    }
    changed |= be_fold_terminator(&block->terminator, constants);
    return changed;
}

static bool be_fold_function(Be_Function* function, Be_Constant* constants) {
    bool changed = false;
    for (int i = 0; i < function->blocks.size; i++) {
        memset(constants, 0, function->values.capacity * sizeof(Be_Constant));
        changed |= be_fold_block(function->blocks.elements[i], constants);
    }
    return changed;
}

bool be_fold(Be_Module* module) {
    Be_Arena scratch;
    be_init_arena(&scratch);
    bool changed = false;
    for (int i = 0; i < module->functions.size; i++) {
        Be_Function* function = module->functions.elements[i];
        Be_Constant* constants = be_arena_alloc(
            &scratch,
            function->values.capacity * sizeof(Be_Constant),
            alignof(Be_Constant)
        );
        changed |= be_fold_function(function, constants);
    }
    be_free_arena(&scratch);
    return changed;
}
