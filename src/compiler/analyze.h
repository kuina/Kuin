#pragma once

#include "..\common.h"

#include "ast.h"
#include "dict.h"
#include "option.h"

SAstFunc* Analyze(SDict* asts, const SOption* option, SDict** dlls);
void ResolveIdentifier(SDict* asts);
