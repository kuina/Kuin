//
// bin_to_text.exe
//
// (C)Kuina-chan
//

#include "main.h"

#define CHAR_PER_LINE (32)

int wmain(int argc, Char** argv)
{
	int i;
	if (argc <= 1)
	{
		wprintf(L"Usage: bin_to_text [file1] [file2] ...\n");
		return 0;
	}
	for (i = 1; i < argc; i++)
	{
		Char name[MAX_PATH];
		FILE* file_ptr = _wfopen(argv[i], L"rb");
		size_t size;
		if (file_ptr == NULL)
		{
			wprintf(L"The file '%s' could not be opened.\n", argv[i]);
			return 1;
		}
		{
			const Char* ptr = wcsrchr(argv[i], L'\\') + 1;
			Char* ptr2;
			wcscpy(name, ptr);
			ptr2 = name;
			while (*ptr2 != L'\0')
			{
				if (*ptr2 == L'.')
					*ptr2 = L'_';
				ptr2++;
			}
			wcscat(name, L"_bin");
		}
		fseek(file_ptr, 0, SEEK_END);
		size = (size_t)ftell(file_ptr);
		fseek(file_ptr, 0, SEEK_SET);
		{
			Char path[MAX_PATH];
			FILE* file_ptr2;
			wcscpy(path, argv[i]);
			{
				Char* ptr = wcsrchr(path, L'\\') + 1;
				while (*ptr != L'\0')
				{
					if (*ptr == L'.')
						*ptr = L'_';
					ptr++;
				}
			}

			Char upper_name[MAX_PATH];
			{
				const Char* ptr = name;
				Char* ptr2 = upper_name;
				Bool first = True;
				while (*ptr != L'\0')
				{
					if (*ptr == '_')
					{
						first = True;
						ptr++;
						continue;
					}
					if (first && 'a' <= *ptr && *ptr <= 'z')
						*ptr2 = *ptr - 'a' + 'A';
					else
						*ptr2 = *ptr;
					first = False;
					ptr++;
					ptr2++;
				}
				*ptr2 = L'\0';
			}

			wcscat(path, L".c");
			file_ptr2 = _wfopen(path, L"w, ccs=UTF-8");
			fwprintf(file_ptr2, L"#include \"../common.h\"\n");
			fwprintf(file_ptr2, L"\n");
			fwprintf(file_ptr2, L"const U8* Get%s(size_t* size)\n", upper_name);
			fwprintf(file_ptr2, L"{\n");
			fwprintf(file_ptr2, L"\tstatic const U8 %s[0x%08x] =\n", name, (U32)size);
			fwprintf(file_ptr2, L"\t{\n");
			{
				size_t j = 0;
				for (j = 0; j < size; j++)
				{
					U8 bin;
					fread(&bin, 1, 1, file_ptr);
					if (j % CHAR_PER_LINE == 0)
						fwprintf(file_ptr2, L"\t\t0x%02x", bin);
					else if (j % CHAR_PER_LINE == CHAR_PER_LINE - 1)
						fwprintf(file_ptr2, L", 0x%02x,\n", bin);
					else
						fwprintf(file_ptr2, L", 0x%02x", bin);
				}
				if (size % CHAR_PER_LINE != 0)
					fwprintf(file_ptr2, L",\n");
			}
			fwprintf(file_ptr2, L"\t};\n");
			fwprintf(file_ptr2, L"\n");
			fwprintf(file_ptr2, L"\t*size = 0x%08x;\n", (U32)size);
			fwprintf(file_ptr2, L"\treturn %s;\n", name);
			fwprintf(file_ptr2, L"}\n");
			fclose(file_ptr2);
		}
		fclose(file_ptr);
	}
	return 0;
}
