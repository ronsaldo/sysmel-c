#include "semantics.h"
#include "memory.h"

static void sysmelb_analyzeDependentArguments(sysmelb_Environment_t *environment, sysmelb_ParseTreeNode_t *ast, sysmelb_FunctionBytecode_t *bytecode)
{
    assert(ast->kind == ParseTreeFunctionalDependentType);
    size_t argumentCount = ast->functionalDependentType.argumentDefinition.size;
    bytecode->argumentCount = argumentCount;
    for(size_t i = 0; i < argumentCount; ++i)
    {
        sysmelb_ParseTreeNode_t *argument = ast->functionalDependentType.argumentDefinition.elements[i];
        assert(argument->kind == ParseTreeBindableName);

        sysmelb_Value_t argumentNameValue = sysmelb_analyzeAndEvaluateScript(environment, argument->bindableName.nameExpression);
        sysmelb_symbol_t *argumentNameSymbol = argumentNameValue.symbolReference;
        sysmelb_Type_t *argumentType = sysmelb_getBasicTypes()->gradual;
        if(argument->bindableName.typeExpression)
        {
            sysmelb_Value_t argumentTypeValue = sysmelb_analyzeAndEvaluateScript(environment, argument->bindableName.typeExpression);
            if(argumentTypeValue.kind != SysmelValueKindTypeReference)
                sysmelb_errorPrintf(argument->bindableName.typeExpression->sourcePosition, "Expected a type expression.");
            argumentType = argumentTypeValue.typeReference;
        }

        sysmelb_SymbolBinding_t *argumentBinding = sysmelb_createSymbolArgumentBinding((uint16_t)i, argumentType);
        if(argumentNameSymbol)
            sysmelb_Environment_setLocalSymbolBinding(environment, argumentNameSymbol, argumentBinding);
    }
}

sysmelb_Value_t sysmelb_analyzeAndCompileClosure(sysmelb_Environment_t *environment, sysmelb_ParseTreeNode_t *ast)
{
    assert(ast->kind == ParseTreeFunction);
    sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
    function->kind = SysmelFunctionKindInterpreted;
    sysmelb_FunctionBytecode_t *bytecode = &function->bytecode;
    
    sysmelb_Value_t functionValue = {
        .kind = SysmelValueKindFunctionReference,
        .functionReference = function,
    };

    if(ast->function.name)
        sysmelb_Environment_setLocalSymbolBinding(environment, ast->function.name, sysmelb_createSymbolValueBinding(functionValue));

    sysmelb_Environment_t *analysisEnvironment = sysmelb_createFunctionAnalysisEnvironment(environment);
    sysmelb_analyzeDependentArguments(analysisEnvironment, ast->function.functionDependentType, bytecode);
    


    return functionValue;
}