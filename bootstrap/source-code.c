#include "source-code.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

void sysmelb_splitFileName(const char *inFileName, const char **outDirectory, const char **outBasename)
{
    int32_t separatorIndex = -1;
    int32_t i;
    for (i = 0; inFileName[i] != 0; ++i)
    {
        char c = inFileName[i];
        if(c == '/' || c == '\\')
            separatorIndex = i;
    }
    int32_t stringSize = i;
    if(separatorIndex < 0)
    {
        *outDirectory = ".";
        *outBasename = inFileName;
    }


    char *directory = sysmelb_allocate(separatorIndex + 2);
    memcpy(directory, inFileName, separatorIndex + 1);

    char *basename = sysmelb_allocate(stringSize - separatorIndex + 1);
    memcpy(basename, inFileName + separatorIndex + 1, stringSize - separatorIndex);
    
    *outDirectory = directory;
    *outBasename = basename;
}

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
        perror("Failed to read input file data.");
        fclose(file);
    }
    fclose(file);

    sysmelb_SourceCode_t *sourceCode = sysmelb_allocate(sizeof(sysmelb_SourceCode_t));
    sysmelb_splitFileName(fileName, &sourceCode->directory, &sourceCode->name);
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

sysmelb_SourcePosition_t sysmelb_sourcePosition_to(sysmelb_SourcePosition_t *start, sysmelb_SourcePosition_t *end)
{
    sysmelb_SourcePosition_t merged = {
        .sourceCode  = start->sourceCode,
        .startIndex  = start->startIndex,
        .startLine   = start->startLine,
        .startColumn = start->startColumn,
        .endIndex    = end->endIndex,
        .endLine     = end->endLine,
        .endColumn   = end->endColumn
    };
    return merged;
}

sysmelb_SourcePosition_t sysmelb_sourcePosition_until(sysmelb_SourcePosition_t *start, sysmelb_SourcePosition_t *end)
{
    sysmelb_SourcePosition_t merged = {
        .sourceCode  = start->sourceCode,
        .startIndex  = start->startIndex,
        .startLine   = start->startLine,
        .startColumn = start->startColumn,
        .endIndex    = end->startIndex,
        .endLine     = end->startLine,
        .endColumn   = end->startColumn,
    };
    return merged;
}
