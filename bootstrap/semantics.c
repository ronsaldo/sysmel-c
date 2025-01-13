#include "semantics.h"
#include "function.h"
#include "types.h"
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
                fprintf(stderr, "Failed to find binding for symbol #%.*s", (int)ast->identifierReference.identifier->size, ast->identifierReference.identifier->string);
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
        abort();
    case ParseTreeMessageSend:
    {
        sysmelb_Value_t receiver = sysmelb_analyzeAndEvaluateScript(environment, ast->messageSend.receiver);
        sysmelb_Value_t selector = sysmelb_analyzeAndEvaluateScript(environment, ast->messageSend.selector);
        if(selector.kind != SysmelValueKindSymbolReference)
        {
            fprintf(stderr, "Expected a symbol for a message send selector.");
            abort();
        }

        assert(receiver.type);
        sysmelb_function_t *method = sysmelb_type_lookupSelector(receiver.type, selector.symbolReference);
        if(!method)
        {
            fprintf(stderr, "Failed to find method with selector #%.*s.\n", selector.symbolReference->size, selector.symbolReference->string);
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
            break;
        case SysmelFunctionKindPrimitiveMacro:
            fprintf(stderr, "TODO: Support macro message sends");
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
        abort();
    default:
        abort();
    }
}