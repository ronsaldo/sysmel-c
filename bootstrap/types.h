#ifndef SYSMEL_TYPES_H
#define SYSMEL_TYPES_H

#pragma once

#include "symbol.h"
#include "function.h"
#include "hashtable.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct sysmelb_Type_s sysmelb_Type_t;
typedef struct sysmelb_ImmutableDictionary_s sysmelb_ImmutableDictionary_t;
typedef struct sysmelb_OrderedCollection_s sysmelb_OrderedCollection_t;

typedef enum sysmelb_TypeKind_e {
    SysmelTypeKindNull,
    SysmelTypeKindGradual,
    SysmelTypeKindUnit,
    SysmelTypeKindVoid,
    SysmelTypeKindBoolean,
    SysmelTypeKindCharacter,
    SysmelTypeKindString,
    SysmelTypeKindSymbol,
    SysmelTypeKindArray,
    SysmelTypeKindAssociation,
    SysmelTypeKindImmutableDictionary,
    SysmelTypeKindSymbolHashtable,
    SysmelTypeKindByteArray,
    SysmelTypeKindTuple,
    SysmelTypeKindRecord,
    SysmelTypeKindSum,
    SysmelTypeKindEnum,
    SysmelTypeKindInteger,
    SysmelTypeKindFloat,
    SysmelTypeKindUniverse,
    SysmelTypeKindSimpleFunction,
    SysmelTypeKindNamespace,
    SysmelTypeKindOrderedCollection,
    SysmelTypeKindParseTreeNode,
    SysmelTypeKindValueReference,
    SysmelTypeKindPrimitiveCharacter,
    SysmelTypeKindPrimitiveSignedInteger,
    SysmelTypeKindPrimitiveUnsignedInteger,
    SysmelTypeKindPrimitiveFloat,
} sysmelb_TypeKind_t;

struct sysmelb_Type_s {
    sysmelb_TypeKind_t kind;
    sysmelb_symbol_t *name;
    const char *printingSuffix;
    sysmelb_Type_t *supertype;
    uint32_t valueSize;
    uint32_t valueAlignment;
    uint32_t heapSize;
    uint32_t heapAlignment;
    sysmelb_SymbolHashtable_t methodDict;
    union
    {
        struct {
            uint32_t fieldCount;
            sysmelb_Type_t **fields;
            sysmelb_symbol_t **fieldNames;
        }tupleAndRecords;

        struct {
            sysmelb_Type_t *baseType;
            uint32_t valueCount;
            sysmelb_Value_t *values;
            sysmelb_symbol_t **valueNames;
        } enumValues;

        struct {
            uint32_t alternativeCount;
            sysmelb_Type_t **alternatives;
        } sumType;
    };
};

typedef struct sysmelb_BasicTypes_s {
    sysmelb_Type_t *null;
    sysmelb_Type_t *gradual;
    sysmelb_Type_t *unit;
    sysmelb_Type_t *voidType;
    sysmelb_Type_t *boolean;
    sysmelb_Type_t *character;
    sysmelb_Type_t *string;
    sysmelb_Type_t *symbol;
    sysmelb_Type_t *integer;
    sysmelb_Type_t *floatingPoint;
    sysmelb_Type_t *universe;

    sysmelb_Type_t *array;
    sysmelb_Type_t *byteArray;
    sysmelb_Type_t *tuple;
    sysmelb_Type_t *record;
    sysmelb_Type_t *enumType;
    sysmelb_Type_t *sum;
    sysmelb_Type_t *association;
    sysmelb_Type_t *immutableDictionary;
    sysmelb_Type_t *symbolHashtable;
    sysmelb_Type_t *parseTreeNode;
    sysmelb_Type_t *valueReference;
    sysmelb_Type_t *function;
    sysmelb_Type_t *namespace;
    sysmelb_Type_t *orderedCollection;

    sysmelb_Type_t *char8;
    sysmelb_Type_t *char16;
    sysmelb_Type_t *char32;
    sysmelb_Type_t *int8;
    sysmelb_Type_t *int16;
    sysmelb_Type_t *int32;
    sysmelb_Type_t *int64;
    sysmelb_Type_t *uint8;
    sysmelb_Type_t *uint16;
    sysmelb_Type_t *uint32;
    sysmelb_Type_t *uint64;

    sysmelb_Type_t *float32;
    sysmelb_Type_t *float64;

} sysmelb_BasicTypes_t;

void sysmelb_type_addPrimitiveMethod(sysmelb_Type_t *type, sysmelb_symbol_t *selector, sysmelb_PrimitiveFunction_t primitive);
void sysmelb_type_addPrimitiveMacroMethod(sysmelb_Type_t *type, sysmelb_symbol_t *selector, sysmelb_PrimitiveMacroFunction_t primitive);

sysmelb_function_t *sysmelb_type_lookupSelector(sysmelb_Type_t *type, sysmelb_symbol_t *selector);

sysmelb_Type_t *sysmelb_allocateValueType(sysmelb_TypeKind_t kind, sysmelb_symbol_t *name, uint32_t size, uint32_t alignment);
sysmelb_Type_t *sysmelb_allocateRecordType(sysmelb_symbol_t *name, sysmelb_ImmutableDictionary_t *fieldsAndTypes);
sysmelb_Type_t *sysmelb_allocateSumType(sysmelb_symbol_t *name, size_t alternativeCount);
sysmelb_Type_t *sysmelb_allocateEnumType(sysmelb_symbol_t *name, sysmelb_Type_t *baseType, sysmelb_ImmutableDictionary_t *namesAndValues);

int sysmelb_findIndexOfFieldNamed(sysmelb_Type_t *type, sysmelb_symbol_t *name);
bool sysmelb_findEnumValueWithName(sysmelb_Type_t *type, sysmelb_symbol_t *name, sysmelb_Value_t *outValue);
int sysmelb_findSumTypeIndexForType(sysmelb_Type_t *sumType, sysmelb_Type_t *injectedType);

sysmelb_Value_t sysmelb_instantiateTypeWithArguments(sysmelb_Type_t *type, size_t argumentCount, sysmelb_Value_t *arguments);
const sysmelb_BasicTypes_t *sysmelb_getBasicTypes(void);

#endif //SYSMEL_TYPES_H