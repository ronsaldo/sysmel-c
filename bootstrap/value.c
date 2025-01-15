#include "value.h"
#include <stdio.h>

sysmelb_Value_t sysmelb_decayValue(sysmelb_Value_t value)
{
    if(value.kind == SysmelValueKindValueBoxReference)
        return value.valueBoxReference->currentValue;
    return value;
}

bool sysmelb_value_equals(sysmelb_Value_t a, sysmelb_Value_t b)
{
    if(a.kind != b.kind)
        return false;

    switch(a.kind)
    {
    case SysmelValueKindNull:
        return true;
    case SysmelValueKindVoid:
        return true;
    case SysmelValueKindBoolean:
        return a.boolean == b.boolean;
    case SysmelValueKindCharacter:
        return a.unsignedInteger == b.unsignedInteger;
    case SysmelValueKindInteger:
        return a.integer == b.integer;
    case SysmelValueKindUnsignedInteger:
        return a.unsignedInteger == b.unsignedInteger;
    case SysmelValueKindFloatingPoint:
        return a.floatingPoint == b.floatingPoint;

    case SysmelValueKindTypeReference:
        return a.typeReference == b.typeReference;
    case SysmelValueKindFunctionReference:
        return a.functionReference == b.functionReference;
    case SysmelValueKindParseTreeReference:
        return a.parseTreeReference == b.parseTreeReference;
    case SysmelValueKindSymbolReference:
        return a.symbolReference == b.symbolReference;
    case SysmelValueKindStringReference:
        return a.string == b.string && a.stringSize == b.stringSize;
    case SysmelValueKindArrayReference:
        return a.arrayReference == b.arrayReference;
    case SysmelValueKindByteArrayReference:
        return a.byteArrayReference == b.byteArrayReference;
    case SysmelValueKindTupleReference:
        return a.tupleReference == b.tupleReference;
    case SysmelValueKindAssociationReference:
        return a.associationReference == b.associationReference;
    case SysmelValueKindDictionaryReference:
        return a.dictionaryReference == b.dictionaryReference;
    case SysmelValueKindValueBoxReference:
        return a.valueBoxReference == b.valueBoxReference;
    default:
        return false;
    }
}

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
        if(value.type->kind == SysmelTypeKindRecord)
        {
            if(value.type->name)
                printf("%.*s", value.type->name->size, value.type->name->string);
            printf("#{");
            for(uint32_t i = 0; i < value.type->tupleAndRecords.fieldCount; ++i)
            {
                if(i != 0) printf(" ");
                sysmelb_symbol_t *fieldName = value.type->tupleAndRecords.fieldNames[i];
                printf("%.*s: ", fieldName->size, fieldName->string);
                sysmelb_printValue(value.tupleReference->elements[i]);
                printf(".");
            }
            printf("}");
        }
        else
        {
            printf("(");
            for(size_t i = 0; i < value.tupleReference->size; ++i)
            {
                if(i != 0)
                    printf(", ");
                sysmelb_printValue(value.tupleReference->elements[i]);
            }
            printf(")");
        }
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
