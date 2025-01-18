#include "parser.h"
#include "parse-tree.h"
#include "memory.h"
#include "string.h"
#include "symbol.h"
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct sysmelb_parserState_s sysmelb_parserState_t;
sysmelb_SourcePosition_t parserState_currentSourcePosition(sysmelb_parserState_t *state);
sysmelb_ParseTreeNodeDynArray_t parser_parseExpressionListUntilEndOrDelimiter(sysmelb_parserState_t *state, sysmelb_TokenKind_t delimiter);
sysmelb_ParseTreeNode_t *parser_parseSequenceUntilEndOrDelimiter(sysmelb_parserState_t *state, sysmelb_TokenKind_t delimiter);
sysmelb_ParseTreeNode_t *parser_parseUnaryPrefixExpression(sysmelb_parserState_t *state);
sysmelb_ParseTreeNode_t *parser_parseTerm(sysmelb_parserState_t *state);
sysmelb_ParseTreeNode_t *parser_parseImmutableDictionary(sysmelb_parserState_t *state);
sysmelb_ParseTreeNode_t *parser_parseBlock(sysmelb_parserState_t *state);
sysmelb_ParseTreeNode_t *parser_parseExpression(sysmelb_parserState_t *state);
sysmelb_ParseTreeNode_t *parser_parseBindableName(sysmelb_parserState_t *state);

void sysmelb_ParseTreeNodeDynArray_incrementCapacity(sysmelb_ParseTreeNodeDynArray_t *dynArray)
{
    size_t newCapacity = dynArray->capacity * 2;
    if (newCapacity < 16)
        newCapacity = 16;

    sysmelb_ParseTreeNode_t **newStorage = sysmelb_allocate(newCapacity * sizeof(sysmelb_ParseTreeNode_t *));
    memcpy(newStorage, dynArray->elements, dynArray->size * sizeof(sysmelb_ParseTreeNode_t *));
    sysmelb_freeAllocation(dynArray->elements);
    dynArray->capacity = newCapacity;
    dynArray->elements = newStorage;
}

void sysmelb_ParseTreeNodeDynArray_add(sysmelb_ParseTreeNodeDynArray_t *dynArray, sysmelb_ParseTreeNode_t *element)
{
    if (dynArray->size >= dynArray->capacity)
        sysmelb_ParseTreeNodeDynArray_incrementCapacity(dynArray);

    dynArray->elements[dynArray->size++] = element;
}

struct sysmelb_parserState_s
{
    sysmelb_SourceCode_t *sourceCode;
    size_t tokenCount;
    sysmelb_ScannerToken_t *tokens;
    size_t position;
};

bool parserState_atEnd(sysmelb_parserState_t *state)
{
    return state->position >= state->tokenCount;
}

sysmelb_TokenKind_t parserState_peekKind(sysmelb_parserState_t *state, int offset)
{
    size_t peekPosition = state->position + offset;
    if (peekPosition < state->tokenCount)
        return state->tokens[peekPosition].kind;
    else
        return SysmelTokenEndOfSource;
}

sysmelb_ScannerToken_t *parserState_peek(sysmelb_parserState_t *state, int offset)
{
    size_t peekPosition = state->position + offset;
    if (peekPosition < state->tokenCount)
        return &state->tokens[peekPosition];
    else
        return NULL;
}

void parserState_advance(sysmelb_parserState_t *state)
{
    assert(state->position < state->tokenCount);
    ++state->position;
}

sysmelb_ScannerToken_t *parserState_next(sysmelb_parserState_t *state)
{
    assert(state->position < state->tokenCount);
    sysmelb_ScannerToken_t *token = &state->tokens[state->position];
    ++state->position;
    return token;
}

sysmelb_ParseTreeNode_t *parserState_advanceWithExpectedError(sysmelb_parserState_t *state, const char *message)
{
    if (parserState_peekKind(state, 0) == SysmelTokenError)
    {
        sysmelb_ScannerToken_t *errorToken = parserState_next(state);
        sysmelb_ParseTreeNode_t *errorNode = sysmelb_newParseTreeNode(ParseTreeErrorNode, errorToken->sourcePosition);
        errorNode->errorNode.errorMessage = errorToken->errorMessage;
        return errorNode;
    }
    else if (parserState_atEnd(state))
    {
        sysmelb_ParseTreeNode_t *errorNode = sysmelb_newParseTreeNode(ParseTreeErrorNode, parserState_currentSourcePosition(state));
        errorNode->errorNode.errorMessage = message;
        return errorNode;
    }
    else
    {
        sysmelb_ParseTreeNode_t *errorNode = sysmelb_newParseTreeNode(ParseTreeErrorNode, parserState_currentSourcePosition(state));
        parserState_advance(state);
        errorNode->errorNode.errorMessage = message;
        return errorNode;
    }
}

size_t parserState_memento(sysmelb_parserState_t *state)
{
    return state->position;
}

void parserState_restore(sysmelb_parserState_t *state, size_t memento)
{
    state->position = memento;
}

sysmelb_SourcePosition_t parserState_previousSourcePosition(sysmelb_parserState_t *state)
{
    assert(state->position > 0);
    return state->tokens[state->position - 1].sourcePosition;
}

sysmelb_SourcePosition_t parserState_currentSourcePosition(sysmelb_parserState_t *state)
{
    if (state->position < state->tokenCount)
        return state->tokens[state->position].sourcePosition;

    assert(state->tokenCount > 0);
    // assert(!tokens->at(tokens->size() - 1)->kind == TokenKind::EndOfSource);
    return state->tokens[state->tokenCount - 1].sourcePosition;
}

sysmelb_SourcePosition_t parserState_sourcePositionFrom(sysmelb_parserState_t *state, size_t startingPosition)
{
    assert(startingPosition < state->tokenCount);
    sysmelb_SourcePosition_t startSourcePosition = state->tokens[startingPosition].sourcePosition;
    if (state->position > 0)
    {
        sysmelb_SourcePosition_t endSourcePosition = parserState_previousSourcePosition(state);
        return sysmelb_sourcePosition_to(&startSourcePosition, &endSourcePosition);
    }
    else
    {
        sysmelb_SourcePosition_t endSourcePosition = parserState_previousSourcePosition(state);
        return sysmelb_sourcePosition_until(&startSourcePosition, &endSourcePosition);
    }
}

sysmelb_ParseTreeNode_t *parserState_makeErrorAtCurrentSourcePosition(sysmelb_parserState_t *state, const char *errorMessage)
{
    sysmelb_ParseTreeNode_t *errorNode = sysmelb_newParseTreeNode(ParseTreeErrorNode, parserState_currentSourcePosition(state));
    errorNode->errorNode.errorMessage = errorMessage;
    return errorNode;
}

sysmelb_ParseTreeNode_t *parserState_expectAddingErrorToNode(sysmelb_parserState_t *state, sysmelb_TokenKind_t expectedKind, sysmelb_ParseTreeNode_t *node)
{
    if (parserState_peekKind(state, 0) == expectedKind)
    {
        parserState_advance(state);
        return node;
    }

    sysmelb_ParseTreeNode_t *errorNode = sysmelb_newParseTreeNode(ParseTreeErrorNode, parserState_currentSourcePosition(state));
    errorNode->errorNode.errorMessage = "Expected a specific token kind.";
    errorNode->errorNode.innerNode = node;
    return errorNode;
}

sysmelb_IntegerLiteralType_t parseIntegerConstant(size_t constantStringSize, const char *constantString)
{
    sysmelb_IntegerLiteralType_t result = 0;
    sysmelb_IntegerLiteralType_t radix = 10;
    bool hasSeenRadix = false;

    for (size_t i = 0; i < constantStringSize; ++i)
    {
        char c = constantString[i];
        if (!hasSeenRadix && (c == 'r' || c == 'R'))
        {
            hasSeenRadix = true;
            radix = result;
            result = 0;
        }
        else
        {
            if ('0' <= c && c <= '9')
                result = result * radix + (sysmelb_IntegerLiteralType_t)(c - '0');
            else if ('A' <= c && c <= 'Z')
                result = result * radix + (sysmelb_IntegerLiteralType_t)(c - 'A' + 10);
            else if ('a' <= c && c <= 'z')
                result = result * radix + (sysmelb_IntegerLiteralType_t)(c - 'a' + 10);
        }
    }
    return result;
}

sysmelb_ParseTreeNode_t *parseLiteralInteger(sysmelb_parserState_t *state)
{
    sysmelb_ScannerToken_t *token = parserState_next(state);
    assert(token->kind == SysmelTokenNat);

    sysmelb_IntegerLiteralType_t parsedConstant = parseIntegerConstant(token->textSize, token->textPosition);
    sysmelb_ParseTreeNode_t *literal = sysmelb_newParseTreeNode(ParseTreeLiteralIntegerNode, token->sourcePosition);
    literal->literalInteger.value = parsedConstant;
    return literal;
}

sysmelb_ParseTreeNode_t *parseLiteralFloat(sysmelb_parserState_t *state)
{
    sysmelb_ScannerToken_t *token = parserState_next(state);
    assert(token->kind == SysmelTokenFloat);
    char parseBuffer[64];
    memset(parseBuffer, 0, sizeof(parseBuffer));
    memcpy(parseBuffer, token->textPosition, token->textSize);
    double value = atof(parseBuffer);

    sysmelb_ParseTreeNode_t *literal = sysmelb_newParseTreeNode(ParseTreeLiteralFloatNode, token->sourcePosition);
    literal->literalFloat.value = value;
    return literal;
}

char *parseCEscapedString(size_t stringSize, const char *string, size_t *outSize)
{
    char *result = sysmelb_allocate(stringSize + 1);
    memset(result, 0, stringSize + 1);
    char *dest = result;

    for (size_t i = 0; i < stringSize; ++i)
    {
        char c = string[i];
        if (c == '\\')
        {
            char c1 = string[++i];
            switch (c1)
            {
            case 'n':
                *dest++ = '\n';
                break;
            case 'r':
                *dest++ = '\r';
                break;
            case 't':
                *dest++ = '\t';
                break;
            default:
                *dest++ = c1;
                break;
            }
        }
        else
        {
            *dest++ = c;
        }
    }

    if (outSize)
        *outSize = dest - result;
    return result;
}

sysmelb_ParseTreeNode_t *parseLiteralCharacter(sysmelb_parserState_t *state)
{
    sysmelb_ScannerToken_t *token = parserState_next(state);
    assert(token->kind == SysmelTokenCharacter);
    char *parsedString = parseCEscapedString(token->textSize - 2, token->textPosition + 1, NULL);

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeLiteralCharacterNode, token->sourcePosition);
    node->literalCharacter.value = *parsedString;
    return node;
}

sysmelb_ParseTreeNode_t *parseLiteralString(sysmelb_parserState_t *state)
{
    sysmelb_ScannerToken_t *token = parserState_next(state);
    assert(token->kind == SysmelTokenString);
    size_t parsedStringSize;
    char *parsedString = parseCEscapedString(token->textSize - 2, token->textPosition + 1, &parsedStringSize);

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeLiteralStringNode, token->sourcePosition);
    node->literalString.string = parsedString;
    node->literalString.stringSize = parsedStringSize;
    return node;
}

sysmelb_ParseTreeNode_t *parseLiteralSymbol(sysmelb_parserState_t *state)
{
    sysmelb_ScannerToken_t *token = parserState_next(state);
    assert(token->kind == SysmelTokenSymbol);

    const char *symbolValue = token->textPosition + 1;
    size_t symbolValueSize = token->textSize - 1;
    if (symbolValue[0] == '"')
    {
        symbolValue = symbolValue + 1;
        symbolValueSize = symbolValueSize - 2;
        symbolValue = parseCEscapedString(symbolValueSize, symbolValue, &symbolValueSize);
    }

    sysmelb_ParseTreeNode_t *literal = sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, token->sourcePosition);
    literal->literalSymbol.internedSymbol = sysmelb_internSymbol(symbolValueSize, symbolValue);
    return literal;
}

sysmelb_ParseTreeNode_t *parser_parseLiteral(sysmelb_parserState_t *state)
{
    switch (parserState_peekKind(state, 0))
    {
    case SysmelTokenNat:
        return parseLiteralInteger(state);
    case SysmelTokenFloat:
        return parseLiteralFloat(state);
    case SysmelTokenCharacter:
        return parseLiteralCharacter(state);
    case SysmelTokenString:
        return parseLiteralString(state);
    case SysmelTokenSymbol:
        return parseLiteralSymbol(state);
    default:
        return parserState_advanceWithExpectedError(state, "Expected a literal");
    }
}

sysmelb_ParseTreeNode_t *parser_parseIdentifier(sysmelb_parserState_t *state)
{
    sysmelb_ScannerToken_t *token = parserState_next(state);
    assert(token->kind == SysmelTokenIdentifier);

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeIdentifierReference, token->sourcePosition);
    node->identifierReference.identifier = sysmelb_internSymbol(token->textSize, token->textPosition);
    return node;
}

bool parser_isBinaryExpressionOperator(sysmelb_TokenKind_t kind)
{
    switch (kind)
    {
    case SysmelTokenOperator:
    case SysmelTokenStar:
    case SysmelTokenLessThan:
    case SysmelTokenGreaterThan:
    case SysmelTokenBar:
        return true;
    default:
        return false;
    }
}

sysmelb_ParseTreeNode_t *parser_parseParenthesis(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenLeftParent);
    parserState_advance(state);

    if (parser_isBinaryExpressionOperator(parserState_peekKind(state, 0)) && parserState_peekKind(state, 1) == SysmelTokenRightParent)
    {
        sysmelb_ScannerToken_t *token = parserState_next(state);
        parserState_advance(state);

        sysmelb_symbol_t *tokenSymbol = sysmelb_internSymbol(token->textSize, token->textPosition);
        sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeIdentifierReference, token->sourcePosition);
        node->identifierReference.identifier = tokenSymbol;
        return node;
    }

    if (parserState_peekKind(state, 0) == SysmelTokenRightParent)
    {
        parserState_advance(state);
        sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeTuple, parserState_sourcePositionFrom(state, startPosition));
        return node;
    }

    sysmelb_ParseTreeNode_t *expression = parser_parseSequenceUntilEndOrDelimiter(state, SysmelTokenRightParent);
    expression = parserState_expectAddingErrorToNode(state, SysmelTokenRightParent, expression);
    return expression;
}

sysmelb_ParseTreeNode_t *parser_parseArray(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenLeftBracket);
    parserState_advance(state);

    sysmelb_ParseTreeNodeDynArray_t expressions = parser_parseExpressionListUntilEndOrDelimiter(state, SysmelTokenRightBracket);

    if (parserState_peekKind(state, 0) == SysmelTokenRightBracket)
    {
        parserState_advance(state);
    }
    else
    {
        // TODO: add an error
        abort();
    }

    // Array
    sysmelb_ParseTreeNode_t *array = sysmelb_newParseTreeNode(ParseTreeArray, parserState_sourcePositionFrom(state, startPosition));
    array->array.elements = expressions;
    return array;
}

sysmelb_ParseTreeNode_t *parser_parseByteArray(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenByteArrayStart);
    parserState_advance(state);

    sysmelb_ParseTreeNodeDynArray_t expressions = parser_parseExpressionListUntilEndOrDelimiter(state, SysmelTokenRightBracket);

    if (parserState_peekKind(state, 0) == SysmelTokenRightBracket)
    {
        parserState_advance(state);
    }
    else
    {
        // TODO: an error
        abort();
    }

    // Byte
    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeByteArray, parserState_sourcePositionFrom(state, startPosition));
    node->byteArray.elements = expressions;
    return node;
}

bool parser_isUnaryPostfixTokenKind(sysmelb_TokenKind_t kind)
{
    switch (kind)
    {
    case SysmelTokenIdentifier:
    case SysmelTokenLeftParent:
    case SysmelTokenLeftBracket:
    case SysmelTokenLeftCurlyBracket:
    case SysmelTokenByteArrayStart:
    case SysmelTokenImmutableDictionaryStart:
        return true;
    default:
        return false;
    }
}

sysmelb_ParseTreeNode_t *parser_parseUnaryPostfixExpression(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    sysmelb_ParseTreeNode_t *receiver = parser_parseTerm(state);

    while (parser_isUnaryPostfixTokenKind(parserState_peekKind(state, 0)))
    {
        sysmelb_ScannerToken_t *token = parserState_peek(state, 0);
        switch (token->kind)
        {
        case SysmelTokenIdentifier:
        {
            parserState_advance(state);
            sysmelb_symbol_t *selectorSymbol = sysmelb_internSymbol(token->textSize, token->textPosition);
            sysmelb_ParseTreeNode_t *selector = sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, token->sourcePosition);
            selector->literalSymbol.internedSymbol = selectorSymbol;

            sysmelb_ParseTreeNode_t *message = sysmelb_newParseTreeNode(ParseTreeMessageSend, parserState_sourcePositionFrom(state, startPosition));
            message->messageSend.receiver = receiver;
            message->messageSend.selector = selector;

            receiver = message;
        }
        break;
        case SysmelTokenLeftParent:
        {
            parserState_advance(state);
            sysmelb_ParseTreeNodeDynArray_t arguments = parser_parseExpressionListUntilEndOrDelimiter(state, SysmelTokenRightParent);
            if (parserState_peekKind(state, 0) == SysmelTokenRightParent)
                parserState_advance(state);
            else
                sysmelb_ParseTreeNodeDynArray_add(&arguments, parserState_makeErrorAtCurrentSourcePosition(state, "Expected a right parenthesis"));

            sysmelb_ParseTreeNode_t *application = sysmelb_newParseTreeNode(ParseTreeFunctionApplication, parserState_sourcePositionFrom(state, startPosition));
            application->functionApplication.functional = receiver;
            application->functionApplication.arguments = arguments;

            receiver = application;
        }
        break;
        case SysmelTokenLeftBracket:
        case SysmelTokenByteArrayStart:
        {
            parserState_advance(state);
            sysmelb_ParseTreeNodeDynArray_t arguments = parser_parseExpressionListUntilEndOrDelimiter(state, SysmelTokenRightBracket);
            if (parserState_peekKind(state, 0) == SysmelTokenRightParent)
                parserState_advance(state);
            else
                sysmelb_ParseTreeNodeDynArray_add(&arguments, parserState_makeErrorAtCurrentSourcePosition(state, "Expected a right parenthesis"));

            sysmelb_ParseTreeNode_t *application = sysmelb_newParseTreeNode(ParseTreeFunctionApplication, parserState_sourcePositionFrom(state, startPosition));
            application->functionApplication.functional = receiver;
            application->functionApplication.arguments = arguments;
            receiver = application;
        }
        break;
        case SysmelTokenLeftCurlyBracket:
        {
            sysmelb_ParseTreeNode_t *argument = parser_parseBlock(state);

            sysmelb_ParseTreeNode_t *application = sysmelb_newParseTreeNode(ParseTreeFunctionApplication, parserState_sourcePositionFrom(state, startPosition));
            application->functionApplication.functional = receiver;
            sysmelb_ParseTreeNodeDynArray_add(&application->functionApplication.arguments, argument);

            receiver = application;
        }
        break;
        case SysmelTokenImmutableDictionaryStart:
        {
            sysmelb_ParseTreeNode_t *argument = parser_parseImmutableDictionary(state);

            sysmelb_ParseTreeNode_t *application = sysmelb_newParseTreeNode(ParseTreeFunctionApplication, parserState_sourcePositionFrom(state, startPosition));
            application->functionApplication.functional = receiver;
            sysmelb_ParseTreeNodeDynArray_add(&application->functionApplication.arguments, argument);

            receiver = application;
        }
        break;
        default:
            return receiver;
        }
    }
    return receiver;
}

sysmelb_ParseTreeNode_t *parser_parseQuote(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenQuote);
    parserState_advance(state);
    sysmelb_ParseTreeNode_t *term = parser_parseUnaryPrefixExpression(state);

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeQuote, parserState_sourcePositionFrom(state, startPosition));
    node->quote.expression = term;
    return node;
}

sysmelb_ParseTreeNode_t *parser_parseQuasiQuote(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenQuasiQuote);
    parserState_advance(state);
    sysmelb_ParseTreeNode_t *term = parser_parseUnaryPrefixExpression(state);

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeQuasiQuote, parserState_sourcePositionFrom(state, startPosition));
    node->quasiQuote.expression = term;
    return node;
}

sysmelb_ParseTreeNode_t *parser_parseQuasiUnquote(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenQuasiUnquote);
    parserState_advance(state);
    sysmelb_ParseTreeNode_t *term = parser_parseUnaryPrefixExpression(state);

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeQuasiUnquote, parserState_sourcePositionFrom(state, startPosition));
    node->quasiUnquote.expression = term;
    return node;
}

sysmelb_ParseTreeNode_t *parser_parseSplice(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenSplice);
    parserState_advance(state);
    sysmelb_ParseTreeNode_t *term = parser_parseUnaryPrefixExpression(state);

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeSplice, parserState_sourcePositionFrom(state, startPosition));
    node->splice.expression = term;
    return node;
}

sysmelb_ParseTreeNode_t *parser_parseUnaryPrefixExpression(sysmelb_parserState_t *state)
{
    switch (parserState_peekKind(state, 0))
    {
    case SysmelTokenQuote:
        return parser_parseQuote(state);
    case SysmelTokenQuasiQuote:
        return parser_parseQuasiQuote(state);
    case SysmelTokenQuasiUnquote:
        return parser_parseQuasiUnquote(state);
    case SysmelTokenSplice:
        return parser_parseSplice(state);
    default:
        return parser_parseUnaryPostfixExpression(state);
    }
}

sysmelb_ParseTreeNode_t *parser_parseBinaryExpressionSequence(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    sysmelb_ParseTreeNode_t *operand = parser_parseUnaryPrefixExpression(state);
    if (!parser_isBinaryExpressionOperator(parserState_peekKind(state, 0)))
        return operand;

    sysmelb_ParseTreeNodeDynArray_t elements = {0};
    sysmelb_ParseTreeNodeDynArray_add(&elements, operand);

    while (parser_isBinaryExpressionOperator(parserState_peekKind(state, 0)))
    {
        sysmelb_ScannerToken_t *operatorToken = parserState_next(state);
        sysmelb_symbol_t *operatorSymbol = sysmelb_internSymbol(operatorToken->textSize, operatorToken->textPosition);
        sysmelb_ParseTreeNode_t *operator= sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, operatorToken->sourcePosition);
        operator->literalSymbol.internedSymbol = operatorSymbol;
        sysmelb_ParseTreeNodeDynArray_add(&elements, operator);

        operand = parser_parseUnaryPrefixExpression(state);
        sysmelb_ParseTreeNodeDynArray_add(&elements, operand);
    }

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeBinaryOperatorSequence, parserState_sourcePositionFrom(state, startPosition));
    node->binaryOperatorSequence.elements = elements;
    return node;
}

sysmelb_ParseTreeNode_t *parser_parseAssociationExpression(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    sysmelb_ParseTreeNode_t *key = parser_parseBinaryExpressionSequence(state);

    if (parserState_peekKind(state, 0) != SysmelTokenColon)
        return key;

    parserState_advance(state);
    sysmelb_ParseTreeNode_t *value = parser_parseAssociationExpression(state);

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeAssociation, parserState_sourcePositionFrom(state, startPosition));
    node->association.key = key;
    node->association.value = value;
    return node;
}

sysmelb_ParseTreeNode_t *parser_parseKeywordApplication(sysmelb_parserState_t *state)
{
    assert(parserState_peekKind(state, 0) == SysmelTokenKeyword);
    size_t startPosition = state->position;

    sysmelb_dynstring_t selectorStream = {0};
    sysmelb_ParseTreeNodeDynArray_t arguments = {0};

    while (parserState_peekKind(state, 0) == SysmelTokenKeyword)
    {
        sysmelb_ScannerToken_t *keywordToken = parserState_next(state);
        sysmelb_dynstring_append(&selectorStream, keywordToken->textSize, keywordToken->textPosition);

        sysmelb_ParseTreeNode_t *argument = parser_parseBinaryExpressionSequence(state);
        sysmelb_ParseTreeNodeDynArray_add(&arguments, argument);
    }

    sysmelb_symbol_t *identifierSymbol = sysmelb_internSymbol(selectorStream.size, selectorStream.data);

    sysmelb_ParseTreeNode_t *identifierReference = sysmelb_newParseTreeNode(ParseTreeIdentifierReference, parserState_sourcePositionFrom(state, startPosition));
    identifierReference->identifierReference.identifier = identifierSymbol;

    sysmelb_ParseTreeNode_t *application = sysmelb_newParseTreeNode(ParseTreeFunctionApplication, parserState_sourcePositionFrom(state, startPosition));
    application->functionApplication.functional = identifierReference;
    application->functionApplication.arguments = arguments;
    return application;
}

sysmelb_ParseTreeNode_t *parser_parseKeywordMessageSend(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    sysmelb_ParseTreeNode_t *receiver = parser_parseAssociationExpression(state);
    if (parserState_peekKind(state, 0) != SysmelTokenKeyword)
        return receiver;

    sysmelb_dynstring_t selectorStream = {0};
    sysmelb_ParseTreeNodeDynArray_t arguments = {0};

    while (parserState_peekKind(state, 0) == SysmelTokenKeyword)
    {
        sysmelb_ScannerToken_t *keywordToken = parserState_next(state);
        sysmelb_dynstring_append(&selectorStream, keywordToken->textSize, keywordToken->textPosition);

        sysmelb_ParseTreeNode_t *argument = parser_parseBinaryExpressionSequence(state);
        sysmelb_ParseTreeNodeDynArray_add(&arguments, argument);
    }

    sysmelb_symbol_t *selectorSymbol = sysmelb_internSymbol(selectorStream.size, selectorStream.data);

    sysmelb_ParseTreeNode_t *selector = sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, parserState_sourcePositionFrom(state, startPosition));
    selector->literalSymbol.internedSymbol = selectorSymbol;

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeMessageSend, parserState_sourcePositionFrom(state, startPosition));
    node->messageSend.receiver = receiver;
    node->messageSend.selector = selector;
    node->messageSend.arguments = arguments;
    return node;
}

sysmelb_ParseTreeNode_t *parser_parseCascadedMessage(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    sysmelb_ScannerToken_t *token = parserState_peek(state, 0);
    if (parserState_peekKind(state, 0) == SysmelTokenIdentifier)
    {
        parserState_advance(state);

        sysmelb_ParseTreeNode_t *selector = sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, token->sourcePosition);
        selector->literalSymbol.internedSymbol = sysmelb_internSymbol(token->textSize, token->textPosition);

        sysmelb_ParseTreeNode_t *cascadedMessage = sysmelb_newParseTreeNode(ParseTreeCascadedMessage, parserState_sourcePositionFrom(state, startPosition));
        cascadedMessage->cascadedMessage.selector = selector;
        return cascadedMessage;
    }
    else if (parserState_peekKind(state, 0) == SysmelTokenKeyword)
    {
        sysmelb_dynstring_t selectorStream = {0};
        sysmelb_ParseTreeNodeDynArray_t arguments = {0};

        while (parserState_peekKind(state, 0) == SysmelTokenKeyword)
        {
            sysmelb_ScannerToken_t *keywordToken = parserState_next(state);
            sysmelb_dynstring_append(&selectorStream, keywordToken->textSize, keywordToken->textPosition);

            sysmelb_ParseTreeNode_t *argument = parser_parseBinaryExpressionSequence(state);
            sysmelb_ParseTreeNodeDynArray_add(&arguments, argument);
        }

        sysmelb_symbol_t *selectorSymbol = sysmelb_internSymbol(selectorStream.size, selectorStream.data);

        sysmelb_ParseTreeNode_t *selector = sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, parserState_sourcePositionFrom(state, startPosition));
        selector->literalSymbol.internedSymbol = selectorSymbol;

        sysmelb_ParseTreeNode_t *cascadedMessage = sysmelb_newParseTreeNode(ParseTreeCascadedMessage, parserState_sourcePositionFrom(state, startPosition));
        cascadedMessage->cascadedMessage.selector = selector;
        cascadedMessage->cascadedMessage.arguments = arguments;
        return cascadedMessage;
    }
    else if (parser_isBinaryExpressionOperator(parserState_peekKind(state, 0)))
    {
        parserState_advance(state);
        sysmelb_symbol_t *selectorSymbol = sysmelb_internSymbol(token->textSize, token->textPosition);

        sysmelb_ParseTreeNode_t *selector = sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, token->sourcePosition);
        selector->literalSymbol.internedSymbol = selectorSymbol;

        sysmelb_ParseTreeNode_t *argument = parser_parseUnaryPostfixExpression(state);

        sysmelb_ParseTreeNode_t *cascadedMessage = sysmelb_newParseTreeNode(ParseTreeCascadedMessage, parserState_sourcePositionFrom(state, startPosition));
        cascadedMessage->cascadedMessage.selector = selector;
        sysmelb_ParseTreeNodeDynArray_add(&cascadedMessage->cascadedMessage.arguments, argument);
        return cascadedMessage;
    }
    else
    {
        return parserState_makeErrorAtCurrentSourcePosition(state, "Expected a cascaded message.");
    }
}

sysmelb_ParseTreeNode_t *parser_parseMessageCascade(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    sysmelb_ParseTreeNode_t *firstMessageOrReceiver = parser_parseKeywordMessageSend(state);
    if (parserState_peekKind(state, 0) != SysmelTokenSemicolon)
        return firstMessageOrReceiver;

    sysmelb_ParseTreeNode_t *receiver = firstMessageOrReceiver;
    sysmelb_ParseTreeNodeDynArray_t cascadedMessages = {0};
    if (firstMessageOrReceiver->kind == ParseTreeMessageSend)
    {
        receiver = firstMessageOrReceiver->messageSend.receiver;

        sysmelb_ParseTreeNode_t *firstCascadedMessage = sysmelb_newParseTreeNode(ParseTreeCascadedMessage, firstMessageOrReceiver->sourcePosition);
        firstCascadedMessage->cascadedMessage.selector = firstMessageOrReceiver->messageSend.selector;
        firstCascadedMessage->cascadedMessage.arguments = firstMessageOrReceiver->messageSend.arguments;
        sysmelb_ParseTreeNodeDynArray_add(&cascadedMessages, firstCascadedMessage);
    }

    while (parserState_peekKind(state, 0) == SysmelTokenSemicolon)
    {
        parserState_advance(state);
        sysmelb_ParseTreeNode_t *cascadedMessage = parser_parseCascadedMessage(state);
        sysmelb_ParseTreeNodeDynArray_add(&cascadedMessages, cascadedMessage);
    }

    sysmelb_ParseTreeNode_t *messageCascade = sysmelb_newParseTreeNode(ParseTreeMessageCascade, parserState_sourcePositionFrom(state, startPosition));
    messageCascade->messageCascade.receiver = receiver;
    messageCascade->messageCascade.cascadedMessages = cascadedMessages;
    return messageCascade;
}

sysmelb_ParseTreeNode_t *parser_parseLowPrecedenceExpression(sysmelb_parserState_t *state)
{
    if (parserState_peekKind(state, 0) == SysmelTokenKeyword)
        return parser_parseKeywordApplication(state);
    return parser_parseMessageCascade(state);
}

sysmelb_ParseTreeNode_t *parser_parseAssignmentExpression(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    sysmelb_ParseTreeNode_t *assignedStore = parser_parseLowPrecedenceExpression(state);
    if (parserState_peekKind(state, 0) == SysmelTokenAssignment)
    {
        parserState_advance(state);
        sysmelb_ParseTreeNode_t *assignedValue = parser_parseAssignmentExpression(state);

        sysmelb_ParseTreeNode_t *assignment = sysmelb_newParseTreeNode(ParseTreeAssignment, parserState_sourcePositionFrom(state, startPosition));
        assignment->assignment.store = assignedStore;
        assignment->assignment.value = assignedValue;
        return assignment;
    }
    else
    {
        return assignedStore;
    }
}

sysmelb_ParseTreeNode_t *parser_parseCommaExpression(sysmelb_parserState_t *state)
{
    size_t startingPosition = state->position;
    sysmelb_ParseTreeNode_t *element = parser_parseAssignmentExpression(state);

    if (parserState_peekKind(state, 0) != SysmelTokenComma)
        return element;

    sysmelb_ParseTreeNodeDynArray_t elements = {0};
    sysmelb_ParseTreeNodeDynArray_add(&elements, element);

    while (parserState_peekKind(state, 0) == SysmelTokenComma)
    {
        parserState_advance(state);
        element = parser_parseAssignmentExpression(state);
        sysmelb_ParseTreeNodeDynArray_add(&elements, element);
    }

    sysmelb_ParseTreeNode_t *tuple = sysmelb_newParseTreeNode(ParseTreeTuple, parserState_sourcePositionFrom(state, startingPosition));
    tuple->tuple.elements = elements;
    return tuple;
}

sysmelb_ParseTreeNode_t *parser_parseFunctionalType(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    sysmelb_ParseTreeNodeDynArray_t arguments = {0};
    sysmelb_ParseTreeNode_t *resultTypeExpression = NULL;
    while(parserState_peekKind(state, 0) == SysmelTokenDollar)
    {
        sysmelb_ParseTreeNode_t *argument = parser_parseBindableName(state);
        sysmelb_ParseTreeNodeDynArray_add(&arguments, argument);
    }

    if (parserState_peekKind(state, 0) == SysmelTokenColonColon)
    {
        parserState_advance(state);
        resultTypeExpression = parser_parseUnaryPrefixExpression(state);
    }

    sysmelb_ParseTreeNode_t *functionalType = sysmelb_newParseTreeNode(ParseTreeFunctionalDependentType, parserState_sourcePositionFrom(state, startPosition));
    functionalType->functionalDependentType.argumentDefinition = arguments;
    functionalType->functionalDependentType.resultTypeExpression = resultTypeExpression;
    return functionalType;
}

sysmelb_ParseTreeNode_t *parser_parseBlock(sysmelb_parserState_t *state)
{
    // {
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenLeftCurlyBracket);
    parserState_advance(state);

    sysmelb_ParseTreeNode_t *functionalType = NULL;
    if (parserState_peekKind(state, 0) == SysmelTokenBar)
    {
        parserState_advance(state);
        if (parserState_peekKind(state, 0) == SysmelTokenBar)
        {
            functionalType = sysmelb_newParseTreeNode(ParseTreeFunctionalDependentType, parserState_currentSourcePosition(state));
        }
        else
        {
            functionalType = parser_parseFunctionalType(state);
            parserState_expectAddingErrorToNode(state, SysmelTokenBar, functionalType);
        }
    }

    // Body
    sysmelb_ParseTreeNode_t *body = parser_parseSequenceUntilEndOrDelimiter(state, SysmelTokenRightCurlyBracket);
    parserState_expectAddingErrorToNode(state, SysmelTokenRightCurlyBracket, body);

    if (functionalType)
    {
        sysmelb_ParseTreeNode_t *blockClosure = sysmelb_newParseTreeNode(ParseTreeBlockClosure, parserState_sourcePositionFrom(state, startPosition));
        blockClosure->blockClosure.functionType = functionalType;
        blockClosure->blockClosure.body = body;
        return blockClosure;
    }
    else
    {
        sysmelb_ParseTreeNode_t *lexicalBlock = sysmelb_newParseTreeNode(ParseTreeLexicalBlock, parserState_sourcePositionFrom(state, startPosition));
        lexicalBlock->lexicalBlock.expression = body;
        return lexicalBlock;
    }
}

sysmelb_ParseTreeNode_t *parser_parseImmutableDictionaryAssociation(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    sysmelb_ParseTreeNode_t *key = NULL;
    sysmelb_ParseTreeNode_t *value = NULL;
    if (parserState_peekKind(state, 0) == SysmelTokenKeyword)
    {
        sysmelb_ScannerToken_t *keyToken = parserState_next(state);
        sysmelb_symbol_t *keySymbol = sysmelb_internSymbol(keyToken->textSize - 1, keyToken->textPosition);
        key = sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, keyToken->sourcePosition);
        key->literalSymbol.internedSymbol = keySymbol;

        if (parserState_peekKind(state, 0) != SysmelTokenDot && parserState_peekKind(state, 0) != SysmelTokenRightCurlyBracket)
            value = parser_parseAssociationExpression(state);
    }
    else
    {
        key = parser_parseBinaryExpressionSequence(state);
        if (parserState_peekKind(state, 0) == SysmelTokenColon)
        {
            parserState_advance(state);
            value = parser_parseAssociationExpression(state);
        }
    }

    sysmelb_ParseTreeNode_t *dictAssociation = sysmelb_newParseTreeNode(ParseTreeAssociation, parserState_sourcePositionFrom(state, startPosition));
    dictAssociation->association.key = key;
    dictAssociation->association.value = value;
    return dictAssociation;
}

sysmelb_ParseTreeNode_t *parser_parseImmutableDictionary(sysmelb_parserState_t *state)
{
    // {
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenImmutableDictionaryStart);
    parserState_advance(state);

    // Chop the initial dots
    while (parserState_peekKind(state, 0) == SysmelTokenDot)
        parserState_advance(state);

    // Parse the next expression
    bool expectsExpression = true;
    sysmelb_ParseTreeNodeDynArray_t elements = {0};

    while (!parserState_atEnd(state) && parserState_peekKind(state, 0) != SysmelTokenRightCurlyBracket)
    {
        if (!expectsExpression)
            sysmelb_ParseTreeNodeDynArray_add(&elements, parserState_makeErrorAtCurrentSourcePosition(state, "Expected dot before association."));

        sysmelb_ParseTreeNode_t *expression = parser_parseImmutableDictionaryAssociation(state);
        sysmelb_ParseTreeNodeDynArray_add(&elements, expression);

        expectsExpression = false;
        // Chop the next dot sequence
        while (parserState_peekKind(state, 0) == SysmelTokenDot)
        {
            expectsExpression = true;
            parserState_advance(state);
        }
    }

    // }
    if (parserState_peekKind(state, 0) == SysmelTokenRightCurlyBracket)
        parserState_advance(state);
    else
        sysmelb_ParseTreeNodeDynArray_add(&elements, parserState_makeErrorAtCurrentSourcePosition(state, "Expected a right curly bracket."));

    sysmelb_ParseTreeNode_t *dictionary = sysmelb_newParseTreeNode(ParseTreeImmutableDictionary, parserState_sourcePositionFrom(state, startPosition));
    dictionary->dictionary.elements = elements;
    return dictionary;
}

void parser_parseOptionalBindableNameType(sysmelb_parserState_t *state, bool *outIsImplicit, sysmelb_ParseTreeNode_t **outTypeExpression)
{
    if (parserState_peekKind(state, 0) == SysmelTokenLeftBracket)
    {
        parserState_advance(state);
        *outTypeExpression = parser_parseExpression(state);
        *outTypeExpression = parserState_expectAddingErrorToNode(state, SysmelTokenRightBracket, *outTypeExpression);
        *outIsImplicit = true;
    }
    else if (parserState_peekKind(state, 0) == SysmelTokenLeftParent)
    {
        parserState_advance(state);
        *outTypeExpression = parser_parseExpression(state);
        *outTypeExpression = parserState_expectAddingErrorToNode(state, SysmelTokenRightParent, *outTypeExpression);
        *outIsImplicit = false;
    }
    else
    {
        *outIsImplicit = false;
        *outTypeExpression = NULL;
    }
}

void parser_parseOptionalBindableDependentType(sysmelb_parserState_t *state, bool *outIsImplicit, sysmelb_ParseTreeNode_t **outTypeExpression)
{
    if (parserState_peekKind(state, 0) == SysmelTokenLeftBracket)
    {
        parserState_advance(state);
        *outTypeExpression = parser_parseFunctionalType(state);
        *outTypeExpression = parserState_expectAddingErrorToNode(state, SysmelTokenRightBracket, *outTypeExpression);
        *outIsImplicit = true;
    }
    else if (parserState_peekKind(state, 0) == SysmelTokenLeftParent)
    {
        parserState_advance(state);
        *outTypeExpression = parser_parseFunctionalType(state);
        *outTypeExpression = parserState_expectAddingErrorToNode(state, SysmelTokenRightParent, *outTypeExpression);
        *outIsImplicit = false;
    }
    else
    {
        *outIsImplicit = false;
        *outTypeExpression = NULL;
    }
}

sysmelb_ParseTreeNode_t *parser_parseNameExpression(sysmelb_parserState_t *state)
{
    if (parserState_peekKind(state, 0) == SysmelTokenIdentifier)
    {
        sysmelb_ScannerToken_t *token = parserState_next(state);
        sysmelb_symbol_t *tokenSymbol = sysmelb_internSymbol(token->textSize, token->textPosition);

        sysmelb_ParseTreeNode_t *nameNode = sysmelb_newParseTreeNode(ParseTreeLiteralSymbolNode, token->sourcePosition);
        nameNode->literalSymbol.internedSymbol = tokenSymbol;
        return nameNode;
    }
    else
    {
        return parserState_makeErrorAtCurrentSourcePosition(state, "Expected a bindable name.");
    }
}

sysmelb_ParseTreeNode_t *parser_parseBindableName(sysmelb_parserState_t *state)
{
    size_t startPosition = state->position;
    assert(parserState_peekKind(state, 0) == SysmelTokenDollar);
    parserState_advance(state);

    bool isExistential = false;
    bool isMutable = false;

    if (parserState_peekKind(state, 0) == SysmelTokenBang)
    {
        isMutable = true;
        parserState_advance(state);
    }
    if (parserState_peekKind(state, 0) == SysmelTokenQuestion)
    {
        isExistential = isExistential || (parserState_peekKind(state, 0) == SysmelTokenQuestion);
        parserState_advance(state);
    }

    bool isAnonymousFunction = false;
    if(parserState_peekKind(state, 0) == SysmelTokenDollar)
    {
        parserState_advance(state);
        isAnonymousFunction = true;
    }


    bool isImplicit = false;
    sysmelb_ParseTreeNode_t *typeExpression = NULL;
    if(!isAnonymousFunction)
        parser_parseOptionalBindableNameType(state, &isImplicit, &typeExpression);
    bool hasPostTypeExpression = false;

    bool isVariadic = false;
    sysmelb_ParseTreeNode_t *nameExpression = NULL;
    if (parserState_peekKind(state, 0) == SysmelTokenEllipsis)
    {
        parserState_advance(state);
        isVariadic = true;
        nameExpression = NULL;
    }
    else
    {
        if(!isAnonymousFunction)
            nameExpression = parser_parseNameExpression(state);
    }

    if (!typeExpression || isAnonymousFunction)
    {
        parser_parseOptionalBindableDependentType(state, &isImplicit, &typeExpression);
        hasPostTypeExpression = typeExpression != NULL;
    }

    if (parserState_peekKind(state, 0) == SysmelTokenEllipsis)
    {
        parserState_advance(state);
        isVariadic = true;
    }

    sysmelb_ParseTreeNode_t *bindableName = sysmelb_newParseTreeNode(ParseTreeBindableName, parserState_sourcePositionFrom(state, startPosition));
    bindableName->bindableName.typeExpression = typeExpression;
    bindableName->bindableName.nameExpression = nameExpression;
    bindableName->bindableName.isImplicit = isImplicit;
    bindableName->bindableName.isExistential = isExistential;
    bindableName->bindableName.isVariadic = isVariadic;
    bindableName->bindableName.isMutable = isMutable;
    bindableName->bindableName.hasPostTypeExpression = hasPostTypeExpression;
    return bindableName;
}

sysmelb_ParseTreeNode_t *parser_parseTerm(sysmelb_parserState_t *state)
{
    switch (parserState_peekKind(state, 0))
    {
    case SysmelTokenIdentifier:
        return parser_parseIdentifier(state);
    case SysmelTokenLeftParent:
        return parser_parseParenthesis(state);
    case SysmelTokenLeftCurlyBracket:
        return parser_parseBlock(state);
    case SysmelTokenLeftBracket:
        return parser_parseArray(state);
    case SysmelTokenByteArrayStart:
        return parser_parseByteArray(state);
    case SysmelTokenImmutableDictionaryStart:
        return parser_parseImmutableDictionary(state);
    case SysmelTokenDollar:
        return parser_parseBindableName(state);
    default:
        return parser_parseLiteral(state);
    }
}

sysmelb_ParseTreeNode_t *parser_parseExpression(sysmelb_parserState_t *state)
{
    return parser_parseCommaExpression(state);
}

sysmelb_ParseTreeNodeDynArray_t parser_parseExpressionListUntilEndOrDelimiter(sysmelb_parserState_t *state, sysmelb_TokenKind_t delimiter)
{
    sysmelb_ParseTreeNodeDynArray_t elements = {0};

    // Leading dots.
    while (parserState_peekKind(state, 0) == SysmelTokenDot)
        parserState_advance(state);

    bool expectsExpression = true;

    while (!parserState_atEnd(state) && parserState_peekKind(state, 0) != delimiter)
    {
        if (!expectsExpression)
            sysmelb_ParseTreeNodeDynArray_add(&elements, parserState_makeErrorAtCurrentSourcePosition(state, "Expected dot before expression."));

        sysmelb_ParseTreeNode_t *expression = parser_parseExpression(state);
        sysmelb_ParseTreeNodeDynArray_add(&elements, expression);

        // Trailing dots.
        while (parserState_peekKind(state, 0) == SysmelTokenDot)
        {
            expectsExpression = true;
            parserState_advance(state);
        }
    }

    return elements;
}

sysmelb_ParseTreeNode_t *parser_parseSequenceUntilEndOrDelimiter(sysmelb_parserState_t *state, sysmelb_TokenKind_t delimiter)
{
    size_t startingPosition = state->position;
    sysmelb_ParseTreeNodeDynArray_t expressions = parser_parseExpressionListUntilEndOrDelimiter(state, delimiter);
    if (expressions.size == 1)
        return expressions.elements[0];

    sysmelb_ParseTreeNode_t *sequenceNode = sysmelb_newParseTreeNode(ParseTreeSequence, parserState_sourcePositionFrom(state, startingPosition));
    sequenceNode->sequence.elements = expressions;
    return sequenceNode;
}

sysmelb_ParseTreeNode_t *parser_parseTopLevelExpressions(sysmelb_parserState_t *state)
{
    return parser_parseSequenceUntilEndOrDelimiter(state, SysmelTokenEndOfSource);
}

sysmelb_ParseTreeNode_t *parseTokenList(sysmelb_SourceCode_t *sourceCode, size_t tokenCount, sysmelb_ScannerToken_t *tokens)
{
    sysmelb_parserState_t state = {
        .sourceCode = sourceCode,
        .tokenCount = tokenCount,
        .tokens = tokens,
        .position = 0,
    };

    return parser_parseTopLevelExpressions(&state);
}
