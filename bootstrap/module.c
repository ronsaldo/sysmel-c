#include "module.h"
#include "namespace.h"
#include "environment.h"

sysmelb_Module_t *sysmelb_createModuleNamed(sysmelb_symbol_t *name)
{
    sysmelb_Module_t *module = sysmelb_allocate(sizeof(sysmelb_Module_t));
    memset(module, 0, sizeof(sysmelb_Module_t));

    module->name = name;
    module->moduleEnvironment = sysmelb_createModuleEnvironment(module, sysmelb_getOrCreateIntrinsicsEnvironment());
    module->globalNamespace = sysmelb_createNamespaceNamed(sysmelb_internSymbolC("__global"));
    module->globalNamespaceEnvironment = sysmelb_createNamespaceEnvironment(module->globalNamespace, module->moduleEnvironment);
    return module;
}

sysmelb_Environment_t *sysmelb_module_createTopLevelEnvironment(sysmelb_Module_t *module)
{
    return sysmelb_createLexicalEnvironment(module->globalNamespaceEnvironment);
}