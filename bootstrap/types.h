#ifndef SYSMEL_TYPES_H
#define SYSMEL_TYPES_H

#pragma once

#include "symbol.h"
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
    sysmelb_Type_t fields[];
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

sysmelb_Type_t *sysmelb_allocateValueType(sysmelb_TypeKind_t kind, sysmelb_symbol_t *name, uint32_t size, uint32_t alignment);
const sysmelb_BasicTypes_t *sysmelb_getBasicTypes(void);

#endif //SYSMEL_TYPES_H