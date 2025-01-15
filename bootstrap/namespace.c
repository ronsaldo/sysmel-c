#include "namespace.h"
#include "environment.h"
#include "error.h"
#include "hashtable.h"
#include "memory.h"
#include "value.h"

sysmelb_Namespace_t *sysmelb_createNamespaceNamed(sysmelb_symbol_t *name)
{
    sysmelb_Namespace_t *namespace = sysmelb_allocate(sizeof(sysmelb_Namespace_t));
    namespace->name = name;
    return namespace;
}

sysmelb_Namespace_t *sysmelb_getOrCreateChildNamespace(sysmelb_Namespace_t *parentNamespace, sysmelb_symbol_t *childName)
{
    const sysmelb_SymbolHashtablePair_t *existing = sysmelb_SymbolHashtable_lookupSymbol(&parentNamespace->exportedObjects, childName);
    if(existing)
    {
        sysmelb_SymbolBinding_t *existingBinding = existing->value;
        assert(existingBinding->kind == SysmelSymbolValueBinding);

        if(existingBinding->value.kind != SysmelValueKindNamespaceReference)
        {
            sysmelb_SourcePosition_t nullPosition = {0};
            sysmelb_errorPrintf(nullPosition, "Expected a child namespace.");
        }
        return existingBinding->value.namespaceReference;
    }
    
    sysmelb_Namespace_t *childNamespace = sysmelb_allocate(sizeof(sysmelb_Namespace_t));
    childNamespace->name = childName;

    sysmelb_Value_t childValue = {
        .kind = SysmelValueKindNamespaceReference,
        .type = sysmelb_getBasicTypes()->namespace,
        .namespaceReference = childNamespace
    };

    sysmelb_SymbolBinding_t *childBinding = sysmelb_createSymbolValueBinding(childValue);

    sysmelb_SymbolHashtable_addSymbolWithValue(&parentNamespace->exportedObjects, childName, childBinding);
    return childNamespace;
}

void sysmelb_namespace_exportValueWithName(sysmelb_Namespace_t *namespace, sysmelb_symbol_t *name, sysmelb_Value_t *value)
{
    sysmelb_SymbolBinding_t *valueBinding = sysmelb_createSymbolValueBinding(*value);
    sysmelb_SymbolHashtable_addSymbolWithValue(&namespace->exportedObjects, name, valueBinding);
}

sysmelb_SymbolBinding_t *sysmelb_namespace_lookupExportedObject(sysmelb_Namespace_t *namespace, sysmelb_symbol_t *name)
{
    const sysmelb_SymbolHashtablePair_t *existing = sysmelb_SymbolHashtable_lookupSymbol(&namespace->exportedObjects, name);
    if(!existing)
        return NULL;
    return (sysmelb_SymbolBinding_t *)existing->value;
}