#include "be.h"

Be_Operand be_evaluate_binary(Be_Instruction_Kind kind, Be_Operand lhs, Be_Operand rhs) {
    BE_TODO("evaluate binary instruction");
    BE_UNUSED(kind);
    BE_UNUSED(lhs);
    BE_UNUSED(rhs);
    return (Be_Operand){.kind = BE_OPERAND_ERROR};
}

Be_Operand be_evaluate(const Be_Function* function, Be_Operand* parameters) {
    BE_TODO("evaluate function call");
    BE_UNUSED(function);
    BE_UNUSED(parameters);
    return (Be_Operand){.kind = BE_OPERAND_ERROR};
}
