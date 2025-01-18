#include "function.h"
#include "error.h"
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
        int16_t jumpOffset;

        uint16_t arraySize;
        uint16_t dictionarySize;
        uint16_t tupleSize;

        sysmelb_SourcePosition_t assertPosition;
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
    bytecode->instructionCapacity = newCapacity;
    bytecode->instructions = newStorage;
}
uint16_t sysmelb_bytecode_addInstruction(sysmelb_FunctionBytecode_t*bytecode, sysmelb_FunctionInstruction_t instructionToAdd)
{
    sysmelb_bytecode_ensureCapacity(bytecode);
    uint16_t instructionIndex = bytecode->instructionSize;
    bytecode->instructions[bytecode->instructionSize++] = instructionToAdd;
    return instructionIndex;
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

void sysmelb_bytecode_storeTemporary(sysmelb_FunctionBytecode_t *bytecode, uint16_t tempIndex)
{
    sysmelb_FunctionInstruction_t inst = {
        .opcode = SysmelFunctionOpcodeStoreTemporary,
        .temporaryIndex = tempIndex
    };
    sysmelb_bytecode_addInstruction(bytecode, inst);
}
void sysmelb_bytecode_popAndStoreTemporary(sysmelb_FunctionBytecode_t *bytecode, uint16_t tempIndex)
{
    sysmelb_FunctionInstruction_t inst = {
        .opcode = SysmelFunctionOpcodePopAndStoreTemporary,
        .temporaryIndex = tempIndex
    };
    sysmelb_bytecode_addInstruction(bytecode, inst);
}

uint16_t sysmelb_bytecode_allocateTemporary(sysmelb_FunctionBytecode_t *bytecode)
{
    assert(bytecode->temporaryZoneSize < SYSMEL_BYTECODE_MAX_TEMPORARY_COUNT);
    return bytecode->temporaryZoneSize++;
}


uint16_t sysmelb_bytecode_jump(sysmelb_FunctionBytecode_t *bytecode)
{
    sysmelb_FunctionInstruction_t inst = {
        .opcode = SysmelFunctionOpcodeJump,
        .jumpOffset = 0
    };
    return sysmelb_bytecode_addInstruction(bytecode, inst);
}

uint16_t sysmelb_bytecode_jumpIfFalse(sysmelb_FunctionBytecode_t *bytecode)
{
    sysmelb_FunctionInstruction_t inst = {
        .opcode = SysmelFunctionOpcodeJumpIfFalse,
        .jumpOffset = 0
    };
    return sysmelb_bytecode_addInstruction(bytecode, inst);
}

uint16_t sysmelb_bytecode_jumpIfTrue(sysmelb_FunctionBytecode_t *bytecode)
{
    sysmelb_FunctionInstruction_t inst = {
        .opcode = SysmelFunctionOpcodeJumpIfTrue,
        .jumpOffset = 0
    };
    return sysmelb_bytecode_addInstruction(bytecode, inst);
}

void sysmelb_bytecode_patchJumpToHere(sysmelb_FunctionBytecode_t *bytecode, uint16_t jumpInstructionIndex)
{
    int16_t offset = bytecode->instructionSize - jumpInstructionIndex;
    bytecode->instructions[jumpInstructionIndex].jumpOffset = offset;
}

uint16_t sysmelb_bytecode_label(sysmelb_FunctionBytecode_t *bytecode)
{
    return bytecode->instructionSize;
}

void sysmelb_bytecode_patchJumpToLabel(sysmelb_FunctionBytecode_t *bytecode, uint16_t jumpInstructionIndex, uint16_t labelTarget)
{
    int16_t offset = labelTarget - jumpInstructionIndex;
    bytecode->instructions[jumpInstructionIndex].jumpOffset = offset;
}

void sysmelb_bytecode_integerEquals(sysmelb_FunctionBytecode_t *bytecode)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodeIntegerEquals
    };

    sysmelb_bytecode_addInstruction(bytecode, inst);
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

void sysmelb_bytecode_makeAssociation(sysmelb_FunctionBytecode_t *bytecode)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodeMakeAssociation,
    };

    sysmelb_bytecode_addInstruction(bytecode, inst);
}
void sysmelb_bytecode_makeArray(sysmelb_FunctionBytecode_t *bytecode, uint16_t size)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodeMakeArray,
        .arraySize = size,
    };

    sysmelb_bytecode_addInstruction(bytecode, inst);
}

void sysmelb_bytecode_makeDictionary(sysmelb_FunctionBytecode_t *bytecode, uint16_t size)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodeMakeDictionary,
        .dictionarySize = size,
    };

    sysmelb_bytecode_addInstruction(bytecode, inst);
}

void sysmelb_bytecode_makeTuple(sysmelb_FunctionBytecode_t *bytecode, uint16_t size)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodeMakeTuple,
        .tupleSize = size,
    };

    sysmelb_bytecode_addInstruction(bytecode, inst);
}

void sysmelb_bytecode_assert(sysmelb_FunctionBytecode_t *bytecode, sysmelb_SourcePosition_t sourcePosition)
{
    sysmelb_FunctionInstruction_t inst ={
        .opcode = SysmelFunctionOpcodeAssert,
        .assertPosition = sourcePosition
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

sysmelb_Value_t sysmelb_callFunctionWithArguments(sysmelb_function_t *function, size_t argumentCount, sysmelb_Value_t *arguments)
{
    switch(function->kind)
    {
    case SysmelFunctionKindPrimitive:
        return function->primitiveFunction(argumentCount, arguments);
    case SysmelFunctionKindPrimitiveMacro: abort();
    case SysmelFunctionKindInterpreted:
        return sysmelb_interpretBytecodeFunction(function, argumentCount, arguments);
    case SysmelFunctionKindInterpretedMacro: abort();
    default: abort();
    }
}

void sysmelb_disassemblyBytecodeFunction(sysmelb_function_t *function)
{
    uint32_t instructionCount = function->bytecode.instructionSize;
    sysmelb_FunctionInstruction_t *instructions = function->bytecode.instructions;
    for(uint32_t pc = 0; pc < instructionCount; ++pc)
    {
        sysmelb_FunctionInstruction_t *currentInstruction = instructions + pc;
        switch(currentInstruction->opcode)
        {
        case SysmelFunctionOpcodeNop:
            printf("%04d Nop\n", pc);
            break;
        case SysmelFunctionOpcodePushLiteral:
            printf("%04d PushLiteral\n", pc);
            break;
        case SysmelFunctionOpcodePushArgument:
            printf("%04d PushArgument %d\n", pc, currentInstruction->argumentIndex);
            break;
        case SysmelFunctionOpcodePushCapture:
            printf("%04d PushCapture\n", pc);
            break;
        case SysmelFunctionOpcodePushTemporary:
            printf("%04d PushTemporary %d\n", pc, currentInstruction->temporaryIndex);
            break;
        case SysmelFunctionOpcodeStoreTemporary:
            printf("%04d StoreTemporary %d\n", pc, currentInstruction->temporaryIndex);
            break;
        case SysmelFunctionOpcodePopAndStoreTemporary:
            printf("%04d PopAndStoreTemporary %d\n", pc, currentInstruction->temporaryIndex);
            break;
        case SysmelFunctionOpcodePop:
            printf("%04d Pop\n", pc);
            break;
        case SysmelFunctionOpcodeReturn:
            printf("%04d Return\n", pc);
            break;
        case SysmelFunctionOpcodeIntegerEquals:
            printf("%04d IntegerEquals\n", pc);
            break;
        case SysmelFunctionOpcodeApplyFunction:
            printf("%04d ApplyFunction %d\n", pc, currentInstruction->applicationArgumentCount);
            break;
        case SysmelFunctionOpcodeSendMessage:
            printf("%04d SendMessage %.*s %d\n", pc, currentInstruction->messageSendSelector->size, currentInstruction->messageSendSelector->string , currentInstruction->messageSendArguments);
            break;
        case SysmelFunctionOpcodeJump:
            printf("%04d Jump %03d:%03d\n", pc, currentInstruction->jumpOffset,pc + currentInstruction->jumpOffset);
            break;
        case SysmelFunctionOpcodeJumpIfFalse:
            printf("%04d JumpIfFalse %03d:%03d\n", pc, currentInstruction->jumpOffset,pc + currentInstruction->jumpOffset);
            break;
        case SysmelFunctionOpcodeJumpIfTrue:
            printf("%04d JumpIfTrue %03d:%03d\n", pc, currentInstruction->jumpOffset,pc + currentInstruction->jumpOffset);
            break;
        case SysmelFunctionOpcodeMakeArray:
            printf("%04d MakeArray %d\n", pc, currentInstruction->arraySize);
            break;
        case SysmelFunctionOpcodeMakeTuple:
            printf("%04d MakeTuple %d\n", pc, currentInstruction->tupleSize);
            break;
        case SysmelFunctionOpcodeMakeAssociation:
            printf("%04d MakeAssociation\n", pc);
            break;
        case SysmelFunctionOpcodeMakeDictionary:
            printf("%04d MakeDictionary %d\n", pc, currentInstruction->dictionarySize);
            break;
        case SysmelFunctionOpcodeAssert:
            printf("%04d Assert\n", pc);
            break;
        default: abort();
        }
    }
}

sysmelb_Value_t sysmelb_interpretBytecodeFunction(sysmelb_function_t *function, size_t argumentCount, sysmelb_Value_t *arguments)
{
    //sysmelb_disassemblyBytecodeFunction(function);

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
            context.temporaryZone[currentInstruction->temporaryIndex] = sysmelb_bytecodeActivationContext_top(&context);
            ++pc;
            break;
        case SysmelFunctionOpcodePopAndStoreTemporary:
            assert(currentInstruction->temporaryIndex < SYSMEL_BYTECODE_MAX_TEMPORARY_COUNT);
            context.temporaryZone[currentInstruction->temporaryIndex] = sysmelb_bytecodeActivationContext_pop(&context);
            ++pc;
            break;
        case SysmelFunctionOpcodePop:
            sysmelb_bytecodeActivationContext_pop(&context);
            ++pc;
            break;
        case SysmelFunctionOpcodeReturn:
            result = sysmelb_bytecodeActivationContext_top(&context);
            return result;
        case SysmelFunctionOpcodeIntegerEquals:
        {
            sysmelb_Value_t rightOperand = sysmelb_bytecodeActivationContext_pop(&context);
            sysmelb_Value_t leftOperand = sysmelb_bytecodeActivationContext_pop(&context);
            assert(leftOperand.kind == SysmelValueKindInteger || leftOperand.kind == SysmelValueKindUnsignedInteger);
            assert(rightOperand.kind == SysmelValueKindInteger || rightOperand.kind == SysmelValueKindUnsignedInteger);
            
            sysmelb_Value_t result = {
                .kind = SysmelValueKindBoolean,
                .type = sysmelb_getBasicTypes()->boolean,
                .boolean = leftOperand.integer == rightOperand.integer,
            };
            sysmelb_bytecodeActivationContext_push(&context, result);
        }
            ++pc;
            break;
        case SysmelFunctionOpcodeApplyFunction:
            {
                uint32_t applicationArgumentCount = currentInstruction->applicationArgumentCount;
                uint32_t popCount = applicationArgumentCount;
                for(uint32_t i = 0; i < popCount; ++i)
                    context.calloutArguments[popCount - 1 - i] = sysmelb_bytecodeActivationContext_pop(&context);

                sysmelb_Value_t function = sysmelb_bytecodeActivationContext_pop(&context);
                switch(function.kind)
                {
                case SysmelValueKindFunctionReference:
                    {
                        sysmelb_Value_t value = sysmelb_callFunctionWithArguments(function.functionReference, popCount, context.calloutArguments);
                        sysmelb_bytecodeActivationContext_push(&context, value);
                    }
                    break;
                case SysmelValueKindTypeReference:
                {
                    sysmelb_Value_t instance = sysmelb_instantiateTypeWithArguments(function.typeReference, popCount, context.calloutArguments);
                    sysmelb_bytecodeActivationContext_push(&context, instance);
                }
                break;
                default:
                    abort();
                }

                ++pc;
                break;
            }
        case SysmelFunctionOpcodeSendMessage:
            {
                uint32_t messageArgumentCount = currentInstruction->messageSendArguments;
                uint32_t popCount = messageArgumentCount + /*receiver*/ 1;
                for(uint32_t i = 0; i < popCount; ++i)
                    context.calloutArguments[popCount - 1 - i] = sysmelb_bytecodeActivationContext_pop(&context);

                sysmelb_Value_t receiver = context.calloutArguments[0];
                assert(receiver.type != NULL);
                sysmelb_function_t *method = sysmelb_type_lookupSelector(receiver.type, currentInstruction->messageSendSelector);
                bool isSynthetic = false;
                if(!method)
                {
                    if(receiver.kind == SysmelValueKindTupleReference && receiver.type->kind == SysmelTypeKindRecord)
                    {
                        if (messageArgumentCount == 0)
                        {
                            int recordFieldIndex = sysmelb_findIndexOfFieldNamed(receiver.type, currentInstruction->messageSendSelector);
                            if(recordFieldIndex >= 0)
                            {
                                sysmelb_Value_t fieldValue = receiver.tupleReference->elements[recordFieldIndex];
                                sysmelb_bytecodeActivationContext_push(&context, fieldValue);
                                isSynthetic = true;
                            }
                        }
                        else if(messageArgumentCount == 1)
                        {
                            // Remove the trailing:
                            sysmelb_symbol_t *fieldName = currentInstruction->messageSendSelector;
                            if(fieldName->size > 0 && fieldName->string[fieldName->size -1] == ':')
                                fieldName = sysmelb_internSymbol(fieldName->size - 1, fieldName->string);

                            int recordFieldIndex = sysmelb_findIndexOfFieldNamed(receiver.type, fieldName);
                            if(recordFieldIndex >= 0)
                            {
                                sysmelb_Value_t newFieldValue = context.calloutArguments[1];
                                receiver.tupleReference->elements[recordFieldIndex] = newFieldValue;
                                sysmelb_bytecodeActivationContext_push(&context, receiver);
                                isSynthetic = true;
                            }
                        }
                    }

                    if(receiver.kind == SysmelValueKindTypeReference)
                    {
                        if(receiver.typeReference->kind == SysmelTypeKindEnum)
                        {
                            sysmelb_Value_t enumValue;
                            if(sysmelb_findEnumValueWithName(receiver.typeReference, currentInstruction->messageSendSelector, &enumValue))
                            {
                                sysmelb_bytecodeActivationContext_push(&context, enumValue);
                                isSynthetic = true;
                            }
                        }
                    }
                    if(!isSynthetic)
                    {
                        sysmelb_SourcePosition_t noPosition = {0};
                        sysmelb_errorPrintf(noPosition, "Message not understood. #%.*s", currentInstruction->messageSendSelector->size, currentInstruction->messageSendSelector->string);
                        abort();
                    }
                }

                if(!isSynthetic)
                {
                    sysmelb_Value_t value = sysmelb_callFunctionWithArguments(method, popCount, context.calloutArguments);
                    sysmelb_bytecodeActivationContext_push(&context, value);
                }
                ++pc;
                break;
            }
        case SysmelFunctionOpcodeJumpIfFalse:
            sysmelb_Value_t condition = sysmelb_bytecodeActivationContext_pop(&context);
            assert(condition.kind == SysmelValueKindBoolean);
            if(condition.boolean)
                ++pc;
            else
                pc += currentInstruction->jumpOffset;
            break;
        case SysmelFunctionOpcodeJumpIfTrue:
        {
            sysmelb_Value_t condition = sysmelb_bytecodeActivationContext_pop(&context);
            assert(condition.kind == SysmelValueKindBoolean);
            if(condition.boolean)
                pc += currentInstruction->jumpOffset;
            else
                ++pc;
        }
            break;
        case SysmelFunctionOpcodeJump:
            pc += currentInstruction->jumpOffset;
            break;
        case SysmelFunctionOpcodeMakeAssociation:
        {
            sysmelb_Value_t value = sysmelb_bytecodeActivationContext_pop(&context);
            sysmelb_Value_t key = sysmelb_bytecodeActivationContext_pop(&context);

            sysmelb_Association_t *assoc =  sysmelb_allocate(sizeof(sysmelb_Association_t));
            assoc->key = key;
            assoc->value = value;

            sysmelb_Value_t assocReference =  {
                .kind = SysmelValueKindAssociationReference,
                .type = sysmelb_getBasicTypes()->association,
                .associationReference = assoc
            };

            sysmelb_bytecodeActivationContext_push(&context, assocReference);
            ++pc;
        }
            break;
        case SysmelFunctionOpcodeMakeDictionary:
        {
            uint16_t dictionarySize = currentInstruction->dictionarySize;
            sysmelb_Dictionary_t *dictionary = sysmelb_allocate(sizeof(sysmelb_Dictionary_t) + dictionarySize*sizeof(sysmelb_Association_t));
            dictionary->size = dictionarySize;
            for(uint16_t i = 0; i < dictionarySize; ++i)
            {
                sysmelb_Value_t assoc = sysmelb_bytecodeActivationContext_pop(&context);
                assert(assoc.kind == SysmelValueKindAssociationReference);
                dictionary->elements[dictionarySize - 1 - i] = assoc.associationReference;
            }

            sysmelb_Value_t dictionaryValue = {
                .kind = SysmelValueKindDictionaryReference,
                .type = sysmelb_getBasicTypes()->dictionary,
                .dictionaryReference = dictionary
            };
            sysmelb_bytecodeActivationContext_push(&context, dictionaryValue);
        }
            ++pc;
            break;
        case SysmelFunctionOpcodeMakeArray:
        {
            uint16_t arraySize = currentInstruction->arraySize;
            sysmelb_ArrayHeader_t *array = sysmelb_allocate(sizeof(sysmelb_ArrayHeader_t) + arraySize*sizeof(sysmelb_Value_t));
            array->size = arraySize;
            for(uint16_t i = 0; i < arraySize; ++i)
            {
                sysmelb_Value_t element = sysmelb_bytecodeActivationContext_pop(&context);
                array->elements[arraySize - 1 - i] = element;
            }

            sysmelb_Value_t arrayValue = {
                .kind = SysmelValueKindArrayReference,
                .type = sysmelb_getBasicTypes()->array,
                .arrayReference = array
            };
            sysmelb_bytecodeActivationContext_push(&context, arrayValue);
        }
            ++pc;
            break;
        case SysmelFunctionOpcodeAssert:
        {
            sysmelb_Value_t condition = sysmelb_bytecodeActivationContext_pop(&context);
            assert(condition.kind == SysmelValueKindBoolean);
            if(!condition.boolean)
            {
                sysmelb_errorPrintf(currentInstruction->assertPosition, "Assertion failure.");
            }

            sysmelb_Value_t voidValue = {
                .kind = SysmelValueKindVoid,
                .type = sysmelb_getBasicTypes()->voidType
            };
            sysmelb_bytecodeActivationContext_push(&context, voidValue);
        }
            ++pc;
            break;
        default:
            abort();
        }
    }

    return result;
}
