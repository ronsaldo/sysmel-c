#include "symbol.h"
#include "memory.h"
#include <string.h>
#include <stdbool.h>

typedef struct sysmelb_symbolHashSet_s
{
    size_t capacity;
    size_t size;
    size_t targetCapacity;
    sysmelb_symbol_t **internedSymbols;
} sysmelb_symbolHashSet_t;

static sysmelb_symbolHashSet_t sysmelb_internedSymbolSet;

bool sysmelb_symbolEquals(sysmelb_symbol_t *a, sysmelb_symbol_t *b)
{
    return a->size == b->size
        && a->hash == b->hash
        && memcmp(a->string, b->string, a->size);
}

bool sysmelb_symbolStringEquals(sysmelb_symbol_t *a, size_t stringSize, const char *string)
{
    return a->size == stringSize
        && memcmp(a->string, string, stringSize) == 0;
}

uint32_t sysmelb_stringHash(size_t stringSize, const char *string)
{
    uint32_t hash = 0;
    for(size_t i = 0; i < stringSize; ++i)
        hash = hash*1664525 + string[i];
    return hash;
}

int32_t sysmelb_symbolScanForString(size_t stringSize, const char *string)
{
    if(sysmelb_internedSymbolSet.capacity == 0)
        return -1;

    uint32_t hash = sysmelb_stringHash(stringSize, string);
    uint32_t hashIndex = hash % sysmelb_internedSymbolSet.capacity;

    for(uint32_t i = hashIndex; i < sysmelb_internedSymbolSet.capacity; ++i)
    {
        if(!sysmelb_internedSymbolSet.internedSymbols[i] ||
            sysmelb_symbolStringEquals(sysmelb_internedSymbolSet.internedSymbols[i], stringSize, string))
                return i;
    }

    for(uint32_t i = 0; i < hashIndex; ++i)
    {
        if(!sysmelb_internedSymbolSet.internedSymbols[i] ||
            sysmelb_symbolStringEquals(sysmelb_internedSymbolSet.internedSymbols[i], stringSize, string))
                return i;
    }

    return -1;
}

sysmelb_symbol_t *sysmelb_internSymbol(size_t stringSize, const char *string)
{
    if(sysmelb_internedSymbolSet.capacity == 0)
    {
        sysmelb_internedSymbolSet.capacity = 1024;
        sysmelb_internedSymbolSet.targetCapacity = sysmelb_internedSymbolSet.capacity * 80 / 100;
        sysmelb_internedSymbolSet.internedSymbols = sysmelb_allocate(sysmelb_internedSymbolSet.capacity * sizeof(sysmelb_symbol_t *));
    }

    int32_t slotIndex = sysmelb_symbolScanForString(stringSize, string);
    assert(slotIndex >= 0);

    if(sysmelb_internedSymbolSet.internedSymbols[slotIndex] != NULL)
        return sysmelb_internedSymbolSet.internedSymbols[slotIndex];

    size_t symbolAllocationSize = sizeof(sysmelb_symbol_t) + stringSize + 1;
    sysmelb_symbol_t *newSymbol = sysmelb_allocate(symbolAllocationSize);

    newSymbol->hash = sysmelb_stringHash(stringSize, string);
    newSymbol->size = stringSize;
    memcpy(newSymbol->string, string, stringSize);

    sysmelb_internedSymbolSet.internedSymbols[slotIndex] = newSymbol;
    ++sysmelb_internedSymbolSet.size;

    return newSymbol;
}

sysmelb_symbol_t *sysmelb_internSymbolC(const char *string)
{
    return sysmelb_internSymbol(strlen(string), string);
}