#ifndef SYSMELB_PARSE_TREE_H
#define SYSMELB_PARSE_TREE_H

#pragma once

#include "source-code.h"
#include "symbol.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

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
    ParseTreeBinaryOperatorSequence,

    // Sequences, array, tuples
    ParseTreeSequence,
    ParseTreeTuple,
    ParseTreeArray,
    ParseTreeByteArray,

    // Dictionary
    ParseTreeAssociation,
    ParseTreeDictionary,

    // Blocks
    ParseTreeBlockClosure,
    ParseTreeLexicalBlock,

    // Macro operators
    ParseTreeQuote,
    ParseTreeQuasiQuote,
    ParseTreeQuasiUnquote,
    ParseTreeSplice,

    // Binding and pattern matching
    ParseTreeBindPattern,
    ParseTreeFunctionalDependentPattern,
    ParseTreeBindableName,

    // Assignment
    ParseTreeAssignment,

    // Control flow. Exposed via macros
    ParseTreeIfSelection,
    ParseTreeWhileLoop,
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
    sysmelb_ParseTreeNodeDynArray_t arguments;
} sysmelb_ParseTreeFunctionApplication_t;

typedef struct sysmelb_ParseTreeMessageSend_s {
    sysmelb_ParseTreeNode_t *receiver;
    sysmelb_ParseTreeNode_t *selector;
    sysmelb_ParseTreeNodeDynArray_t arguments;
} sysmelb_ParseTreeMessageSend_t;

typedef struct sysmelb_ParseTreeMessageCascade_s {
    sysmelb_ParseTreeNode_t *receiver;
    sysmelb_ParseTreeNodeDynArray_t cascadedMessages;
} sysmelb_ParseTreeMessageCascade_t;

typedef struct sysmelb_ParseTreeCascadedMessage_s {
    sysmelb_ParseTreeNode_t *selector;
    sysmelb_ParseTreeNodeDynArray_t arguments;
} sysmelb_ParseTreeCascadedMessage_t;

typedef struct sysmelb_ParseTreeBinaryOperatorSequence_s {
    sysmelb_ParseTreeNodeDynArray_t elements;
} sysmelb_ParseTreeBinaryOperatorSequence_t;

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

// Blocks
typedef struct sysmelb_ParseTreeBlockClosure_s {
    sysmelb_ParseTreeNode_t *functionType;
    sysmelb_ParseTreeNode_t *body;
} sysmelb_ParseTreeBlockClosure_t;

typedef struct sysmelb_ParseTreeLexicalBlock_s {
    sysmelb_ParseTreeNode_t *expression;
} sysmelb_ParseTreeLexicalBlock_t;

// Macro operators
typedef struct sysmelb_ParseTreeQuote_s {
    sysmelb_ParseTreeNode_t *expression;
} sysmelb_ParseTreeQuote_t;

typedef struct sysmelb_ParseTreeQuasiQuote_s {
    sysmelb_ParseTreeNode_t *expression;
} sysmelb_ParseTreeQuasiQuote_t;

typedef struct sysmelb_ParseTreeQuasiUnquote_s {
    sysmelb_ParseTreeNode_t *expression;
} sysmelb_ParseTreeQuasiUnquote_t;

typedef struct sysmelb_ParseTreeSplice_s {
    sysmelb_ParseTreeNode_t *expression;
} sysmelb_ParseTreeSplice_t;

// Binding and pattern matching
typedef struct sysmelb_ParseTreeBindPattern_s {
    sysmelb_ParseTreeNode_t *pattern;
    sysmelb_ParseTreeNode_t *value;
} sysmelb_ParseTreeBindPattern_t;

typedef struct sysmelb_ParseTreeFunctionalDependentPattern_s {
    sysmelb_ParseTreeNode_t *argumentPattern;
    sysmelb_ParseTreeNode_t *resultValue;
    sysmelb_symbol_t *callingConvention;
} sysmelb_ParseTreeFunctionalDependentPattern_t;

typedef struct sysmelb_ParseTreeBindableName_s {
    sysmelb_ParseTreeNode_t *typeExpression;
    sysmelb_ParseTreeNode_t *nameExpression;
    bool isImplicit;
    bool isExistential;
    bool isVariadic;
    bool isMutable;
    bool hasPostTypeExpression;
    bool isPublic;
} sysmelb_ParseTreeBindableName_t;

// Assignment
typedef struct sysmelb_ParseTreeAssignment_s {
    sysmelb_ParseTreeNode_t *store;
    sysmelb_ParseTreeNode_t *value;
} sysmelb_ParseTreeAssignment_t;

// Control flow. Exposed via macros
typedef struct sysmelb_ParseTreeIfSelection_s {
    sysmelb_ParseTreeNode_t *condition;
    sysmelb_ParseTreeNode_t *trueExpression;
    sysmelb_ParseTreeNode_t *falseExpression;
} sysmelb_ParseTreeIfSelection_t;

typedef struct sysmelb_ParseTreeWhileLoop_s {
    sysmelb_ParseTreeNode_t *condition;
    sysmelb_ParseTreeNode_t *body;
    sysmelb_ParseTreeNode_t *continueExpression;
} sysmelb_ParseTreeWhileLoop_t;

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
        sysmelb_ParseTreeBinaryOperatorSequence_t binaryOperatorSequence;

        // Sequence
        sysmelb_ParseTreeSequence_t sequence;
        sysmelb_ParseTreeTuple_t tuple;
        sysmelb_ParseTreeArray_t array;
        sysmelb_ParseTreeByteArray_t byteArray;

        // Dictionary
        sysmelb_ParseTreeDictionary_t dictionary;
        sysmelb_ParseTreeAssociation_t association;

        // Blocks
        sysmelb_ParseTreeBlockClosure_t blockClosure;
        sysmelb_ParseTreeLexicalBlock_t lexicalBlock;

        // Macro operators
        sysmelb_ParseTreeQuote_t quote;
        sysmelb_ParseTreeQuasiQuote_t quasiQuote;
        sysmelb_ParseTreeQuasiUnquote_t quasiUnquote;
        sysmelb_ParseTreeSplice_t splice;

        // Binding and pattern matching.
        sysmelb_ParseTreeBindPattern_t bindPattern;
        sysmelb_ParseTreeFunctionalDependentPattern_t functionalDependentPattern;
        sysmelb_ParseTreeBindableName_t bindableName;

        // Assignment
        sysmelb_ParseTreeAssignment_t assignment;

        // Control flow
        sysmelb_ParseTreeIfSelection_t ifSelection;
        sysmelb_ParseTreeWhileLoop_t whileLoop;
    };
} sysmelb_ParseTreeNode_t;

void sysmelb_ParseTreeNodeDynArray_add(sysmelb_ParseTreeNodeDynArray_t *dynArray, sysmelb_ParseTreeNode_t *element);

sysmelb_ParseTreeNode_t *sysmelb_newParseTreeNode(sysmelb_ParseTreeNodeKind_t kind, sysmelb_SourcePosition_t sourcePosition);
void sysmelb_dumpParseTree(sysmelb_ParseTreeNode_t *node);

#endif //SYSMELB_PARSE_TREE_H