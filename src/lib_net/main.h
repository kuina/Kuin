#pragma once

#include "..\common.h"

// 'net'
EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name);
EXPORT void _fin(void);
EXPORT SClass* _makeTcpServer(SClass* me_, S64 port);
EXPORT void _tcpServerDtor(SClass* me_);
EXPORT void _tcpServerFin(SClass* me_);
EXPORT SClass* _tcpServerGet(SClass* me_, SClass* me2);
EXPORT SClass* _makeTcpClient(SClass* me_, const U8* host, S64 port);
EXPORT void _tcpDtor(SClass* me_);
EXPORT void _tcpFin(SClass* me_);
EXPORT void _tcpSend(SClass* me_, const U8* data);
EXPORT void* _tcpReceive(SClass* me_, S64 size);
EXPORT Bool _tcpConnecting(SClass* me_);
