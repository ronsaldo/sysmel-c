#include "function.h"
#include "value.h"

typedef struct sysmelb_FunctionInstruction_s {
    sysmelb_FunctionOpcode_t opcode;
    union
    {
        uint16_t argumentIndex;
        sysmelb_Value_t literalValue;
    };
} sysmelb_FunctionInstruction_t;

sysmelb_Value_t sysmelb_interpretBytecodeFunction(sysmelb_function_t *function, size_t argumentCount, sysmelb_Value_t *arguments)
{
    sysmelb_Value_t result = {};
    result.type = sysmelb_getBasicTypes()->null;
    return result;
}
