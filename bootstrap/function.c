#include "function.h"
#include "memory.h"
#include "value.h"
#include <string.h>

#define SYSMEL_BYTECODE_MAX_TEMPORARY_COUNT 64
#define SYSMEL_BYTECODE_MAX_STACK_DEPTH 64

typedef struct sysmelb_FunctionInstruction_s {
    sysmelb_FunctionOpcode_t opcode;
    union
    {
        uint16_t argumentIndex;
        sysmelb_Value_t literalValue;
        uint16_t applicationArgumentCount;

        struct
        {
            uint16_t messageSendArguments;
            sysmelb_symbol_t *messageSendSelector;
        };

        uint16_t temporaryIndex;
    };
} sysmelb_FunctionInstruction_t;

void sysmelb_bytecode_ensureCapacity(sysmelb_FunctionBytecode_t*bytecode)
{
    if(bytecode->instructionSize < bytecode->instructionCapacity)
        return;

    uint32_t newCapacity = bytecode->instructionCapacity * 2;
    if(newCapacity < 32)
        newCapacity = 32;

    sysmelb_FunctionInstruction_t *newStorage = sysmelb_allocate(sizeof(sysmelb_FunctionInstruction_t) * newCapacity);
    if(bytecode->instructions && bytecode->instructionSize > 0)
        memcpy(newStorage, bytecode->instructions, sizeof(sysmelb_FunctionInstruction_t)*bytecode->instructionSize);

    sysmelb_freeAllocation(bytecode->instructions);
    bytecode->instructions = newStorage;
}
void sysmelb_bytecode_addInstruction(sysmelb_FunctionBytecode_t*bytecode, sysmelb_FunctionInstruction_t instructionToAdd)
{
    sysmelb_bytecode_ensureCapacity(bytecode);
    bytecode->instructions[bytecode->instructionSize++] = instructionToAdd;
}

void sysmelb_bytecode_pushLiteral(sysmelb_FunctionBytecode_t *bytecode, sysmelb_Value_t *literal)
{
    sysmelb_FunctionInstruction_t inst = {
        .opcode = SysmelFunctionOpcodePushLiteral,
        .literalValue = *literal
    };
    sysmelb_bytecode_addInstruction(bytecode, inst);
}

void sysmelb_bytecode_pushArgument(sysmelb_FunctionBytecode_t *bytecode, uint16_t argumentIndex)
{
    sysmelb_FunctionInstruction_t inst = {
        .opcode = SysmelFunctionOpcodePushArgument,
        .argumentIndex = argumentIndex
    };
    sysmelb_bytecode_addInstruction(bytecode, inst);
}

void sysmelb_bytecode_pushCapture(sysmelb_FunctionBytecode_t *bytecode, uint16_t captureIndex)
{
    sysmelb_FunctionInstruction_t inst = {
        .opcode = SysmelFunctionOpcodePushTemporary,
        .temporaryIndex = captureIndex
    };
    sysmelb_bytecode_addInstruction(bytecode, inst);
}

void sysmelb_bytecode_pushTemporary(sysmelb_FunctionBytecode_t *bytecode, uint16_t temporaryIndex)
{
    sysmelb_FunctionInstruction_t inst = {
        .opcode = SysmelFunctionOpcodePushTemporary,
        .temporaryIndex = temporaryIndex
    };
    sysmelb_bytecode_addInstruction(bytecode, inst);
}

uint16_t sysmelb_bytecode_allocateTemporary(sysmelb_FunctionBytecode_t *bytecode)
{
    assert(bytecode->temporaryZoneSize < SYSMEL_BYTECODE_MAX_TEMPORARY_COUNT);
    return bytecode->temporaryZoneSize++;
}

void sysmelb_bytecode_applyFunction(sysmelb_FunctionBytecode_t *bytecode, uint16_t argumentCount)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodeApplyFunction,
        .applicationArgumentCount = argumentCount
    };

    sysmelb_bytecode_addInstruction(bytecode, inst);
}

void sysmelb_bytecode_sendMessage(sysmelb_FunctionBytecode_t *bytecode, sysmelb_symbol_t *selector, uint16_t argumentCount)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodeSendMessage,
        .messageSendSelector = selector,
        .messageSendArguments = argumentCount
    };

    sysmelb_bytecode_addInstruction(bytecode, inst);
}

void sysmelb_bytecode_pop(sysmelb_FunctionBytecode_t *bytecode)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodePop,
    };
    
    sysmelb_bytecode_addInstruction(bytecode, inst);
}
void sysmelb_bytecode_return(sysmelb_FunctionBytecode_t *bytecode)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodeReturn,
    };
    
    sysmelb_bytecode_addInstruction(bytecode, inst);
}

typedef struct sysmelb_bytecodeActivationContext_s
{
    sysmelb_Value_t calloutArguments[SYSMEL_MAX_ARGUMENT_COUNT + 1];
    sysmelb_Value_t temporaryZone[SYSMEL_BYTECODE_MAX_TEMPORARY_COUNT];
    sysmelb_Value_t stack[SYSMEL_BYTECODE_MAX_STACK_DEPTH];
    uint32_t stackSize;
} sysmelb_bytecodeActivationContext_t;

void sysmelb_bytecodeActivationContext_push(sysmelb_bytecodeActivationContext_t *context, sysmelb_Value_t stackValue)
{
    assert(context->stackSize < SYSMEL_BYTECODE_MAX_STACK_DEPTH);
    context->stack[context->stackSize++] = stackValue;
}

sysmelb_Value_t sysmelb_bytecodeActivationContext_pop(sysmelb_bytecodeActivationContext_t *context)
{
    assert(context->stackSize > 0);
    return context->stack[--context->stackSize];
}

sysmelb_Value_t sysmelb_bytecodeActivationContext_top(sysmelb_bytecodeActivationContext_t *context)
{
    assert(context->stackSize > 0);
    return context->stack[context->stackSize - 1];
}

sysmelb_Value_t sysmelb_interpretBytecodeFunction(sysmelb_function_t *function, size_t argumentCount, sysmelb_Value_t *arguments)
{
    sysmelb_Value_t result = {
        .kind = SysmelValueKindNull,
        .type = sysmelb_getBasicTypes()->null,
    };
    
    sysmelb_bytecodeActivationContext_t context = {};

    uint32_t pc = 0;
    uint32_t instructionCount = function->bytecode.instructionSize;
    sysmelb_FunctionInstruction_t *instructions = function->bytecode.instructions;
    while(pc < instructionCount)
    {
        sysmelb_FunctionInstruction_t *currentInstruction = instructions + pc;
        switch(currentInstruction->opcode)
        {
        case SysmelFunctionOpcodeNop:
            ++pc;
            break;
        case SysmelFunctionOpcodePushLiteral:
            sysmelb_bytecodeActivationContext_push(&context, currentInstruction->literalValue);
            ++pc;
            break;
        case SysmelFunctionOpcodePushArgument:
            assert(currentInstruction->argumentIndex < argumentCount);
            sysmelb_bytecodeActivationContext_push(&context, arguments[currentInstruction->argumentIndex]);
            ++pc;
            break;
        case SysmelFunctionOpcodePushCapture:
            abort();
        case SysmelFunctionOpcodePushTemporary:
            assert(currentInstruction->temporaryIndex < SYSMEL_BYTECODE_MAX_TEMPORARY_COUNT);
            sysmelb_bytecodeActivationContext_push(&context, context.temporaryZone[currentInstruction->temporaryIndex]);
            ++pc;
            break;
        case SysmelFunctionOpcodeStoreTemporary:
            assert(currentInstruction->temporaryIndex < SYSMEL_BYTECODE_MAX_TEMPORARY_COUNT);
            abort();
        case SysmelFunctionOpcodePopAndStoreTemporary:
            assert(currentInstruction->temporaryIndex < SYSMEL_BYTECODE_MAX_TEMPORARY_COUNT);
            abort();
        case SysmelFunctionOpcodePop:
            sysmelb_bytecodeActivationContext_pop(&context);
            ++pc;
            break;
        case SysmelFunctionOpcodeReturn:
            result = sysmelb_bytecodeActivationContext_top(&context);
            return result;
        case SysmelFunctionOpcodeApplyFunction:
            abort();
        case SysmelFunctionOpcodeSendMessage:
            abort();
        default:
            abort();
        }
    }

    return result;
}
