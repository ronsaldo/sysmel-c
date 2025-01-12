#ifndef SYSMELB_MODULE_H
#define SYSMELB_MODULE_H

#pragma once

#include "symbol.h"

typedef struct sysmelb_Environment_s sysmelb_Environment_t;
typedef struct sysmelb_Namespace_s sysmelb_Namespace_t;
typedef struct sysmelb_Module_s
{
    sysmelb_symbol_t *name;
    sysmelb_Namespace_t *globalNamespace;

    sysmelb_Environment_t *moduleEnvironment;
    sysmelb_Environment_t *globalNamespaceEnvironment;
    
}sysmelb_Module_t;

sysmelb_Module_t *sysmelb_createModuleNamed(sysmelb_symbol_t *name);
sysmelb_Environment_t *sysmelb_module_createTopLevelEnvironment(sysmelb_Module_t *module);

#endif //SYSMELB_MODULE_H