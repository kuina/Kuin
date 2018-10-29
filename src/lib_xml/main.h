#pragma once

#include "..\common.h"

// 'xml'
EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags);
EXPORT_CPP SClass* _makeXml(SClass* me_, const U8* path);
EXPORT_CPP SClass* _makeXmlEmpty(SClass* me_);
EXPORT_CPP void _xmlDtor(SClass* me_);
EXPORT_CPP Bool _xmlSave(SClass* me_, const U8* path, Bool compact);
EXPORT_CPP SClass* _xmlRoot(SClass* me_, SClass* me2);
EXPORT_CPP void _xmlNodeDtor(SClass* me_);
EXPORT_CPP void _xmlNodeSetName(SClass* me_, const U8* name);
EXPORT_CPP void* _xmlNodeGetName(SClass* me_);
EXPORT_CPP void _xmlNodeSetValue(SClass* me_, const U8* value);
EXPORT_CPP void* _xmlNodeGetValue(SClass* me_);
EXPORT_CPP SClass* _xmlNodeFirstChild(SClass* me_, SClass* me2);
EXPORT_CPP SClass* _xmlNodeLastChild(SClass* me_, SClass* me2);
EXPORT_CPP SClass* _xmlNodeNext(SClass* me_, SClass* me2);
EXPORT_CPP SClass* _xmlNodePrev(SClass* me_, SClass* me2);
EXPORT_CPP SClass* _xmlNodeParent(SClass* me_, SClass* me2);
EXPORT_CPP SClass* _xmlNodeFindNext(SClass* me_, SClass* me2, const U8* name);
EXPORT_CPP SClass* _xmlNodeFindPrev(SClass* me_, SClass* me2, const U8* name);
EXPORT_CPP SClass* _xmlNodeFindChild(SClass* me_, SClass* me2, const U8* name);
EXPORT_CPP SClass* _xmlNodeFindChildLast(SClass* me_, SClass* me2, const U8* name);
EXPORT_CPP SClass* _xmlNodeAddChild(SClass* me_, SClass* me2, const U8* name);
EXPORT_CPP SClass* _xmlNodeInsChild(SClass* me_, SClass* me2, SClass* node, const U8* name);
EXPORT_CPP void _xmlNodeDelChild(SClass* me_, SClass* node);
EXPORT_CPP void _xmlNodeSetAttr(SClass* me_, const U8* attr_name, const U8* attr_value);
EXPORT_CPP void* _xmlNodeGetAttr(SClass* me_, const U8* attr_name);
