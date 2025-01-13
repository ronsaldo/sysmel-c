#ifndef SYSMELB_VALUE_H
#define SYSMELB_VALUE_H

#include "types.h"
#include <stdbool.h>

typedef enum sysmelb_ValueKind_e {
    SysmelValueKindNull,
    SysmelValueKindVoid,
    SysmelValueKindBoolean,
    SysmelValueKindCharacter,
    SysmelValueKindInteger,
    SysmelValueKindUnsignedInteger,
    SysmelValueKindFloatingPoint,

    SysmelValueKindTypeReference,
    SysmelValueKindFunctionReference,
    SysmelValueKindParseTreeReference,
    SysmelValueKindSymbolReference,
    SysmelValueKindStringReference,
} sysmelb_ValueKind_t;

typedef struct sysmelb_Value_s sysmelb_Value_t;
typedef struct sysmelb_function_s sysmelb_function_t;

typedef int64_t sysmelb_IntegerLiteralType_t;
typedef uint64_t sysmelb_UnsignedIntegerLiteralType_t;

typedef struct sysmelb_ParseTreeNode_s sysmelb_ParseTreeNode_t;

struct sysmelb_Value_s
{
    sysmelb_ValueKind_t kind;
    sysmelb_Type_t *type;
    union {
        bool boolean;
        sysmelb_IntegerLiteralType_t integer;
        sysmelb_UnsignedIntegerLiteralType_t unsignedInteger;
        double floatingPoint;

        sysmelb_Type_t *typeReference;
        sysmelb_symbol_t *symbolReference;
        sysmelb_function_t *functionReference;
        sysmelb_ParseTreeNode_t *parseTreeReference;

        struct
        {
            size_t stringSize;
            const char *string;
        };
    };
};

void sysmelb_printValue(sysmelb_Value_t value);

#endif //SYSMELB_VALUE_H