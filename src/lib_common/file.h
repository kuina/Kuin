#pragma once

#include "..\common.h"

// 'file'
EXPORT SClass* _makeReader(SClass* me_, const U8* path);
EXPORT SClass* _makeWriter(SClass* me_, const U8* path, Bool append);
EXPORT void _streamDtor(SClass* me_);
EXPORT void _streamFin(SClass* me_);
EXPORT void _streamSetPos(SClass* me_, S64 origin, S64 pos);
EXPORT S64 _streamGetPos(SClass* me_);
EXPORT void _streamDelimiter(SClass* me_, const U8* delimiters);
EXPORT void* _streamRead(SClass* me_, S64 size);
EXPORT Char _streamReadLetter(SClass* me_);
EXPORT S64 _streamReadInt(SClass* me_);
EXPORT double _streamReadFloat(SClass* me_);
EXPORT Char _streamReadChar(SClass* me_);
EXPORT void* _streamReadStr(SClass* me_);
EXPORT void* _streamReadLine(SClass* me_);
EXPORT void _streamWrite(SClass* me_, void* bin);
EXPORT void _streamWriteInt(SClass* me_, S64 n);
EXPORT void _streamWriteFloat(SClass* me_, double n);
EXPORT void _streamWriteChar(SClass* me_, Char n);
EXPORT void _streamWriteStr(SClass* me_, const U8* n);
EXPORT S64 _streamFileSize(SClass* me_);
EXPORT Bool _streamTerm(SClass* me_);
EXPORT Bool _makeDir(const U8* path);
EXPORT Bool _forEachDir(const U8* path, Bool recursion, void* callback, void* data);
EXPORT Bool _existPath(const U8* path);
EXPORT Bool _delDir(const U8* path);
EXPORT Bool _delFile(const U8* path);
EXPORT Bool _copyDir(const U8* dst, const U8* src);
EXPORT Bool _copyFile(const U8* dst, const U8* src);
EXPORT Bool _moveDir(const U8* dst, const U8* src);
EXPORT Bool _moveFile(const U8* dst, const U8* src);
EXPORT void* _dir(const U8* path);
EXPORT void* _ext(const U8* path);
EXPORT void* _fileName(const U8* path);
EXPORT void* _fullPath(const U8* path);
EXPORT void* _delExt(const U8* path);
EXPORT void* _tmpFile(void);
EXPORT void* _sysDir(S64 kind);
EXPORT void* _exeDir(void);
EXPORT S64 _fileSize(const U8* path);
