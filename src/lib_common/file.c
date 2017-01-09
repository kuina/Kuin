#include "file.h"

typedef struct SStream
{
	SClass Class;
	void(*Fin)(SClass* me_);
	void* GetPos;
	void* SetPos;
	void* ReadWrite;
	void* FileSize;
	void* Term;
	FILE* Handle;
} SStream;

static void StreamDtor(SClass* me_);

EXPORT void _streamFin(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	fclose(me2->Handle);
	me2->Handle = NULL;
}

EXPORT S64 _streamGetPos(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	return _ftelli64(me2->Handle);
}

EXPORT void _streamSetPos(SClass* me_, S64 origin, S64 pos)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
	ASSERT(0 <= origin && origin <= 2);
#endif
	if (_fseeki64(me2->Handle, pos, (int)origin))
		THROW(0x1000, L"");
}

EXPORT void* _streamRead(SClass* me_, S64 size)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	{
		U8* result = (U8*)AllocMem(0x10 + (size_t)size);
		size_t size2;
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = size;
		size2 = fread(result + 0x10, 1, (size_t)size, me2->Handle);
		if (size2 != (size_t)size)
		{
			FreeMem(result);
			return NULL;
		}
		return result;
	}
}

EXPORT void _streamWrite(SClass* me_, void* bin)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	{
		U8* bin2 = (U8*)bin;
		fwrite(bin2 + 0x10, 1, (size_t)*(S64*)(bin2 + 0x08), me2->Handle);
	}
}

EXPORT S64 _streamFileSize(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	{
		S64 current = _ftelli64(me2->Handle);
		S64 result;
		_fseeki64(me2->Handle, 0, SEEK_END);
		result = _ftelli64(me2->Handle);
		_fseeki64(me2->Handle, current, SEEK_SET);
		return result;
	}
}

EXPORT Bool _streamTerm(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
#if defined(DBG)
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
#endif
	{
		Bool result = fgetc(me2->Handle) == -1;
		if (!result)
			_fseeki64(me2->Handle, -1, SEEK_CUR);
		return result;
	}
}

EXPORT SClass* _makeReader(SClass* me_, const U8* path)
{
	SStream* me2 = (SStream*)me_;
	FILE* file_ptr = _wfopen((Char*)(path + 0x10), L"rb");
	if (file_ptr == NULL)
		return NULL;
	me2->Class.Dtor = StreamDtor;
	me2->Handle = file_ptr;
	return (SClass*)me2;
}

EXPORT SClass* _makeWriter(SClass* me_, const U8* path, Bool append)
{
	SStream* me2 = (SStream*)me_;
	FILE* file_ptr = _wfopen((Char*)(path + 0x10), append ? L"ab" : L"wb");
	if (file_ptr == NULL)
		return NULL;
	InitClass(&me2->Class, NULL, StreamDtor);
	me2->Handle = file_ptr;
	return (SClass*)me2;
}

static void StreamDtor(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
	if (me2->Handle != NULL)
		fclose(me2->Handle);
}
