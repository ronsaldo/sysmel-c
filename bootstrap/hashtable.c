#include "hashtable.h"
#include "memory.h"

int sysmelb_SymbolHashtable_scanFor(sysmelb_SymbolHashtable_t *table, sysmelb_symbol_t *key)
{
    size_t index = key->hash % table->capacity;
    for(size_t i = index; i < table->capacity; ++i)
    {
        if(!table->data[i].key || table->data[i].key == key)
            return i;
    }

    for(size_t i = 0; i < index; ++i)
    {
        if(!table->data[i].key || table->data[i].key == key)
            return i;
    }

    return -1;
}

void sysmelb_SymbolHashtable_incrementCapacity(sysmelb_SymbolHashtable_t *table)
{
    size_t newCapacity = table->capacity*2;
    if(newCapacity < 32)
        newCapacity = 32;

    sysmelb_SymbolHashtablePair_t *oldData = table->data;
    size_t oldCapacity = table->capacity;

    table->capacity = newCapacity;
    table->targetCapacity = newCapacity * 80 / 100;
    table->size = 0;
    table->data = sysmelb_allocate(sizeof(sysmelb_SymbolHashtablePair_t)*newCapacity);

    // Reinsert the old elements.
    for(size_t i = 0; i < oldCapacity; ++i)
    {
        if(oldData[i].key)
            sysmelb_SymbolHashtable_addSymbolWithValue(table, oldData[i].key, oldData[i].value);
    }

    sysmelb_freeAllocation(oldData);
}

void sysmelb_SymbolHashtable_addSymbolWithValue(sysmelb_SymbolHashtable_t *table, sysmelb_symbol_t *key, void *value)
{
    if(!table->capacity)
    {
        table->capacity = 32;
        table->targetCapacity = 32*80/100;
        table->data = sysmelb_allocate(sizeof(sysmelb_SymbolHashtablePair_t) * table->capacity);
    }

    int bucketIndex = sysmelb_SymbolHashtable_scanFor(table, key);
    if(bucketIndex < 0 || table->size + 1 >= table->targetCapacity)
    {
        sysmelb_SymbolHashtable_incrementCapacity(table);
        bucketIndex = sysmelb_SymbolHashtable_scanFor(table, key);
        assert(bucketIndex >= 0);
    }

    sysmelb_SymbolHashtablePair_t *bucket = table->data + bucketIndex;
    if(!bucket->key)
        ++table->size;
    bucket->key = key;
    bucket->value = value;
}

const sysmelb_SymbolHashtablePair_t *sysmelb_SymbolHashtable_lookupSymbol(sysmelb_SymbolHashtable_t *table,  sysmelb_symbol_t *key)
{
    if(table->size == 0)
        return NULL;

    int bucketIndex = sysmelb_SymbolHashtable_scanFor(table, key);
    if(bucketIndex < 0)
        return NULL;

    sysmelb_SymbolHashtablePair_t *bucket = table->data + bucketIndex;
    if(!bucket->key)
        return NULL;
    return bucket;
}