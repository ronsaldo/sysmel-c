#include "string.h"
#include "memory.h"
#include <string.h>

void sysmelb_dynstring_ensureCapacityFor(sysmelb_dynstring_t *dynstring, size_t neededCapacity)
{
    size_t requiredCapacity = dynstring->size + neededCapacity + 1;

    if(dynstring->capacity < requiredCapacity)
    {
        size_t newCapacity = dynstring->capacity * 2;
        if(newCapacity < 32)
            newCapacity = 32;
        if(newCapacity < requiredCapacity)
            newCapacity = requiredCapacity;

        char *newStorage = sysmelb_allocate(newCapacity);
        if(dynstring->data)
        {
            memcpy(newStorage, dynstring->data, dynstring->size + 1);
            sysmelb_freeAllocation(dynstring->data);
        }

        dynstring->data = newStorage;
        dynstring->capacity = newCapacity;
    }
}

void sysmelb_dynstring_append(sysmelb_dynstring_t *dynstring, size_t textSize, const char *text)
{
    sysmelb_dynstring_ensureCapacityFor(dynstring, textSize);
    memcpy(dynstring->data + dynstring->size, text, textSize);
    dynstring->size += textSize;
    dynstring->data[dynstring->size] = 0;
}

void sysmelb_dynstring_appendCString(sysmelb_dynstring_t *dynstring, const char *text)
{
    sysmelb_dynstring_append(dynstring, strlen(text), text);
}
