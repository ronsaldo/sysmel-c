#ifndef SYSMELB_SOURCE_CODE_H
#define SYSMELB_SOURCE_CODE_H

#pragma once

typedef struct sysmelb_SourceCode_s
{
    const char *directory;
    const char *name;
    const char *text;
    size_t textSize;
}sysmelb_SourceCode_t;

typedef struct sysmelb_SourcePosition_s
{
    sysmelb_SourceCode_t *sourceCode;
    int startIndex;
    int endIndex;
    int startLine;
    int endLine;
    int startColumn;
    int endColumn;
}sysmelb_SourcePosition_t;

sysmelb_SourceCode_t *sysmelb_makeSourceCodeFromFileNamed(const char *fileName);
sysmelb_SourceCode_t *sysmelb_makeSourceCodeFromString(const char *string);

#endif //SYSMELB_SOURCE_CODE_H