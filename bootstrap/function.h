#ifndef SYSMELB_FUNCTION_H
#define SYSMELB_FUNCTION_H

#pragma once

#include "symbol.h"
#include "source-code.h"
#include <stddef.h>

typedef struct sysmelb_Value_s sysmelb_Value_t;

typedef enum sysmelb_FunctionKind_e {
    SysmelFunctionKindPrimitive,
    SysmelFunctionKindPrimitiveMacro,
} sysmelb_FunctionKind_t;

typedef struct sysmelb_MacroContext_s {
    sysmelb_SourcePosition_t sourcePosition;
} sysmelb_MacroContext_t;

typedef sysmelb_Value_t (*sysmelb_PrimitiveFunction_t) (size_t argumentCount, sysmelb_Value_t *arguments);
typedef sysmelb_Value_t (*sysmelb_PrimitiveMacroFunction_t) (sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments);

typedef struct sysmelb_function_s {
    sysmelb_FunctionKind_t kind;
    sysmelb_symbol_t *name;

    union
    {
        sysmelb_PrimitiveFunction_t primitiveFunction;
        sysmelb_PrimitiveMacroFunction_t primitiveMacroFunction;
    };
} sysmelb_function_t;

#endif //SYSMELB_FUNCTION_H