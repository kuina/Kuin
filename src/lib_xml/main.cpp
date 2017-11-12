// LibXml.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include "tinyxml2/tinyxml2.h"

typedef struct SXml
{
	SClass Class;
	tinyxml2::XMLDocument* Tree;
} SXml;

typedef struct SXmlNode
{
	SClass Class;
	tinyxml2::XMLNode* Node;
} SXmlNode;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* app_name)
{
	if (Heap != NULL)
		return;
	Heap = heap;
	HeapCnt = heap_cnt;
	AppCode = app_code;
	AppName = app_name == NULL ? L"Untitled" : (Char*)(app_name + 0x10);
	Instance = (HINSTANCE)GetModuleHandle(NULL);
}

EXPORT_CPP SClass* _makeXml(SClass* me_, const U8* path)
{
	THROWDBG(path == NULL, 0xc0000005);
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	FILE* file_ptr = _wfopen(path2, L"rb");
	if (file_ptr == NULL)
		return NULL;
	me2->Tree = static_cast<tinyxml2::XMLDocument*>(AllocMem(sizeof(tinyxml2::XMLDocument)));
	new(me2->Tree)tinyxml2::XMLDocument(true, tinyxml2::COLLAPSE_WHITESPACE);
	tinyxml2::XMLError result = me2->Tree->LoadFile(file_ptr);
	fclose(file_ptr);
	if (result != tinyxml2::XML_SUCCESS)
	{
		me2->Tree->~XMLDocument();
		FreeMem(me2->Tree);
		return NULL;
	}
	ASSERT(me2->Tree->FirstChild() != NULL);
	return me_;
}

EXPORT_CPP SClass* _makeXmlEmpty(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	me2->Tree = static_cast<tinyxml2::XMLDocument*>(AllocMem(sizeof(tinyxml2::XMLDocument)));
	new(me2->Tree)tinyxml2::XMLDocument(true, tinyxml2::COLLAPSE_WHITESPACE);
	me2->Tree->InsertEndChild(me2->Tree->NewDeclaration());
	return me_;
}

EXPORT_CPP void _xmlDtor(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	if (me2->Tree != NULL)
	{
		me2->Tree->~XMLDocument();
		FreeMem(me2->Tree);
	}
}

EXPORT_CPP Bool _xmlSave(SClass* me_, const U8* path, Bool compact)
{
	THROWDBG(path == NULL, 0xc0000005);
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	FILE* file_ptr = _wfopen(path2, L"wb");
	if (file_ptr == NULL)
		return False;
	tinyxml2::XMLError result = me2->Tree->SaveFile(file_ptr, compact != False);
	fclose(file_ptr);
	return result == tinyxml2::XML_SUCCESS;
}

EXPORT_CPP SClass* _xmlAddNode(SClass* me_, SClass* me2, const U8* name)
{
	SXml* me3 = reinterpret_cast<SXml*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	THROWDBG(name == NULL, 0xc0000005);
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
	{
		THROWDBG(True, 0xe9170006);
		return NULL;
	}
	me4->Node = me3->Tree->InsertEndChild(me3->Tree->NewElement(buf));
	FreeMem(buf);
	return me2;
}

EXPORT_CPP SClass* _xmlRoot(SClass* me_, SClass* me2)
{
	SXml* me3 = reinterpret_cast<SXml*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	me4->Node = me3->Tree->FirstChild()->NextSibling();
	if (me4->Node == NULL)
		return NULL;
	return me2;
}

EXPORT_CPP void _xmlNodeDtor(SClass* me_)
{
	// Do nothing.
	UNUSED(me_);
}

EXPORT_CPP void* _xmlNodeName(SClass* me_)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	return Utf8ToUtf16(me2->Node->ToElement()->Name());
}

EXPORT_CPP void* _xmlNodeValue(SClass* me_)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	return Utf8ToUtf16(me2->Node->ToElement()->GetText());
}

EXPORT_CPP SClass* _xmlNodeNext(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	me4->Node = me3->Node->NextSibling();
	if (me4->Node == NULL)
		return NULL;
	return me2;
}

EXPORT_CPP SClass* _xmlNodePrev(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	me4->Node = me3->Node->PreviousSibling();
	if (me4->Node == NULL)
		return NULL;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeParent(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	me4->Node = me3->Node->Parent();
	if (me4->Node == NULL)
		return NULL;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeChild(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	me4->Node = me3->Node->FirstChild();
	if (me4->Node == NULL)
		return NULL;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeAddNode(SClass* me_, SClass* me2, const U8* name)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	THROWDBG(name == NULL, 0xc0000005);
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
	{
		THROWDBG(True, 0xe9170006);
		return NULL;
	}
	me4->Node = me3->Node->InsertEndChild(me3->Node->GetDocument()->NewElement(buf));
	FreeMem(buf);
	return me2;
}

EXPORT_CPP SClass* _xmlNodeAddValue(SClass* me_, SClass* me2, const U8* value)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	THROWDBG(value == NULL, 0xc0000005);
	char* buf = Utf16ToUtf8(value);
	if (buf == NULL)
	{
		THROWDBG(True, 0xe9170006);
		return NULL;
	}
	me4->Node = me3->Node->InsertEndChild(me3->Node->GetDocument()->NewText(buf));
	FreeMem(buf);
	return me2;
}

EXPORT_CPP void _xmlNodeSetAttr(SClass* me_, const U8* attr_name, const U8* attr_value)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	THROWDBG(attr_name == NULL, 0xc0000005);
	char* buf_name = Utf16ToUtf8(attr_name);
	if (buf_name == NULL)
		THROWDBG(True, 0xe9170006);
	char* buf_value = Utf16ToUtf8(attr_value);
	me2->Node->ToElement()->SetAttribute(buf_name, buf_value);
	FreeMem(buf_name);
	if (buf_value != NULL)
		FreeMem(buf_value);
}

EXPORT_CPP void* _xmlNodeGetAttr(SClass* me_, const U8* attr_name)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	THROWDBG(attr_name == NULL, 0xc0000005);
	char* buf = Utf16ToUtf8(attr_name);
	if (buf == NULL)
		THROWDBG(True, 0xe9170006);
	const char* str = me2->Node->ToElement()->Attribute(buf);
	FreeMem(buf);
	return Utf8ToUtf16(str);
}

EXPORT_CPP SClass* _xmlNodeNextFind(SClass* me_, SClass* me2, const U8* name)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	THROWDBG(name == NULL, 0xc0000005);
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
		THROWDBG(True, 0xe9170006);
	me4->Node = me3->Node->NextSiblingElement(buf);
	FreeMem(buf);
	if (me4->Node == NULL)
		return NULL;
	return me2;
}

EXPORT_CPP SClass* _xmlNodePrevFind(SClass* me_, SClass* me2, const U8* name)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	THROWDBG(name == NULL, 0xc0000005);
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
		THROWDBG(True, 0xe9170006);
	me4->Node = me3->Node->PreviousSiblingElement(buf);
	FreeMem(buf);
	if (me4->Node == NULL)
		return NULL;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeChildFind(SClass* me_, SClass* me2, const U8* name)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	THROWDBG(name == NULL, 0xc0000005);
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
		THROWDBG(True, 0xe9170006);
	me4->Node = me3->Node->FirstChildElement(buf);
	FreeMem(buf);
	if (me4->Node == NULL)
		return NULL;
	return me2;
}
