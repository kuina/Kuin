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
	tinyxml2::XMLNode* Ptr;
} SXml;

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
	new(me2->Tree)tinyxml2::XMLDocument();
	me2->Ptr = NULL;
	tinyxml2::XMLError result = me2->Tree->LoadFile(file_ptr);
	fclose(file_ptr);
	if (result != tinyxml2::XML_SUCCESS)
	{
		me2->Tree->~XMLDocument();
		FreeMem(me2->Tree);
		return NULL;
	}
	return me_;
}

EXPORT_CPP SClass* _makeXmlEmpty(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	me2->Tree = static_cast<tinyxml2::XMLDocument*>(AllocMem(sizeof(tinyxml2::XMLDocument)));
	new(me2->Tree)tinyxml2::XMLDocument();
	me2->Ptr = NULL;
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

EXPORT_CPP Bool _xmlSave(SClass* me_, const U8* path)
{
	THROWDBG(path == NULL, 0xc0000005);
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
	FILE* file_ptr = _wfopen(path2, L"wb");
	if (file_ptr == NULL)
		return False;
	tinyxml2::XMLError result = me2->Tree->SaveFile(file_ptr);
	fclose(file_ptr);
	return result == tinyxml2::XML_SUCCESS;
}

EXPORT_CPP void _xmlRoot(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	me2->Ptr = me2->Tree->FirstChild();
}

EXPORT_CPP Bool _xmlTerm(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	return me2->Ptr == NULL;
}

EXPORT_CPP void* _xmlGet(SClass* me_)
{
	SXml* me2 = reinterpret_cast<SXml*>(me_);
	THROWDBG(me2->Ptr == NULL, 0xc0000005);
	const char* str = me2->Ptr->ToElement()->GetText();
	int len_str = static_cast<int>(strlen(str));
	size_t len = static_cast<size_t>(MultiByteToWideChar(CP_UTF8, 0, str, len_str, NULL, 0));
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = static_cast<S64>(len);
	if (MultiByteToWideChar(CP_UTF8, 0, str, len_str, reinterpret_cast<Char*>(result + 0x10), static_cast<int>(len)) != static_cast<int>(len))
	{
		FreeMem(result);
		return NULL;
	}
	reinterpret_cast<Char*>(result + 0x10)[len] = L'\0';
	return result;
}
