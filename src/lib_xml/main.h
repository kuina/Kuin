#pragma once

#include "..\common.h"

// 'xml'
EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name);
EXPORT_CPP SClass* _makeXml(SClass* me_, const U8* path);
EXPORT_CPP SClass* _makeXmlEmpty(SClass* me_);
EXPORT_CPP void _xmlDtor(SClass* me_);
EXPORT_CPP Bool _xmlSave(SClass* me_, const U8* path, Bool compact);
EXPORT_CPP SClass* _xmlAddNode(SClass* me_, SClass* me2, const U8* name);
EXPORT_CPP SClass* _xmlRoot(SClass* me_, SClass* me2);
EXPORT_CPP void _xmlNodeDtor(SClass* me_);
EXPORT_CPP void* _xmlNodeName(SClass* me_);
EXPORT_CPP void* _xmlNodeValue(SClass* me_);
EXPORT_CPP SClass* _xmlNodeNext(SClass* me_, SClass* me2);
EXPORT_CPP SClass* _xmlNodePrev(SClass* me_, SClass* me2);
EXPORT_CPP SClass* _xmlNodeParent(SClass* me_, SClass* me2);
EXPORT_CPP SClass* _xmlNodeChild(SClass* me_, SClass* me2);
EXPORT_CPP SClass* _xmlNodeAddNode(SClass* me_, SClass* me2, const U8* name);
EXPORT_CPP SClass* _xmlNodeAddValue(SClass* me_, SClass* me2, const U8* value);
EXPORT_CPP void _xmlNodeSetAttr(SClass* me_, const U8* attr_name, const U8* attr_value);
EXPORT_CPP void* _xmlNodeGetAttr(SClass* me_, const U8* attr_name);
EXPORT_CPP SClass* _xmlNodeNextFind(SClass* me_, SClass* me2, const U8* name);
EXPORT_CPP SClass* _xmlNodePrevFind(SClass* me_, SClass* me2, const U8* name);
EXPORT_CPP SClass* _xmlNodeChildFind(SClass* me_, SClass* me2, const U8* name);
