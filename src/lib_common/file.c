#include "file.h"

typedef struct SStream
{
	SClass Class;
	void(*Fin)(SClass* me_);
	S64(*FileSize)(SClass* me_);
	FILE* Handle;
} SStream;

static void StreamDtor(SClass* me_);

EXPORT void _streamFin(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
	fclose(me2->Handle);
	me2->Handle = NULL;
}

EXPORT S64 _streamFileSize(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
	if (me2->Handle == NULL)
		THROW(0x1000, L"");
	{
		long current = ftell(me2->Handle);
		S64 result;
		fseek(me2->Handle, 0, SEEK_END);
		result = (S64)ftell(me2->Handle);
		fseek(me2->Handle, current, SEEK_SET);
		return result;
	}
}

EXPORT SClass* _loadReader(SClass* me_, const U8* path)
{
	SStream* me2 = (SStream*)me_;
	FILE* file_ptr = _wfopen((Char*)(path + 0x10), L"rb");
	if (file_ptr == NULL)
		return NULL;
	me2->Class.Dtor = StreamDtor;
	me2->Handle = file_ptr;
	return (SClass*)me2;
}

static void StreamDtor(SClass* me_)
{
	SStream* me2 = (SStream*)me_;
	if (me2->Handle != NULL)
		fclose(me2->Handle);
}
