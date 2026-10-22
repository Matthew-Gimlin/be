#include "be.h"

typedef struct {
    bool known;
    union {
        uint64_t int_const;
    };
} Be_Constant;

Be_Operand be_evaluate_binary(Be_Instruction_Kind kind, Be_Operand lhs, Be_Operand rhs) {
    BE_TODO("evaluate binary instruction");
    BE_UNUSED(kind);
    BE_UNUSED(lhs);
    BE_UNUSED(rhs);
    return (Be_Operand){.kind = BE_OPERAND_ERROR};
}

Be_Operand be_evaluate(Be_Arena* scratch, const Be_Function* function, const Be_Vector* parameters) {
    Be_Constant* constants = be_arena_alloc(
        scratch,
        function->values.size * sizeof(Be_Constant),
        alignof(Be_Constant)
    );
    for (int i = 0; i < parameters->size; i++) {
        Be_Operand* parameter = parameters->elements[i];
        if (parameter->kind != BE_OPERAND_INT) return (Be_Operand){.kind = BE_OPERAND_ERROR};
        constants[i] = (Be_Constant){
            .known = true,
            .int_const = parameter->int_const,
        };
    }
    // TODO
    return (Be_Operand){.kind = BE_OPERAND_ERROR};
}
