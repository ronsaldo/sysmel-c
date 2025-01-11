#ifndef SYSMELB_SCANNER_H
#define SYSMELB_SCANNER_H

#pragma once

#include "source-code.h"
#include "token.h"

typedef struct sysmelb_ScannerToken_s
{
    sysmelb_TokenKind_t kind;
    sysmelb_SourcePosition_t sourcePosition;
    const char *textPosition;
    size_t textSize;
    const char *errorMessage;
} sysmelb_ScannerToken_t;

typedef struct sysmelb_TokenDynarray_s
{
    size_t size;
    size_t capacity;
    sysmelb_ScannerToken_t *tokens;
}sysmelb_TokenDynarray_t;

sysmelb_TokenDynarray_t sysmelb_scanSourceCode(sysmelb_SourceCode_t *sourceCode);
#endif //SYSMELB_SCANNER_H
