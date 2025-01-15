#ifndef SYSMELB_NAMESPACE_H
#define SYSMELB_NAMESPACE_H

#pragma once

#include "symbol.h"
#include "hashtable.h"

typedef struct sysmelb_Namespace_s {
    sysmelb_symbol_t *name;
    sysmelb_SymbolHashtable_t exportedObjects;
} sysmelb_Namespace_t;

sysmelb_Namespace_t *sysmelb_createNamespaceNamed(sysmelb_symbol_t *name);
sysmelb_Namespace_t *sysmelb_getOrCreateChildNamespace(sysmelb_Namespace_t *parentNamespace, sysmelb_symbol_t *childName);

void sysmelb_namespace_exportValueWithName(sysmelb_Namespace_t *namespace, sysmelb_symbol_t *name, sysmelb_Value_t *value);
sysmelb_SymbolBinding_t *sysmelb_namespace_lookupExportedObject(sysmelb_Namespace_t *namespace, sysmelb_symbol_t *name);

#endif //SYSMELB_MODULE_H