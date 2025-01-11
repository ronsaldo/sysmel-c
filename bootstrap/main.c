#include "memory.h"
#include "scanner.h"
#include <string.h>
#include <stdio.h>

void printHelp()
{
    printf("bootstrap <inputFiles>\n");
}

void printVersion()
{
    printf("bootstrap <inputFiles>\n");
}

void scanOnlyText(const char *text)
{
    sysmelb_SourceCode_t *sourceCode = sysmelb_makeSourceCodeFromString("cli", text);
    sysmelb_TokenDynarray_t scannedTokens = sysmelb_scanSourceCode(sourceCode);
    printf("Scanned %d tokens:", (int)scannedTokens.size);
    for(size_t i = 0; i < scannedTokens.size; ++i)
    {
        printf(" %s", sysmelb_TokenKindToString(scannedTokens.tokens[i].kind));
    }
}

int main(int argc, const char **argv)
{
    for(int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if(*arg == '-')
        {
            if(!strcmp(arg, "-h"))
            {
                printHelp();
                return 0;
            }
            else if(!strcmp(arg, "-v"))
            {
                printVersion();
                return 0;
            }
            else if(!strcmp(arg, "-scan-only") && i + 1 < argc)
            {
                scanOnlyText(argv[++i]);
            }
        }

    }
    sysmelb_freeAll();
    return 0;
}
