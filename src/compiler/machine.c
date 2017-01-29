#include "machine.h"

#include "log.h"
#include "mem.h"

static const U8 DOSHeader[] =
{
	0x4d, 0x5a, // Magic number(MZ)
	0x90, 0x00, // Bytes on last page of file
	0x03, 0x00, // Pages in file
	0x00, 0x00, // Relocations
	0x04, 0x00, // Size of header in paragraphs
	0x00, 0x00, // Minimum extra paragraphs needed
	0xff, 0xff, // Maximum extra paragraphs needed
	0x00, 0x00, // Initial SS value
	0xb8, 0x00, // Initial SP value
	0x00, 0x00, // Checksum
	0x00, 0x00, // Initial IP value
	0x00, 0x00, // Initial CS value
	0x40, 0x00, // File address of relocation table
	0x00, 0x00, // Overlay number
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved words
	0x00, 0x00, // OEM identifier
	0x00, 0x00, // OEM information
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, // Reserved words
	0x80, 0x00, 0x00, 0x00, // File address of new exe header
	// MS-DOS
	0x0e, // push cs
	0x1f, // pop ds
	0xba, 0x0e, 0x00, // mov dx, 000eh
	0xb4, 0x09, // mov ah, 09h
	0xcd, 0x21, // int 21h
	0xb8, 0x01, 0x4c, // mov ax, 4c01h
	0xcd, 0x21, // int 21h
	// This program cannot be run in DOS mode.\r\n$
	0x54, 0x68, 0x69, 0x73, 0x20, 0x70, 0x72, 0x6f,
	0x67, 0x72, 0x61, 0x6d, 0x20, 0x63, 0x61, 0x6e,
	0x6e, 0x6f, 0x74, 0x20, 0x62, 0x65, 0x20, 0x72,
	0x75, 0x6e, 0x20, 0x69, 0x6e, 0x20, 0x44, 0x4f,
	0x53, 0x20, 0x6d, 0x6f, 0x64, 0x65, 0x2e, 0x0d,
	0x0a, 0x24,
};

static const U8 PEHeader[] =
{
	0x50, 0x45, 0x00, 0x00, // PE
	0x64, 0x86, // Machine = IMAGE_FILE_MACHINE_AMD64
	0x05, 0x00, // Number of sections
	0x00, 0x00, 0x00, 0x00, // Time date stamp
	0x00, 0x00, 0x00, 0x00, // Pointer to symbol table
	0x00, 0x00, 0x00, 0x00, // Number of symbols
	0xf0, 0x00, // Optional header size
	0x2e, 0x00, // Characteristics
	0x0b, 0x02, // Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC
	0x00, // Major linker version
	0x00, // Minor linker version
	0x00, 0x00, 0x00, 0x00, // Code size
	0x00, 0x00, 0x00, 0x00, // Initialized data size
	0x00, 0x00, 0x00, 0x00, // Uninitialized data size
	0x00, 0x10, 0x00, 0x00, // Entry point RVA
	0x00, 0x10, 0x00, 0x00, // Base of Code
	0x00, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00, // Image base
	0x00, 0x10, 0x00, 0x00, // Section alignment
	0x00, 0x02, 0x00, 0x00, // File alignment
	0x06, 0x00, // Major OS version
	0x00, 0x00, // Minor OS version
	0x00, 0x00, // Major image version
	0x00, 0x00, // Minor image version
	0x06, 0x00, // Major subsystem version
	0x00, 0x00, // Minor subsystem version
	0x00, 0x00, 0x00, 0x00, // Reserved
	0x00, 0x00, 0x00, 0x00, // Image size
	0x00, 0x04, 0x00, 0x00, // Header size
	0x00, 0x00, 0x00, 0x00, // File checksum
	0x00, 0x00, // Subsystem
	0x40, 0x81, // DLL flags
	0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, // Stack reserve size
	0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Stack commit size
	0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, // Heap reserve size
	0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Heap commit size
	0x00, 0x00, 0x00, 0x00, // Loader flags
	0x10, 0x00, 0x00, 0x00, // Number of data directories
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Export table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Import table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Resource table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Exception table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Certificate table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Base relocation table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Debug
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Copyright
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Global pointer
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Thread local strage table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Load configuration table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Bound import
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Import address table
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Delay import descriptor
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // CLI header
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved
};

static const U8 SectionHeader[] =
{
	0x2e, 0x74, 0x65, 0x78, 0x74, 0x00, 0x00, 0x00, // .text
	0x00, 0x00, 0x00, 0x00, // Misc
	0x00, 0x00, 0x00, 0x00, // Virtual address
	0x00, 0x00, 0x00, 0x00, // Size of raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to relocations
	0x00, 0x00, 0x00, 0x00, // Pointer to line numbers
	0x00, 0x00, // Number of relocations
	0x00, 0x00, // Number of line numbers
	0x20, 0x00, 0x00, 0x60, // Characteristics
	0x2e, 0x72, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, // .rdata
	0x00, 0x00, 0x00, 0x00, // Misc
	0x00, 0x00, 0x00, 0x00, // Virtual address
	0x00, 0x00, 0x00, 0x00, // Size of raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to relocations
	0x00, 0x00, 0x00, 0x00, // Pointer to line numbers
	0x00, 0x00, // Number of relocations
	0x00, 0x00, // Number of line numbers
	0x40, 0x00, 0x00, 0x40, // Characteristics
	0x2e, 0x70, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, // .pdata
	0x00, 0x00, 0x00, 0x00, // Misc
	0x00, 0x00, 0x00, 0x00, // Virtual address
	0x00, 0x00, 0x00, 0x00, // Size of raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to relocations
	0x00, 0x00, 0x00, 0x00, // Pointer to line numbers
	0x00, 0x00, // Number of relocations
	0x00, 0x00, // Number of line numbers
	0x40, 0x00, 0x00, 0x40, // Characteristics
	0x2e, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, // .data
	0x00, 0x00, 0x00, 0x00, // Misc
	0x00, 0x00, 0x00, 0x00, // Virtual address
	0x00, 0x00, 0x00, 0x00, // Size of raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to relocations
	0x00, 0x00, 0x00, 0x00, // Pointer to line numbers
	0x00, 0x00, // Number of relocations
	0x00, 0x00, // Number of line numbers
	0x40, 0x00, 0x00, 0xc0, // Characteristics
	0x2e, 0x72, 0x73, 0x72, 0x63, 0x00, 0x00, 0x00, // .rsrc
	0x00, 0x00, 0x00, 0x00, // Misc
	0x00, 0x00, 0x00, 0x00, // Virtual address
	0x00, 0x00, 0x00, 0x00, // Size of raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to raw data
	0x00, 0x00, 0x00, 0x00, // Pointer to relocations
	0x00, 0x00, 0x00, 0x00, // Pointer to line numbers
	0x00, 0x00, // Number of relocations
	0x00, 0x00, // Number of line numbers
	0x40, 0x00, 0x00, 0x40, // Characteristics
};

typedef struct SSection
{
	U64 Len;
	U64 DataPos;
	U64 DataSize;
	U64 ImgPos;
	U64 ImgSize;
} SSection;

static const SPackAsm* PackAsm;
static const SOption* Option;
static FILE* FilePtr;
static SSection Code;
static SSection ReadonlyData;
static SSection Excpt;
static SSection WritableData;
static SSection Res;
static S64 LookupLen;
static S64 ReadonlyDataLen;
static S64 ImportLen;

static void Write(U64 addr, size_t size, U64 data);
static void WriteCode(void);
static void WriteReadonlyData(void);
static void WriteFuncAddrRecursion(SAstClass* class_, S64 file_origin, S64 addr_origin);
static void WriteExcpt(void);
static const void* CalcWritableData(const Char* key, const void* value, void* param);
static void WriteRes(void);
static void WriteResRecursion(SList* res, long* data_entry_addrs, int* idx, S64 base_addr);

void ToMachineCode(const SPackAsm* pack_asm, const SOption* option)
{
	PackAsm = pack_asm;
	Option = option;
	FilePtr = _wfopen(option->OutputFile, L"wb");
	if (FilePtr == NULL)
	{
		Err(L"EK0004", NULL, option->OutputFile);
		return;
	}
	{
		fwrite(DOSHeader, 1, sizeof(DOSHeader), FilePtr);
		while (ftell(FilePtr) < 0x80)
			fputc(0x00, FilePtr);
		fwrite(PEHeader, 1, sizeof(PEHeader), FilePtr);
		fwrite(SectionHeader, 1, sizeof(SectionHeader), FilePtr);
		while (ftell(FilePtr) < 0x0400)
			fputc(0x00, FilePtr);
		{
			Code.DataPos = (U64)ftell(FilePtr);
			Code.ImgPos = 0x1000;
			WriteCode();
			Code.Len = (U64)ftell(FilePtr) - Code.DataPos;
			while (ftell(FilePtr) % 0x0200 != 0)
				fputc(0x00, FilePtr);
			Code.DataSize = (U64)ftell(FilePtr) - Code.DataPos;
			Code.ImgSize = Code.Len;
			if (Code.ImgSize % 0x1000 != 0)
				Code.ImgSize += 0x1000 - Code.ImgSize % 0x1000;
		}
		{
			ReadonlyData.DataPos = (U64)ftell(FilePtr);
			ReadonlyData.ImgPos = Code.ImgPos + Code.ImgSize;
			WriteReadonlyData();
			ReadonlyData.Len = (U64)ftell(FilePtr) - ReadonlyData.DataPos;
			while (ftell(FilePtr) % 0x0200 != 0)
				fputc(0x00, FilePtr);
			ReadonlyData.DataSize = (U64)ftell(FilePtr) - ReadonlyData.DataPos;
			ReadonlyData.ImgSize = ReadonlyData.Len;
			if (ReadonlyData.ImgSize % 0x1000 != 0)
				ReadonlyData.ImgSize += 0x1000 - ReadonlyData.ImgSize % 0x1000;
		}
		{
			Excpt.DataPos = (U64)ftell(FilePtr);
			Excpt.ImgPos = ReadonlyData.ImgPos + ReadonlyData.ImgSize;
			WriteExcpt();
			Excpt.Len = (U64)ftell(FilePtr) - Excpt.DataPos;
			while (ftell(FilePtr) % 0x0200 != 0)
				fputc(0x00, FilePtr);
			Excpt.DataSize = (U64)ftell(FilePtr) - Excpt.DataPos;
			Excpt.ImgSize = Excpt.Len;
			if (Excpt.ImgSize % 0x1000 != 0)
				Excpt.ImgSize += 0x1000 - Excpt.ImgSize % 0x1000;
		}
		{
			// 'WritableData' is virtual.
			WritableData.DataPos = 0;
			WritableData.ImgPos = Excpt.ImgPos + Excpt.ImgSize;
			{
				U64 addr = 0;
				DictForEach(PackAsm->WritableData, CalcWritableData, &addr);
				WritableData.Len = addr;
			}
			WritableData.DataSize = 0;
			WritableData.ImgSize = WritableData.Len;
			if (WritableData.ImgSize % 0x1000 != 0)
				WritableData.ImgSize += 0x1000 - WritableData.ImgSize % 0x1000;
		}
		{
			Res.DataPos = (U64)ftell(FilePtr);
			Res.ImgPos = WritableData.ImgPos + WritableData.ImgSize;
			WriteRes();
			Res.Len = (U64)ftell(FilePtr) - Res.DataPos;
			while (ftell(FilePtr) % 0x0200 != 0)
				fputc(0x00, FilePtr);
			Res.DataSize = (U64)ftell(FilePtr) - Res.DataPos;
			Res.ImgSize = Res.Len;
			if (Res.ImgSize % 0x1000 != 0)
				Res.ImgSize += 0x1000 - Res.ImgSize % 0x1000;
		}
	}
	{
		// Update the header information.
		const U64 top = 0x80; // The start address of 'PEHeader'
		Write(top + 0x0008, 4, (U64)(U32)time(NULL)); // 'Time date stamp'
		Write(top + 0x001a, 1, 0x09); // 'Major linker version'
		Write(top + 0x001b, 1, 0x11); // 'Minor linker version'
		Write(top + 0x001c, 4, Code.DataSize); // 'Code size'
		Write(top + 0x0020, 4, ReadonlyData.DataSize); // 'Initialized data size'
		Write(top + 0x0050, 4, Res.ImgPos + Res.ImgSize); // 'Image size'
		if (Option->Env == Env_Wnd)
			Write(top + 0x005c, 2, 0x02); // 'Subsystem' = 'GUI'
		else if (Option->Env == Env_Cui)
			Write(top + 0x005c, 2, 0x03); // 'Subsystem' = 'CUI'
		// 'Import table'
		Write(top + 0x0090, 4, ReadonlyData.ImgPos + (U64)(LookupLen + ReadonlyDataLen + LookupLen));
		Write(top + 0x0094, 4, (U64)ImportLen);
		// 'Resource table'
		Write(top + 0x0098, 4, Res.ImgPos);
		Write(top + 0x009c, 4, Res.Len);
		// 'Exception table'
		Write(top + 0x00a0, 4, Excpt.ImgPos);
		Write(top + 0x00a4, 4, Excpt.Len);
		// 'Import address table'
		Write(top + 0x00e8, 4, ReadonlyData.ImgPos);
		Write(top + 0x00ec, 4, LookupLen);
		// '.text'
		Write(top + 0x0110, 4, Code.Len); // 'Misc'
		Write(top + 0x0114, 4, Code.ImgPos); // 'Virtual address'
		Write(top + 0x0118, 4, Code.DataSize); // 'Size of raw data'
		Write(top + 0x011c, 4, Code.DataPos); // 'Pointer to raw data'
		// '.rdata'
		Write(top + 0x0138, 4, ReadonlyData.Len); // 'Misc'
		Write(top + 0x013c, 4, ReadonlyData.ImgPos); // 'Virtual address'
		Write(top + 0x0140, 4, ReadonlyData.DataSize); // 'Size of raw data'
		Write(top + 0x0144, 4, ReadonlyData.DataPos); // 'Pointer to raw data'
		// '.pdata'
		Write(top + 0x0160, 4, Excpt.Len); // 'Misc'
		Write(top + 0x0164, 4, Excpt.ImgPos); // 'Virtual address'
		Write(top + 0x0168, 4, Excpt.DataSize); // 'Size of raw data'
		Write(top + 0x016c, 4, Excpt.DataPos); // 'Pointer to raw data'
		// '.data'
		Write(top + 0x0188, 4, WritableData.Len); // 'Misc'
		Write(top + 0x018c, 4, WritableData.ImgPos); // 'Virtual address'
		// '.rsrc'
		Write(top + 0x01b0, 4, Res.Len); // 'Misc'
		Write(top + 0x01b4, 4, Res.ImgPos); // 'Virtual address'
		Write(top + 0x01b8, 4, Res.DataSize); // 'Size of raw data'
		Write(top + 0x01bc, 4, Res.DataPos); // 'Pointer to raw data'
	}
	{
		// Update reference addresses.
		SListNode* ptr = PackAsm->RefValueList->Top;
		while (ptr != NULL)
		{
			SRefValueAddr* ref_addr = (SRefValueAddr*)ptr->Data;
			{
				U64 addr = ref_addr->Relative ? ((U64)*ref_addr->Addr - (U64)ref_addr->Bottom) : (U64)*ref_addr->Addr;
				U64 pos = (U64)ref_addr->Pos - Code.ImgPos + Code.DataPos;
				ASSERT(Code.DataPos <= pos && pos < Code.DataPos + Code.DataSize);
				Write(pos, 4, addr);
			}
			ptr = ptr->Next;
		}
	}
	fclose(FilePtr);
}

static void Write(U64 addr, size_t size, U64 data)
{
	fseek(FilePtr, (long)addr, SEEK_SET);
	fwrite(&data, 1, size, FilePtr);
}

static void WriteCode(void)
{
	SListNode* ptr = PackAsm->Asms->Top;
	int idx = 0;
	while (ptr != NULL)
	{
		SAsm* asm_ = (SAsm*)ptr->Data;
		S64 addr = (S64)ftell(FilePtr) - (S64)Code.DataPos + (S64)Code.ImgPos;
		ASSERT(addr >= 0);
		{
			// Set all the function addresses.
			S64* func_addr = (S64*)DictISearch(PackAsm->FuncAddrs, (U64)idx);
			if (func_addr != NULL)
				*func_addr = addr;
		}
		{
			// Write machine language.
			SListNode* ptr2 = PackAsm->RefValueList->Bottom;
			WriteAsmBin(FilePtr, PackAsm->RefValueList, addr, asm_);
			// Set 'Bottom' of newly added 'RefValue'.
			if (ptr2 == NULL)
				ptr2 = PackAsm->RefValueList->Top;
			else
				ptr2 = ptr2->Next;
			{
				S64 addr2 = (S64)ftell(FilePtr) - (S64)Code.DataPos + (S64)Code.ImgPos;
				while (ptr2 != NULL)
				{
					((SRefValueAddr*)ptr2->Data)->Bottom = addr2;
					ptr2 = ptr2->Next;
				}
			}
		}
		ptr = ptr->Next;
		idx++;
	}
}

static void WriteReadonlyData(void)
{
	S64 padding;
	// Calculate 'LookupLen'
	{
		LookupLen = 0;
		{
			SListNode* ptr = PackAsm->DLLImport->Top;
			while (ptr != NULL)
			{
				LookupLen += (S64)(((SDLLImport*)ptr->Data)->Funcs->Len * 8 + 8);
				ptr = ptr->Next;
			}
		}
	}
	// Calculate 'ReadonlyDataLen'
	{
		padding = LookupLen % 16;
		if (padding != 0)
			padding = 16 - padding;
		ReadonlyDataLen = 0;
		{
			SListNode* ptr = PackAsm->ReadonlyData->Top;
			while (ptr != NULL)
			{
				SReadonlyData* data = (SReadonlyData*)ptr->Data;
				if (data->Align128 && ReadonlyDataLen % 16 != 0)
					ReadonlyDataLen += 16 - ReadonlyDataLen % 16;
				ReadonlyDataLen += (S64)data->BufSize;
				ptr = ptr->Next;
			}
		}
		if (ReadonlyDataLen % 8 != 0)
			ReadonlyDataLen += 8 - ReadonlyDataLen % 8;
		{
			SListNode* ptr = PackAsm->ClassTables->Top;
			while (ptr != NULL)
			{
				SClassTable* table = (SClassTable*)ptr->Data;
				ReadonlyDataLen += 0x08 + (S64)table->Class->FuncSize;
				ptr = ptr->Next;
			}
		}
		if (ReadonlyDataLen % 8 != 0)
			ReadonlyDataLen += 8 - ReadonlyDataLen % 8;
		ReadonlyDataLen += padding;
	}
	{
		// 'Lookup'
		S64 base_addr = (S64)ftell(FilePtr);
		S64 dll_pos = 0;
		SListNode* ptr = PackAsm->DLLImport->Top;
		while (ptr != NULL)
		{
			SDLLImport* dll = (SDLLImport*)ptr->Data;
			dll->Addr = dll_pos;
			dll_pos += (S64)dll->DLLNameSize + 1;
			if (dll_pos % 2 != 0)
				dll_pos++;
			{
				SListNode* ptr2 = dll->Funcs->Top;
				while (ptr2 != NULL)
				{
					SDLLImportFunc* func = (SDLLImportFunc*)ptr2->Data;
					*func->Addr = (S64)ReadonlyData.ImgPos + (S64)ftell(FilePtr) - base_addr;
					{
						U32 addr = (U32)((S64)ReadonlyData.ImgPos + LookupLen + ReadonlyDataLen + LookupLen + (S64)(20 * (PackAsm->DLLImport->Len + 1)) + dll_pos);
						fwrite(&addr, 1, 4, FilePtr);
					}
					fputc(0x00, FilePtr);
					fputc(0x00, FilePtr);
					fputc(0x00, FilePtr);
					fputc(0x00, FilePtr);
					dll_pos += 2 + (S64)func->FuncNameSize + 1;
					if (dll_pos % 2 != 0)
						dll_pos++;
					ptr2 = ptr2->Next;
				}
				// 'Null function'
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
			}
			ptr = ptr->Next;
		}
	}
	{
		// 'ReadonlyData'
		S64 base_addr = (S64)ftell(FilePtr);
		{
			// Write a blank.
			S64 i;
			for (i = 0; i < padding; i++)
				fputc(0x00, FilePtr);
		}
		{
			// Write all the literals.
			SListNode* ptr = PackAsm->ReadonlyData->Top;
			while (ptr != NULL)
			{
				SReadonlyData* data = (SReadonlyData*)ptr->Data;
				if (data->Align128)
				{
					while (ftell(FilePtr) % 16 != 0)
						fputc(0x00, FilePtr);
				}
				*data->Addr = (S64)ftell(FilePtr) - base_addr + ReadonlyData.ImgPos + LookupLen;
				fwrite(data->Buf, 1, (size_t)data->BufSize, FilePtr);
				ptr = ptr->Next;
			}
		}
		while (ftell(FilePtr) % 8 != 0)
			fputc(0x00, FilePtr);
		{
			// Write all the class tables.
			S64 class_pos = (S64)ftell(FilePtr) - base_addr;
			{
				SListNode* ptr = PackAsm->ClassTables->Top;
				while (ptr != NULL)
				{
					SClassTable* table = (SClassTable*)ptr->Data;
					*table->Addr = ReadonlyData.ImgPos + LookupLen + class_pos;
					class_pos += 0x08 + (S64)table->Class->FuncSize;
					ptr = ptr->Next;
				}
			}
			{
				SListNode* ptr = PackAsm->ClassTables->Top;
				int j;
				U64 blank = 0;
				while (ptr != NULL)
				{
					SClassTable* table = (SClassTable*)ptr->Data;
					U64 addr = table->Parent == NULL ? 0 : (U64)(*table->Parent - *table->Addr);
					S64 origin;
					fwrite(&addr, 1, 8, FilePtr);
					origin = (S64)ftell(FilePtr);
					for (j = 0; j < table->Class->FuncSize; j += 8)
						fwrite(&blank, 0, 8, FilePtr);
					WriteFuncAddrRecursion(table->Class, origin, *table->Addr + 0x08);
					fseek(FilePtr, (long)(origin + table->Class->FuncSize), SEEK_SET);
					ptr = ptr->Next;
				}
			}
		}
		while (ftell(FilePtr) % 8 != 0)
			fputc(0x00, FilePtr);
	}
	{
		// 'Lookup'
		S64 dll_pos = 0;
		SListNode* ptr = PackAsm->DLLImport->Top;
		while (ptr != NULL)
		{
			SDLLImport* dll = (SDLLImport*)ptr->Data;
			SListNode* ptr2 = dll->Funcs->Top;
			dll_pos += (S64)dll->DLLNameSize + 1;
			if (dll_pos % 2 != 0)
				dll_pos++;
			while (ptr2 != NULL)
			{
				SDLLImportFunc* func = (SDLLImportFunc*)ptr2->Data;
				{
					U32 addr = (U32)((S64)ReadonlyData.ImgPos + LookupLen + ReadonlyDataLen + LookupLen + (S64)(20 * (PackAsm->DLLImport->Len + 1)) + dll_pos);
					fwrite(&addr, 1, 4, FilePtr);
				}
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				dll_pos += 2 + (S64)func->FuncNameSize + 1;
				if (dll_pos % 2 != 0)
					dll_pos++;
				ptr2 = ptr2->Next;
			}
			// 'Null function'
			fputc(0x00, FilePtr);
			fputc(0x00, FilePtr);
			fputc(0x00, FilePtr);
			fputc(0x00, FilePtr);
			fputc(0x00, FilePtr);
			fputc(0x00, FilePtr);
			fputc(0x00, FilePtr);
			fputc(0x00, FilePtr);
			ptr = ptr->Next;
		}
	}
	{
		// 'DLL Import'
		S64 base_addr = (S64)ftell(FilePtr);
		{
			SListNode* ptr = PackAsm->DLLImport->Top;
			S64 lookup_pos = 0;
			S64 dll_pos = 0;
			while (ptr != NULL)
			{
				SDLLImport* dll = (SDLLImport*)ptr->Data;
				{
					U32 original_first_chunk = (U32)((S64)ReadonlyData.ImgPos + LookupLen + ReadonlyDataLen + lookup_pos);
					fwrite(&original_first_chunk, 1, 4, FilePtr);
				}
				// 'Time date stamp'
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				// 'Forwarder chain'
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				{
					U32 name = (U32)((S64)ReadonlyData.ImgPos + LookupLen + ReadonlyDataLen + LookupLen + (S64)(20 * (PackAsm->DLLImport->Len + 1)) + dll_pos);
					fwrite(&name, 1, 4, FilePtr);
					dll_pos += (S64)dll->DLLNameSize + 1;
					if (dll_pos % 2 != 0)
						dll_pos++;
					{
						SListNode* ptr2 = dll->Funcs->Top;
						while (ptr2 != NULL)
						{
							dll_pos += 2 + (S64)((SDLLImportFunc*)ptr2->Data)->FuncNameSize + 1;
							if (dll_pos % 2 != 0)
								dll_pos++;
							ptr2 = ptr2->Next;
						}
					}
				}
				{
					U32 first_chunk = (U32)((S64)ReadonlyData.ImgPos + lookup_pos);
					fwrite(&first_chunk, 1, 4, FilePtr);
				}
				lookup_pos += (S64)(dll->Funcs->Len * 8 + 8);
				ptr = ptr->Next;
			}
			{
				// 'Null'
				int i;
				for (i = 0; i < 20; i++)
					fputc(0x00, FilePtr);
			}
		}
		{
			SListNode* ptr = PackAsm->DLLImport->Top;
			ASSERT(ftell(FilePtr) % 2 == 0);
			while (ptr != NULL)
			{
				SDLLImport* dll = (SDLLImport*)ptr->Data;
				fwrite(dll->DllName, 1, (size_t)dll->DLLNameSize, FilePtr);
				fputc(0x00, FilePtr); // The terminating character.
				if (ftell(FilePtr) % 2 != 0)
					fputc(0x00, FilePtr);
				{
					SListNode* ptr2 = dll->Funcs->Top;
					while (ptr2 != NULL)
					{
						SDLLImportFunc* func = (SDLLImportFunc*)ptr2->Data;
						fputc(0x00, FilePtr);
						fputc(0x00, FilePtr);
						fwrite(func->FuncName, 1, (size_t)func->FuncNameSize, FilePtr);
						fputc(0x00, FilePtr); // The terminating character.
						if (ftell(FilePtr) % 2 != 0)
							fputc(0x00, FilePtr);
						ptr2 = ptr2->Next;
					}
				}
				ptr = ptr->Next;
			}
			while (ftell(FilePtr) % 4 != 0)
				fputc(0x00, FilePtr);
		}
		ImportLen = (S64)ftell(FilePtr) - base_addr;
	}
	{
		// 'Unwind'
		SListNode* ptr = PackAsm->ExcptTables->Top;
		while (ptr != NULL)
		{
			SExcptTable* table = (SExcptTable*)ptr->Data;
			U8 size_of_prologue;
			{
				S64 size = *((SAsm*)table->PostPrologue)->Addr - *((SAsm*)table->Begin)->Addr;
				size_of_prologue = (U8)size;
				ASSERT(size == (S64)size_of_prologue); // The size of 'Prologue' should fit in 'U8'.
			}
			table->Addr = (S64)ftell(FilePtr) - (S64)ReadonlyData.DataPos + (S64)ReadonlyData.ImgPos;
			fputc(table->TryScopes->Len == 0 ? 0x01 : 0x09, FilePtr); // 'Version and flags'
			fputc(size_of_prologue, FilePtr);
			if (table->StackSize == -1)
			{
				// For special functions of exception handling.
				fputc(0x0c, FilePtr); // 'Count of unwind codes'
				fputc(0x00, FilePtr); // 'Frame register'
				fputc(size_of_prologue, FilePtr); // 'Offset in prologue'
				fputc(0x64, FilePtr); // 'Unwind operation code and operation infomation'
				fputc(0x11, FilePtr);
				fputc(0x00, FilePtr);
				fputc(size_of_prologue, FilePtr);
				fputc(0x54, FilePtr);
				fputc(0x10, FilePtr);
				fputc(0x00, FilePtr);
				fputc(size_of_prologue, FilePtr);
				fputc(0x34, FilePtr);
				fputc(0x0e, FilePtr);
				fputc(0x00, FilePtr);
				fputc(size_of_prologue, FilePtr);
				fputc(0x72, FilePtr);
				fputc(0x1c, FilePtr);
				fputc(0xf0, FilePtr);
				fputc(0x1a, FilePtr);
				fputc(0xe0, FilePtr);
				fputc(0x18, FilePtr);
				fputc(0xd0, FilePtr);
				fputc(0x16, FilePtr);
				fputc(0xc0, FilePtr);
				fputc(0x14, FilePtr);
				fputc(0x70, FilePtr);
			}
			else
			{
				int stack_size = (table->StackSize - 8) / 8;
				if (stack_size > 0x0f)
				{
					fputc(0x02, FilePtr); // 'Count of unwind codes'
					fputc(0x00, FilePtr); // 'Frame register'
					fputc(size_of_prologue, FilePtr); // 'Offset in prologue'
					fputc(0x01, FilePtr); // 'Unwind operation code and operation information'
					{
						U16 stack_size2 = (U16)(table->StackSize / 8);
						fwrite(&stack_size2, 1, 2, FilePtr);
					}
				}
				else
				{
					fputc(0x01, FilePtr); // 'Count of unwind codes'
					fputc(0x00, FilePtr); // 'Frame register'
					fputc(size_of_prologue, FilePtr); // 'Offset in prologue'
					fputc((U8)(0x02 | (stack_size << 4)), FilePtr); // 'Unwind operation code and operation information'
					// 'None'
					fputc(0x00, FilePtr);
					fputc(0x00, FilePtr);
				}
			}
			if (table->TryScopes->Len != 0)
			{
				fwrite(PackAsm->ExcptFunc, 1, 4, FilePtr);
				fwrite(&table->TryScopes->Len, 1, 4, FilePtr);
				{
					// Write 'TryScopes' in reverse order.
					SListNode* ptr2 = table->TryScopes->Bottom;
					while (ptr2 != NULL)
					{
						SExcptTableTry* try_ = (SExcptTableTry*)ptr2->Data;
						ASSERT(*((SAsm*)try_->Begin)->Addr == (S64)(U32)*((SAsm*)try_->Begin)->Addr);
						fwrite(((SAsm*)try_->Begin)->Addr, 1, 4, FilePtr);
						ASSERT(*((SAsm*)try_->End)->Addr == (S64)(U32)*((SAsm*)try_->End)->Addr);
						fwrite(((SAsm*)try_->End)->Addr, 1, 4, FilePtr);
						if (try_->CatchFunc == NULL)
						{
							fputc(0x01, FilePtr);
							fputc(0x00, FilePtr);
							fputc(0x00, FilePtr);
							fputc(0x00, FilePtr);
						}
						else
							fwrite(try_->CatchFunc, 1, 4, FilePtr);
						fwrite(((SAsm*)try_->End)->Addr, 1, 4, FilePtr);
						ptr2 = ptr2->Prev;
					}
				}
			}
			ptr = ptr->Next;
		}
	}
}

static void WriteFuncAddrRecursion(SAstClass* class_, S64 file_origin, S64 addr_origin)
{
	if (((SAst*)class_)->RefItem != NULL)
		WriteFuncAddrRecursion((SAstClass*)((SAst*)class_)->RefItem, file_origin, addr_origin);
	{
		SListNode* ptr = class_->Items->Top;
		while (ptr != NULL)
		{
			SAstClassItem* item = (SAstClassItem*)ptr->Data;
			if (item->Def->TypeId == AstTypeId_Func)
			{
				ASSERT(item->Addr >= 0);
				{
					S64 addr = *((SAstFunc*)item->Def)->Addr;
					ASSERT(addr != -1 && addr != -2);
					addr -= addr_origin + item->Addr;
					fseek(FilePtr, (long)(file_origin + item->Addr), SEEK_SET);
					fwrite(&addr, 1, 8, FilePtr);
				}
			}
			ptr = ptr->Next;
		}
	}
}

static void WriteExcpt(void)
{
	SListNode* ptr = PackAsm->ExcptTables->Top;
	while (ptr != NULL)
	{
		SExcptTable* table = (SExcptTable*)ptr->Data;
		ASSERT(*((SAsm*)table->Begin)->Addr == (S64)(U32)*((SAsm*)table->Begin)->Addr);
		fwrite(((SAsm*)table->Begin)->Addr, 1, 4, FilePtr);
		ASSERT(*((SAsm*)table->End)->Addr == (S64)(U32)*((SAsm*)table->End)->Addr);
		fwrite(((SAsm*)table->End)->Addr, 1, 4, FilePtr);
		ASSERT(table->Addr == (S64)(U32)table->Addr);
		fwrite(&table->Addr, 1, 4, FilePtr);
		ptr = ptr->Next;
	}
}

static const void* CalcWritableData(const Char* key, const void* value, void* param)
{
	UNUSED(key);
	{
		U64* addr = (U64*)param;
		SWritableData* data = (SWritableData*)value;
		*data->Addr = WritableData.ImgPos + (S64)*addr;
		*addr += (U64)data->Size;
	}
	return value;
}

static void WriteRes(void)
{
	S64 base_addr = (S64)ftell(FilePtr);
	long* data_entry_addrs = (long*)Alloc(sizeof(long) * PackAsm->ResIconNum + 1);
	{
		int idx = 0;
		WriteResRecursion(PackAsm->ResEntries, data_entry_addrs, &idx, base_addr);
		if (idx != PackAsm->ResIconNum + 1)
		{
			Err(L"EK0008", NULL, Option->IconFile);
			return;
		}
	}
	{
		int i;
		for (i = 0; i < PackAsm->ResIconNum + 1; i++)
		{
			{
				U32 addr = (U32)((S64)ftell(FilePtr) - base_addr + Res.ImgPos);
				U32 size = i == PackAsm->ResIconNum ? (U32)(6 + 14 * PackAsm->ResIconNum) : (U32)PackAsm->ResIconBinSize[i];
				long pos = data_entry_addrs[i];
				long tmp = ftell(FilePtr);
				fseek(FilePtr, pos, SEEK_SET);
				fwrite(&addr, 1, 4, FilePtr);
				fwrite(&size, 1, 4, FilePtr);
				fseek(FilePtr, tmp, SEEK_SET);
			}
			if (i == PackAsm->ResIconNum)
			{
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x01, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x09, FilePtr);
				fputc(0x00, FilePtr);
				{
					int j;
					for (j = 0; j < PackAsm->ResIconNum; j++)
						fwrite(PackAsm->ResIconHeaderBins[j], 1, 14, FilePtr);
				}
			}
			else
				fwrite(PackAsm->ResIconBins[i], 1, (size_t)PackAsm->ResIconBinSize[i], FilePtr);
		}
	}
}

static void WriteResRecursion(SList* res, long* data_entry_addrs, int* idx, S64 base_addr)
{
	// 'Characteristics'
	fputc(0x00, FilePtr);
	fputc(0x00, FilePtr);
	fputc(0x00, FilePtr);
	fputc(0x00, FilePtr);
	// 'Time date stamp'
	fputc(0x00, FilePtr);
	fputc(0x00, FilePtr);
	fputc(0x00, FilePtr);
	fputc(0x00, FilePtr);
	// 'Major version'
	fputc(0x04, FilePtr);
	fputc(0x00, FilePtr);
	// 'Minor version'
	fputc(0x00, FilePtr);
	fputc(0x00, FilePtr);
	// 'Number of named entries'
	fputc(0x00, FilePtr);
	fputc(0x00, FilePtr);
	{
		// 'Number of id entries'
		U16 len = (U16)res->Len;
		if (res->Len != (int)len)
		{
			Err(L"EK0009", NULL, res->Len);
			return;
		}
		fwrite(&len, 1, 2, FilePtr);
	}
	{
		SListNode* ptr = res->Top;
		while (ptr != NULL)
		{
			SResEntry* entry = (SResEntry*)ptr->Data;
			fwrite(&entry->Value, 1, 4, FilePtr);
			entry->Addr = ftell(FilePtr);
			fputc(0xff, FilePtr);
			fputc(0xff, FilePtr);
			fputc(0xff, FilePtr);
			fputc(0xff, FilePtr);
			ptr = ptr->Next;
		}
	}
	{
		SListNode* ptr = res->Top;
		while (ptr != NULL)
		{
			SResEntry* entry = (SResEntry*)ptr->Data;
			{
				U32 addr = (U32)((S64)ftell(FilePtr) - base_addr);
				long tmp = ftell(FilePtr);
				if (entry->Children != NULL)
					addr |= 0x80000000;
				fseek(FilePtr, entry->Addr, SEEK_SET);
				fwrite(&addr, 1, 4, FilePtr);
				fseek(FilePtr, tmp, SEEK_SET);
			}
			if (entry->Children == NULL)
			{
				if (*idx == PackAsm->ResIconNum + 1)
				{
					Err(L"EK0008", NULL, Option->IconFile);
					return;
				}
				data_entry_addrs[*idx] = ftell(FilePtr);
				(*idx)++;
				fputc(0xff, FilePtr);
				fputc(0xff, FilePtr);
				fputc(0xff, FilePtr);
				fputc(0xff, FilePtr);
				fputc(0xff, FilePtr);
				fputc(0xff, FilePtr);
				fputc(0xff, FilePtr);
				fputc(0xff, FilePtr);
				fputc(0xe4, FilePtr);
				fputc(0x04, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
				fputc(0x00, FilePtr);
			}
			else
				WriteResRecursion(entry->Children, data_entry_addrs, idx, base_addr);
			ptr = ptr->Next;
		}
	}
}
