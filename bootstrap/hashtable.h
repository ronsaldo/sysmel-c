#ifndef SYSMELB_HASHTABLE_H
#define SYSMELB_HASHTABLE_H

#pragma once

#include "symbol.h"
#include <stdbool.h>

typedef struct sysmelb_SymbolHashtablePair_s
{
    sysmelb_symbol_t *key;
    void *value;
} sysmelb_SymbolHashtablePair_t;

typedef struct sysmelb_SymbolHashtable_s
{
    size_t capacity;
    size_t targetCapacity;
    size_t size;
    sysmelb_SymbolHashtablePair_t *data;
} sysmelb_SymbolHashtable_t;

typedef struct sysmelb_IdentityHashset_s
{
    size_t capacity;
    size_t targetCapacity;
    size_t size;
    void **data;
} sysmelb_IdentityHashset_t;

void sysmelb_SymbolHashtable_addSymbolWithValue(sysmelb_SymbolHashtable_t *table, sysmelb_symbol_t *key, void *value);
const sysmelb_SymbolHashtablePair_t *sysmelb_SymbolHashtable_lookupSymbol(sysmelb_SymbolHashtable_t *table, sysmelb_symbol_t *keyToLookup);

void sysmelb_IdentityHashset_add(sysmelb_IdentityHashset_t *set, void *value);
bool sysmelb_IdentityHashset_includes(sysmelb_IdentityHashset_t *set, void *value);

#endif //SYSMELB_HASHTABLE_H