#include "memory.h"
#include "scanner.h"
#include "parser.h"
#include "module.h"
#include "semantics.h"
#include "value.h"
#include <string.h>
#include <stdio.h>

static sysmelb_Module_t *currentModule = NULL;

void printHelp()
{
    printf("bootstrap <inputFiles>\n");
}

void printVersion()
{
    printf("bootstrap V0.1\n");
}

void scanOnlyText(const char *text)
{
    sysmelb_SourceCode_t *sourceCode = sysmelb_makeSourceCodeFromString("CLI", text);
    sysmelb_TokenDynarray_t scannedTokens = sysmelb_scanSourceCode(sourceCode);
    printf("Scanned %d tokens:", (int)scannedTokens.size);
    for(size_t i = 0; i < scannedTokens.size; ++i)
    {
        printf(" %s", sysmelb_TokenKindToString(scannedTokens.tokens[i].kind));
    }
}

void parseOnlyText(const char *text)
{
    sysmelb_SourceCode_t *sourceCode = sysmelb_makeSourceCodeFromString("CLI", text);
    sysmelb_TokenDynarray_t scannedTokens = sysmelb_scanSourceCode(sourceCode);

    sysmelb_ParseTreeNode_t *parseTree = parseTokenList(sourceCode, scannedTokens.size, scannedTokens.tokens);
    sysmelb_dumpParseTree(parseTree);
    printf("\n");
}

void evaluateText(const char *text)
{
    sysmelb_SourceCode_t *sourceCode = sysmelb_makeSourceCodeFromString("CLI", text);
    sysmelb_TokenDynarray_t scannedTokens = sysmelb_scanSourceCode(sourceCode);

    sysmelb_ParseTreeNode_t *parseTree = parseTokenList(sourceCode, scannedTokens.size, scannedTokens.tokens);
    if(sysmelb_visitForDisplayingAndCountingErrors(parseTree) != 0)
        return;

    sysmelb_Environment_t *environment = sysmelb_module_createTopLevelEnvironment(currentModule);
    sysmelb_Value_t result = sysmelb_analyzeAndEvaluateScript(environment, parseTree);
    sysmelb_printValue(result);
    printf("\n");
}

void evaluateTextFileNamed(const char *textFileName)
{
    sysmelb_SourceCode_t *sourceCode = sysmelb_makeSourceCodeFromFileNamed(textFileName);
    sysmelb_TokenDynarray_t scannedTokens = sysmelb_scanSourceCode(sourceCode);
    sysmelb_ParseTreeNode_t *parseTree = parseTokenList(sourceCode, scannedTokens.size, scannedTokens.tokens);
    if(sysmelb_visitForDisplayingAndCountingErrors(parseTree) != 0)
        return;
    
    if(!currentModule)
        currentModule = sysmelb_createModuleNamed(sysmelb_internSymbolC(sourceCode->name));
    sysmelb_Environment_t *environment = sysmelb_module_createTopLevelEnvironment(currentModule);
    sysmelb_analyzeAndEvaluateScript(environment, parseTree);
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
            else if(!strcmp(arg, "-parse-only") && i + 1 < argc)
            {
                parseOnlyText(argv[++i]);
            }
            else if(!strcmp(arg, "-module-name") && i + 1 < argc)
            {
                currentModule = sysmelb_createModuleNamed(sysmelb_internSymbolC(argv[++i]));
            }
            else if(!strcmp(arg, "-eval") && i + 1 < argc)
            {
                if(!currentModule)
                    currentModule = sysmelb_createModuleNamed(sysmelb_internSymbolC("CLI"));
                evaluateText(argv[++i]);
            }
            else if(!strcmp(arg, "--"))
            {
                size_t remainingArguments = argc - i;
                sysmelb_ArrayHeader_t *array = sysmelb_allocate(sizeof(sysmelb_ArrayHeader_t) + sizeof(sysmelb_Value_t) * remainingArguments);
                array->size = remainingArguments;
                for(size_t j = 0; j < remainingArguments; ++j)
                {
                    sysmelb_Value_t stringValue = {
                        .kind = SysmelValueKindStringReference,
                        .type = sysmelb_getBasicTypes()->string,
                        .string = argv[i + j],
                        .stringSize = strlen(argv[i + j]),
                    };

                    array->elements[j] = stringValue;
                }

                if(currentModule->mainEntryPointFunction.kind == SysmelValueKindFunctionReference)
                {
                    sysmelb_Value_t arrayArgument = {
                        .kind = SysmelValueKindArrayReference,
                        .type = sysmelb_getBasicTypes()->array,
                        .arrayReference = array,
                    };
                    sysmelb_Value_t result = sysmelb_callFunctionWithArguments(currentModule->mainEntryPointFunction.functionReference, 1, &arrayArgument);
                    return result.integer;
                }
            }
        }
        else
        {
            evaluateTextFileNamed(arg);            
        }

    }
    sysmelb_freeAll();
    return 0;
}
