#ifndef SYSMELB_SEMANTICS_H
#define SYSMELB_SEMANTICS_H

#include "parse-tree.h"
#include "environment.h"
#include "value.h"

sysmelb_Value_t sysmelb_analyzeAndEvaluateScript(sysmelb_Environment_t *environment, sysmelb_ParseTreeNode_t *ast);

#endif //SYSMELB_SEMANTICS_H