#pragma once

#include "..\common.h"

EXPORT Bool _tcpConnecting(SClass* me_);
EXPORT void _tcpFin(SClass* me_);
EXPORT void* _tcpReceive(SClass* me_, S64 size);
EXPORT S64 _tcpReceivedSize(SClass* me_);
EXPORT void _tcpSend(SClass* me_, const U8* data);
EXPORT void _tcpServerFin(SClass* me_);
EXPORT SClass* _tcpServerGet(SClass* me_, SClass* me2);
EXPORT SClass* _makeTcpClient(SClass* me_, const U8* host, S64 port);
EXPORT SClass* _makeTcpServer(SClass* me_, S64 port);
