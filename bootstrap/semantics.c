#include "semantics.h"
#include "function.h"
#include "types.h"
#include "error.h"
#include <stdio.h>

#define SYSMEL_MAX_ARGUMENT_COUNT 16

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
                    .sourcePosition = ast->sourcePosition
                };

                sysmelb_Value_t macroResult = function->primitiveMacroFunction(&macroContext, argumentCount, applicationArguments);
                assert(macroResult.kind == SysmelValueKindParseTreeReference);
                return sysmelb_analyzeAndEvaluateScript(environment, macroResult.parseTreeReference);
            }
            default:
                abort();
            }
        }
        else if(functionalValue.kind == SysmelValueKindTypeReference)
        {
            sysmelb_errorPrintf(ast->sourcePosition, "TODO: type() constructors.");
            abort();
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
            sysmelb_errorPrintf(ast->sourcePosition, "Failed to find method with selector #%.*s.\n", selector.symbolReference->size, selector.symbolReference->string);
            abort();
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
        }
    }
        abort();
    case ParseTreeMessageCascade:
        abort();
    case ParseTreeCascadedMessage:
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