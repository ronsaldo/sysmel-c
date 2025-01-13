#include "value.h"
#include <stdio.h>
void sysmelb_printValue(sysmelb_Value_t value)
{
    switch(value.kind)
    {
    case SysmelValueKindNull:
        printf("null");
        break;
    case SysmelValueKindVoid:
        printf("void");
        break;
    case SysmelValueKindBoolean:
        printf(value.boolean ? "true" : "false");
        break;
    case SysmelValueKindTypeReference:
        if (value.typeReference->name)
            printf("%.*s", value.typeReference->name->size, value.typeReference->name->string);
        else
            printf("AnonTypeReference");
        break;
    case SysmelValueKindSymbolReference:
        printf("#%.*s", value.symbolReference->size, value.symbolReference->string);
        break;
    case SysmelValueKindStringReference:
        printf("\"%.*s\"", (int)value.stringSize, value.string);
        break;
    case SysmelValueKindCharacter:
        printf("%c", (int)value.unsignedInteger);
        break;
    case SysmelValueKindInteger:
        printf("%lld", (long long int)value.integer);
        break;
    case SysmelValueKindUnsignedInteger:
        printf("%llu", (long long unsigned int)value.integer);
        break;
    case SysmelValueKindFloatingPoint:
        printf("%f", value.floatingPoint);
        break;
    default: abort();
    }
}
