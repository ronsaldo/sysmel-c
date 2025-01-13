#include "environment.h"
#include "function.h"
#include "memory.h"
#include "parse-tree.h"
#include "namespace.h"
#include "types.h"
#include "value.h"
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

static sysmelb_Environment_t sysmelb_EmptyEnvironment = {
    .kind = SysmelEnvKindEmpty, .parent = NULL
};

static bool sysmelb_IntrinsicsEnvironmentCreated;
static sysmelb_Environment_t sysmelb_IntrinsicsEnvironment;

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
        };

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, sysmelb_internSymbolC("null"), sysmelb_createSymbolValueBinding(nullValue));        
    }

    // void
    {
        sysmelb_Value_t voidValue = {
            .kind = SysmelValueKindVoid,
        };

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, sysmelb_internSymbolC("void"), sysmelb_createSymbolValueBinding(voidValue));        
    }

    // Boolean false
    {
        sysmelb_Value_t booleanFalse = {
            .kind = SysmelValueKindBoolean,
            .boolean = false,
        };

        sysmelb_Environment_setLocalSymbolBinding(&sysmelb_IntrinsicsEnvironment, sysmelb_internSymbolC("false"), sysmelb_createSymbolValueBinding(booleanFalse));        
    }

    // Boolean true
    {
        sysmelb_Value_t booleanTrue = {
            .kind = SysmelValueKindBoolean,
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
