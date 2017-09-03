#include "util.h"

#include "mem.h"

int BinSearch(const Char** hay_stack, int num, const Char* needle)
{
	int min = 0;
	int max = num - 1;
	while (min <= max)
	{
		int mid = (min + max) / 2;
		int cmp = wcscmp(needle, hay_stack[mid]);
		if (cmp < 0)
			max = mid - 1;
		else if (cmp > 0)
			min = mid + 1;
		else
			return mid;
	}
	return -1;
}

const Char* NewStr(int* len, const Char* format, ...)
{
	int size;
	Char* buf;
	va_list arg;
	va_start(arg, format);
	size = _vscwprintf(format, arg);
	buf = (Char*)Alloc(sizeof(Char) * (size_t)(size + 1));
	vswprintf(buf, size + 1, format, arg);
	va_end(arg);
	if (len != NULL)
		*len = size;
	return buf;
}

const Char* SubStr(const Char* str, int start, int len)
{
	Char* buf = (Char*)Alloc(sizeof(Char) * (size_t)(len + 1));
	int i;
	for (i = 0; i < len; i++)
	{
		ASSERT(str[start + i] != L'\0');
		buf[i] = str[start + i];
	}
	buf[len] = L'\0';
	return buf;
}

S64* NewAddr(void)
{
	S64* result = (S64*)Alloc(sizeof(S64));
	*result = -1;
	return result;
}

U8* StrToBin(const Char* str, int* size)
{
	size_t len = wcslen(str);
	U8* bin = (U8*)Alloc(len);
	size_t i;
	for (i = 0; i < len; i++)
		bin[i] = (U8)str[i];
	*size = (int)len;
	return bin;
}

Bool CmpData(int size1, const U8* data1, int size2, const U8* data2)
{
	int i;
	if (size1 != size2)
		return False;
	for (i = 0; i < size1; i++)
	{
		if (data1[i] != data2[i])
			return False;
	}
	return True;
}

void ReadFileLine(Char* buf, int size, FILE* file_ptr)
{
	fgetws(buf, size, file_ptr);
	{
		Char* ptr = buf;
		while (ptr[1] != L'\0')
			ptr++;
		while (ptr >= buf && (*ptr == L'\n' || *ptr == L'\r'))
		{
			*ptr = L'\0';
			ptr--;
		}
	}
}

Char* GetDir(const Char* path, Bool dir, const Char* add_name)
{
	size_t len = wcslen(path);
	size_t len_add_name = add_name == NULL ? 0 : wcslen(add_name);
	Char* result;
	if (len == 0)
	{
		result = (Char*)Alloc(sizeof(Char) * (2 + len_add_name + 1));
		wcscpy(result, L"./");
		return result;
	}
	if (dir)
	{
		Bool backslash_eol = path[len - 1] == L'\\' || path[len - 1] == L'/';
		size_t i;
		result = (Char*)Alloc(sizeof(Char) * (len + len_add_name + (backslash_eol ? 0 : 1) + 1));
		for (i = 0; i < len; i++)
			result[i] = path[i] == L'\\' ? L'/' : path[i];
		if (!backslash_eol)
		{
			result[i] = L'/';
			i++;
		}
		result[i] = L'\0';
	}
	else
	{
		const Char* ptr = path + len - 1;
		while (ptr != path && *ptr != L'\\' && *ptr != L'/')
			ptr--;
		if (ptr == path)
		{
			result = (Char*)Alloc(sizeof(Char) * (2 + len_add_name + 1));
			wcscpy(result, L"./");
		}
		else
		{
			size_t len2 = ptr - path + 1;
			size_t i;
			result = (Char*)Alloc(sizeof(Char) * (len2 + len_add_name + 1));
			for (i = 0; i < len2; i++)
				result[i] = path[i] == L'\\' ? L'/' : path[i];
			result[i] = L'\0';
		}
	}
	if (len_add_name != 0)
		wcscat(result, add_name);
	return result;
}

Bool DelDir(const Char* path)
{
	Char path2[1024];
	Char path3[1024];
	if (wcslen(path) > MAX_PATH)
		return False;
	wcscpy(path2, path);
	{
		size_t len = wcslen(path2);
		if (path2[len - 1] != L'\\' && path2[len - 1] != L'/')
			wcscat(path2, L"/");
	}
	if (!PathFileExists(path2))
		return True;
	wcscpy(path3, path2);
	wcscat(path3, L"*");
	{
		WIN32_FIND_DATA find_data;
		HANDLE handle = FindFirstFile(path3, &find_data);
		if (handle == INVALID_HANDLE_VALUE)
			return False;
		do
		{
			if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0)
				continue;
			{
				Char path4[1024];
				wcscpy(path4, path2);
				wcscat(path4, find_data.cFileName);
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					if (!DelDir(path4))
					{
						FindClose(handle);
						return False;
					}
				}
				else
				{
					if (DeleteFile(path4) == 0)
					{
						FindClose(handle);
						return False;
					}
				}
			}
		} while (FindNextFile(handle, &find_data));
		FindClose(handle);
	}
	return RemoveDirectory(path2) != 0;
}

const Char* CharToStr(Char c)
{
	switch (c)
	{
		case L'\n':
			return L"(Return)";
		case L'\t':
		case L' ':
			return L"(Space)";
	}
	{
		Char* result = (Char*)Alloc(sizeof(Char) * 2);
		result[0] = c;
		result[1] = L'\0';
		return result;
	}
}
