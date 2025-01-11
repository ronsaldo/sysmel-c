#ifndef SYSMELB_PARSER_H
#define SYSMELB_PARSER_H

#pragma once

#include "scanner.h"
#include "parse-tree.h"

sysmelb_ParseTreeNode_t *parseTokenList(size_t tokenCount, sysmelb_ScannerToken_t *tokens);

#endif // SYSMELB_PARSER_H