#include "namespace.h"
#include "hashtable.h"
#include "memory.h"

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
        return existing->value;
    
    sysmelb_Namespace_t *childNamespace = sysmelb_allocate(sizeof(sysmelb_Namespace_t));
    childNamespace->name = childName;
    sysmelb_SymbolHashtable_addSymbolWithValue(&parentNamespace->exportedObjects, childName, childNamespace);
    return childNamespace;
}