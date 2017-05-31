#pragma once

#include "..\common.h"

// 'kuin'
EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name);
EXPORT void _fin(void);
EXPORT void _err(const void* excpt);
EXPORT void _freeSet(void* ptr, const U8* type);
EXPORT void* _copy(const void* me_, const U8* type);
EXPORT void* _toBin(const void* me_, const U8* type);
EXPORT void* _fromBin(const U8* me_, const void** type, S64* idx);
EXPORT S64 _powInt(S64 n, S64 m);
EXPORT double _powFloat(double n, double m);
EXPORT double _mod(double n, double m);
EXPORT S64 _cmpStr(const U8* a, const U8* b);
EXPORT void* _newArray(S64 len, S64* nums, const U8* type);

// Built-in methods.
EXPORT U8* _toStr(const void* me_, const U8* type);
EXPORT S64 _absInt(S64 me_);
EXPORT double _absFloat(double me_);
EXPORT S64 _clampInt(S64 me_, S64 min, S64 max);
EXPORT double _clampFloat(double me_, double min, double max);
EXPORT S64 _clampMinInt(S64 me_, S64 min);
EXPORT double _clampMinFloat(double me_, double min);
EXPORT S64 _clampMaxInt(S64 me_, S64 max);
EXPORT double _clampMaxFloat(double me_, double max);
EXPORT Bool _same(double me_, double n);
EXPORT Char _offset(Char me_, int n);
EXPORT S64 _or(const void* me_, const U8* type, const void* n);
EXPORT S64 _and(const void* me_, const U8* type, const void* n);
EXPORT S64 _xor(const void* me_, const U8* type, const void* n);
EXPORT S64 _not(const void* me_, const U8* type);
EXPORT S64 _shl(const void* me_, const U8* type, S64 n);
EXPORT S64 _shr(const void* me_, const U8* type, S64 n);
EXPORT S64 _sar(const void* me_, const U8* type, S64 n);
EXPORT S64 _endian(const void* me_, const U8* type);
EXPORT void* _sub(const void* me_, const U8* type, S64 start, S64 len);
EXPORT void _reverse(void* me_, const U8* type);
EXPORT void _shuffle(void* me_, const U8* type);
EXPORT void _sort(void* me_, const U8* type);
EXPORT void _sortDesc(void* me_, const U8* type);
EXPORT S64 _find(const void* me_, const U8* type, const void* item);
EXPORT S64 _findLast(const void* me_, const U8* type, const void* item);
EXPORT S64 _findBin(const void* me_, const U8* type, const void* item);
EXPORT void _fill(void* me_, const U8* type, const void* value);
EXPORT S64 _toInt(const U8* me_);
EXPORT double _toFloat(const U8* me_);
EXPORT void* _lower(const U8* me_);
EXPORT void* _upper(const U8* me_);
EXPORT void* _trim(const U8* me_);
EXPORT void* _trimLeft(const U8* me_);
EXPORT void* _trimRight(const U8* me_);
EXPORT void* _split(const U8* me_, const U8* delimiter);
EXPORT void* _join(const U8* me_, const U8* delimiter);
EXPORT void* _replace(const U8* me_, Char old, Char new_);
EXPORT S64 _cmp(const U8* me_, const U8* target);
EXPORT void _addList(void* me_, const U8* type, const void* item);
EXPORT void _addStack(void* me_, const U8* type, const void* item);
EXPORT void _addQueue(void* me_, const U8* type, const void* item);
EXPORT void _addDict(void* me_, const U8* type, const U8* value_type, const void* key, const void* item);
EXPORT void* _getList(void* me_, const U8* type);
EXPORT void* _getStack(void* me_, const U8* type);
EXPORT void* _getQueue(void* me_, const U8* type);
EXPORT void* _getDict(void* me_, const U8* type, const void* key);
EXPORT void* _getOffset(void* me_, const U8* type, S64 offset);
EXPORT void _head(void* me_, const U8* type);
EXPORT void _tail(void* me_, const U8* type);
EXPORT void _next(void* me_, const U8* type);
EXPORT void _prev(void* me_, const U8* type);
EXPORT void _moveOffset(void* me_, const U8* type, S64 offset);
EXPORT Bool _term(void* me_, const U8* type);
EXPORT Bool _termOffset(void* me_, const U8* type, S64 offset);
EXPORT void _del(void* me_, const U8* type);
EXPORT void _delNext(void* me_, const U8* type);
EXPORT void _ins(void* me_, const U8* type, const void* item);
EXPORT void* _toArray(void* me_, const U8* type);
EXPORT void* _peek(void* me_, const U8* type);
EXPORT Bool _exist(void* me_, const U8* type, const void* key);
EXPORT void _forEach(void* me_, const U8* type, const void* callback);

// Assembly functions.
void* ToBinClassAsm(const void* me_);
void* FromBinClassAsm(const void* me_, const U8* bin, S64* idx);
int CmpClassAsm(const void* me_, const void* target);
void DtorClassAsm(void* me_);
void* CopyClassAsm(const void* me_);
Bool AddAsm(S64* a, S64 b);
Bool MulAsm(S64* a, S64 b);
void* Call0Asm(void* func);
void* Call1Asm(void* arg1, void* func);
void* Call2Asm(void* arg1, void* arg2, void* func);
void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);
