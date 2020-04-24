#pragma once

#include "..\common.h"

// 'file'
EXPORT SClass* _makeReader(SClass* me_, const U8* path);
EXPORT SClass* _makeWriter(SClass* me_, const U8* path, Bool append);
EXPORT void _readerDtor(SClass* me_);
EXPORT void _readerFin(SClass* me_);
EXPORT void _readerSetPos(SClass* me_, S64 origin, S64 pos);
EXPORT S64 _readerGetPos(SClass* me_);
EXPORT void _readerDelimiter(SClass* me_, const U8* delimiters);
EXPORT void* _readerRead(SClass* me_, S64 size);
EXPORT Char _readerReadLetter(SClass* me_);
EXPORT S64 _readerReadInt(SClass* me_);
EXPORT double _readerReadFloat(SClass* me_);
EXPORT Char _readerReadChar(SClass* me_);
EXPORT void* _readerReadStr(SClass* me_);
EXPORT void* _readerReadLine(SClass* me_);
EXPORT S64 _readerFileSize(SClass* me_);
EXPORT Bool _readerTerm(SClass* me_);
EXPORT void _writerDtor(SClass* me_);
EXPORT void _writerFin(SClass* me_);
EXPORT void _writerSetPos(SClass* me_, S64 origin, S64 pos);
EXPORT S64 _writerGetPos(SClass* me_);
EXPORT void _writerWrite(SClass* me_, void* bin);
EXPORT void _writerWriteInt(SClass* me_, S64 n);
EXPORT void _writerWriteFloat(SClass* me_, double n);
EXPORT void _writerWriteChar(SClass* me_, Char n);
EXPORT void _writerWriteStr(SClass* me_, const U8* n);
EXPORT void _writerFlush(SClass* me_);
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
EXPORT void _setCurDir(const U8* path);
EXPORT void* _getCurDir(void);
