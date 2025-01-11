#include "token.h"

static const char* tokenKindNames[] = {
#define TokenKindName(name) #name,
#include "token-kind.inc"
#undef TokenKindName
};

const char* sysmelb_TokenKindToString(sysmelb_TokenKind_t kind)
{
    return tokenKindNames[kind];
}
