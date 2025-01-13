#ifndef SYSMEL_TYPES_H
#define SYSMEL_TYPES_H

#pragma once

#include "symbol.h"
#include "function.h"
#include "hashtable.h"
#include <stddef.h>
#include <stdint.h>

typedef struct sysmelb_Type_s sysmelb_Type_t;

typedef enum sysmelb_TypeKind_e {
    SysmelTypeKindNull,
    SysmelTypeKindGradual,
    SysmelTypeKindUnit,
    SysmelTypeKindCharacter,
    SysmelTypeKindString,
    SysmelTypeKindSymbol,
    SysmelTypeKindTuple,
    SysmelTypeKindSum,
    SysmelTypeKindInteger,
    SysmelTypeKindFloat,
    SysmelTypeKindUniverse,
    SysmelTypeKindPrimitiveCharacter,
    SysmelTypeKindPrimitiveSignedInteger,
    SysmelTypeKindPrimitiveUnsignedInteger,
    SysmelTypeKindPrimitiveFloat,
} sysmelb_TypeKind_t;

struct sysmelb_Type_s {
    sysmelb_TypeKind_t kind;
    sysmelb_symbol_t *name;
    uint32_t valueSize;
    uint32_t valueAlignment;
    uint32_t heapSize;
    uint32_t heapAlignment;
    uint32_t fieldCount;
    sysmelb_SymbolHashtable_t methodDict;
    sysmelb_Type_t *fields;
};

typedef struct sysmelb_BasicTypes_s {
    sysmelb_Type_t *null;
    sysmelb_Type_t *gradual;
    sysmelb_Type_t *unit;
    sysmelb_Type_t *character;
    sysmelb_Type_t *string;
    sysmelb_Type_t *symbol;
    sysmelb_Type_t *integer;
    sysmelb_Type_t *floatingPoint;
    sysmelb_Type_t *universe;

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
const sysmelb_BasicTypes_t *sysmelb_getBasicTypes(void);

#endif //SYSMEL_TYPES_H