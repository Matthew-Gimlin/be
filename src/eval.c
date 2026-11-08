#include "be.h"
#include <setjmp.h>

typedef struct {
    jmp_buf env;
    Be_Arena* scratch;
    Be_Constant* constants;
} Be_Evaluator;

static Be_Constant be_evaluate_binary_int(Be_Instruction_Kind kind, const Be_Constant* lhs, const Be_Constant* rhs) {
    switch (kind) {
        case BE_INSTRUCTION_ADD:
            return (Be_Constant){.known = true, .type = BE_TYPE_INT, .int_const = lhs->int_const + rhs->int_const};
        case BE_INSTRUCTION_SUB:
            return (Be_Constant){.known = true, .type = BE_TYPE_INT, .int_const = lhs->int_const - rhs->int_const};
        case BE_INSTRUCTION_EQ:
            return (Be_Constant){.known = true, .type = BE_TYPE_INT, .int_const = lhs->int_const == rhs->int_const};
        default:
            return (Be_Constant){.known = false};
    }
}

static Be_Constant be_evaluate_binary_float(Be_Instruction_Kind kind, const Be_Constant* lhs, const Be_Constant* rhs) {
    switch (kind) {
        case BE_INSTRUCTION_ADD:
            return (Be_Constant){.known = true, .type = BE_TYPE_FLOAT, .float_const = lhs->float_const + rhs->float_const};
        case BE_INSTRUCTION_SUB:
            return (Be_Constant){.known = true, .type = BE_TYPE_FLOAT, .float_const = lhs->float_const - rhs->float_const};
        case BE_INSTRUCTION_EQ:
            return (Be_Constant){.known = true, .type = BE_TYPE_FLOAT, .float_const = lhs->float_const == rhs->float_const};
        default:
            return (Be_Constant){.known = false};
    }
}

Be_Constant be_evaluate_binary(Be_Instruction_Kind kind, const Be_Constant* lhs, const Be_Constant* rhs) {
    if (lhs->type != rhs->type) return (Be_Constant){.known = false};
    switch (lhs->type) {
        case BE_TYPE_BYTE:
        case BE_TYPE_SHORT:
        case BE_TYPE_INT:
        case BE_TYPE_LONG:
            return be_evaluate_binary_int(kind, lhs, rhs);
        case BE_TYPE_FLOAT:
        case BE_TYPE_DOUBLE:
            return be_evaluate_binary_float(kind, lhs, rhs);
        default:
            return (Be_Constant){.known = false};
    }
}

static void be_evaluate_instruction(Be_Evaluator* evaluator, const Be_Instruction* instruction) {
    Be_Constant* result = &evaluator->constants[instruction->result_id];
    Be_Constant lhs, rhs;
    switch (instruction->kind) {
        case BE_INSTRUCTION_CONST:
            *result = be_operand_to_constant(&instruction->constant.operand, evaluator->constants);
            break;
        case BE_INSTRUCTION_ADD:
        case BE_INSTRUCTION_SUB:
        case BE_INSTRUCTION_EQ:
            lhs = be_operand_to_constant(&instruction->binary.lhs, evaluator->constants);
            if (!lhs.known) longjmp(evaluator->env, 1);
            rhs = be_operand_to_constant(&instruction->binary.rhs, evaluator->constants);
            if (!rhs.known) longjmp(evaluator->env, 1);
            *result = be_evaluate_binary(instruction->kind, &lhs, &rhs);
            break;
        default:
            longjmp(evaluator->env, 1);
    }
}

static void be_evaluate_block(Be_Evaluator* evaluator, const Be_Block* block) {
    for (int i = 0; i < block->instructions.size; i++) {
        be_evaluate_instruction(evaluator, block->instructions.elements[i]);
    }
}

static Be_Constant be_evaluate_function(Be_Evaluator* evaluator, const Be_Function* function) {
    const Be_Block* block = function->blocks.elements[0];
    while (true) {
        be_evaluate_block(evaluator, block);
        Be_Terminator terminator = block->terminator;
        switch (terminator.kind) {
            case BE_TERMINATOR_RET:
                return be_operand_to_constant(&terminator.ret.operand, evaluator->constants);
            default:
                longjmp(evaluator->env, 1);
        }
    }
    longjmp(evaluator->env, 1);
}

Be_Constant be_evaluate(Be_Arena* scratch, const Be_Function* function, const Be_Vector* parameters) {
    Be_Constant* constants = be_arena_alloc(
        scratch,
        function->values.size * sizeof(Be_Constant),
        alignof(Be_Constant)
    );
    Be_Evaluator evaluator = {.scratch = scratch, .constants = constants};
    for (int i = 0; i < parameters->size; i++) {
        constants[i] = be_operand_to_constant(parameters->elements[i], constants);
        if (!constants[i].known) return (Be_Constant){.known = false};
    }
    if (setjmp(evaluator.env) == 0) return be_evaluate_function(&evaluator, function);
    else return (Be_Constant){.known = false};
}
