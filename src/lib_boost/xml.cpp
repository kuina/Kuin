#include "xml.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <codecvt>

using namespace boost::property_tree;

typedef struct SXml
{
	SClass Class;
	wptree* Tree;
	wptree* Ptr;
} SXml;

extern "C" void* Call0Asm(void* func);
extern "C" void* Call1Asm(void* arg1, void* func);
extern "C" void* Call2Asm(void* arg1, void* arg2, void* func);
extern "C" void* Call3Asm(void* arg1, void* arg2, void* arg3, void* func);

EXPORT_CPP SClass* _makeXml(SClass* me_, const U8* path)
{
	THROWDBG(path == NULL, 0xc0000005);
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	me2->Tree = static_cast<wptree*>(AllocMem(sizeof(wptree)));
	new(me2->Tree)wptree();
	me2->Ptr = NULL;
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	try
	{
		std::wifstream stream(path2);
		stream.imbue(std::locale(std::locale(), new std::codecvt_utf8<Char>));
		read_xml(stream, *me2->Tree, xml_parser::trim_whitespace);
		stream.close();
	}
	catch (...)
	{
		THROW(0xe9170008);
	}
	return me_;
}

EXPORT_CPP SClass* _makeXmlEmpty(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	me2->Tree = static_cast<wptree*>(AllocMem(sizeof(wptree)));
	new(me2->Tree)wptree();
	return me_;
}

EXPORT_CPP void _xmlDtor(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	if (me2->Tree != NULL)
	{
		me2->Tree->~basic_ptree();
		FreeMem(me2->Tree);
	}
}

EXPORT_CPP Bool _xmlSave(SClass* me_, const U8* path)
{
	THROWDBG(path == NULL, 0xc0000005);
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	try
	{
		std::wofstream stream(path2);
		stream.imbue(std::locale(std::locale(), new std::codecvt_utf8<Char>));
		write_xml(stream, *me2->Tree, xml_writer_make_settings<std::wstring>(L'\t', 1));
		stream.close();
	}
	catch (...)
	{
		return False;
	}
	return True;
}

EXPORT_CPP void _xmlHead(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	me2->Ptr = me2->Tree;
}

EXPORT_CPP Bool _xmlTerm(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	return me2->Ptr == NULL;
}

EXPORT_CPP void _xmlNextOffset(SClass* me_, const U8* elementPath)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	if (me2->Ptr == NULL)
		return;
	boost::optional<wptree&> optional = me2->Ptr->get_child_optional(elementPath == NULL ? L"" : reinterpret_cast<const Char*>(elementPath + 0x10));
	if (!optional)
	{
		me2->Ptr = NULL;
		return;
	}
	me2->Ptr = &*optional;
}

EXPORT_CPP void* _xmlGet(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	THROWDBG(me2->Ptr == NULL, 0xc0000005);
	boost::optional<std::wstring> str = me2->Ptr->get_value_optional<std::wstring>();
	if (!str)
		return NULL;
	size_t len = str->size();
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = static_cast<S64>(len);
	memcpy(result + 0x10, str->c_str(), sizeof(Char) * (len + 1));
	return result;
}

EXPORT_CPP void* _xmlGetOffset(SClass* me_, const U8* elementPath)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	THROWDBG(me2->Ptr == NULL, 0xc0000005);
	boost::optional<std::wstring> str = me2->Ptr->get_optional<std::wstring>(elementPath == NULL ? L"" : reinterpret_cast<const Char*>(elementPath + 0x10));
	if (!str)
		return NULL;
	size_t len = str->size();
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = static_cast<S64>(len);
	memcpy(result + 0x10, str->c_str(), sizeof(Char) * (len + 1));
	return result;
}

EXPORT_CPP void _xmlSet(SClass* me_, const U8* value)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	THROWDBG(me2->Ptr == NULL, 0xc0000005);
	const Char* value2 = value == NULL ? L"" : reinterpret_cast<const Char*>(value + 0x10);
	me2->Ptr->put_value(value2);
}

EXPORT_CPP void _xmlSetOffset(SClass* me_, const U8* elementPath, const U8* value)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	THROWDBG(me2->Ptr == NULL, 0xc0000005);
	const Char* value2 = value == NULL ? L"" : reinterpret_cast<const Char*>(value + 0x10);
	me2->Ptr->put(elementPath == NULL ? L"" : reinterpret_cast<const Char*>(elementPath + 0x10), value2);
}

EXPORT_CPP void _xmlAdd(SClass* me_, const U8* elementPath, const U8* value)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	THROWDBG(me2->Ptr == NULL, 0xc0000005);
	const Char* value2 = value == NULL ? L"" : reinterpret_cast<const Char*>(value + 0x10);
	me2->Ptr->add(elementPath == NULL ? L"" : reinterpret_cast<const Char*>(elementPath + 0x10), value2);
}

EXPORT_CPP S64 _xmlLen(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	THROWDBG(me2->Ptr == NULL, 0xc0000005);
	return static_cast<S64>(me2->Ptr->size());
}

EXPORT_CPP Bool _xmlForEach(SClass* me_, void* callback, void* data)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	THROWDBG(me2->Ptr == NULL, 0xc0000005);
	boost::optional<wptree&> child = me2->Ptr->get_child_optional(L"");
	if (!child)
		return False;
	wptree::iterator iter1 = child->begin();
	wptree::iterator iter2 = child->end();
	S64 idx = 0;
	while (iter1 != iter2)
	{
		const std::wstring& key = iter1->first;
		size_t len = static_cast<size_t>(key.size());
		U8* str = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
		(reinterpret_cast<S64*>(str))[0] = 1;
		(reinterpret_cast<S64*>(str))[1] = static_cast<S64>(len);
		memcpy(str + 0x10, key.c_str(), sizeof(Char) * (len + 1));
		if (data != NULL)
			(*static_cast<S64*>(data))++;
		Bool result = static_cast<Bool>(reinterpret_cast<S64>(Call3Asm(reinterpret_cast<void*>(idx), static_cast<void*>(str), data , callback)));
		if (!result)
			return False;
		idx++;
		++iter1;
	}
	return True;
}
