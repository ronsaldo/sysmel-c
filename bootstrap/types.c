#include "types.h"
#include <stdbool.h>

sysmelb_Type_t *sysmelb_allocateValueType(sysmelb_TypeKind_t kind, sysmelb_symbol_t *name, uint32_t size, uint32_t alignment)
{
    sysmelb_Type_t *type = sysmelb_allocate(sizeof(sysmelb_Type_t));
    memset(type, 0, sizeof(sysmelb_Type_t));
    type->kind = kind;
    type->name = name;
    type->valueAlignment = alignment;
    type->valueSize = size;
    return type;
}

bool sysmelb_BasicTypesDataInitialized;
sysmelb_BasicTypes_t sysmelb_BasicTypesData;

const sysmelb_BasicTypes_t *sysmelb_getBasicTypes(void)
{
    if(sysmelb_BasicTypesDataInitialized)
        return &sysmelb_BasicTypesData;

    uint32_t pointerSize = sizeof(void*);
    uint32_t pointerAlignment = sizeof(void*);

    sysmelb_BasicTypesData.null           = sysmelb_allocateValueType(SysmelTypeKindNull, sysmelb_internSymbolC("NullType"), 0, 0);
    sysmelb_BasicTypesData.gradual        = sysmelb_allocateValueType(SysmelTypeKindGradual, sysmelb_internSymbolC("?"), pointerSize, pointerSize);
    sysmelb_BasicTypesData.unit           = sysmelb_allocateValueType(SysmelTypeKindUnit, sysmelb_internSymbolC("Unit"), 0, 0);
    sysmelb_BasicTypesData.character      = sysmelb_allocateValueType(SysmelTypeKindCharacter, sysmelb_internSymbolC("Character"), 4, 4);
    sysmelb_BasicTypesData.string         = sysmelb_allocateValueType(SysmelTypeKindString, sysmelb_internSymbolC("String"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.symbol         = sysmelb_allocateValueType(SysmelTypeKindSymbol, sysmelb_internSymbolC("Symbol"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.integer        = sysmelb_allocateValueType(SysmelTypeKindInteger, sysmelb_internSymbolC("Integer"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.floatingPoint  = sysmelb_allocateValueType(SysmelTypeKindFloat, sysmelb_internSymbolC("Float"), 8, 8);

    sysmelb_BasicTypesData.int8    = sysmelb_allocateValueType(SysmelTypeKindPrimitiveSignedInteger, sysmelb_internSymbolC("Int8"), 1, 1);
    sysmelb_BasicTypesData.int16   = sysmelb_allocateValueType(SysmelTypeKindPrimitiveSignedInteger, sysmelb_internSymbolC("Int16"), 2, 2);
    sysmelb_BasicTypesData.int32   = sysmelb_allocateValueType(SysmelTypeKindPrimitiveSignedInteger, sysmelb_internSymbolC("Int32"), 4, 4);
    sysmelb_BasicTypesData.int64   = sysmelb_allocateValueType(SysmelTypeKindPrimitiveSignedInteger, sysmelb_internSymbolC("Int64"), 8, 8);
    
    sysmelb_BasicTypesData.uint8   = sysmelb_allocateValueType(SysmelTypeKindPrimitiveUnsignedInteger, sysmelb_internSymbolC("UInt8"), 1, 1);
    sysmelb_BasicTypesData.uint16  = sysmelb_allocateValueType(SysmelTypeKindPrimitiveUnsignedInteger, sysmelb_internSymbolC("UInt16"), 2, 2);
    sysmelb_BasicTypesData.uint32  = sysmelb_allocateValueType(SysmelTypeKindPrimitiveUnsignedInteger, sysmelb_internSymbolC("UInt32"), 4, 4);
    sysmelb_BasicTypesData.uint64  = sysmelb_allocateValueType(SysmelTypeKindPrimitiveUnsignedInteger, sysmelb_internSymbolC("UInt64"), 8, 8);

    sysmelb_BasicTypesData.float32 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveFloat, sysmelb_internSymbolC("Float32"), 4, 4);
    sysmelb_BasicTypesData.float64 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveFloat, sysmelb_internSymbolC("Float64"), 8, 8);

    sysmelb_BasicTypesDataInitialized = true;
    return &sysmelb_BasicTypesData;
}