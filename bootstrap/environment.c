#include "environment.h"
#include "memory.h"
#include "types.h"
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

sysmelb_SymbolBinding_t *sysmelb_createSymbolTypeBinding(sysmelb_Type_t *type)
{
    sysmelb_SymbolBinding_t *binding = sysmelb_allocate(sizeof(sysmelb_SymbolBinding_t));
    memset(binding, 0, sizeof(sysmelb_SymbolBinding_t));
    binding->kind = SysmelSymbolTypeBinding;
    binding->type = type;
    return binding;
}

sysmelb_Environment_t *sysmelb_getEmptyEnvironment()
{
    return &sysmelb_EmptyEnvironment;
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
    memset(environment, 0, sizeof(sysmelb_Environment_t));
    environment->kind = SysmelEnvKindModule;
    environment->parent = parent;
    environment->ownerModule = module;
    return environment;
}

sysmelb_Environment_t *sysmelb_createNamespaceEnvironment(sysmelb_Namespace_t *namespace, sysmelb_Environment_t *parent)
{
    sysmelb_Environment_t *environment = sysmelb_allocate(sizeof(sysmelb_Environment_t));
    memset(environment, 0, sizeof(sysmelb_Environment_t));
    environment->kind = SysmelEnvKindNamespace;
    environment->parent = parent;
    environment->ownerNamespace = namespace;
    return environment;
}

sysmelb_Environment_t *sysmelb_createLexicalEnvironment(sysmelb_Environment_t *parent)
{
    sysmelb_Environment_t *environment = sysmelb_allocate(sizeof(sysmelb_Environment_t));
    memset(environment, 0, sizeof(sysmelb_Environment_t));
    environment->kind = SysmelEnvKindLexical;
    environment->parent = parent;
    return environment;
}
