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
    SysmelFunctionKindInterpreted,
    SysmelFunctionKindInterpretedMacro,
} sysmelb_FunctionKind_t;

typedef enum sysmelb_FunctionOpcode_e
{
    SysmelFunctionOpcodeNop,
    SysmelFunctionOpcodePushLiteral,
    SysmelFunctionOpcodePushArgument,
    SysmelFunctionOpcodePushCapture,
} sysmelb_FunctionOpcode_t;

typedef struct sysmelb_FunctionInstruction_s sysmelb_FunctionInstruction_t;

typedef struct sysmelb_MacroContext_s {
    sysmelb_SourcePosition_t sourcePosition;
} sysmelb_MacroContext_t;

typedef sysmelb_Value_t (*sysmelb_PrimitiveFunction_t) (size_t argumentCount, sysmelb_Value_t *arguments);
typedef sysmelb_Value_t (*sysmelb_PrimitiveMacroFunction_t) (sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments);

typedef struct sysmelb_FunctionBytecode_s {
    uint16_t argumentCount;
    uint16_t captureCount;
    uint32_t activationContextSize;
    uint32_t instructionCapacity;
    uint32_t instructionSize;
    sysmelb_FunctionInstruction_t *instruction;
} sysmelb_FunctionBytecode_t;

typedef struct sysmelb_function_s {
    sysmelb_FunctionKind_t kind;
    sysmelb_symbol_t *name;

    union
    {
        sysmelb_PrimitiveFunction_t primitiveFunction;
        sysmelb_PrimitiveMacroFunction_t primitiveMacroFunction;
        sysmelb_FunctionBytecode_t bytecode;
    };
} sysmelb_function_t;

sysmelb_FunctionBytecode_t sysmelb_bytecode_addInstruction(sysmelb_FunctionBytecode_t*bytecode, sysmelb_FunctionInstruction_t instructionToAdd);
void sysmelb_bytecode_pushLiteral(sysmelb_FunctionBytecode_t *bytecode, sysmelb_Value_t *literal);
void sysmelb_bytecode_pushArgument(sysmelb_FunctionBytecode_t *bytecode, uint16_t argumentIndex);
void sysmelb_bytecode_pushCapture(sysmelb_FunctionBytecode_t *bytecode, uint16_t captureIndex);

sysmelb_Value_t sysmelb_interpretBytecodeFunction(sysmelb_function_t *function, size_t argumentCount, sysmelb_Value_t *arguments);
#endif //SYSMELB_FUNCTION_H