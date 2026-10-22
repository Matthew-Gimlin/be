#include "be.h"
#include <string.h>

typedef struct {
    bool known;
    union {
        int64_t int_const;
        double float_const;
    };
} Be_Constant;

typedef struct {
    Be_Module* module;
    Be_Arena scratch;
    Be_Constant* constants;
} Be_Folder;

static bool be_is_known(const Be_Folder* folder, const Be_Operand* operand) {
    switch (operand->kind) {
        case BE_OPERAND_VALUE:
            return folder->constants[operand->value_id].known;
        case BE_OPERAND_INT:
        case BE_OPERAND_FLOAT:
            return true;
        default:
            return false;
    }
}

static int64_t be_get_constant(const Be_Folder* folder, const Be_Operand* operand) {
    switch (operand->kind) {
        case BE_OPERAND_VALUE:
            return folder->constants[operand->value_id].int_const;
        case BE_OPERAND_INT:
        case BE_OPERAND_FLOAT:
            return operand->int_const;
        default:
            return 0;
    }
}

static int64_t be_binary_operation(Be_Instruction_Kind kind, uint64_t lhs, uint64_t rhs) {
    switch (kind) {
        case BE_INSTRUCTION_ADD: return lhs + rhs;
        case BE_INSTRUCTION_SUB: return lhs - rhs;
        case BE_INSTRUCTION_EQ: return lhs == rhs;
        default: return 0;
    }
}

static bool be_fold_instruction(Be_Folder* folder, Be_Instruction* instruction) {
    int id = instruction->result_id;
    switch (instruction->kind) {
        case BE_INSTRUCTION_CONST:
            if (be_is_known(folder, &instruction->constant.operand)) {
                folder->constants[id] = (Be_Constant){
                    .known = true,
                    .int_const = instruction->constant.operand.int_const,
                };
            } else folder->constants[id].known = false;
            return false;
        case BE_INSTRUCTION_ADD:
        case BE_INSTRUCTION_SUB:
        case BE_INSTRUCTION_EQ:
            if (be_is_known(folder, &instruction->binary.lhs) && be_is_known(folder, &instruction->binary.rhs)) {
                instruction->constant.operand = (Be_Operand){
                    .kind = BE_OPERAND_INT,
                    .int_const = be_binary_operation(
                        instruction->kind,
                        be_get_constant(folder, &instruction->binary.lhs),
                        be_get_constant(folder, &instruction->binary.rhs)
                    ),
                };
                instruction->kind = BE_INSTRUCTION_CONST;
                folder->constants[id] = (Be_Constant) {
                    .known = true,
                    .int_const = instruction->constant.operand.int_const,
                };
                return true;
            } else folder->constants[id].known = false;
            return false;
        case BE_INSTRUCTION_CALL:
            for (int i = 0; i < instruction->call.parameters.size; i++) {
                Be_Operand* parameter = instruction->call.parameters.elements[i];
                if (be_is_known(folder, parameter)) {
                    *parameter = (Be_Operand){
                        .kind = BE_OPERAND_INT,
                        .int_const = be_get_constant(folder, parameter),
                    };
                }
            }
            if (instruction->call.function_id >= 0) {
                Be_Function* function = be_get_function(
                    folder->module,
                    instruction->call.function_id
                );
                if (!function) return false;
                Be_Operand result = be_evaluate(&folder->scratch, function, &instruction->call.parameters);
                BE_UNUSED(result);
            }
            return false;
        default:
            return false;
    }
}

static bool be_fold_terminator(Be_Folder* folder, Be_Terminator* terminator) {
    switch (terminator->kind) {
        case BE_TERMINATOR_RET:
            if (terminator->ret.operand.kind == BE_OPERAND_VALUE && be_is_known(folder, &terminator->ret.operand)) {
                terminator->ret.operand = (Be_Operand){
                    .kind = BE_OPERAND_INT,
                    .int_const = be_get_constant(folder, &terminator->ret.operand),
                };
                return true;
            }
            return false;
        case BE_TERMINATOR_CBR:
            if (terminator->cbr.condition.kind == BE_OPERAND_VALUE
                    && be_is_known(folder, &terminator->cbr.condition)) {
                terminator->kind = BE_TERMINATOR_BR;
                int64_t condition = be_get_constant(folder, &terminator->cbr.condition);
                if (condition) terminator->br.block = terminator->cbr.true_block;
                else terminator->br.block = terminator->cbr.false_block;
                return true;
            }
            return false;
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
