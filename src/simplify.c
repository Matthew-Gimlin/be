#include "be.h"
#include <string.h>

// DEBUG
#include <stdio.h>

static void be_track_operand(const Be_Operand* operand, int* usages) {
    switch (operand->kind) {
        case BE_OPERAND_VALUE:
            usages[operand->value_id]++;
            break;
        default:
            break;
    }
}

static void be_track_instruction(const Be_Instruction* instruction, int* usages) {
    switch (instruction->kind) {
        case BE_INSTRUCTION_CONST:
            be_track_operand(&instruction->constant.operand, usages);
            break;
        case BE_INSTRUCTION_ADD:
            be_track_operand(&instruction->binary.lhs, usages);
            be_track_operand(&instruction->binary.rhs, usages);
            break;
        default:
            break;
    }
}

static void be_track_terminator(const Be_Terminator* terminator, int* usages) {
    switch (terminator->kind) {
        case BE_TERMINATOR_RET:
            be_track_operand(&terminator->ret.operand, usages);
            break;
        case BE_TERMINATOR_CBR:
            be_track_operand(&terminator->cbr.condition, usages);
            break;
        default:
            break;
    }
}

static void be_track_block(const Be_Block* block, int* usages) {
    for (int i = 0; i < block->instructions.size; i++) {
        be_track_instruction(block->instructions.elements[i], usages);
    }
    be_track_terminator(&block->terminator, usages);
}

static void be_track_function(const Be_Function* function, int* usages) {
    for (int i = 0; i < function->blocks.size; i++) {
        be_track_block(function->blocks.elements[i], usages);
    }
}

static bool be_simplify_block(Be_Block* block, const int* usages) {
    bool changed = false;
    for (int i = 0; i < block->instructions.size; i++) {
        Be_Instruction* instruction = block->instructions.elements[i];
        switch (instruction->kind) {
            case BE_INSTRUCTION_CONST:
            case BE_INSTRUCTION_ADD:
                if (instruction->result_id >= 0 && usages[instruction->result_id] > 0) break;
                be_vector_remove(&block->instructions, i);
                i--;
                changed = true;
                break;
            default:
                break;
        }
    }
    return changed;
}

static bool be_simplify_function(Be_Function* function, const int* usages) {
    bool changed = false;
    for (int i = 0; i < function->blocks.size; i++) {
        changed |= be_simplify_block(function->blocks.elements[i], usages);
    }
    return changed;
}

bool be_simplify(Be_Module* module) {
    Be_Arena scratch;
    be_init_arena(&scratch);
    bool changed = false;
    for (int i = 0; i < module->functions.size; i++) {
        Be_Function* function = module->functions.elements[i];
        int* usages = be_arena_alloc(
            &scratch,
            function->values.size * sizeof(int),
            alignof(int)
        );
        memset(usages, 0, function->values.size * sizeof(int));
        be_track_function(function, usages);
        changed |= be_simplify_function(function, usages);
    }
    be_free_arena(&scratch);
    return changed;
}

static bool be_simplify_branches_function(Be_Function* function) {
    bool changed = false;
    for (int i = 0; i < function->blocks.size; i++) {
        Be_Block* block = function->blocks.elements[i];
        if (block->terminator.kind != BE_TERMINATOR_BR) continue;
        if (block->successors.size != 1) continue;
        Be_Block* successor = block->successors.elements[0];
        if (successor->predecessors.size != 1) continue;
        be_vector_combine(&block->instructions, &successor->instructions);
        block->terminator = successor->terminator;
        block->successors = successor->successors;
        changed = true;
    }
    return changed;
}

bool be_simplify_branches(Be_Module* module) {
    bool changed = false;
    for (int i = 0; i < module->functions.size; i++) {
        changed |= be_simplify_branches_function(module->functions.elements[i]);
    }
    return changed;
}
