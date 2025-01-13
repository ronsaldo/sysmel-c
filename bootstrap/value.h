#ifndef SYSMELB_VALUE_H
#define SYSMELB_VALUE_H

#include "types.h"

typedef enum sysmelb_ValueKind_e {
    SysmelValueKindNull,
    SysmelValueKindTypeReference,
    SysmelValueKindSymbolReference,
    SysmelValueKindStringReference,
    SysmelValueKindCharacter,
    SysmelValueKindInteger,
    SysmelValueKindUnsignedInteger,
    SysmelValueKindFloatingPoint,
} sysmelb_ValueKind_t;

typedef struct sysmelb_Value_s sysmelb_Value_t;

typedef int64_t sysmelb_IntegerLiteralType_t;
typedef uint64_t sysmelb_UnsignedIntegerLiteralType_t;

struct sysmelb_Value_s
{
    sysmelb_ValueKind_t kind;
    sysmelb_Type_t *type;
    union {
        sysmelb_IntegerLiteralType_t integer;
        sysmelb_UnsignedIntegerLiteralType_t unsignedInteger;
        double floatingPoint;

        sysmelb_Type_t *typeReference;
        sysmelb_symbol_t *symbolReference;

        struct
        {
            size_t stringSize;
            const char *string;
        };
    };
};

void sysmelb_printValue(sysmelb_Value_t value);

#endif //SYSMELB_VALUE_H