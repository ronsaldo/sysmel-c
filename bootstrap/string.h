#ifndef SYSMELB_STRING_H
#define SYSMELB_STRING_H

#include <stddef.h>

typedef struct sysmelb_dynstring_s {
    size_t capacity;
    size_t size;
    char *data;
} sysmelb_dynstring_t;

void sysmelb_dynstring_append(sysmelb_dynstring_t *dynstring, size_t textSize, const char *text);
void sysmelb_dynstring_appendCString(sysmelb_dynstring_t *dynstring, const char *text);

#endif //SYSMELB_STRING_H