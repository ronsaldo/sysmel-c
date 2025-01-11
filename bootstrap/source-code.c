#include "source-code.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

sysmelb_SourceCode_t *sysmelb_makeSourceCodeFromFileNamed(const char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    if(!file)
    {
        perror("Failed to open input file.");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *fileData = sysmelb_allocate(fileSize);
    if(fread(fileData, fileSize, 1, file) != 1)
    {
        fclose(file);
    }
    fclose(file);

    sysmelb_SourceCode_t *sourceCode = sysmelb_allocate(sizeof(sysmelb_SourceCode_t));
    sourceCode->name = fileName;
    sourceCode->text = fileData;
    sourceCode->textSize = fileSize;
    return sourceCode;
}

sysmelb_SourceCode_t *sysmelb_makeSourceCodeFromString(const char *name, const char *string)
{
    size_t stringLength = strlen(string);
    char* duplicate = sysmelb_allocate(stringLength + 1);
    memcpy(duplicate, string, stringLength);
    duplicate[stringLength] = 0;

    sysmelb_SourceCode_t *sourceCode = sysmelb_allocate(sizeof(sysmelb_SourceCode_t));
    sourceCode->name = name;
    sourceCode->text = duplicate;
    sourceCode->textSize = stringLength;
    return sourceCode;
}
