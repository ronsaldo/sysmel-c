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
    sysmelb_printValueWithMaxDepth(value, 8);
}

void sysmelb_printValueWithMaxDepth(sysmelb_Value_t value, int depth)
{
    --depth;
    if(depth <= 0)
    {
        printf("[...]");
        return;
    }

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
            sysmelb_printValueWithMaxDepth(value.arrayReference->elements[i], depth);
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
                sysmelb_printValueWithMaxDepth(value.tupleReference->elements[i], depth);
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
                sysmelb_printValueWithMaxDepth(value.tupleReference->elements[i], depth);
            }
            printf(")");
        }
        break;
    case SysmelValueKindObjectReference:
        if(value.type->name)
            printf("%.*s", value.type->name->size, value.type->name->string);
        printf("#{");
        for(uint32_t i = 0; i < value.type->clazz.fieldCount; ++i)
        {
            if(i != 0) printf(" ");
            sysmelb_symbol_t *fieldName = value.type->clazz.fieldNames[i];
            printf("%.*s: ", fieldName->size, fieldName->string);
            sysmelb_printValueWithMaxDepth(value.objectReference->elements[value.type->clazz.superFieldCount + i], depth);
            printf(".");
        }
        printf("}");
        break;
    case SysmelValueKindAssociationReference:
        sysmelb_printValueWithMaxDepth(value.associationReference->key, depth);
        printf(" : ");
        sysmelb_printValueWithMaxDepth(value.associationReference->value, depth);
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
                sysmelb_printValueWithMaxDepth(assoc->key, depth);
                printf(" : ");
                sysmelb_printValueWithMaxDepth(assoc->value, depth);
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
        sysmelb_printValueWithMaxDepth(value.valueBoxReference->currentValue, depth);
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
            sysmelb_printValueWithMaxDepth(value.orderedCollectionReference->elements[i], depth);
        }
        printf("]");
        break;
    case SysmelValueKindSumValueReference:
        printf("%.*s[%u:", value.type->name->size, value.type->name->string,
             value.sumTypeValueReference->alternativeIndex);
        sysmelb_printValueWithMaxDepth(value.sumTypeValueReference->alternativeValue, depth);
        printf("]");
        break;
    case SysmelValueKindSymbolHashtableReference:
        printf("SymbolHashtable with: [");
        // TODO: print the elements
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

void *sysmelb_getValuePointer(sysmelb_Value_t value)
{
    switch(value.kind)
    {
    case SysmelValueKindTypeReference:
    case SysmelValueKindFunctionReference:
    case SysmelValueKindParseTreeReference:
    case SysmelValueKindSymbolReference:
    case SysmelValueKindStringReference:
    case SysmelValueKindArrayReference:
    case SysmelValueKindByteArrayReference:
    case SysmelValueKindTupleReference:
    case SysmelValueKindObjectReference:
    case SysmelValueKindAssociationReference:
    case SysmelValueKindImmutableDictionaryReference:
    case SysmelValueKindSymbolHashtableReference:
    case SysmelValueKindValueBoxReference:
    case SysmelValueKindNamespaceReference:
    case SysmelValueKindOrderedCollectionReference:
    case SysmelValueKindSumValueReference:
    case SysmelValueKindIdentityHashsetReference:
        return value.objectReference;
    default:
        return NULL;
    }
}

uint64_t sysmelb_getValueIdentityHash(sysmelb_Value_t value)
{
    uint64_t basePointer = (uintptr_t)sysmelb_getValuePointer(value);
    // LCG. See https://en.wikipedia.org/wiki/Linear_congruential_generator
    // for coefficient choice.
    return basePointer * 6364136223846793005ull + 1;
}