#include "value.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

sysmelb_Value_t *sysmelb_allocateValue(void)
{
    return sysmelb_allocate(sizeof(sysmelb_Value_t));
}

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
    case SysmelValueKindImmutableDictionaryReference:
        return a.immutableDictionaryReference == b.immutableDictionaryReference;
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
    case SysmelValueKindImmutableDictionaryReference:
        {
            size_t dictionarySize = value.immutableDictionaryReference->size;
            printf("#{");
            for(size_t i = 0; i < dictionarySize; ++i)
            {
                if(i != 0)
                    printf(". ");
                sysmelb_Association_t *assoc = value.immutableDictionaryReference->elements[i];
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
    case SysmelValueKindFunctionReference:
        printf("function(");
        if(value.functionReference->name)
            printf("%.*s", value.functionReference->name->size, value.functionReference->name->string);
        printf(")");
        break;
    case SysmelValueKindOrderedCollectionReference:
        printf("OrderedCollection with: [");
        for(size_t i = 0; i < value.orderedCollectionReference->size; ++i)
        {
            if(i != 0) printf(" . ");
            sysmelb_printValue(value.orderedCollectionReference->elements[i]);
        }
        printf("]");
        break;
    case SysmelValueKindSumValueReference:
        printf("%.*s[%u:", value.type->name->size, value.type->name->string,
             value.sumTypeValueReference->alternativeIndex);
        sysmelb_printValue(value.sumTypeValueReference->alternativeValue);
        printf("]");
        break;
    default: abort();
    }
}

void sysmelb_OrderedCollection_increaseCapacity(sysmelb_OrderedCollection_t *collection)
{
    size_t newCapacity = collection->capacity*2;
    if(newCapacity < 32) newCapacity = 32;

    sysmelb_Value_t *newStorage = sysmelb_allocate(sizeof(sysmelb_Value_t) * newCapacity);
    if(collection->size != 0)
    {
        memcpy(newStorage, collection->elements, collection->size*sizeof(sysmelb_Value_t));
        sysmelb_freeAllocation(collection->elements);
    }

    collection->capacity = newCapacity;
    collection->elements = newStorage;

}

void sysmelb_OrderedCollection_add(sysmelb_OrderedCollection_t *collection, sysmelb_Value_t value)
{
    if(collection->size >= collection->capacity)
        sysmelb_OrderedCollection_increaseCapacity(collection);
    collection->elements[collection->size++] = value;
}
