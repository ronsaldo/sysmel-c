#ifndef SYSMELB_ERROR_H
#define SYSMELB_ERROR_H

#pragma once

#include "source-code.h"

void sysmelb_errorPrintf(sysmelb_SourcePosition_t, const char *format, ...);

#endif //SYSMELB_ERROR_H