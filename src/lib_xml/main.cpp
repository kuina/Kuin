// LibXml.dll
//
// (C)Kuina-chan
//

// I modified the following points in the 'tinyxml2' library:
// - I inserted a space before the closing symbol of empty tags.
// - Instead of four spaces in the indent I changed it to a tab character.

#include "main.h"

#include "tinyxml2/tinyxml2.h"

struct SXml
{
	SClass Class;
	tinyxml2::XMLDocument* Tree;
};

struct SXmlNode
{
	SClass Class;
	Bool Root;
	void* Node;
};

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	if (!InitEnvVars(heap, heap_cnt, app_code, use_res_flags))
		return;
}

EXPORT_CPP SClass* _makeXml(SClass* me_, const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	size_t size;
	void* buf = LoadFileAll(path2, &size);
	if (buf == NULL)
		return NULL;
	me2->Tree = static_cast<tinyxml2::XMLDocument*>(AllocMem(sizeof(tinyxml2::XMLDocument)));
	new(me2->Tree)tinyxml2::XMLDocument(true, tinyxml2::PRESERVE_WHITESPACE);
	tinyxml2::XMLError result = me2->Tree->Parse(static_cast<char*>(buf), size);
	FreeMem(buf);
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
	new(me2->Tree)tinyxml2::XMLDocument(true, tinyxml2::PRESERVE_WHITESPACE);
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
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	FILE* file_ptr = _wfopen(path2, L"wb");
	if (file_ptr == NULL)
		return False;
	tinyxml2::XMLError result = me2->Tree->SaveFile(file_ptr, compact != False);
	fclose(file_ptr);
	return result == tinyxml2::XML_SUCCESS;
}

EXPORT_CPP SClass* _xmlRoot(SClass* me_, SClass* me2)
{
	SXml* me3 = reinterpret_cast<SXml*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	me4->Root = True;
	me4->Node = me3->Tree;
	return me2;
}

EXPORT_CPP void _xmlNodeDtor(SClass* me_)
{
	// Do nothing.
	UNUSED(me_);
}

EXPORT_CPP void _xmlNodeSetName(SClass* me_, const U8* name)
{
	THROWDBG(name == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return;
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
	{
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
	}
	static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->SetName(buf);
	FreeMem(buf);
}

EXPORT_CPP void* _xmlNodeGetName(SClass* me_)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return NULL;
	return Utf8ToUtf16(static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->Name());
}

EXPORT_CPP void _xmlNodeSetValue(SClass* me_, const U8* value)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return;
	if (value == NULL)
		static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->SetText("");
	else
	{
		char* buf = Utf16ToUtf8(value);
		if (buf == NULL)
		{
			THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
		}
		static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->SetText(buf);
		FreeMem(buf);
	}
}

EXPORT_CPP void* _xmlNodeGetValue(SClass* me_)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return NULL;
	return Utf8ToUtf16(static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->GetText());
}

EXPORT_CPP SClass* _xmlNodeFirstChild(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		me4->Node = static_cast<tinyxml2::XMLDocument*>(me3->Node)->FirstChild();
	else
		me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->FirstChild();
	if (me4->Node == NULL)
		return NULL;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeLastChild(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		me4->Node = static_cast<tinyxml2::XMLDocument*>(me3->Node)->LastChild();
	else
		me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->LastChild();
	if (me4->Node == NULL)
		return NULL;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeNext(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return NULL;
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->NextSibling();
	if (me4->Node == NULL)
		return NULL;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodePrev(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return NULL;
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->PreviousSibling();
	if (me4->Node == NULL)
		return NULL;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeParent(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return NULL;
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->Parent();
	if (me4->Node == NULL || static_cast<tinyxml2::XMLNode*>(me4->Node)->Parent() == NULL)
	{
		me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->ToDocument();
		me4->Root = True;
	}
	else
		me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeFindNext(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return NULL;
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->NextSiblingElement(buf);
	FreeMem(buf);
	if (me4->Node == NULL)
		return NULL;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeFindPrev(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return NULL;
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->PreviousSiblingElement(buf);
	FreeMem(buf);
	if (me4->Node == NULL)
		return NULL;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeFindChild(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
	if (me3->Root)
		me4->Node = static_cast<tinyxml2::XMLDocument*>(me3->Node)->FirstChildElement(buf);
	else
		me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->FirstChildElement(buf);
	FreeMem(buf);
	if (me4->Node == NULL)
		return NULL;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeFindChildLast(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
	if (me3->Root)
		me4->Node = static_cast<tinyxml2::XMLDocument*>(me3->Node)->LastChildElement(buf);
	else
		me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->LastChildElement(buf);
	FreeMem(buf);
	if (me4->Node == NULL)
		return NULL;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeAddChild(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
	if (me3->Root)
	{
		tinyxml2::XMLDocument* node = static_cast<tinyxml2::XMLDocument*>(me3->Node);
		me4->Node = node->InsertEndChild(node->NewElement(buf));
	}
	else
	{
		tinyxml2::XMLNode* node = static_cast<tinyxml2::XMLNode*>(me3->Node);
		me4->Node = node->InsertEndChild(node->GetDocument()->NewElement(buf));
	}
	FreeMem(buf);
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeInsChild(SClass* me_, SClass* me2, SClass* node, const U8* name)
{
	THROWDBG(node == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(name == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	SXmlNode* node2 = reinterpret_cast<SXmlNode*>(node);
	if (node2->Root)
		return NULL;
	char* buf = Utf16ToUtf8(name);
	if (buf == NULL)
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
	if (me3->Root)
	{
		tinyxml2::XMLDocument* node3 = static_cast<tinyxml2::XMLDocument*>(me3->Node);
		if (static_cast<tinyxml2::XMLNode*>(node2->Node)->Parent() == node3)
		{
			tinyxml2::XMLNode* node4 = static_cast<tinyxml2::XMLNode*>(node2->Node)->PreviousSibling();
			if (node4 == NULL)
				me4->Node = node3->InsertFirstChild(node3->GetDocument()->NewElement(buf));
			else
				me4->Node = node3->InsertAfterChild(node4, node3->GetDocument()->NewElement(buf));
		}
	}
	else
	{
		tinyxml2::XMLNode* node3 = static_cast<tinyxml2::XMLNode*>(me3->Node);
		if (static_cast<tinyxml2::XMLNode*>(node2->Node)->Parent() == node3)
		{
			tinyxml2::XMLNode* node4 = static_cast<tinyxml2::XMLNode*>(node2->Node)->PreviousSibling();
			if (node4 == NULL)
				me4->Node = node3->InsertFirstChild(node3->GetDocument()->NewElement(buf));
			else
				me4->Node = node3->InsertAfterChild(node4, node3->GetDocument()->NewElement(buf));
		}
	}
	FreeMem(buf);
	me4->Root = False;
	return me2;
}

EXPORT_CPP void _xmlNodeDelChild(SClass* me_, SClass* node)
{
	THROWDBG(node == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* node2 = reinterpret_cast<SXmlNode*>(node);
	if (node2->Root)
		return;
	if (me3->Root)
	{
		tinyxml2::XMLDocument* node3 = static_cast<tinyxml2::XMLDocument*>(me3->Node);
		if (static_cast<tinyxml2::XMLNode*>(node2->Node)->Parent() == node3)
		{
			tinyxml2::XMLNode* node4 = static_cast<tinyxml2::XMLNode*>(node2->Node);
			node3->DeleteChild(node4);
		}
	}
	else
	{
		tinyxml2::XMLNode* node3 = static_cast<tinyxml2::XMLNode*>(me3->Node);
		if (static_cast<tinyxml2::XMLNode*>(node2->Node)->Parent() == node3)
		{
			tinyxml2::XMLNode* node4 = static_cast<tinyxml2::XMLNode*>(node2->Node);
			node3->DeleteChild(node4);
		}
	}
}

EXPORT_CPP void _xmlNodeSetAttr(SClass* me_, const U8* attr_name, const U8* attr_value)
{
	THROWDBG(attr_name == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return;
	char* buf_name = Utf16ToUtf8(attr_name);
	if (buf_name == NULL)
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
	if (attr_value == NULL)
		static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->DeleteAttribute(buf_name);
	else
	{
		char* buf_value = Utf16ToUtf8(attr_value);
		if (buf_value == NULL)
		{
			FreeMem(buf_name);
			THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
		}
		static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->SetAttribute(buf_name, buf_value);
		FreeMem(buf_value);
	}
	FreeMem(buf_name);
}

EXPORT_CPP void* _xmlNodeGetAttr(SClass* me_, const U8* attr_name)
{
	THROWDBG(attr_name == NULL, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return NULL;
	char* buf = Utf16ToUtf8(attr_name);
	if (buf == NULL)
		THROWDBG(True, EXCPT_DBG_ARG_OUT_DOMAIN);
	const char* str = static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->Attribute(buf);
	FreeMem(buf);
	return Utf8ToUtf16(str);
}
