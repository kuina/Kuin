#pragma once

#include "..\common.h"

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
EXPORT void _fin(void);
EXPORT void _err(S64 excpt);
#if defined(DBG)
EXPORT void _pause(void);
#endif
EXPORT void _freeSet(void* ptr, const U8* type);
EXPORT void* _copy(const void* me_, const U8* type);
EXPORT void* _toBin(const void* me_, const U8* type, const void* root);
EXPORT void* _fromBin(const U8* me_, const U8* type, S64* idx, const void* root);
EXPORT S64 _powInt(S64 a, S64 b);
EXPORT double _powFloat(double n, double m);
EXPORT double _mod(double n, double m);
EXPORT S64 _cmpStr(const U8* a, const U8* b);
EXPORT void* _newArray(S64 len, S64* nums, const U8* type);
EXPORT U8* _dictValueType(const U8* type);

// Built-in methods.
EXPORT void _addDict(void* me_, const U8* type, const void* key, const void* item);
EXPORT void _addList(void* me_, const U8* type, const void* item);
EXPORT void _addQueue(void* me_, const U8* type, const void* item);
EXPORT void _addStack(void* me_, const U8* type, const void* item);
EXPORT S64 _and(const void* me_, const U8* type, const void* n);
EXPORT void _del(void* me_, const U8* type);
EXPORT void _delDict(void* me_, const U8* type, const void* key);
EXPORT void _delNext(void* me_, const U8* type);
EXPORT S64 _endian(const void* me_, const U8* type);
EXPORT Bool _exist(void* me_, const U8* type, const void* key);
EXPORT void _fill(void* me_, const U8* type, const void* value);
EXPORT S64 _findArray(const void* me_, const U8* type, const void* item, S64 start);
EXPORT S64 _findBin(const void* me_, const U8* type, const void* item);
EXPORT S64 _findLastArray(const void* me_, const U8* type, const void* item, S64 start);
EXPORT Bool _findLastList(const void* me_, const U8* type, const void* item);
EXPORT Bool _findList(const void* me_, const U8* type, const void* item);
EXPORT Bool _forEach(void* me_, const U8* type, const void* callback, void* data);
EXPORT void* _getDict(void* me_, const U8* type, const void* key, Bool* existed);
EXPORT void* _getList(void* me_, const U8* type);
EXPORT void* _getOffset(void* me_, const U8* type, S64 offset);
EXPORT SClass* _getPtr(void* me_, const U8* type, SClass* me2);
EXPORT void* _getQueue(void* me_, const U8* type);
EXPORT void* _getStack(void* me_, const U8* type);
EXPORT void _head(void* me_, const U8* type);
EXPORT S64 _idx(void* me_, const U8* type);
EXPORT void _ins(void* me_, const U8* type, const void* item);
EXPORT void* _max(const void* me_, const U8* type);
EXPORT void* _min(const void* me_, const U8* type);
EXPORT void _moveOffset(void* me_, const U8* type, S64 offset);
EXPORT Bool _nan(double me_);
EXPORT void _next(void* me_, const U8* type);
EXPORT S64 _not(const void* me_, const U8* type);
EXPORT S64 _or(const void* me_, const U8* type, const void* n);
EXPORT void* _peek(void* me_, const U8* type);
EXPORT void _prev(void* me_, const U8* type);
EXPORT void* _repeat(const void* me_, const U8* type, S64 len);
EXPORT void _reverse(void* me_, const U8* type);
EXPORT S64 _sar(const void* me_, const U8* type, S64 n);
EXPORT void _setPtr(void* me_, const U8* type, SClass* ptr);
EXPORT S64 _shl(const void* me_, const U8* type, S64 n);
EXPORT S64 _shr(const void* me_, const U8* type, S64 n);
EXPORT void _sort(void* me_, const U8* type);
EXPORT void* _sub(const void* me_, const U8* type, S64 start, S64 len);
EXPORT void _tail(void* me_, const U8* type);
EXPORT Bool _term(void* me_, const U8* type);
EXPORT Bool _termOffset(void* me_, const U8* type, S64 offset);
EXPORT void* _toArray(void* me_, const U8* type);
EXPORT U64 _toBit64(const U8* me_, Bool* success);
EXPORT double _toFloat(const U8* me_, Bool* success);
EXPORT S64 _toInt(const U8* me_, Bool* success);
EXPORT U8* _toStr(const void* me_, const U8* type);
EXPORT S64 _xor(const void* me_, const U8* type, const void* n);
