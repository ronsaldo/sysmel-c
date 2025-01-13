#include "value.h"
#include <stdio.h>
void sysmelb_printValue(sysmelb_Value_t value)
{
    const char *printingSuffix = "";
    if(value.type && value.type->printingSuffix)
        printingSuffix = value.type->printingSuffix;

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
        printf("%c%s", (int)value.unsignedInteger, printingSuffix);
        break;
    case SysmelValueKindInteger:
        printf("%lld%s", (long long int)value.integer, printingSuffix);
        break;
    case SysmelValueKindUnsignedInteger:
        printf("%llu%s", (long long unsigned int)value.integer, printingSuffix);
        break;
    case SysmelValueKindFloatingPoint:
        printf("%f%s", value.floatingPoint, printingSuffix);
        break;
    default: abort();
    }


}
