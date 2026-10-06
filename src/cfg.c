#include "be.h"
#include <string.h>

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

static void be_track_reachable(const Be_Block* block, bool* reachable) {
    if (reachable[block->id]) return;
    reachable[block->id] = true;
    for (int i = 0; i < block->successors.size; i++) {
        be_track_reachable(block->successors.elements[i], reachable);
    }
}

static bool be_remove_unreachable_function(Be_Function* function, const bool* reachable) {
    bool changed = false;
    for (int i = 1; i < function->blocks.size; i++) {
        Be_Block* block = function->blocks.elements[i];
        if (reachable[block->id]) continue;
        be_vector_remove(&function->blocks, i);
        i--;
        changed = true;
    }
    return changed;
}

bool be_remove_unreachable(Be_Module* module) {
    Be_Arena scratch;
    be_init_arena(&scratch);
    bool changed = false;
    for (int i = 0; i < module->functions.size; i++) {
        Be_Function* function = module->functions.elements[i];
        bool* reachable = be_arena_alloc(&scratch, sizeof(bool), alignof(bool));
        memset(reachable, 0, function->blocks.capacity * sizeof(bool));
        be_track_reachable(function->blocks.elements[0], reachable);
        changed |= be_remove_unreachable_function(function, reachable);
    }
    be_free_arena(&scratch);
    return changed;
}
