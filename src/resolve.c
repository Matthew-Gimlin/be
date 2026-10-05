#include "be.h"
#include <setjmp.h>
#include <string.h>

typedef struct {
    jmp_buf env;
    const char* filename;
} Be_Resolver;

static bool be_resolve_block_reference(const Be_Function* function, Be_Block_Reference* reference) {
    for (int i = 0; i < function->blocks.size; i++) {
        Be_Block* block = function->blocks.elements[i];
        if (block->label.len != reference->label.len) continue;
        if (strncmp(block->label.str, reference->label.str, block->label.len)) continue;
        reference->block_id = block->id;
        return true;
    }
    return false;
}

static void be_resolve_terminator(Be_Resolver* resolver, Be_Function* function, Be_Block* block) {
    switch (block->terminator.kind) {
        case BE_TERMINATOR_RET:
            if (block->terminator.ret.type != function->type) {
                be_error(
                    resolver->filename,
                    block->terminator.ret.line,
                    block->terminator.ret.column,
                    "terminator returns type `%s` but function is type `%s`",
                    be_type_symbol(block->terminator.ret.type),
                    be_type_symbol(function->type)
                );
                longjmp(resolver->env, 1);
            }
            break;
        case BE_TERMINATOR_BR:
            if (!be_resolve_block_reference(function, &block->terminator.br.block)) {
                be_error(
                    resolver->filename,
                    block->terminator.br.block.line,
                    block->terminator.br.block.column,
                    "undefined block `%.*s`",
                    block->terminator.br.block.label.len,
                    block->terminator.br.block.label.str
                );
                longjmp(resolver->env, 1);
            }
            break;
        case BE_TERMINATOR_CBR:
            if (!be_resolve_block_reference(function, &block->terminator.cbr.true_block)) {
                be_error(
                    resolver->filename,
                    block->terminator.cbr.true_block.line,
                    block->terminator.cbr.true_block.column,
                    "undefined block `%.*s`",
                    block->terminator.cbr.true_block.label.len,
                    block->terminator.cbr.true_block.label.str
                );
                longjmp(resolver->env, 1);
            }
            if (!be_resolve_block_reference(function, &block->terminator.cbr.false_block)) {
                be_error(
                    resolver->filename,
                    block->terminator.cbr.false_block.line,
                    block->terminator.cbr.false_block.column,
                    "undefined block `%.*s`",
                    block->terminator.cbr.false_block.label.len,
                    block->terminator.cbr.false_block.label.str
                );
                longjmp(resolver->env, 1);
            }
            break;
        default:
            break;
    }
}

static void be_resolve_block(Be_Resolver* resolver, Be_Function* function, Be_Block* block) {
    for (int i = 0; i < function->values.size; i++) {
        Be_Value* value = function->values.elements[i];
        if (!value->defined) {
            be_error(
                resolver->filename,
                value->line,
                value->column,
                "undefined value `%.*s`",
                value->name.len,
                value->name.str
            );
            longjmp(resolver->env, 1);
        }
    }
    be_resolve_terminator(resolver, function, block);
}

static void be_resolve_function(Be_Resolver* resolver, Be_Function* function) {
    for (int i = 0; i < function->blocks.size; i++) {
        be_resolve_block(resolver, function, function->blocks.elements[i]);
    }
}

bool be_resolve(Be_Module* module) {
    Be_Resolver resolver = {
        .filename = module->filename,
    };
    if (setjmp(resolver.env) == 0) {
        for (int i = 0; i < module->functions.size; i++) {
            be_resolve_function(&resolver, module->functions.elements[i]);
        }
        return true;
    } else return false;
}
