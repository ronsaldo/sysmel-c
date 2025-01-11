#include "source-code.h"
#include "memory.h"
#include <stdio.h>

sysmelb_SourceCode_t *sysmelb_makeSourceCodeFromFileNamed(const char *fileName);
sysmelb_SourceCode_t *sysmelb_makeSourceCodeFromString(const char *string);