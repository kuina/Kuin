#include "snd.h"

#pragma comment(lib, "dsound.lib")

#include <dsound.h>

#include "wav.h"

const S64 BufSize = 2;

struct SSnd
{
	SClass Class;
	LPDIRECTSOUNDBUFFER8 SndBuf;
	HANDLE Event[2];
	S64 SizePerSec;
	S64 LoopPos;
	double EndPos;
	DWORD Freq;
	Bool Streaming;
	HANDLE Thread;
	double Volume;
	void* Handle;
	void (*FuncClose)(void*);
	Bool (*FuncRead)(void*, void*, S64, S64);
};

// The main volume controls these.
struct SListSnd
{
	SListSnd* Next;
	SSnd* Snd;
};

static HWND Wnd = NULL;
static LPDIRECTSOUND8 Device = NULL;
static SListSnd* ListSndTop = NULL;
static SListSnd* ListSndBottom = NULL;
static double MainVolume = 1.0;

static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
static DWORD WINAPI CBStreamThread(LPVOID param);
static Bool StreamCopy(SSnd* me_, S64 id);

EXPORT_CPP void _setMainVolume(double value)
{
	MainVolume = value;
	SListSnd* ptr = ListSndTop;
	while (ptr != NULL)
	{
		_sndVolume(reinterpret_cast<SClass*>(ptr->Snd), ptr->Snd->Volume);
		ptr = ptr->Next;
	}
}

EXPORT_CPP double _getMainVolume()
{
	return MainVolume;
}

EXPORT_CPP SClass* _makeSnd(SClass* me_, const U8* path, Bool streaming)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);

	WAVEFORMATEX pcmwf;
	DSBUFFERDESC desc;
	Bool success = False;
	for (; ; )
	{
		{
			S64 channel = 0;
			S64 samples_per_sec = 0;
			S64 bits_per_sample = 0;
			S64 total = 0;
			const Char* path2 = reinterpret_cast<const Char*>(path + 0x10);
			int len = static_cast<int>(wcslen(path2));
			if (StrCmpIgnoreCase(path2 + len - 4, L".wav"))
			{
				me2->Handle = LoadWav(path2, &channel, &samples_per_sec, &bits_per_sample, &total, &me2->FuncClose, &me2->FuncRead);
				if (me2->Handle == NULL)
					break;
			}
			else
				break;
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
		me2->Streaming = streaming;
		if (me2->Streaming)
		{
			if (me2->EndPos < 1.0)
				THROW(0x1000, L"");
			desc.dwBufferBytes = static_cast<DWORD>(BufSize * me2->SizePerSec);
		}
		{
			LPDIRECTSOUNDBUFFER pdsb;
			if (FAILED(Device->CreateSoundBuffer(&desc, &pdsb, NULL)))
				break;
			if (FAILED(pdsb->QueryInterface(IID_IDirectSoundBuffer8, reinterpret_cast<LPVOID*>(&me2->SndBuf))))
				break;
			pdsb->Release();
		}
		{
			DWORD freq;
			me2->SndBuf->GetFrequency(&freq);
			me2->Freq = freq;
		}
		me2->LoopPos = 0;
		me2->Volume = 0.0;
		if (me2->Streaming)
		{
			DWORD id;
			me2->Thread = CreateThread(NULL, 0, CBStreamThread, static_cast<LPVOID>(me2), 0, &id);
		}
		else
		{
			LPVOID lpvptr;
			DWORD dwbytes;
			if (FAILED(me2->SndBuf->Lock(0, desc.dwBufferBytes, &lpvptr, &dwbytes, NULL, NULL, 0)))
				break;
			me2->FuncRead(me2->Handle, lpvptr, static_cast<S64>(dwbytes), -1);
			me2->SndBuf->Unlock(lpvptr, dwbytes, NULL, 0);
			me2->FuncClose(me2->Handle);
			me2->Handle = NULL;
			me2->Thread = NULL;
		}
		_sndVolume(me_, 1.0);
		{
			SListSnd* node = (SListSnd*)AllocMem(sizeof(SListSnd));
			node->Snd = me2;
			node->Next = NULL;
			if (ListSndTop == NULL)
				ListSndTop = node;
			else
				ListSndBottom->Next = node;
			ListSndBottom = node;
		}
		success = True;
		break;
	}
	if (!success)
		return NULL;
	return me_;
}

EXPORT_CPP void _sndDtor(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	if (me2->SndBuf != NULL)
	{
		me2->SndBuf->Stop();
		if (me2->Streaming)
			TerminateThread(me2->Thread, 0);
		me2->SndBuf->Release();
	}
	if (me2->Handle != NULL)
		me2->FuncClose(me2->Handle);
	{
		SListSnd* ptr = ListSndTop;
		SListSnd* ptr2 = NULL;
		while (ptr != NULL)
		{
			if (ptr->Snd == me2)
			{
				if (ptr2 == NULL)
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

EXPORT_CPP void _sndPlay(SClass* me_, double startPos)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	if (startPos < 0.0 || startPos >= me2->EndPos)
		THROW(0x1000, L"");
	me2->LoopPos = -1;
	me2->SndBuf->SetCurrentPosition(static_cast<DWORD>(startPos * (double)me2->SizePerSec));
	me2->SndBuf->Play(0, 0, me2->Streaming ? DSBPLAY_LOOPING : 0);
}

EXPORT_CPP void _sndPlayLoop(SClass* me_, double startPos, double loopPos)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	if (startPos < 0.0 || startPos >= me2->EndPos || !me2->Streaming && loopPos != 0.0 || loopPos < 0.0 || loopPos >= me2->EndPos)
		THROW(0x1000, L"");
	me2->LoopPos = static_cast<S64>(loopPos * (double)me2->SizePerSec);
	me2->SndBuf->SetCurrentPosition(static_cast<DWORD>(startPos * (double)me2->SizePerSec));
	me2->SndBuf->Play(0, 0, DSBPLAY_LOOPING);
}

EXPORT_CPP void _sndStop(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	me2->SndBuf->Stop();
}

EXPORT_CPP Bool _sndPlaying(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	DWORD status;
	me2->SndBuf->GetStatus(&status);
	return (status & DSBSTATUS_PLAYING) != 0;
}

EXPORT_CPP void _sndVolume(SClass* me_, double value)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	me2->Volume = value;
	value *= MainVolume;
	double v = 1.0 - value;
	me2->SndBuf->SetVolume(static_cast<LONG>(-v * v * 10000.0));
}

EXPORT_CPP void _sndPan(SClass* me_, double value)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	me2->SndBuf->SetPan(static_cast<LONG>(value * 10000.0));
}

EXPORT_CPP void _sndFreq(SClass* me_, double value)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	me2->SndBuf->SetFrequency(static_cast<DWORD>(static_cast<double>(me2->Freq) * value));
}

EXPORT_CPP double _sndPos(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	DWORD pos = 0;
	me2->SndBuf->GetCurrentPosition(&pos, NULL);
	return (double)pos / (double)me2->SizePerSec;
}

EXPORT_CPP double _sndLen(SClass* me_)
{
	SSnd* me2 = reinterpret_cast<SSnd*>(me_);
	return me2->EndPos;
}

namespace Snd
{

void Init()
{
	{
		WNDCLASSEX wnd_class;
		wnd_class.cbSize = sizeof(WNDCLASSEX);
		wnd_class.style = 0;
		wnd_class.lpfnWndProc = WndProc;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = Instance;
		wnd_class.hIcon = NULL;
		wnd_class.hCursor = NULL;
		wnd_class.hbrBackground = NULL;
		wnd_class.lpszMenuName = NULL;
		wnd_class.lpszClassName = L"KuinSndClass";
		wnd_class.hIconSm = NULL;
		RegisterClassEx(&wnd_class);
	}
	Wnd = CreateWindowEx(0, L"KuinSndClass", L"", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, Instance, NULL);

	if (FAILED(DirectSoundCreate8(NULL, &Device, NULL)))
		THROW(0x1000, L"");
	if (FAILED(Device->SetCooperativeLevel(Wnd, DSSCL_PRIORITY)))
		THROW(0x1000, L"");
	ListSndTop = NULL;
	ListSndBottom = NULL;
}

void Fin()
{
	ASSERT(ListSndTop == NULL);
	if (Device != NULL)
		Device->Release();
	if (Wnd != NULL)
		SendMessage(Wnd, WM_DESTROY, 0, 0);
}

} // namespace Snd

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

static DWORD WINAPI CBStreamThread(LPVOID param)
{
	SSnd* me_ = reinterpret_cast<SSnd*>(param);
	S64 dsize = me_->SizePerSec * BufSize / 2;
	Bool flag = False;
	StreamCopy(me_, 0);
	StreamCopy(me_, 1);
	for (; ; )
	{
		int finish_cnt = 0;
		while (finish_cnt < 3)
		{
			DWORD pos = 0;
			me_->SndBuf->GetCurrentPosition(&pos, NULL);
			if (flag)
			{
				if (pos < dsize)
				{
					if (StreamCopy(me_, 1))
						finish_cnt++;
					flag = False;
				}
			}
			else
			{
				if (pos >= dsize)
				{
					if (StreamCopy(me_, 0))
						finish_cnt++;
					flag = True;
				}
			}
			Sleep(500);
		}
		me_->SndBuf->Stop();
		finish_cnt = 0;
	}
}

static Bool StreamCopy(SSnd* me_, S64 id)
{
	S64 dsize = me_->SizePerSec * BufSize / 2;
	LPVOID lpvptr0, lpvptr1;
	DWORD dwbytes0, dwbytes1;
	Bool finish = False;
	for (; ; )
	{
		if (FAILED(me_->SndBuf->Lock(static_cast<DWORD>(dsize * id), static_cast<DWORD>(dsize), &lpvptr0, &dwbytes0, &lpvptr1, &dwbytes1, 0)))
		{
			Sleep(1);
			continue;
		}
		break;
	}
	if (dwbytes0 > 0)
	{
		if (me_->FuncRead(me_->Handle, lpvptr0, static_cast<S64>(dwbytes0), me_->LoopPos))
			finish = True;
	}
	if (dwbytes1 > 0)
	{
		if (me_->FuncRead(me_->Handle, lpvptr1, static_cast<S64>(dwbytes1), me_->LoopPos))
			finish = True;
	}
	me_->SndBuf->Unlock(lpvptr0, dwbytes0, lpvptr1, dwbytes1);
	return finish;
}
