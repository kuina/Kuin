#pragma once

#include "..\common.h"

typedef enum EEnv
{
	Env_Wnd = 0,
	Env_Cui = 1,
	Env_Web = 2,
} EEnv;

typedef struct SOption
{
	Bool Rls;
	const Char* IconFile;
	const Char* SysDir;
	const Char* SrcDir;
	const Char* SrcName;
	const Char* OutputFile;
	const Char* OutputDir;
	EEnv Env;
} SOption;

void MakeOption(SOption* option, const Char* path, const Char* output, const Char* sys_dir, const Char* icon, Bool rls, const Char* env);
