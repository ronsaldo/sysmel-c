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
    case SysmelValueKindArrayReference:
        printf("[");
        for(size_t i = 0; i < value.arrayReference->size; ++i)
        {
            if(i != 0)
                printf(" . ");
            sysmelb_printValue(value.arrayReference->elements[i]);
        }
        printf("]");
        break;
    case SysmelValueKindByteArrayReference:
        printf("#[");
        for(size_t i = 0; i < value.byteArrayReference->size; ++i)
        {
            if(i != 0)
                printf(" . ");
            printf("%d", value.byteArrayReference->elements[i]);
        }
        printf("]");
        break;
    case SysmelValueKindTupleReference:
        printf("(");
        for(size_t i = 0; i < value.tupleReference->size; ++i)
        {
            if(i != 0)
                printf(", ");
            sysmelb_printValue(value.tupleReference->elements[i]);
        }
        printf(")");
        break;
    case SysmelValueKindAssociationReference:
        sysmelb_printValue(value.associationReference->key);
        printf(" : ");
        sysmelb_printValue(value.associationReference->value);
        break;
    case SysmelValueKindDictionaryReference:
        {
            size_t dictionarySize = value.dictionaryReference->size;
            printf("#{");
            for(size_t i = 0; i < dictionarySize; ++i)
            {
                if(i != 0)
                    printf(". ");
                sysmelb_Association_t *assoc = value.dictionaryReference->elements[i];
                sysmelb_printValue(assoc->key);
                printf(" : ");
                sysmelb_printValue(assoc->value);
            }
            printf("}");
        }
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
    case SysmelValueKindValueBoxReference:
        printf("Box[");
        sysmelb_printValue(value.valueBoxReference->currentValue);
        printf("]");
        break;
    default: abort();
    }


}
