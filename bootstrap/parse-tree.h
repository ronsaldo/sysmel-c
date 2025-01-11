#ifndef SYSMELB_PARSE_TREE_H
#define SYSMELB_PARSE_TREE_H

#pragma once

#include "source-code.h"
#include <stddef.h>
#include <stdint.h>

#define SYSMELB_PARSE_TREE_MAX_ARGUMENTS 8

typedef enum sysmelb_ParseTreeNodeKind_e {
    // Error
    ParseTreeErrorNode,
    
    // Literals
    ParseTreeLiteralIntegerNode,
    ParseTreeLiteralCharacterNode,
    ParseTreeLiteralFloatNode,
    ParseTreeLiteralStringNode,
    ParseTreeLiteralSymbolNode,

    // Functions and message send
    ParseTreeFunctionApplication,
    ParseTreeMessageSend,
    ParseTreeMessageCascade,
    ParseTreeCascadedMessage,
} sysmelb_ParseTreeNodeKind_t;

typedef struct sysmelb_ParseTreeNode_s sysmelb_ParseTreeNode_t;

// Errors
typedef struct sysmelb_ParseTreeErrorNode_s {
    const char *errorMessage;
} sysmelb_ParseTreeErrorNode_t;

// Literals
typedef struct sysmelb_ParseTreeLiteralIntegerNode_s {
    int64_t value;
} sysmelb_ParseTreeLiteralIntegerNode_t;

typedef struct sysmelb_ParseTreeLiteralCharacterNode_s {
    int32_t value;
} sysmelb_ParseTreeLiteralCharacterNode_t;

typedef struct sysmelb_ParseTreeLiteralFloatNode_s {
    double value;
} sysmelb_ParseTreeLiteralFloatNode_t;

typedef struct sysmelb_ParseTreeLiteralStringNode_s {
    size_t stringSize;
    const char *string;
} sysmelb_ParseTreeLiteralStringNode_t;

typedef struct sysmelb_ParseTreeLiteralSymbolNode_s {
    const char *internedSymbol;
} sysmelb_ParseTreeLiteralSymbolNode_t;

// Function application and message send
typedef struct sysmelb_ParseTreeFunctionApplication_s {
    sysmelb_ParseTreeNode_t *functional;
    size_t argumentCount;
    sysmelb_ParseTreeNode_t *arguments[SYSMELB_PARSE_TREE_MAX_ARGUMENTS];
} sysmelb_ParseTreeFunctionApplication_t;

typedef struct sysmelb_ParseTreeMessageSend_s {
    sysmelb_ParseTreeNode_t *receiver;
    sysmelb_ParseTreeNode_t *selector;
    size_t argumentCount;
    sysmelb_ParseTreeNode_t *arguments[SYSMELB_PARSE_TREE_MAX_ARGUMENTS];
} sysmelb_ParseTreeMessageSend_t;

typedef struct sysmelb_ParseTreeMessageCascade_s {
    sysmelb_ParseTreeNode_t *receiver;
    size_t cascadeSize;
    sysmelb_ParseTreeNode_t *cascadedMessages[SYSMELB_PARSE_TREE_MAX_ARGUMENTS];
} sysmelb_ParseTreeMessageCascade_t;

typedef struct sysmelb_ParseTreeCascadedMessage_s {
    sysmelb_ParseTreeNode_t *selector;
    size_t argumentCount;
    sysmelb_ParseTreeNode_t *arguments[SYSMELB_PARSE_TREE_MAX_ARGUMENTS];
} sysmelb_ParseTreeCascadedMessage_t;

typedef struct sysmelb_ParseTreeNode_s {
    sysmelb_ParseTreeNodeKind_t kind;
    sysmelb_SourcePosition_t sourcePosition;
    union
    {
        // Error
        sysmelb_ParseTreeErrorNode_t errorNode;
        
        // Literals
        sysmelb_ParseTreeLiteralIntegerNode_t literalInteger;
        sysmelb_ParseTreeLiteralCharacterNode_t literalCharacter;
        sysmelb_ParseTreeLiteralFloatNode_t literalFloat;
        sysmelb_ParseTreeLiteralStringNode_t literalString;
        sysmelb_ParseTreeLiteralSymbolNode_t literalSymbol;

        // Functions and message send
        sysmelb_ParseTreeFunctionApplication_t functionApplication;
        sysmelb_ParseTreeMessageSend_t messageSend;
        sysmelb_ParseTreeMessageCascade_t messageCascade;
        sysmelb_ParseTreeCascadedMessage_t cascadedMessage;
    };
} sysmelb_ParseTreeNode_t;

sysmelb_ParseTreeNode_t *sysmelb_newParseTreeNode(sysmelb_ParseTreeNodeKind_t kind, sysmelb_SourcePosition_t sourcePosition);
void sysmelb_dumpParseTree(sysmelb_ParseTreeNode_t *node);

#endif //SYSMELB_PARSE_TREE_H