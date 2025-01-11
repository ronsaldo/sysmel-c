#include "scanner.h"
#include "memory.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

bool scanner_isDigit(int character)
{
    return '0' <= character && character <= '9';
}

bool scanner_isIdentifierStart(int character)
{
    return
        ('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z') ||
        (character == '_')
        ;
}

bool scanner_isIdentifierMiddle(int character)
{
    return scanner_isIdentifierStart(character) || scanner_isDigit(character);
}

bool scanner_isOperatorCharacter(int character)
{
    const char *charset = "+-/\\*~<>=@%|&?!^";
    while(*charset != 0)
    {
        if(*charset == character)
            return true;
        ++charset;
    }
    return false;
}

typedef struct sysmelb_scannerState_s
{
    sysmelb_SourceCode_t *sourceCode;
    int32_t position;
    int32_t line;
    int32_t column;
    bool isPreviousCR;
}sysmelb_scannerState_t;

sysmelb_scannerState_t scannerState_newForSourceCode(sysmelb_SourceCode_t *sourceCode)
{
    sysmelb_scannerState_t state = {
        .sourceCode = sourceCode,
        .position = 0, .line = 1, .column = 1,
        .isPreviousCR = false 
    };

    return state;
}

bool scannerState_atEnd(sysmelb_scannerState_t *state)
{
    return (size_t)(state->position) >= state->sourceCode->textSize;
}

int scannerState_peek(sysmelb_scannerState_t *state, int peekOffset)
{
    size_t peekPosition = state->position + peekOffset;
    if(peekPosition < state->sourceCode->textSize)
        return state->sourceCode->text[peekPosition];
    else
        return -1;
}

void scannerState_advanceSinglePosition(sysmelb_scannerState_t *state)
{
    assert(!scannerState_atEnd(state));
    char c = state->sourceCode->text[state->position];
    ++state->position;
    switch(c)
    {
    case '\r':
        ++state->line;
        state->column = 1;
        state->isPreviousCR = true;
        break;
    case '\n':
        if (!state->isPreviousCR)
        {
            ++state->line;
            state->column = 1;
        }
        state->isPreviousCR = false;
        break;
    case '\t':
        state->column = (state->column + 4) % 4 * 4 + 1;
        state->isPreviousCR = false;
        break;
    default:
        ++state->column;
        state->isPreviousCR = false;
        break;
    }
}

void scannerState_advance(sysmelb_scannerState_t *state, int count)
{
    for(int i = 0; i < count; ++i)
    {
        scannerState_advanceSinglePosition(state);
    }
}

sysmelb_ScannerToken_t scannerState_makeToken(sysmelb_scannerState_t *state, sysmelb_TokenKind_t kind)
{
    sysmelb_SourcePosition_t sourcePosition = {
        .sourceCode = state->sourceCode,
        .startIndex = state->position,
        .startLine = state->line,
        .startColumn = state->column,
        .endIndex = state->position,
        .endLine = state->line,
        .endColumn = state->column,
    };

    sysmelb_ScannerToken_t token = {
        .kind = kind,
        .sourcePosition = sourcePosition,
    };
    return token;
}

sysmelb_ScannerToken_t scannerState_makeTokenStartingFrom(sysmelb_scannerState_t *state, sysmelb_TokenKind_t kind, sysmelb_scannerState_t *initialState)
{
    sysmelb_SourcePosition_t sourcePosition = {
        .sourceCode = state->sourceCode,
        .startIndex = initialState->position,
        .startLine = initialState->line,
        .startColumn = initialState->column,
        .endIndex = state->position,
        .endLine = state->line,
        .endColumn = state->column,
    };

    sysmelb_ScannerToken_t token = {
        .kind = kind,
        .sourcePosition = sourcePosition,
    };

    return token;
}

sysmelb_ScannerToken_t scannerState_makeErrorTokenStartingFrom(sysmelb_scannerState_t *state, const char *errorMessage, const sysmelb_scannerState_t *initialState)
{
    sysmelb_SourcePosition_t sourcePosition = {
        .sourceCode = state->sourceCode,
        .startIndex = initialState->position,
        .startLine = initialState->line,
        .startColumn = initialState->column,
        .endIndex = state->position,
        .endLine = state->line,
        .endColumn = state->column,
    };

    sysmelb_ScannerToken_t token = {
        .kind = SysmelTokenError,
        .sourcePosition = sourcePosition,
        .errorMessage = errorMessage,
    };
    
    return token;
}

sysmelb_ScannerToken_t skipWhite(sysmelb_scannerState_t *state)
{
    bool hasSeenComment = false;
    
    do
    {
        hasSeenComment = false;
        while (!scannerState_atEnd(state) && scannerState_peek(state, 0) <= ' ')
            scannerState_advance(state, 1);

        if(scannerState_peek(state, 0) == '#')
        {
            // Single line comment.
            if(scannerState_peek(state, 1) == '#')
            {
                scannerState_advance(state, 2);

                while (!scannerState_atEnd(state))
                {
                    if (scannerState_peek(state, 0) == '\r' || scannerState_peek(state, 0) == '\n')
                        break;
                    scannerState_advance(state, 1);
                }
                hasSeenComment = true;
            }
            else if(scannerState_peek(state, 1) == '*')
            {
                sysmelb_scannerState_t commentInitialState = *state;
                scannerState_advance(state, 2);
                bool hasCommentEnd = false;
                while (!scannerState_atEnd(state))
                {
                    hasCommentEnd = scannerState_peek(state, 0) == '*' &&  scannerState_peek(state, 0) == '#';
                    if (hasCommentEnd)
                    {
                        scannerState_advance(state, 2);
                        break;
                    }
                }
                if (!hasCommentEnd)
                {
                    return scannerState_makeErrorTokenStartingFrom(state, "Incomplete multiline comment.", &commentInitialState);
                }
            }
        }
    } while (hasSeenComment);
    
    sysmelb_ScannerToken_t nullToken = {0};
    return nullToken;
}


bool scanAdvanceKeyword(sysmelb_scannerState_t *state)
{
    if(!scanner_isIdentifierStart(scannerState_peek(state, 0)))
        return false;

    sysmelb_scannerState_t initialState = *state;
    while (scanner_isIdentifierMiddle(scannerState_peek(state, 0)))
        scannerState_advance(state, 1);

    if(scannerState_peek(state, 0) != ':')
    {
        *state = initialState;
        return false;
    }

    return true;
}

sysmelb_ScannerToken_t sysmelb_scanSingleToken(sysmelb_scannerState_t *state)
{
    sysmelb_ScannerToken_t whiteToken = skipWhite(state);
    if(whiteToken.kind)
        return whiteToken;

    if(scannerState_atEnd(state))
        return scannerState_makeToken(state, SysmelTokenEndOfSource);

    sysmelb_scannerState_t initialState = *state;
    int c = scannerState_peek(state, 0);

    // Identifiers, keywords and multi-keywords
    if(scanner_isIdentifierStart(c))
    {
        scannerState_advance(state, 1);
        while (scanner_isIdentifierMiddle(scannerState_peek(state, 0)))
            scannerState_advance(state, 1);

        if(scannerState_peek(state, 0) == ':')
        {
            scannerState_advance(state, 1);
            bool isMultiKeyword = false;
            bool hasAdvanced = true;
            while(hasAdvanced)
            {
                hasAdvanced = scanAdvanceKeyword(state);
                isMultiKeyword = isMultiKeyword || hasAdvanced;
            }

            if(isMultiKeyword)
                return scannerState_makeTokenStartingFrom(state, SysmelTokenMultiKeyword, &initialState);
            else
                return scannerState_makeTokenStartingFrom(state, SysmelTokenKeyword, &initialState);
        }

        return scannerState_makeTokenStartingFrom(state, SysmelTokenIdentifier, &initialState);
    }

    // Numbers
    if(scanner_isDigit(c))
    {
        scannerState_advance(state, 1);
        while(scanner_isDigit(scannerState_peek(state, 0)))
            scannerState_advance(state, 1);

        // Parse the radix
        if(scannerState_peek(state, 0) == 'r')
        {
            scannerState_advance(state, 1);
            while(scanner_isIdentifierMiddle(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenNat, &initialState);
        }

        // Parse the decimal point
        if(scannerState_peek(state, 0) == '.' && scanner_isDigit(scannerState_peek(state, 1)))
        {
            scannerState_advance(state, 2);
            while(scanner_isDigit(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);

            // Parse the exponent
            if(scannerState_peek(state, 0) == 'e' || scannerState_peek(state, 0) == 'E')
            {
                if(scanner_isDigit(scannerState_peek(state, 1)) ||
                ((scannerState_peek(state, 1) == '+' || scannerState_peek(state, 1) == '-') && scanner_isDigit(scannerState_peek(state, 2) )))
                {
                    scannerState_advance(state, 2);
                    while(scanner_isDigit(scannerState_peek(state, 0)))
                        scannerState_advance(state, 1);
                }
            }

            return scannerState_makeTokenStartingFrom(state, SysmelTokenFloat, &initialState);
        }

        return scannerState_makeTokenStartingFrom(state, SysmelTokenNat, &initialState);
    }

    // Symbols
    if(c == '#')
    {
        int c1 = scannerState_peek(state, 1);
        if(scanner_isIdentifierStart(c1))
        {
            scannerState_advance(state, 2);
            while (scanner_isIdentifierMiddle(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);


            if (scannerState_peek(state, 0) == ':')
            {
                scannerState_advance(state, 1);
                bool hasAdvanced = true;
                while(hasAdvanced)
                {
                    hasAdvanced = scanAdvanceKeyword(state);
                }

                return scannerState_makeTokenStartingFrom(state, SysmelTokenSymbol, &initialState); 
            }
            return scannerState_makeTokenStartingFrom(state, SysmelTokenSymbol, &initialState); 
        }
        else if(scanner_isOperatorCharacter(c1))
        {
            scannerState_advance(state, 2);
            while(scanner_isOperatorCharacter(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenSymbol, &initialState);
        }
        else if(c1 == '"')
        {
            scannerState_advance(state, 2);
            while (!scannerState_atEnd(state) && scannerState_peek(state, 0) != '"')
            {
                if(scannerState_peek(state, 0) == '\\' && scannerState_peek(state, 1) > 0)
                    scannerState_advance(state, 2);
                else
                    scannerState_advance(state, 1);
            }

            if (scannerState_peek(state, 0) != '"')
                return scannerState_makeErrorTokenStartingFrom(state, "Incomplete symbol string literal.", &initialState);
            
            scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenSymbol, &initialState);
        }
        else if (c1 == '[')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenByteArrayStart, &initialState);
        }
        else if (c1 == '{')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenDictionaryStart, &initialState);
        }
        else if (c1 == '(')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenLiteralArrayStart, &initialState);
        }
    }

    // Strings
    if(c == '"')
    {
        scannerState_advance(state, 1);
        while (!scannerState_atEnd(state) && scannerState_peek(state, 0) != '"')
        {
            if(scannerState_peek(state, 0) == '\\' && scannerState_peek(state, 1) > 0)
                scannerState_advance(state, 2);
            else
                scannerState_advance(state, 1);
        }

        if (scannerState_peek(state, 0) != '"')
            return scannerState_makeErrorTokenStartingFrom(state, "Incomplete string literal.", &initialState);
        
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenString, &initialState);
    }

    // Character
    if(c == '\'')
    {
        scannerState_advance(state, 1);
        while (!scannerState_atEnd(state) && scannerState_peek(state, 0) != '\'')
        {
            if(scannerState_peek(state, 0) == '\\' && scannerState_peek(state, 1) > 0)
                scannerState_advance(state, 2);
            else
                scannerState_advance(state, 1);
        }

        if (scannerState_peek(state, 0) != '\'')
            return scannerState_makeErrorTokenStartingFrom(state, "Incomplete character literal.", &initialState);
        
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenCharacter, &initialState);
    }

    switch(c)
    {
    case '(':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenLeftParent, &initialState);
    case ')':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenRightParent, &initialState);
    case '[':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenLeftBracket, &initialState);
    case ']':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenRightBracket, &initialState);
    case '{':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenLeftCurlyBracket, &initialState);
    case '}':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenRightCurlyBracket, &initialState);
    case ';':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenSemicolon, &initialState);
    case ',':
        scannerState_advance(state, 1);
        return scannerState_makeTokenStartingFrom(state, SysmelTokenComma, &initialState);
    case '.':
        scannerState_advance(state, 1);
        if(scannerState_peek(state, 0) == '.' && scannerState_peek(state, 1) == '.')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenEllipsis, &initialState);
        }
        return scannerState_makeTokenStartingFrom(state, SysmelTokenDot, &initialState);
    case ':':
        scannerState_advance(state, 1);
        if(scannerState_peek(state, 0) == ':')
        {
            scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenColonColon, &initialState);
        }
        else if(scannerState_peek(state, 0) == '=')
        {
            scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenAssignment, &initialState);
        }
        return scannerState_makeTokenStartingFrom(state, SysmelTokenColon, &initialState);
    case '`':
        if (scannerState_peek(state, 0) == '\'')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenQuote, &initialState);
        }
        else if (scannerState_peek(state, 0) == '`')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenQuasiQuote, &initialState);
        }
        else if (scannerState_peek(state, 0) == ',')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenQuasiUnquote, &initialState);
        }
        else if (scannerState_peek(state, 0) == '@')
        {
            scannerState_advance(state, 2);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenSplice, &initialState);
        }
        break;
    case '|':
        scannerState_advance(state, 1);
        if (scanner_isOperatorCharacter(scannerState_peek(state, 0)))
        {
            while(scanner_isOperatorCharacter(scannerState_peek(state, 0)))
                scannerState_advance(state, 1);
            return scannerState_makeTokenStartingFrom(state, SysmelTokenOperator, &initialState);
        }

        return scannerState_makeTokenStartingFrom(state, SysmelTokenBar, &initialState);
    default:
        break;
    }

    if(scanner_isOperatorCharacter(c))
    {
        scannerState_advance(state, 1);
        if(!scanner_isOperatorCharacter(scannerState_peek(state, 0)))
        {
            switch(c)
            {
            case '<':
                return scannerState_makeTokenStartingFrom(state, SysmelTokenLessThan, &initialState);
            case '>':
                return scannerState_makeTokenStartingFrom(state, SysmelTokenGreaterThan, &initialState);
            case '*':
                return scannerState_makeTokenStartingFrom(state, SysmelTokenStar, &initialState);
            case '!':
                return scannerState_makeTokenStartingFrom(state, SysmelTokenBang, &initialState);
            default:
                // Generic character, do nothing.
                break;
            }
        }

        while(scanner_isOperatorCharacter(scannerState_peek(state, 0)))
            scannerState_advance(state, 1);

        return scannerState_makeTokenStartingFrom(state, SysmelTokenOperator, &initialState);
    }

    scannerState_advance(state, 1);
    return scannerState_makeErrorTokenStartingFrom(state, "Unknown character", &initialState);
}

void tokenDynarray_increaseCapacity(sysmelb_TokenDynarray_t *dynarray)
{
    size_t newCapacity = dynarray->capacity * 2;
    if(newCapacity < 32)
        newCapacity = 32;

    void *newStorage = sysmelb_allocate(newCapacity * sizeof(sysmelb_ScannerToken_t));
    memcpy(newStorage, dynarray->tokens, dynarray->size * sizeof(sysmelb_ScannerToken_t));

    dynarray->capacity = newCapacity;
    dynarray->tokens = newStorage;
}

void tokenDynarray_add(sysmelb_TokenDynarray_t *dynarray, sysmelb_ScannerToken_t token)
{
    if(dynarray->size >= dynarray->capacity)
        tokenDynarray_increaseCapacity(dynarray);
    dynarray->tokens[dynarray->size++] = token;
}

sysmelb_TokenDynarray_t sysmelb_scanSourceCode(sysmelb_SourceCode_t *sourceCode)
{
    sysmelb_TokenDynarray_t dynarray = {0};
    sysmelb_scannerState_t currentState = scannerState_newForSourceCode(sourceCode);
    sysmelb_ScannerToken_t scannedToken;
    do
    {
        scannedToken = sysmelb_scanSingleToken(&currentState);
        if(scannedToken.kind != SysmelTokenNullToken)
        {
            tokenDynarray_add(&dynarray, scannedToken);
            if (scannedToken.kind == SysmelTokenEndOfSource)
                break;
        }
        
    }
    while(scannedToken.kind != SysmelTokenEndOfSource);

    return dynarray;
}