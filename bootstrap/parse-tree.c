#include "parse-tree.h"
#include "error.h"
#include "memory.h"
#include <stdio.h>

sysmelb_ParseTreeNode_t *sysmelb_newParseTreeNode(sysmelb_ParseTreeNodeKind_t kind, sysmelb_SourcePosition_t sourcePosition)
{
    sysmelb_ParseTreeNode_t *node = sysmelb_allocate(sizeof(sysmelb_ParseTreeNode_t));
    node->kind = kind;
    node->sourcePosition = sourcePosition;
    return node;
}

void sysmelb_dumpParseTree(sysmelb_ParseTreeNode_t *node)
{
    if(!node)
        return;

    switch(node->kind)
    {
    case ParseTreeErrorNode:
        printf("Error node: %s\n", node->errorNode.errorMessage);
        break;

    // Literals
    case ParseTreeLiteralIntegerNode:
        printf("LiteralInteger(%lld)", (long long int)node->literalInteger.value);
        break;
    case ParseTreeLiteralCharacterNode:
        printf("LiteralCharacter(%c)", node->literalCharacter.value);
        break;
    case ParseTreeLiteralFloatNode:
        printf("LiteralFloat(%f)", node->literalFloat.value);
        break;
    case ParseTreeLiteralStringNode:
        printf("LiteralString(\"%.*s\")", (int)node->literalString.stringSize, node->literalString.string);
        break;
    case ParseTreeLiteralSymbolNode:
        printf("LiteralSymbol(%.*s)", node->literalSymbol.internedSymbol->size, node->literalSymbol.internedSymbol->string);
        break;
    case ParseTreeLiteralValueNode:
        printf("LiteralValue(");
        sysmelb_printValue(node->literalValue.value);
        printf(")");
        break;

    // Identifier
    case ParseTreeIdentifierReference:
        printf("IdentififerRef(%.*s)", node->identifierReference.identifier->size, node->identifierReference.identifier->string);
        break;

    // Applications and messages.
    case ParseTreeFunctionApplication:
        printf("FunctionApplication(");
        sysmelb_dumpParseTree(node->functionApplication.functional);
        for(size_t i = 0; i < node->functionApplication.arguments.size; ++i)
        {
            printf(", ");
            sysmelb_dumpParseTree(node->functionApplication.arguments.elements[i]);
        }
        printf(")");
        break;
    case ParseTreeMessageSend:
        printf("MessageSend(");
        sysmelb_dumpParseTree(node->messageSend.receiver);
        printf(", ");
        sysmelb_dumpParseTree(node->messageSend.selector);

        for(size_t i = 0; i < node->messageSend.arguments.size; ++i)
        {
            printf(", ");
            sysmelb_dumpParseTree(node->messageSend.arguments.elements[i]);
        }
        printf(")");    
        break;
    case ParseTreeMessageCascade:
        printf("MessageCascade(");
        sysmelb_dumpParseTree(node->messageCascade.receiver);
        for(size_t i = 0; i < node->messageCascade.cascadedMessages.size; ++i)
        {
            printf(", ");
            sysmelb_dumpParseTree(node->messageCascade.cascadedMessages.elements[i]);
        }
        printf(")");    
        break;
    case ParseTreeCascadedMessage:
        printf("CascadedMessage(");
        sysmelb_dumpParseTree(node->cascadedMessage.selector);
        printf(", ");

        for(size_t i = 0; i < node->cascadedMessage.arguments.size; ++i)
        {
            printf(", ");
            sysmelb_dumpParseTree(node->cascadedMessage.arguments.elements[i]);
        }
        printf(")");
        break;
    case ParseTreeBinaryOperatorSequence:
        printf("BinopSequence(");
        for(size_t i = 0; i < node->binaryOperatorSequence.elements.size; ++i)
        {
            if (i != 0)
                printf(", ");
            sysmelb_dumpParseTree(node->binaryOperatorSequence.elements.elements[i]);
        }
        printf(")");
        break;


    // Sequences, array, tuple
    case ParseTreeSequence:
        printf("ParseTreeSequence(");
        for(size_t i = 0; i < node->sequence.elements.size; ++i)
        {
            if( i!= 0 )
                printf(", ");
            sysmelb_dumpParseTree(node->sequence.elements.elements[i]);
        }
        printf(")");
        break;
    case ParseTreeTuple:
        printf("ParseTreeTuple(");
        for(size_t i = 0; i < node->tuple.elements.size; ++i)
        {
            if( i!= 0 )
                printf(", ");
            sysmelb_dumpParseTree(node->tuple.elements.elements[i]);
        }
        printf(")");
        break;
    case ParseTreeArray:
        printf("ParseTreeArray(");
        for(size_t i = 0; i < node->array.elements.size; ++i)
        {
            if( i!= 0 )
                printf(", ");
            sysmelb_dumpParseTree(node->array.elements.elements[i]);
        }
        printf(")");
        break;
    case ParseTreeByteArray:
        printf("ParseTreeByteArray(");
        for(size_t i = 0; i < node->byteArray.elements.size; ++i)
        {
            if( i!= 0 )
                printf(", ");
            sysmelb_dumpParseTree(node->byteArray.elements.elements[i]);
        }
        printf(")");
        break;

    // ImmutableDictionary
    case ParseTreeImmutableDictionary:
        printf("ParseTreeImmutableDictionary(");
        for(size_t i = 0; i < node->dictionary.elements.size; ++i)
        {
            if( i!= 0 )
                printf(", ");
            sysmelb_dumpParseTree(node->dictionary.elements.elements[i]);
        }
        printf(")");
        break;
    case ParseTreeAssociation:
        printf("ParseTreeAssociation(");
        sysmelb_dumpParseTree(node->association.key);
        if(node->association.value)
        {
            printf(", ");
            sysmelb_dumpParseTree(node->association.value);
        }
        printf(")");
        break;

    // Blocks
    case ParseTreeBlockClosure:
        printf("ParseTreeBlockClosure(");
        sysmelb_dumpParseTree(node->blockClosure.functionType);
        printf(", ");
        sysmelb_dumpParseTree(node->blockClosure.body);
        printf(")");
        break;
    case ParseTreeLexicalBlock:
        printf("ParseTreeLexicalBlock(");
        sysmelb_dumpParseTree(node->lexicalBlock.expression);
        printf(")");
        break;

    // Macro operators
    case ParseTreeQuote:
        printf("ParseTreeQuote(");
        sysmelb_dumpParseTree(node->quote.expression);
        printf(")");
        break;
    case ParseTreeQuasiQuote:
        printf("ParseTreeQuasiQuote(");
        sysmelb_dumpParseTree(node->quasiQuote.expression);
        printf(")");
        break;
    case ParseTreeQuasiUnquote:
        printf("ParseTreeQuasiQuote(");
        sysmelb_dumpParseTree(node->quasiUnquote.expression);
        printf(")");
        break;
    case ParseTreeSplice:
        printf("ParseTreeQuasiQuote(");
        sysmelb_dumpParseTree(node->splice.expression);
        printf(")");
        break;

    // Binding and pattern matching
    case ParseTreeFunctionalDependentType:
        printf("ParseTreeFunctionalDependentPattern(");
        size_t argumentCount = node->functionalDependentType.argumentDefinition.size;
        for(size_t i = 0; i < argumentCount; ++i)
        {
            if(i != 0)
                printf(" ");
            sysmelb_dumpParseTree(node->functionalDependentType.argumentDefinition.elements[i]);
        }
        printf(" :: ");
        sysmelb_dumpParseTree(node->functionalDependentType.resultTypeExpression);
        printf(")");
        break;
    case ParseTreeBindableName:
        printf("ParseTreeBindableName(");
        sysmelb_dumpParseTree(node->bindableName.nameExpression);
        printf(" hasPostType(%s) ", node->bindableName.hasPostTypeExpression ? "yes" : "no");
        sysmelb_dumpParseTree(node->bindableName.typeExpression);
        printf(")");
        break;

    // Assignment
    case ParseTreeAssignment:
        printf("ParseTreeAssignment(");
        sysmelb_dumpParseTree(node->assignment.store);
        printf(" := ");
        sysmelb_dumpParseTree(node->assignment.value);
        printf(")");
        break;

    // Control flow
    case ParseTreeIfSelection:
        printf("ParseTreeIfSelection(");
        sysmelb_dumpParseTree(node->ifSelection.condition);
        printf(" ifTrue: ");
        sysmelb_dumpParseTree(node->ifSelection.trueExpression);
        printf(" ifFalse: ");
        sysmelb_dumpParseTree(node->ifSelection.falseExpression);
        printf(")");
        break;
    case ParseTreeWhileLoop:
        printf("ParseTreeWhileLoop(");
        sysmelb_dumpParseTree(node->whileLoop.condition);
        printf(" do: ");
        sysmelb_dumpParseTree(node->whileLoop.body);
        printf(" continueWith: ");
        sysmelb_dumpParseTree(node->whileLoop.continueExpression);
        printf(")");
        break;
    case ParseTreeReturnValue:
        printf("ParseTreeReturnValue(");
        sysmelb_dumpParseTree(node->returnExpression.valueExpression);
        printf(")");
        break;
    default:
        abort();
    }
}

int sysmelb_visitForDisplayingAndCountingErrors(sysmelb_ParseTreeNode_t *node)
{
    if(node)
        return 0;
    int errorCount = 0;
    switch(node->kind)
    {
        // Error
    case ParseTreeErrorNode:
        sysmelb_errorPrintf(node->sourcePosition, "Parse error: %s\n", node->errorNode.errorMessage);
        return 1;
    
    // Literals
    case ParseTreeLiteralIntegerNode:
    case ParseTreeLiteralCharacterNode:
    case ParseTreeLiteralFloatNode:
    case ParseTreeLiteralStringNode:
    case ParseTreeLiteralSymbolNode:
    case ParseTreeLiteralValueNode:
        return 0;

    // Identifiers
    case ParseTreeIdentifierReference: return 0;

    // Functions and message send
    case ParseTreeFunctionApplication:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->functionApplication.functional);
        for(size_t i = 0; i <node->functionApplication.arguments.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->functionApplication.arguments.elements[i]);
        return errorCount;
    }
    
    case ParseTreeMessageSend:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->messageSend.receiver);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->messageSend.selector);
        for(size_t i = 0; i <node->messageSend.arguments.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->messageSend.arguments.elements[i]);
        return errorCount;
    }   
    case ParseTreeMessageCascade:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->messageCascade.receiver);
    
        for(size_t i = 0; i < node->messageCascade.cascadedMessages.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->messageCascade.cascadedMessages.elements[i]);
        return errorCount;        
    }
    case ParseTreeCascadedMessage:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->cascadedMessage.selector);
        for(size_t i = 0; i <node->cascadedMessage.arguments.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->cascadedMessage.arguments.elements[i]);
        return errorCount;        
    }
    case ParseTreeBinaryOperatorSequence:
    {
        for(size_t i = 0; i <node->binaryOperatorSequence.elements.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->binaryOperatorSequence.elements.elements[i]);
        return errorCount;
    }

    // Sequences, array, tuples
    case ParseTreeSequence:
    {
        for(size_t i = 0; i <node->sequence.elements.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->sequence.elements.elements[i]);
        return errorCount;
    }
    case ParseTreeTuple:
    {
        for(size_t i = 0; i <node->tuple.elements.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->tuple.elements.elements[i]);
        return errorCount;
    }
    case ParseTreeArray:
    {
        for(size_t i = 0; i <node->array.elements.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->array.elements.elements[i]);
        return errorCount;
    }
    case ParseTreeByteArray:
    {
        for(size_t i = 0; i <node->byteArray.elements.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->byteArray.elements.elements[i]);
        return errorCount;
    }

    // ImmutableDictionary
    case ParseTreeAssociation:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->association.key);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->association.value);
        return errorCount;
    }
    case ParseTreeImmutableDictionary:
    {
        for(size_t i = 0; i <node->dictionary.elements.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->dictionary.elements.elements[i]);
        return errorCount;
    }

    // Blocks
    case ParseTreeBlockClosure:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->blockClosure.functionType);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->blockClosure.body);
        return errorCount;
    }
    case ParseTreeLexicalBlock:
    {
        return sysmelb_visitForDisplayingAndCountingErrors(node->lexicalBlock.expression);
    }

    // Macro operators
    case ParseTreeQuote:
        return sysmelb_visitForDisplayingAndCountingErrors(node->quote.expression);
    case ParseTreeQuasiQuote:
        return sysmelb_visitForDisplayingAndCountingErrors(node->quasiQuote.expression);
    case ParseTreeQuasiUnquote:
        return sysmelb_visitForDisplayingAndCountingErrors(node->quasiUnquote.expression);
    case ParseTreeSplice:
        return sysmelb_visitForDisplayingAndCountingErrors(node->splice.expression);

    // Binding and pattern matching
    case ParseTreeFunctionalDependentType:
    {
        for(size_t i = 0; i < node->functionalDependentType.argumentDefinition.size; ++i)
            errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->functionalDependentType.argumentDefinition.elements[i]);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->functionalDependentType.resultTypeExpression);
        return errorCount;
    }
    case ParseTreeBindableName:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->bindableName.nameExpression);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->bindableName.typeExpression);
        return errorCount;
    }

    // Assignment
    case ParseTreeAssignment:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->assignment.store);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->assignment.value);
        return errorCount;
    }

    // Closure and functions
    case ParseTreeFunction:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->function.functionDependentType);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->function.bodyExpression);
        return errorCount;
    }

    // Control flow. Exposed via macros
    case ParseTreeIfSelection:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->ifSelection.condition);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->ifSelection.trueExpression);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->ifSelection.falseExpression);
        return errorCount;
    }
    case ParseTreeWhileLoop:
    {
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->whileLoop.condition);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->whileLoop.body);
        errorCount += sysmelb_visitForDisplayingAndCountingErrors(node->whileLoop.continueExpression);
        return errorCount;
    }
    case ParseTreeReturnValue:
    {
        return sysmelb_visitForDisplayingAndCountingErrors(node->returnExpression.valueExpression);
    }
    
    /// Namespaces
    case ParseTreeNamespaceDefinition:
        return sysmelb_visitForDisplayingAndCountingErrors(node->namespaceDefinition.definition);
    default: abort();
    }
}