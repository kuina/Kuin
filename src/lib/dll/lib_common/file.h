#pragma once

#include "..\common.h"

EXPORT void _fileInit(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
EXPORT void _fileFin(void);
EXPORT Bool _copyDir(const U8* dst, const U8* src);
EXPORT Bool _copyFile(const U8* dst, const U8* src);
EXPORT Bool _delDir(const U8* path);
EXPORT Bool _delFile(const U8* path);
EXPORT Bool _existPath(const U8* path);
EXPORT Bool _forEachDir(const U8* path, Bool recursion, void* callback, void* data);
EXPORT void* _fullPath(const U8* path);
EXPORT void* _getCurDir(void);
EXPORT Bool _makeDir(const U8* path);
EXPORT Bool _moveDir(const U8* dst, const U8* src);
EXPORT Bool _moveFile(const U8* dst, const U8* src);
EXPORT void _setCurDir(const U8* path);
EXPORT void* _openAsReadingImpl(const U8* path, Bool pack, Bool* success);
EXPORT void _readerCloseImpl(void* handle);
EXPORT void _readerSeekImpl(void* handle, S64 origin, S64 pos);
EXPORT S64 _readerTellImpl(void* handle);
EXPORT Bool _readerReadImpl(void* handle, void* buf, S64 start, S64 size);
EXPORT void* _openAsWritingImpl(const U8* path, Bool append, Bool* success);
EXPORT void _writerCloseImpl(void* handle);
EXPORT void _writerFlushImpl(void* handle);
EXPORT void _writerSeekImpl(void* handle, S64 origin, S64 pos);
EXPORT S64 _writerTellImpl(void* handle);
EXPORT void _writerWriteImpl(void* handle, void* data, S64 start, S64 size);
EXPORT S64 _writerWriteNewLineImpl(void* handle);
