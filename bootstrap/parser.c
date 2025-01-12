#include "parser.h"
#include "parse-tree.h"
#include "memory.h"
#include <stdbool.h>
#include <assert.h>
#include <string.h>

typedef struct sysmelb_parserState_s sysmelb_parserState_t;
sysmelb_SourcePosition_t parserState_currentSourcePosition(sysmelb_parserState_t *state);

void sysmelb_ParseTreeNodeDynArray_incrementCapacity(sysmelb_ParseTreeNodeDynArray_t *dynArray)
{
    size_t newCapacity = dynArray->capacity*2;
    if(newCapacity < 16)
        newCapacity = 16;

    sysmelb_ParseTreeNode_t **newStorage = sysmelb_allocate(newCapacity * sizeof(sysmelb_ParseTreeNode_t*));
    memcpy(newStorage, dynArray->elements, dynArray->size * sizeof(sysmelb_ParseTreeNode_t*));
    dynArray->capacity = newCapacity;
    dynArray->elements = newStorage;
}

void sysmelb_ParseTreeNodeDynArray_add(sysmelb_ParseTreeNodeDynArray_t *dynArray, sysmelb_ParseTreeNode_t *element)
{
    if(dynArray->size >= dynArray->capacity)
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

sysmelb_ParseTreeNode_t* parseLiteralInteger(sysmelb_parserState_t *state)
{
    sysmelb_ScannerToken_t *token = parserState_next(state);
    assert(token->kind == SysmelTokenNat);

    sysmelb_IntegerLiteralType_t parsedConstant = parseIntegerConstant(token->textSize, token->textPosition);
    sysmelb_ParseTreeNode_t *literal = sysmelb_newParseTreeNode(ParseTreeLiteralIntegerNode, token->sourcePosition);
    literal->literalInteger.value = parsedConstant;
    return literal;
}

/*
sysmelb_ParseTreeNode_t* parseLiteralFloat(sysmelb_parserState_t *state)
{
    auto token = state.next();
    assert(token->kind == TokenKind::Float);
    auto literal = std::make_shared<SyntaxLiteralFloat>();
    literal->sourcePosition = token->position;
    literal->value = atof(token->getValue().c_str());
    return literal;
}

std::string parseCEscapedString(const std::string &str)
{
    std::string unescaped;
    unescaped.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i)
    {
        auto c = str[i];
        if (c == '\\')
        {
            auto c1 = str[++i];
            switch (c1)
            {
            case 'n':
                unescaped.push_back('\n');
                break;
            case 'r':
                unescaped.push_back('\r');
                break;
            case 't':
                unescaped.push_back('\t');
                break;
            default:
                unescaped.push_back(c1);
                break;
            }
        }
        else
        {
            unescaped.push_back(c);
        }
    }

    return unescaped;
}

sysmelb_ParseTreeNode_t *parseLiteralCharacter(sysmelb_parserState_t *state)
{
    auto token = state.next();
    assert(token->kind == TokenKind::Character);
    auto literal = std::make_shared<SyntaxLiteralCharacter>();
    literal->sourcePosition = token->position;
    auto tokenValue = token->getValue();
    literal->value = parseCEscapedString(tokenValue.substr(1, tokenValue.size() - 2))[0];
    return literal;
}

sysmelb_ParseTreeNode_t *parseLiteralString(sysmelb_parserState_t *state)
{
    auto token = state.next();
    assert(token->kind == TokenKind::String);
    auto literal = std::make_shared<SyntaxLiteralString>();
    literal->sourcePosition = token->position;
    auto tokenValue = token->getValue();
    literal->value = parseCEscapedString(tokenValue.substr(1, tokenValue.size() - 2));
    return literal;
}

sysmelb_ParseTreeNode_t *parseLiteralSymbol(sysmelb_parserState_t *state)
{
    auto token = state.next();
    assert(token->kind == TokenKind::Symbol);
    auto literal = std::make_shared<SyntaxLiteralSymbol>();
    literal->sourcePosition = token->position;
    auto tokenValue = token->getValue().substr(1);
    if (tokenValue[0] == '\"')
        literal->value = parseCEscapedString(tokenValue.substr(1, tokenValue.size() - 2));
    else
        literal->value = tokenValue;

    return literal;
}
*/

sysmelb_ParseTreeNode_t *parser_parseLiteral(sysmelb_parserState_t *state)
{
    switch (parserState_peekKind(state, 0))
    {
    case SysmelTokenNat:
        return parseLiteralInteger(state);
    /*case SysmelTokenFloat:
        return parseLiteralFloat(state);
    case SysmelTokenCharacter:
        return parseLiteralCharacter(state);
    case SysmelTokenString:
        return parseLiteralString(state);
    case SysmelTokenSymbol:
        return parseLiteralSymbol(state);*/
    default:
        return parserState_advanceWithExpectedError(state, "Expected a literal");
    }
}
/*
    ValuePtr parseIdentifier(sysmelb_parserState_t *state)
    {
        auto token = state.next();
        assert(token->kind == TokenKind::Identifier);

        auto node = std::make_shared<SyntaxIdentifierReference>();
        node->sourcePosition = token->position;
        node->value = token->getValue();
        return node;
    }

    bool isBinaryExpressionOperator(TokenKind kind)
    {
        switch (kind)
        {
        case TokenKind::Operator:
        case TokenKind::Star:
        case TokenKind::LessThan:
        case TokenKind::GreaterThan:
        case TokenKind::Bar:
            return true;
        default:
            return false;
        }
    }

    ValuePtr parseParenthesis(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::LeftParent);
        state.advance();

        if (isBinaryExpressionOperator(parserState_peekKind(state, 0)) && state.peekKind(1) == TokenKind::RightParent)
        {
            auto token = state.next();
            state.advance();
        }

        if (parserState_peekKind(state, 0) == TokenKind::RightParent)
        {
            state.advance();
            auto tuple = std::make_shared<SyntaxTuple>();
            tuple->sourcePosition = state.sourcePositionFrom(startPosition);
            return tuple;
        }

        auto expression = parseSequenceUntilEndOrDelimiter(state, TokenKind::RightParent);
        expression = state.expectAddingErrorToNode(TokenKind::RightParent, expression);
        return expression;
    }

    ValuePtr parseArray(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::LeftBracket);
        state.advance();

        auto expressions = parseExpressionListUntilEndOrDelimiter(state, TokenKind::RightBracket);

        if(parserState_peekKind(state, 0) == TokenKind::RightBracket)
        {
            state.advance();
        }
        else
        {
            // TODO: add an error
            abort();
        }
        
        // Array
        auto array = std::make_shared<SyntaxArray> ();
        array->sourcePosition = state.sourcePositionFrom(startPosition);
        array->expressions.swap(expressions);
        return array;
    }

    ValuePtr parseByteArray(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::ByteArrayStart);
        state.advance();

        auto expressions = parseExpressionListUntilEndOrDelimiter(state, TokenKind::RightBracket);

        if(parserState_peekKind(state, 0) == TokenKind::RightBracket)
        {
            state.advance();
        }
        else
        {
            // TODO: an error
            abort();
        }
        
        // Byte
        auto byteArray = std::make_shared<SyntaxByteArray> ();
        byteArray->sourcePosition = state.sourcePositionFrom(startPosition);
        byteArray->byteExpressions.swap(expressions);
        return byteArray;
    }

    bool isUnaryPostfixTokenKind(TokenKind kind)
    {
        switch(kind)
        {
        case TokenKind::Identifier:
        case TokenKind::LeftParent:
        case TokenKind::LeftBracket:
        case TokenKind::LeftCurlyBracket:
        case TokenKind::ByteArrayStart:
        case TokenKind::DictionaryStart:
            return true;
        default:
            return false;
        }
    }

    ValuePtr parseUnaryPostfixExpression(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        auto receiver = parseTerm(state);

        while (isUnaryPostfixTokenKind(parserState_peekKind(state, 0)))
        {
            auto token = state.peek();
            switch(token->kind)
            {
            case TokenKind::Identifier:
                {
                    state.advance();
                    auto selector = std::make_shared<SyntaxLiteralSymbol> ();
                    selector->sourcePosition = token->position;
                    selector->value = token->getValue();

                    auto message = std::make_shared<SyntaxMessageSend> ();
                    message->sourcePosition = state.sourcePositionFrom(startPosition);
                    message->receiver = receiver;
                    message->selector = selector;
                    
                    receiver = message;
                }
                break;
            case TokenKind::LeftParent:
                {
                    state.advance();
                    auto arguments = parseExpressionListUntilEndOrDelimiter(state, TokenKind::RightParent);
                    if(parserState_peekKind(state, 0) == TokenKind::RightParent)
                        state.advance();
                    else
                        arguments.push_back(state.makeErrorAtCurrentSourcePosition("Expected a right parenthesis"));
                    
                    auto application = std::make_shared<SyntaxApplication> ();
                    application->sourcePosition = state.sourcePositionFrom(startPosition);
                    application->functional = receiver;
                    application->arguments = arguments;
                    application->kind = token->kind;
                    receiver = application;
                }
                break;
            case TokenKind::LeftBracket:
            case TokenKind::ByteArrayStart:
                {
                    state.advance();
                    auto arguments = parseExpressionListUntilEndOrDelimiter(state, TokenKind::RightBracket);
                    if(parserState_peekKind(state, 0) == TokenKind::RightBracket)
                        state.advance();
                    else
                        arguments.push_back(state.makeErrorAtCurrentSourcePosition("Expected a right bracket"));
                    
                    auto application = std::make_shared<SyntaxApplication> ();
                    application->sourcePosition = state.sourcePositionFrom(startPosition);
                    application->functional = receiver;
                    application->arguments = arguments;
                    application->kind = token->kind;
                    receiver = application;
                }
                break;
            case TokenKind::LeftCurlyBracket:
                {
                    auto argument = parseBlock(state);
                    auto application = std::make_shared<SyntaxApplication> ();
                    application->sourcePosition = state.sourcePositionFrom(startPosition);
                    application->functional = receiver;
                    application->arguments.push_back(argument);
                    application->kind = token->kind;
                    receiver = application;
                }
                break;
            case TokenKind::DictionaryStart:
                {
                    auto argument = parseDictionary(state);
                    auto application = std::make_shared<SyntaxApplication> ();
                    application->sourcePosition = state.sourcePositionFrom(startPosition);
                    application->functional = receiver;
                    application->arguments.push_back(argument);
                    application->kind = token->kind;
                    receiver = application;
                }
                break;
            default:
                return receiver;
            }
        }
        return receiver;
    }

    ValuePtr parseQuote(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::Quote);
        state.advance();
        auto term = parseUnaryPrefixExpression(state);
        auto quoteNode = std::make_shared<SyntaxQuote>();
        quoteNode->sourcePosition = state.sourcePositionFrom(startPosition);
        quoteNode->value = term;
        return quoteNode;
    }

    ValuePtr parseQuasiQuote(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::QuasiQuote);
        state.advance();
        auto term = parseUnaryPrefixExpression(state);
        auto quoteNode = std::make_shared<SyntaxQuasiQuote>();
        quoteNode->sourcePosition = state.sourcePositionFrom(startPosition);
        quoteNode->value = term;
        return quoteNode;
    }

    ValuePtr parseQuasiUnquote(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::QuasiUnquote);
        state.advance();
        auto term = parseUnaryPrefixExpression(state);
        auto quoteNode = std::make_shared<SyntaxQuasiUnquote>();
        quoteNode->sourcePosition = state.sourcePositionFrom(startPosition);
        quoteNode->value = term;
        return quoteNode;
    }

    ValuePtr parseSplice(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::Splice);
        state.advance();
        auto term = parseUnaryPrefixExpression(state);
        auto spliceNode = std::make_shared<SyntaxSplice>();
        spliceNode->sourcePosition = state.sourcePositionFrom(startPosition);
        spliceNode->value = term;
        return spliceNode;
    }

    ValuePtr parseUnaryPrefixExpression(sysmelb_parserState_t *state)
    {
        switch (parserState_peekKind(state, 0))
        {
        case TokenKind::Quote:
            return parseQuote(state);
        case TokenKind::QuasiQuote:
            return parseQuasiQuote(state);
        case TokenKind::QuasiUnquote:
            return parseQuasiUnquote(state);
        case TokenKind::Splice:
            return parseSplice(state);
        default:
            return parseUnaryPostfixExpression(state);
        }
    }

    ValuePtr parseBinaryExpressionSequence(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        auto operand = parseUnaryPrefixExpression(state);
        if (!isBinaryExpressionOperator(parserState_peekKind(state, 0)))
            return operand;

        std::vector<ValuePtr> elements;
        elements.push_back(operand);
        while (isBinaryExpressionOperator(parserState_peekKind(state, 0)))
        {
            auto operatorToken = state.next();
            auto operatorNode = std::make_shared<SyntaxLiteralSymbol>();
            operatorNode->sourcePosition = operatorToken->position;
            operatorNode->value = operatorToken->getValue();
            elements.push_back(operatorNode);

            auto operand = parseUnaryPrefixExpression(state);
            elements.push_back(operand);
        }

        auto binaryExpression = std::make_shared<SyntaxBinaryExpressionSequence>();
        binaryExpression->sourcePosition = state.sourcePositionFrom(startPosition);
        binaryExpression->elements.swap(elements);
        return binaryExpression;
    }

    ValuePtr parseAssociationExpression(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        auto key = parseBinaryExpressionSequence(state);

        if (parserState_peekKind(state, 0) != TokenKind::Colon)
            return key;

        state.advance();
        auto value = parseAssociationExpression(state);
        auto assoc = std::make_shared<SyntaxAssociation>();
        assoc->sourcePosition = state.sourcePositionFrom(startPosition);
        assoc->key = key;
        assoc->value = value;
        return assoc;
    }

    ValuePtr parseKeywordApplication(sysmelb_parserState_t *state)
    {
        assert(parserState_peekKind(state, 0) == TokenKind::Keyword);
        auto startPosition = state.position;

        std::string symbolValue;
        std::vector<ValuePtr> arguments;

        while (parserState_peekKind(state, 0) == TokenKind::Keyword)
        {
            auto keywordToken = state.next();
            symbolValue.append(keywordToken->getValue());

            auto argument = parseAssociationExpression(state);
            arguments.push_back(argument);
        }

        auto identifier = std::make_shared<SyntaxLiteralSymbol>();
        identifier->sourcePosition = state.sourcePositionFrom(startPosition);
        identifier->value = symbolValue;

        auto messageSend = std::make_shared<SyntaxMessageSend>();
        messageSend->sourcePosition = state.sourcePositionFrom(startPosition);
        messageSend->selector = identifier;
        messageSend->arguments = arguments;
        return messageSend;
    }

    ValuePtr parseKeywordMessageSend(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        auto receiver = parseAssociationExpression(state);
        if(parserState_peekKind(state, 0) != TokenKind::Keyword)
            return receiver;
        
        std::string selectorValue;
        std::vector<ValuePtr> arguments;

        while(parserState_peekKind(state, 0) == TokenKind::Keyword)
        {
            auto keywordToken = state.next();
            selectorValue.append(keywordToken->getValue());

            auto argument = parseAssociationExpression(state);
            arguments.push_back(argument);
        }

        auto selectorSymbol = std::make_shared<SyntaxLiteralSymbol> ();
        selectorSymbol->sourcePosition = state.sourcePositionFrom(startPosition);
        selectorSymbol->value = selectorValue;

        auto messageSend = std::make_shared<SyntaxMessageSend> ();
        messageSend->sourcePosition = state.sourcePositionFrom(startPosition);
        messageSend->receiver = receiver;
        messageSend->selector = selectorSymbol;
        messageSend->arguments.swap(arguments);
        return messageSend;
    }

    ValuePtr parseCascadedMessage(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        auto token = state.peek();
        if(parserState_peekKind(state, 0) == TokenKind::Identifier)
        {
            state.advance();
            
            auto selector = std::make_shared<SyntaxLiteralSymbol> ();
            selector->sourcePosition = token->getSourcePosition();
            selector->value = token->getValue();

            auto cascadedMessage = std::make_shared<SyntaxMessageCascadeMessage> ();
            cascadedMessage->sourcePosition = state.sourcePositionFrom(startPosition);
            cascadedMessage->selector = selector;
            return cascadedMessage;
        }
        else if(parserState_peekKind(state, 0) == TokenKind::Keyword)
        {
            std::string selectorValue;
            std::vector<ValuePtr> arguments;

            while(parserState_peekKind(state, 0) == TokenKind::Keyword)
            {
                auto keywordToken = state.next();
                selectorValue.append(keywordToken->getValue());

                auto argument = parseBinaryExpressionSequence(state);
                arguments.push_back(argument);
            }

            auto selector = std::make_shared<SyntaxLiteralSymbol> ();
            selector->sourcePosition = state.sourcePositionFrom(startPosition);
            selector->value = selectorValue;

            auto cascadedMessage = std::make_shared<SyntaxMessageCascadeMessage> ();
            cascadedMessage->sourcePosition = state.sourcePositionFrom(startPosition);
            cascadedMessage->selector = selector;
            cascadedMessage->arguments.swap(arguments);
            return cascadedMessage;
        }
        else if(isBinaryExpressionOperator(parserState_peekKind(state, 0)))
        {
            state.advance();
            auto selector = std::make_shared<SyntaxLiteralSymbol> ();
            selector->sourcePosition = state.sourcePositionFrom(startPosition);
            selector->value = token->getValue();

            auto argument = parseUnaryPostfixExpression(state);

            auto cascadedMessage = std::make_shared<SyntaxMessageCascadeMessage> ();
            cascadedMessage->sourcePosition = state.sourcePositionFrom(startPosition);
            cascadedMessage->selector = selector;
            cascadedMessage->arguments.push_back(argument);
            return cascadedMessage;
        }
        else
        {
            return state.makeErrorAtCurrentSourcePosition("Expected a cascaded message.");
        }
    }

    ValuePtr parseMessageCascade(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        auto firstMessage = parseKeywordMessageSend(state);
        if (parserState_peekKind(state, 0) != TokenKind::Semicolon)
            return firstMessage;
        
        auto messageCascade = firstMessage->asMessageCascade();
        while (parserState_peekKind(state, 0) == TokenKind::Semicolon)
        {
            state.advance();
            auto cascadedMessage = parseCascadedMessage(state);
            messageCascade->messages.push_back(cascadedMessage);
        }

        messageCascade->sourcePosition = state.sourcePositionFrom(startPosition);
        return messageCascade;
    }

    ValuePtr parseLowPrecedenceExpression(sysmelb_parserState_t *state)
    {
        if (parserState_peekKind(state, 0) == TokenKind::Keyword)
            return parseKeywordApplication(state);
        return parseMessageCascade(state);
    }

    ValuePtr parseAssignmentExpression(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        auto assignedStore = parseLowPrecedenceExpression(state);
        if (parserState_peekKind(state, 0) == TokenKind::Assignment)
        {
            state.advance();
            auto assignedValue = parseAssignmentExpression(state);
            auto assignment = std::make_shared<SyntaxAssignment>();
            assignment->sourcePosition = state.sourcePositionFrom(startPosition);
            assignment->store = assignedStore;
            assignment->value = assignedValue;
            return assignment;
        }
        else
        {
            return assignedStore;
        }
    }

    ValuePtr parseCommaExpression(sysmelb_parserState_t *state)
    {
        auto startingPosition = state.position;
        auto element = parseAssignmentExpression(state);

        if (parserState_peekKind(state, 0) != TokenKind::Comma)
            return element;

        std::vector<ValuePtr> elements;
        elements.push_back(element);

        while (parserState_peekKind(state, 0) == TokenKind::Comma)
        {
            state.advance();
            element = parseAssignmentExpression(state);
            elements.push_back(element);
        }

        auto tuple = std::make_shared<SyntaxTuple>();
        tuple->sourcePosition = state.sourcePositionFrom(startingPosition);
        tuple->elements = elements;
        return tuple;
    }

    ValuePtr parseFunctionalType(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        auto argumentPatternOrExpression = parseCommaExpression(state);

        if (parserState_peekKind(state, 0) == TokenKind::ColonColon)
        {
            state.advance();
            auto resultTypeExpression = parseFunctionalType(state);
            auto functionalType = std::make_shared<SyntaxFunctionalDependentType>();
            functionalType->sourcePosition = state.sourcePositionFrom(startPosition);
            functionalType->argumentPattern = argumentPatternOrExpression;
            functionalType->resultType = resultTypeExpression;
            return functionalType;
        }

        return argumentPatternOrExpression;
    }

    ValuePtr parseBlock(sysmelb_parserState_t *state)
    {
        // {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::LeftCurlyBracket);
        state.advance();

        ValuePtr functionalType;
        if (parserState_peekKind(state, 0) == TokenKind::Bar)
        {
            state.advance();
            if (parserState_peekKind(state, 0) == TokenKind::Bar)
            {
                auto functionalTypeNode = std::make_shared<SyntaxFunctionalDependentType>();
                functionalTypeNode->sourcePosition = state.currentSourcePosition();
                functionalType = functionalTypeNode;
            }
            else
            {
                functionalType = parseFunctionalType(state);
                state.expectAddingErrorToNode(TokenKind::Bar, functionalType);
            }
        }
        // Body
        auto body = parseSequenceUntilEndOrDelimiter(state, TokenKind::RightCurlyBracket);
        body = state.expectAddingErrorToNode(TokenKind::RightCurlyBracket, body);

        if (functionalType)
        {
            auto block = std::make_shared<SyntaxBlock>();
            block->sourcePosition = state.sourcePositionFrom(startPosition);
            block->functionType = functionalType;
            block->body = body;
            return block;
        }
        else
        {
            auto lexicalBlock = std::make_shared<SyntaxLexicalBlock>();
            lexicalBlock->sourcePosition = state.sourcePositionFrom(startPosition);
            lexicalBlock->body = body;
            return lexicalBlock;
        }
    }

    ValuePtr parseDictionaryAssociation(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        ValuePtr key;
        ValuePtr value;
        if(parserState_peekKind(state, 0) == TokenKind::Keyword)
        {
            auto keyToken = state.next();
            auto keyTokenValue = keyToken->getValue();
            auto keySymbol = std::make_shared<SyntaxLiteralSymbol> ();
            keySymbol->sourcePosition = keyToken->position;
            keySymbol->value = keyTokenValue.substr(0, keyTokenValue.size() - 1);
            key = keySymbol;

            if(parserState_peekKind(state, 0) != TokenKind::Dot && parserState_peekKind(state, 0) != TokenKind::RightCurlyBracket)
                value = parseAssociationExpression(state);
        }
        else
        {
            key = parseBinaryExpressionSequence(state);
            if(parserState_peekKind(state, 0) == TokenKind::Colon)
            {
                state.advance();
                value = parseAssociationExpression(state);
            }
        }

        auto dictAssociation = std::make_shared<SyntaxAssociation> ();
        dictAssociation->sourcePosition = state.sourcePositionFrom(startPosition);
        dictAssociation->key = key;
        dictAssociation->value = value;
        return dictAssociation;
    }

    ValuePtr parseDictionary(sysmelb_parserState_t *state)
    {
        // {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::DictionaryStart);
        state.advance();

        // Chop the initial dots
        while (parserState_peekKind(state, 0) == TokenKind::Dot)
            state.advance();

        // Parse the next expression
        bool expectsExpression = true;
        std::vector<ValuePtr> elements;
        while (!parserState_atEnd(state) and parserState_peekKind(state, 0) != TokenKind::RightCurlyBracket)
        {
            if (!expectsExpression)
                elements.push_back(state.makeErrorAtCurrentSourcePosition("Expected dot before association."));

            auto expression = parseDictionaryAssociation(state);
            elements.push_back(expression);

            expectsExpression = false;
            // Chop the next dot sequence
            while (parserState_peekKind(state, 0) == TokenKind::Dot)
            {
                expectsExpression = true;
                state.advance();
            }
        }

        // }
        if(parserState_peekKind(state, 0) == TokenKind::RightCurlyBracket)
            state.advance();
        else
            elements.push_back(state.makeErrorAtCurrentSourcePosition("Expected a right curly bracket."));

        auto dictionary = std::make_shared<SyntaxDictionary> ();
        dictionary->sourcePosition = state.sourcePositionFrom(startPosition);
        dictionary->elements.swap(elements);
        return dictionary;
    }
    

    void parseOptionalBindableNameType(sysmelb_parserState_t *state, bool &outIsImplicit, ValuePtr &outTypeExpression)
    {
        if(parserState_peekKind(state, 0) == TokenKind::LeftBracket)
        {
            state.advance();
            outTypeExpression = parseExpression(state);
            outTypeExpression = state.expectAddingErrorToNode(TokenKind::RightBracket, outTypeExpression);
            outIsImplicit = true;
        }
        else if(parserState_peekKind(state, 0) == TokenKind::LeftParent)
        {
            state.advance();
            outTypeExpression = parseExpression(state);
            outTypeExpression = state.expectAddingErrorToNode(TokenKind::RightParent, outTypeExpression);
            outIsImplicit = false;
        }
        else
        {
            outIsImplicit = false;
            outTypeExpression.reset();
        }
    }

    ValuePtr parseNameExpression(sysmelb_parserState_t *state)
    {
        if(parserState_peekKind(state, 0) == TokenKind::Identifier)
        {
            auto token = state.next();
            auto nameSymbol = std::make_shared<SyntaxLiteralSymbol> ();
            nameSymbol->sourcePosition = token->position;
            nameSymbol->value = token->getValue();
            return nameSymbol;
        }
        else
        {
            return state.makeErrorAtCurrentSourcePosition("Expected a bindable name.");
        }
    }

    ValuePtr parseBindableName(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        assert(parserState_peekKind(state, 0) == TokenKind::Colon);
        state.advance();

        bool isExistential = false;
        bool isMutable = false;

        if (parserState_peekKind(state, 0) == TokenKind::Bang)
        {
            isMutable = true;
            state.advance();
        }
        if (parserState_peekKind(state, 0) == TokenKind::Question)
        {
            isExistential = isExistential || (parserState_peekKind(state, 0) == TokenKind::Question);
            state.advance();
        }

        bool isImplicit = false;
        ValuePtr typeExpression;
        parseOptionalBindableNameType(state, isImplicit, typeExpression);
        bool hasPostTypeExpression = false;

        bool isVariadic = false;
        ValuePtr nameExpression;
        if (parserState_peekKind(state, 0) == TokenKind::Ellipsis)
        {
            state.advance();
            isVariadic = true;
            nameExpression.reset();
        }
        else
        {
            nameExpression = parseNameExpression(state);
            if(!typeExpression)
            {
                parseOptionalBindableNameType(state, isImplicit, typeExpression);
                hasPostTypeExpression = typeExpression.get() != nullptr;    
            }

            if(parserState_peekKind(state, 0) == TokenKind::Ellipsis)
            {
                state.advance();
                isVariadic = true;
            }
        }

        auto bindableName = std::make_shared<SyntaxBindableName> ();
        bindableName->sourcePosition = state.sourcePositionFrom(startPosition);
        bindableName->typeExpression = typeExpression;
        bindableName->nameExpression = nameExpression;

        bindableName->isImplicit    = isImplicit;
        bindableName->isExistential = isExistential;
        bindableName->isVariadic    = isVariadic;
        bindableName->isMutable     = isMutable;
        bindableName->hasPostTypeExpression = hasPostTypeExpression;
        return bindableName;
    }

    ValuePtr parseTerm(sysmelb_parserState_t *state)
    {
        switch (parserState_peekKind(state, 0))
        {
        case TokenKind::Identifier:
            return parseIdentifier(state);
        case TokenKind::LeftParent:
            return parseParenthesis(state);
        case TokenKind::LeftCurlyBracket:
            return parseBlock(state);
        case TokenKind::LeftBracket:
            return parseArray(state);
        case TokenKind::ByteArrayStart:
            return parseByteArray(state);
        case TokenKind::DictionaryStart:
            return parseDictionary(state);
        case TokenKind::Colon:
            return parseBindableName(state);
        default:
            return parseLiteral(state);
        }
    }

    ValuePtr parseFunctionalTypeWithOptionalArgument(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        if (parserState_peekKind(state, 0) == TokenKind::ColonColon)
        {
            state.advance();
            auto resultTypeExpression = parseFunctionalType(state);
            auto functionalNode = std::make_shared<SyntaxFunctionalDependentType>();
            functionalNode->sourcePosition = state.sourcePositionFrom(startPosition);
            functionalNode->resultType = resultTypeExpression;
            return functionalNode;
        }
        else
        {
            return parseFunctionalType(state);
        }
    }

    ValuePtr parseBindExpression(sysmelb_parserState_t *state)
    {
        auto startPosition = state.position;
        auto patternExpressionOrValue = parseFunctionalTypeWithOptionalArgument(state);
        if (parserState_peekKind(state, 0) == TokenKind::BindOperator)
        {
            state.advance();
            auto boundValue = parseBindExpression(state);
            auto bindPattern = std::make_shared<SyntaxBindPattern>();
            bindPattern->sourcePosition = state.sourcePositionFrom(startPosition);
            bindPattern->pattern = patternExpressionOrValue;
            bindPattern->value = boundValue;
            return bindPattern;
        }
        else
        {
            return patternExpressionOrValue;
        }
    }

    ValuePtr parseExpression(sysmelb_parserState_t *state)
    {
        return parseBindExpression(state);
    }
*/

sysmelb_ParseTreeNode_t *parser_parseExpression(sysmelb_parserState_t *state)
{
    return parser_parseLiteral(state);
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
    if(expressions.size == 1)
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
