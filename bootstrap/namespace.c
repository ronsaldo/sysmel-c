#include "namespace.h"
#include "memory.h"

sysmelb_Namespace_t *sysmelb_createNamespaceNamed(sysmelb_symbol_t *name)
{
    sysmelb_Namespace_t *namespace =sysmelb_allocate(sizeof(sysmelb_Namespace_t));
    memset(namespace, 0, sizeof(sysmelb_Namespace_t));
    namespace->name = name;
    return namespace;
}