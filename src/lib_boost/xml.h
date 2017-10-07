#pragma once

#include "..\common.h"

EXPORT_CPP SClass* _makeXml(SClass* me_, const U8* path);
EXPORT_CPP SClass* _makeXmlEmpty(SClass* me_);
EXPORT_CPP void _xmlDtor(SClass* me_);
EXPORT_CPP Bool _xmlSave(SClass* me_, const U8* path);
EXPORT_CPP void _xmlHead(SClass* me_);
EXPORT_CPP Bool _xmlTerm(SClass* me_);
EXPORT_CPP void _xmlNextOffset(SClass* me_, const U8* elementPath);
EXPORT_CPP void* _xmlGet(SClass* me_);
EXPORT_CPP void* _xmlGetOffset(SClass* me_, const U8* elementPath);
EXPORT_CPP void _xmlSet(SClass* me_, const U8* value);
EXPORT_CPP void _xmlSetOffset(SClass* me_, const U8* elementPath, const U8* value);
EXPORT_CPP void _xmlAdd(SClass* me_, const U8* elementPath, const U8* value);
EXPORT_CPP S64 _xmlLen(SClass* me_);
EXPORT_CPP Bool _xmlForEach(SClass* me_, void* callback, void* data);
