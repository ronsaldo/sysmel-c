#include "environment.h"
#include "error.h"
#include "function.h"
#include "memory.h"
#include "parse-tree.h"
#include "namespace.h"
#include "semantics.h"
#include "types.h"
#include "value.h"
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static sysmelb_Environment_t sysmelb_EmptyEnvironment = {
    .kind = SysmelEnvKindEmpty, .parent = NULL
};

static bool sysmelb_IntrinsicsEnvironmentCreated;
static sysmelb_Environment_t sysmelb_IntrinsicsEnvironment;

sysmelb_SymbolBinding_t *sysmelb_createSymbolArgumentBinding(uint16_t argumentIndex, sysmelb_Type_t *type)
{
    sysmelb_SymbolBinding_t *binding = sysmelb_allocate(sizeof(sysmelb_SymbolBinding_t));
    binding->kind = SysmelSymbolArgumentBinding;
    binding->argumentIndex = argumentIndex;
    binding->argumentType = type;
    return binding;
}

sysmelb_SymbolBinding_t *sysmelb_createSymbolValueBinding(sysmelb_Value_t value)
{
    sysmelb_SymbolBinding_t *binding = sysmelb_allocate(sizeof(sysmelb_SymbolBinding_t));
    binding->kind = SysmelSymbolValueBinding;
    binding->value = value;
    return binding;
}

sysmelb_SymbolBinding_t *sysmelb_createSymbolTypeBinding(sysmelb_Type_t *type)
{
    sysmelb_SymbolBinding_t *binding = sysmelb_allocate(sizeof(sysmelb_SymbolBinding_t));
    binding->kind = SysmelSymbolValueBinding;
    binding->value.kind = SysmelValueKindTypeReference;
    binding->value.type = sysmelb_getBasicTypes()->universe;
    binding->value.typeReference = type;
    return binding;
}

sysmelb_SymbolBinding_t *sysmelb_createSymbolFunctionBinding(sysmelb_function_t *function)
{
    sysmelb_SymbolBinding_t *binding = sysmelb_allocate(sizeof(sysmelb_SymbolBinding_t));
    binding->kind = SysmelSymbolValueBinding;
    binding->value.kind = SysmelValueKindFunctionReference;
    binding->value.type = sysmelb_getBasicTypes()->gradual;
    binding->value.functionReference = function;
    return binding;
}

sysmelb_SymbolBinding_t *sysmelb_environmentLookRecursively(sysmelb_Environment_t *environment, sysmelb_symbol_t *symbol)
{
    if(!environment)
        return NULL;

    const sysmelb_SymbolHashtablePair_t *lookupResult = sysmelb_SymbolHashtable_lookupSymbol(&environment->localSymbolTable, symbol);
    if(lookupResult && lookupResult->key)
        return (sysmelb_SymbolBinding_t*)lookupResult->value;

    switch(environment->kind)
    {
    case SysmelEnvKindNamespace:
        {
            sysmelb_Namespace_t *namespace = environment->ownerNamespace;
            lookupResult = sysmelb_SymbolHashtable_lookupSymbol(&namespace->exportedObjects, symbol);
            if(lookupResult && lookupResult->key)
                return (sysmelb_SymbolBinding_t*)lookupResult->value;
        }
        break;
    default:
        break;
    }

    return sysmelb_environmentLookRecursively(environment->parent, symbol);
}

sysmelb_Namespace_t *sysmelb_lookEnvironmentForNamespace(sysmelb_Environment_t *environment)
{
    if(!environment)
        return NULL;

    if(environment->kind == SysmelEnvKindNamespace)
        return environment->ownerNamespace;
    
    return sysmelb_lookEnvironmentForNamespace(environment->parent);
}

sysmelb_Environment_t *sysmelb_getEmptyEnvironment()
{
    return &sysmelb_EmptyEnvironment;
}

static sysmelb_Value_t sysmelb_ifThenElsePrimitiveMacro(sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeIfSelection, macroContext->sourcePosition);
    node->ifSelection.condition = arguments[0].parseTreeReference;
    node->ifSelection.trueExpression = arguments[1].parseTreeReference;
    node->ifSelection.falseExpression = arguments[2].parseTreeReference;
    sysmelb_Value_t result = {
        .kind = SysmelValueKindParseTreeReference,
        .parseTreeReference = node
    };
    return result;
}

static sysmelb_Value_t sysmelb_ifThenPrimitiveMacro(sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeIfSelection, macroContext->sourcePosition);
    node->ifSelection.condition = arguments[0].parseTreeReference;
    node->ifSelection.trueExpression = arguments[1].parseTreeReference;
    sysmelb_Value_t result = {
        .kind = SysmelValueKindParseTreeReference,
        .parseTreeReference = node
    };
    return result;
}

static sysmelb_Value_t sysmelb_WhileDoContinueWithPrimitiveMacro(sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeWhileLoop, macroContext->sourcePosition);
    node->whileLoop.condition = arguments[0].parseTreeReference;
    node->whileLoop.body = arguments[1].parseTreeReference;
    node->whileLoop.continueExpression = arguments[2].parseTreeReference;

    sysmelb_Value_t result = {
        .kind = SysmelValueKindParseTreeReference,
        .parseTreeReference = node
    };
    return result;
}

static sysmelb_Value_t sysmelb_WhileDoPrimitiveMacro(sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeWhileLoop, macroContext->sourcePosition);
    node->whileLoop.condition = arguments[0].parseTreeReference;
    node->whileLoop.body = arguments[1].parseTreeReference;

    sysmelb_Value_t result = {
        .kind = SysmelValueKindParseTreeReference,
        .parseTreeReference = node
    };
    return result;
}

static sysmelb_Value_t sysmelb_printLine(size_t argumentCount, sysmelb_Value_t *arguments)
{
    for(size_t i = 0; i < argumentCount; ++i)
    {
        if(arguments[i].kind == SysmelValueKindStringReference)
            printf("%.*s", (int)arguments[i].stringSize, arguments[i].string);
        else
            sysmelb_printValue(arguments[i]);
    }
        
    printf("\n");
    sysmelb_Value_t result = {
        .kind = SysmelValueKindVoid,
        .type = sysmelb_getBasicTypes()->null
    };
    return result;
}

static sysmelb_Value_t sysmelb_RecordWithFieldsMacro(sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_symbol_t *name = NULL;

    if (arguments[0].parseTreeReference->kind == ParseTreeIdentifierReference)
        name = arguments[0].parseTreeReference->identifierReference.identifier;
    else if (arguments[0].parseTreeReference->kind == ParseTreeLiteralSymbolNode)
        name = arguments[0].parseTreeReference->literalSymbol.internedSymbol;
    else
    {
        sysmelb_Value_t nameValue = sysmelb_analyzeAndEvaluateScript(macroContext->environment, arguments[0].parseTreeReference);
        if(nameValue.kind != SysmelValueKindSymbolReference)
            sysmelb_errorPrintf(macroContext->sourcePosition, "A non-valid name object is being passed.");
        name = nameValue.symbolReference;
    }

    sysmelb_Value_t dictionaryWithFieldAndType = sysmelb_analyzeAndEvaluateScript(macroContext->environment, arguments[1].parseTreeReference);
    if(dictionaryWithFieldAndType.kind != SysmelValueKindDictionaryReference)
    {
        sysmelb_errorPrintf(arguments[1].parseTreeReference->sourcePosition, "An ImmutableDictionar with field names and types is expected.");
        abort();
    }

    sysmelb_Type_t *recordType = sysmelb_allocateRecordType(name, dictionaryWithFieldAndType.dictionaryReference);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindTypeReference,
        .type = sysmelb_getBasicTypes()->universe,
        .typeReference = recordType
    };
    
    if(name)
    {
        sysmelb_SymbolBinding_t *resultBinding = sysmelb_createSymbolValueBinding(result);
        sysmelb_Environment_setLocalSymbolBinding(macroContext->environment, name, resultBinding);
    }
    
    return result;
}

static sysmelb_Value_t sysmelb_EnumWithBaseTypeAndValuesMacro(sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 3);
    sysmelb_symbol_t *name = NULL;

    if (arguments[0].parseTreeReference->kind == ParseTreeIdentifierReference)
        name = arguments[0].parseTreeReference->identifierReference.identifier;
    else if (arguments[0].parseTreeReference->kind == ParseTreeLiteralSymbolNode)
        name = arguments[0].parseTreeReference->literalSymbol.internedSymbol;
    else
    {
        sysmelb_Value_t nameValue = sysmelb_analyzeAndEvaluateScript(macroContext->environment, arguments[0].parseTreeReference);
        if(nameValue.kind != SysmelValueKindSymbolReference)
            sysmelb_errorPrintf(macroContext->sourcePosition, "A non-valid name object is being passed.");
        name = nameValue.symbolReference;
    }

    sysmelb_Value_t baseTypeValue =  sysmelb_analyzeAndEvaluateScript(macroContext->environment, arguments[1].parseTreeReference);
    if(baseTypeValue.kind != SysmelValueKindTypeReference)
    {
        sysmelb_errorPrintf(arguments[1].parseTreeReference->sourcePosition, "A base type specification is expected.");
        abort();
    }

    sysmelb_Value_t dictionaryWithFieldAndType = sysmelb_analyzeAndEvaluateScript(macroContext->environment, arguments[2].parseTreeReference);
    if(dictionaryWithFieldAndType.kind != SysmelValueKindDictionaryReference)
    {
        sysmelb_errorPrintf(arguments[2].parseTreeReference->sourcePosition, "An ImmutableDictionar with field names and types is expected.");
        abort();
    }

    sysmelb_Type_t *enumType = sysmelb_allocateEnumType(name, baseTypeValue.typeReference, dictionaryWithFieldAndType.dictionaryReference);
    sysmelb_Value_t result = {
        .kind = SysmelValueKindTypeReference,
        .type = sysmelb_getBasicTypes()->universe,
        .typeReference = enumType
    };
    
    if(name)
    {
        sysmelb_SymbolBinding_t *resultBinding = sysmelb_createSymbolValueBinding(result);
        sysmelb_Environment_setLocalSymbolBinding(macroContext->environment, name, resultBinding);
    }
    
    return result;
}

static sysmelb_Value_t sysmelb_NamespaceDefinitionMacro(sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 2);
    sysmelb_symbol_t *name = NULL;

    if (arguments[0].parseTreeReference->kind == ParseTreeIdentifierReference)
        name = arguments[0].parseTreeReference->identifierReference.identifier;
    else if (arguments[0].parseTreeReference->kind == ParseTreeLiteralSymbolNode)
        name = arguments[0].parseTreeReference->literalSymbol.internedSymbol;
    else
    {
        sysmelb_Value_t nameValue = sysmelb_analyzeAndEvaluateScript(macroContext->environment, arguments[0].parseTreeReference);
        if(nameValue.kind != SysmelValueKindSymbolReference)
            sysmelb_errorPrintf(macroContext->sourcePosition, "A non-valid name object is being passed.");
        name = nameValue.symbolReference;
    }

    sysmelb_Namespace_t *currentNamespace = sysmelb_lookEnvironmentForNamespace(macroContext->environment);
    sysmelb_Namespace_t *childNamespace = sysmelb_getOrCreateChildNamespace(currentNamespace, name);

    sysmelb_ParseTreeNode_t *node = sysmelb_newParseTreeNode(ParseTreeNamespaceDefinition, macroContext->sourcePosition);
    node->namespaceDefinition.namespace = childNamespace;
    node->namespaceDefinition.definition = arguments[1].parseTreeReference;
    
    sysmelb_Value_t result = {
        .kind = SysmelValueKindParseTreeReference,
        .type = sysmelb_getBasicTypes()->parseTreeNode,
        .parseTreeReference = node
    };
    return result;
}

static sysmelb_Value_t sysmelb_PublicMacro(sysmelb_MacroContext_t *macroContext, size_t argumentCount, sysmelb_Value_t *arguments)
{
    assert(argumentCount == 1);
    sysmelb_Value_t argument = arguments[0];
    if(argument.parseTreeReference->kind != ParseTreeArray)
    {
        sysmelb_errorPrintf(macroContext->sourcePosition, "Expected an array to public objects.");
        abort();
    }

    sysmelb_Namespace_t *ownerNamespace = sysmelb_lookEnvironmentForNamespace(macroContext->environment);
    uint32_t elementCount = argument.parseTreeReference->array.elements.size;
    sysmelb_Value_t lastElement = {
        .kind = SysmelValueKindVoid,
        .type = sysmelb_getBasicTypes()->voidType,
    };
    
    for(uint32_t i = 0; i < elementCount; ++i)
    {
        sysmelb_ParseTreeNode_t *elementToExport = argument.parseTreeReference->array.elements.elements[i];
        sysmelb_Value_t valueToExport = sysmelb_analyzeAndEvaluateScript(macroContext->environment, elementToExport);
        switch(valueToExport.kind)
        {
        case SysmelValueKindTypeReference:
        {
            if(valueToExport.typeReference->name)
                sysmelb_namespace_exportValueWithName(ownerNamespace, valueToExport.typeReference->name, &valueToExport);
        }
            break;
        case SysmelValueKindFunctionReference:
        {
            if(!valueToExport.functionReference->name)
                sysmelb_errorPrintf(elementToExport->sourcePosition, "Cannot export function without a name.");
            else
                sysmelb_namespace_exportValueWithName(ownerNamespace, valueToExport.functionReference->name, &valueToExport);
        }
            break;
        default:
            abort();
        }
        
    }

    return lastElement;

}


sysmelb_Environment_t *sysmelb_getOrCreateIntrinsicsEnvironment()
{
    if(sysmelb_IntrinsicsEnvironmentCreated)
        return &sysmelb_IntrinsicsEnvironment;

    sysmelb_IntrinsicsEnvironment.kind = SysmelEnvKindIntrinsic;
    sysmelb_IntrinsicsEnvironment.parent = sysmelb_getEmptyEnvironment();
    
    // Basic types
    {
        const sysmelb_BasicTypes_t *basicTypes = sysmelb_getBasicTypes();
        sysmelb_Type_t **basicTypeList = (sysmelb_Type_t**)basicTypes;
        size_t basicTypeListSize = sizeof(sysmelb_BasicTypes_t)/sizeof(sysmelb_Type_t*);
        for(size_t i = 0; i < basicTypeListSize; ++i)
        {
            sysmelb_Type_t *basicType = basicTypeList[i];
            //printf("Basic type %d %.*s %d %d\n", basicType->kind, basicType->name->size, basicType->name->string, basicType->valueSize, basicType->valueAlignment);
            
            sysmelb_SymbolBinding_t *symbolTypeBinding = sysmelb_createSymbolTypeBinding(basicType);
            sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, basicType->name, symbolTypeBinding);
        }
    }

    // null
    {
        sysmelb_Value_t nullValue = {
            .kind = SysmelValueKindNull,
            .type = sysmelb_getBasicTypes()->null
        };

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, sysmelb_internSymbolC("null"), sysmelb_createSymbolValueBinding(nullValue));        
    }

    // void
    {
        sysmelb_Value_t voidValue = {
            .kind = SysmelValueKindVoid,
            .type = sysmelb_getBasicTypes()->voidType
        };

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, sysmelb_internSymbolC("void"), sysmelb_createSymbolValueBinding(voidValue));        
    }

    // Boolean false
    {
        sysmelb_Value_t booleanFalse = {
            .kind = SysmelValueKindBoolean,
            .type = sysmelb_getBasicTypes()->boolean,
            .boolean = false,
        };

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, sysmelb_internSymbolC("false"), sysmelb_createSymbolValueBinding(booleanFalse));        
    }

    // Boolean true
    {
        sysmelb_Value_t booleanTrue = {
            .kind = SysmelValueKindBoolean,
            .type = sysmelb_getBasicTypes()->boolean,
            .boolean = true,
        };

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, sysmelb_internSymbolC("true"), sysmelb_createSymbolValueBinding(booleanTrue));
    }

    // If then else control flow macro
    {
        sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
        function->kind = SysmelFunctionKindPrimitiveMacro;
        function->name = sysmelb_internSymbolC("if:then:else:");
        function->primitiveMacroFunction = sysmelb_ifThenElsePrimitiveMacro;

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, function->name, sysmelb_createSymbolFunctionBinding(function));
    }

    // If then control flow macro
    {
        sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
        function->kind = SysmelFunctionKindPrimitiveMacro;
        function->name = sysmelb_internSymbolC("if:then:");
        function->primitiveMacroFunction = sysmelb_ifThenPrimitiveMacro;

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, function->name, sysmelb_createSymbolFunctionBinding(function));
    }

    // While control flow macro
    {
        sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
        function->kind = SysmelFunctionKindPrimitiveMacro;
        function->name = sysmelb_internSymbolC("while:do:continueWith:");
        function->primitiveMacroFunction = sysmelb_WhileDoContinueWithPrimitiveMacro;

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, function->name, sysmelb_createSymbolFunctionBinding(function));
    }

    {
        sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
        function->kind = SysmelFunctionKindPrimitiveMacro;
        function->name = sysmelb_internSymbolC("while:do:");
        function->primitiveMacroFunction = sysmelb_WhileDoPrimitiveMacro;

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, function->name, sysmelb_createSymbolFunctionBinding(function));
    }

    // Console printing
    {
        sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
        function->kind = SysmelFunctionKindPrimitive;
        function->name = sysmelb_internSymbolC("printLine");
        function->primitiveFunction = sysmelb_printLine;

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, function->name, sysmelb_createSymbolFunctionBinding(function));
    }

    // Record type
    {
        sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
        function->kind = SysmelFunctionKindPrimitiveMacro;
        function->name = sysmelb_internSymbolC("Record:withFields:");
        function->primitiveMacroFunction = sysmelb_RecordWithFieldsMacro;

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, function->name, sysmelb_createSymbolFunctionBinding(function));
    }

    // Enum type
    {
        sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
        function->kind = SysmelFunctionKindPrimitiveMacro;
        function->name = sysmelb_internSymbolC("Enum:withBaseType:values:");
        function->primitiveMacroFunction = sysmelb_EnumWithBaseTypeAndValuesMacro;

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, function->name, sysmelb_createSymbolFunctionBinding(function));
    }

    // namespace:definition:
    {
        sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
        function->kind = SysmelFunctionKindPrimitiveMacro;
        function->name = sysmelb_internSymbolC("namespace:definition:");
        function->primitiveMacroFunction = sysmelb_NamespaceDefinitionMacro;

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, function->name, sysmelb_createSymbolFunctionBinding(function));
    }

    // public:
    {
        sysmelb_function_t *function = sysmelb_allocate(sizeof(sysmelb_function_t));
        function->kind = SysmelFunctionKindPrimitiveMacro;
        function->name = sysmelb_internSymbolC("public:");
        function->primitiveMacroFunction = sysmelb_PublicMacro;

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, function->name, sysmelb_createSymbolFunctionBinding(function));
    }

    sysmelb_IntrinsicsEnvironmentCreated = true;
    return &sysmelb_IntrinsicsEnvironment;
}

void sysmelb_Environment_setLocalSymbolBinding(sysmelb_Environment_t *environment, sysmelb_symbol_t *name, sysmelb_SymbolBinding_t *value)
{
    sysmelb_SymbolHashtable_addSymbolWithValue(&environment->localSymbolTable, name, value);
}

sysmelb_Environment_t *sysmelb_createModuleEnvironment(sysmelb_Module_t *module, sysmelb_Environment_t *parent)
{
    sysmelb_Environment_t *environment = sysmelb_allocate(sizeof(sysmelb_Environment_t));
    environment->kind = SysmelEnvKindModule;
    environment->parent = parent;
    environment->ownerModule = module;
    return environment;
}

sysmelb_Environment_t *sysmelb_createNamespaceEnvironment(sysmelb_Namespace_t *namespace, sysmelb_Environment_t *parent)
{
    sysmelb_Environment_t *environment = sysmelb_allocate(sizeof(sysmelb_Environment_t));
    environment->kind = SysmelEnvKindNamespace;
    environment->parent = parent;
    environment->ownerNamespace = namespace;
    return environment;
}

sysmelb_Environment_t *sysmelb_createLexicalEnvironment(sysmelb_Environment_t *parent)
{
    sysmelb_Environment_t *environment = sysmelb_allocate(sizeof(sysmelb_Environment_t));
    environment->kind = SysmelEnvKindLexical;
    environment->parent = parent;
    return environment;
}

sysmelb_Environment_t *sysmelb_createFunctionAnalysisEnvironment(sysmelb_Environment_t *parent)
{
    sysmelb_Environment_t *environment = sysmelb_allocate(sizeof(sysmelb_Environment_t));
    environment->kind = SysmelEnvKindFunctionalAnalysis;
    environment->parent = parent;
    return environment;
}