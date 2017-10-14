#pragma once

#include "..\common.h"

// 'xml'
EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name);
EXPORT_CPP SClass* _makeXml(SClass* me_, const U8* path);
EXPORT_CPP SClass* _makeXmlEmpty(SClass* me_);
EXPORT_CPP void _xmlDtor(SClass* me_);
EXPORT_CPP Bool _xmlSave(SClass* me_, const U8* path);
EXPORT_CPP void _xmlRoot(SClass* me_);
EXPORT_CPP Bool _xmlTerm(SClass* me_);
EXPORT_CPP void* _xmlGet(SClass* me_);
