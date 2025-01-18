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
    case ParseTreeErrorNode:
        sysmelb_errorPrintf(ast->sourcePosition, "%s", ast->errorNode.errorMessage);
        abort();
    case ParseTreeAssertNode:
        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->assertNode.condition);
        return sysmelb_bytecode_assert(&function->bytecode, ast->sourcePosition);
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
            {
                sysmelb_errorPrintf(ast->sourcePosition, "Failed to find binding for #%.*s .", ast->identifierReference.identifier->size, ast->identifierReference.identifier->string);
                abort();
            }
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
                {
                    sysmelb_errorPrintf(ast->functionApplication.functional->sourcePosition, "Failed to find identifier.");
                    abort();
                }

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
            sysmelb_Value_t selectorValue = sysmelb_analyzeAndEvaluateScript(environment, ast->messageSend.selector);
            assert(selectorValue.kind == SysmelValueKindSymbolReference);
            if(ast->messageSend.arguments.size == 1)
            {
                if(selectorValue.symbolReference == sysmelb_internSymbolC("&&"))
                {
                    sysmelb_Value_t falseValue = {
                        .kind = SysmelValueKindBoolean,
                        .type = sysmelb_getBasicTypes()->boolean,
                        .boolean = false,
                    };

                    sysmelb_ParseTreeNode_t *falseLiteralResult = sysmelb_newParseTreeNode(ParseTreeLiteralValueNode, ast->sourcePosition);
                    falseLiteralResult->literalValue.value = falseValue;

                    sysmelb_ParseTreeNode_t *ifNode = sysmelb_newParseTreeNode(ParseTreeIfSelection, ast->sourcePosition);
                    ifNode->ifSelection.condition = ast->messageSend.receiver;
                    ifNode->ifSelection.trueExpression = ast->messageSend.arguments.elements[0];
                    ifNode->ifSelection.falseExpression = falseLiteralResult;
                    return sysmelb_analyzeAndCompileClosureBody(environment, function, ifNode);

                }
                else if(selectorValue.symbolReference == sysmelb_internSymbolC("||"))
                {
                    sysmelb_Value_t trueValue = {
                        .kind = SysmelValueKindBoolean,
                        .type = sysmelb_getBasicTypes()->boolean,
                        .boolean = true,
                    };

                    sysmelb_ParseTreeNode_t *trueLiteralResult = sysmelb_newParseTreeNode(ParseTreeLiteralValueNode, ast->sourcePosition);
                    trueLiteralResult->literalValue.value = trueValue;

                    sysmelb_ParseTreeNode_t *ifNode = sysmelb_newParseTreeNode(ParseTreeIfSelection, ast->sourcePosition);
                    ifNode->ifSelection.condition = ast->messageSend.receiver;
                    ifNode->ifSelection.trueExpression = trueLiteralResult;
                    ifNode->ifSelection.falseExpression = ast->messageSend.arguments.elements[0];
                    return sysmelb_analyzeAndCompileClosureBody(environment, function, ifNode);
                }
            }

            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->messageSend.receiver);
            
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
    // Sequences, array, tuples
    case ParseTreeSequence:
        {
            size_t elementCount = ast->sequence.elements.size;
            if(elementCount == 0)
            {
                sysmelb_Value_t null = {
                    .type = sysmelb_getBasicTypes()->null
                };
                sysmelb_bytecode_pushLiteral(&function->bytecode, &null);
            }
            for(size_t i = 0; i < elementCount; ++i)
            {
                sysmelb_analyzeAndCompileClosureBody(environment, function, ast->sequence.elements.elements[i]);
                if(i + 1 < elementCount)
                    sysmelb_bytecode_pop(&function->bytecode);
            }
            return;
        }
    case ParseTreeArray:
        for(size_t i = 0; i < ast->array.elements.size; ++i)
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->array.elements.elements[i]);
        return sysmelb_bytecode_makeArray(&function->bytecode, ast->array.elements.size);
    case ParseTreeTuple:
        for(size_t i = 0; i < ast->tuple.elements.size; ++i)
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->tuple.elements.elements[i]);
        return sysmelb_bytecode_makeArray(&function->bytecode, ast->tuple.elements.size);
        
    // Association, dictionary
    case ParseTreeAssociation:
    {
        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->association.key);
        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->association.value);
        return sysmelb_bytecode_makeAssociation(&function->bytecode);
    }
    case ParseTreeImmutableDictionary:
    {
        for(size_t i = 0; i < ast->dictionary.elements.size; ++i)
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->dictionary.elements.elements[i]);
        return sysmelb_bytecode_makeImmutableDictionary(&function->bytecode, ast->dictionary.elements.size);
    }

    // Lexical block
    case ParseTreeLexicalBlock:
    {
        sysmelb_Environment_t *blockEnvironment = sysmelb_createLexicalEnvironment(environment);
        return sysmelb_analyzeAndCompileClosureBody(blockEnvironment, function, ast->lexicalBlock.expression);
    }

    // Assignment.
    case ParseTreeAssignment:
        sysmelb_ParseTreeNode_t *store = ast->assignment.store;
        sysmelb_ParseTreeNode_t *value = ast->assignment.value;
        if(store->kind == ParseTreeBindableName)
        {   
            bool isAnonymous = !store->bindableName.nameExpression;
            sysmelb_Value_t nameValue = {};
            
            if(!isAnonymous)
            {
                nameValue = sysmelb_analyzeAndEvaluateScript(environment, store->bindableName.nameExpression);
                if(nameValue.kind != SysmelValueKindSymbolReference)
                    sysmelb_errorPrintf(store->bindableName.nameExpression->sourcePosition, "Expected a name");
            }

            /*if(store->bindableName.hasPostTypeExpression && store->bindableName.typeExpression->kind == ParseTreeFunctionalDependentType)
            {
                // TODO: Support closure captures.
                abort();
                sysmelb_ParseTreeNode_t *functionNode = sysmelb_newParseTreeNode(ParseTreeFunction, ast->sourcePosition);
                functionNode->function.functionDependentType = store->bindableName.typeExpression;
                functionNode->function.bodyExpression = value;
                if(!isAnonymous)
                    functionNode->function.name = nameValue.symbolReference;
                return sysmelb_analyzeAndCompileClosure(environment, functionNode);
            }*/

            // Store the initial value.
            sysmelb_analyzeAndCompileClosureBody(environment, function, value);
            uint16_t temporaryIndex = sysmelb_bytecode_allocateTemporary(&function->bytecode);
            sysmelb_bytecode_storeTemporary(&function->bytecode, temporaryIndex);
            sysmelb_Environment_setLocalSymbolBinding(environment, nameValue.symbolReference, sysmelb_createSymbolTemporaryBinding(temporaryIndex, sysmelb_getBasicTypes()->gradual));
            return;
        }
        if(store->kind == ParseTreeIdentifierReference)
        {
            sysmelb_SymbolBinding_t *binding = sysmelb_environmentLookRecursively(environment, store->identifierReference.identifier);
            if(!binding)
            {
                sysmelb_errorPrintf(store->sourcePosition, "Failed to find binding for %.*s", store->identifierReference.identifier->size, store->identifierReference.identifier->string);
                abort();
            }
            if(binding->kind == SysmelSymbolTemporaryBinding)
            {
                sysmelb_analyzeAndCompileClosureBody(environment, function, value);
                sysmelb_bytecode_storeTemporary(&function->bytecode, binding->temporaryIndex);
                return;
            }
        }


        abort();

    // Control flow
    case ParseTreeIfSelection:
    {
        sysmelb_Value_t voidValue = {
            .kind = SysmelValueKindVoid,
            .type = sysmelb_getBasicTypes()->voidType
        };
        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->ifSelection.condition);
        uint16_t ifFalseJump = sysmelb_bytecode_jumpIfFalse(&function->bytecode);

        if(ast->ifSelection.trueExpression)
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->ifSelection.trueExpression);
        else
            sysmelb_bytecode_pushLiteral(&function->bytecode, &voidValue);
        uint16_t mergeJump = sysmelb_bytecode_jump(&function->bytecode);

        sysmelb_bytecode_patchJumpToHere(&function->bytecode, ifFalseJump);
        if(ast->ifSelection.falseExpression)
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->ifSelection.falseExpression);        
        else
            sysmelb_bytecode_pushLiteral(&function->bytecode, &voidValue);

        sysmelb_bytecode_patchJumpToHere(&function->bytecode, mergeJump);
        return;
    }
    case ParseTreeSwitch:
    {
        bool isCaseWithJump[256];
        uint16_t casesJumps[256];
        uint16_t casesMergeJumps[256];
        uint16_t defaultCaseMergeJump;
        memset(isCaseWithJump, 0, sizeof(isCaseWithJump));

        assert(ast->switchExpression.cases->kind == ParseTreeImmutableDictionary);
        sysmelb_ParseTreeImmutableDictionary_t *dictionary = &ast->switchExpression.cases->dictionary;
        size_t caseCount = dictionary->elements.size;
        assert(caseCount <= 256);
        
        sysmelb_ParseTreeNode_t *defaultCase = NULL;

        // Compile the value expression, and store it in a temporary.
        uint16_t valueTemporary = sysmelb_bytecode_allocateTemporary(&function->bytecode);
        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->switchExpression.value);
        sysmelb_bytecode_popAndStoreTemporary(&function->bytecode, valueTemporary);

        // First pass. Generate a series of jump if true.
        for(size_t i = 0; i < caseCount; ++i)
        {
            assert(dictionary->elements.elements[i]->kind == ParseTreeAssociation);
            sysmelb_ParseTreeAssociation_t *caseAssoc = &dictionary->elements.elements[i]->association;
            sysmelb_Value_t caseKeyValue = sysmelb_analyzeAndEvaluateScript(environment, caseAssoc->key);
            
            if(caseKeyValue.kind == SysmelValueKindSymbolReference)
            {
                assert(caseKeyValue.symbolReference->size == 1 && caseKeyValue.symbolReference->string[0] == '_');
                defaultCase = caseAssoc->value;

            }
            else if(caseKeyValue.kind == SysmelValueKindInteger || caseKeyValue.kind == SysmelValueKindUnsignedInteger)
            {
                sysmelb_bytecode_pushLiteral(&function->bytecode, &caseKeyValue);
                sysmelb_bytecode_pushTemporary(&function->bytecode, valueTemporary);
                sysmelb_bytecode_integerEquals(&function->bytecode);
                casesJumps[i] = sysmelb_bytecode_jumpIfTrue(&function->bytecode);
                isCaseWithJump[i] = true;
            }
        }

        // Generate the default case
        if(defaultCase)
        {
            sysmelb_analyzeAndCompileClosureBody(environment, function, defaultCase);
            defaultCaseMergeJump = sysmelb_bytecode_jump(&function->bytecode);
        }
        else
        {
            // Emit void to balance the results.
            sysmelb_Value_t voidValue = {
                .kind = SysmelValueKindVoid,
                .type = sysmelb_getBasicTypes()->voidType
            };
            sysmelb_bytecode_pushLiteral(&function->bytecode, &voidValue);
            defaultCaseMergeJump = sysmelb_bytecode_jump(&function->bytecode);
        }

        // Second pass. Generate a series of jump if true.
        for(size_t i = 0; i < caseCount; ++i)
        {
            if(!isCaseWithJump[i])
                continue;

            sysmelb_bytecode_patchJumpToHere(&function->bytecode, casesJumps[i]);
            
            assert(dictionary->elements.elements[i]->kind == ParseTreeAssociation);
            sysmelb_ParseTreeAssociation_t *caseAssoc = &dictionary->elements.elements[i]->association;
            sysmelb_Environment_t *lexicalEnvironment = sysmelb_createLexicalEnvironment(environment);
            sysmelb_analyzeAndCompileClosureBody(lexicalEnvironment, function, caseAssoc->value);

            casesMergeJumps[i] = sysmelb_bytecode_jump(&function->bytecode);
        }

        // Merge everything.
        sysmelb_bytecode_patchJumpToHere(&function->bytecode, defaultCaseMergeJump);
        for(size_t i = 0; i < caseCount; ++i)
        {
            if(isCaseWithJump[i])
                sysmelb_bytecode_patchJumpToHere(&function->bytecode, casesMergeJumps[i]);
        }
        return;
    }
    case ParseTreeSwitchPatternMatching:
    {
        bool isCaseWithJump[256];
        uint16_t casesJumps[256];
        uint16_t casesMergeJumps[256];
        uint16_t defaultCaseMergeJump;
        memset(isCaseWithJump, 0, sizeof(isCaseWithJump));

        assert(ast->switchPatternMatching.cases->kind == ParseTreeImmutableDictionary);
        sysmelb_ParseTreeImmutableDictionary_t *dictionary = &ast->switchPatternMatching.cases->dictionary;
        size_t caseCount = dictionary->elements.size;
        assert(caseCount <= 256);
        
        sysmelb_ParseTreeNode_t *defaultCase = NULL;

        // Evaluate the sum type.
        sysmelb_Value_t sumTypeValue = sysmelb_analyzeAndEvaluateScript(environment, ast->switchPatternMatching.valueSumType);
        assert(sumTypeValue.kind == SysmelValueKindTypeReference && sumTypeValue.typeReference->kind == SysmelTypeKindSum);
        sysmelb_Type_t *sumType = sumTypeValue.typeReference;

        // Compile the value expression, and store it in a temporary.
        uint16_t valueTemporary = sysmelb_bytecode_allocateTemporary(&function->bytecode);
        uint16_t indexTemporary = sysmelb_bytecode_allocateTemporary(&function->bytecode);

        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->switchExpression.value);
        sysmelb_bytecode_storeTemporary(&function->bytecode, valueTemporary);

        sysmelb_bytecode_getSumIndex(&function->bytecode);
        sysmelb_bytecode_popAndStoreTemporary(&function->bytecode, indexTemporary);


        // First pass. Generate a series of jump if true.
        for(size_t i = 0; i < caseCount; ++i)
        {
            assert(dictionary->elements.elements[i]->kind == ParseTreeAssociation);
            sysmelb_ParseTreeAssociation_t *caseAssoc = &dictionary->elements.elements[i]->association;
            if (caseAssoc->key->kind == ParseTreeBindableName)
            {
                sysmelb_ParseTreeNode_t *bindableName = caseAssoc->key;
                assert(bindableName->bindableName.typeExpression);
                sysmelb_Value_t bindableTypeValue = sysmelb_analyzeAndEvaluateScript(environment, bindableName->bindableName.typeExpression);
                sysmelb_Type_t *bindableType = bindableTypeValue.typeReference;
                int alternativeIndex = sysmelb_findSumTypeIndexForType(sumType, bindableType);
                if(alternativeIndex < 0)
                {
                    sysmelb_errorPrintf(bindableName->sourcePosition, "Pattern does not match anything.");
                    abort();
                }

                sysmelb_Value_t alternativeIndexValue = {
                    .kind = SysmelValueKindInteger,
                    .type = sysmelb_getBasicTypes()->integer,
                    .integer = alternativeIndex,
                };

                sysmelb_bytecode_pushLiteral(&function->bytecode, &alternativeIndexValue);
                sysmelb_bytecode_pushTemporary(&function->bytecode, indexTemporary);
                sysmelb_bytecode_integerEquals(&function->bytecode);
                casesJumps[i] = sysmelb_bytecode_jumpIfTrue(&function->bytecode);
                isCaseWithJump[i] = true;
                continue;
            }
            else if(caseAssoc->key->kind == ParseTreeLiteralSymbolNode)
            {
                sysmelb_ParseTreeNode_t *keyNode = caseAssoc->key;
                assert(keyNode->literalSymbol.internedSymbol->size == 1 && keyNode->literalSymbol.internedSymbol->string[0] == '_');
                defaultCase = caseAssoc->value;
                continue;
            }

        }

        // Generate the default case
        if(defaultCase)
        {
            sysmelb_analyzeAndCompileClosureBody(environment, function, defaultCase);
            defaultCaseMergeJump = sysmelb_bytecode_jump(&function->bytecode);
        }
        else
        {
            // Emit void to balance the results.
            sysmelb_Value_t voidValue = {
                .kind = SysmelValueKindVoid,
                .type = sysmelb_getBasicTypes()->voidType
            };
            sysmelb_bytecode_pushLiteral(&function->bytecode, &voidValue);
            defaultCaseMergeJump = sysmelb_bytecode_jump(&function->bytecode);
        }

        // Second pass. Generate the body of the different alternatives.
        for(size_t i = 0; i < caseCount; ++i)
        {
            if(!isCaseWithJump[i])
                continue;

            sysmelb_bytecode_patchJumpToHere(&function->bytecode, casesJumps[i]);
            
            assert(dictionary->elements.elements[i]->kind == ParseTreeAssociation);
            sysmelb_ParseTreeAssociation_t *caseAssoc = &dictionary->elements.elements[i]->association;
            sysmelb_Environment_t *lexicalEnvironment = sysmelb_createLexicalEnvironment(environment);
            if (caseAssoc->key->kind == ParseTreeBindableName && caseAssoc->key->bindableName.nameExpression && caseAssoc->key->bindableName.typeExpression)
            {
                sysmelb_Value_t bindableNameValue = sysmelb_analyzeAndEvaluateScript(lexicalEnvironment, caseAssoc->key->bindableName.nameExpression);
                sysmelb_Value_t bindableNameType = sysmelb_analyzeAndEvaluateScript(lexicalEnvironment, caseAssoc->key->bindableName.typeExpression);
                if(bindableNameValue.kind == SysmelValueKindSymbolReference)
                {
                    sysmelb_symbol_t *bindableSymbol = bindableNameValue.symbolReference;
                    sysmelb_bytecode_pushTemporary(&function->bytecode, valueTemporary);
                    sysmelb_bytecode_getSumInjectedValue(&function->bytecode);
                    
                    uint16_t caseTemporary = sysmelb_bytecode_allocateTemporary(&function->bytecode);
                    sysmelb_bytecode_popAndStoreTemporary(&function->bytecode, caseTemporary);

                    sysmelb_SymbolBinding_t *caseBinding = sysmelb_createSymbolTemporaryBinding(caseTemporary, bindableNameType.typeReference);
                    sysmelb_Environment_setLocalSymbolBinding(lexicalEnvironment, bindableSymbol, caseBinding);
                }
            }
            
            if(caseAssoc->value)
            {
                sysmelb_analyzeAndCompileClosureBody(lexicalEnvironment, function, caseAssoc->value);
            }
            else
            {
                sysmelb_Value_t voidValue = {
                    .kind = SysmelValueKindVoid,
                    .type = sysmelb_getBasicTypes()->voidType
                };
                sysmelb_bytecode_pushLiteral(&function->bytecode, &voidValue);
            }

            casesMergeJumps[i] = sysmelb_bytecode_jump(&function->bytecode);
        }

        // Merge everything.
        sysmelb_bytecode_patchJumpToHere(&function->bytecode, defaultCaseMergeJump);
        for(size_t i = 0; i < caseCount; ++i)
        {
            if(isCaseWithJump[i])
                sysmelb_bytecode_patchJumpToHere(&function->bytecode, casesMergeJumps[i]);
        }
        return;
    }
    case ParseTreeWhileLoop:
    {
        // Header
        uint16_t loopHeader = sysmelb_bytecode_label(&function->bytecode);
        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->whileLoop.condition);
        uint16_t conditionFalseJump = sysmelb_bytecode_jumpIfFalse(&function->bytecode);
    
        // Body
        if(ast->whileLoop.body)
        {
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->whileLoop.body);
            sysmelb_bytecode_pop(&function->bytecode);
        }
        
        // Continue with
        if(ast->whileLoop.continueExpression)
        {
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->whileLoop.continueExpression);
            sysmelb_bytecode_pop(&function->bytecode);
        }
        
        uint16_t backJump = sysmelb_bytecode_jump(&function->bytecode);
        sysmelb_bytecode_patchJumpToLabel(&function->bytecode, backJump, loopHeader);
        sysmelb_bytecode_patchJumpToHere(&function->bytecode, conditionFalseJump);
        
        sysmelb_Value_t voidValue = {
            .kind = SysmelValueKindVoid,
            .type = sysmelb_getBasicTypes()->voidType
        };
        return sysmelb_bytecode_pushLiteral(&function->bytecode, &voidValue);
    }

case ParseTreeDoWhileLoop:
    {
        // Header
        uint16_t loopHeader = sysmelb_bytecode_label(&function->bytecode);
    
        // Body
        if(ast->doWhileLoop.body)
        {
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->doWhileLoop.body);
            sysmelb_bytecode_pop(&function->bytecode);
        }
        
        // Continue with
        if(ast->doWhileLoop.continueExpression)
        {
            sysmelb_analyzeAndCompileClosureBody(environment, function, ast->doWhileLoop.continueExpression);
            sysmelb_bytecode_pop(&function->bytecode);
        }

        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->doWhileLoop.condition);
        uint16_t conditionBackJump = sysmelb_bytecode_jumpIfTrue(&function->bytecode);
        sysmelb_bytecode_patchJumpToLabel(&function->bytecode, conditionBackJump, loopHeader);
        
        sysmelb_Value_t voidValue = {
            .kind = SysmelValueKindVoid,
            .type = sysmelb_getBasicTypes()->voidType
        };
        return sysmelb_bytecode_pushLiteral(&function->bytecode, &voidValue);
    }
case ParseTreeReturnValue:
    {
        sysmelb_analyzeAndCompileClosureBody(environment, function, ast->returnExpression.valueExpression);
        return sysmelb_bytecode_return(&function->bytecode);
    }
    default:
        abort();
    }

}