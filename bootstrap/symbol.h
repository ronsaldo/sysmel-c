#ifndef SYSMELB_SYMBOL_H
#define SYSMELB_SYMBOL_H

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct sysmelb_symbol_s {
    uint32_t size;
    uint32_t hash;
    char string[];
} sysmelb_symbol_t;

uint32_t sysmelb_stringHash(size_t stringSize, const char *string);
sysmelb_symbol_t *sysmelb_internSymbol(size_t stringSize, const char *string);
sysmelb_symbol_t *sysmelb_internSymbolC(const char *string);

#endif //SYSMELB_SYMBOL_H