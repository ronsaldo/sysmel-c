#include "types.h"
#include "error.h"
#include "memory.h"
#include "parse-tree.h"
#include "value.h"
#include "hashtable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool sysmelb_BasicTypesDataInitialized;
static sysmelb_BasicTypes_t sysmelb_BasicTypesData;

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
    if (pair)
        return pair->value;
    if (type->supertype)
        return sysmelb_type_lookupSelector(type->supertype, selector);
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

sysmelb_Type_t *sysmelb_allocateFixedArrayType(sysmelb_Type_t *baseType, uint32_t size)
{
    sysmelb_Type_t *type = sysmelb_allocate(sizeof(sysmelb_Type_t));
    type->kind = SysmelTypeKindRecord;
    type->valueAlignment = baseType->valueAlignment;
    type->valueSize = baseType->valueSize * size;
    type->supertype = sysmelb_getBasicTypes()->fixedArray;
    type->fixedArray.baseType = baseType;
    type->fixedArray.size = size;
    return type;
}

sysmelb_Type_t *sysmelb_allocateRecordType(sysmelb_symbol_t *name, sysmelb_ImmutableDictionary_t *fieldsAndTypes)
{
    sysmelb_Type_t *type = sysmelb_allocate(sizeof(sysmelb_Type_t));
    type->kind = SysmelTypeKindRecord;
    type->name = name;
    type->valueAlignment = 1;
    type->valueSize = 0;
    type->supertype = sysmelb_getBasicTypes()->record;

    size_t fieldCount = fieldsAndTypes->size;
    type->tupleAndRecords.fieldCount = fieldCount;
    type->tupleAndRecords.fields = sysmelb_allocate(sizeof(sysmelb_Type_t *) * fieldCount);
    type->tupleAndRecords.fieldNames = sysmelb_allocate(sizeof(sysmelb_symbol_t *) * fieldCount);

    for (size_t i = 0; i < fieldCount; ++i)
    {
        sysmelb_Association_t *assoc = fieldsAndTypes->elements[i];
        assert(assoc->key.kind == SysmelValueKindSymbolReference);
        assert(assoc->value.kind == SysmelValueKindTypeReference);
        type->tupleAndRecords.fieldNames[i] = assoc->key.symbolReference;
        type->tupleAndRecords.fields[i] = assoc->value.typeReference;
    }
    return type;
}

sysmelb_Type_t *sysmelb_allocateClassType(sysmelb_symbol_t *name, sysmelb_Type_t *superclass, sysmelb_ImmutableDictionary_t *fieldsAndTypes)
{
    sysmelb_Type_t *type = sysmelb_allocate(sizeof(sysmelb_Type_t));
    type->kind = SysmelTypeKindClass;
    type->name = name;
    type->valueAlignment = 1;
    type->valueSize = 0;
    type->supertype = superclass ? superclass : sysmelb_getBasicTypes()->clazz;

    size_t fieldCount = fieldsAndTypes->size;
    type->clazz.superFieldCount = 0;
    if (superclass)
        type->clazz.superFieldCount = superclass->clazz.superFieldCount + superclass->clazz.fieldCount;

    type->clazz.fieldCount = fieldCount;
    type->clazz.fields = sysmelb_allocate(sizeof(sysmelb_Type_t *) * fieldCount);
    type->clazz.fieldNames = sysmelb_allocate(sizeof(sysmelb_symbol_t *) * fieldCount);

    for (size_t i = 0; i < fieldCount; ++i)
    {
        sysmelb_Association_t *assoc = fieldsAndTypes->elements[i];
        assert(assoc->key.kind == SysmelValueKindSymbolReference);
        assert(assoc->value.kind == SysmelValueKindTypeReference);
        type->clazz.fieldNames[i] = assoc->key.symbolReference;
        type->clazz.fields[i] = assoc->value.typeReference;
    }
    return type;
}

sysmelb_Type_t *sysmelb_allocateSumType(sysmelb_symbol_t *name, size_t alternativeCount)
{
    sysmelb_Type_t *type = sysmelb_allocate(sizeof(sysmelb_Type_t));
    type->kind = SysmelTypeKindSum;
    type->name = name;
    type->valueAlignment = 1;
    type->valueSize = 1;
    type->supertype = sysmelb_getBasicTypes()->sum;
    type->sumType.alternativeCount = alternativeCount;
    type->sumType.alternatives = sysmelb_allocate(sizeof(sysmelb_Type_t *) * alternativeCount);
    return type;
}

sysmelb_Type_t *sysmelb_allocateEnumType(sysmelb_symbol_t *name, sysmelb_Type_t *baseType, sysmelb_ImmutableDictionary_t *namesAndValues)
{
    sysmelb_Type_t *type = sysmelb_allocate(sizeof(sysmelb_Type_t));
    type->kind = SysmelTypeKindEnum;
    type->name = name;
    type->valueAlignment = 1;
    type->valueSize = 0;
    type->supertype = sysmelb_getBasicTypes()->record;

    size_t valueCount = namesAndValues->size;
    type->enumValues.baseType = baseType;
    type->enumValues.valueCount = valueCount;
    type->enumValues.values = sysmelb_allocate(sizeof(sysmelb_Value_t) * valueCount);
    type->enumValues.valueNames = sysmelb_allocate(sizeof(sysmelb_symbol_t *) * valueCount);

    sysmelb_Value_t lastValue = {
        .kind = SysmelValueKindInteger,
        .type = baseType};

    for (size_t i = 0; i < valueCount; ++i)
    {
        sysmelb_Association_t *assoc = namesAndValues->elements[i];
        assert(assoc->key.kind == SysmelValueKindSymbolReference);
        if (assoc->value.kind == SysmelValueKindNull && i != 0)
        {
            ++lastValue.integer;
        }
        else
        {
            lastValue = assoc->value;
        }

        type->enumValues.valueNames[i] = assoc->key.symbolReference;
        type->enumValues.values[i] = lastValue;
    }
    return type;
}

bool sysmelb_findEnumValueWithName(sysmelb_Type_t *type, sysmelb_symbol_t *name, sysmelb_Value_t *outValue)
{
    if (!type->enumValues.valueNames || !type->enumValues.values)
        return false;

    for (uint32_t i = 0; i < type->enumValues.valueCount; ++i)
    {
        if (type->enumValues.valueNames[i] == name)
        {
            *outValue = type->enumValues.values[i];
            return true;
        }
    }

    return false;
}

int sysmelb_findIndexOfFieldNamed(sysmelb_Type_t *type, sysmelb_symbol_t *name)
{
    if (!type->tupleAndRecords.fields || !type->tupleAndRecords.fieldNames)
        return -1;

    for (uint32_t i = 0; i < type->tupleAndRecords.fieldCount; ++i)
    {
        if (type->tupleAndRecords.fieldNames[i] == name)
            return (int)i;
    }

    return -1;
}

int sysmelb_findIndexOfFieldNamedInClass(sysmelb_Type_t *type, sysmelb_symbol_t *name)
{
    if (!type->clazz.fields || !type->clazz.fieldNames || type->kind != SysmelTypeKindClass)
        return -1;

    for (uint32_t i = 0; i < type->clazz.fieldCount; ++i)
    {
        if (type->clazz.fieldNames[i] == name)
            return (int)(i + type->clazz.superFieldCount);
    }
    if (type->supertype)
        return sysmelb_findIndexOfFieldNamedInClass(type->supertype, name);

    return -1;
}

int sysmelb_findSumTypeIndexForType(sysmelb_Type_t *sumType, sysmelb_Type_t *injectedType)
{
    for (uint32_t i = 0; i < sumType->sumType.alternativeCount; ++i)
    {
        if (sumType->sumType.alternatives[i] == injectedType)
            return (int)i;
    }
    return -1;
}

sysmelb_Value_t sysmelb_instantiateTypeWithArguments(sysmelb_Type_t *type, size_t argumentCount, sysmelb_Value_t *arguments)
{
    if (type->kind == SysmelTypeKindRecord || type->kind == SysmelTypeKindTuple)
    {
        assert(argumentCount <= type->tupleAndRecords.fieldCount);
        sysmelb_TupleHeader_t *tupleOrRecord = sysmelb_allocate(sizeof(sysmelb_TupleHeader_t) + sizeof(sysmelb_Value_t) * type->tupleAndRecords.fieldCount);
        tupleOrRecord->size = type->tupleAndRecords.fieldCount;

        // Prefill with null values.
        sysmelb_Value_t nullValue = {
            .kind = SysmelValueKindNull,
            .type = sysmelb_getBasicTypes()->null};
        for (size_t i = 0; i < tupleOrRecord->size; ++i)
            tupleOrRecord->elements[i] = nullValue;

        if (argumentCount == 1 && arguments[0].kind == SysmelValueKindImmutableDictionaryReference)
        {
            sysmelb_ImmutableDictionary_t *dict = arguments[0].immutableDictionaryReference;
            for (size_t i = 0; i < dict->size; ++i)
            {
                sysmelb_Association_t *assoc = dict->elements[i];
                assert(assoc->key.kind == SysmelValueKindSymbolReference);
                sysmelb_symbol_t *fieldName = assoc->key.symbolReference;
                int fieldIndex = sysmelb_findIndexOfFieldNamed(type, assoc->key.symbolReference);
                if (fieldIndex < 0)
                {
                    sysmelb_SourcePosition_t nullPosition = {};
                    sysmelb_errorPrintf(nullPosition, "Failed to find field %.*s in record.", fieldName->size, fieldName->string);
                    abort();
                }

                tupleOrRecord->elements[fieldIndex] = assoc->value;
            }
        }
        else
        {
            for (size_t i = 0; i < argumentCount; ++i)
                tupleOrRecord->elements[i] = arguments[i];
        }

        sysmelb_Value_t result = {
            .kind = SysmelValueKindTupleReference,
            .type = type,
            .tupleReference = tupleOrRecord,
        };

        return result;
    }
    if (type->kind == SysmelTypeKindClass)
    {
        size_t totalFieldCount = type->clazz.superFieldCount + type->clazz.fieldCount;
        assert(argumentCount <= totalFieldCount);
        sysmelb_ObjectHeader_t *object = sysmelb_allocate(sizeof(sysmelb_ObjectHeader_t) + sizeof(sysmelb_Value_t) * totalFieldCount);
        object->size = totalFieldCount;
        object->clazz = type;

        // Prefill with null values.
        sysmelb_Value_t nullValue = {
            .kind = SysmelValueKindNull,
            .type = sysmelb_getBasicTypes()->null};
        for (size_t i = 0; i < object->size; ++i)
            object->elements[i] = nullValue;

        if (argumentCount == 1 && arguments[0].kind == SysmelValueKindImmutableDictionaryReference)
        {
            sysmelb_ImmutableDictionary_t *dict = arguments[0].immutableDictionaryReference;
            for (size_t i = 0; i < dict->size; ++i)
            {
                sysmelb_Association_t *assoc = dict->elements[i];
                assert(assoc->key.kind == SysmelValueKindSymbolReference);
                sysmelb_symbol_t *fieldName = assoc->key.symbolReference;
                int fieldIndex = sysmelb_findIndexOfFieldNamedInClass(type, assoc->key.symbolReference);
                if (fieldIndex < 0)
                {
                    sysmelb_SourcePosition_t nullPosition = {};
                    sysmelb_errorPrintf(nullPosition, "Failed to find field %.*s in class.", fieldName->size, fieldName->string);
                    abort();
                }

                object->elements[fieldIndex] = assoc->value;
            }
        }
        else
        {
            for (size_t i = 0; i < argumentCount; ++i)
                object->elements[i] = arguments[i];
        }

        sysmelb_Value_t result = {
            .kind = SysmelValueKindObjectReference,
            .type = type,
            .objectReference = object,
        };

        return result;
    }
    if (type->kind == SysmelTypeKindOrderedCollection)
    {
        sysmelb_OrderedCollection_t *collection = sysmelb_allocate(sizeof(sysmelb_OrderedCollection_t));
        sysmelb_Value_t result = {
            .kind = SysmelValueKindOrderedCollectionReference,
            .type = sysmelb_getBasicTypes()->orderedCollection,
            .orderedCollectionReference = collection};
        return result;
    }
    if (type->kind == SysmelTypeKindByteOrderedCollection)
    {
        sysmelb_ByteOrderedCollection_t *collection = sysmelb_allocate(sizeof(sysmelb_ByteOrderedCollection_t));
        sysmelb_Value_t result = {
            .kind = SysmelValueKindByteOrderedCollectionReference,
            .type = sysmelb_getBasicTypes()->byteOrderedCollection,
            .byteOrderedCollectionReference = collection};
        return result;
    }
    if (type->kind == SysmelTypeKindSymbolHashtable)
    {
        sysmelb_SymbolHashtable_t *table = sysmelb_allocate(sizeof(sysmelb_SymbolHashtable_t));
        sysmelb_Value_t result = {
            .kind = SysmelValueKindSymbolHashtableReference,
            .type = sysmelb_getBasicTypes()->symbolHashtable,
            .symbolHashtableReference = table};
        return result;
    }
    if (type->kind == SysmelTypeKindIdentityHashset)
    {
        sysmelb_IdentityHashset_t *set = sysmelb_allocate(sizeof(sysmelb_IdentityHashset_t));
        sysmelb_Value_t result = {
            .kind = SysmelValueKindIdentityHashsetReference,
            .type = sysmelb_getBasicTypes()->identityHashset,
            .identityHashsetReference = set};
        return result;
    }
    if (type->kind == SysmelTypeKindIdentityDictionary)
    {
        sysmelb_IdentityDictionary_t *dict = sysmelb_allocate(sizeof(sysmelb_IdentityDictionary_t));
        sysmelb_Value_t result = {
            .kind = SysmelValueKindIdentityDictionaryReference,
            .type = sysmelb_getBasicTypes()->identityDictionary,
            .identityDictionaryReference = dict};
        return result;
    }
    if (type->kind == SysmelTypeKindSum)
    {
        if (argumentCount != 1)
        {
            sysmelb_SourcePosition_t emptyPosition = {0};
            sysmelb_errorPrintf(emptyPosition, "Sum types can only be instantiated with a single parameter.");
        }
        sysmelb_Type_t *argumentType = arguments[0].type;
        int injectionIndex = sysmelb_findSumTypeIndexForType(type, argumentType);
        if (injectionIndex < 0)
        {
            sysmelb_SourcePosition_t emptyPosition = {0};
            sysmelb_errorPrintf(emptyPosition, "Failed to inject value of a type into a sum type.");
            abort();
        }

        sysmelb_SumTypeValue_t *sumValue = sysmelb_allocate(sizeof(sysmelb_SumTypeValue_t));
        sumValue->alternativeIndex = injectionIndex;
        sumValue->alternativeValue = arguments[0];

        sysmelb_Value_t sumValueValue = {
            .kind = SysmelValueKindSumValueReference,
            .type = type,
            .sumTypeValueReference = sumValue};
        return sumValueValue;
    }

    abort();
}

static void sysmelb_createBasicTypes(void)
{
    uint32_t pointerSize = sizeof(void *);
    uint32_t pointerAlignment = sizeof(void *);

    sysmelb_BasicTypesData.null = sysmelb_allocateValueType(SysmelTypeKindNull, sysmelb_internSymbolC("NullType"), 0, 0);
    sysmelb_BasicTypesData.gradual = sysmelb_allocateValueType(SysmelTypeKindGradual, sysmelb_internSymbolC("Gradual"), pointerSize, pointerSize);
    sysmelb_BasicTypesData.unit = sysmelb_allocateValueType(SysmelTypeKindUnit, sysmelb_internSymbolC("Unit"), 0, 0);
    sysmelb_BasicTypesData.voidType = sysmelb_allocateValueType(SysmelTypeKindVoid, sysmelb_internSymbolC("Void"), 0, 0);
    sysmelb_BasicTypesData.boolean = sysmelb_allocateValueType(SysmelTypeKindBoolean, sysmelb_internSymbolC("Boolean"), 1, 1);
    sysmelb_BasicTypesData.character = sysmelb_allocateValueType(SysmelTypeKindCharacter, sysmelb_internSymbolC("Character"), 4, 4);
    sysmelb_BasicTypesData.string = sysmelb_allocateValueType(SysmelTypeKindString, sysmelb_internSymbolC("String"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.symbol = sysmelb_allocateValueType(SysmelTypeKindSymbol, sysmelb_internSymbolC("Symbol"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.integer = sysmelb_allocateValueType(SysmelTypeKindInteger, sysmelb_internSymbolC("Integer"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.floatingPoint = sysmelb_allocateValueType(SysmelTypeKindFloat, sysmelb_internSymbolC("Float"), 8, 8);
    sysmelb_BasicTypesData.universe = sysmelb_allocateValueType(SysmelTypeKindUniverse, sysmelb_internSymbolC("Type"), pointerSize, pointerAlignment);

    sysmelb_BasicTypesData.array = sysmelb_allocateValueType(SysmelTypeKindArray, sysmelb_internSymbolC("Array"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.byteArray = sysmelb_allocateValueType(SysmelTypeKindByteArray, sysmelb_internSymbolC("ByteArray"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.fixedArray = sysmelb_allocateValueType(SysmelTypeKindByteArray, sysmelb_internSymbolC("FixedSizeArray"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.tuple = sysmelb_allocateValueType(SysmelTypeKindTuple, sysmelb_internSymbolC("Tuple"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.record = sysmelb_allocateValueType(SysmelTypeKindRecord, sysmelb_internSymbolC("Record"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.record->supertype = sysmelb_BasicTypesData.tuple;
    sysmelb_BasicTypesData.clazz = sysmelb_allocateValueType(SysmelTypeKindClass, sysmelb_internSymbolC("Class"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.sum = sysmelb_allocateValueType(SysmelTypeKindSum, sysmelb_internSymbolC("Sum"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.enumType = sysmelb_allocateValueType(SysmelTypeKindEnum, sysmelb_internSymbolC("Enum"), 4, 4);
    sysmelb_BasicTypesData.association = sysmelb_allocateValueType(SysmelTypeKindAssociation, sysmelb_internSymbolC("Association"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.immutableDictionary = sysmelb_allocateValueType(SysmelTypeKindImmutableDictionary, sysmelb_internSymbolC("ImmutableDictionary"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.symbolHashtable = sysmelb_allocateValueType(SysmelTypeKindSymbolHashtable, sysmelb_internSymbolC("SymbolHashtable"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.parseTreeNode = sysmelb_allocateValueType(SysmelTypeKindParseTreeNode, sysmelb_internSymbolC("ParseTreeNode"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.valueReference = sysmelb_allocateValueType(SysmelTypeKindValueReference, sysmelb_internSymbolC("ValueReference"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.function = sysmelb_allocateValueType(SysmelTypeKindSimpleFunction, sysmelb_internSymbolC("Function"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.namespace = sysmelb_allocateValueType(SysmelTypeKindNamespace, sysmelb_internSymbolC("Namespace"), pointerSize, pointerAlignment);

    sysmelb_BasicTypesData.orderedCollection = sysmelb_allocateValueType(SysmelTypeKindOrderedCollection, sysmelb_internSymbolC("OrderedCollection"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.byteOrderedCollection = sysmelb_allocateValueType(SysmelTypeKindByteOrderedCollection, sysmelb_internSymbolC("ByteOrderedCollection"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.identityHashset = sysmelb_allocateValueType(SysmelTypeKindIdentityHashset, sysmelb_internSymbolC("IdentityHashset"), pointerSize, pointerAlignment);
    sysmelb_BasicTypesData.identityDictionary = sysmelb_allocateValueType(SysmelTypeKindIdentityDictionary, sysmelb_internSymbolC("IdentityDictionary"), pointerSize, pointerAlignment);

    sysmelb_BasicTypesData.char8 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveCharacter, sysmelb_internSymbolC("Char8"), 1, 1);
    sysmelb_BasicTypesData.char16 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveCharacter, sysmelb_internSymbolC("Char16"), 2, 2);
    sysmelb_BasicTypesData.char32 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveCharacter, sysmelb_internSymbolC("Char32"), 4, 4);

    sysmelb_BasicTypesData.int8 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveSignedInteger, sysmelb_internSymbolC("Int8"), 1, 1);
    sysmelb_BasicTypesData.int16 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveSignedInteger, sysmelb_internSymbolC("Int16"), 2, 2);
    sysmelb_BasicTypesData.int32 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveSignedInteger, sysmelb_internSymbolC("Int32"), 4, 4);
    sysmelb_BasicTypesData.int64 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveSignedInteger, sysmelb_internSymbolC("Int64"), 8, 8);

    sysmelb_BasicTypesData.uint8 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveUnsignedInteger, sysmelb_internSymbolC("UInt8"), 1, 1);
    sysmelb_BasicTypesData.uint16 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveUnsignedInteger, sysmelb_internSymbolC("UInt16"), 2, 2);
    sysmelb_BasicTypesData.uint32 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveUnsignedInteger, sysmelb_internSymbolC("UInt32"), 4, 4);
    sysmelb_BasicTypesData.uint64 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveUnsignedInteger, sysmelb_internSymbolC("UInt64"), 8, 8);

    sysmelb_BasicTypesData.float32 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveFloat, sysmelb_internSymbolC("Float32"), 4, 4);
    sysmelb_BasicTypesData.float64 = sysmelb_allocateValueType(SysmelTypeKindPrimitiveFloat, sysmelb_internSymbolC("Float64"), 8, 8);

    sysmelb_BasicTypesData.char8->printingSuffix = "c8";
    sysmelb_BasicTypesData.char16->printingSuffix = "c16";
    sysmelb_BasicTypesData.char32->printingSuffix = "c32";

    sysmelb_BasicTypesData.int8->printingSuffix = "i8";
    sysmelb_BasicTypesData.int16->printingSuffix = "i16";
    sysmelb_BasicTypesData.int32->printingSuffix = "i32";
    sysmelb_BasicTypesData.int64->printingSuffix = "i64";

    sysmelb_BasicTypesData.uint8->printingSuffix = "u8";
    sysmelb_BasicTypesData.uint16->printingSuffix = "u16";
    sysmelb_BasicTypesData.uint32->printingSuffix = "u32";
    sysmelb_BasicTypesData.uint64->printingSuffix = "u64";

    sysmelb_BasicTypesData.float32->printingSuffix = "f32";
    sysmelb_BasicTypesData.float64->printingSuffix = "f64";
}

static sysmelb_IntegerLiteralType_t sysmelb_normalizeIntegerValue(sysmelb_Type_t *integerType, sysmelb_IntegerLiteralType_t value)
{
    if (integerType == sysmelb_BasicTypesData.integer || integerType->valueSize == 8)
        return value;

    sysmelb_UnsignedIntegerLiteralType_t integerBitCount = integerType->valueSize * 8;
    sysmelb_UnsignedIntegerLiteralType_t integerBitMask = ((sysmelb_UnsignedIntegerLiteralType_t)1 << integerBitCount) - 1;

    sysmelb_IntegerLiteralType_t integerSignedBitMask = (1 << (integerBitCount - 1)) - 1;
    sysmelb_IntegerLiteralType_t integerSignBitMask = (1 << (integerBitCount - 1));

    switch (integerType->kind)
    {
    case SysmelTypeKindPrimitiveCharacter:
    case SysmelTypeKindPrimitiveUnsignedInteger:
        return value & integerBitMask;
    case SysmelTypeKindPrimitiveSignedInteger:
        return (value & integerSignedBitMask) - (value & integerSignBitMask);
    case SysmelTypeKindInteger:
    default:
        return value;
    }
}
static sysmelb_Value_t sysmelb_primitive_negated(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t integerValue = sysmelb_decayValue(arguments[0]);
    integerValue.integer = sysmelb_normalizeIntegerValue(integerValue.type, -integerValue.integer);
    return integerValue;
}

static sysmelb_Value_t sysmelb_primitive_bitInvert(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t integerValue = sysmelb_decayValue(arguments[0]);
    integerValue.integer = sysmelb_normalizeIntegerValue(integerValue.type, ~integerValue.integer);
    return integerValue;
}

static sysmelb_Value_t sysmelb_primitive_plus(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer + rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_minus(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer - rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_times(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer * rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerDivision(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer / rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerModule(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer % rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerBitAnd(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer & rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerBitOr(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer | rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerBitXor(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer ^ rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerBitShiftLeft(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer << rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerBitShiftRight(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.unsignedInteger >> rightValue.unsignedInteger);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerBitArithmeicShiftRight(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = leftValue;
    result.integer = sysmelb_normalizeIntegerValue(result.type, leftValue.integer >> rightValue.integer);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = leftValue.integer == rightValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerNotEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = leftValue.integer != rightValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerLessThan(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = (leftValue.kind == SysmelValueKindUnsignedInteger)
                       ? leftValue.unsignedInteger < rightValue.unsignedInteger
                       : leftValue.integer < rightValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerLessOrEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = (leftValue.kind == SysmelValueKindUnsignedInteger)
                       ? leftValue.unsignedInteger <= rightValue.unsignedInteger
                       : leftValue.integer <= rightValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerGreaterThan(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = (leftValue.kind == SysmelValueKindUnsignedInteger)
                       ? leftValue.unsignedInteger > rightValue.unsignedInteger
                       : leftValue.integer > rightValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerGreaterOrEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_Value_t leftValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t rightValue = sysmelb_decayValue(arguments[1]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = (leftValue.kind == SysmelValueKindUnsignedInteger)
                       ? leftValue.unsignedInteger >= rightValue.unsignedInteger
                       : leftValue.integer >= rightValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsInteger(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindInteger,
        .type = sysmelb_BasicTypesData.integer,
        .integer = originalValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsCharacter(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindInteger,
        .type = sysmelb_BasicTypesData.character,
        .integer = originalValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsInt8(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindInteger,
        .type = sysmelb_BasicTypesData.int8,
        .integer = (int8_t)originalValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsInt16(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindInteger,
        .type = sysmelb_BasicTypesData.int16,
        .integer = (int16_t)originalValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsInt32(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindInteger,
        .type = sysmelb_BasicTypesData.int32,
        .integer = (int32_t)originalValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsInt64(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindInteger,
        .type = sysmelb_BasicTypesData.int64,
        .integer = (int64_t)originalValue.integer};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsUInt8(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_BasicTypesData.uint8,
        .unsignedInteger = (uint8_t)originalValue.unsignedInteger};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsUInt16(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_BasicTypesData.uint16,
        .unsignedInteger = (uint16_t)originalValue.unsignedInteger};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsUInt32(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_BasicTypesData.uint32,
        .unsignedInteger = (uint32_t)originalValue.unsignedInteger};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsUInt64(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_BasicTypesData.uint64,
        .unsignedInteger = (uint64_t)originalValue.unsignedInteger};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsFloat32(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindFloatingPoint,
        .type = sysmelb_BasicTypesData.float32,
        .floatingPoint = (originalValue.kind == SysmelValueKindUnsignedInteger)
                             ? (float)originalValue.unsignedInteger
                             : (float)originalValue.integer,
    };
    return result;
}

static sysmelb_Value_t sysmelb_primitive_integerAsFloat64(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t originalValue = sysmelb_decayValue(arguments[0]);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindFloatingPoint,
        .type = sysmelb_BasicTypesData.float64,
        .floatingPoint = (originalValue.kind == SysmelValueKindUnsignedInteger)
                             ? (double)originalValue.unsignedInteger
                             : (double)originalValue.integer,
    };
    return result;
}

static void sysmelb_createBasicIntegersPrimitives(void)
{
    sysmelb_Type_t *integerTypes[] = {
        sysmelb_BasicTypesData.integer,
        sysmelb_BasicTypesData.character,

        sysmelb_BasicTypesData.char8,
        sysmelb_BasicTypesData.char16,
        sysmelb_BasicTypesData.char32,
        sysmelb_BasicTypesData.int8,
        sysmelb_BasicTypesData.int16,
        sysmelb_BasicTypesData.int32,
        sysmelb_BasicTypesData.int64,
        sysmelb_BasicTypesData.uint8,
        sysmelb_BasicTypesData.uint16,
        sysmelb_BasicTypesData.uint32,
        sysmelb_BasicTypesData.uint64,
    };
    size_t integerTypeCount = sizeof(integerTypes) / sizeof(integerTypes[0]);
    for (size_t i = 0; i < integerTypeCount; ++i)
    {
        sysmelb_Type_t *type = integerTypes[i];
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("negated"), sysmelb_primitive_negated);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("bitInvert"), sysmelb_primitive_bitInvert);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("+"), sysmelb_primitive_plus);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("-"), sysmelb_primitive_minus);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("*"), sysmelb_primitive_times);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("//"), sysmelb_primitive_integerDivision);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("%"), sysmelb_primitive_integerModule);

        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("&"), sysmelb_primitive_integerBitAnd);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("|"), sysmelb_primitive_integerBitOr);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("^"), sysmelb_primitive_integerBitXor);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("<<"), sysmelb_primitive_integerBitShiftLeft);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC(">>"), sysmelb_primitive_integerBitShiftRight);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC(">>>"), sysmelb_primitive_integerBitArithmeicShiftRight);

        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("="), sysmelb_primitive_integerEquals);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("~="), sysmelb_primitive_integerNotEquals);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("=="), sysmelb_primitive_integerEquals);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("~~"), sysmelb_primitive_integerNotEquals);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("<"), sysmelb_primitive_integerLessThan);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("<="), sysmelb_primitive_integerLessOrEquals);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC(">"), sysmelb_primitive_integerGreaterThan);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC(">="), sysmelb_primitive_integerGreaterOrEquals);

        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asInteger"), sysmelb_primitive_integerAsInteger);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asCharacter"), sysmelb_primitive_integerAsCharacter);

        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asInt8"), sysmelb_primitive_integerAsInt8);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asInt16"), sysmelb_primitive_integerAsInt16);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asInt32"), sysmelb_primitive_integerAsInt32);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asInt64"), sysmelb_primitive_integerAsInt64);

        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asUInt8"), sysmelb_primitive_integerAsUInt8);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asUInt16"), sysmelb_primitive_integerAsUInt16);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asUInt32"), sysmelb_primitive_integerAsUInt32);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asUInt64"), sysmelb_primitive_integerAsUInt64);

        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asFloat32"), sysmelb_primitive_integerAsFloat32);
        sysmelb_type_addPrimitiveMethod(type, sysmelb_internSymbolC("asFloat64"), sysmelb_primitive_integerAsFloat64);
    }

    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("i8"), sysmelb_primitive_integerAsInt8);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("i16"), sysmelb_primitive_integerAsInt16);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("i32"), sysmelb_primitive_integerAsInt32);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("i64"), sysmelb_primitive_integerAsInt64);

    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("u8"), sysmelb_primitive_integerAsUInt8);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("u16"), sysmelb_primitive_integerAsUInt16);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("u32"), sysmelb_primitive_integerAsUInt32);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("u64"), sysmelb_primitive_integerAsUInt64);

    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("f32"), sysmelb_primitive_integerAsFloat32);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.integer, sysmelb_internSymbolC("f64"), sysmelb_primitive_integerAsFloat64);

    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.character, sysmelb_internSymbolC("i8"), sysmelb_primitive_integerAsInt8);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.character, sysmelb_internSymbolC("i16"), sysmelb_primitive_integerAsInt16);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.character, sysmelb_internSymbolC("i32"), sysmelb_primitive_integerAsInt32);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.character, sysmelb_internSymbolC("i64"), sysmelb_primitive_integerAsInt64);

    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.character, sysmelb_internSymbolC("u8"), sysmelb_primitive_integerAsUInt8);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.character, sysmelb_internSymbolC("u16"), sysmelb_primitive_integerAsUInt16);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.character, sysmelb_internSymbolC("u32"), sysmelb_primitive_integerAsUInt32);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.character, sysmelb_internSymbolC("u64"), sysmelb_primitive_integerAsUInt64);
}

static sysmelb_Value_t sysmelb_primitive_stringEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindStringReference && arguments[1].kind == SysmelValueKindStringReference);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
    };

    if (arguments[0].stringSize == arguments[1].stringSize && memcmp(arguments[0].string, arguments[1].string, arguments[1].stringSize) == 0)
    {
        result.boolean = true;
    }
    else
    {
        result.boolean = false;
    }

    return result;
}

static sysmelb_Value_t sysmelb_primitive_stringNotEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindStringReference && arguments[1].kind == SysmelValueKindStringReference);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
    };

    if (arguments[0].stringSize == arguments[1].stringSize && memcmp(arguments[0].string, arguments[1].string, arguments[1].stringSize) == 0)
    {
        result.boolean = false;
    }
    else
    {
        result.boolean = true;
    }

    return result;
}

static sysmelb_Value_t sysmelb_primitive_concatenateString(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindStringReference && arguments[1].kind == SysmelValueKindStringReference);

    size_t stringSize = arguments[0].stringSize + arguments[1].stringSize;
    char *stringData = sysmelb_allocate(stringSize);
    memcpy(stringData, arguments[0].string, arguments[0].stringSize);
    memcpy(stringData + arguments[0].stringSize, arguments[1].string, arguments[1].stringSize);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindStringReference,
        .type = sysmelb_getBasicTypes()->string,
        .string = stringData,
        .stringSize = stringSize};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_stringSize(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindStringReference);

    size_t stringSize = arguments[0].stringSize;
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_BasicTypesData.integer,
        .unsignedInteger = stringSize};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_stringAt(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindStringReference && (arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger));

    size_t stringSize = arguments[0].stringSize;
    unsigned int stringIndex = arguments[1].unsignedInteger;
    assert(stringIndex < stringSize);

    char element = arguments[0].string[stringIndex];
    sysmelb_Value_t result = {
        .kind = SysmelValueKindCharacter,
        .type = sysmelb_getBasicTypes()->character,
        .unsignedInteger = element};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_stringAtPut(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    assert(arguments[0].kind == SysmelValueKindStringReference
        && (arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger)
        && (arguments[2].kind == SysmelValueKindInteger || arguments[2].kind == SysmelValueKindUnsignedInteger));

    size_t stringSize = arguments[0].stringSize;
    unsigned int stringIndex = arguments[1].unsignedInteger;
    assert(stringIndex < stringSize);

    arguments[0].string[stringIndex] = (char)arguments[2].integer;

    return arguments[2];
}

static sysmelb_Value_t sysmelb_primitive_substringFromUntil(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    assert(arguments[0].kind == SysmelValueKindStringReference && (arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger) && (arguments[2].kind == SysmelValueKindInteger || arguments[2].kind == SysmelValueKindUnsignedInteger));

    size_t stringSize = arguments[0].stringSize;
    unsigned int startIndex = arguments[1].unsignedInteger;
    unsigned int endIndex = arguments[2].unsignedInteger;
    assert(startIndex < stringSize);
    assert(endIndex <= stringSize);

    unsigned int substringSize = endIndex - startIndex;
    char *substring = sysmelb_allocate(substringSize);
    memcpy(substring, arguments[0].string + startIndex, substringSize);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindStringReference,
        .type = sysmelb_getBasicTypes()->string,
        .string = substring,
        .stringSize = substringSize,
    };
    return result;
}

static sysmelb_Value_t sysmelb_primitive_stringAsFloat(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindStringReference);

    size_t stringSize = arguments[0].stringSize;
    char *cstring = calloc(stringSize, 1);
    memcpy(cstring, arguments[0].string, stringSize);
    double floatValue = atof(cstring);
    free(cstring);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindFloatingPoint,
        .type = sysmelb_BasicTypesData.integer,
        .floatingPoint = floatValue};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_stringAsSymbol(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindStringReference);

    sysmelb_symbol_t *internedString = sysmelb_internSymbol(arguments[0].stringSize, arguments[0].string);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindSymbolReference,
        .type = sysmelb_BasicTypesData.symbol,
        .symbolReference = internedString};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_parseCEscapeSequences(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindStringReference);

    size_t stringSize = arguments[0].stringSize;
    char *parsedString = sysmelb_allocate(stringSize);
    size_t parsedStringSize = 0;

    for (size_t i = 0; i < stringSize; ++i)
    {
        char c = arguments[0].string[i];
        if (c == '\\' && i + 1 < stringSize)
        {
            char escape = arguments[0].string[++i];
            char resultingChar = escape;
            switch (escape)
            {
            case 'n':
                resultingChar = '\n';
                break;
            case 'r':
                resultingChar = '\r';
                break;
            case 't':
                resultingChar = '\t';
                break;
            default:
                resultingChar = escape;
                break;
            }

            parsedString[parsedStringSize++] = resultingChar;
        }
        else
        {
            parsedString[parsedStringSize++] = c;
        }
    }

    sysmelb_Value_t result = {
        .kind = SysmelValueKindStringReference,
        .type = sysmelb_getBasicTypes()->string,
        .string = parsedString,
        .stringSize = parsedStringSize,
    };
    return result;
}

static void sysmelb_createBasicStringPrimitives(void)
{
    
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("="), sysmelb_primitive_stringEquals);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("~="), sysmelb_primitive_stringNotEquals);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("--"), sysmelb_primitive_concatenateString);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("size"), sysmelb_primitive_stringSize);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("at:"), sysmelb_primitive_stringAt);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("at:put:"), sysmelb_primitive_stringAtPut);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("substringFrom:until:"), sysmelb_primitive_substringFromUntil);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("asFloat"), sysmelb_primitive_stringAsFloat);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("asSymbol"), sysmelb_primitive_stringAsSymbol);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.string, sysmelb_internSymbolC("parseCEscapeSequences"), sysmelb_primitive_parseCEscapeSequences);
}

static sysmelb_Value_t sysmelb_primitive_symbolIdentityEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindSymbolReference);
    assert(arguments[1].kind == SysmelValueKindSymbolReference);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = arguments[0].symbolReference == arguments[1].symbolReference};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_symbolIdentityNotEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindSymbolReference);
    assert(arguments[1].kind == SysmelValueKindSymbolReference);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = arguments[0].symbolReference != arguments[1].symbolReference};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_symbolWithoutTrailingColon(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindSymbolReference);

    sysmelb_Value_t result = arguments[0];
    if(arguments[0].symbolReference->string[arguments[0].symbolReference->size - 1] == ':')
        result.symbolReference = sysmelb_internSymbol(arguments[0].symbolReference->size - 1, arguments[0].symbolReference->string);
    return result;
}

static sysmelb_Value_t sysmelb_primitive_symbolHash(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindSymbolReference);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .unsignedInteger = arguments[0].symbolReference->hash,
        .type = sysmelb_getBasicTypes()->integer
    };
    return result;
}

static void sysmelb_createBasicSymbolPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.symbol, sysmelb_internSymbolC("=="), sysmelb_primitive_symbolIdentityEquals);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.symbol, sysmelb_internSymbolC("~~"), sysmelb_primitive_symbolIdentityNotEquals);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.symbol, sysmelb_internSymbolC("withoutTrailingColon"), sysmelb_primitive_symbolWithoutTrailingColon);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.symbol, sysmelb_internSymbolC("hash"), sysmelb_primitive_symbolHash);
}

static sysmelb_Value_t sysmelb_primitive_concatenateArrays(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindArrayReference && arguments[1].kind == SysmelValueKindArrayReference);

    size_t arraySize = arguments[0].arrayReference->size + arguments[1].arrayReference->size;
    sysmelb_ArrayHeader_t *arrayData = sysmelb_allocate(sizeof(sysmelb_ArrayHeader_t) + sizeof(sysmelb_Value_t) * arraySize);
    arrayData->size = arraySize;

    memcpy(arrayData->elements, arguments[0].arrayReference->elements, arguments[0].arrayReference->size * sizeof(sysmelb_Value_t));
    memcpy(arrayData->elements + arguments[0].arrayReference->size, arguments[1].arrayReference->elements, arguments[1].arrayReference->size * sizeof(sysmelb_Value_t));

    sysmelb_Value_t result = {
        .kind = SysmelValueKindArrayReference,
        .type = sysmelb_getBasicTypes()->array,
        .arrayReference = arrayData};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_arraySize(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindArrayReference);

    size_t arraySize = arguments[0].arrayReference->size;
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_BasicTypesData.integer,
        .unsignedInteger = arraySize,
    };
    return result;
}

static sysmelb_Value_t sysmelb_primitive_arrayAt(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindArrayReference && (arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger));

    size_t arraySize = arguments[0].arrayReference->size;
    unsigned int arrayIndex = arguments[1].unsignedInteger;
    assert(arrayIndex < arraySize);

    sysmelb_Value_t result = arguments[0].arrayReference->elements[arrayIndex];
    return result;
}

static sysmelb_Value_t sysmelb_primitive_arrayAtPut(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    assert(arguments[0].kind == SysmelValueKindArrayReference && (arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger));

    size_t arraySize = arguments[0].arrayReference->size;
    unsigned int arrayIndex = arguments[1].unsignedInteger;
    assert(arrayIndex < arraySize);

    sysmelb_Value_t result = arguments[0].arrayReference->elements[arrayIndex] = arguments[2];
    return result;
}

static sysmelb_Value_t sysmelb_primitive_arrayAsTuple(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindArrayReference);

    sysmelb_ArrayHeader_t *array = arguments[0].arrayReference;
    size_t arraySize = array->size;
    sysmelb_TupleHeader_t *tuple = sysmelb_allocate(sizeof(sysmelb_TupleHeader_t) + sizeof(sysmelb_Value_t) * arraySize);
    tuple->size = arraySize;
    for (size_t i = 0; i < arraySize; ++i)
        tuple->elements[i] = array->elements[i];

    sysmelb_Value_t result = {
        .kind = SysmelValueKindTupleReference,
        .tupleReference = tuple,
        .type = sysmelb_getBasicTypes()->tuple};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_arrayAsImmutableDictionary(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindArrayReference);

    sysmelb_ArrayHeader_t *array = arguments[0].arrayReference;
    size_t arraySize = array->size;
    sysmelb_ImmutableDictionary_t *dictionary = sysmelb_allocate(sizeof(sysmelb_ImmutableDictionary_t) + sizeof(sysmelb_Association_t *)*arraySize);
    dictionary->size = arraySize;
    for (size_t i = 0; i < arraySize; ++i)
        dictionary->elements[i] = array->elements[i].associationReference;

    sysmelb_Value_t result = {
        .kind = SysmelValueKindImmutableDictionaryReference,
        .immutableDictionaryReference = dictionary,
        .type = sysmelb_getBasicTypes()->immutableDictionary};
    return result;
}

static void sysmelb_createBasicArrayPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.array, sysmelb_internSymbolC("--"), sysmelb_primitive_concatenateArrays);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.array, sysmelb_internSymbolC("size"), sysmelb_primitive_arraySize);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.array, sysmelb_internSymbolC("at:"), sysmelb_primitive_arrayAt);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.array, sysmelb_internSymbolC("at:put:"), sysmelb_primitive_arrayAtPut);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.array, sysmelb_internSymbolC("asTuple"), sysmelb_primitive_arrayAsTuple);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.array, sysmelb_internSymbolC("asImmutableDictionary"), sysmelb_primitive_arrayAsImmutableDictionary);
}

static sysmelb_Value_t sysmelb_primitive_tupleSize(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindTupleReference);

    size_t tupleSize = arguments[0].tupleReference->size;

    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_BasicTypesData.integer,
        .unsignedInteger = tupleSize};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_tupleAt(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindTupleReference && (arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger));

    size_t tupleSize = arguments[0].tupleReference->size;
    unsigned int tupleIndex = arguments[1].unsignedInteger;
    assert(tupleIndex < tupleSize);

    sysmelb_Value_t result = arguments[0].tupleReference->elements[tupleIndex];
    return result;
}

static sysmelb_Value_t sysmelb_primitive_tupleAtPut(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    assert(arguments[0].kind == SysmelValueKindTupleReference && (arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger));

    size_t tupleSize = arguments[0].tupleReference->size;
    unsigned int tupleIndex = arguments[1].unsignedInteger;
    assert(tupleIndex < tupleSize);

    sysmelb_Value_t result = arguments[2];
    arguments[0].tupleReference->elements[tupleIndex] = result;
    return result;
}

static void sysmelb_createBasicTuplePrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.tuple, sysmelb_internSymbolC("size"), sysmelb_primitive_tupleSize);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.tuple, sysmelb_internSymbolC("at:"), sysmelb_primitive_tupleAt);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.tuple, sysmelb_internSymbolC("at:put:"), sysmelb_primitive_tupleAtPut);
}

static sysmelb_Value_t sysmelb_primitive_associationKey(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindAssociationReference);
    sysmelb_Value_t result = arguments[0].associationReference->key;
    return result;
}

static sysmelb_Value_t sysmelb_primitive_associationValue(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindAssociationReference);
    sysmelb_Value_t result = arguments[0].associationReference->value;
    return result;
}

static void sysmelb_createBasicAssociationPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.association, sysmelb_internSymbolC("key"), sysmelb_primitive_associationKey);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.association, sysmelb_internSymbolC("value"), sysmelb_primitive_associationValue);
}

static sysmelb_Value_t sysmelb_primitive_dictionaryAssocAt(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindImmutableDictionaryReference && (arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger));

    size_t dictionarySize = arguments[0].immutableDictionaryReference->size;
    unsigned int dictionaryIndex = arguments[1].unsignedInteger;
    assert(dictionaryIndex < dictionarySize);

    sysmelb_Association_t *assoc = arguments[0].immutableDictionaryReference->elements[dictionaryIndex];
    sysmelb_Value_t result = {
        .kind = SysmelValueKindAssociationReference,
        .type = sysmelb_getBasicTypes()->association,
        .associationReference = assoc};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_dictionarySize(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindImmutableDictionaryReference);

    size_t dictionarySize = arguments[0].immutableDictionaryReference->size;
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_getBasicTypes()->integer,
        .unsignedInteger = dictionarySize};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_dictionaryIncludesKey(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindImmutableDictionaryReference);

    size_t dictionarySize = arguments[0].immutableDictionaryReference->size;
    for (size_t i = 0; i < dictionarySize; ++i)
    {
        sysmelb_Association_t *assoc = arguments[0].immutableDictionaryReference->elements[i];
        if (sysmelb_value_equals(arguments[1], assoc->key))
        {
            sysmelb_Value_t result = {
                .kind = SysmelValueKindBoolean,
                .type = sysmelb_getBasicTypes()->boolean,
                .boolean = true};
            return result;
        }
    }

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = false};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_dictionaryAt(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindImmutableDictionaryReference);

    size_t dictionarySize = arguments[0].immutableDictionaryReference->size;
    for (size_t i = 0; i < dictionarySize; ++i)
    {
        sysmelb_Association_t *assoc = arguments[0].immutableDictionaryReference->elements[i];
        if (sysmelb_value_equals(arguments[1], assoc->key))
            return assoc->value;
    }

    sysmelb_SourcePosition_t nullPosition = {};
    sysmelb_errorPrintf(nullPosition, "Key is not present in the dictionary.");
    abort();
}

static sysmelb_Value_t sysmelb_primitive_withSelectorAddMethod(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    assert(arguments[0].kind == SysmelValueKindTypeReference);
    assert(arguments[1].kind == SysmelValueKindSymbolReference);
    assert(arguments[2].kind == SysmelValueKindFunctionReference);

    sysmelb_Type_t *type = arguments[0].typeReference;
    sysmelb_symbol_t *selector = arguments[1].symbolReference;
    sysmelb_function_t *function = arguments[2].functionReference;

    sysmelb_SymbolHashtable_addSymbolWithValue(&type->methodDict, selector, function);
    return arguments[0];
}

static void sysmelb_createBasicImmutableDictionaryPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.immutableDictionary, sysmelb_internSymbolC("size"), sysmelb_primitive_dictionarySize);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.immutableDictionary, sysmelb_internSymbolC("associationAt:"), sysmelb_primitive_dictionaryAssocAt);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.immutableDictionary, sysmelb_internSymbolC("includesKey:"), sysmelb_primitive_dictionaryIncludesKey);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.immutableDictionary, sysmelb_internSymbolC("at:"), sysmelb_primitive_dictionaryAt);
}

static sysmelb_Value_t sysmelb_primitive_newWithSize(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindTypeReference);
    assert(arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger);


    if(arguments[0].typeReference->kind == SysmelTypeKindArray)
    {
        size_t arraySize = arguments[1].integer;
        sysmelb_ArrayHeader_t *array = sysmelb_allocate(sizeof(sysmelb_ArrayHeader_t) + sizeof(sysmelb_Value_t) * arraySize);
        array->size = arraySize;
        sysmelb_Value_t result = {
            .kind = SysmelValueKindArrayReference,
            .type = sysmelb_getBasicTypes()->array,
            .arrayReference = array};
        return result;
    }

    if(arguments[0].typeReference->kind == SysmelTypeKindByteArray)
    {
        size_t byteArraySize = arguments[1].integer;
        sysmelb_ByteArrayHeader_t *byteArray = sysmelb_allocate(sizeof(sysmelb_ByteArrayHeader_t) + byteArraySize);
        byteArray->size = byteArraySize;
        sysmelb_Value_t result = {
            .kind = SysmelValueKindByteArrayReference,
            .type = sysmelb_getBasicTypes()->byteArray,
            .byteArrayReference = byteArray};
        return result;
    }

    if(arguments[0].typeReference->kind == SysmelTypeKindTuple)
    {
        size_t tupleSize = arguments[1].integer;
        sysmelb_TupleHeader_t *tuple = sysmelb_allocate(sizeof(sysmelb_TupleHeader_t) + sizeof(sysmelb_Value_t) * tupleSize);
        tuple->size = tupleSize;
        sysmelb_Value_t result = {
            .kind = SysmelValueKindTupleReference,
            .type = sysmelb_getBasicTypes()->tuple,
            .tupleReference = tuple
        };
        return result;
    }
    if(arguments[0].typeReference->kind == SysmelTypeKindString)
    {
        size_t stringSize = arguments[1].integer;
        char *stringData = sysmelb_allocate(stringSize);
        sysmelb_Value_t result = {
            .kind = SysmelValueKindTupleReference,
            .type = sysmelb_getBasicTypes()->string,
            .stringSize = stringSize,
            .string = stringData
        };
        return result;
    }

    sysmelb_SourcePosition_t nullPosition;
    sysmelb_errorPrintf(nullPosition, "#new: only supports tuples, arrays, strings and byteArray.");
    abort();
}

static sysmelb_Value_t sysmelb_primitive_fixedArrayType(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindTypeReference);
    assert(arguments[1].kind == SysmelValueKindInteger || arguments[1].kind == SysmelValueKindUnsignedInteger);

    sysmelb_Type_t *fixedArrayType = sysmelb_allocateFixedArrayType(arguments[0].typeReference, arguments[1].integer);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindTypeReference,
        .type = sysmelb_getBasicTypes()->universe,
        .typeReference = fixedArrayType
    };
    return result;
}

static void sysmelb_createBasicTypeUniversePrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.universe, sysmelb_internSymbolC("withSelector:addMethod:"), sysmelb_primitive_withSelectorAddMethod);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.universe, sysmelb_internSymbolC("new:"), sysmelb_primitive_newWithSize);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.universe, sysmelb_internSymbolC("array:"), sysmelb_primitive_fixedArrayType);
}

static sysmelb_Value_t sysmelb_primitive_OrderedCollection_add(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindOrderedCollectionReference);
    sysmelb_OrderedCollection_add(arguments[0].orderedCollectionReference, arguments[1]);
    return arguments[1];
}

static sysmelb_Value_t sysmelb_primitive_OrderedCollection_size(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindOrderedCollectionReference);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_getBasicTypes()->integer,
        .unsignedInteger = arguments[0].orderedCollectionReference->size,
    };
    return result;
}

static sysmelb_Value_t sysmelb_primitive_OrderedCollection_at(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindOrderedCollectionReference);
    assert(arguments[1].kind == SysmelValueKindInteger ||
           arguments[1].kind == SysmelValueKindUnsignedInteger);
    size_t size = arguments[0].orderedCollectionReference->size;
    size_t index = arguments[1].unsignedInteger;
    if (index >= size)
    {
        sysmelb_SourcePosition_t pos = {};
        sysmelb_errorPrintf(pos, "Index %d is out of bounds (size %d).\n", (int)index, (int)size);
        abort();
    }

    return arguments[0].orderedCollectionReference->elements[index];
}

static sysmelb_Value_t sysmelb_primitive_OrderedCollection_atPut(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    assert(arguments[0].kind == SysmelValueKindOrderedCollectionReference);
    assert(arguments[1].kind == SysmelValueKindInteger ||
           arguments[1].kind == SysmelValueKindUnsignedInteger);
    size_t size = arguments[0].orderedCollectionReference->size;
    size_t index = arguments[1].unsignedInteger;
    if (index >= size)
    {
        sysmelb_SourcePosition_t pos = {};
        sysmelb_errorPrintf(pos, "Index %d is out of bounds (size %d).\n", (int)index, (int)size);
        abort();
    }

    return arguments[0].orderedCollectionReference->elements[index] = arguments[2];
}

static sysmelb_Value_t sysmelb_primitive_OrderedCollection_asArray(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindOrderedCollectionReference);

    size_t dataSize = arguments[0].orderedCollectionReference->size;
    sysmelb_ArrayHeader_t *array = sysmelb_allocate(sizeof(sysmelb_ArrayHeader_t) + dataSize * sizeof(sysmelb_Value_t));
    array->size = dataSize;
    for (size_t i = 0; i < dataSize; ++i)
        array->elements[i] = arguments[0].orderedCollectionReference->elements[i];

    sysmelb_Value_t result = {
        .kind = SysmelValueKindArrayReference,
        .type = sysmelb_getBasicTypes()->array,
        .arrayReference = array};
    return result;
}

static void sysmelb_createBasicOrderedCollectionPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.orderedCollection, sysmelb_internSymbolC("add:"), sysmelb_primitive_OrderedCollection_add);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.orderedCollection, sysmelb_internSymbolC("size"), sysmelb_primitive_OrderedCollection_size);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.orderedCollection, sysmelb_internSymbolC("at:"), sysmelb_primitive_OrderedCollection_at);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.orderedCollection, sysmelb_internSymbolC("at:put:"), sysmelb_primitive_OrderedCollection_atPut);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.orderedCollection, sysmelb_internSymbolC("asArray"), sysmelb_primitive_OrderedCollection_asArray);
}

static sysmelb_Value_t sysmelb_primitive_ByteOrderedCollection_add(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindByteOrderedCollectionReference);
    sysmelb_ByteOrderedCollection_add(arguments[0].byteOrderedCollectionReference, arguments[1].integer);
    return arguments[1];
}

static sysmelb_Value_t sysmelb_primitive_ByteOrderedCollection_addAll(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindByteOrderedCollectionReference);
    assert(arguments[1].kind == SysmelValueKindByteArrayReference);
    for(size_t i = 0; i < arguments[1].byteArrayReference->size; ++i)
        sysmelb_ByteOrderedCollection_add(arguments[0].byteOrderedCollectionReference, arguments[1].byteArrayReference->elements[i]);
    return arguments[1];
}

static sysmelb_Value_t sysmelb_primitive_ByteOrderedCollection_size(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindByteOrderedCollectionReference);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_getBasicTypes()->integer,
        .unsignedInteger = arguments[0].byteOrderedCollectionReference->size,
    };
    return result;
}

static sysmelb_Value_t sysmelb_primitive_ByteOrderedCollection_at(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindByteOrderedCollectionReference);
    assert(arguments[1].kind == SysmelValueKindInteger ||
           arguments[1].kind == SysmelValueKindUnsignedInteger);
    size_t size = arguments[0].byteOrderedCollectionReference->size;
    size_t index = arguments[1].unsignedInteger;
    if (index >= size)
    {
        sysmelb_SourcePosition_t pos = {};
        sysmelb_errorPrintf(pos, "Index %d is out of bounds (size %d).\n", (int)index, (int)size);
        abort();
    }

    uint8_t byte = arguments[0].byteOrderedCollectionReference->elements[index];
    sysmelb_Value_t result = {
        .kind = SysmelValueKindInteger,
        .type = sysmelb_getBasicTypes()->integer,
        .integer = byte,
    };
    return result;
}

static sysmelb_Value_t sysmelb_primitive_ByteOrderedCollection_atPut(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    assert(arguments[0].kind == SysmelValueKindByteOrderedCollectionReference);
    assert(arguments[1].kind == SysmelValueKindInteger ||
           arguments[1].kind == SysmelValueKindUnsignedInteger);
    size_t size = arguments[0].byteOrderedCollectionReference->size;
    size_t index = arguments[1].unsignedInteger;
    if (index >= size)
    {
        sysmelb_SourcePosition_t pos = {};
        sysmelb_errorPrintf(pos, "Index %d is out of bounds (size %d).\n", (int)index, (int)size);
        abort();
    }

    arguments[0].byteOrderedCollectionReference->elements[index] = arguments[2].integer;

    sysmelb_Value_t result = {
        .kind = SysmelValueKindVoid,
        .type = sysmelb_getBasicTypes()->voidType,
    };
    return result;
}

static sysmelb_Value_t sysmelb_primitive_ByteOrderedCollection_asByteArray(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindByteOrderedCollectionReference);

    size_t dataSize = arguments[0].byteOrderedCollectionReference->size;
    sysmelb_ByteArrayHeader_t *byteArray = sysmelb_allocate(sizeof(sysmelb_ByteArrayHeader_t) + dataSize);
    byteArray->size = dataSize;
    memcpy(byteArray->elements, arguments[0].byteOrderedCollectionReference->elements, dataSize);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindByteArrayReference,
        .type = sysmelb_getBasicTypes()->byteArray,
        .byteArrayReference = byteArray
    };
    return result;
}

static void sysmelb_createBasicByteOrderedCollectionPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.byteOrderedCollection, sysmelb_internSymbolC("add:"),        sysmelb_primitive_ByteOrderedCollection_add);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.byteOrderedCollection, sysmelb_internSymbolC("addAll:"),     sysmelb_primitive_ByteOrderedCollection_addAll);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.byteOrderedCollection, sysmelb_internSymbolC("size"),        sysmelb_primitive_ByteOrderedCollection_size);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.byteOrderedCollection, sysmelb_internSymbolC("at:"),         sysmelb_primitive_ByteOrderedCollection_at);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.byteOrderedCollection, sysmelb_internSymbolC("at:put:"),     sysmelb_primitive_ByteOrderedCollection_atPut);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.byteOrderedCollection, sysmelb_internSymbolC("asByteArray"), sysmelb_primitive_ByteOrderedCollection_asByteArray);
}

static sysmelb_Value_t sysmelb_primitive_SymbolHashtable_size(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindSymbolHashtableReference);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindUnsignedInteger,
        .type = sysmelb_getBasicTypes()->integer,
        .unsignedInteger = arguments[0].symbolHashtableReference->size,
    };
    return result;
}

static sysmelb_Value_t sysmelb_primitive_SymbolHashtable_includesKey(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindSymbolHashtableReference);

    const sysmelb_SymbolHashtablePair_t *lookupResult = sysmelb_SymbolHashtable_lookupSymbol(arguments[0].symbolHashtableReference, arguments[1].symbolReference);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = lookupResult != NULL && lookupResult->value != NULL};

    return result;
}

static sysmelb_Value_t sysmelb_primitive_SymbolHashtable_at(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindSymbolHashtableReference);

    const sysmelb_SymbolHashtablePair_t *lookupResult = sysmelb_SymbolHashtable_lookupSymbol(arguments[0].symbolHashtableReference, arguments[1].symbolReference);
    if (!lookupResult)
    {
        sysmelb_SourcePosition_t null = {0};
        sysmelb_errorPrintf(null, "Failed to find key in symbol hashtable.");
    }

    sysmelb_Value_t *resultPointer = (sysmelb_Value_t *)lookupResult->value;
    return *resultPointer;
}

static sysmelb_Value_t sysmelb_primitive_SymbolHashtable_atPut(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    assert(arguments[0].kind == SysmelValueKindSymbolHashtableReference);
    assert(arguments[1].kind == SysmelValueKindSymbolReference);

    sysmelb_Value_t *resultPointer = sysmelb_allocateValue();
    *resultPointer = arguments[2];

    sysmelb_SymbolHashtable_addSymbolWithValue(arguments[0].symbolHashtableReference, arguments[1].symbolReference, resultPointer);
    return *resultPointer;
}

static void sysmelb_createBasicSymbolHashtablePrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.symbolHashtable, sysmelb_internSymbolC("size"), sysmelb_primitive_SymbolHashtable_size);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.symbolHashtable, sysmelb_internSymbolC("includesKey:"), sysmelb_primitive_SymbolHashtable_includesKey);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.symbolHashtable, sysmelb_internSymbolC("at:"), sysmelb_primitive_SymbolHashtable_at);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.symbolHashtable, sysmelb_internSymbolC("at:put:"), sysmelb_primitive_SymbolHashtable_atPut);
}

static sysmelb_Value_t sysmelb_primitive_IdentityHashset_add(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindIdentityHashsetReference);

    sysmelb_IdentityHashset_add(arguments[0].identityHashsetReference, sysmelb_getValuePointer(arguments[1]));
    return arguments[1];
}

static sysmelb_Value_t sysmelb_primitive_IdentityHashset_includes(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindIdentityHashsetReference);

    bool includesResult = sysmelb_IdentityHashset_includes(arguments[0].identityHashsetReference, sysmelb_getValuePointer(arguments[1]));
    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = includesResult};

    return result;
}

static void sysmelb_createBasicIdentityHashsetPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.identityHashset, sysmelb_internSymbolC("add:"), sysmelb_primitive_IdentityHashset_add);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.identityHashset, sysmelb_internSymbolC("includes:"), sysmelb_primitive_IdentityHashset_includes);
}

static sysmelb_Value_t sysmelb_primitive_IdentityDictionary_includesKey(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindIdentityDictionaryReference);

    bool includesResult = sysmelb_IdentityDictionary_includesKey(arguments[0].identityDictionaryReference, sysmelb_getValuePointer(arguments[1]));
    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = includesResult};

    return result;
}

static sysmelb_Value_t sysmelb_primitive_IdentityDictionary_atPut(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    assert(arguments[0].kind == SysmelValueKindIdentityDictionaryReference);
    assert(arguments[1].kind == SysmelValueKindObjectReference);

    sysmelb_IdentityDictionary_atPut(arguments[0].identityDictionaryReference, sysmelb_getValuePointer(arguments[1]), sysmelb_getValuePointer(arguments[2]));
    return arguments[2];
}

static sysmelb_Value_t sysmelb_primitive_IdentityDictionary_at(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindIdentityDictionaryReference);

    void *objectRawPointer = sysmelb_IdentityDictionary_at(arguments[0].identityDictionaryReference, sysmelb_getValuePointer(arguments[1]));
    sysmelb_ObjectHeader_t *objectPointer = objectRawPointer;
    
    sysmelb_Value_t resultValue = {
        .kind = SysmelValueKindObjectReference,
        .objectReference = objectPointer,
        .type = objectPointer->clazz
    };
    return resultValue;
}

static void sysmelb_createBasicIdentityDictionaryPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.identityDictionary, sysmelb_internSymbolC("includesKey:"), sysmelb_primitive_IdentityDictionary_includesKey);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.identityDictionary, sysmelb_internSymbolC("at:put:"), sysmelb_primitive_IdentityDictionary_atPut);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.identityDictionary, sysmelb_internSymbolC("at:"), sysmelb_primitive_IdentityDictionary_at);
}


static sysmelb_Value_t sysmelb_primitive_Boolean_Not(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindBoolean);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = !arguments[0].boolean};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_Boolean_Equals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindBoolean);
    assert(arguments[1].kind == SysmelValueKindBoolean);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = arguments[0].boolean == arguments[1].boolean};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_Boolean_NotEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindBoolean);
    assert(arguments[1].kind == SysmelValueKindBoolean);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = arguments[0].boolean != arguments[1].boolean};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_Boolean_And(sysmelb_MacroContext_t *context, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindParseTreeReference);
    assert(arguments[1].kind == SysmelValueKindParseTreeReference);

    sysmelb_ParseTreeNode_t *falseResult = sysmelb_newParseTreeNode(ParseTreeLiteralValueNode, context->sourcePosition);
    falseResult->literalValue.value.kind = SysmelValueKindBoolean;
    falseResult->literalValue.value.type = sysmelb_getBasicTypes()->boolean;
    falseResult->literalValue.value.boolean = false;

    sysmelb_ParseTreeNode_t *ifNode = sysmelb_newParseTreeNode(ParseTreeIfSelection, context->sourcePosition);
    ifNode->ifSelection.condition = arguments[0].parseTreeReference;
    ifNode->ifSelection.trueExpression = arguments[1].parseTreeReference;
    ifNode->ifSelection.falseExpression = falseResult;

    sysmelb_Value_t nodeValue = {
        .kind = SysmelValueKindParseTreeReference,
        .type = sysmelb_getBasicTypes()->parseTreeNode,
        .parseTreeReference = ifNode};
    return nodeValue;
}

static sysmelb_Value_t sysmelb_primitive_Boolean_Or(sysmelb_MacroContext_t *context, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindParseTreeReference);
    assert(arguments[1].kind == SysmelValueKindParseTreeReference);

    sysmelb_ParseTreeNode_t *trueResult = sysmelb_newParseTreeNode(ParseTreeLiteralValueNode, context->sourcePosition);
    trueResult->literalValue.value.kind = SysmelValueKindBoolean;
    trueResult->literalValue.value.type = sysmelb_getBasicTypes()->boolean;
    trueResult->literalValue.value.boolean = true;

    sysmelb_ParseTreeNode_t *ifNode = sysmelb_newParseTreeNode(ParseTreeIfSelection, context->sourcePosition);
    ifNode->ifSelection.condition = arguments[0].parseTreeReference;
    ifNode->ifSelection.trueExpression = trueResult;
    ifNode->ifSelection.falseExpression = arguments[1].parseTreeReference;

    sysmelb_Value_t nodeValue = {
        .kind = SysmelValueKindParseTreeReference,
        .type = sysmelb_getBasicTypes()->parseTreeNode,
        .parseTreeReference = ifNode};
    return nodeValue;
}

static void sysmelb_createBasicBooleanPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.boolean, sysmelb_internSymbolC("="), sysmelb_primitive_Boolean_Equals);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.boolean, sysmelb_internSymbolC("~="), sysmelb_primitive_Boolean_NotEquals);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.boolean, sysmelb_internSymbolC("not"), sysmelb_primitive_Boolean_Not);
    sysmelb_type_addPrimitiveMacroMethod(sysmelb_BasicTypesData.boolean, sysmelb_internSymbolC("&&"), sysmelb_primitive_Boolean_And);
    sysmelb_type_addPrimitiveMacroMethod(sysmelb_BasicTypesData.boolean, sysmelb_internSymbolC("||"), sysmelb_primitive_Boolean_Or);
}

static sysmelb_Value_t sysmelb_primitive_Null_Equals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindNull);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = arguments[1].kind == SysmelValueKindNull};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_Null_NotEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindNull);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = arguments[1].kind != SysmelValueKindNull};
    return result;
}

static void sysmelb_createBasicNullPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.null, sysmelb_internSymbolC("=="), sysmelb_primitive_Null_Equals);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.null, sysmelb_internSymbolC("~~"), sysmelb_primitive_Null_NotEquals);
}

static sysmelb_Value_t sysmelb_primitive_Object_Equals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindObjectReference);
    assert(arguments[1].kind == SysmelValueKindObjectReference);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = arguments[0].objectReference == arguments[1].objectReference};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_Object_NotEquals(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    assert(arguments[0].kind == SysmelValueKindObjectReference);
    assert(arguments[1].kind == SysmelValueKindObjectReference);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindBoolean,
        .type = sysmelb_getBasicTypes()->boolean,
        .boolean = arguments[0].objectReference == arguments[1].objectReference};
    return result;
}

static sysmelb_Value_t sysmelb_primitive_Object_GetClass(size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    assert(arguments[0].kind == SysmelValueKindObjectReference);

    sysmelb_Value_t result = {
        .kind = SysmelValueKindTypeReference,
        .type = sysmelb_getBasicTypes()->universe,
        .typeReference = arguments[0].objectReference->clazz};
    return result;
}

static void sysmelb_createObjectPrimitives(void)
{
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.clazz, sysmelb_internSymbolC("=="), sysmelb_primitive_Object_Equals);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.clazz, sysmelb_internSymbolC("~~"), sysmelb_primitive_Object_NotEquals);
    sysmelb_type_addPrimitiveMethod(sysmelb_BasicTypesData.clazz, sysmelb_internSymbolC("class"), sysmelb_primitive_Object_GetClass);
}

static void sysmelb_createBasicTypesPrimitives(void)
{
    sysmelb_createBasicBooleanPrimitives();
    sysmelb_createBasicNullPrimitives();
    sysmelb_createBasicIntegersPrimitives();
    sysmelb_createBasicArrayPrimitives();
    sysmelb_createBasicStringPrimitives();
    sysmelb_createBasicSymbolPrimitives();
    sysmelb_createBasicTuplePrimitives();
    sysmelb_createBasicAssociationPrimitives();
    sysmelb_createBasicImmutableDictionaryPrimitives();
    sysmelb_createBasicOrderedCollectionPrimitives();
    sysmelb_createBasicByteOrderedCollectionPrimitives();
    sysmelb_createBasicSymbolHashtablePrimitives();
    sysmelb_createBasicIdentityHashsetPrimitives();
    sysmelb_createBasicIdentityDictionaryPrimitives();
    sysmelb_createObjectPrimitives();
    sysmelb_createBasicTypeUniversePrimitives();
}

const sysmelb_BasicTypes_t *sysmelb_getBasicTypes(void)
{
    if (sysmelb_BasicTypesDataInitialized)
        return &sysmelb_BasicTypesData;

    sysmelb_createBasicTypes();
    sysmelb_createBasicTypesPrimitives();

    sysmelb_BasicTypesDataInitialized = true;
    return &sysmelb_BasicTypesData;
}