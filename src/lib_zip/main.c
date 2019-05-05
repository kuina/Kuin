// LibZip.dll
//
// (C)Kuina-chan
//

#include "main.h"

#define TMP_BUF_SIZE 0x4000
#define EOR_LOCATOR_SIZE 512

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

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	if (!InitEnvVars(heap, heap_cnt, app_code, use_res_flags))
		return;
}

EXPORT Bool _zip(const U8* out_path, const U8* path, S64 compression_level)
{
	THROWDBG(out_path == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(!(compression_level == -1 || 1 <= compression_level && compression_level <= 9), EXCPT_DBG_ARG_OUT_DOMAIN);
	Char path2[KUIN_MAX_PATH + 1];
	if (GetFullPathName((const Char*)(path + 0x10), KUIN_MAX_PATH, path2, NULL) == 0)
		return False;
	size_t len = wcslen(path2);
	const Char* slash_pos;
	THROWDBG(len < 2 || path2[len - 1] != L'\\', EXCPT_DBG_ARG_OUT_DOMAIN);
	slash_pos = path2 + len - 2;
	while (slash_pos > path2 && *slash_pos != L'\\')
		slash_pos--;
	int file_num = 0;
	if (!Search(path2, CntFileNum, &file_num))
		return False;
	char** file_names = (char**)AllocMem(sizeof(char*) * (size_t)file_num);
	int root_path_len = WideCharToMultiByte(CP_ACP, 0, path2, (int)(slash_pos - path2 + 1), NULL, 0, NULL, NULL);
	{
		int tmp_num = 0;
		void* param[2];
		param[0] = file_names;
		param[1] = &tmp_num;
		if (!Search(path2, GetFileName, param))
		{
			FreeMem(file_names);
			return False;
		}
	}

	ZipHeader* zip_header = (ZipHeader*)AllocMem(sizeof(ZipHeader) * (size_t)file_num);
	memset(zip_header, 0, sizeof(ZipHeader) * (size_t)file_num);
	CentralDirHeader* central_dir_header = (CentralDirHeader*)AllocMem(sizeof(CentralDirHeader) * (size_t)file_num);
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
		z_stream stream = { 0 };
		deflateInit2(&stream, compression_level == -1 ? Z_DEFAULT_COMPRESSION : (int)compression_level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);

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
			if (file_names[i][strlen(file_names[i]) - 1] == L'/' || file_names[i][strlen(file_names[i]) - 1] == L'\\')
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
					FreeMem(zip_header);
					FreeMem(central_dir_header);
					for (i = 0; i < file_num; i++)
						FreeMem(file_names[i]);
					FreeMem(file_names);
					return False;
				}
				fseek(file_ptr2, 0, SEEK_END);
				long file_size = ftell(file_ptr2);
				fseek(file_ptr2, 0, SEEK_SET);
				void* data = AllocMem((size_t)file_size);
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
				FreeMem(data);
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
	FreeMem(zip_header);
	FreeMem(central_dir_header);
	for (i = 0; i < file_num; i++)
		FreeMem(file_names[i]);
	FreeMem(file_names);
	return True;
}

EXPORT Bool _unzip(const U8* out_path, const U8* path)
{
	THROWDBG(out_path == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(path == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(((const Char*)(out_path + 0x10))[*(S64*)(out_path + 0x08) - 1] != '/', EXCPT_DBG_ARG_OUT_DOMAIN);
	void* file_ptr = OpenFileStream((const Char*)(path + 0x10));
	if (file_ptr == NULL)
		return False;

	Bool success = False;
	void* dir_image = NULL;
	void* src_buf = NULL;
	void* dst_buf = NULL;
	for (; ; )
	{
		U32 directory_entries;
		size_t dir_size;

		{
			U8 locator[EOR_LOCATOR_SIZE];
			if (!SeekFileStream(file_ptr, -EOR_LOCATOR_SIZE, SEEK_END))
				break;
			if (ReadFileStream(file_ptr, EOR_LOCATOR_SIZE, locator) != EOR_LOCATOR_SIZE)
				break;
			const U8* ptr = locator;
			const U8* end_ptr = ptr + EOR_LOCATOR_SIZE - sizeof(EndCentralDirHeader) + sizeof(U32);
			while (ptr < end_ptr)
			{
				if (*ptr == 'P' && *(U32*)ptr == 0x06054b50)
					break;
				ptr++;
			}
			if (ptr >= end_ptr)
				break;

			const EndCentralDirHeader* eor = (EndCentralDirHeader*)ptr;
			dir_size = (size_t)eor->CentralDirSize;
			directory_entries = eor->CentralDirRecordsTotalNum;
			dir_image = AllocMem(dir_size);
			SeekFileStream(file_ptr, eor->CentralDirOffset, SEEK_SET);
			if (ReadFileStream(file_ptr, dir_size, dir_image) != dir_size)
				break;
		}

		{
			char prev_path[KUIN_MAX_PATH + 1];
			prev_path[0] = '\0';
			const CentralDirHeader* dir_ptr = (CentralDirHeader*)dir_image;
			while ((const U8*)dir_ptr < (U8*)dir_image + dir_size)
			{
				if (dir_ptr->Signature != 0x02014b50)
					break;
				if (dir_ptr->FileNameLen >= KUIN_MAX_PATH)
					break;
				char name[KUIN_MAX_PATH + 1];
				memcpy(name, dir_ptr + 1, (size_t)dir_ptr->FileNameLen);
				name[dir_ptr->FileNameLen] = '\0';
				if (name[dir_ptr->FileNameLen - 1] != '/' && name[dir_ptr->FileNameLen - 1] != '\\')
				{
					const char* last_path = NULL;
					const char* ptr = name;
					while (*ptr != '\0')
					{
						if (*ptr == '/' || *ptr == '\\')
							last_path = ptr + 1;
						ptr++;
					}
					if (last_path != NULL)
					{
						size_t size = last_path - name;
						if (memcmp(prev_path, name, size) != 0)
						{
							memcpy(prev_path, name, size);
							prev_path[size] = '\0';
							const char* ptr2 = prev_path;
							char dir_path[KUIN_MAX_PATH + 1];
							char* ptr_dir_path = dir_path;
							Bool failure = False;
							while (*ptr2 != '\0')
							{
								if (*ptr2 == '/' || *ptr2 == '\\')
								{
									*ptr_dir_path = '\0';
									Char dir_path2[KUIN_MAX_PATH + 1];
									if (MultiByteToWideChar(CP_ACP, 0, dir_path, -1, dir_path2, KUIN_MAX_PATH) == 0)
									{
										failure = True;
										break;
									}
									{
										Char dir_path3[KUIN_MAX_PATH * 2 + 1];
										wcscpy(dir_path3, (const Char*)(out_path + 0x10));
										wcscat(dir_path3, dir_path2);
										if (GetFullPathName(dir_path3, KUIN_MAX_PATH, dir_path2, NULL) == 0)
										{
											failure = True;
											break;
										}
									}
									if (PathFileExists(dir_path2) == 0)
									{
										if (SHCreateDirectory(NULL, dir_path2) != ERROR_SUCCESS)
										{
											failure = True;
											break;
										}
									}
								}
								*ptr_dir_path = *ptr2;
								ptr_dir_path++;
								ptr2++;
							}
							if (failure)
								break;
						}
					}
					{
						ZipHeader header;
						if (!SeekFileStream(file_ptr, dir_ptr->HeaderOffset, SEEK_SET))
							break;
						if (ReadFileStream(file_ptr, sizeof(ZipHeader), &header) != sizeof(ZipHeader))
							break;
						if (header.Signature != 0x04034b50)
							break;
						size_t local_offset = sizeof(ZipHeader) + header.FileNameLen + header.ExtraFieldLen;

						size_t src_buf_size = (size_t)dir_ptr->CompressedSize;
						src_buf = AllocMem(src_buf_size);
						if (!SeekFileStream(file_ptr, (S64)dir_ptr->HeaderOffset + (S64)local_offset, SEEK_SET))
							break;
						if (ReadFileStream(file_ptr, src_buf_size, src_buf) != src_buf_size)
							break;

						size_t dst_buf_size = (size_t)dir_ptr->UncompressedSize;
						dst_buf = AllocMem(dst_buf_size);
						if (dir_ptr->CompressionMethod == 0)
						{
							if (dir_ptr->CompressedSize != dir_ptr->UncompressedSize)
								break;
							memcpy(dst_buf, src_buf, dst_buf_size);
						}
						else if (dir_ptr->CompressionMethod == 8)
						{
							z_stream stream = { 0 };
							stream.next_in = (Bytef*)src_buf;
							stream.avail_in = (uInt)src_buf_size;
							stream.next_out = (Bytef*)dst_buf;
							stream.avail_out = (uInt)dst_buf_size;
							if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
								break;
							int err;
							err = inflate(&stream, Z_FINISH);
							if (err != Z_STREAM_END && err != Z_OK)
								break;
							err = inflateEnd(&stream);
							if (stream.total_out != dst_buf_size)
								break;
						}
						else
							break;
						if (crc32(0, dst_buf, dir_ptr->UncompressedSize) != dir_ptr->Crc32)
							break;

						{
							Char path2[KUIN_MAX_PATH + 1];
							if (MultiByteToWideChar(CP_ACP, 0, name, -1, path2, KUIN_MAX_PATH) == 0)
								break;
							Char path3[KUIN_MAX_PATH * 2 + 1];
							wcscpy(path3, (const Char*)(out_path + 0x10));
							wcscat(path3, path2);
							FILE* file_ptr2 = _wfopen(path3, L"wb");
							if (file_ptr2 == NULL)
								break;
							fwrite(dst_buf, 1, dst_buf_size, file_ptr2);
							FreeMem(dst_buf);
							dst_buf = NULL;
							FreeMem(src_buf);
							src_buf = NULL;
							fclose(file_ptr2);
						}
					}
				}
				dir_ptr = (const CentralDirHeader*)((const U8*)(dir_ptr + 1) + dir_ptr->FileNameLen + dir_ptr->ExtraFieldLen + dir_ptr->FileCommentLen);
			}
		}

		success = True;
		break;
	}

	if (dst_buf != NULL)
		FreeMem(dst_buf);
	if (src_buf != NULL)
		FreeMem(src_buf);
	if (dir_image != NULL)
		FreeMem(dir_image);
	CloseFileStream(file_ptr);
	return success;
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
	file_names[*num] = (char*)AllocMem((size_t)len);
	WideCharToMultiByte(CP_ACP, 0, path, -1, file_names[*num], len, NULL, NULL);
	(*num)++;
}
