#include "be.h"

static void be_edge(Be_Block* from, Be_Block* to) {
    be_vector_push(&from->successors, to);
    be_vector_push(&to->predecessors, from);
}

static void be_build_cfg_function(Be_Function* function) {
    for (int i = 0; i < function->blocks.size; i++) {
        Be_Block* block = function->blocks.elements[i];
        block->predecessors.size = 0;
        block->successors.size = 0;
    }
    for (int i = 0; i < function->blocks.size; i++) {
        Be_Block* block = function->blocks.elements[i];
        Be_Block* to = NULL;
        switch (block->terminator.kind) {
            case BE_TERMINATOR_BR:
                to = be_get_block(function, block->terminator.br.block.block_id);
                be_edge(block, to);
                break;
            case BE_TERMINATOR_CBR:
                to = be_get_block(function, block->terminator.cbr.true_block.block_id);
                be_edge(block, to);
                to = be_get_block(function, block->terminator.cbr.false_block.block_id);
                be_edge(block, to);
                break;
            default:
                break;
        }
    }
}

void be_build_cfg(Be_Module* module) {
    for (int i = 0; i < module->functions.size; i++) {
        be_build_cfg_function(module->functions.elements[i]);
    }
}

static bool be_remove_unreachable_function(Be_Function* function) {
    bool changed = false;
    for (int i = 1; i < function->blocks.size; i++) {
        Be_Block* block = function->blocks.elements[i];
        if (block->predecessors.size > 0) continue;
        if (block->successors.size > 0) continue;
        be_vector_remove(&function->blocks, i);
        i--;
        changed = true;
    }
    return changed;
}

bool be_remove_unreachable(Be_Module* module) {
    bool changed = false;
    for (int i = 0; i < module->functions.size; i++) {
        changed |= be_remove_unreachable_function(module->functions.elements[i]);
    }
    return changed;
}
