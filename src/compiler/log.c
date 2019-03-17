#include "log.h"

#include "mem.h"
#include "util.h"

#define MSG_NUM (453 / 3)
#define MSG_MAX (128)
#define LANG_NUM (2)

typedef struct SErrMsg
{
	U32 Code;
	Char Msg[MSG_MAX + 1];
} SErrMsg;

static void(*LogFunc)(const Char* code, const Char* msg, const Char* src, int row, int col) = NULL;
static int ErrCnt;
static SErrMsg ErrMsgs[MSG_NUM];
static const Char* LastErrSrc;
static int LastErrRow;
static Bool MsgLoaded = (Bool)0;

Bool SetLogFunc(void(*func_log)(const Char* code, const Char* msg, const Char* src, int row, int col), int lang, const Char* sys_dir)
{
	LogFunc = func_log;
	ErrCnt = 0;
	LastErrSrc = L"$";
	LastErrRow = -1;

	if (!MsgLoaded)
	{
		FILE* file_ptr;
		ASSERT(0 <= lang && lang < LANG_NUM); // 0 = 'Ja', 1 = 'En'.
		{
			Char path[KUIN_MAX_PATH + 1];
			wcscpy(path, sys_dir);
			wcscat(path, L"msg.knd");
			file_ptr = _wfopen(path, L"r, ccs=UTF-8");
			if (file_ptr == NULL)
			{
				Char msg[1024];
				swprintf(msg, 1024, lang == 0 ? L"\u30b7\u30b9\u30c6\u30e0\u30d5\u30a1\u30a4\u30eb\u300c%s\u300d\u304c\u958b\u3051\u307e\u305b\u3093\u3002" : L"A system file '%s' could not be opened.", path);
				func_log(L"EK0000", msg, NULL, -1, -1);
				return False;
			}
		}
		{
			int i;
			int j;
			Char buf[256];
#if defined(_DEBUG)
			U32 prev_code = 0;
#endif
			for (i = 0; i < MSG_NUM; i++)
			{
				ReadFileLine(buf, 256, file_ptr);
				ASSERT(wcslen(buf) == 6);
				ErrMsgs[i].Code = 0;
				{
					switch (buf[0])
					{
						case L'I': ErrMsgs[i].Code |= 0x00000000; break;
						case L'E': ErrMsgs[i].Code |= 0x00100000; break;
						case L'W': ErrMsgs[i].Code |= 0x00200000; break;
						default:
							ASSERT(False);
							break;
					}
					switch (buf[1])
					{
						case L'K': ErrMsgs[i].Code |= 0x00000000; break;
						case L'P': ErrMsgs[i].Code |= 0x00010000; break;
						case L'A': ErrMsgs[i].Code |= 0x00020000; break;
						default:
							ASSERT(False);
							break;
					}
					{
						int code = _wtoi(buf + 2);
						ASSERT(0 <= code && code <= 9999);
						ErrMsgs[i].Code |= code;
					}
				}
#if defined(_DEBUG)
				if (i != 0)
					ASSERT(prev_code < ErrMsgs[i].Code);
				prev_code = ErrMsgs[i].Code;
#endif
				for (j = 0; j < LANG_NUM; j++)
				{
					ReadFileLine(buf, 256, file_ptr);
					ASSERT(wcslen(buf) < MSG_MAX);
					if (j == lang)
						wcscpy(ErrMsgs[i].Msg, buf);
				}
			}
			ASSERT(fgetwc(file_ptr) == WEOF);
		}
		fclose(file_ptr);
		MsgLoaded = True;
	}
	return True;
}

void Err(const Char* code, const SPos* pos, ...)
{
	if (LogFunc == NULL)
		return;
	if (ErrCnt == 100)
		return; // Stop error detection at 100 pieces.
	if (code[0] != L'I' && pos != NULL && wcscmp(pos->SrcName, LastErrSrc) == 0 && pos->Row == LastErrRow)
		return;
	if (pos != NULL)
	{
		LastErrSrc = pos->SrcName;
		LastErrRow = pos->Row;
	}
	{
		U32 code2 = 0;
		ASSERT(wcslen(code) == 6);
		{
			switch (code[0])
			{
				case L'I': code2 |= 0x00000000; break;
				case L'E': code2 |= 0x00100000; break;
				case L'W': code2 |= 0x00200000; break;
				default:
					ASSERT(False);
					break;
			}
			switch (code[1])
			{
				case L'K': code2 |= 0x00000000; break;
				case L'P': code2 |= 0x00010000; break;
				case L'A': code2 |= 0x00020000; break;
				default:
					ASSERT(False);
					break;
			}
			{
				int code3 = _wtoi(code + 2);
				ASSERT(0 <= code3 && code3 <= 9999);
				code2 |= code3;
			}
		}
		{
			int min = 0;
			int max = MSG_NUM - 1;
			int found = -1;
			while (min <= max)
			{
				int mid = (min + max) / 2;
				if (code2 < ErrMsgs[mid].Code)
					max = mid - 1;
				else if (code2 > ErrMsgs[mid].Code)
					min = mid + 1;
				else
				{
					found = mid;
					break;
				}
			}
			ASSERT(found != -1);
			{
				Char* buf;
				va_list arg;
				va_start(arg, pos);
				{
					const Char* msg = (const Char*)ErrMsgs[found].Msg;
					int size = _vscwprintf(msg, arg);
					buf = (Char*)Alloc(sizeof(Char) * (size_t)(size + 1));
					vswprintf(buf, size + 1, msg, arg);
				}
				va_end(arg);
				{
					Char* ptr = buf;
					while (*ptr != L'\0')
					{
						switch (*ptr)
						{
							case L'\n':
							case L'\r':
							case L'\0':
							case L'\t':
								*ptr = L' ';
								break;
						}
						ptr++;
					}
				}
				if (pos == NULL)
					LogFunc(code, buf, NULL, -1, -1);
				else
					LogFunc(code, buf, pos->SrcName, pos->Row, pos->Col);
			}
		}
	}
	if (code[0] == L'E')
		ErrCnt++;
}

void ResetErrOccurred(void)
{
	ErrCnt = 0;
}

Bool ErrOccurred(void)
{
	return ErrCnt > 0;
}
