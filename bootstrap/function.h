#ifndef SYSMELB_FUNCTION_H
#define SYSMELB_FUNCTION_H

#pragma once

#include "symbol.h"
#include "source-code.h"
#include <stddef.h>

#define SYSMEL_MAX_ARGUMENT_COUNT 16

typedef struct sysmelb_Value_s sysmelb_Value_t;
typedef struct sysmelb_Environment_s sysmelb_Environment_t;

typedef enum sysmelb_FunctionKind_e
{
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
    SysmelFunctionOpcodePushTemporary,
    SysmelFunctionOpcodeStoreTemporary,
    SysmelFunctionOpcodePopAndStoreTemporary,
    SysmelFunctionOpcodePop,
    SysmelFunctionOpcodeReturn,
    SysmelFunctionOpcodeApplyFunction,
    SysmelFunctionOpcodeSendMessage,
    SysmelFunctionOpcodeJump,
    SysmelFunctionOpcodeJumpIfFalse,
    SysmelFunctionOpcodeJumpIfTrue,
    SysmelFunctionOpcodeMakeArray,
    SysmelFunctionOpcodeMakeAssociation,
    SysmelFunctionOpcodeMakeDictionary,
    SysmelFunctionOpcodeMakeTuple,
    SysmelFunctionOpcodeAssert,
} sysmelb_FunctionOpcode_t;

typedef struct sysmelb_FunctionInstruction_s sysmelb_FunctionInstruction_t;

typedef struct sysmelb_MacroContext_s
{
    sysmelb_SourcePosition_t sourcePosition;
    sysmelb_Environment_t *environment;
} sysmelb_MacroContext_t;

typedef sysmelb_Value_t (*sysmelb_PrimitiveFunction_t)(size_t argumentCount, sysmelb_Value_t *arguments);
typedef sysmelb_Value_t (*sysmelb_PrimitiveMacroFunction_t)(sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments);

typedef struct sysmelb_FunctionBytecode_s
{
    uint16_t argumentCount;
    uint16_t captureCount;
    uint32_t temporaryZoneSize;
    uint32_t instructionCapacity;
    uint32_t instructionSize;
    sysmelb_FunctionInstruction_t *instructions;
} sysmelb_FunctionBytecode_t;

typedef struct sysmelb_function_s
{
    sysmelb_FunctionKind_t kind;
    sysmelb_symbol_t *name;

    union
    {
        sysmelb_PrimitiveFunction_t primitiveFunction;
        sysmelb_PrimitiveMacroFunction_t primitiveMacroFunction;
        sysmelb_FunctionBytecode_t bytecode;
    };
} sysmelb_function_t;

uint16_t sysmelb_bytecode_addInstruction(sysmelb_FunctionBytecode_t *bytecode, sysmelb_FunctionInstruction_t instructionToAdd);
void sysmelb_bytecode_pushLiteral(sysmelb_FunctionBytecode_t *bytecode, sysmelb_Value_t *literal);
void sysmelb_bytecode_pushArgument(sysmelb_FunctionBytecode_t *bytecode, uint16_t argumentIndex);
void sysmelb_bytecode_pushCapture(sysmelb_FunctionBytecode_t *bytecode, uint16_t captureIndex);
void sysmelb_bytecode_pushTemporary(sysmelb_FunctionBytecode_t *bytecode, uint16_t captureIndex);
void sysmelb_bytecode_pop(sysmelb_FunctionBytecode_t *bytecode);
void sysmelb_bytecode_return(sysmelb_FunctionBytecode_t *bytecode);

uint16_t sysmelb_bytecode_allocateTemporary(sysmelb_FunctionBytecode_t *bytecode);
void sysmelb_bytecode_storeTemporary(sysmelb_FunctionBytecode_t *bytecode, uint16_t tempIndex);
void sysmelb_bytecode_popAndStoreTemporary(sysmelb_FunctionBytecode_t *bytecode, uint16_t tempIndex);

uint16_t sysmelb_bytecode_jump(sysmelb_FunctionBytecode_t *bytecode);
uint16_t sysmelb_bytecode_jumpIfFalse(sysmelb_FunctionBytecode_t *bytecode);
uint16_t sysmelb_bytecode_jumpIfTrue(sysmelb_FunctionBytecode_t *bytecode);
void sysmelb_bytecode_patchJumpToHere(sysmelb_FunctionBytecode_t *bytecode, uint16_t jumpInstructionIndex);

uint16_t sysmelb_bytecode_label(sysmelb_FunctionBytecode_t *bytecode);
void sysmelb_bytecode_patchJumpToLabel(sysmelb_FunctionBytecode_t *bytecode, uint16_t jumpInstructionIndex, uint16_t labelTarget);

void sysmelb_bytecode_applyFunction(sysmelb_FunctionBytecode_t *bytecode, uint16_t argumentCount);
void sysmelb_bytecode_sendMessage(sysmelb_FunctionBytecode_t *bytecode, sysmelb_symbol_t *selector, uint16_t argumentCount);

void sysmelb_bytecode_makeAssociation(sysmelb_FunctionBytecode_t *bytecode);
void sysmelb_bytecode_makeArray(sysmelb_FunctionBytecode_t *bytecode, uint16_t size);
void sysmelb_bytecode_makeDictionary(sysmelb_FunctionBytecode_t *bytecode, uint16_t size);
void sysmelb_bytecode_makeTuple(sysmelb_FunctionBytecode_t *bytecode, uint16_t size);

void sysmelb_bytecode_assert(sysmelb_FunctionBytecode_t *bytecode, sysmelb_SourcePosition_t);

sysmelb_Value_t sysmelb_callFunctionWithArguments(sysmelb_function_t *function, size_t argumentCount, sysmelb_Value_t *arguments);
sysmelb_Value_t sysmelb_interpretBytecodeFunction(sysmelb_function_t *function, size_t argumentCount, sysmelb_Value_t *arguments);
#endif // SYSMELB_FUNCTION_H
