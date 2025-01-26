#ifndef SYSMELB_VALUE_H
#define SYSMELB_VALUE_H

#include "types.h"
#include "hashtable.h"
#include <stdbool.h>

typedef enum sysmelb_ValueKind_e
{
    SysmelValueKindNull,
    SysmelValueKindVoid,
    SysmelValueKindBoolean,
    SysmelValueKindCharacter,
    SysmelValueKindInteger,
    SysmelValueKindUnsignedInteger,
    SysmelValueKindFloatingPoint,

    SysmelValueKindTypeReference,
    SysmelValueKindFunctionReference,
    SysmelValueKindParseTreeReference,
    SysmelValueKindSymbolReference,
    SysmelValueKindStringReference,
    SysmelValueKindArrayReference,
    SysmelValueKindByteArrayReference,
    SysmelValueKindTupleReference,
    SysmelValueKindObjectReference,
    SysmelValueKindAssociationReference,
    SysmelValueKindImmutableDictionaryReference,
    SysmelValueKindSymbolHashtableReference,
    SysmelValueKindValueBoxReference,
    SysmelValueKindNamespaceReference,
    SysmelValueKindOrderedCollectionReference,
    SysmelValueKindSumValueReference,
    SysmelValueKindIdentityHashsetReference,
} sysmelb_ValueKind_t;

typedef struct sysmelb_Value_s sysmelb_Value_t;
typedef struct sysmelb_function_s sysmelb_function_t;
typedef struct sysmelb_Namespace_s sysmelb_Namespace_t;

typedef int64_t sysmelb_IntegerLiteralType_t;
typedef uint64_t sysmelb_UnsignedIntegerLiteralType_t;

typedef struct sysmelb_ParseTreeNode_s sysmelb_ParseTreeNode_t;

typedef struct sysmelb_ArrayHeader_s sysmelb_ArrayHeader_t;
typedef struct sysmelb_ByteArrayHeader_s sysmelb_ByteArrayHeader_t;
typedef struct sysmelb_TupleHeader_s sysmelb_TupleHeader_t;
typedef struct sysmelb_ObjectHeader_s sysmelb_ObjectHeader_t;

typedef struct sysmelb_Association_s sysmelb_Association_t;
typedef struct sysmelb_ImmutableDictionary_s sysmelb_ImmutableDictionary_t;
typedef struct sysmelb_IdentityDictionary_s sysmelb_IdentityDictionary_t;

typedef struct sysmelb_ValueBox_s sysmelb_ValueBox_t;

typedef struct sysmelb_OrderedCollection_s sysmelb_OrderedCollection_t;
typedef struct sysmelb_SumTypeValue_s sysmelb_SumTypeValue_t;

struct sysmelb_Value_s
{
    sysmelb_ValueKind_t kind;
    sysmelb_Type_t *type;
    union
    {
        bool boolean;
        sysmelb_IntegerLiteralType_t integer;
        sysmelb_UnsignedIntegerLiteralType_t unsignedInteger;
        double floatingPoint;

        sysmelb_Type_t *typeReference;
        sysmelb_symbol_t *symbolReference;
        sysmelb_function_t *functionReference;
        sysmelb_ParseTreeNode_t *parseTreeReference;
        sysmelb_ArrayHeader_t *arrayReference;
        sysmelb_ByteArrayHeader_t *byteArrayReference;
        sysmelb_TupleHeader_t *tupleReference;
        sysmelb_ObjectHeader_t *objectReference;
        sysmelb_Association_t *associationReference;
        sysmelb_ImmutableDictionary_t *immutableDictionaryReference;
        sysmelb_IdentityDictionary_t *identityDictionaryReference;

        sysmelb_ValueBox_t *valueBoxReference;

        sysmelb_Namespace_t *namespaceReference;

        sysmelb_OrderedCollection_t *orderedCollectionReference;
        sysmelb_SymbolHashtable_t *symbolHashtableReference;
        sysmelb_IdentityHashset_t *identityHashsetReference;
        
        sysmelb_SumTypeValue_t *sumTypeValueReference;

        struct
        {
            size_t stringSize;
            char *string;
        };
    };
};

struct sysmelb_ArrayHeader_s
{
    size_t size;
    sysmelb_Value_t elements[];
};

struct sysmelb_ByteArrayHeader_s
{
    size_t size;
    uint8_t elements[];
};

struct sysmelb_TupleHeader_s
{
    size_t size;
    sysmelb_Value_t elements[];
};

struct sysmelb_ObjectHeader_s
{
    size_t size;
    sysmelb_Type_t *clazz;
    sysmelb_Value_t elements[];
};

struct sysmelb_Association_s
{
    sysmelb_Value_t key;
    sysmelb_Value_t value;
};

struct sysmelb_ImmutableDictionary_s
{
    size_t size;
    sysmelb_Association_t *elements[];
};

struct sysmelb_ValueBox_s
{
    sysmelb_Value_t currentValue;
};

struct sysmelb_OrderedCollection_s
{
    size_t capacity;
    size_t size;
    sysmelb_Value_t *elements;
};

struct sysmelb_SumTypeValue_s
{
    uint32_t alternativeIndex;
    sysmelb_Value_t alternativeValue;
};

void sysmelb_OrderedCollection_add(sysmelb_OrderedCollection_t *collection, sysmelb_Value_t value);

sysmelb_Value_t *sysmelb_allocateValue(void);
sysmelb_Value_t sysmelb_decayValue(sysmelb_Value_t value);
bool sysmelb_value_equals(sysmelb_Value_t a, sysmelb_Value_t b);
void sysmelb_printValue(sysmelb_Value_t value);
void sysmelb_printValueWithMaxDepth(sysmelb_Value_t value, int depth);
void *sysmelb_getValuePointer(sysmelb_Value_t value);
uint64_t sysmelb_getValueIdentityHash(sysmelb_Value_t value);

#endif // SYSMELB_VALUE_H