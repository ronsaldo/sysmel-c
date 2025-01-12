#include "memory.h"
#include <stdlib.h>

typedef struct sysmelb_AllocationHeader_s sysmelb_AllocationHeader_t;

static sysmelb_AllocationHeader_t *sysmelb_LastMemoryAllocation;

struct sysmelb_AllocationHeader_s
{
    struct sysmelb_AllocationHeader_s *nextAllocation;
    size_t allocationSize;
};

void *sysmelb_allocate(size_t allocationSize)
{
    size_t sizeWithHeader = sizeof(sysmelb_AllocationHeader_t) + allocationSize;
    sysmelb_AllocationHeader_t *header = malloc(sizeWithHeader);
    header->nextAllocation = sysmelb_LastMemoryAllocation;
    header->allocationSize = allocationSize;
    sysmelb_LastMemoryAllocation = header;
    return header + 1;
}


void sysmelb_freeAllocation(void *allocation)
{
    if(!allocation)
        return;
        
    sysmelb_AllocationHeader_t *header = (sysmelb_AllocationHeader_t*)allocation - 1;
    free(header);
}

void sysmelb_freeAll(void)
{
    sysmelb_AllocationHeader_t *position = sysmelb_LastMemoryAllocation;
    while(position)
    {
        sysmelb_LastMemoryAllocation = sysmelb_LastMemoryAllocation->nextAllocation;
        free(position);
        position = sysmelb_LastMemoryAllocation;
    }
}
