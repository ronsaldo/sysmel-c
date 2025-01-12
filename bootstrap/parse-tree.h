#ifndef SYSMELB_PARSE_TREE_H
#define SYSMELB_PARSE_TREE_H

#pragma once

#include "source-code.h"
#include "symbol.h"
#include <stddef.h>
#include <stdint.h>

#define SYSMELB_PARSE_TREE_MAX_ARGUMENTS 8
typedef int64_t sysmelb_IntegerLiteralType_t;

typedef enum sysmelb_ParseTreeNodeKind_e {
    // Error
    ParseTreeErrorNode,
    
    // Literals
    ParseTreeLiteralIntegerNode,
    ParseTreeLiteralCharacterNode,
    ParseTreeLiteralFloatNode,
    ParseTreeLiteralStringNode,
    ParseTreeLiteralSymbolNode,

    // Identifiers
    ParseTreeIdentifierReference,

    // Functions and message send
    ParseTreeFunctionApplication,
    ParseTreeMessageSend,
    ParseTreeMessageCascade,
    ParseTreeCascadedMessage,

    // Sequences, array, tuples
    ParseTreeSequence,
    ParseTreeTuple,
    ParseTreeArray,
    ParseTreeByteArray,

    // Dictionary
    ParseTreeAssociation,
    ParseTreeDictionary,
} sysmelb_ParseTreeNodeKind_t;

typedef struct sysmelb_ParseTreeNode_s sysmelb_ParseTreeNode_t;

typedef struct sysmelb_ParseTreeNodeDynArray_s {
    size_t capacity;
    size_t size;
    sysmelb_ParseTreeNode_t **elements;
} sysmelb_ParseTreeNodeDynArray_t;

// Errors
typedef struct sysmelb_ParseTreeErrorNode_s {
    const char *errorMessage;
    sysmelb_ParseTreeNode_t *innerNode;
} sysmelb_ParseTreeErrorNode_t;

// Literals
typedef struct sysmelb_ParseTreeLiteralIntegerNode_s {
    sysmelb_IntegerLiteralType_t value;
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
    sysmelb_symbol_t *internedSymbol;
} sysmelb_ParseTreeLiteralSymbolNode_t;

// Identifier reference.
typedef struct sysmelb_ParseTreeIdentifierReference_s {
    sysmelb_symbol_t *identifier;
} sysmelb_ParseTreeIdentifierReference_t;

// Function application and message send.
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


// Sequences
typedef struct sysmelb_ParseTreeSequence_s {
    sysmelb_ParseTreeNodeDynArray_t elements;
} sysmelb_ParseTreeSequence_t;

typedef struct sysmelb_ParseTreeTuple_s {
    sysmelb_ParseTreeNodeDynArray_t elements;
} sysmelb_ParseTreeTuple_t;

typedef struct sysmelb_ParseTreeArray_s {
    sysmelb_ParseTreeNodeDynArray_t elements;
} sysmelb_ParseTreeArray_t;

typedef struct sysmelb_ParseTreeByteArray_s {
    sysmelb_ParseTreeNodeDynArray_t elements;
} sysmelb_ParseTreeByteArray_t;

// Dictionary.
typedef struct sysmelb_ParseTreeDictionary_s {
    sysmelb_ParseTreeNodeDynArray_t elements;
} sysmelb_ParseTreeDictionary_t;

typedef struct sysmelb_ParseTreeAssociation_s {
    sysmelb_ParseTreeNode_t *key;
    sysmelb_ParseTreeNode_t *value;
} sysmelb_ParseTreeAssociation_t;

// Macro operators
typedef struct sysmelb_ParseTreeQuote_s {
    sysmelb_ParseTreeNode_t *expression;
} sysmelb_ParseTreeQuote_t;

// Tagged node union.
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

        // Identifiers
        sysmelb_ParseTreeIdentifierReference_t identifierReference;

        // Functions and message send
        sysmelb_ParseTreeFunctionApplication_t functionApplication;
        sysmelb_ParseTreeMessageSend_t messageSend;
        sysmelb_ParseTreeMessageCascade_t messageCascade;
        sysmelb_ParseTreeCascadedMessage_t cascadedMessage;

        // Sequence
        sysmelb_ParseTreeSequence_t sequence;
        sysmelb_ParseTreeTuple_t tuple;
        sysmelb_ParseTreeArray_t array;
        sysmelb_ParseTreeByteArray_t byteArray;

        // Dictionary
        sysmelb_ParseTreeDictionary_t dictionary;
        sysmelb_ParseTreeAssociation_t association;
    };
} sysmelb_ParseTreeNode_t;

void sysmelb_ParseTreeNodeDynArray_add(sysmelb_ParseTreeNodeDynArray_t *dynArray, sysmelb_ParseTreeNode_t *element);

sysmelb_ParseTreeNode_t *sysmelb_newParseTreeNode(sysmelb_ParseTreeNodeKind_t kind, sysmelb_SourcePosition_t sourcePosition);
void sysmelb_dumpParseTree(sysmelb_ParseTreeNode_t *node);

#endif //SYSMELB_PARSE_TREE_H