#pragma once

#include "..\common.h"

// 'kuin'
EXPORT void _init(void* heap, S64* heap_cnt);
EXPORT void _fin(void);
EXPORT void _err(const void* excpt);
EXPORT void _freeSet(void* ptr, const U8* type);
EXPORT void* _copy(const void* me_, const U8* type);
EXPORT S64 _powInt(S64 n, S64 m);
EXPORT double _powFloat(double n, double m);
EXPORT double _mod(double n, double m);
EXPORT S64 _cmpStr(const U8* a, const U8* b);
EXPORT void* _newArray(S64 len, S64* nums, const U8* type);

// Built-in methods.
EXPORT void* _toBin(const void* me_, const U8* type);
EXPORT S64 _fromBin(void** me_, const U8* type, const U8* bin, S64 idx);
EXPORT U8* _toStr(const void* me_, const U8* type);
EXPORT Bool _same(double me_, double n);
EXPORT Char _offset(Char me_, int n);
EXPORT S64 _or(const void* me_, const U8* type, const void* n);
EXPORT S64 _and(const void* me_, const U8* type, const void* n);
EXPORT S64 _xor(const void* me_, const U8* type, const void* n);
EXPORT S64 _not(const void* me_, const U8* type);
EXPORT S64 _shl(const void* me_, const U8* type, S64 n);
EXPORT S64 _shr(const void* me_, const U8* type, S64 n);
EXPORT S64 _sar(const void* me_, const U8* type, S64 n);
EXPORT void* _sub(const void* me_, const U8* type, S64 start, S64 len);
EXPORT void _reverse(void* me_, const U8* type);
EXPORT void _shuffle(void* me_, const U8* type);
EXPORT void _sort(void* me_, const U8* type);
EXPORT void _sortDesc(void* me_, const U8* type);
EXPORT S64 _find(const void* me_, const U8* type, const void* item);
EXPORT S64 _findLast(const void* me_, const U8* type, const void* item);
EXPORT S64 _toInt(const U8* me_);
EXPORT double _toFloat(const U8* me_);
EXPORT void* _lower(const U8* me_);
EXPORT void* _upper(const U8* me_);
EXPORT void* _trim(const U8* me_);
EXPORT void* _trimLeft(const U8* me_);
EXPORT void* _trimRight(const U8* me_);
EXPORT void* _split(const U8* me_, const U8* delimiter);
EXPORT void _addList(void* me_, const U8* type, const void* item);
EXPORT void _addStack(void* me_, const U8* type, const void* item);
EXPORT void _addQueue(void* me_, const U8* type, const void* item);
EXPORT void _addDict(void* me_, const U8* type, const U8* value_type, const void* key, const void* item);
EXPORT void* _getList(void* me_, const U8* type);
EXPORT void* _getStack(void* me_, const U8* type);
EXPORT void* _getQueue(void* me_, const U8* type);
EXPORT void* _getDict(void* me_, const U8* type, const void* key);
EXPORT void _head(void* me_, const U8* type);
EXPORT void _tail(void* me_, const U8* type);
EXPORT void _next(void* me_, const U8* type);
EXPORT void _prev(void* me_, const U8* type);
EXPORT Bool _term(void* me_, const U8* type);
EXPORT void _del(void* me_, const U8* type);
EXPORT void _ins(void* me_, const U8* type, const void* item);
EXPORT void* _toArray(void* me_, const U8* type);
EXPORT void* _peek(void* me_, const U8* type);

// Assembly functions.
void* ToBinClassAsm(const void* me_);
S64 FromBinClassAsm(void* me_, const U8* bin, S64 idx);
int CmpClassAsm(const void* me_, const void* target);
void DtorClassAsm(void* me_);
void* CopyClassAsm(const void* me_);
Bool AddAsm(S64* a, S64 b);
Bool MulAsm(S64* a, S64 b);
