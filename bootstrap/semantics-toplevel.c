#include "semantics.h"
#include "function.h"
#include "types.h"
#include "error.h"
#include "memory.h"
#include <stdio.h>

sysmelb_Value_t sysmelb_analyzeAndEvaluateScript(sysmelb_Environment_t *environment, sysmelb_ParseTreeNode_t *ast)
{
    switch(ast->kind)
    {
    // Literals
    case ParseTreeLiteralIntegerNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindInteger,
                .type = sysmelb_getBasicTypes()->integer,
                .integer = ast->literalInteger.value
            };
            return value;
        }
    case ParseTreeLiteralCharacterNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindCharacter,
                .type = sysmelb_getBasicTypes()->character,
                .unsignedInteger = ast->literalCharacter.value
            };
            return value;
        }
    case ParseTreeLiteralFloatNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindFloatingPoint,
                .type = sysmelb_getBasicTypes()->floatingPoint,
                .floatingPoint = ast->literalFloat.value
            };
            return value;
        }
    case ParseTreeLiteralStringNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindStringReference,
                .type = sysmelb_getBasicTypes()->string,
                .string = ast->literalString.string,
                .stringSize = ast->literalString.stringSize,
            };
            return value;
        }
    case ParseTreeLiteralSymbolNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindSymbolReference,
                .type = sysmelb_getBasicTypes()->symbol,
                .symbolReference = ast->literalSymbol.internedSymbol
            };
            return value;
        }
    case ParseTreeLiteralValueNode:
        return ast->literalValue.value;

    // Identifiers
    case ParseTreeIdentifierReference:
        {
            sysmelb_SymbolBinding_t *binding = sysmelb_environmentLookRecursively(environment, ast->identifierReference.identifier);
            if(!binding)
            {
                sysmelb_errorPrintf(ast->sourcePosition, "Failed to find binding for symbol #%.*s\n", (int)ast->identifierReference.identifier->size, ast->identifierReference.identifier->string);
                abort();
            }
            switch(binding->kind)
            {
            case SysmelSymbolValueBinding:
                return binding->value;
            default:
                abort();
            }
        }

    // Functions and message send
    case ParseTreeFunctionApplication:
    {
        sysmelb_Value_t functionalValue = sysmelb_analyzeAndEvaluateScript(environment, ast->functionApplication.functional);
        if(functionalValue.kind == SysmelValueKindFunctionReference)
        {
            sysmelb_function_t *function = functionalValue.functionReference;
            switch(function->kind)
            {
            case SysmelFunctionKindPrimitive:
            {
                size_t argumentCount = ast->functionApplication.arguments.size;
                assert(ast->functionApplication.arguments.size <= SYSMEL_MAX_ARGUMENT_COUNT);
                sysmelb_Value_t applicationArguments[SYSMEL_MAX_ARGUMENT_COUNT];

                for(size_t i = 0; i < argumentCount; ++i)
                    applicationArguments[i] = sysmelb_analyzeAndEvaluateScript(environment, ast->functionApplication.arguments.elements[i]);
                
                return function->primitiveFunction(argumentCount, applicationArguments);
            }
            case SysmelFunctionKindPrimitiveMacro:
            {
                size_t argumentCount = ast->functionApplication.arguments.size;
                assert(ast->functionApplication.arguments.size <= SYSMEL_MAX_ARGUMENT_COUNT);
                sysmelb_Value_t applicationArguments[SYSMEL_MAX_ARGUMENT_COUNT];

                for(size_t i = 0; i < argumentCount; ++i)
                {
                    sysmelb_Value_t argumentValue = {
                        .kind = SysmelValueKindParseTreeReference,
                        .parseTreeReference = ast->functionApplication.arguments.elements[i],
                    };

                    applicationArguments[i] = argumentValue;
                }
                
                sysmelb_MacroContext_t macroContext = {
                    .sourcePosition = ast->sourcePosition,
                    .environment = environment
                };

                sysmelb_Value_t macroResult = function->primitiveMacroFunction(&macroContext, argumentCount, applicationArguments);
                if(macroResult.kind == SysmelValueKindParseTreeReference)
                    return sysmelb_analyzeAndEvaluateScript(environment, macroResult.parseTreeReference);
                else
                    return macroResult;
            }
            case SysmelFunctionKindInterpreted:
                size_t argumentCount = ast->functionApplication.arguments.size;
                assert(ast->functionApplication.arguments.size <= SYSMEL_MAX_ARGUMENT_COUNT);
                sysmelb_Value_t applicationArguments[SYSMEL_MAX_ARGUMENT_COUNT];

                for(size_t i = 0; i < argumentCount; ++i)
                    applicationArguments[i] = sysmelb_analyzeAndEvaluateScript(environment, ast->functionApplication.arguments.elements[i]);
                
                return sysmelb_interpretBytecodeFunction(function, argumentCount, applicationArguments);

            default:
                abort();
            }
        }
        else if(functionalValue.kind == SysmelValueKindTypeReference)
        {
            size_t argumentCount = ast->functionApplication.arguments.size;
            assert(ast->functionApplication.arguments.size <= SYSMEL_MAX_ARGUMENT_COUNT);
            sysmelb_Value_t applicationArguments[SYSMEL_MAX_ARGUMENT_COUNT];

            for(size_t i = 0; i < argumentCount; ++i)
                applicationArguments[i] = sysmelb_analyzeAndEvaluateScript(environment, ast->functionApplication.arguments.elements[i]);

            sysmelb_Value_t instance = sysmelb_instantiateTypeWithArguments(functionalValue.typeReference, argumentCount, applicationArguments);
            return instance;
        }
        else
        {
            sysmelb_errorPrintf(ast->sourcePosition, "Unsupported application.");
            abort();           
        }
        break;
    }
    case ParseTreeMessageSend:
    {
        sysmelb_Value_t receiver = sysmelb_analyzeAndEvaluateScript(environment, ast->messageSend.receiver);
        sysmelb_Value_t selector = sysmelb_analyzeAndEvaluateScript(environment, ast->messageSend.selector);
        if(selector.kind != SysmelValueKindSymbolReference)
        {
            sysmelb_errorPrintf(ast->sourcePosition, "Expected a symbol for a message send selector.");
            abort();
        }

        assert(receiver.type);
        sysmelb_function_t *method = sysmelb_type_lookupSelector(receiver.type, selector.symbolReference);
        if(!method)
        {
            if(receiver.kind == SysmelValueKindValueBoxReference)
            {
                receiver = receiver.valueBoxReference->currentValue;
                method = sysmelb_type_lookupSelector(receiver.type, selector.symbolReference);
            }

            if(!method)
            {
                sysmelb_errorPrintf(ast->sourcePosition, "Failed to find method with selector #%.*s.\n", selector.symbolReference->size, selector.symbolReference->string);
                abort();
            }
        }
        
        switch(method->kind)
        {
        case SysmelFunctionKindPrimitive:
        {
            assert(ast->messageSend.arguments.size <= SYSMEL_MAX_ARGUMENT_COUNT);
            sysmelb_Value_t messageArguments[SYSMEL_MAX_ARGUMENT_COUNT + 1];
            messageArguments[0] = receiver;
            size_t argumentCount = ast->messageSend.arguments.size;
            for(size_t i = 0; i < argumentCount; ++i)
                messageArguments[i + 1] = sysmelb_analyzeAndEvaluateScript(environment, ast->messageSend.arguments.elements[i]);
            return method->primitiveFunction(1 + argumentCount, messageArguments);
        }
        case SysmelFunctionKindPrimitiveMacro:
            sysmelb_errorPrintf(ast->sourcePosition, "TODO: Support macro message sends");
            abort();
            break;
        default:
            abort();
        }
    }
        abort();
    case ParseTreeMessageCascade:
    {
        sysmelb_Value_t receiver = sysmelb_analyzeAndEvaluateScript(environment, ast->messageCascade.receiver);
        sysmelb_ParseTreeNode_t *receiverLiteral = sysmelb_newParseTreeNode(ParseTreeLiteralValueNode, ast->messageCascade.receiver->sourcePosition);
        receiverLiteral->literalValue.value = receiver;

        sysmelb_Value_t result = receiver;
        size_t messageCount = ast->messageCascade.cascadedMessages.size;
        for(size_t i = 0; i < messageCount; ++i)
        {
            sysmelb_ParseTreeNode_t *cascaded = ast->messageCascade.cascadedMessages.elements[i];
            if(cascaded->kind != ParseTreeCascadedMessage)
            {
                sysmelb_errorPrintf(cascaded->sourcePosition, "Expected a cascaded message.");
                abort();
            }

            sysmelb_ParseTreeNode_t *cascadedMessage = sysmelb_newParseTreeNode(ParseTreeMessageSend, cascaded->sourcePosition);
            cascadedMessage->messageSend.receiver = receiverLiteral;
            cascadedMessage->messageSend.selector = cascaded->cascadedMessage.selector;
            cascadedMessage->messageSend.arguments = cascaded->cascadedMessage.arguments;
            result = sysmelb_analyzeAndEvaluateScript(environment, cascadedMessage);
        }

        return result;
    }
    case ParseTreeCascadedMessage:
        sysmelb_errorPrintf(ast->sourcePosition, "Cascaded message must be a part of a cascade.");
        abort();
    case ParseTreeBinaryOperatorSequence:
        {
            //TODO: use an operator precedence parser.
            sysmelb_ParseTreeNode_t *receiver = ast->binaryOperatorSequence.elements.elements[0];
            for(size_t i = 1; i < ast->binaryOperatorSequence.elements.size; i += 2)
            {
                sysmelb_ParseTreeNode_t *selector = ast->binaryOperatorSequence.elements.elements[i];
                sysmelb_ParseTreeNode_t *operand = ast->binaryOperatorSequence.elements.elements[i + 1];

                sysmelb_ParseTreeNode_t *binaryMessage = sysmelb_newParseTreeNode(ParseTreeMessageSend, selector->sourcePosition);
                binaryMessage->messageSend.receiver = receiver;
                binaryMessage->messageSend.selector = selector;
                sysmelb_ParseTreeNodeDynArray_add(&binaryMessage->messageSend.arguments, operand);
                receiver = binaryMessage;
            }
            return sysmelb_analyzeAndEvaluateScript(environment, receiver);
        }
    // Sequences, array, tuples
    case ParseTreeSequence:
        {
            sysmelb_Value_t lastResult = {};
            for(size_t i = 0; i < ast->sequence.elements.size; ++i)
                lastResult = sysmelb_analyzeAndEvaluateScript(environment, ast->sequence.elements.elements[i]);
            return lastResult;
        }

    case ParseTreeArray:
        {
            size_t arraySize = ast->tuple.elements.size;
            sysmelb_ArrayHeader_t *arrayData = sysmelb_allocate(sizeof(sysmelb_ByteArrayHeader_t) + arraySize * sizeof(sysmelb_Value_t));
            arrayData->size = arraySize;
            for(size_t i = 0; i < arraySize; ++i)
                arrayData->elements[i] = sysmelb_decayValue(sysmelb_analyzeAndEvaluateScript(environment, ast->tuple.elements.elements[i]));

            sysmelb_Value_t result = {
                .kind = SysmelValueKindArrayReference,
                .type = sysmelb_getBasicTypes()->array,
                .arrayReference = arrayData
            };
            return result;
        }
    case ParseTreeByteArray:
        {
            size_t arraySize = ast->tuple.elements.size;
            sysmelb_ByteArrayHeader_t *byteArrayData = sysmelb_allocate(sizeof(sysmelb_ByteArrayHeader_t) + arraySize * sizeof(sysmelb_Value_t));
            byteArrayData->size = arraySize;
            for(size_t i = 0; i < arraySize; ++i)
            {
                sysmelb_Value_t elementValue = sysmelb_decayValue(sysmelb_analyzeAndEvaluateScript(environment, ast->tuple.elements.elements[i]));
                byteArrayData->elements[i] = (uint8_t)elementValue.unsignedInteger;
            }

            sysmelb_Value_t result = {
                .kind = SysmelValueKindByteArrayReference,
                .type = sysmelb_getBasicTypes()->byteArray,
                .byteArrayReference = byteArrayData
            };
            return result;
        }
    case ParseTreeTuple:
        {
            size_t tupleSize = ast->tuple.elements.size;
            sysmelb_TupleHeader_t *tupleData = sysmelb_allocate(sizeof(sysmelb_TupleHeader_t) + tupleSize * sizeof(sysmelb_Value_t));
            tupleData->size = tupleSize;
            for(size_t i = 0; i < tupleSize; ++i)
                tupleData->elements[i] = sysmelb_decayValue(sysmelb_analyzeAndEvaluateScript(environment, ast->tuple.elements.elements[i]));

            sysmelb_Value_t result = {
                .kind = SysmelValueKindTupleReference,
                .type = sysmelb_getBasicTypes()->tuple,
                .tupleReference = tupleData
            };
            return result;
        }

    // Dictionary
    case ParseTreeAssociation:
        {
            sysmelb_Association_t *association = sysmelb_allocate(sizeof(sysmelb_Association_t));
            association->key = sysmelb_analyzeAndEvaluateScript(environment, ast->association.key);
            if(ast->association.value)
                association->value = sysmelb_analyzeAndEvaluateScript(environment, ast->association.value);

            sysmelb_Value_t result = {
                .kind = SysmelValueKindAssociationReference,
                .type = sysmelb_getBasicTypes()->association,
                .associationReference = association
            };
            return result;
        }

    case ParseTreeDictionary:
        {
            size_t dictionarySize = ast->dictionary.elements.size;
            sysmelb_Dictionary_t *dictionary = sysmelb_allocate(sizeof(sysmelb_Dictionary_t) + dictionarySize * sizeof(sysmelb_Association_t*));
            dictionary->size = dictionarySize;
            for(size_t i = 0; i < dictionarySize; ++i)
            {
                sysmelb_Value_t elementValue = sysmelb_analyzeAndEvaluateScript(environment, ast->dictionary.elements.elements[i]);
                if(elementValue.kind != SysmelValueKindAssociationReference)
                    sysmelb_errorPrintf(ast->dictionary.elements.elements[i]->sourcePosition, "Expected an association for the dictionary.");
                dictionary->elements[i] = elementValue.associationReference;
            }
            
            sysmelb_Value_t result = {
                .kind = SysmelValueKindDictionaryReference,
                .type = sysmelb_getBasicTypes()->dictionary,
                .dictionaryReference = dictionary
            };
            return result;
        }

    // Lexical block
    case ParseTreeLexicalBlock:
    {
        sysmelb_Environment_t *blockEnvironment = sysmelb_createLexicalEnvironment(environment);
        return sysmelb_analyzeAndEvaluateScript(blockEnvironment, ast->lexicalBlock.expression);
    }

    case ParseTreeFunctionalDependentType:
    {
        sysmelb_errorPrintf(ast->bindableName.nameExpression->sourcePosition, "Todo: Support dependent types by themselves.");
        abort();
    }
    case ParseTreeBindableName:
    {
        if(ast->bindableName.isMutable)
        {
            sysmelb_Value_t nameValue = sysmelb_analyzeAndEvaluateScript(environment, ast->bindableName.nameExpression);
            if(nameValue.kind != SysmelValueKindSymbolReference)
                sysmelb_errorPrintf(ast->bindableName.nameExpression->sourcePosition, "Expected a name");

            sysmelb_ValueBox_t *box = sysmelb_allocate(sizeof(sysmelb_ValueBox_t));
            sysmelb_Value_t value = {
                .kind = SysmelValueKindValueBoxReference,
                .type = sysmelb_getBasicTypes()->valueReference,
                .valueBoxReference = box,
            };

            sysmelb_Environment_setLocalSymbolBinding(environment, nameValue.symbolReference, sysmelb_createSymbolValueBinding(value));
            return value;
        }

        sysmelb_errorPrintf(ast->sourcePosition, "Immutable bindable names must be part of an assignment.");
        abort();
    }

    case ParseTreeAssignment:
    {
        sysmelb_ParseTreeNode_t *store = ast->assignment.store;
        sysmelb_ParseTreeNode_t *value = ast->assignment.value;
        if(store->kind == ParseTreeBindableName)
        {   
            sysmelb_Value_t nameValue = sysmelb_analyzeAndEvaluateScript(environment, store->bindableName.nameExpression);
            if(nameValue.kind != SysmelValueKindSymbolReference)
                sysmelb_errorPrintf(store->bindableName.nameExpression->sourcePosition, "Expected a name");


            if(store->bindableName.hasPostTypeExpression && store->bindableName.typeExpression->kind == ParseTreeFunctionalDependentType)
            {
                sysmelb_ParseTreeNode_t *functionNode = sysmelb_newParseTreeNode(ParseTreeFunction, ast->sourcePosition);
                functionNode->function.functionDependentType = store->bindableName.typeExpression;
                functionNode->function.bodyExpression = value;
                functionNode->function.name = nameValue.symbolReference;
                return sysmelb_analyzeAndCompileClosure(environment, functionNode);
            }

            sysmelb_Value_t initialValue = sysmelb_analyzeAndEvaluateScript(environment, value);
            if(store->bindableName.isMutable)
            {
                sysmelb_ValueBox_t *box = sysmelb_allocate(sizeof(sysmelb_ValueBox_t));
                box->currentValue = initialValue;

                sysmelb_Value_t boxValue = {
                    .kind = SysmelValueKindValueBoxReference,
                    .type = sysmelb_getBasicTypes()->valueReference,
                    .valueBoxReference = box,
                };
                sysmelb_Environment_setLocalSymbolBinding(environment, nameValue.symbolReference, sysmelb_createSymbolValueBinding(boxValue));
                return boxValue;
            }
            else
            {
                sysmelb_Environment_setLocalSymbolBinding(environment, nameValue.symbolReference, sysmelb_createSymbolValueBinding(initialValue));
                return initialValue;
            }
        }
        else if (store->kind == ParseTreeIdentifierReference)
        {
            sysmelb_Value_t storeValue = sysmelb_analyzeAndEvaluateScript(environment, store);
            if(storeValue.kind == SysmelValueKindValueBoxReference)
            {
                sysmelb_Value_t newValue = sysmelb_analyzeAndEvaluateScript(environment, value);
                storeValue.valueBoxReference->currentValue = newValue;
                return storeValue;
            }
        }

        // Transform the assignment into a message send.
        sysmelb_ParseTreeNode_t *selector = sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, ast->sourcePosition);
        selector->literalSymbol.internedSymbol = sysmelb_internSymbolC(":=");

        sysmelb_ParseTreeNode_t *message = sysmelb_newParseTreeNode(ParseTreeMessageSend, ast->sourcePosition);
        message->messageSend.receiver = store;
        message->messageSend.selector = selector;
        sysmelb_ParseTreeNodeDynArray_add(&message->messageSend.arguments, value);
        
        return sysmelb_analyzeAndEvaluateScript(environment, message);
    }

    // Control flow.
    case ParseTreeIfSelection:
        {
            sysmelb_Value_t condition = sysmelb_analyzeAndEvaluateScript(environment, ast->ifSelection.condition);
            if(condition.kind != SysmelValueKindBoolean)
            {
                sysmelb_errorPrintf(ast->sourcePosition, "Expected a boolean condition.");
            }

            if(condition.boolean)
            {
                if(ast->ifSelection.trueExpression)
                    return sysmelb_analyzeAndEvaluateScript(environment, ast->ifSelection.trueExpression);
            }
            else
            {
                if(ast->ifSelection.falseExpression)
                    return sysmelb_analyzeAndEvaluateScript(environment, ast->ifSelection.falseExpression);
            }

            sysmelb_Value_t voidResult = {
                .kind = SysmelValueKindVoid
            };
            return voidResult;
        }
    case ParseTreeWhileLoop:
    {
        sysmelb_Value_t condition = sysmelb_analyzeAndEvaluateScript(environment, ast->whileLoop.condition);
        if(condition.kind != SysmelValueKindBoolean)
            sysmelb_errorPrintf(ast->sourcePosition, "While loop condition must be a boolean.");

        while(condition.boolean)
        {
            if(ast->whileLoop.body)
                sysmelb_analyzeAndEvaluateScript(environment, ast->whileLoop.body);

            if(ast->whileLoop.continueExpression)
                sysmelb_analyzeAndEvaluateScript(environment, ast->whileLoop.continueExpression);

            condition = sysmelb_analyzeAndEvaluateScript(environment, ast->whileLoop.condition);
            if(condition.kind != SysmelValueKindBoolean)
                sysmelb_errorPrintf(ast->sourcePosition, "While loop condition must be a boolean.");
        }

        sysmelb_Value_t voidValue = {
            .kind = SysmelValueKindVoid
        };
        return voidValue;
    }
    default:
        abort();
    }
    abort();
}