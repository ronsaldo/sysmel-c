#ifndef SYSMELB_ENVIRONMENT_H
#define SYSMELB_ENVIRONMENT_H

#pragma once

#include "hashtable.h"
#include "value.h"

typedef struct sysmelb_Environment_s sysmelb_Environment_t;
typedef struct sysmelb_Module_s sysmelb_Module_t;
typedef struct sysmelb_Namespace_s sysmelb_Namespace_t;
typedef struct sysmelb_Type_s sysmelb_Type_t;

typedef enum sysmelb_EnvironmentKind_e {
    SysmelEnvKindEmpty,
    SysmelEnvKindIntrinsic,
    SysmelEnvKindModule,
    SysmelEnvKindNamespace,
    SysmelEnvKindLexical,
    SysmelEnvKindFunctionalAnalysis,
    SysmelEnvKindFunctionalActivation,
} sysmelb_EnvironmentKind_t;

struct sysmelb_Environment_s {
    sysmelb_EnvironmentKind_t kind;
    sysmelb_Environment_t *parent;
    sysmelb_SymbolHashtable_t localSymbolTable;
    
    union {
        sysmelb_Module_t *ownerModule;
        sysmelb_Namespace_t *ownerNamespace;
    };
};

typedef enum sysmelb_SymbolBindingKind_e
{
    SysmelSymbolValueBinding,
} sysmelb_SymbolBindingKind_t;

typedef struct sysmelb_SymbolBinding_s
{
    sysmelb_SymbolBindingKind_t kind;
    
    union {
        sysmelb_Value_t value;
    };
} sysmelb_SymbolBinding_t;

sysmelb_SymbolBinding_t *sysmelb_createSymbolTypeBinding(sysmelb_Type_t *type);

sysmelb_SymbolBinding_t *sysmelb_environmentLookRecursively(sysmelb_Environment_t *environment, sysmelb_symbol_t *symbol);

sysmelb_Environment_t *sysmelb_getEmptyEnvironment();
sysmelb_Environment_t *sysmelb_getOrCreateIntrinsicsEnvironment();
void sysmelb_Environment_setLocalSymbolBinding(sysmelb_Environment_t *environment, sysmelb_symbol_t *name, sysmelb_SymbolBinding_t *value);

sysmelb_Environment_t *sysmelb_createModuleEnvironment(sysmelb_Module_t *module, sysmelb_Environment_t *parent);
sysmelb_Environment_t *sysmelb_createNamespaceEnvironment(sysmelb_Namespace_t *namespace, sysmelb_Environment_t *parent);
sysmelb_Environment_t *sysmelb_createLexicalEnvironment(sysmelb_Environment_t *parent);

#endif //SYSMELB_ENVIRONMENT_H