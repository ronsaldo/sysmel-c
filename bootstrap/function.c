#include "function.h"
#include "value.h"

sysmelb_Value_t sysmelb_interpretBytecodeFunction(sysmelb_function_t *function, size_t argumentCount, sysmelb_Value_t *arguments)
{
    sysmelb_Value_t result = {};
    result.type = sysmelb_getBasicTypes()->null;
    return result;
}
