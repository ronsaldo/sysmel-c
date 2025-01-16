#include "error.h"
#include <stdarg.h>
#include <stdio.h>

void sysmelb_errorPrintf(sysmelb_SourcePosition_t sourcePosition, const char *format, ...)
{
    va_list args;
    va_start (args, format);

    if(sourcePosition.sourceCode)
    {
        if(sourcePosition.sourceCode->directory)
            fprintf(stderr, "%s%s", sourcePosition.sourceCode->directory, sourcePosition.sourceCode->name);
        else
            fprintf(stderr, "%s", sourcePosition.sourceCode->name);
    }

    fprintf(stderr, ":%d.%d-%d.%d: ", sourcePosition.startLine, sourcePosition.startColumn, sourcePosition.endLine, sourcePosition.endColumn);
    vfprintf(stderr, format, args);
    va_end (args);
}