#include "types.h"
#include "value.h"
#include <stdbool.h>

void sysmelb_type_addPrimitiveMethod(sysmelb_Type_t *type, sysmelb_symbol_t *selector, sysmelb_PrimitiveFunction_t primitive)
{
    sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
    function->kind = SysmelFunctionKindPrimitive;
    function->name = selector;
    function->primitiveFunction = primitive;

    sysmelb_SymbolHashtable_addSymbolWithValue(&type->methodDict, selector, function);
}

void sysmelb_type_addPrimitiveMacroMethod(sysmelb_Type_t *type, sysmelb_symbol_t *selector, sysmelb_PrimitiveMacroFunction_t primitive)
{
    sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
    function->kind = SysmelFunctionKindPrimitiveMacro;
    function->name = selector;
    function->primitiveMacroFunction = primitive;

    sysmelb_SymbolHashtable_addSymbolWithValue(&type->methodDict, selector, function);
}

sysmelb_function_t *sysmelb_type_lookupSelector(sysmelb_Type_t *type, sysmelb_symbol_t *selector)
{
    const sysmelb_SymbolHashtablePair_t *pair = sysmelb_SymbolHashtable_lookupSymbol(&type->methodDict, selector);
    if(pair)
        return pair->value;
    return NULL;
}

sysmelb_Type_t *sysmelb_allocateValueType(sysmelb_TypeKind_t kind, sysmelb_symbol_t *name, uint32_t size, uint32_t alignment)
{
    sysmelb_Type_t *type = sysmelb_allocate(sizeof(sysmelb_Type_t));
    type->kind = kind;
    type->name = name;
    type->valueAlignment = alignment;
    type->valueSize = size;
    return type;
}

bool sysmelb_BasicTypesDataInitialized;
sysmelb_BasicTypes_t sysmelb_BasicTypesData;

static void sysmelb_createBasicTypes(void)
{
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
    sysmelb_BasicTypesData.universe       = sysmelb_allocateValueType(SysmelTypeKindUniverse, sysmelb_internSymbolC("Type"), 8, 8);

    sysmelb_BasicTypesData.char8    = sysmelb_allocateValueType(SysmelTypeKindPrimitiveCharacter, sysmelb_internSymbolC("Int8"), 1, 1);
    sysmelb_BasicTypesData.char16   = sysmelb_allocateValueType(SysmelTypeKindPrimitiveCharacter, sysmelb_internSymbolC("Int16"), 2, 2);
    sysmelb_BasicTypesData.char32   = sysmelb_allocateValueType(SysmelTypeKindPrimitiveCharacter, sysmelb_internSymbolC("Int32"), 4, 4);

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
}

static sysmelb_IntegerLiteralType_t sysmelb_normalizeIntegerValue(sysmelb_Type_t *integerType, sysmelb_IntegerLiteralType_t value)
{
    if(integerType == sysmelb_BasicTypesData.integer || integerType->valueSize == 8)
        return value;

    switch(integerType->kind)
    {
    case SysmelTypeKindPrimitiveCharacter:
    case SysmelTypeKindPrimitiveUnsignedInteger:
        return value & ((1 << integerType->valueSize*8) - 1);
    case SysmelTypeKindPrimitiveSignedInteger:
        return (value & ((1 << (integerType->valueSize*8 - 1)) - 1))
            - (value & (1 << (integerType->valueSize*8 - 1)));
    case SysmelTypeKindInteger:
    default: return value;
    }
}
static sysmelb_Value_t sysmelb_primitive_negated(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t integerValue = arguments[0];
    integerValue.integer = sysmelb_normalizeIntegerValue(integerValue.type, -integerValue.integer);
    return integerValue;
}

static sysmelb_Value_t sysmelb_primitive_bitInvert(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t integerValue = arguments[0];
    integerValue.integer = sysmelb_normalizeIntegerValue(integerValue.type, ~integerValue.integer);
    return integerValue;
}

static sysmelb_Value_t sysmelb_primitive_plus(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = arguments[0];
    sysmelb_Value_t rightValue = arguments[1];
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer + rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_minus(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = arguments[0];
    sysmelb_Value_t rightValue = arguments[1];
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer - rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_times(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = arguments[0];
    sysmelb_Value_t rightValue = arguments[1];
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer * rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerDivision(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = arguments[0];
    sysmelb_Value_t rightValue = arguments[1];
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer / rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerModule(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = arguments[0];
    sysmelb_Value_t rightValue = arguments[1];
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer % rightValue.integer);
    return result;
}

static void sysmelb_createBasicIntegersPrimitives(void)
{
    sysmelb_Type_t *integerTypes[] = {
        sysmelb_BasicTypesData.integer,

        sysmelb_BasicTypesData.char8, sysmelb_BasicTypesData.char16, sysmelb_BasicTypesData.char32,
        sysmelb_BasicTypesData.int8, sysmelb_BasicTypesData.int16, sysmelb_BasicTypesData.int32, sysmelb_BasicTypesData.int64,
        sysmelb_BasicTypesData.uint8, sysmelb_BasicTypesData.uint16, sysmelb_BasicTypesData.uint32, sysmelb_BasicTypesData.uint64,
    };
    size_t integerTypeCount = sizeof(integerTypes) / sizeof(integerTypes[0]);
    for(size_t i = 0; i < integerTypeCount; ++i)
    {   
        sysmelb_Type_t *type = integerTypes[i];
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("negated"), sysmelb_primitive_negated);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("bitInvert"), sysmelb_primitive_bitInvert);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("+"), sysmelb_primitive_plus);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("-"), sysmelb_primitive_minus);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("*"), sysmelb_primitive_times);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("//"), sysmelb_primitive_integerDivision);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("%"), sysmelb_primitive_integerModule);
    }
}

static void sysmelb_createBasicTypesPrimitives(void)
{
    sysmelb_createBasicIntegersPrimitives();
}

const sysmelb_BasicTypes_t *sysmelb_getBasicTypes(void)
{
    if(sysmelb_BasicTypesDataInitialized)
        return &sysmelb_BasicTypesData;

    sysmelb_createBasicTypes();
    sysmelb_createBasicTypesPrimitives();

    sysmelb_BasicTypesDataInitialized = true;
    return &sysmelb_BasicTypesData;
}