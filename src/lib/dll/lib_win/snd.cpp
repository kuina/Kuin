#include "snd.h"

#include "wav_decoder.h"
#include <dsound.h>

#pragma comment(lib, "dsound.lib")

static const S64 BufSize = 2;

struct SSnd
{
	SClass Class;
	LPDIRECTSOUNDBUFFER8 SndBuf;
	S64 SizePerSec;
	double EndPos;
	double Freq;
	double Volume;
};

// The main volume controls these.
struct SListSnd
{
	SListSnd* Next;
	SSnd* Snd;
};

static HWND Wnd = nullptr;
static LPDIRECTSOUND8 Device = nullptr;
static SListSnd* ListSndTop = nullptr;
static SListSnd* ListSndBottom = nullptr;
static HMODULE DllOgg = nullptr;
static void* (*LoadOgg)(size_t size, const U8* data, S64* channel, S64* samples_per_sec, S64* bits_per_sample, S64* total, void(**func_close)(void*), Bool(**func_read)(void*, void*, S64, S64)) = nullptr;
static double MainVolume = 1.0;

static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static Bool StrCmpIgnoreCase(const Char* a, const Char* b);

EXPORT_CPP void _sndInit(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);

	const HINSTANCE instance = (HINSTANCE)GetModuleHandle(nullptr);
	{
		WNDCLASSEX wnd_class;
		wnd_class.cbSize = sizeof(WNDCLASSEX);
		wnd_class.style = 0;
		wnd_class.lpfnWndProc = WndProc;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = instance;
		wnd_class.hIcon = nullptr;
		wnd_class.hCursor = nullptr;
		wnd_class.hbrBackground = nullptr;
		wnd_class.lpszMenuName = nullptr;
		wnd_class.lpszClassName = L"KuinSndClass";
		wnd_class.hIconSm = nullptr;
		RegisterClassEx(&wnd_class);
	}
	Wnd = CreateWindowEx(0, L"KuinSndClass", L"", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, instance, nullptr);

	if (FAILED(DirectSoundCreate8(nullptr, &Device, nullptr)))
	{
		Device = nullptr;
		return;
	}
	if (FAILED(Device->SetCooperativeLevel(Wnd, DSSCL_PRIORITY)))
	{
		Device = nullptr;
		return;
	}
	ListSndTop = nullptr;
	ListSndBottom = nullptr;
	LoadOgg = nullptr;

	DllOgg = LoadLibrary(L"data/d1000.knd");
	if (DllOgg != nullptr)
	{
		LoadOgg = reinterpret_cast<void* (*)(size_t size, const U8 * data, S64 * channel, S64 * samples_per_sec, S64 * bits_per_sample, S64 * total, void(**func_close)(void*), Bool(**func_read)(void*, void*, S64, S64))>(GetProcAddress(DllOgg, "LoadOgg"));
		if (LoadOgg == nullptr)
		{
			FreeLibrary(DllOgg);
			DllOgg = nullptr;
		}
	}
}

EXPORT_CPP void _sndFin()
{
	if (DllOgg != nullptr)
		FreeLibrary(DllOgg);
	ASSERT(ListSndTop == nullptr);
	if (Device != nullptr)
		Device->Release();
	if (Wnd != nullptr)
		SendMessage(Wnd, WM_DESTROY, 0, 0);
}

EXPORT_CPP void _sndFin2(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	if (me2->SndBuf != nullptr)
	{
		me2->SndBuf->Stop();
		me2->SndBuf->Release();
		me2->SndBuf = nullptr;
	}
	{
		SListSnd* ptr = ListSndTop;
		SListSnd* ptr2 = nullptr;
		while (ptr != nullptr)
		{
			if (ptr->Snd == me2)
			{
				if (ptr2 == nullptr)
					ListSndTop = ptr->Next;
				else
					ptr2->Next = ptr->Next;
				if (ListSndBottom == ptr)
					ListSndBottom = ptr2;
				FreeMem(ptr);
				break;
			}
			ptr2 = ptr;
			ptr = ptr->Next;
		}
	}
}

EXPORT_CPP void _sndFreq(SClass* me_, double value)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	THROWDBG(value < 0.1 || 2.0 < value, 0xe9170006);
	me2->SndBuf->SetFrequency(static_cast<DWORD>(me2->Freq * value));
}

EXPORT_CPP double _sndGetPos(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	DWORD pos = 0;
	me2->SndBuf->GetCurrentPosition(&pos, nullptr);
	return static_cast<double>(pos) / static_cast<double>(me2->SizePerSec);
}

EXPORT_CPP double _sndLen(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	return me2->EndPos;
}

EXPORT_CPP void _sndPan(SClass* me_, double value)
{
	THROWDBG(value < -1.0 || 1.0 < value, 0xe9170006);
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	me2->SndBuf->SetPan(static_cast<LONG>(value * 10000.0));
}

EXPORT_CPP void _sndPlay(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	me2->SndBuf->Play(0, 0, 0);
}

EXPORT_CPP Bool _sndPlaying(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	DWORD status;
	me2->SndBuf->GetStatus(&status);
	return (status & DSBSTATUS_PLAYING) != 0;
}

EXPORT_CPP void _sndPlayLoop(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	me2->SndBuf->Play(0, 0, DSBPLAY_LOOPING);
}

EXPORT_CPP void _sndSetPos(SClass* me_, double value)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	THROWDBG(value < 0.0 || me2->EndPos <= value, 0xe9170006);
	me2->SndBuf->SetCurrentPosition(static_cast<DWORD>(value * static_cast<double>(me2->SizePerSec)));
}

EXPORT_CPP void _sndStop(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	me2->SndBuf->Stop();
}

EXPORT_CPP void _sndVolume(SClass* me_, double value)
{
	THROWDBG(value < 0.0 || 1.0 < value, 0xe9170006);
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	me2->Volume = value;
	value *= MainVolume;
	double v = 1.0 - value;
	me2->SndBuf->SetVolume(static_cast<LONG>(-v * v * 10000.0));
}

EXPORT_CPP double _getMainVolume()
{
	return MainVolume;
}

EXPORT_CPP SClass* _makeSnd(SClass* me_, const U8* data, const U8* path)
{
	THROWDBG(path == nullptr, EXCPT_ACCESS_VIOLATION);
	if (Device == nullptr)
		THROW(0xe9170009);
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);

	WAVEFORMATEX pcmwf;
	DSBUFFERDESC desc;
	Bool success = False;
	void(*func_close)(void*) = nullptr;
	Bool(*func_read)(void*, void*, S64, S64) = nullptr;
	void* handle = nullptr;
	for (; ; )
	{
		{
			S64 channel = 0;
			S64 samples_per_sec = 0;
			S64 bits_per_sample = 0;
			S64 total = 0;
			const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
			int len = static_cast<int>(wcslen(path2));
			size_t size = static_cast<size_t>(*reinterpret_cast<const S64*>(data + 0x08));
			if (StrCmpIgnoreCase(path2 + len - 4, L".wav"))
			{
				handle = LoadWav(size, data + 0x10, &channel, &samples_per_sec, &bits_per_sample, &total, &func_close, &func_read);
				if (handle == nullptr)
					break;
			}
			else if (StrCmpIgnoreCase(path2 + len - 4, L".ogg"))
			{
				if (LoadOgg == nullptr)
					break;
				handle = LoadOgg(size, data + 0x10, &channel, &samples_per_sec, &bits_per_sample, &total, &func_close, &func_read);
				if (handle == nullptr)
					break;
			}
			else
			{
				THROWDBG(True, 0xe9170006);
				break;
			}
			{
				pcmwf.wFormatTag = WAVE_FORMAT_PCM;
				pcmwf.nChannels = static_cast<WORD>(channel);
				pcmwf.nSamplesPerSec = static_cast<DWORD>(samples_per_sec);
				pcmwf.nAvgBytesPerSec = static_cast<DWORD>(samples_per_sec * channel * bits_per_sample / 8);
				pcmwf.nBlockAlign = static_cast<WORD>(channel * bits_per_sample / 8);
				pcmwf.wBitsPerSample = static_cast<WORD>(bits_per_sample);
				pcmwf.cbSize = 0;
				desc.dwSize = sizeof(DSBUFFERDESC);
				desc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCDEFER;
				desc.dwBufferBytes = static_cast<DWORD>(total);
				desc.dwReserved = 0;
				desc.lpwfxFormat = &pcmwf;
				desc.guid3DAlgorithm = DS3DALG_DEFAULT;
			}
			me2->SizePerSec = desc.lpwfxFormat->nAvgBytesPerSec;
			me2->EndPos = static_cast<double>(total) / static_cast<double>(samples_per_sec * channel * bits_per_sample / 8);
		}
		{
			LPDIRECTSOUNDBUFFER pdsb;
			if (FAILED(Device->CreateSoundBuffer(&desc, &pdsb, nullptr)))
				break;
			if (FAILED(pdsb->QueryInterface(IID_IDirectSoundBuffer8, reinterpret_cast<LPVOID*>(&me2->SndBuf))))
				break;
			pdsb->Release();
		}
		{
			DWORD freq;
			me2->SndBuf->GetFrequency(&freq);
			me2->Freq = static_cast<double>(freq);
		}
		me2->Volume = 0.0;
		{
			LPVOID lpvptr;
			DWORD dwbytes;
			if (FAILED(me2->SndBuf->Lock(0, desc.dwBufferBytes, &lpvptr, &dwbytes, nullptr, nullptr, 0)))
				break;
			func_read(handle, lpvptr, static_cast<S64>(dwbytes), -1);
			me2->SndBuf->Unlock(lpvptr, dwbytes, nullptr, 0);
			func_close(handle);
		}
		_sndVolume(me_, 1.0);
		{
			SListSnd* node = (SListSnd*)AllocMem(sizeof(SListSnd));
			node->Snd = me2;
			node->Next = nullptr;
			if (ListSndTop == nullptr)
				ListSndTop = node;
			else
				ListSndBottom->Next = node;
			ListSndBottom = node;
		}
		success = True;
		break;
	}
	if (!success)
	{
		THROW(0xe9170009);
		if (handle != nullptr)
			func_close(handle);
		if (me2->SndBuf != nullptr)
			me2->SndBuf->Release();
		return nullptr;
	}
	return me_;
}

EXPORT_CPP void _setMainVolume(double value)
{
	THROWDBG(value < 0.0 || 1.0 < value, 0xe9170006);
	MainVolume = value;
	SListSnd* ptr = ListSndTop;
	while (ptr != nullptr)
	{
		_sndVolume(reinterpret_cast<SClass*>(ptr->Snd), ptr->Snd->Volume);
		ptr = ptr->Next;
	}
}

static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	switch (msg)
	{
		case WM_CLOSE:
			SendMessage(wnd, WM_DESTROY, 0, 0);
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}
	return DefWindowProc(wnd, msg, w_param, l_param);
}

static Bool StrCmpIgnoreCase(const Char* a, const Char* b)
{
	while (*a != L'\0')
	{
		Char a2 = L'A' <= *a && *a <= L'Z' ? (*a - L'A' + L'a') : *a;
		Char b2 = L'A' <= *b && *b <= L'Z' ? (*b - L'A' + L'a') : *b;
		if (a2 != b2)
			return False;
		a++;
		b++;
	}
	return *b == L'\0';
}
