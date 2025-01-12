#include "parse-tree.h"
#include "memory.h"
#include <stdio.h>

sysmelb_ParseTreeNode_t *sysmelb_newParseTreeNode(sysmelb_ParseTreeNodeKind_t kind, sysmelb_SourcePosition_t sourcePosition)
{
    sysmelb_ParseTreeNode_t *node = sysmelb_allocate(sizeof(sysmelb_ParseTreeNode_t));
    memset(node, 0, sizeof(sysmelb_ParseTreeNode_t));
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
    case ParseTreeLiteralIntegerNode:
        printf("LiteralInteger(%lld)", node->literalInteger.value);
        break;
    case ParseTreeLiteralCharacterNode:
        printf("LiteralCharacter(%c)", node->literalCharacter.value);
        break;
    case ParseTreeLiteralFloatNode:
        printf("LiteralFloat(%f)", node->literalFloat.value);
        break;
    case ParseTreeLiteralStringNode:
        printf("LiteralString(%.*s)", (int)node->literalString.stringSize, node->literalString.string);
        break;
    case ParseTreeLiteralSymbolNode:
        printf("LiteralSymbol(%s)", node->literalSymbol.internedSymbol);
        break;
    case ParseTreeFunctionApplication:
        printf("FunctionApplication(");
        sysmelb_dumpParseTree(node->functionApplication.functional);
        for(size_t i = 0; i < node->functionApplication.argumentCount; ++i)
        {
            printf(", ");
            sysmelb_dumpParseTree(node->functionApplication.arguments[i]);
        }
        printf(")");
        break;
    case ParseTreeMessageSend:
        printf("MessageSend(");
        sysmelb_dumpParseTree(node->messageSend.receiver);
        printf(", ");
        sysmelb_dumpParseTree(node->messageSend.selector);

        for(size_t i = 0; i < node->messageSend.argumentCount; ++i)
        {
            printf(", ");
            sysmelb_dumpParseTree(node->messageSend.arguments[i]);
        }
        printf(")");    
        break;
    case ParseTreeMessageCascade:
        printf("MessageCascade(");
        sysmelb_dumpParseTree(node->messageCascade.receiver);
        for(size_t i = 0; i < node->messageCascade.cascadeSize; ++i)
        {
            printf(", ");
            sysmelb_dumpParseTree(node->messageCascade.cascadedMessages[i]);
        }
        printf(")");    
        break;
    case ParseTreeCascadedMessage:
        printf("CascadedMessage(");
        sysmelb_dumpParseTree(node->cascadedMessage.selector);
        printf(", ");

        for(size_t i = 0; i < node->cascadedMessage.argumentCount; ++i)
        {
            printf(", ");
            sysmelb_dumpParseTree(node->cascadedMessage.arguments[i]);
        }
        printf(")");
        break;
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
    default:
        abort();
    }
}