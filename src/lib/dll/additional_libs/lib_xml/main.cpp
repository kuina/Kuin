// LibXml.dll
//
// (C)Kuina-chan
//

// I modified the following points in the 'tinyxml2' library:
// - I inserted a space before the closing symbol of empty tags.
// - Instead of four spaces in the indent I changed it to a tab character.

#include "main.h"

#include "tinyxml2/tinyxml2.h"

#include <codecvt>
#include <sstream>

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

static std::string utf16ToUtf8_(const std::u16string& s);
static void* utf8ToUtf16_(const std::string& s);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT_CPP void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);
}

EXPORT_CPP SClass* _xmlNodeAddChild(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == nullptr, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	std::u16string s = reinterpret_cast<const char16_t*>(name + 0x10);
	const std::string& t = utf16ToUtf8_(s);
	const char* buf = t.c_str();
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
	me4->Root = False;
	return me2;
}

EXPORT_CPP void _xmlNodeDelChild(SClass* me_, SClass* node)
{
	THROWDBG(node == nullptr, EXCPT_ACCESS_VIOLATION);
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

EXPORT_CPP SClass* _xmlNodeFindChild(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == nullptr, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	std::u16string s = reinterpret_cast<const char16_t*>(name + 0x10);
	const std::string& t = utf16ToUtf8_(s);
	const char* buf = t.c_str();
	if (me3->Root)
		me4->Node = static_cast<tinyxml2::XMLDocument*>(me3->Node)->FirstChildElement(buf);
	else
		me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->FirstChildElement(buf);
	if (me4->Node == nullptr)
		return nullptr;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeFindChildLast(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == nullptr, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	std::u16string s = reinterpret_cast<const char16_t*>(name + 0x10);
	const std::string& t = utf16ToUtf8_(s);
	const char* buf = t.c_str();
	if (me3->Root)
		me4->Node = static_cast<tinyxml2::XMLDocument*>(me3->Node)->LastChildElement(buf);
	else
		me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->LastChildElement(buf);
	if (me4->Node == nullptr)
		return nullptr;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeFindNext(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == nullptr, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return nullptr;
	std::u16string s = reinterpret_cast<const char16_t*>(name + 0x10);
	const std::string& t = utf16ToUtf8_(s);
	const char* buf = t.c_str();
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->NextSiblingElement(buf);
	if (me4->Node == nullptr)
		return nullptr;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeFindPrev(SClass* me_, SClass* me2, const U8* name)
{
	THROWDBG(name == nullptr, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return nullptr;
	std::u16string s = reinterpret_cast<const char16_t*>(name + 0x10);
	const std::string& t = utf16ToUtf8_(s);
	const char* buf = t.c_str();
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->PreviousSiblingElement(buf);
	if (me4->Node == nullptr)
		return nullptr;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeFirstChild(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		me4->Node = static_cast<tinyxml2::XMLDocument*>(me3->Node)->FirstChild();
	else
		me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->FirstChild();
	if (me4->Node == nullptr)
		return nullptr;
	me4->Root = False;
	return me2;
}

EXPORT_CPP void* _xmlNodeGetAttr(SClass* me_, const U8* attr_name)
{
	THROWDBG(attr_name == nullptr, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return nullptr;
	std::u16string s = reinterpret_cast<const char16_t*>(attr_name + 0x10);
	const std::string& t = utf16ToUtf8_(s);
	const char* buf = t.c_str();
	const char* str = static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->Attribute(buf);
	if (str == nullptr)
		return nullptr;
	std::string s2 = str;
	return utf8ToUtf16_(s2);
}

EXPORT_CPP void* _xmlNodeGetName(SClass* me_)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return nullptr;
	std::string s = static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->Name();
	return utf8ToUtf16_(s);
}

EXPORT_CPP void* _xmlNodeGetValue(SClass* me_)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return nullptr;
	const char* str = static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->GetText();
	if (str == nullptr)
		return nullptr;
	std::string s = str;
	return utf8ToUtf16_(s);
}

EXPORT_CPP SClass* _xmlNodeInsChild(SClass* me_, SClass* me2, SClass* node, const U8* name)
{
	THROWDBG(node == nullptr, EXCPT_ACCESS_VIOLATION);
	THROWDBG(name == nullptr, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	SXmlNode* node2 = reinterpret_cast<SXmlNode*>(node);
	if (node2->Root)
		return nullptr;
	std::u16string s = reinterpret_cast<const char16_t*>(name + 0x10);
	const std::string& t = utf16ToUtf8_(s);
	const char* buf = t.c_str();
	if (me3->Root)
	{
		tinyxml2::XMLDocument* node3 = static_cast<tinyxml2::XMLDocument*>(me3->Node);
		if (static_cast<tinyxml2::XMLNode*>(node2->Node)->Parent() == node3)
		{
			tinyxml2::XMLNode* node4 = static_cast<tinyxml2::XMLNode*>(node2->Node)->PreviousSibling();
			if (node4 == nullptr)
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
			if (node4 == nullptr)
				me4->Node = node3->InsertFirstChild(node3->GetDocument()->NewElement(buf));
			else
				me4->Node = node3->InsertAfterChild(node4, node3->GetDocument()->NewElement(buf));
		}
	}
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
	if (me4->Node == nullptr)
		return nullptr;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeNext(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return nullptr;
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->NextSibling();
	if (me4->Node == nullptr)
		return nullptr;
	me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodeParent(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return nullptr;
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->Parent();
	if (me4->Node == nullptr || static_cast<tinyxml2::XMLNode*>(me4->Node)->Parent() == nullptr)
	{
		me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->ToDocument();
		me4->Root = True;
	}
	else
		me4->Root = False;
	return me2;
}

EXPORT_CPP SClass* _xmlNodePrev(SClass* me_, SClass* me2)
{
	SXmlNode* me3 = reinterpret_cast<SXmlNode*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	if (me3->Root)
		return nullptr;
	me4->Node = static_cast<tinyxml2::XMLNode*>(me3->Node)->PreviousSibling();
	if (me4->Node == nullptr)
		return nullptr;
	me4->Root = False;
	return me2;
}

EXPORT_CPP void _xmlNodeSetAttr(SClass* me_, const U8* attr_name, const U8* attr_value)
{
	THROWDBG(attr_name == nullptr, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return;
	std::u16string s = reinterpret_cast<const char16_t*>(attr_name + 0x10);
	const std::string& t = utf16ToUtf8_(s);
	const char* buf_name = t.c_str();
	if (attr_value == nullptr)
		static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->DeleteAttribute(buf_name);
	else
	{
		std::u16string s2 = reinterpret_cast<const char16_t*>(attr_value + 0x10);
		const std::string& t2 = utf16ToUtf8_(s2);
		const char* buf_value = t2.c_str();
		static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->SetAttribute(buf_name, buf_value);
	}
}

EXPORT_CPP void _xmlNodeSetName(SClass* me_, const U8* name)
{
	THROWDBG(name == nullptr, EXCPT_ACCESS_VIOLATION);
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return;
	std::u16string s = reinterpret_cast<const char16_t*>(name + 0x10);
	const std::string& t = utf16ToUtf8_(s);
	const char* buf = t.c_str();
	static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->SetName(buf);
}

EXPORT_CPP void _xmlNodeSetValue(SClass* me_, const U8* value)
{
	SXmlNode* me2 = reinterpret_cast<SXmlNode*>(me_);
	if (me2->Root)
		return;
	if (value == nullptr)
		static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->SetText("");
	else
	{
		std::u16string s = reinterpret_cast<const char16_t*>(value + 0x10);
		const std::string& t = utf16ToUtf8_(s);
		const char* buf = t.c_str();
		static_cast<tinyxml2::XMLNode*>(me2->Node)->ToElement()->SetText(buf);
	}
}

EXPORT_CPP void _xmlDtor(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	if (me2->Tree != nullptr)
	{
		me2->Tree->~XMLDocument();
		FreeMem(me2->Tree);
	}
}

EXPORT_CPP SClass* _xmlRoot(SClass* me_, SClass* me2)
{
	SXml* me3 = reinterpret_cast<SXml*>(me_);
	SXmlNode* me4 = reinterpret_cast<SXmlNode*>(me2);
	me4->Root = True;
	me4->Node = me3->Tree;
	return me2;
}

EXPORT_CPP Bool _xmlSave(SClass* me_, const U8* path, Bool compact)
{
	THROWDBG(path == nullptr, EXCPT_ACCESS_VIOLATION);
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	FILE* file_ptr = _wfopen(path2, L"wb");
	if (file_ptr == nullptr)
		return False;
	tinyxml2::XMLError result = me2->Tree->SaveFile(file_ptr, compact != False);
	fclose(file_ptr);
	return result == tinyxml2::XML_SUCCESS;
}

EXPORT_CPP SClass* _makeXml(SClass* me_, const U8* data)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	size_t size = static_cast<size_t>(*reinterpret_cast<const S64*>(data + 0x08));
	const char* buf = reinterpret_cast<const char*>(data + 0x10);
	if (buf == nullptr)
		return nullptr;
	me2->Tree = static_cast<tinyxml2::XMLDocument*>(AllocMem(sizeof(tinyxml2::XMLDocument)));
	new(me2->Tree)tinyxml2::XMLDocument(true, tinyxml2::PRESERVE_WHITESPACE);
	tinyxml2::XMLError result = me2->Tree->Parse(buf, size);
	if (result != tinyxml2::XML_SUCCESS)
	{
		me2->Tree->~XMLDocument();
		FreeMem(me2->Tree);
		return nullptr;
	}
	ASSERT(me2->Tree->FirstChild() != nullptr);
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

static std::string utf16ToUtf8_(const std::u16string& s)
{
	return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(s);
}

static void* utf8ToUtf16_(const std::string& s)
{
	const std::u16string& t = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(s);
	U8* r = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * static_cast<size_t>(t.size() + 1)));
	reinterpret_cast<S64*>(r)[0] = DefaultRefCntFunc;
	reinterpret_cast<S64*>(r)[1] = static_cast<S64>(t.size());
	memcpy(r + 0x10, t.c_str(), sizeof(Char) * static_cast<size_t>(t.size() + 1));
	return r;
}
