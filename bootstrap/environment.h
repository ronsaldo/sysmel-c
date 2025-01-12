#ifndef SYSMELB_ENVIRONMENT_H
#define SYSMELB_ENVIRONMENT_H

#pragma once

typedef struct sysmelb_Environment_s sysmelb_Environment_t;

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
};

#endif //SYSMELB_ENVIRONMENT_H