#include "semantics.h"
#include "error.h"
#include "memory.h"

static void sysmelb_analyzeAndCompileClosureBody(sysmelb_Environment_t *environment, sysmelb_function_t *function, sysmelb_ParseTreeNode_t *ast);

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
    function->name = ast->function.name;
    function->kind = SysmelFunctionKindInterpreted;
    sysmelb_FunctionBytecode_t *bytecode = &function->bytecode;
    
    sysmelb_Value_t functionValue = {
        .kind = SysmelValueKindFunctionReference,
        .type = sysmelb_getBasicTypes()->function,
        .functionReference = function,
    };

    if(ast->function.name)
        sysmelb_Environment_setLocalSymbolBinding(environment, ast->function.name, sysmelb_createSymbolValueBinding(functionValue));

    sysmelb_Environment_t *analysisEnvironment = sysmelb_createFunctionAnalysisEnvironment(environment);
    sysmelb_analyzeDependentArguments(analysisEnvironment, ast->function.functionDependentType, bytecode);
    
    sysmelb_analyzeAndCompileClosureBody(analysisEnvironment, function, ast->function.bodyExpression);
    sysmelb_bytecode_return(&function->bytecode);
    return functionValue;
}

static void sysmelb_analyzeAndCompileClosureBody(sysmelb_Environment_t *environment, sysmelb_function_t *function, sysmelb_ParseTreeNode_t *ast)
{
    switch(ast->kind)
    {
    case ParseTreeLiteralIntegerNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindInteger,
                .type = sysmelb_getBasicTypes()->integer,
                .integer = ast->literalInteger.value
            };
            return sysmelb_bytecode_pushLiteral(&function->bytecode, &value);
        }
    case ParseTreeLiteralCharacterNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindCharacter,
                .type = sysmelb_getBasicTypes()->character,
                .unsignedInteger = ast->literalCharacter.value
            };
            return sysmelb_bytecode_pushLiteral(&function->bytecode, &value);
        }
    case ParseTreeLiteralFloatNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindFloatingPoint,
                .type = sysmelb_getBasicTypes()->floatingPoint,
                .floatingPoint = ast->literalFloat.value
            };
            return sysmelb_bytecode_pushLiteral(&function->bytecode, &value);
        }
    case ParseTreeLiteralStringNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindStringReference,
                .type = sysmelb_getBasicTypes()->string,
                .string = ast->literalString.string,
                .stringSize = ast->literalString.stringSize,
            };
            return sysmelb_bytecode_pushLiteral(&function->bytecode, &value);
        }
    case ParseTreeLiteralSymbolNode:
        {
            sysmelb_Value_t value = {
                .kind = SysmelValueKindSymbolReference,
                .type = sysmelb_getBasicTypes()->symbol,
                .symbolReference = ast->literalSymbol.internedSymbol
            };
            return sysmelb_bytecode_pushLiteral(&function->bytecode, &value);
        }
    case ParseTreeLiteralValueNode:
        return sysmelb_bytecode_pushLiteral(&function->bytecode, &ast->literalValue.value);

    case ParseTreeIdentifierReference:
        {
            sysmelb_SymbolBinding_t *binding = sysmelb_environmentLookRecursively(environment, ast->identifierReference.identifier);
            if(!binding)
                sysmelb_errorPrintf(ast->sourcePosition, "Failed to find binding.");
            switch(binding->kind)
            {
            case SysmelSymbolValueBinding:
                return sysmelb_bytecode_pushLiteral(&function->bytecode, &binding->value);
            case SysmelSymbolArgumentBinding:
                return sysmelb_bytecode_pushArgument(&function->bytecode, binding->argumentIndex);
            case SysmelSymbolCaptureBinding:
                return sysmelb_bytecode_pushCapture(&function->bytecode, binding->captureIndex);
            case SysmelSymbolTemporaryBinding:
                return sysmelb_bytecode_pushTemporary(&function->bytecode, binding->temporaryIndex);
            }
            abort();
        }
    case ParseTreeFunctionApplication:
        {
            if(ast->functionApplication.functional->kind == ParseTreeIdentifierReference)
            {
                sysmelb_SymbolBinding_t *functionalBinding = sysmelb_environmentLookRecursively(environment, ast->functionApplication.functional->identifierReference.identifier);
                if(!functionalBinding)
                    sysmelb_errorPrintf(ast->functionApplication.functional->sourcePosition, "Failed to find identifier.");

                if(functionalBinding->kind == SysmelSymbolValueBinding && functionalBinding->value.kind == SysmelValueKindFunctionReference)
                {
                    sysmelb_function_t *calledFunctionOrMacro = functionalBinding->value.functionReference;
                    switch(calledFunctionOrMacro->kind)
                    {
                    case SysmelFunctionKindPrimitiveMacro:
                    {
                        size_t argumentCount = ast->functionApplication.arguments.size;
                        assert(ast->functionApplication.arguments.size <= SYSMEL_MAX_ARGUMENT_COUNT);
                        sysmelb_Value_t applicationArguments[SYSMEL_MAX_ARGUMENT_COUNT];

                        for(size_t i = 0; i < argumentCount; ++i)
                        {
                            sysmelb_Value_t argumentValue = {
                                .kind = SysmelValueKindParseTreeReference,
                                .parseTreeReference = ast->functionApplication.arguments.elements[i],
                            };

                            applicationArguments[i] = argumentValue;
                        }
                        
                        sysmelb_MacroContext_t macroContext = {
                            .sourcePosition = ast->sourcePosition,
                            .environment = environment,
                        };

                        sysmelb_Value_t macroResult = calledFunctionOrMacro->primitiveMacroFunction(&macroContext, argumentCount, applicationArguments);
                        if(macroResult.kind == SysmelValueKindParseTreeReference)
                        {
                            return sysmelb_analyzeAndCompileClosureBody(environment, function, macroResult.parseTreeReference);
                        }
                        else
                        {
                            return sysmelb_bytecode_pushLiteral(&function->bytecode, &macroResult);
                        }
                    }
                    default:
                        break;
                    }
                }
            }
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->functionApplication.functional);
            size_t argumentCount = ast->functionApplication.arguments.size;
            for(size_t i = 0; i < argumentCount; ++i)
                sysmelb_analyzeAndCompileClosureBody(environment, function, ast->functionApplication.arguments.elements[i]);
            
            sysmelb_bytecode_applyFunction(&function->bytecode, (uint16_t)argumentCount);
            return;
        }
    case ParseTreeMessageSend:
        {
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->messageSend.receiver);
            sysmelb_Value_t selectorValue = sysmelb_analyzeAndEvaluateScript(environment, ast->messageSend.selector);
            assert(selectorValue.kind == SysmelValueKindSymbolReference);
            
            size_t argumentCount = ast->messageSend.arguments.size;
            for(size_t i = 0; i < argumentCount; ++i)
                sysmelb_analyzeAndCompileClosureBody(environment, function, ast->messageSend.arguments.elements[i]);

            sysmelb_bytecode_sendMessage(&function->bytecode, selectorValue.symbolReference, (uint16_t)argumentCount);
            return;
            
        }
    case ParseTreeBinaryOperatorSequence:
        {
            //TODO: use an operator precedence parser.
            sysmelb_ParseTreeNode_t *receiver = ast->binaryOperatorSequence.elements.elements[0];
            for(size_t i = 1; i < ast->binaryOperatorSequence.elements.size; i += 2)
            {
                sysmelb_ParseTreeNode_t *selector = ast->binaryOperatorSequence.elements.elements[i];
                sysmelb_ParseTreeNode_t *operand = ast->binaryOperatorSequence.elements.elements[i + 1];

                sysmelb_ParseTreeNode_t *binaryMessage = sysmelb_newParseTreeNode(ParseTreeMessageSend, selector->sourcePosition);
                binaryMessage->messageSend.receiver = receiver;
                binaryMessage->messageSend.selector = selector;
                sysmelb_ParseTreeNodeDynArray_add(&binaryMessage->messageSend.arguments, operand);
                receiver = binaryMessage;
            }
            return sysmelb_analyzeAndCompileClosureBody(environment, function, receiver);
        }

    // Lexical block
    case ParseTreeLexicalBlock:
    {
        sysmelb_Environment_t *blockEnvironment = sysmelb_createLexicalEnvironment(environment);
        return sysmelb_analyzeAndCompileClosureBody(blockEnvironment, function, ast->lexicalBlock.expression);
    }

    // Control flow
    case ParseTreeIfSelection:
    {
        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->ifSelection.condition);
        uint16_t ifFalseJump = sysmelb_bytecode_jumpIfFalse(&function->bytecode);

        if(ast->ifSelection.trueExpression)
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->ifSelection.trueExpression);
        uint16_t mergeJump = sysmelb_bytecode_jump(&function->bytecode);

        sysmelb_bytecode_patchJumpToHere(&function->bytecode, ifFalseJump);
        if(ast->ifSelection.falseExpression)
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->ifSelection.falseExpression);        
        sysmelb_bytecode_patchJumpToHere(&function->bytecode, mergeJump);
        return;
    }
    default:
        abort();
    }

}