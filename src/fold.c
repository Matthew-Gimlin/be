#include "be.h"
#include <string.h>

typedef struct {
    Be_Module* module;
    Be_Arena scratch;
    Be_Constant* constants;
} Be_Folder;

static bool be_fold_instruction(Be_Folder* folder, Be_Instruction* instruction) {
    Be_Constant* result = &folder->constants[instruction->result_id];
    Be_Constant lhs, rhs;
    switch (instruction->kind) {
        case BE_INSTRUCTION_CONST:
            *result = be_operand_to_constant(&instruction->constant.operand, folder->constants);
            return false;
        case BE_INSTRUCTION_ADD:
        case BE_INSTRUCTION_SUB:
        case BE_INSTRUCTION_EQ:
            lhs = be_operand_to_constant(&instruction->binary.lhs, folder->constants);
            if (!lhs.known) return false;
            rhs = be_operand_to_constant(&instruction->binary.rhs, folder->constants);
            if (!rhs.known) return false;
            *result = be_evaluate_binary(instruction->kind, &lhs, &rhs);
            if (!result->known) return false;
            *instruction = (Be_Instruction){
                .kind = BE_INSTRUCTION_CONST,
                .constant.operand = be_constant_to_operand(result),
            };
            return true;
        case BE_INSTRUCTION_CALL:
            for (int i = 0; i < instruction->call.parameters.size; i++) {
                Be_Operand* parameter = instruction->call.parameters.elements[i];
                Be_Constant c = be_operand_to_constant(parameter, folder->constants);
                if (c.known) *parameter = be_constant_to_operand(&c);
            }
            if (instruction->call.function_id >= 0) {
                Be_Function* function = be_get_function(folder->module, instruction->call.function_id);
                if (!function) return false;
                *result = be_evaluate(&folder->scratch, function, &instruction->call.parameters);
                if (!result->known) return false;
                *instruction = (Be_Instruction){
                    .kind = BE_INSTRUCTION_CONST,
                    .constant.operand = be_constant_to_operand(result),
                };
            }
            return false;
        default:
            return false;
    }
}

static bool be_fold_terminator(Be_Folder* folder, Be_Terminator* terminator) {
    Be_Constant value = {0};
    switch (terminator->kind) {
        case BE_TERMINATOR_RET:
            if (terminator->ret.operand.kind != BE_OPERAND_VALUE) return false;
            value = be_operand_to_constant(&terminator->ret.operand, folder->constants);
            if (!value.known) return false;
            terminator->ret.operand = be_constant_to_operand(&value);
            return true;
        case BE_TERMINATOR_CBR:
            if (terminator->cbr.condition.kind != BE_OPERAND_VALUE) return false;
            value = be_operand_to_constant(&terminator->cbr.condition, folder->constants);
            if (!value.known) return false;
            terminator->kind = BE_TERMINATOR_BR;
            if (value.int_const) terminator->br.block = terminator->cbr.true_block;
            else terminator->br.block = terminator->cbr.false_block;
            return true;
        default:
            return false;
    }
}

static bool be_fold_block(Be_Folder* folder, Be_Block* block) {
    bool changed = false;
    for (int i = 0; i < block->instructions.size; i++) {
        changed |= be_fold_instruction(folder, block->instructions.elements[i]);
    }
    changed |= be_fold_terminator(folder, &block->terminator);
    return changed;
}

static bool be_fold_function(Be_Folder* folder, Be_Function* function) {
    bool changed = false;
    for (int i = 0; i < function->blocks.size; i++) {
        memset(folder->constants, 0, function->values.capacity * sizeof(Be_Constant));
        changed |= be_fold_block(folder, function->blocks.elements[i]);
    }
    return changed;
}

bool be_fold(Be_Module* module) {
    Be_Folder folder = {.module = module};
    be_init_arena(&folder.scratch);
    bool changed = false;
    for (int i = 0; i < module->functions.size; i++) {
        Be_Function* function = module->functions.elements[i];
        folder.constants = be_arena_alloc(
            &folder.scratch,
            function->values.capacity * sizeof(Be_Constant),
            alignof(Be_Constant)
        );
        changed |= be_fold_function(&folder, function);
    }
    be_free_arena(&folder.scratch);
    return changed;
}
