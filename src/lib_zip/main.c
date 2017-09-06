// LibZip.dll
//
// (C)Kuina-chan
//

#include "main.h"

#define TMP_BUF_SIZE (0x4000)

#pragma pack(push, 2)

typedef struct ZipHeader
{
	U32 Signature;
	U16 MinVersion;
	U16 BitFlag;
	U16 CompressionMethod;
	U16 ModificationTime;
	U16 ModificationDate;
	U32 Crc32;
	U32 CompressedSize;
	U32 UncompressedSize;
	U16 FileNameLen;
	U16 ExtraFieldLen;
} ZipHeader;

typedef struct CentralDirHeader
{
	U32 Signature;
	U16 Version;
	U16 MinVersion;
	U16 BitFlag;
	U16 CompressionMethod;
	U16 ModificationTime;
	U16 ModificationDate;
	U32 Crc32;
	U32 CompressedSize;
	U32 UncompressedSize;
	U16 FileNameLen;
	U16 ExtraFieldLen;
	U16 FileCommentLen;
	U16 DiskNum;
	U16 InternalFileAttr;
	U32 ExternalFileAttr;
	U32 HeaderOffset;
} CentralDirHeader;

typedef struct EndCentralDirHeader
{
	U32 Signature;
	U16 DiskNum;
	U16 StartDisk;
	U16 CentralDirRecordsNum;
	U16 CentralDirRecordsTotalNum;
	U32 CentralDirSize;
	U32 CentralDirOffset;
	U16 CommentLen;
} EndCentralDirHeader;

#pragma pack(pop)

static Bool Search(const Char* path, void(*func)(const Char*, Bool, void*), void* param);
static void CntFileNum(const Char* path, Bool is_dir, void* param);
static void GetFileName(const Char* path, Bool is_dir, void* param);

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
	UNUSED(hinst);
	UNUSED(reason);
	UNUSED(reserved);
	return TRUE;
}

EXPORT Bool _zip(const U8* out_path, const U8* path, S64 compression_level)
{
	THROWDBG(out_path == NULL, 0xc0000005);
	THROWDBG(path == NULL, 0xc0000005);
	THROWDBG(!(compression_level == -1 || 1 <= compression_level && compression_level <= 9), 0xe9170006);
	const Char* path2 = (const Char*)(path + 0x10);
	const Char* slash_pos;
	THROWDBG(*(const S64*)(path + 0x08) < 2, 0xe9170006);
	slash_pos = path2 + *(const S64*)(path + 0x08) - 2;
	while (slash_pos > path2 && *slash_pos != L'/')
		slash_pos--;
	THROWDBG(slash_pos == path2, 0xe9170006);
	int file_num = 0;
	if (!Search(path2, CntFileNum, &file_num))
		return False;
	char** file_names = (char**)malloc(sizeof(char*) * (size_t)file_num);
	int root_path_len = WideCharToMultiByte(CP_ACP, 0, path2, (int)(slash_pos - path2 + 1), NULL, 0, NULL, NULL);
	{
		int tmp_num = 0;
		void* param[2];
		param[0] = file_names;
		param[1] = &tmp_num;
		if (!Search(path2, GetFileName, param))
		{
			free(file_names);
			return False;
		}
	}

	ZipHeader* zip_header = (ZipHeader*)malloc(sizeof(ZipHeader) * (size_t)file_num);
	memset(zip_header, 0, sizeof(ZipHeader) * (size_t)file_num);
	CentralDirHeader* central_dir_header = (CentralDirHeader*)malloc(sizeof(CentralDirHeader) * (size_t)file_num);
	memset(central_dir_header, 0, sizeof(CentralDirHeader) * (size_t)file_num);

	int i, j;
	for (i = 0; i < file_num; i++)
	{
		zip_header[i].Signature = 0x04034b50;
		zip_header[i].MinVersion = 20;
		zip_header[i].CompressionMethod = 8;
		{
			time_t now = time(NULL);
			struct tm* t = localtime(&now);
			zip_header[i].ModificationTime = (U16)(((U32)t->tm_hour << 11) | ((U32)t->tm_min << 5) | ((U32)t->tm_sec >> 1));
			zip_header[i].ModificationDate = (U16)(((U32)(t->tm_year - 80) << 9) | ((U32)t->tm_mon << 5) | ((U32)t->tm_mday));
		}
		zip_header[i].FileNameLen = (U16)(strlen(file_names[i]) - root_path_len);
		central_dir_header[i].Signature = 0x02014b50;
		central_dir_header[i].Version = 20;
		central_dir_header[i].MinVersion = 20;
		central_dir_header[i].CompressionMethod = 8;
		central_dir_header[i].ModificationTime = zip_header[i].ModificationTime;
		central_dir_header[i].ModificationDate = zip_header[i].ModificationDate;
		central_dir_header[i].FileNameLen = zip_header[i].FileNameLen;
	}

	FILE* file_ptr = _wfopen((const Char*)(out_path + 0x10), L"wb");
	if (file_ptr != NULL)
	{
		z_stream stream;
		memset(&stream, 0, sizeof(z_stream));
		deflateInit2(&stream, compression_level == -1 ? Z_DEFAULT_COMPRESSION: (int)compression_level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);

		U32 crc32[256];
		U32 n;
		for (i = 0; i < 256; i++)
		{
			n = (U32)i;
			for (j = 0; j < 8; j++)
			{
				if ((n & 0x01) != 0)
					n = (n >> 1) ^ 0xedb88320;
				else
					n >>= 1;
			}
			crc32[i] = n;
		}

		long pos;
		Bytef tmp_buf[TMP_BUF_SIZE];
		for (i = 0; i < file_num; i++)
		{
			if (file_names[i][strlen(file_names[i]) - 1] == L'/')
			{
				central_dir_header[i].HeaderOffset = (U32)ftell(file_ptr);
				fwrite(&zip_header[i], sizeof(ZipHeader), 1, file_ptr);
				fwrite(file_names[i] + root_path_len, sizeof(char), (size_t)zip_header[i].FileNameLen, file_ptr);
			}
			else
			{
				void* file_ptr2 = fopen(file_names[i], "rb");
				if (file_ptr2 == NULL)
				{
					fclose(file_ptr);
					free(zip_header);
					for (i = 0; i < file_num; i++)
						free(file_names[i]);
					free(file_names);
					return False;
				}
				fseek(file_ptr2, 0, SEEK_END);
				long file_size = ftell(file_ptr2);
				fseek(file_ptr2, 0, SEEK_SET);
				void* data = malloc((size_t)file_size);
				fread(data, 1, (size_t)file_size, file_ptr2);
				fclose(file_ptr2);

				zip_header[i].UncompressedSize = (U32)file_size;
				central_dir_header[i].HeaderOffset = (U32)ftell(file_ptr);
				fwrite(&zip_header[i], sizeof(ZipHeader), 1, file_ptr);
				fwrite(file_names[i] + root_path_len, sizeof(char), (size_t)zip_header[i].FileNameLen, file_ptr);
				pos = ftell(file_ptr);
				deflateReset(&stream);
				zip_header[i].Crc32 = 0xffffffff;
				stream.avail_in = (uInt)zip_header[i].UncompressedSize;
				stream.next_in = (Bytef*)data;
				for (j = 0; j < (int)stream.avail_in; j++)
					zip_header[i].Crc32 = (zip_header[i].Crc32 >> 8) ^ crc32[(U8)stream.next_in[j] ^ (U8)zip_header[i].Crc32];
				zip_header[i].Crc32 = ~zip_header[i].Crc32;
				do
				{
					stream.avail_out = TMP_BUF_SIZE;
					stream.next_out = tmp_buf;
					deflate(&stream, Z_FINISH);
					fwrite(tmp_buf, 1, (size_t)(TMP_BUF_SIZE - stream.avail_out), file_ptr);
				} while (stream.avail_out == 0);
				zip_header[i].CompressedSize = (U32)(ftell(file_ptr) - pos);
				pos = ftell(file_ptr);
				fseek(file_ptr, central_dir_header[i].HeaderOffset + 14, SEEK_SET);
				fwrite(&zip_header[i].Crc32, 4, 1, file_ptr);
				fwrite(&zip_header[i].CompressedSize, 4, 1, file_ptr);
				fseek(file_ptr, 0, SEEK_END);
				central_dir_header[i].Crc32 = zip_header[i].Crc32;
				central_dir_header[i].CompressedSize = zip_header[i].CompressedSize;
				central_dir_header[i].UncompressedSize = zip_header[i].UncompressedSize;
				free(data);
			}
		}
		deflateEnd(&stream);
		pos = ftell(file_ptr);
		for (i = 0; i < file_num; i++)
		{
			fwrite(&central_dir_header[i], sizeof(CentralDirHeader), 1, file_ptr);
			fwrite(file_names[i] + root_path_len, sizeof(char), (size_t)central_dir_header[i].FileNameLen, file_ptr);
		}

		EndCentralDirHeader end_header;
		memset(&end_header, 0, sizeof(EndCentralDirHeader));
		end_header.Signature = 0x06054b50;
		end_header.CentralDirRecordsNum = (U16)file_num;
		end_header.CentralDirRecordsTotalNum = (U16)file_num;
		end_header.CentralDirOffset = (U32)pos;
		end_header.CentralDirSize = (U32)(ftell(file_ptr) - pos);
		fwrite(&end_header, sizeof(EndCentralDirHeader), 1, file_ptr);

		fclose(file_ptr);
	}
	free(zip_header);
	for (i = 0; i < file_num; i++)
		free(file_names[i]);
	free(file_names);
	return True;
}

static Bool Search(const Char* path, void(*func)(const Char*, Bool, void*), void* param)
{
	Char path2[MAX_PATH + 1];
	if (wcslen(path) > MAX_PATH)
		return False;
	if (!PathFileExists(path))
		return False;
	wcscpy(path2, path);
	wcscat(path2, L"*");
	{
		WIN32_FIND_DATA find_data;
		HANDLE handle = FindFirstFile(path2, &find_data);
		if (handle == INVALID_HANDLE_VALUE)
			return False;
		do
		{
			if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0)
				continue;
			{
				wcscpy(path2, path);
				wcscat(path2, find_data.cFileName);
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					wcscat(path2, L"/");
					func(path2, True, param);
					if (!Search(path2, func, param))
						return False;
				}
				else
					func(path2, False, param);
			}
		} while (FindNextFile(handle, &find_data));
		FindClose(handle);
	}
	return True;
}

static void CntFileNum(const Char* path, Bool is_dir, void* param)
{
	UNUSED(path);
	UNUSED(is_dir);
	(*(int*)param)++;
}

static void GetFileName(const Char* path, Bool is_dir, void* param)
{
	UNUSED(is_dir);
	char** file_names = (char**)((void**)param)[0];
	int* num = (int*)((void**)param)[1];
	int len = WideCharToMultiByte(CP_ACP, 0, path, -1, NULL, 0, NULL, NULL);
	file_names[*num] = (char*)malloc((size_t)len);
	WideCharToMultiByte(CP_ACP, 0, path, -1, file_names[*num], len, NULL, NULL);
	(*num)++;
}
