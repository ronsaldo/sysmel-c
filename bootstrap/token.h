#ifndef SYSMELB_TOKEN_H
#define SYSMELB_TOKEN_H

#pragma once

/**
 * TokenKind. The different kinds of token used in Sysmel.
 */
typedef enum sysmelb_TokenKind_e
{
#define TokenKindName(name) SysmelToken ##name,
#include "token-kind.inc"
#undef TokenKindName
} sysmelb_TokenKind_t;

const char* sysmelb_TokenKindToString(sysmelb_TokenKind_t kind);

#endif //SYSMELB_TOKEN_H
