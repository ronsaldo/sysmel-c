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

#endif //SYSMELB_MODULE_H