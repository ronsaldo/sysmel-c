#ifndef SYSMELB_MEMORY_H
#define SYSMELB_MEMORY_H

#pragma once

#include <stddef.h>

void *sysmelb_allocate(size_t allocationSize);
void sysmelb_freeAllocation(void *allocation);
void sysmelb_freeAll(void);

#endif //SYSMELB_MEMORY_H