#include "file.h"

typedef struct SFileInfo
{
	U8 Path[260];
	S64 Offset;
	S64 Size;
} SFileInfo;

typedef struct SPackHandle
{
	S64 Head;
	S64 Size;
	S64 Cur;
} SPackHandle;

static FILE* PackFile;
S64 FileInfoNum;
SFileInfo* FileInfo;
U64 FileKey;
static char* NewLine = "\r\n";

static Bool ForEachDirRecursion(const Char* path, Bool recursion, void* callback, void* data);
static Bool DelDirRecursion(const Char* path);
static Bool CopyDirRecursion(const Char* dst, const Char* src);
static void NormPath(Char* path, Bool dir);

EXPORT void _fileInit(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);

#if !defined(DBG)
	Char path[KUIN_MAX_PATH];
	wcscpy_s(path, KUIN_MAX_PATH, EnvVars.ResRoot);
	wcscat_s(path, KUIN_MAX_PATH, L"res.knd");
	PackFile = _wfopen(path, L"rb");
	if (PackFile != NULL)
	{
		S64 i, j;
		_fseeki64(PackFile, 0, SEEK_END);
		S64 size = _ftelli64(PackFile);
		_fseeki64(PackFile, 0, SEEK_SET);
		if (size < 0x18)
			THROW(0xe9170008);
		fread(&FileKey, sizeof(U64), 1, PackFile);
		FileKey ^= ((U64)EnvVars.AppCode * 0x9271ac8394027acb + 0x35718394ca72849e);
		{
			U64 signature;
			fread(&signature, sizeof(U64), 1, PackFile);
			if ((signature ^ FileKey) != 0x83261772fa0c01a7)
				THROW(0xe9170008);
		}
		fread(&FileInfoNum, sizeof(U64), 1, PackFile);
		FileInfoNum ^= 0x9c4cab83ce74a67e ^ FileKey;
		if (FileInfoNum <= 0 || size < 0x18 + FileInfoNum * 0x10)
			THROW(0xe9170008);
		FileInfo = (SFileInfo*)AllocMem(sizeof(SFileInfo) * (size_t)FileInfoNum);
		{
			U64 v = 0x17100b7ac917dc87 ^ FileKey;
			for (i = 0; i < FileInfoNum; i++)
			{
				fread(&FileInfo[i].Path, 1, 260, PackFile);
				fread(&FileInfo[i].Offset, sizeof(S64), 1, PackFile);
				for (j = 0; j < 260; j++)
				{
					v = v * 0x8121bba7c238010f + 0x190273b5c19bf763;
					FileInfo[i].Path[j] ^= (U8)(v >> 32);
				}
				v = v * 0x8121bba7c238010f + 0x190273b5c19bf763;
				FileInfo[i].Offset ^= v;
			}
		}
		for (i = 0; i < FileInfoNum - 1; i++)
			FileInfo[i].Size = FileInfo[i + 1].Offset - FileInfo[i].Offset;
		FileInfo[FileInfoNum - 1].Size = size - FileInfo[FileInfoNum - 1].Offset;
		for (i = 0; i < FileInfoNum - 1; i++)
		{
			if (FileInfo[i].Size < 0)
				THROW(0xe9170008);
		}
	}
#endif
}

EXPORT void _fileFin(void)
{
#if !defined(DBG)
	if (FileInfo != NULL)
		FreeMem(FileInfo);
	if (PackFile != NULL)
		fclose(PackFile);
#endif
}

EXPORT Bool _copyDir(const U8* dst, const U8* src)
{
	THROWDBG(dst == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(src == NULL, EXCPT_ACCESS_VIOLATION);
	return CopyDirRecursion((const Char*)(dst + 0x10), (const Char*)(src + 0x10));
}

EXPORT Bool _copyFile(const U8* dst, const U8* src)
{
	THROWDBG(dst == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(src == NULL, EXCPT_ACCESS_VIOLATION);
	return CopyFile((const Char*)(src + 0x10), (const Char*)(dst + 0x10), FALSE) != 0;
}

EXPORT Bool _delDir(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	return DelDirRecursion((const Char*)(path + 0x10));
}

EXPORT Bool _delFile(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	if (!PathFileExists((const Char*)(path + 0x10)))
		return True;
	return DeleteFile((const Char*)(path + 0x10)) != 0;
}

EXPORT Bool _existPath(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	return PathFileExists((const Char*)(path + 0x10)) != 0;
}

EXPORT Bool _forEachDir(const U8* path, Bool recursion, void* callback, void* data)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(callback == NULL, EXCPT_ACCESS_VIOLATION);
	Char path2[KUIN_MAX_PATH + 1];
	Char* file_name_pos;
	if (!GetFullPathName((const Char*)(path + 0x10), KUIN_MAX_PATH, path2, &file_name_pos))
		return False;
	NormPath(path2, True);
	return ForEachDirRecursion(path2, recursion, callback, data);
}

EXPORT void* _fullPath(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	Char path2[KUIN_MAX_PATH + 1];
	Char* file_name_pos;
	if (!GetFullPathName((const Char*)(path + 0x10), KUIN_MAX_PATH, path2, &file_name_pos))
		return NULL;
	NormPath(path2, ((const Char*)(path + 0x10))[*(S64*)(path + 0x08) - 1] == L'/');
	size_t len = wcslen(path2);
	U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
	*(S64*)(result + 0x00) = DefaultRefCntFunc;
	*(S64*)(result + 0x08) = (S64)len;
	wcscpy((Char*)(result + 0x10), path2);
	return result;
}

EXPORT void* _getCurDir(void)
{
	Char path[KUIN_MAX_PATH + 1];
	if (!GetCurrentDirectory(KUIN_MAX_PATH, path))
		return NULL;
	NormPath(path, True);
	size_t len = wcslen(path);
	U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
	*(S64*)(result + 0x00) = DefaultRefCntFunc;
	*(S64*)(result + 0x08) = (S64)len;
	wcscpy((Char*)(result + 0x10), path);
	return result;
}

EXPORT Bool _makeDir(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	Char path2[KUIN_MAX_PATH + 1];
	Char* file_name_pos;
	if (!GetFullPathName((const Char*)(path + 0x10), KUIN_MAX_PATH, path2, &file_name_pos))
		return False;
	NormPath(path2, True);
	const Char* ptr = path2;
	while (*ptr != L'\0')
	{
		ptr = wcschr(ptr, L'/');
		ASSERT(ptr != NULL);
		Char path3[KUIN_MAX_PATH + 1];
		memcpy(path3, path2, sizeof(Char) * (size_t)(ptr - path2 + 1));
		path3[ptr - path2 + 1] = L'\0';
		if (!PathFileExists(path3))
		{
			if (!CreateDirectory(path3, NULL))
				return False;
		}
		ptr++;
	}
	return True;
}

EXPORT Bool _moveDir(const U8* dst, const U8* src)
{
	THROWDBG(dst == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(src == NULL, EXCPT_ACCESS_VIOLATION);
	if (!MoveFileEx((const Char*)(src + 0x10), (const Char*)(dst + 0x10), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING))
	{
		if (!CopyDirRecursion((const Char*)(dst + 0x10), (const Char*)(src + 0x10)))
			return False;
		if (!DelDirRecursion((const Char*)(src + 0x10)))
			return False;
	}
	return True;
}

EXPORT Bool _moveFile(const U8* dst, const U8* src)
{
	THROWDBG(dst == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(src == NULL, EXCPT_ACCESS_VIOLATION);
	return MoveFileEx((const Char*)(src + 0x10), (const Char*)(dst + 0x10), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH | MOVEFILE_REPLACE_EXISTING) != 0;
}

EXPORT void _setCurDir(const U8* path)
{
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	SetCurrentDirectory((const Char*)(path + 0x10));
}

EXPORT void* _openAsReadingImpl(const U8* path, Bool pack, Bool* success)
{
	const Char* true_path;
#if defined(DBG)
	Char buf[KUIN_MAX_PATH];
	if (pack)
	{
		wcscpy_s(buf, KUIN_MAX_PATH, EnvVars.ResRoot);
		wcscat_s(buf, KUIN_MAX_PATH, (Char*)(path + 0x10));
		true_path = buf;
	}
	else
		true_path = (Char*)(path + 0x10);
#else
	if (pack)
	{
		S64 idx = -1;
		{
			const Char* path2 = (const Char*)(path + 0x10) + 4 /* "res/" */;
			S64 a = 0, b = FileInfoNum - 1;
			while (a <= b)
			{
				S64 c = (a + b) / 2;
				S64 m = 0;
				int j;
				for (j = 0; j < 260; j++)
				{
					U8 c1 = (U8)path2[j];
					U8 c2 = FileInfo[c].Path[j];
					if (c1 > c2)
					{
						m = 1;
						break;
					}
					if (c1 < c2)
					{
						m = -1;
						break;
					}
					if (c1 == 0)
						break;
				}
				if (m < 0)
					b = c - 1;
				else if (m > 0)
					a = c + 1;
				else
				{
					idx = c;
					break;
				}
			}
		}
		if (idx == -1)
		{
			*success = False;
			return NULL;
		}
		SPackHandle* handle = (SPackHandle*)AllocMem(sizeof(SPackHandle));
		handle->Head = FileInfo[idx].Offset;
		handle->Size = FileInfo[idx].Size;
		handle->Cur = 0;
		*success = True;
		return (void*)((S64)handle | 1LL);
	}
	else
		true_path = (Char*)(path + 0x10);
#endif
	FILE* file_ptr = _wfopen(true_path, L"rb");
	if (file_ptr == NULL)
	{
		*success = False;
		return NULL;
	}
	*success = True;
	return file_ptr;
}

EXPORT void _readerCloseImpl(void* handle)
{
	void* handle2 = (void*)((S64)handle & (~1LL));
	Bool pack = ((S64)handle & 1LL) != 0LL;
	if (handle2 != NULL)
	{
		if (pack)
			FreeMem((SPackHandle*)handle2);
		else
			fclose((FILE*)handle2);
	}
}

EXPORT void _readerSeekImpl(void* handle, S64 origin, S64 pos)
{
	void* handle2 = (void*)((S64)handle & (~1LL));
	Bool pack = ((S64)handle & 1LL) != 0LL;
	if (pack)
	{
		SPackHandle* handle3 = (SPackHandle*)handle2;
		S64 p;
		switch (origin)
		{
			case SEEK_SET: p = handle3->Head; break;
			case SEEK_CUR: p = handle3->Cur; break;
			case SEEK_END: p = handle3->Head + handle3->Size; break;
			default:
				return;
		}
		p += pos;
		if (p < handle3->Head)
			return;
		handle3->Cur = p;
	}
	else
	{
#if defined(DBG)
		if (_fseeki64((FILE*)handle2, pos, (int)origin) != 0)
			THROW(0xe9170006);
#else
		_fseeki64((FILE*)handle2, pos, (int)origin);
#endif
	}
}

EXPORT S64 _readerTellImpl(void* handle)
{
	void* handle2 = (void*)((S64)handle & (~1LL));
	Bool pack = ((S64)handle & 1LL) != 0LL;
	if (pack)
	{
		SPackHandle* handle3 = (SPackHandle*)handle2;
		return handle3->Cur - handle3->Head;
	}
	else
		return _ftelli64((FILE*)handle2);
}

EXPORT Bool _readerReadImpl(void* handle, void* buf, S64 start, S64 size)
{
	void* handle2 = (void*)((S64)handle & (~1LL));
	Bool pack = ((S64)handle & 1LL) != 0LL;
	if (pack)
	{
		SPackHandle* handle3 = (SPackHandle*)handle2;
		if (handle3->Cur + size > handle3->Head + handle3->Size)
			return False;
		_fseeki64(PackFile, handle3->Cur, SEEK_SET);
		if (fread((U8*)buf + 0x10 + start, 1, (size_t)size, PackFile) != (size_t)size)
			return False;
		U8* p = (U8*)buf + 0x10 + start;
		S64 v = handle3->Cur - handle3->Head;
		S64 i;
		for (i = 0; i < size; i++)
		{
			*p ^= (U8)((((U64)v ^ FileKey) * 0x351cd819923acae7) >> 32);
			p++;
			v++;
		}
		handle3->Cur += size;
		return True;
	}
	else
		return fread((U8*)buf + 0x10 + start, 1, (size_t)size, (FILE*)handle2) == (size_t)size;
}

EXPORT void* _openAsWritingImpl(const U8* path, Bool append, Bool* success)
{
	FILE* file_ptr = _wfopen((Char*)(path + 0x10), append ? L"ab" : L"wb");
	if (file_ptr == NULL)
	{
		*success = False;
		return 0;
	}
	*success = True;
	return file_ptr;
}

EXPORT void _writerCloseImpl(void* handle)
{
	fclose((FILE*)handle);
}

EXPORT void _writerFlushImpl(void* handle)
{
	fflush((FILE*)handle);
}

EXPORT void _writerSeekImpl(void* handle, S64 origin, S64 pos)
{
#if defined(DBG)
	if (_fseeki64((FILE*)handle, pos, (int)origin) != 0)
		THROW(0xe9170006);
#else
	_fseeki64((FILE*)handle, pos, (int)origin);
#endif
}

EXPORT S64 _writerTellImpl(void* handle)
{
	return _ftelli64((FILE*)handle);
}

EXPORT void _writerWriteImpl(void* handle, void* data, S64 start, S64 size)
{
	fwrite((U8*)data + 0x10 + start, 1, (size_t)size, (FILE*)handle);
}

EXPORT S64 _writerWriteNewLineImpl(void* handle)
{
	fwrite(NewLine, 1, 2, (FILE*)handle);
	return 2;
}

static Bool ForEachDirRecursion(const Char* path, Bool recursion, void* callback, void* data)
{
	size_t len = wcslen(path);
	if (len > KUIN_MAX_PATH)
		return False;
	WIN32_FIND_DATA find_data;
	HANDLE handle;
	Bool continue_loop = True;
	{
		Char path2[KUIN_MAX_PATH + 2];
		memcpy(path2, path, sizeof(Char) * len);
		path2[len] = L'*';
		path2[len + 1] = 0;
		handle = FindFirstFile(path2, &find_data);
	}
	if (handle == INVALID_HANDLE_VALUE)
		return False;
	{
		U8* path3 = (U8*)AllocMem(0x10 + sizeof(Char) * (len + 1));
		((S64*)path3)[0] = 2;
		((S64*)path3)[1] = len;
		memcpy(path3 + 0x10, path, sizeof(Char) * (len + 1));
		if (data != NULL)
			(*(S64*)data)++;
		Bool result = (Bool)(S64)Call3Asm((void*)path3, (void*)(S64)True, data, callback);
		(*(S64*)path3)--;
		if (*(S64*)path3 == 0)
			FreeMem(path3);
		if (!result)
			continue_loop = False;
	}
	if (continue_loop)
	{
		do
		{
			Char* name = find_data.cFileName;
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				size_t len2 = wcslen(name);
				if (len + len2 + 1 > KUIN_MAX_PATH)
				{
					continue_loop = False;
					break;
				}
				U8* path3 = (U8*)AllocMem(0x10 + sizeof(Char) * (len + len2 + 1));
				((S64*)path3)[0] = 2;
				((S64*)path3)[1] = len + len2;
				memcpy(path3 + 0x10, path, sizeof(Char) * len);
				memcpy(path3 + 0x10 + sizeof(Char) * len, name, sizeof(Char) * (len2 + 1));
				if (data != NULL)
					(*(S64*)data)++;
				Bool result = (Bool)(S64)Call3Asm((void*)path3, (void*)(S64)False, data, callback);
				(*(S64*)path3)--;
				if (*(S64*)path3 == 0)
					FreeMem(path3);
				if (!result)
				{
					continue_loop = False;
					break;
				}
			}
			else if (recursion)
			{
				if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
					continue;
				size_t len2 = wcslen(name);
				if (len + len2 + 2 > KUIN_MAX_PATH + 2)
				{
					continue_loop = False;
					break;
				}
				Char path2[KUIN_MAX_PATH + 2];
				memcpy(path2, path, sizeof(Char) * len);
				memcpy(path2 + len, name, sizeof(Char) * len2);
				path2[len + len2] = '/';
				path2[len + len2 + 1] = 0;
				if (!ForEachDirRecursion(path2, recursion, callback, data))
				{
					continue_loop = False;
					break;
				}
			}
		} while (FindNextFile(handle, &find_data));
	}
	FindClose(handle);
	return continue_loop;
}

static Bool DelDirRecursion(const Char* path)
{
	size_t len = wcslen(path);
	if (len > KUIN_MAX_PATH)
		return False;
	if (!PathFileExists(path))
		return True;
	WIN32_FIND_DATA find_data;
	HANDLE handle;
	Bool continue_loop = True;
	{
		Char path2[KUIN_MAX_PATH + 2];
		memcpy(path2, path, sizeof(Char) * len);
		path2[len] = L'*';
		path2[len + 1] = 0;
		handle = FindFirstFile(path2, &find_data);
	}
	if (handle == INVALID_HANDLE_VALUE)
		return False;
	do
	{
		Char* name = find_data.cFileName;
		if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			size_t len2 = wcslen(name);
			if (len + len2 + 1 > KUIN_MAX_PATH + 1)
			{
				continue_loop = False;
				break;
			}
			Char path2[KUIN_MAX_PATH + 1];
			memcpy(path2, path, sizeof(Char) * len);
			memcpy(path2 + len, name, sizeof(Char) * (len2 + 1));
			if (!DeleteFile(path2))
			{
				continue_loop = False;
				break;
			}
		}
		else
		{
			if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
				continue;
			size_t len2 = wcslen(name);
			if (len + len2 + 2 > KUIN_MAX_PATH + 2)
			{
				continue_loop = False;
				break;
			}
			Char path2[KUIN_MAX_PATH + 2];
			memcpy(path2, path, sizeof(Char) * len);
			memcpy(path2 + len, name, sizeof(Char) * len2);
			path2[len + len2] = '/';
			path2[len + len2 + 1] = 0;
			if (!DelDirRecursion(path2))
			{
				continue_loop = False;
				break;
			}
		}
	} while (FindNextFile(handle, &find_data));
	FindClose(handle);
	if (continue_loop)
		continue_loop = RemoveDirectory(path) != 0;
	return continue_loop;
}

static Bool CopyDirRecursion(const Char* dst, const Char* src)
{
	size_t len_src = wcslen(src);
	if (len_src > KUIN_MAX_PATH)
		return False;
	WIN32_FIND_DATA find_data;
	HANDLE handle;
	Bool continue_loop = True;
	{
		Char src2[KUIN_MAX_PATH + 2];
		memcpy(src2, src, sizeof(Char) * len_src);
		src2[len_src] = L'*';
		src2[len_src + 1] = 0;
		handle = FindFirstFile(src2, &find_data);
	}
	if (handle == INVALID_HANDLE_VALUE)
		return False;
	if (!PathFileExists(dst))
	{
		if (!CreateDirectory(dst, NULL))
			continue_loop = False;
	}
	if (continue_loop)
	{
		do
		{
			Char* name = find_data.cFileName;
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				size_t len_name = wcslen(name);
				size_t len_dst = wcslen(dst);
				if (len_src + len_name + 1 > KUIN_MAX_PATH + 1 || len_dst + len_name + 1 > KUIN_MAX_PATH + 1)
				{
					continue_loop = False;
					break;
				}
				Char dst2[KUIN_MAX_PATH + 1];
				Char src2[KUIN_MAX_PATH + 1];
				memcpy(src2, src, sizeof(Char) * len_src);
				memcpy(src2 + len_src, name, sizeof(Char) * (len_name + 1));
				memcpy(dst2, dst, sizeof(Char) * len_src);
				memcpy(dst2 + len_src, name, sizeof(Char) * (len_name + 1));
				if (!CopyFile(src2, dst2, FALSE))
				{
					continue_loop = False;
					break;
				}
			}
			else
			{
				if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
					continue;
				size_t len_name = wcslen(name);
				size_t len_dst = wcslen(dst);
				if (len_src + len_name + 2 > KUIN_MAX_PATH + 2 || len_dst + len_name + 2 > KUIN_MAX_PATH + 2)
				{
					continue_loop = False;
					break;
				}
				Char dst2[KUIN_MAX_PATH + 2];
				Char src2[KUIN_MAX_PATH + 2];
				memcpy(src2, src, sizeof(Char) * len_src);
				memcpy(src2 + len_src, name, sizeof(Char) * len_name);
				src2[len_src + len_name] = '/';
				src2[len_src + len_name + 1] = 0;
				memcpy(dst2, dst, sizeof(Char) * len_src);
				memcpy(dst2 + len_src, name, sizeof(Char) * len_name);
				dst2[len_src + len_name] = '/';
				dst2[len_src + len_name + 1] = 0;
				if (!CopyDirRecursion(dst2, src2))
				{
					continue_loop = False;
					break;
				}
			}
		} while (FindNextFile(handle, &find_data));
	}
	FindClose(handle);
	return continue_loop;
}

static void NormPath(Char* path, Bool dir)
{
	if (*path == L'\0')
		return;
	do
	{
		if (*path == L'\\')
			*path = L'/';
		path++;
	} while (*path != L'\0');
	if (dir && path[-1] != L'/')
	{
		path[0] = L'/';
		path[1] = L'\0';
	}
}
