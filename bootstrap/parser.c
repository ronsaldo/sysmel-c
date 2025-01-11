#include "parser.h"
#include "parse-tree.h"
#include <stdbool.h>
#include <assert.h>

typedef struct sysmelb_parserState_s sysmelb_parserState_t;
sysmelb_SourcePosition_t parserState_currentSourcePosition(sysmelb_parserState_t *state);

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
/*
    std::vector<ValuePtr> parseExpressionListUntilEndOrDelimiter(ParserState &state, TokenKind delimiter);
    ValuePtr parseSequenceUntilEndOrDelimiter(ParserState &state, TokenKind delimiter);
    ValuePtr parseTerm(ParserState &state);
    ValuePtr parseUnaryPrefixExpression(ParserState &state);
    ValuePtr parseBlock(ParserState &state);
    ValuePtr parseDictionary(ParserState &state);
    ValuePtr parseExpression(ParserState &state);

    LargeInteger parseIntegerConstant(const std::string &constant)
    {
        LargeInteger result = LargeInteger::Zero;
        LargeInteger radix = LargeInteger::Ten;
        bool hasSeenRadix = false;
        for (size_t i = 0; i < constant.size(); ++i)
        {
            auto c = constant[i];
            if (!hasSeenRadix && (c == 'r' || c == 'R'))
            {
                hasSeenRadix = true;
                radix = result;
                result = LargeInteger::Zero;
            }
            else
            {
                if ('0' <= c && c <= '9')
                    result = result * radix + LargeInteger(c - '0');
                else if ('A' <= c && c <= 'Z')
                    result = result * radix + LargeInteger(c - 'A' + 10);
                else if ('a' <= c && c <= 'z')
                    result = result * radix + LargeInteger(c - 'a' + 10);
            }
        }
        return result;
    }

    ValuePtr parseLiteralInteger(ParserState &state)
    {
        auto token = state.next();
        assert(token->kind == TokenKind::Nat);
        auto literal = std::make_shared<SyntaxLiteralInteger>();
        literal->sourcePosition = token->position;
        literal->value = parseIntegerConstant(token->getValue());
        return literal;
    }

    ValuePtr parseLiteralFloat(ParserState &state)
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

    ValuePtr parseLiteralCharacter(ParserState &state)
    {
        auto token = state.next();
        assert(token->kind == TokenKind::Character);
        auto literal = std::make_shared<SyntaxLiteralCharacter>();
        literal->sourcePosition = token->position;
        auto tokenValue = token->getValue();
        literal->value = parseCEscapedString(tokenValue.substr(1, tokenValue.size() - 2))[0];
        return literal;
    }

    ValuePtr parseLiteralString(ParserState &state)
    {
        auto token = state.next();
        assert(token->kind == TokenKind::String);
        auto literal = std::make_shared<SyntaxLiteralString>();
        literal->sourcePosition = token->position;
        auto tokenValue = token->getValue();
        literal->value = parseCEscapedString(tokenValue.substr(1, tokenValue.size() - 2));
        return literal;
    }

    ValuePtr parseLiteralSymbol(ParserState &state)
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

    ValuePtr parseLiteral(ParserState &state)
    {
        switch (state.peekKind())
        {
        case TokenKind::Nat:
            return parseLiteralInteger(state);
        case TokenKind::Float:
            return parseLiteralFloat(state);
        case TokenKind::Character:
            return parseLiteralCharacter(state);
        case TokenKind::String:
            return parseLiteralString(state);
        case TokenKind::Symbol:
            return parseLiteralSymbol(state);
        default:
            return state.advanceWithExpectedError("Expected a literal");
        }
    }

    ValuePtr parseIdentifier(ParserState &state)
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

    ValuePtr parseParenthesis(ParserState &state)
    {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::LeftParent);
        state.advance();

        if (isBinaryExpressionOperator(state.peekKind()) && state.peekKind(1) == TokenKind::RightParent)
        {
            auto token = state.next();
            state.advance();
        }

        if (state.peekKind() == TokenKind::RightParent)
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

    ValuePtr parseArray(ParserState &state)
    {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::LeftBracket);
        state.advance();

        auto expressions = parseExpressionListUntilEndOrDelimiter(state, TokenKind::RightBracket);

        if(state.peekKind() == TokenKind::RightBracket)
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

    ValuePtr parseByteArray(ParserState &state)
    {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::ByteArrayStart);
        state.advance();

        auto expressions = parseExpressionListUntilEndOrDelimiter(state, TokenKind::RightBracket);

        if(state.peekKind() == TokenKind::RightBracket)
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

    ValuePtr parseUnaryPostfixExpression(ParserState &state)
    {
        auto startPosition = state.position;
        auto receiver = parseTerm(state);

        while (isUnaryPostfixTokenKind(state.peekKind()))
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
                    if(state.peekKind() == TokenKind::RightParent)
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
                    if(state.peekKind() == TokenKind::RightBracket)
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

    ValuePtr parseQuote(ParserState &state)
    {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::Quote);
        state.advance();
        auto term = parseUnaryPrefixExpression(state);
        auto quoteNode = std::make_shared<SyntaxQuote>();
        quoteNode->sourcePosition = state.sourcePositionFrom(startPosition);
        quoteNode->value = term;
        return quoteNode;
    }

    ValuePtr parseQuasiQuote(ParserState &state)
    {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::QuasiQuote);
        state.advance();
        auto term = parseUnaryPrefixExpression(state);
        auto quoteNode = std::make_shared<SyntaxQuasiQuote>();
        quoteNode->sourcePosition = state.sourcePositionFrom(startPosition);
        quoteNode->value = term;
        return quoteNode;
    }

    ValuePtr parseQuasiUnquote(ParserState &state)
    {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::QuasiUnquote);
        state.advance();
        auto term = parseUnaryPrefixExpression(state);
        auto quoteNode = std::make_shared<SyntaxQuasiUnquote>();
        quoteNode->sourcePosition = state.sourcePositionFrom(startPosition);
        quoteNode->value = term;
        return quoteNode;
    }

    ValuePtr parseSplice(ParserState &state)
    {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::Splice);
        state.advance();
        auto term = parseUnaryPrefixExpression(state);
        auto spliceNode = std::make_shared<SyntaxSplice>();
        spliceNode->sourcePosition = state.sourcePositionFrom(startPosition);
        spliceNode->value = term;
        return spliceNode;
    }

    ValuePtr parseUnaryPrefixExpression(ParserState &state)
    {
        switch (state.peekKind())
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

    ValuePtr parseBinaryExpressionSequence(ParserState &state)
    {
        auto startPosition = state.position;
        auto operand = parseUnaryPrefixExpression(state);
        if (!isBinaryExpressionOperator(state.peekKind()))
            return operand;

        std::vector<ValuePtr> elements;
        elements.push_back(operand);
        while (isBinaryExpressionOperator(state.peekKind()))
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

    ValuePtr parseAssociationExpression(ParserState &state)
    {
        auto startPosition = state.position;
        auto key = parseBinaryExpressionSequence(state);

        if (state.peekKind() != TokenKind::Colon)
            return key;

        state.advance();
        auto value = parseAssociationExpression(state);
        auto assoc = std::make_shared<SyntaxAssociation>();
        assoc->sourcePosition = state.sourcePositionFrom(startPosition);
        assoc->key = key;
        assoc->value = value;
        return assoc;
    }

    ValuePtr parseKeywordApplication(ParserState &state)
    {
        assert(state.peekKind() == TokenKind::Keyword);
        auto startPosition = state.position;

        std::string symbolValue;
        std::vector<ValuePtr> arguments;

        while (state.peekKind() == TokenKind::Keyword)
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

    ValuePtr parseKeywordMessageSend(ParserState &state)
    {
        auto startPosition = state.position;
        auto receiver = parseAssociationExpression(state);
        if(state.peekKind() != TokenKind::Keyword)
            return receiver;
        
        std::string selectorValue;
        std::vector<ValuePtr> arguments;

        while(state.peekKind() == TokenKind::Keyword)
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

    ValuePtr parseCascadedMessage(ParserState &state)
    {
        auto startPosition = state.position;
        auto token = state.peek();
        if(state.peekKind() == TokenKind::Identifier)
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
        else if(state.peekKind() == TokenKind::Keyword)
        {
            std::string selectorValue;
            std::vector<ValuePtr> arguments;

            while(state.peekKind() == TokenKind::Keyword)
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
        else if(isBinaryExpressionOperator(state.peekKind()))
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

    ValuePtr parseMessageCascade(ParserState &state)
    {
        auto startPosition = state.position;
        auto firstMessage = parseKeywordMessageSend(state);
        if (state.peekKind() != TokenKind::Semicolon)
            return firstMessage;
        
        auto messageCascade = firstMessage->asMessageCascade();
        while (state.peekKind() == TokenKind::Semicolon)
        {
            state.advance();
            auto cascadedMessage = parseCascadedMessage(state);
            messageCascade->messages.push_back(cascadedMessage);
        }

        messageCascade->sourcePosition = state.sourcePositionFrom(startPosition);
        return messageCascade;
    }

    ValuePtr parseLowPrecedenceExpression(ParserState &state)
    {
        if (state.peekKind() == TokenKind::Keyword)
            return parseKeywordApplication(state);
        return parseMessageCascade(state);
    }

    ValuePtr parseAssignmentExpression(ParserState &state)
    {
        auto startPosition = state.position;
        auto assignedStore = parseLowPrecedenceExpression(state);
        if (state.peekKind() == TokenKind::Assignment)
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

    ValuePtr parseCommaExpression(ParserState &state)
    {
        auto startingPosition = state.position;
        auto element = parseAssignmentExpression(state);

        if (state.peekKind() != TokenKind::Comma)
            return element;

        std::vector<ValuePtr> elements;
        elements.push_back(element);

        while (state.peekKind() == TokenKind::Comma)
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

    ValuePtr parseFunctionalType(ParserState &state)
    {
        auto startPosition = state.position;
        auto argumentPatternOrExpression = parseCommaExpression(state);

        if (state.peekKind() == TokenKind::ColonColon)
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

    ValuePtr parseBlock(ParserState &state)
    {
        // {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::LeftCurlyBracket);
        state.advance();

        ValuePtr functionalType;
        if (state.peekKind() == TokenKind::Bar)
        {
            state.advance();
            if (state.peekKind() == TokenKind::Bar)
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

    ValuePtr parseDictionaryAssociation(ParserState &state)
    {
        auto startPosition = state.position;
        ValuePtr key;
        ValuePtr value;
        if(state.peekKind() == TokenKind::Keyword)
        {
            auto keyToken = state.next();
            auto keyTokenValue = keyToken->getValue();
            auto keySymbol = std::make_shared<SyntaxLiteralSymbol> ();
            keySymbol->sourcePosition = keyToken->position;
            keySymbol->value = keyTokenValue.substr(0, keyTokenValue.size() - 1);
            key = keySymbol;

            if(state.peekKind() != TokenKind::Dot && state.peekKind() != TokenKind::RightCurlyBracket)
                value = parseAssociationExpression(state);
        }
        else
        {
            key = parseBinaryExpressionSequence(state);
            if(state.peekKind() == TokenKind::Colon)
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

    ValuePtr parseDictionary(ParserState &state)
    {
        // {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::DictionaryStart);
        state.advance();

        // Chop the initial dots
        while (state.peekKind() == TokenKind::Dot)
            state.advance();

        // Parse the next expression
        bool expectsExpression = true;
        std::vector<ValuePtr> elements;
        while (!state.atEnd() and state.peekKind() != TokenKind::RightCurlyBracket)
        {
            if (!expectsExpression)
                elements.push_back(state.makeErrorAtCurrentSourcePosition("Expected dot before association."));

            auto expression = parseDictionaryAssociation(state);
            elements.push_back(expression);

            expectsExpression = false;
            // Chop the next dot sequence
            while (state.peekKind() == TokenKind::Dot)
            {
                expectsExpression = true;
                state.advance();
            }
        }

        // }
        if(state.peekKind() == TokenKind::RightCurlyBracket)
            state.advance();
        else
            elements.push_back(state.makeErrorAtCurrentSourcePosition("Expected a right curly bracket."));

        auto dictionary = std::make_shared<SyntaxDictionary> ();
        dictionary->sourcePosition = state.sourcePositionFrom(startPosition);
        dictionary->elements.swap(elements);
        return dictionary;
    }
    

    void parseOptionalBindableNameType(ParserState &state, bool &outIsImplicit, ValuePtr &outTypeExpression)
    {
        if(state.peekKind() == TokenKind::LeftBracket)
        {
            state.advance();
            outTypeExpression = parseExpression(state);
            outTypeExpression = state.expectAddingErrorToNode(TokenKind::RightBracket, outTypeExpression);
            outIsImplicit = true;
        }
        else if(state.peekKind() == TokenKind::LeftParent)
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

    ValuePtr parseNameExpression(ParserState &state)
    {
        if(state.peekKind() == TokenKind::Identifier)
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

    ValuePtr parseBindableName(ParserState &state)
    {
        auto startPosition = state.position;
        assert(state.peekKind() == TokenKind::Colon);
        state.advance();

        bool isExistential = false;
        bool isMutable = false;

        if (state.peekKind() == TokenKind::Bang)
        {
            isMutable = true;
            state.advance();
        }
        if (state.peekKind() == TokenKind::Question)
        {
            isExistential = isExistential || (state.peekKind() == TokenKind::Question);
            state.advance();
        }

        bool isImplicit = false;
        ValuePtr typeExpression;
        parseOptionalBindableNameType(state, isImplicit, typeExpression);
        bool hasPostTypeExpression = false;

        bool isVariadic = false;
        ValuePtr nameExpression;
        if (state.peekKind() == TokenKind::Ellipsis)
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

            if(state.peekKind() == TokenKind::Ellipsis)
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

    ValuePtr parseTerm(ParserState &state)
    {
        switch (state.peekKind())
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

    ValuePtr parseFunctionalTypeWithOptionalArgument(ParserState &state)
    {
        auto startPosition = state.position;
        if (state.peekKind() == TokenKind::ColonColon)
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

    ValuePtr parseBindExpression(ParserState &state)
    {
        auto startPosition = state.position;
        auto patternExpressionOrValue = parseFunctionalTypeWithOptionalArgument(state);
        if (state.peekKind() == TokenKind::BindOperator)
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

    ValuePtr parseExpression(ParserState &state)
    {
        return parseBindExpression(state);
    }

    std::vector<ValuePtr> parseExpressionListUntilEndOrDelimiter(ParserState &state, TokenKind delimiter)
    {
        std::vector<ValuePtr> elements;

        // Leading dots.
        while (state.peekKind() == TokenKind::Dot)
            state.advance();

        bool expectsExpression = true;

        while (!state.atEnd() && state.peekKind() != delimiter)
        {
            if (!expectsExpression)
                elements.push_back(state.makeErrorAtCurrentSourcePosition("Expected dot before expression."));
            auto expression = parseExpression(state);
            elements.push_back(expression);

            // Trailing dots.
            while (state.peekKind() == TokenKind::Dot)
            {
                expectsExpression = true;
                state.advance();
            }
        }

        return elements;
    }

    ValuePtr parseSequenceUntilEndOrDelimiter(ParserState &state, TokenKind delimiter)
    {
        auto initialPosition = state.position;
        auto expressions = parseExpressionListUntilEndOrDelimiter(state, delimiter);
        if (expressions.size() == 1)
            return expressions[0];

        auto syntaxSequence = std::make_shared<SyntaxValueSequence>();
        syntaxSequence->sourcePosition = state.sourcePositionFrom(initialPosition);
        syntaxSequence->elements = expressions;
        return syntaxSequence;
    }

    ValuePtr parseTopLevelExpression(ParserState &state)
    {
        return parseSequenceUntilEndOrDelimiter(state, TokenKind::EndOfSource);
    }

    ValuePtr parseTokens(const SourceCodePtr &sourceCode, const std::vector<TokenPtr> &tokens)
    {
        auto state = ParserState{sourceCode, &tokens};
        return parseTopLevelExpression(state);
    }

} // End of namespace Sysmel
*/

sysmelb_ParseTreeNode_t *parseTokenList(size_t tokenCount, sysmelb_ScannerToken_t *tokens)
{
    return NULL;
}
