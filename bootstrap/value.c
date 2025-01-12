#include "value.h"
#include <stdio.h>
void sysmelb_printValue(sysmelb_Value_t value)
{
    switch(value.kind)
    {
    case SysmelValueKindNull:
        printf("Null");
        break;
    case SysmelValueKindTypeReference:
        if (value.typeReference->name)
            printf("%.*s", value.typeReference->name->size, value.typeReference->name->string);
        else
            printf("AnonTypeReference");
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
