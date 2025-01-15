#ifndef SYSMELB_VALUE_H
#define SYSMELB_VALUE_H

#include "types.h"
#include <stdbool.h>

typedef enum sysmelb_ValueKind_e {
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
    SysmelValueKindAssociationReference,
    SysmelValueKindDictionaryReference,
    SysmelValueKindValueBoxReference,
} sysmelb_ValueKind_t;

typedef struct sysmelb_Value_s sysmelb_Value_t;
typedef struct sysmelb_function_s sysmelb_function_t;

typedef int64_t sysmelb_IntegerLiteralType_t;
typedef uint64_t sysmelb_UnsignedIntegerLiteralType_t;

typedef struct sysmelb_ParseTreeNode_s sysmelb_ParseTreeNode_t;

typedef struct sysmelb_ArrayHeader_s sysmelb_ArrayHeader_t;
typedef struct sysmelb_ByteArrayHeader_s sysmelb_ByteArrayHeader_t;
typedef struct sysmelb_TupleHeader_s sysmelb_TupleHeader_t;

typedef struct sysmelb_Association_s sysmelb_Association_t;
typedef struct sysmelb_Dictionary_s sysmelb_Dictionary_t;

typedef struct sysmelb_ValueBox_s sysmelb_ValueBox_t;

struct sysmelb_Value_s
{
    sysmelb_ValueKind_t kind;
    sysmelb_Type_t *type;
    union {
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
        sysmelb_Association_t *associationReference;
        sysmelb_Dictionary_t *dictionaryReference;

        sysmelb_ValueBox_t *valueBoxReference;

        struct
        {
            size_t stringSize;
            const char *string;
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

struct sysmelb_Association_s
{
    sysmelb_Value_t key;
    sysmelb_Value_t value;
};

struct sysmelb_Dictionary_s
{
    size_t size;
    sysmelb_Association_t *elements[];
};

struct sysmelb_ValueBox_s
{
    sysmelb_Value_t currentValue;
};

sysmelb_Value_t sysmelb_decayValue(sysmelb_Value_t value);
bool sysmelb_value_equals(sysmelb_Value_t a, sysmelb_Value_t b);
void sysmelb_printValue(sysmelb_Value_t value);

#endif //SYSMELB_VALUE_H