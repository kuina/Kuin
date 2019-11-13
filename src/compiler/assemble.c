#include "assemble.h"

#include "log.h"
#include "mem.h"
#include "stack.h"
#include "util.h"

#define REG_I_NUM 12
#define REG_F_NUM 14

static const S64 RegisteredAddr = -2;
static const EReg RegI[REG_I_NUM] =
{
	Reg_AX,
	Reg_CX,
	Reg_DX,
	Reg_R8,
	Reg_R9,
	Reg_R10,
	Reg_R11,
	Reg_R12,
	Reg_R13,
	Reg_R14,
	Reg_R15,
	Reg_BX,
};
static const EReg RegF[REG_F_NUM] =
{
	Reg_XMM0,
	Reg_XMM1,
	Reg_XMM2,
	Reg_XMM3,
	Reg_XMM4,
	Reg_XMM5,
	Reg_XMM6,
	Reg_XMM7,
	Reg_XMM8,
	Reg_XMM9,
	Reg_XMM10,
	Reg_XMM11,
	Reg_XMM12,
	Reg_XMM13,
};
static const U8 FloatNeg[16] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
};
static const U8 Real43f0000000000000[8] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x43,
};
static const U8 Real43e0000000000000[8] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x43,
};
static const U8 BlankData[8] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

typedef struct STmpVars
{
	int IVarNum;
	SAstArg** IVars;
	int FVarNum;
	SAstArg** FVars;
} STmpVars;

typedef enum ETypeId
{
	TypeId_Int = 0x00,
	TypeId_Float = 0x01,
	TypeId_Char = 0x02,
	TypeId_Bool = 0x03,
	TypeId_Bit8 = 0x04,
	TypeId_Bit16 = 0x05,
	TypeId_Bit32 = 0x06,
	TypeId_Bit64 = 0x07,
	TypeId_Func = 0x08,
	TypeId_Enum = 0x09,
	TypeId_Array = 0x80,
	TypeId_List = 0x81,
	TypeId_Stack = 0x82,
	TypeId_Queue = 0x83,
	TypeId_Dict = 0x84,
	TypeId_Class = 0x85,
} ETypeId;

static SPackAsm* PackAsm;
static const SOption* Option;
static SDict* Dlls;
static SList* Funcs;
static SStack* LocalVars;
static SStack* RefFuncArgNums;
static const U8* MakeUseResFlags;

static Bool LoadResources(void);
static SList* GetExceptionFunc(void);
static SList* GetExceptionFuncHeader(void);
static S64* AddReadonlyData(int data_size, const U8* data, Bool align128);
static S64* AddDllImport(int dll_name_size, const U8* dll_name, int func_name_size, U8* func_name);
static S64* AddWritableData(const Char* tag, int size);
static void CallAPI(SList* asms, const Char* dll_name, const Char* func_name);
static void CallKuinLib(const Char* func_name);
static SAstArg* MakeTmpVar(int size, SAstType* type);
static S64* RefLocalFunc(SAstFunc* func);
static S64* RefLocalVar(SAstArg* var);
static S64* RefClass(SAstClass* class_);
static void ToValue(SAstExpr* expr, int reg_i, int reg_f);
static void ToRef(SAstExpr* expr, EReg dst, EReg src);
static STmpVars* PushRegs(int reg_i, int reg_f);
static void PopRegs(const STmpVars* tmp);
static void GcInc(int reg_i);
static void GcDec(int reg_i, int reg_f, const SAstType* type);
static void SetGcInstance(int reg_i, int reg_f, SAstType* type);
static void AllocHeap(SVal* size);
static void FreeHeap(SVal* reg);
static void ChkOverflow(void);
static void RaiseExcpt(U64 code);
static void CopyMem(EReg dst, U64 size, EReg src, EReg reg_tmp);
static void ClearMem(EReg dst, U64 size, EReg reg_tmp);
static int GetSize(const SAstType* type);
static void DbgBreak(void);
static void RefFuncRecursion(SAstClass* class_);
static const void* InitDlls(const Char* key, const void* value, void* param);
static const void* InitDllFuncs(const Char* key, const void* value, void* param);
static const void* FinDlls(const Char* key, const void* value, void* param);
static void SetTypeId(EReg reg, const SAstType* type);
static int SetTypeIdRecursion(U8* ptr, int idx, const SAstType* type);
static void SetTypeIdForFromBin(EReg reg, const SAstType* type);
static void SetTypeIdForFromBinRecursion(int* idx_type, U8* data_type, int* idx_class, S64** data_class, const SAstType* type);
static void ExpandMe(SAstExprDot* dot, int reg_i);
static void SetStatAsm(SAstStat* ast, SListNode* asms_top, SListNode* asms_bottom);
static void AssembleFunc(SAstFunc* ast, Bool entry);
static void AssembleStats(SList* asts);
static void AssembleIf(SAstStatIf* ast);
static void AssembleSwitch(SAstStatSwitch* ast);
static void AssembleWhile(SAstStatWhile* ast);
static void AssembleFor(SAstStatFor* ast);
static void AssembleTry(SAstStatTry* ast);
static void AssembleThrow(SAstStatThrow* ast);
static void AssembleBlock(SAstStatBlock* ast);
static void AssembleRet(SAstStatRet* ast);
static void AssembleDo(SAstStatDo* ast);
static void AssembleBreak(SAstStat* ast);
static void AssembleSkip(SAstStat* ast);
static void AssembleAssert(SAstStatAssert* ast);
static void AssembleExpr(SAstExpr* ast, int reg_i, int reg_f);
static void AssembleExpr1(SAstExpr1* ast, int reg_i, int reg_f);
static void AssembleExpr2(SAstExpr2* ast, int reg_i, int reg_f);
static void AssembleExpr3(SAstExpr3* ast, int reg_i, int reg_f);
static void AssembleExprNew(SAstExprNew* ast, int reg_i, int reg_f);
static void AssembleExprNewArray(SAstExprNewArray* ast, int reg_i, int reg_f);
static void AssembleExprAs(SAstExprAs* ast, int reg_i, int reg_f);
static void AssembleExprToBin(SAstExprToBin* ast, int reg_i, int reg_f);
static void AssembleExprFromBin(SAstExprFromBin* ast, int reg_i, int reg_f);
static void AssembleExprCall(SAstExprCall* ast, int reg_i, int reg_f);
static void AssembleExprArray(SAstExprArray* ast, int reg_i, int reg_f);
static void AssembleExprDot(SAstExprDot* ast, int reg_i, int reg_f, Bool expand_me);
static void AssembleExprValue(SAstExprValue* ast, int reg_i, int reg_f);
static void AssembleExprValueArray(SAstExprValueArray* ast, int reg_i, int reg_f);
static void AssembleExprRef(SAstExpr* ast, int reg_i, int reg_f);

void Assemble(SPackAsm* pack_asm, const SAstFunc* entry, const SOption* option, SDict* dlls, S64 app_code, const U8* use_res_flags)
{
	PackAsm = pack_asm;
	Option = option;
	Dlls = dlls;
	MakeUseResFlags = use_res_flags;

	PackAsm->Asms = ListNew();
	PackAsm->ReadonlyData = ListNew();
	PackAsm->DLLImport = ListNew();
	PackAsm->ExcptTables = ListNew();
	PackAsm->WritableData = NULL;
	PackAsm->ResEntries = ListNew();
	PackAsm->ResIconNum = 0;
	PackAsm->ResIconHeaderBins = NULL;
	PackAsm->ResIconBinSize = 0;
	PackAsm->ResIconBins = NULL;
	PackAsm->RefValueList = ListNew();
	PackAsm->FuncAddrs = NULL;
	PackAsm->ClassTables = ListNew();
	PackAsm->AppCode = app_code;

	ASSERT(Option->IconFile != NULL);
	if (!LoadResources())
		return;

	Funcs = ListNew();
	ListAdd(Funcs, entry);
	LocalVars = NULL;
	RefFuncArgNums = NULL;

	{
		SAstFuncRaw* func = (SAstFuncRaw*)Alloc(sizeof(SAstFuncRaw));
		memset(func, 0, sizeof(SAst));
		((SAst*)func)->TypeId = AstTypeId_FuncRaw;
		((SAst*)func)->AnalyzedCache = (SAst*)func;
		((SAstFunc*)func)->AddrTop = NewAddr();
		((SAstFunc*)func)->AddrBottom = -1;
		((SAstFunc*)func)->PosRowBottom = -1;
		((SAstFunc*)func)->DllName = NULL;
		((SAstFunc*)func)->DllFuncName = NULL;
		((SAstFunc*)func)->FuncAttr = FuncAttr_None;
		((SAstFunc*)func)->Args = ListNew();
		((SAstFunc*)func)->Ret = NULL;
		((SAstFunc*)func)->Stats = ListNew();
		((SAstFunc*)func)->RetPoint = NULL;
		func->Asms = GetExceptionFunc();
		func->ArgNum = -1;
		func->Header = GetExceptionFuncHeader();
		ListAdd(Funcs, func);
		PackAsm->ExcptFunc = ((SAstFunc*)func)->AddrTop;
	}

	while (Funcs->Len != 0)
	{
		SListNode* last_node = PackAsm->Asms->Bottom;
		SAsm** asms = (SAsm**)Alloc(sizeof(SAsm*) * 2);
		SAstFunc* func = (SAstFunc*)Funcs->Top->Data;
		AssembleFunc(func, func == entry);
		if (last_node == NULL)
			asms[0] = (SAsm*)PackAsm->Asms->Top->Data;
		else
			asms[0] = (SAsm*)last_node->Next->Data;
		asms[1] = (SAsm*)PackAsm->Asms->Bottom->Data;
		PackAsm->FuncAddrs = DictIAdd(PackAsm->FuncAddrs, (U64)func, asms);
		ListDel(Funcs, &Funcs->Top);
	}

#if defined(_DEBUG)
	Dump2(NewStr(NULL, L"%s_assembled.txt", Option->OutputFile), PackAsm->Asms);
#endif
}

static Bool LoadResources(void)
{
	for (; ; )
	{
		FILE* file_ptr = _wfopen(Option->IconFile, L"rb");
		U8 data;
		if (file_ptr == NULL)
		{
			Err(L"EK0007", NULL, Option->IconFile);
			return False;
		}
		if (fread(&data, 1, 1, file_ptr) < 1 || data != 0x00)
			goto Failed;
		if (fread(&data, 1, 1, file_ptr) < 1 || data != 0x00)
			goto Failed;
		if (fread(&data, 1, 1, file_ptr) < 1 || data != 0x01)
			goto Failed;
		if (fread(&data, 1, 1, file_ptr) < 1 || data != 0x00)
			goto Failed;
		{
			U32 num;
			if (fread(&data, 1, 1, file_ptr) < 1)
				goto Failed;
			num = (U32)data;
			if (fread(&data, 1, 1, file_ptr) < 1)
				goto Failed;
			num |= (U32)data << 8;
			PackAsm->ResIconNum = (int)num;
		}
		PackAsm->ResIconHeaderBins = (U8(*)[14])Alloc(sizeof(U8[14]) * (size_t)PackAsm->ResIconNum);
		PackAsm->ResIconBinSize = (int*)Alloc(sizeof(int) * (size_t)PackAsm->ResIconNum);
		PackAsm->ResIconBins = (U8**)Alloc(sizeof(U8*) * (size_t)PackAsm->ResIconNum);
		{
			int i;
			int j;
			for (i = 0; i < PackAsm->ResIconNum; i++)
			{
				for (j = 0; j < 12; j++)
				{
					if (fread(&data, 1, 1, file_ptr) < 1)
						goto Failed;
					PackAsm->ResIconHeaderBins[i][j] = data;
				}
				PackAsm->ResIconHeaderBins[i][4] = 0x01; // This should be 0 or 1 depending on the specification, but somehow 1 seems to be better.
				{
					U32 tmp;
					if (fread(&tmp, 1, 4, file_ptr) < 4)
						goto Failed;
				}
				PackAsm->ResIconHeaderBins[i][12] = (U8)(i + 1);
				PackAsm->ResIconHeaderBins[i][13] = 0x00;
				{
					U32 size = (U32)PackAsm->ResIconHeaderBins[i][8];
					size |= (U32)PackAsm->ResIconHeaderBins[i][9] << 8;
					size |= (U32)PackAsm->ResIconHeaderBins[i][10] << 16;
					size |= (U32)PackAsm->ResIconHeaderBins[i][11] << 24;
					PackAsm->ResIconBinSize[i] = (int)size;
					PackAsm->ResIconBins[i] = (U8*)Alloc(sizeof(U8) * (size_t)size);
				}
			}
			for (i = 0; i < PackAsm->ResIconNum; i++)
			{
				for (j = 0; j < PackAsm->ResIconBinSize[i]; j++)
				{
					if (fread(&data, 1, 1, file_ptr) < 1)
						goto Failed;
					PackAsm->ResIconBins[i][j] = data;
				}
			}
		}
		fclose(file_ptr);
		break;
Failed:
		fclose(file_ptr);
		return False;
	}
	{
		PackAsm->ResEntries = ListNew();
		{
			SResEntry* entry_icon = (SResEntry*)Alloc(sizeof(SResEntry));
			entry_icon->Addr = -1;
			entry_icon->Value = 0x03; // 'RT_ICON'
			entry_icon->Children = ListNew();
			{
				int i;
				for (i = 0; i < PackAsm->ResIconNum; i++)
				{
					SResEntry* entry_id = (SResEntry*)Alloc(sizeof(SResEntry));
					entry_id->Addr = -1;
					entry_id->Value = i + 1; // 'ID'
					entry_id->Children = ListNew();
					{
						SResEntry* entry_lang = (SResEntry*)Alloc(sizeof(SResEntry));
						entry_lang->Addr = -1;
						entry_lang->Value = 0x0409; // 'en-us'
						entry_lang->Children = NULL;
						ListAdd(entry_id->Children, entry_lang);
					}
					ListAdd(entry_icon->Children, entry_id);
				}
			}
			ListAdd(PackAsm->ResEntries, entry_icon);
		}
		{
			SResEntry* entry_icon = (SResEntry*)Alloc(sizeof(SResEntry));
			entry_icon->Addr = -1;
			entry_icon->Value = 0x0e; // 'RT_GROUP_ICON'
			entry_icon->Children = ListNew();
			{
				SResEntry* entry_id = (SResEntry*)Alloc(sizeof(SResEntry));
				entry_id->Addr = -1;
				entry_id->Value = 0x65; // 'ID'
				entry_id->Children = ListNew();
				{
					SResEntry* entry_lang = (SResEntry*)Alloc(sizeof(SResEntry));
					entry_lang->Addr = -1;
					entry_lang->Value = 0x0409; // 'en-us'
					entry_lang->Children = NULL;
					ListAdd(entry_id->Children, entry_lang);
				}
				ListAdd(entry_icon->Children, entry_id);
			}
			ListAdd(PackAsm->ResEntries, entry_icon);
		}
		{
			SResEntry* entry_manifest = (SResEntry*)Alloc(sizeof(SResEntry));
			entry_manifest->Addr = -1;
			entry_manifest->Value = 0x18; // 'RT_MANIFEST'
			entry_manifest->Children = ListNew();
			{
				SResEntry* entry_id = (SResEntry*)Alloc(sizeof(SResEntry));
				entry_id->Addr = -1;
				entry_id->Value = 0x01; // 'ID'
				entry_id->Children = ListNew();
				{
					SResEntry* entry_lang = (SResEntry*)Alloc(sizeof(SResEntry));
					entry_lang->Addr = -1;
					entry_lang->Value = 0x0409; // 'en-us'
					entry_lang->Children = NULL;
					ListAdd(entry_id->Children, entry_lang);
				}
				ListAdd(entry_manifest->Children, entry_id);
			}
			ListAdd(PackAsm->ResEntries, entry_manifest);
		}
	}
	return True;
}

static SList* GetExceptionFunc(void)
{
	SList* asms = ListNew();
	static U8 bin1[] =
	{
		0x8b, 0x41, 0x04, // mov eax,dword ptr [rcx+4]
		0x4d, 0x8b, 0x31, // mov r14,qword ptr [r9]
		0x4d, 0x8b, 0x79, 0x08, // mov r15,qword ptr [r9+8]
		0x49, 0x8b, 0x79, 0x38, // mov rdi,qword ptr [r9+38h]
		0x4d, 0x29, 0xfe, // sub r14,r15
		0x4d, 0x89, 0xcc, // mov r12,r9
		0x49, 0x89, 0xd5, // mov r13,rdx
		0x48, 0x89, 0xcd, // mov rbp,rcx
		0xa8, 0x66, // test al,66h
		0x0f, 0x85, 0xc4, 0x00, 0x00, 0x00, // jne lbl5
		0x49, 0x63, 0x71, 0x48, // movsxd rsi,dword ptr [r9+48h]
		0x49, 0x89, 0x4b, 0xc8, // mov qword ptr [r11-38h],rcx
		0x4d, 0x89, 0x43, 0xd0, // mov qword ptr [r11-30h],r8
		0x48, 0x89, 0xf0, // mov rax,rsi
		0x3b, 0x37, // cmp esi,dword ptr [rdi]
		0x0f, 0x83, 0x9a, 0x01, 0x00, 0x00, // jae lbl5
		0x48, 0x01, 0xc0, // add rax,rax
		0x48, 0x8d, 0x5c, 0xc7, 0x0c, // lea rbx,[rdi+rax*8+0ch]
		// lbl1
		0x8b, 0x43, 0xf8, // mov eax,dword ptr [rbx-8]
		0x49, 0x39, 0xc6, // cmp r14,rax
		0x0f, 0x82, 0x7f, 0x00, 0x00, 0x00, // jb lbl3
		0x8b, 0x43, 0xfc, // mov eax,dword ptr[rbx-4]
		0x49, 0x39, 0xc6, // cmp r14,rax
		0x0f, 0x83, 0x73, 0x00, 0x00, 0x00, // jae lbl3
		0x83, 0x7b, 0x04, 0x00, // cmp dword ptr[rbx+4],0
		0x0f, 0x84, 0x69, 0x00, 0x00, 0x00, // je lbl3
		0x83, 0x3b, 0x01, // cmp dword ptr [rbx],1
		0x0f, 0x84, 0x1d, 0x00, 0x00, 0x00, // je lbl2
		0x8b, 0x03, // mov eax, dword ptr [rbx]
		0x48, 0x8d, 0x4c, 0x24, 0x30, // lea rcx,[rsp+30h]
		0x4c, 0x89, 0xea, // mov rdx,r13
		0x4c, 0x01, 0xf8, // add rax,r15
		0xff, 0xd0, // call rax
		0x85, 0xc0, // test eax,eax
		0x0f, 0x88, 0x5c, 0x00, 0x00, 0x00, // js lbl4
		0x0f, 0x8e, 0x43, 0x00, 0x00, 0x00, // jle lbl3
		// lbl2
		0x8b, 0x4b, 0x04, // mov ecx,dword ptr [rbx+4]
		0x41, 0xb8, 0x01, 0x00, 0x00, 0x00, // mov r8d,1
		0x4c, 0x89, 0xea, // mov rdx,r13
		0x4c, 0x01, 0xf9, // add rcx,r15
		0xe8, 0x5a, 0x01, 0x00, 0x00, // call lbl17
		0x49, 0x8b, 0x44, 0x24, 0x40, // mov rax,qword ptr [r12+40h]
		0x8b, 0x53, 0x04, // mov edx,dword ptr [rbx+4]
		0x4c, 0x63, 0x4d, 0x00, // movsxd r9,dword ptr [rbp]
		0x48, 0x89, 0x44, 0x24, 0x28, // mov qword ptr [rsp+28h],rax
		0x49, 0x8b, 0x44, 0x24, 0x28, // mov rax,qword ptr [r12+28h]
		0x4c, 0x01, 0xfa, // add rdx,r15
		0x49, 0x89, 0xe8, // mov r8,rbp
		0x4c, 0x89, 0xe9, // mov rcx,r13
		0x48, 0x89, 0x44, 0x24, 0x20 // mov qword ptr [rsp+20h],rax
	};
	static U8 bin2[] =
	{
		0xe8, 0x46, 0x01, 0x00, 0x00, // call lbl18
		// lbl3
		0xff, 0xc6, // inc esi
		0x48, 0x83, 0xc3, 0x10, // add rbx,10h
		0x3b, 0x37, // cmp esi,dword ptr [rdi],
		0x0f, 0x83, 0xf9, 0x00, 0x00, 0x00, // jae lbl15
		0xe9, 0x62, 0xff, 0xff, 0xff, // jmp lbl1
		// lbl4
		0x31, 0xc0, // xor eax,eax
		0xe9, 0xf2, 0x00, 0x00, 0x00, // jmp lbl16
		// lbl5
		0x4d, 0x8b, 0x41, 0x20, // mov r8,qword ptr [r9+20h]
		0x31, 0xed, // xor ebp,ebp
		0x45, 0x31, 0xed, // xor r13d,r13d
		0x4d, 0x29, 0xf8, // sub r8,r15
		0xa8, 0x20, // test al,20h
		0x0f, 0x84, 0x55, 0x00, 0x00, 0x00, // je lbl10
		0x31, 0xd2, // xor edx,edx
		0x39, 0x17, // cmp dword ptr [rdi],edx
		0x0f, 0x86, 0x4b, 0x00, 0x00, 0x00, // jbe lbl10
		0x48, 0x8d, 0x4f, 0x08, // lea rcx,[rdi+8]
		// lbl6
		0x8b, 0x41, 0xfc, // mov eax,dword ptr [rcx-4]
		0x49, 0x39, 0xc0, // cmp r8,rax
		0x0f, 0x82, 0x0b, 0x00, 0x00, 0x00, // jb lbl7
		0x8b, 0x01, // mov eax,dword ptr [rcx]
		0x49, 0x39, 0xc0, // cmp r8,rax
		0x0f, 0x86, 0xf3, 0x00, 0x00, 0x00, // jbe lbl8
		// lbl7
		0xff, 0xc2, // inc edx
		0x48, 0x83, 0xc1, 0x10, // add rcx,10h
		0x3b, 0x17, // cmp edx,dword ptr [rdi]
		0x0f, 0x83, 0x22, 0x00, 0x00, 0x00, // jae lbl10
		0xe9, 0xd6, 0xff, 0xff, 0xff, // jmp lbl6
		// lbl8
		0x89, 0xd0, // mov eax,edx,
		0x48, 0x01, 0xc0, // add rax,rax
		0x8b, 0x4c, 0xc7, 0x10, // mov ecx,dword ptr [rdi+rax*8+10h]
		0x85, 0xc9, // test ecx,ecx
		0x0f, 0x85, 0x09, 0x00, 0x00, 0x00, // jne lbl9
		0x8b, 0x6c, 0xc7, 0x0c, // mov ebp,dword ptr [rdi+rax*8+0ch]
		0xe9, 0x03, 0x00, 0x00, 0x00, // jmp lbl10
		// lbl9
		0x41, 0x89, 0xcd, // mov r13d,ecx
		// lbl10
		0x49, 0x63, 0x71, 0x48, // movsxd rsi,dword ptr [r9+48h]
		0x48, 0x89, 0xf3, // mov rbx,rsi
		0x3b, 0x37, // cmp esi,dword ptr [rdi]
		0x0f, 0x83, 0x75, 0x00, 0x00, 0x00, // jae lbl15
		0x48, 0xff, 0xc3, // inc rbx
		0x48, 0xc1, 0xe3, 0x04, // shl rbx,4
		0x48, 0x01, 0xfb, // add rbx,rdi
		// lbl11
		0x8b, 0x43, 0xf4, // mov eax,dword ptr [rbx-0ch]
		0x49, 0x39, 0xc6, // cmp r14,rax
		0x0f, 0x82, 0x51, 0x00, 0x00, 0x00, // jb lbl14
		0x8b, 0x43, 0xf8, // mov eax,dword ptr [rbx-8]
		0x49, 0x39, 0xc6, // cmp r14,rax
		0x0f, 0x83, 0x45, 0x00, 0x00, 0x00, // jae lbl14
		0x45, 0x85, 0xed, // test r13d,r13d
		0x0f, 0x84, 0x09, 0x00, 0x00, 0x00, // je lbl12
		0x44, 0x3b, 0x2b, // cmp r13d,dword ptr[rbx]
		0x0f, 0x84, 0x41, 0x00, 0x00, 0x00, // je lbl15
		// lbl12
		0x85, 0xed, // test ebp, ebp
		0x0f, 0x84, 0x09, 0x00, 0x00, 0x00, // je lbl13
		0x3b, 0x6b, 0xfc, // cmp ebp,dword ptr [rbx-4]
		0x0f, 0x84, 0x30, 0x00, 0x00, 0x00, // je lbl15
		// lbl13
		0x83, 0x3b, 0x00, // cmp dword ptr [rbx],0
		0x0f, 0x85, 0x19, 0x00, 0x00, 0x00, // jne lbl14
		0x48, 0x8b, 0x54, 0x24, 0x78, // mov rdx,qword ptr [rsp+78h]
		0x8d, 0x46, 0x01, // lea eax,[rsi+1]
		0xb1, 0x01, // mov cl,1
		0x41, 0x89, 0x44, 0x24, 0x48, // mov dword ptr [r12+48h],eax
		0x44, 0x8b, 0x43, 0xfc, // mov r8d,dword ptr [rbx-4]
		0x4d, 0x01, 0xf8, // add r8,r15
		0x41, 0xff, 0xd0, // call r8
		// lbl14
		0xff, 0xc6, // inc esi
		0x48, 0x83, 0xc3, 0x10, // add rbx,10h
		0x3b, 0x37, // cmp esi,dword ptr [rdi]
		0x0f, 0x82, 0x95, 0xff, 0xff, 0xff, // jb lbl11
		// lbl15
		0xb8, 0x01, 0x00, 0x00, 0x00, // mov eax,1
		// lbl16
		0x4c, 0x8d, 0x5c, 0x24, 0x40, // lea r11,[rsp+40h]
		0x49, 0x8b, 0x5b, 0x30, // mov rbx,qword ptr [r11+30h]
		0x49, 0x8b, 0x6b, 0x40, // mov rbp,qword ptr [r11+40h]
		0x49, 0x8b, 0x73, 0x48, // mov rsi,qword ptr [r11+48h]
		0x4c, 0x89, 0xdc, // mov rsp,r11
		0x41, 0x5f, // pop r15
		0x41, 0x5e, // pop r14
		0x41, 0x5d, // pop r13
		0x41, 0x5c, // pop r12
		0x5f, // pop rdi
		0xc3, // ret
		0xcc, // int 3
		// lbl17
		0x48, 0x89, 0x4c, 0x24, 0x08, // mov qword ptr [rsp+8],rcx
		0x44, 0x89, 0x44, 0x24, 0x10, // mov dword ptr [rsp+10h],r8d
		0x48, 0x89, 0x54, 0x24, 0x18, // mov qword ptr [rsp+18h],rdx
		0x49, 0xb9, 0x20, 0x05, 0x93, 0x19, 0x00, 0x00, 0x00, 0x00, // r9, 19930520h
		0xc3, // ret
		0xcc, // int 3
		// lbl18
		0xc3 // ret
	};

	{
		SAsmMachine* asm_ = AsmMachine((int)sizeof(bin1), bin1);
		ListAdd(asms, asm_);
	}
	CallAPI(asms, L"KERNEL32.dll", L"RtlUnwindEx");
	{
		SAsmMachine* asm_ = AsmMachine((int)sizeof(bin2), bin2);
		ListAdd(asms, asm_);
	}
	return asms;
}

static SList* GetExceptionFuncHeader(void)
{
	SList* asms = ListNew();
	static U8 bin[] =
	{
		0x49, 0x89, 0xe3, // mov r11,rsp
		0x49, 0x89, 0x5b, 0x08, // mov qword ptr [r11+8],rbx
		0x49, 0x89, 0x53, 0x10, // mov qword ptr [r11+10h],rdx
		0x49, 0x89, 0x6b, 0x18, // mov qword ptr [r11+18h],rbp
		0x49, 0x89, 0x73, 0x20, // mov qword ptr [r11+20h],rsi
		0x57, // push rdi
		0x41, 0x54, // push r12
		0x41, 0x55, // push r13
		0x41, 0x56, // push r14
		0x41, 0x57, // push r15
		0x48, 0x83, 0xec, 0x40 // sub rsp,40h
	};
	{
		SAsmMachine* asm_ = AsmMachine((int)sizeof(bin), bin);
		ListAdd(asms, asm_);
	}
	return asms;
}

static S64* AddReadonlyData(int data_size, const U8* data, Bool align128)
{
	{
		SListNode* ptr = PackAsm->ReadonlyData->Top;
		while (ptr != NULL)
		{
			SReadonlyData* data2 = (SReadonlyData*)ptr->Data;
			if (CmpData(data2->BufSize, data2->Buf, data_size, data))
			{
				if (!data2->Align128 && align128)
					data2->Align128 = True;
				return data2->Addr;
			}
			ptr = ptr->Next;
		}
	}
	{
		SReadonlyData* data2 = (SReadonlyData*)Alloc(sizeof(SReadonlyData));
		data2->Align128 = align128;
		data2->BufSize = data_size;
		data2->Buf = data;
		data2->Addr = NewAddr();
		ListAdd(PackAsm->ReadonlyData, data2);
		return data2->Addr;
	}
}

static S64* AddDllImport(int dll_name_size, const U8* dll_name, int func_name_size, U8* func_name)
{
	{
		SListNode* ptr = PackAsm->DLLImport->Top;
		while (ptr != NULL)
		{
			SDLLImport* dll = (SDLLImport*)ptr->Data;
			if (CmpData(dll_name_size, dll_name, dll->DLLNameSize, dll->DllName))
			{
				SListNode* ptr2 = dll->Funcs->Top;
				while (ptr2 != NULL)
				{
					SDLLImportFunc* func = (SDLLImportFunc*)ptr2->Data;
					if (CmpData(func_name_size, func_name, func->FuncNameSize, func->FuncName))
						return func->Addr;
					ptr2 = ptr2->Next;
				}
				{
					SDLLImportFunc* func = (SDLLImportFunc*)Alloc(sizeof(SDLLImportFunc));
					func->FuncNameSize = func_name_size;
					func->FuncName = func_name;
					func->Addr = NewAddr();
					ListAdd(dll->Funcs, func);
					return func->Addr;
				}
			}
			ptr = ptr->Next;
		}
	}
	{
		SDLLImport* dll = (SDLLImport*)Alloc(sizeof(SDLLImport));
		dll->DLLNameSize = dll_name_size;
		dll->DllName = dll_name;
		dll->Addr = -1;
		dll->Funcs = ListNew();
		{
			SDLLImportFunc* func = (SDLLImportFunc*)Alloc(sizeof(SDLLImportFunc));
			func->FuncName = func_name;
			func->FuncNameSize = func_name_size;
			func->Addr = NewAddr();
			ListAdd(dll->Funcs, func);
			ListAdd(PackAsm->DLLImport, dll);
			return func->Addr;
		}
	}
}

static S64* AddWritableData(const Char* tag, int size)
{
	const Char* tag2 = NewStr(NULL, L"%s%d", tag, size);
	const SWritableData* data = (const SWritableData*)DictSearch(PackAsm->WritableData, tag2);
	if (data != NULL)
	{
		ASSERT(data->Size == size);
		return data->Addr;
	}
	{
		SWritableData* data2 = (SWritableData*)Alloc(sizeof(SWritableData));
		data2->Size = size;
		data2->Addr = NewAddr();
		PackAsm->WritableData = DictAdd(PackAsm->WritableData, tag2, data2);
		return data2->Addr;
	}
}

static void CallAPI(SList* asms, const Char* dll_name, const Char* func_name)
{
	int bin_dll_name_size;
	U8* bin_dll_name;
	int bin_func_name_size;
	U8* bin_func_name;
	bin_dll_name = StrToBin(dll_name, &bin_dll_name_size);
	bin_func_name = StrToBin(func_name, &bin_func_name_size);
	{
		S64* addr = AddDllImport(bin_dll_name_size, bin_dll_name, bin_func_name_size, bin_func_name);
		ListAdd(asms, AsmCALL(ValRIP(8, RefValueAddr(addr, True))));
	}
}

static void CallKuinLib(const Char* func_name)
{
	// You must register the functions you call here in the 'Analyze' function.
	S64* addr = AddWritableData(NewStr(NULL, L"%s$%s", L"d0000.knd", func_name), 8);
	ListAdd(PackAsm->Asms, AsmCALL(ValRIP(8, RefValueAddr(addr, True))));
}

static SAstArg* MakeTmpVar(int size, SAstType* type)
{
	SAstArg* arg = (SAstArg*)Alloc(sizeof(SAstArg));
	memset(arg, 0, sizeof(SAst));
	((SAst*)arg)->TypeId = AstTypeId_Arg;
	((SAst*)arg)->AnalyzedCache = (SAst*)arg;
	arg->Addr = NewAddr();
	arg->Kind = AstArgKind_LocalVar;
	arg->RefVar = False;
	if (type != NULL)
	{
		ASSERT(size == 8);
		arg->Type = type;
	}
	else
	{
		SAstTypeBit* type2 = (SAstTypeBit*)Alloc(sizeof(SAstTypeBit));
		ASSERT(size == 1 || size == 2 || size == 4 || size == 8);
		memset(type2, 0, sizeof(SAst));
		((SAst*)type2)->TypeId = AstTypeId_TypeBit;
		((SAst*)type2)->AnalyzedCache = (SAst*)type2;
		type2->Size = size;
		arg->Type = (SAstType*)type2;
	}
	arg->Expr = NULL;
	return arg;
}

static S64* RefLocalFunc(SAstFunc* func)
{
	if (*func->AddrTop == -1)
	{
		ListAdd(Funcs, func);
		*func->AddrTop = -2;
	}
	ASSERT(*func->AddrTop == -2);
	return func->AddrTop;
}

static S64* RefLocalVar(SAstArg* var)
{
	if (*var->Addr == -1)
	{
		ListAdd((SList*)StackPeek(LocalVars), var);
		*var->Addr = -2;
	}
	ASSERT(*var->Addr == -2);
	return var->Addr;
}

static S64* RefClass(SAstClass* class_)
{
	if (*class_->Addr == -1)
	{
		// Calculate sizes and tables of classes when they are referenced.
		S64* parent = NULL;
		if (((SAst*)class_)->RefItem != NULL)
			parent = RefClass((SAstClass*)((SAst*)class_)->RefItem);
		{
			SClassTable* class_table = (SClassTable*)Alloc(sizeof(SClassTable));
			class_table->Addr = class_->Addr;
			class_table->Parent = parent;
			class_table->Class = class_;
			ListAdd(PackAsm->ClassTables, class_table);
		}
		class_->VarSize = ((SAst*)class_)->RefItem == NULL ? 0 : ((SAstClass*)((SAst*)class_)->RefItem)->VarSize;
		class_->FuncSize = ((SAst*)class_)->RefItem == NULL ? 0 : ((SAstClass*)((SAst*)class_)->RefItem)->FuncSize;
		{
			SListNode* ptr = class_->Items->Top;
			while (ptr != NULL)
			{
				SAstClassItem* item = (SAstClassItem*)ptr->Data;
				if ((item->Def->TypeId & AstTypeId_Func) == AstTypeId_Func)
				{
					if (item->Override)
						item->Addr = item->ParentItem->Addr;
					else
					{
						item->Addr = (S64)class_->FuncSize;
						class_->FuncSize += 0x08; // A function pointer.
					}
				}
				else if (item->Def->TypeId == AstTypeId_Var)
				{
					int size = GetSize(((SAstVar*)item->Def)->Var->Type);
					if (class_->VarSize % size != 0)
						class_->VarSize += size - class_->VarSize % size;
					item->Addr = (S64)class_->VarSize;
					class_->VarSize += size;
				}
				ptr = ptr->Next;
			}
			if (class_->VarSize % 8 != 0)
				class_->VarSize += 8 - class_->VarSize % 8;
		}
		RefFuncRecursion(class_);
		*class_->Addr = -2;
	}
	ASSERT(*class_->Addr == -2);
	return class_->Addr;
}

static void ToValue(SAstExpr* expr, int reg_i, int reg_f)
{
	// The values of less than 'reg_i', less than 'reg_f', SI, and DI are to be restored.
	ASSERT(0 <= reg_i && reg_i < REG_I_NUM && 0 <= reg_f && reg_f < REG_F_NUM);
	ASSERT(expr->VarKind != AstExprVarKind_Unknown);
	if (expr->VarKind == AstExprVarKind_Value)
		return;
	if (IsFloat(expr->Type))
	{
		switch (expr->VarKind)
		{
			case AstExprVarKind_LocalVar:
				ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValMemS(4, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00)));
				break;
			case AstExprVarKind_GlobalVar:
				ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValMemS(4, ValReg(8, RegI[reg_i]), NULL, 0x00)));
				break;
			case AstExprVarKind_RefVar:
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00)));
				ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValMemS(4, ValReg(8, RegI[reg_i]), NULL, 0x00)));
				break;
			default:
				ASSERT(False);
				break;
		}
	}
	else
	{
		int size = GetSize(expr->Type);
		switch (expr->VarKind)
		{
			case AstExprVarKind_LocalVar:
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i]), ValMemS(size, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00)));
				break;
			case AstExprVarKind_GlobalVar:
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i]), ValMemS(size, ValReg(8, RegI[reg_i]), NULL, 0x00)));
				break;
			case AstExprVarKind_RefVar:
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00)));
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i]), ValMemS(size, ValReg(8, RegI[reg_i]), NULL, 0x00)));
				break;
			default:
				ASSERT(False);
				break;
		}
	}
}

static void ToRef(SAstExpr* expr, EReg dst, EReg src)
{
	switch (expr->VarKind)
	{
		case AstExprVarKind_LocalVar:
			ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, dst), ValMemS(8, ValReg(8, Reg_SP), ValReg(1, src), 0x00)));
			break;
		case AstExprVarKind_GlobalVar:
			ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, dst), ValMemS(8, ValReg(8, src), NULL, 0x00)));
			break;
		case AstExprVarKind_RefVar:
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, dst), ValMemS(8, ValReg(8, Reg_SP), ValReg(1, src), 0x00)));
			break;
		default:
			ASSERT(False);
			break;
	}
}

static STmpVars* PushRegs(int reg_i, int reg_f)
{
	// The values of 'reg_i' and less, 'reg_f' and less, 'SI', and 'DI' are to be restored.
	// Set 'reg_i' and 'reg_f' to -1 if you do not need to restore them.
	if (reg_i == -1 && reg_f == -1)
		return NULL;
	{
		STmpVars* tmp = (STmpVars*)Alloc(sizeof(STmpVars));
		int i;
		ASSERT(-1 <= reg_i && reg_i < REG_I_NUM && -1 <= reg_f && reg_f < REG_F_NUM);
		tmp->IVarNum = reg_i + 1;
		tmp->IVars = (SAstArg**)Alloc(sizeof(SAstArg*) * (size_t)tmp->IVarNum);
		for (i = 0; i < tmp->IVarNum; i++)
		{
			tmp->IVars[i] = MakeTmpVar(8, NULL);
			ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp->IVars[i]), False)), ValReg(8, RegI[i])));
		}
		tmp->FVarNum = reg_f + 1;
		tmp->FVars = (SAstArg**)Alloc(sizeof(SAstArg*) * (size_t)tmp->FVarNum);
		for (i = 0; i < tmp->FVarNum; i++)
		{
			tmp->FVars[i] = MakeTmpVar(8, NULL);
			ListAdd(PackAsm->Asms, AsmMOVSD(ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp->FVars[i]), False)), ValReg(4, RegF[i])));
		}
		return tmp;
	}
}

static void PopRegs(const STmpVars* tmp)
{
	// The values of 'SI' and 'DI' are to be restored.
	if (tmp == NULL)
		return;
	{
		int i;
		for (i = 0; i < tmp->IVarNum; i++)
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[i]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp->IVars[i]), False))));
		for (i = 0; i < tmp->FVarNum; i++)
			ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[i]), ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp->FVars[i]), False))));
	}
}

static void GcInc(int reg_i)
{
	// The values of 'reg_i' and less, 'reg_f' and less, 'SI', and 'DI' are to be restored.
	{
		SAsmLabel* lbl1 = AsmLabel();
		ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, RegI[reg_i]), ValImmU(8, 0x00)));
		ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
		ListAdd(PackAsm->Asms, AsmINC(ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00)));
		ListAdd(PackAsm->Asms, lbl1);
	}
}

static void GcDec(int reg_i, int reg_f, const SAstType* type)
{
	// The values of less than 'reg_i', 'reg_f' and less, 'SI', and 'DI' are to be restored.
	// Since 'reg_i' may be released, its value is not restored.
	SAsmLabel* lbl1 = AsmLabel();
	ASSERT(IsRef(type));
	ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, RegI[reg_i]), ValImmU(8, 0x00)));
	ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
	ListAdd(PackAsm->Asms, AsmDEC(ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00)));
#if defined(_DEBUG)
	/*
	{
		// Since reference counters rarely exceeds 0x10, it is regarded as memory corruption.
		SAsmLabel* lbl2 = AsmLabel();
		ListAdd(PackAsm->Asms, AsmCMP(ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00), ValImmU(8, 0x10)));
		ListAdd(PackAsm->Asms, AsmJBE(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
		ListAdd(PackAsm->Asms, AsmINT(ValImmU(8, 0x03)));
		ListAdd(PackAsm->Asms, lbl2);
	}
	*/
#endif
	ListAdd(PackAsm->Asms, AsmCMP(ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00), ValImmU(8, 0x00)));
	ListAdd(PackAsm->Asms, AsmJNE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
	{
		STmpVars* tmp = PushRegs(reg_i - 1, reg_f);
		if (((SAst*)type)->TypeId == AstTypeId_TypeArray && !IsRef(((SAstTypeArray*)type)->ItemType))
			FreeHeap(ValReg(8, RegI[reg_i]));
		else if (IsClass(type))
		{
			SAstArg* tmp_si = MakeTmpVar(8, NULL);
			SAstArg* tmp_di = MakeTmpVar(8, NULL);
			SAstArg* tmp_reg_i = MakeTmpVar(8, NULL);
			ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_si), False)), ValReg(8, Reg_SI)));
			ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_di), False)), ValReg(8, Reg_DI)));
			ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_reg_i), False)), ValReg(8, RegI[reg_i])));
			ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00), ValImmU(8, 0x02))); // Set the reference counter to 2 so that the instance will not be released twice.
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValReg(8, RegI[reg_i])));
			ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, 0x00), ValReg(8, RegI[reg_i])));
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x08)));
			ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x10)));
			ListAdd(PackAsm->Asms, AsmADD(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00)));
			ListAdd(PackAsm->Asms, AsmCALL(ValReg(8, RegI[reg_i]))); // Call '_dtor'.
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_reg_i), False))));
			FreeHeap(ValReg(8, RegI[reg_i]));
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_si), False))));
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DI), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_di), False))));
		}
		else
		{
			// Leave complicated types to the library.
			ASSERT(((SAst*)type)->TypeId == AstTypeId_TypeArray && IsRef(((SAstTypeArray*)type)->ItemType) || ((SAst*)type)->TypeId == AstTypeId_TypeGen || ((SAst*)type)->TypeId == AstTypeId_TypeDict);
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValReg(8, RegI[reg_i])));
			SetTypeId(Reg_DX, type);
			CallKuinLib(L"_freeSet");
		}
		PopRegs(tmp);
	}
	ListAdd(PackAsm->Asms, lbl1);
}

static void SetGcInstance(int reg_i, int reg_f, SAstType* type)
{
	// The values of 'reg_i' and less, 'reg_f' and less, 'SI', and 'DI' are to be restored.
	// The reference counter must be incremented by the caller.
	SAstArg* instance = MakeTmpVar(8, type);
	SAstArg* tmp_reg_i = MakeTmpVar(8, NULL);
	ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_reg_i), False)), ValReg(8, RegI[reg_i])));
	// Assign a value to the variable after cutting off its reference because the assignment process may be done twice in a loop.
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(instance), False))));
	GcDec(reg_i, reg_f, type);
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_reg_i), False))));
	ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(instance), False)), ValReg(8, RegI[reg_i])));
}

static void AllocHeap(SVal* size)
{
	// The values of 'SI' and 'DI' are to be restored.
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_R8), size));
	{
		S64* addr = AddWritableData(L"HeapHnd", 8);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValRIP(8, RefValueAddr(addr, True))));
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, Reg_DX), ValImmU(4, 0x04))); // Make 'HeapAlloc' throw an exception when an error occurs.
		CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"HeapAlloc");
	}
#if defined (_DEBUG)
	{
		S64* addr = AddWritableData(L"HeapCnt", 8);
		ListAdd(PackAsm->Asms, AsmINC(ValRIP(8, RefValueAddr(addr, True))));
	}
#endif
}

static void FreeHeap(SVal* reg)
{
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_R8), reg));
	{
		S64* addr = AddWritableData(L"HeapHnd", 8);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValRIP(8, RefValueAddr(addr, True))));
		ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_DX), ValReg(4, Reg_DX)));
		CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"HeapFree");
	}
#if defined (_DEBUG)
	{
		S64* addr = AddWritableData(L"HeapCnt", 8);
		ListAdd(PackAsm->Asms, AsmDEC(ValRIP(8, RefValueAddr(addr, True))));
	}
#endif
}

static void ChkOverflow(void)
{
	if (!Option->Rls)
	{
		SAsmLabel* lbl1 = AsmLabel();
		ListAdd(PackAsm->Asms, AsmJNO(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
		RaiseExcpt(EXCPT_DBG_INT_OVERFLOW);
		ListAdd(PackAsm->Asms, lbl1);
	}
}

static void RaiseExcpt(U64 code)
{
	S64* addr = AddReadonlyData(sizeof(BlankData), BlankData, False);
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, Reg_CX), ValImmU(4, code)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_DX), ValReg(4, Reg_DX)));
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, Reg_R8), ValImmU(4, 0x01)));
	ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_R9), ValRIP(8, RefValueAddr(addr, True))));
	CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"RaiseException");
}

static void CopyMem(EReg dst, U64 size, EReg src, EReg reg_tmp)
{
	// The values of 'dst', 'src', 'SI', and 'DI' are to be restored.
	// 'reg_tmp' and 'xmm14' are released.
	U64 addr = 0;
	if (size >= 16 * 16)
	{
		// TODO: Handle it in a loop if an excessively large number is specified.
	}
	while (addr + 16 <= size)
	{
		ListAdd(PackAsm->Asms, AsmMOVUPS(ValReg(4, Reg_XMM14), ValMemS(4, ValReg(8, src), NULL, (S32)addr)));
		ListAdd(PackAsm->Asms, AsmMOVUPS(ValMemS(4, ValReg(8, dst), NULL, (S64)addr), ValReg(4, Reg_XMM14)));
		addr += 16;
	}
	if (addr + 8 <= size)
	{
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, reg_tmp), ValMemS(8, ValReg(8, src), NULL, (S32)addr)));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, dst), NULL, (S64)addr), ValReg(8, reg_tmp)));
		addr += 8;
	}
	if (addr + 4 <= size)
	{
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, reg_tmp), ValMemS(4, ValReg(8, src), NULL, (S32)addr)));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(4, ValReg(8, dst), NULL, (S64)addr), ValReg(4, reg_tmp)));
		addr += 4;
	}
	if (addr + 2 <= size)
	{
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(2, reg_tmp), ValMemS(2, ValReg(8, src), NULL, (S32)addr)));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(2, ValReg(8, dst), NULL, (S64)addr), ValReg(2, reg_tmp)));
		addr += 2;
	}
	if (addr + 1 <= size)
	{
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(1, reg_tmp), ValMemS(1, ValReg(8, src), NULL, (S32)addr)));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(1, ValReg(8, dst), NULL, (S64)addr), ValReg(1, reg_tmp)));
		addr += 1;
	}
	ASSERT(addr == size);
}

static void ClearMem(EReg dst, U64 size, EReg reg_tmp)
{
	if (size == 0)
		return;
	{
		// The values of 'dst', 'SI', and 'DI' are to be restored.
		// 'reg_tmp' is released.
		U64 addr = 0;
		// TODO: Handle it in a loop if an excessively large number is specified.
		ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, reg_tmp), ValReg(4, reg_tmp)));
		while (addr + 8 <= size)
		{
			ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, dst), NULL, (S64)addr), ValReg(8, reg_tmp)));
			addr += 8;
		}
		if (addr + 4 <= size)
		{
			ListAdd(PackAsm->Asms, AsmMOV(ValMemS(4, ValReg(8, dst), NULL, (S64)addr), ValReg(4, reg_tmp)));
			addr += 4;
		}
		if (addr + 2 <= size)
		{
			ListAdd(PackAsm->Asms, AsmMOV(ValMemS(2, ValReg(8, dst), NULL, (S64)addr), ValReg(2, reg_tmp)));
			addr += 2;
		}
		if (addr + 1 <= size)
		{
			ListAdd(PackAsm->Asms, AsmMOV(ValMemS(1, ValReg(8, dst), NULL, (S64)addr), ValReg(1, reg_tmp)));
			addr += 1;
		}
		ASSERT(addr == size);
	}
}

static int GetSize(const SAstType* type)
{
	EAstTypeId type_id = ((SAst*)type)->TypeId;
	if ((type_id & AstTypeId_TypeNullable) == AstTypeId_TypeNullable)
		return 8;
	switch (type_id)
	{
		case AstTypeId_TypeBit:
			return ((SAstTypeBit*)type)->Size;
		case AstTypeId_TypePrim:
			switch (((SAstTypePrim*)type)->Kind)
			{
				case AstTypePrimKind_Int: return 8;
				case AstTypePrimKind_Float: return 8;
				case AstTypePrimKind_Char: return 2;
				case AstTypePrimKind_Bool: return 1;
			}
			break;
		case AstTypeId_TypeNull:
			return 8;
	}
	ASSERT(False);
	return 0;
}

static void DbgBreak(void)
{
#if defined(_DEBUG)
	// Set a breakpoint to be caught by the debugger.
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_CX), ValReg(4, Reg_CX)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_DX), ValReg(4, Reg_DX)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R8), ValReg(4, Reg_R8)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R9), ValReg(4, Reg_R9)));
	CallAPI(PackAsm->Asms, L"USER32.dll", L"MessageBoxW");
	ListAdd(PackAsm->Asms, AsmINT(ValImmU(8, 0x03)));
#endif
}

static void RefFuncRecursion(SAstClass* class_)
{
	if (((SAst*)class_)->RefItem != NULL)
		RefFuncRecursion((SAstClass*)((SAst*)class_)->RefItem);
	{
		SListNode* ptr = class_->Items->Top;
		while (ptr != NULL)
		{
			SAstClassItem* item = (SAstClassItem*)ptr->Data;
			if (item->Def->TypeId == AstTypeId_Func)
			{
				ASSERT(item->Addr >= 0);
				RefLocalFunc((SAstFunc*)item->Def);
			}
			ptr = ptr->Next;
		}
	}
}

static const void* InitDlls(const Char* key, const void* value, void* param)
{
	SAsmLabel* lbl1 = AsmLabel();
	SAsmLabel* lbl2 = AsmLabel();
	{
		int len;
		const Char* msg = NewStr(&len, L"data/%s", key);
		S64* addr = AddReadonlyData(sizeof(Char) * (len + 1), (const U8*)msg, False);
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_CX), ValRIP(8, RefValueAddr(addr, True))));
	}
	CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"LoadLibraryW");
	ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_AX), ValImmU(8, 0x00)));
	ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
	{
		S64* addr = AddWritableData(NewStr(NULL, L"$%s", key), 8);
		ListAdd(PackAsm->Asms, AsmMOV(ValRIP(8, RefValueAddr(addr, True)), ValReg(8, Reg_AX)));
	}
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
	DictForEach((SDict*)value, InitDllFuncs, lbl1);
	ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
	ListAdd(PackAsm->Asms, lbl1);
	{
		int len;
		const Char* msg = NewStr(&len, L"File not found: %s", key);
		S64* addr = AddReadonlyData(sizeof(Char) * (len + 1), (const U8*)msg, False);
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_DX), ValRIP(8, RefValueAddr(addr, True))));
	}
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_CX), ValReg(4, Reg_CX)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R8), ValReg(4, Reg_R8)));
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, Reg_R9), ValImmU(4, 0x10 | 0x00010000))); // 'MB_ICONERROR | MB_SETFOREGROUND'
	CallAPI(PackAsm->Asms, L"USER32.dll", L"MessageBoxW");
	ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)param)->Addr, True))));
	ListAdd(PackAsm->Asms, lbl2);
	return value;
}

static const void* InitDllFuncs(const Char* key, const void* value, void* param)
{
	{
		const Char* func_name = (const Char*)value;
		int len = (int)wcslen(func_name);
		U8* name = (U8*)Alloc(len + 1);
		int i;
		for (i = 0; i < len; i++)
		{
			ASSERT(func_name[i] == (Char)(U8)func_name[i]);
			name[i] = (U8)func_name[i];
		}
		name[len] = 0;
		{
			S64* addr = AddReadonlyData(len + 1, name, False);
			ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_DX), ValRIP(8, RefValueAddr(addr, True))));
		}
	}
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValReg(8, Reg_SI)));
	CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"GetProcAddress");
	ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_AX), ValImmU(8, 0x00)));
	ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)param)->Addr, True))));
	{
		S64* addr = AddWritableData(key, 8);
		ListAdd(PackAsm->Asms, AsmMOV(ValRIP(8, RefValueAddr(addr, True)), ValReg(8, Reg_AX)));
	}
	return value;
}

static const void* FinDlls(const Char* key, const void* value, void* param)
{
	SAsmLabel* lbl1 = AsmLabel();
	UNUSED(param);
	{
		S64* addr = AddWritableData(NewStr(NULL, L"$%s", key), 8);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValRIP(8, RefValueAddr(addr, True))));
	}
	ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_CX), ValImmU(8, 0x00)));
	ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
	CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"FreeLibrary");
	ListAdd(PackAsm->Asms, lbl1);
	return value;
}

static void SetTypeId(EReg reg, const SAstType* type)
{
	U8 data[1024];
	int idx = SetTypeIdRecursion(data, 0, type);
	U8* data2 = (U8*)Alloc(idx);
	int i;
	for (i = 0; i < idx; i++)
		data2[i] = data[i];
	{
		S64* addr = AddReadonlyData(idx, data2, False);
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, reg), ValRIP(8, RefValueAddr(addr, True))));
	}
}

static int SetTypeIdRecursion(U8* ptr, int idx, const SAstType* type)
{
	if (IsInt(type))
	{
		if (idx >= 1024)
			goto Err;
		ptr[idx] = TypeId_Int;
		return idx + 1;
	}
	if (IsFloat(type))
	{
		if (idx >= 1024)
			goto Err;
		ptr[idx] = TypeId_Float;
		return idx + 1;
	}
	if (IsChar(type))
	{
		if (idx >= 1024)
			goto Err;
		ptr[idx] = TypeId_Char;
		return idx + 1;
	}
	if (IsBool(type))
	{
		if (idx >= 1024)
			goto Err;
		ptr[idx] = TypeId_Bool;
		return idx + 1;
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeBit)
	{
		if (idx >= 1024)
			goto Err;
		switch (((SAstTypeBit*)type)->Size)
		{
			case 1: ptr[idx] = TypeId_Bit8; break;
			case 2: ptr[idx] = TypeId_Bit16; break;
			case 4: ptr[idx] = TypeId_Bit32; break;
			case 8: ptr[idx] = TypeId_Bit64; break;
			default:
				ASSERT(False);
				break;
		}
		return idx + 1;
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeArray)
	{
		if (idx >= 1024)
			goto Err;
		ptr[idx] = TypeId_Array;
		return SetTypeIdRecursion(ptr, idx + 1, ((SAstTypeArray*)type)->ItemType);
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeGen)
	{
		if (idx >= 1024)
			goto Err;
		switch (((SAstTypeGen*)type)->Kind)
		{
			case AstTypeGenKind_List: ptr[idx] = TypeId_List; break;
			case AstTypeGenKind_Stack: ptr[idx] = TypeId_Stack; break;
			case AstTypeGenKind_Queue: ptr[idx] = TypeId_Queue; break;
			default:
				ASSERT(False);
				break;
		}
		return SetTypeIdRecursion(ptr, idx + 1, ((SAstTypeGen*)type)->ItemType);
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeDict)
	{
		if (idx >= 1024)
			goto Err;
		ptr[idx] = TypeId_Dict;
		idx = SetTypeIdRecursion(ptr, idx + 1, ((SAstTypeDict*)type)->ItemTypeKey);
		return SetTypeIdRecursion(ptr, idx, ((SAstTypeDict*)type)->ItemTypeValue);
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeFunc)
	{
		if (idx >= 1024)
			goto Err;
		ptr[idx] = TypeId_Func;
		return idx + 1;
	}
	if (IsEnum(type))
	{
		if (idx >= 1024)
			goto Err;
		ptr[idx] = TypeId_Enum;
		return idx + 1;
	}
	ASSERT(IsClass(type));
	if (idx >= 1024)
		goto Err;
	ptr[idx] = TypeId_Class;
	return idx + 1;
Err:
	Err(L"EK9999", ((SAst*)type)->Pos);
	return 0;
}

static void SetTypeIdForFromBin(EReg reg, const SAstType* type)
{
	SAstArg* tmp_type = MakeTmpVar(8, NULL);
	SAstArg* tmp_class = MakeTmpVar(8, NULL);
	U8 data_type[1024];
	S64* data_class[128];
	int idx_type = 0;
	int idx_class = 0;
	SAstArg* top_class = NULL;
	int i;
	SetTypeIdForFromBinRecursion(&idx_type, data_type, &idx_class, data_class, type);
	{
		U8* data_type2 = (U8*)Alloc(idx_type);
		for (i = 0; i < idx_type; i++)
			data_type2[i] = data_type[i];
		{
			S64* addr = AddReadonlyData(idx_type, data_type2, False);
			ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, reg), ValRIP(8, RefValueAddr(addr, True))));
			ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_type), False)), ValReg(8, reg)));
			RefLocalVar(tmp_class); // Place 'tmp_class' next to 'tmp_type'.
		}
	}
	for (i = 0; i < idx_class; i++)
	{
		SAstArg* tmp = MakeTmpVar(8, NULL);
		if (i == 0)
			top_class = tmp;
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, reg), ValRIP(8, RefValueAddr(data_class[i], True))));
		ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp), False)), ValReg(8, reg)));
	}
	if (top_class == NULL)
		ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_class), False)), ValImmU(8, 0x00)));
	else
	{
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, reg), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(top_class), False))));
		ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_class), False)), ValReg(8, reg)));
	}
	ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, reg), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_type), False))));
}

static void SetTypeIdForFromBinRecursion(int* idx_type, U8* data_type, int* idx_class, S64** data_class, const SAstType* type)
{
	if (IsInt(type))
	{
		if (*idx_type >= 1024)
			goto Err;
		data_type[*idx_type] = TypeId_Int;
		(*idx_type)++;
		return;
	}
	if (IsFloat(type))
	{
		if (*idx_type >= 1024)
			goto Err;
		data_type[*idx_type] = TypeId_Float;
		(*idx_type)++;
		return;
	}
	if (IsChar(type))
	{
		if (*idx_type >= 1024)
			goto Err;
		data_type[*idx_type] = TypeId_Char;
		(*idx_type)++;
		return;
	}
	if (IsBool(type))
	{
		if (*idx_type >= 1024)
			goto Err;
		data_type[*idx_type] = TypeId_Bool;
		(*idx_type)++;
		return;
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeBit)
	{
		if (*idx_type >= 1024)
			goto Err;
		switch (((SAstTypeBit*)type)->Size)
		{
			case 1: data_type[*idx_type] = TypeId_Bit8; break;
			case 2: data_type[*idx_type] = TypeId_Bit16; break;
			case 4: data_type[*idx_type] = TypeId_Bit32; break;
			case 8: data_type[*idx_type] = TypeId_Bit64; break;
			default:
				ASSERT(False);
				break;
		}
		(*idx_type)++;
		return;
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeArray)
	{
		if (*idx_type >= 1024)
			goto Err;
		data_type[*idx_type] = TypeId_Array;
		(*idx_type)++;
		SetTypeIdForFromBinRecursion(idx_type, data_type, idx_class, data_class, ((SAstTypeArray*)type)->ItemType);
		return;
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeGen)
	{
		if (*idx_type >= 1024)
			goto Err;
		switch (((SAstTypeGen*)type)->Kind)
		{
			case AstTypeGenKind_List: data_type[*idx_type] = TypeId_List; break;
			case AstTypeGenKind_Stack: data_type[*idx_type] = TypeId_Stack; break;
			case AstTypeGenKind_Queue: data_type[*idx_type] = TypeId_Queue; break;
			default:
				ASSERT(False);
				break;
		}
		(*idx_type)++;
		SetTypeIdForFromBinRecursion(idx_type, data_type, idx_class, data_class, ((SAstTypeGen*)type)->ItemType);
		return;
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeDict)
	{
		if (*idx_type >= 1024)
			goto Err;
		data_type[*idx_type] = TypeId_Dict;
		(*idx_type)++;
		SetTypeIdForFromBinRecursion(idx_type, data_type, idx_class, data_class, ((SAstTypeDict*)type)->ItemTypeKey);
		SetTypeIdForFromBinRecursion(idx_type, data_type, idx_class, data_class, ((SAstTypeDict*)type)->ItemTypeValue);
		return;
	}
	if (((SAst*)type)->TypeId == AstTypeId_TypeFunc)
	{
		if (*idx_type >= 1024)
			goto Err;
		data_type[*idx_type] = TypeId_Func;
		(*idx_type)++;
		return;
	}
	if (IsEnum(type))
	{
		if (*idx_type >= 1024)
			goto Err;
		data_type[*idx_type] = TypeId_Enum;
		(*idx_type)++;
		return;
	}
	ASSERT(IsClass(type));
	if (*idx_type >= 1024)
		goto Err;
	data_type[*idx_type] = TypeId_Class;
	(*idx_type)++;
	if (*idx_type >= 1024)
		goto Err;
	data_type[*idx_type] = (U8)*idx_class;
	(*idx_type)++;
	{
		SAstClass* class_ = (SAstClass*)((SAst*)type)->RefItem;
		S64* addr = RefClass(class_);
		if (*idx_class >= 128)
			goto Err;
		data_class[*idx_class] = addr;
		(*idx_class)++;
	}
	return;
Err:
	Err(L"EK9999", ((SAst*)type)->Pos);
	return;
}

static void ExpandMe(SAstExprDot* dot, int reg_i)
{
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x08)));
	ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x08 + dot->ClassItem->Addr)));
	ListAdd(PackAsm->Asms, AsmADD(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00)));
}

static void SetStatAsm(SAstStat* ast, SListNode* asms_top, SListNode* asms_bottom)
{
	if (asms_top == NULL || asms_top->Next == NULL)
	{
		if (PackAsm->Asms->Top != NULL)
			ast->AsmTop = (SAsm*)PackAsm->Asms->Top->Data;
	}
	else
		ast->AsmTop = (SAsm*)asms_top->Next->Data;
	if (asms_bottom != NULL)
		ast->AsmBottom = (SAsm*)asms_bottom->Data;
}

static void AssembleFunc(SAstFunc* ast, Bool entry)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ast->RetPoint = AsmLabel();
	{
		SExcptTableTry* scope_entry = NULL;
		SExcptTableTry* scope_gc_dec = NULL;
		S64* var_size = NewAddr(); // The total size of the variables in the function.
		SAstArg* excpt = NULL; // A variable that records the exception information in the exception function.
		SExcptTable* table = (SExcptTable*)Alloc(sizeof(SExcptTable));
		table->Begin = AsmLabel();
		table->End = AsmLabel();
		table->PostPrologue = AsmLabel();
		table->TryScopes = ListNew();
		table->Addr = -1;
		ListAdd(PackAsm->ExcptTables, table);
		ListAdd(PackAsm->Asms, table->Begin);

		if (entry)
		{
			scope_entry = (SExcptTableTry*)Alloc(sizeof(SExcptTableTry));
			ListAdd(table->TryScopes, scope_entry);
			scope_entry->End = AsmLabel(); // Make the instance to be jumped when an error occurs.
		}
		if (((SAst*)ast)->TypeId != AstTypeId_FuncRaw)
		{
			scope_gc_dec = (SExcptTableTry*)Alloc(sizeof(SExcptTableTry));
			ListAdd(table->TryScopes, scope_gc_dec);
		}

		LocalVars = StackPush(LocalVars, ListNew());
		RefFuncArgNums = StackPush(RefFuncArgNums, ListNew());
		{
			SListNode* ptr = ast->Args->Top;
			while (ptr != NULL)
			{
				*((SAstArg*)ptr->Data)->Addr = RegisteredAddr;
				ptr = ptr->Next;
			}
		}

		if (((SAst*)ast)->TypeId == AstTypeId_FuncRaw && ((SAstFuncRaw*)ast)->Header != NULL)
		{
			SListNode* ptr = ((SAstFuncRaw*)ast)->Header->Top;
			while (ptr != NULL)
			{
				ListAdd(PackAsm->Asms, ptr->Data);
				ptr = ptr->Next;
			}
		}
		else
			ListAdd(PackAsm->Asms, AsmSUB(ValReg(8, Reg_SP), ValImm(4, RefValueAddr(var_size, False))));
		ListAdd(PackAsm->Asms, table->PostPrologue);
		if (((SAst*)ast)->TypeId != AstTypeId_FuncRaw)
		{
			// Clear the stack area.
			ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_DI), ValMemS(8, ValReg(8, Reg_SP), NULL, 0x00)));
			ListAdd(PackAsm->Asms, AsmXOR(ValReg(1, Reg_AX), ValReg(1, Reg_AX)));
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, Reg_CX), ValImm(4, RefValueAddr(var_size, False))));
			ListAdd(PackAsm->Asms, AsmREPSTOS(ValReg(1, Reg_AX)));
		}
		if (entry)
		{
			// Initialize the program.
			{
				SAstArg* tmp = MakeTmpVar(4, NULL);
				ListAdd(PackAsm->Asms, AsmMOV(ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp), False)), ValImmU(4, 0x00007f00)));
				ListAdd(PackAsm->Asms, AsmLDMXCSR(ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp), False))));
			}
			{
				ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_CX), ValReg(4, Reg_CX)));
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DX), ValImmU(8, 4096))); // The initial size of the heap.
				ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R8), ValReg(4, Reg_R8)));
				CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"HeapCreate");
				{
					S64* addr = AddWritableData(L"HeapHnd", 8);
					ListAdd(PackAsm->Asms, AsmMOV(ValRIP(8, RefValueAddr(addr, True)), ValReg(8, Reg_AX)));
				}
				{
					S64* addr = AddWritableData(L"ExitCode", 8);
					ListAdd(PackAsm->Asms, AsmMOV(ValRIP(8, RefValueAddr(addr, True)), ValImmU(8, 0x00)));
				}
#if defined(_DEBUG)
				{
					S64* addr = AddWritableData(L"HeapCnt", 8);
					ListAdd(PackAsm->Asms, AsmMOV(ValRIP(8, RefValueAddr(addr, True)), ValImmU(8, 0x00)));
				}
#endif
			}

			// Initialize the Dlls.
			{
				SAsmLabel* lbl1 = AsmLabel();
				DictForEach(Dlls, InitDlls, scope_entry->End);
#if defined(_DEBUG)
				// DbgBreak();
#endif
				ListAdd(PackAsm->Asms, lbl1);
			}
		}
		AssembleStats(ast->Stats);
		if (((SAst*)ast)->TypeId == AstTypeId_FuncRaw)
		{
			SListNode* ptr = ((SAstFuncRaw*)ast)->Asms->Top;
			while (ptr != NULL)
			{
				ListAdd(PackAsm->Asms, ptr->Data);
				ptr = ptr->Next;
			}
			ListAdd((SList*)StackPeek(RefFuncArgNums), (void*)(S64)((SAstFuncRaw*)ast)->ArgNum);
		}
		if ((ast->FuncAttr & FuncAttr_ExitCode) != 0)
		{
			// Set 'ExitCode'.
			{
				S64* addr = AddWritableData(L"ExitCode", 8);
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(((SAstArg*)ast->Args->Top->Data)->Addr, False))));
				ListAdd(PackAsm->Asms, AsmMOV(ValRIP(8, RefValueAddr(addr, True)), ValReg(8, Reg_CX)));
			}
		}
		if (ast->DllName != NULL)
		{
			// Call functions in Dlls.
			if ((ast->FuncAttr & FuncAttr_Init) != 0)
			{
				// Pass necessary information to '_init' functions.
				{
					S64* addr = AddWritableData(L"HeapHnd", 8);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValRIP(8, RefValueAddr(addr, True))));
				}
#if defined (_DEBUG)
				{
					S64* addr = AddWritableData(L"HeapCnt", 8);
					ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_DX), ValRIP(8, RefValueAddr(addr, True))));
				}
#else
				ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_DX), ValReg(4, Reg_DX)));
#endif
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_R8), ValImmS(8, PackAsm->AppCode)));
				{
					S64* addr = AddReadonlyData((int)USE_RES_FLAGS_LEN, MakeUseResFlags, False);
					ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_R9), ValRIP(8, RefValueAddr(addr, True))));
				}
			}
			else
			{
				SListNode* ptr = ast->Args->Top;
				int idx = 0;
				while (ptr != NULL)
				{
					SAstArg* arg = (SAstArg*)ptr->Data;
					if (idx < 4)
					{
						// Arguments passed in registers.
						if (arg->RefVar)
						{
							EReg reg = Reg_CX;
							switch (idx)
							{
								case 0: reg = Reg_CX; break;
								case 1: reg = Reg_DX; break;
								case 2: reg = Reg_R8; break;
								case 3: reg = Reg_R9; break;
								default:
									ASSERT(False);
									break;
							}
							ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, reg), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(arg->Addr, False))));
						}
						else if (IsFloat(arg->Type))
						{
							EReg reg = Reg_XMM0;
							switch (idx)
							{
								case 0: reg = Reg_XMM0; break;
								case 1: reg = Reg_XMM1; break;
								case 2: reg = Reg_XMM2; break;
								case 3: reg = Reg_XMM3; break;
								default:
									ASSERT(False);
									break;
							}
							ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, reg), ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(arg->Addr, False))));
						}
						else
						{
							int size = GetSize(arg->Type);
							EReg reg = Reg_CX;
							switch (idx)
							{
								case 0: reg = Reg_CX; break;
								case 1: reg = Reg_DX; break;
								case 2: reg = Reg_R8; break;
								case 3: reg = Reg_R9; break;
								default:
									ASSERT(False);
									break;
							}
							ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, reg), ValMem(size, ValReg(8, Reg_SP), NULL, RefValueAddr(arg->Addr, False))));
						}
					}
					else
					{
						// Arguments passed in the stack area.
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(arg->Addr, False))));
						ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, (S64)idx * 8), ValReg(8, Reg_SI)));
					}
					ptr = ptr->Next;
					idx++;
				}
			}
			{
				S64* addr;
				if (ast->DllFuncName != NULL)
					addr = AddWritableData(NewStr(NULL, L"%s$%s", ast->DllName, ast->DllFuncName), 8);
				else
					addr = AddWritableData(NewStr(NULL, L"%s$%s", ast->DllName, ((SAst*)ast)->Name), 8);
				ListAdd(PackAsm->Asms, AsmCALL(ValRIP(8, RefValueAddr(addr, True))));
			}
		}
		else if (((SAst*)ast)->TypeId != AstTypeId_FuncRaw && ast->Ret != NULL)
		{
			// Return 0 when 'ret' is not explicitly written.
			if (IsFloat(ast->Ret))
				ListAdd(PackAsm->Asms, AsmXORPD(ValReg(4, Reg_XMM0), ValReg(4, Reg_XMM0)));
			else
				ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_AX), ValReg(4, Reg_AX)));
		}
		ListAdd(PackAsm->Asms, ast->RetPoint);

		if (((SAst*)ast)->TypeId != AstTypeId_FuncRaw)
		{
			if (ast->Ret != NULL)
			{
				if (IsRef(ast->Ret))
					GcInc(0);
			}
			{
				scope_gc_dec->Begin = table->Begin;
				scope_gc_dec->End = AsmLabel();
				excpt = MakeTmpVar(8, NULL);
				ListAdd(PackAsm->Asms, AsmNOP());
				ListAdd(PackAsm->Asms, scope_gc_dec->End);
				{
					// Save the return value so that it is not overwritten.
					if (ast->Ret != NULL && !IsFloat(ast->Ret))
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
					// Decrement the reference counters of local values.
					{
						SListNode* ptr = ((SList*)StackPeek(LocalVars))->Top;
						while (ptr != NULL)
						{
							SAstArg* arg = (SAstArg*)ptr->Data;
							ASSERT(!arg->RefVar);
							if (IsRef(arg->Type))
							{
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(arg), False))));
								GcDec(0, -1, arg->Type);
							}
							ptr = ptr->Next;
						}
					}
					{
						SListNode* ptr = ast->Args->Top;
						while (ptr != NULL)
						{
							SAsmLabel* lbl1 = NULL;
							if ((ast->FuncAttr & FuncAttr_AnyType) != 0 && ptr == ast->Args->Top || (ast->FuncAttr & FuncAttr_TakeMe) != 0 && ptr == ast->Args->Top->Next->Next)
							{
								lbl1 = AsmLabel();
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar((SAstArg*)ast->Args->Top->Next->Data), False))));
							}
							else if ((ast->FuncAttr & FuncAttr_TakeChild) != 0 && ptr == ast->Args->Top->Next->Next || (ast->FuncAttr & FuncAttr_TakeKeyValue) != 0 && ptr == ast->Args->Top->Next->Next->Next)
							{
								lbl1 = AsmLabel();
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar((SAstArg*)ast->Args->Top->Next->Data), False))));
								ListAdd(PackAsm->Asms, AsmINC(ValReg(8, RegI[0])));
							}
							else if ((ast->FuncAttr & FuncAttr_TakeKeyValue) != 0 && ptr == ast->Args->Top->Next->Next->Next->Next)
							{
								lbl1 = AsmLabel();
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar((SAstArg*)ast->Args->Top->Next->Next->Data), False))));
							}
							if (lbl1 != NULL)
							{
								ASSERT((ast->FuncAttr & FuncAttr_AnyType) != 0);
								ASSERT(((SAst*)((SAstArg*)ptr->Data)->Type)->TypeId == AstTypeId_TypeArray && ((SAst*)((SAstTypeArray*)((SAstArg*)ptr->Data)->Type)->ItemType)->TypeId == AstTypeId_TypeBit && ((SAstTypeBit*)((SAstTypeArray*)((SAstArg*)ptr->Data)->Type)->ItemType)->Size == 1);
								ListAdd(PackAsm->Asms, AsmCMP(ValMemS(1, ValReg(8, RegI[0]), NULL, 0x00), ValImmU(1, 0x80)));
								ListAdd(PackAsm->Asms, AsmJB(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
							}
							{
								SAstArg* arg = (SAstArg*)ptr->Data;
								if (IsRef(arg->Type) && !arg->RefVar)
								{
									ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(arg), False))));
									GcDec(0, -1, arg->Type);
								}
							}
							if (lbl1 != NULL)
								ListAdd(PackAsm->Asms, lbl1);
							ptr = ptr->Next;
						}
					}
					if (ast->Ret != NULL && !IsFloat(ast->Ret))
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_AX), ValReg(8, Reg_SI)));
				}
				{
					// Throw the exception again if it has occurred.
					SAsmLabel* lbl1 = AsmLabel();
					ListAdd(PackAsm->Asms, AsmCMP(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(excpt), False)), ValImmU(8, 0x00)));
					ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, Reg_CX), ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(excpt), False))));
					ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_DX), ValReg(4, Reg_DX)));
					ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R8), ValReg(4, Reg_R8)));
					ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R9), ValReg(4, Reg_R9)));
					CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"RaiseException");
					ListAdd(PackAsm->Asms, lbl1);
				}
				{
					// The exception function.
					SAstFuncRaw* func = (SAstFuncRaw*)Alloc(sizeof(SAstFuncRaw));
					memset(func, 0, sizeof(SAst));
					((SAst*)func)->TypeId = AstTypeId_FuncRaw;
					((SAst*)func)->AnalyzedCache = (SAst*)func;
					((SAstFunc*)func)->AddrTop = NewAddr();
					((SAstFunc*)func)->AddrBottom = -1;
					((SAstFunc*)func)->PosRowBottom = -1;
					((SAstFunc*)func)->DllName = NULL;
					((SAstFunc*)func)->DllFuncName = NULL;
					((SAstFunc*)func)->FuncAttr = FuncAttr_None;
					((SAstFunc*)func)->Args = ListNew();
					((SAstFunc*)func)->Ret = NULL;
					((SAstFunc*)func)->Stats = ListNew();
					((SAstFunc*)func)->RetPoint = NULL;
					func->Asms = ListNew();
					func->ArgNum = 0;
					func->Header = NULL;
					ListAdd(func->Asms, AsmMOV(ValReg(8, Reg_AX), ValMemS(8, ValReg(8, Reg_CX), NULL, 0x00)));
					ListAdd(func->Asms, AsmMOV(ValReg(4, Reg_AX), ValMemS(4, ValReg(8, Reg_AX), NULL, 0x00)));
					ListAdd(func->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_DX), NULL, RefValueAddr(RefLocalVar(excpt), False)), ValReg(8, Reg_AX)));
					ListAdd(func->Asms, AsmMOV(ValReg(4, Reg_AX), ValImmU(4, 0x01)));
					scope_gc_dec->CatchFunc = RefLocalFunc((SAstFunc*)func);
				}
			}
		}

		if (entry)
		{
			scope_entry->Begin = table->Begin;
			scope_entry->CatchFunc = NULL;
			ListAdd(PackAsm->Asms, AsmNOP());
			ListAdd(PackAsm->Asms, scope_entry->End);
			// Finalize the program.
			{
				// Finalize the Dlls.
				SAsmLabel* lbl1 = AsmLabel();
				DictForEach(Dlls, FinDlls, NULL);
				ListAdd(PackAsm->Asms, lbl1);
			}
			{
#if defined (_DEBUG)
				{
					SAsmLabel* lbl1 = AsmLabel();
					S64* addr = AddWritableData(L"HeapCnt", 8);
					ListAdd(PackAsm->Asms, AsmCMP(ValRIP(8, RefValueAddr(addr, True)), ValImmU(8, 0x00)));
					ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
					ListAdd(PackAsm->Asms, AsmINT(ValImmU(8, 0x03)));
					ListAdd(PackAsm->Asms, lbl1);
				}
#endif
				{
					S64* addr = AddWritableData(L"HeapHnd", 8);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValRIP(8, RefValueAddr(addr, True))));
					CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"HeapDestroy");
				}
			}
			{
				S64* addr = AddWritableData(L"ExitCode", 8);
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValRIP(8, RefValueAddr(addr, True))));
				CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"ExitProcess");
				ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_AX), ValReg(4, Reg_AX)));
			}
		}

		{
			// Calculate the size required for the stack area.
			int size;
			table->StackSize = 0;
			{
				// Calculate the size of the arguments.
				int max_arg_num = 4; // According to the specification, at least four arguments must be reserved.
				if (ast->DllName != NULL)
				{
					// Arguments for DLL calls.
					if (max_arg_num < ast->Args->Len)
						max_arg_num = ast->Args->Len;
				}
				{
					SListNode* ptr = ((SList*)StackPeek(RefFuncArgNums))->Top;
					while (ptr != NULL)
					{
						int arg_num = (int)(S64)ptr->Data;
						if (arg_num == -1)
							table->StackSize = -1; // For the special function of exception handling.
						else if (max_arg_num < arg_num)
							max_arg_num = arg_num;
						ptr = ptr->Next;
					}
				}
				size = max_arg_num * 8;
			}
			{
				// Calculate the size of local variables and update their addresses.
				SListNode* ptr = ((SList*)StackPeek(LocalVars))->Top;
				while (ptr != NULL)
				{
					SAstArg* arg = (SAstArg*)ptr->Data;
					int size2 = GetSize(arg->Type);
					if (size % size2 != 0)
						size += size2 - size % size2;
					*arg->Addr = size;
					size += size2;
					ptr = ptr->Next;
				}
			}
			size += 8;
			if ((size + 8) % 16 != 0)
				size += 16 - (size + 8) % 16; // Align to 16 bytes according to the specification.
			*var_size = size;
			if (table->StackSize != -1)
				table->StackSize = size;
			{
				// Update the addresses of arguments.
				SListNode* ptr = ast->Args->Top;
				int addr = 8;
				while (ptr != NULL)
				{
					*((SAstArg*)ptr->Data)->Addr = size + addr;
					addr += 8;
					ptr = ptr->Next;
				}
			}
		}

		if (!(((SAst*)ast)->TypeId == AstTypeId_FuncRaw && ((SAstFuncRaw*)ast)->Header != NULL))
		{
			// Restore the stack pointer and exit the function.
			if (*var_size != 0)
				ListAdd(PackAsm->Asms, AsmADD(ValReg(8, Reg_SP), ValImmS(8, *var_size)));
			ListAdd(PackAsm->Asms, AsmRET());
		}
		RefFuncArgNums = StackPop(RefFuncArgNums);
		LocalVars = StackPop(LocalVars);
		ListAdd(PackAsm->Asms, table->End);
		ListAdd(PackAsm->Asms, AsmINT(ValImmU(8, 0x03)));
	}
}

static void AssembleStats(SList* asts)
{
	SListNode* ptr = asts->Top;
	while (ptr != NULL)
	{
		SAstStat* ast = (SAstStat*)ptr->Data;
		SListNode* asms_bottom = PackAsm->Asms->Bottom;
		switch (((SAst*)ast)->TypeId)
		{
			case AstTypeId_StatIf: AssembleIf((SAstStatIf*)ast); break;
			case AstTypeId_StatSwitch: AssembleSwitch((SAstStatSwitch*)ast); break;
			case AstTypeId_StatWhile: AssembleWhile((SAstStatWhile*)ast); break;
			case AstTypeId_StatFor: AssembleFor((SAstStatFor*)ast); break;
			case AstTypeId_StatTry: AssembleTry((SAstStatTry*)ast); break;
			case AstTypeId_StatThrow: AssembleThrow((SAstStatThrow*)ast); break;
			case AstTypeId_StatBlock: AssembleBlock((SAstStatBlock*)ast); break;
			case AstTypeId_StatRet: AssembleRet((SAstStatRet*)ast); break;
			case AstTypeId_StatDo: AssembleDo((SAstStatDo*)ast); break;
			case AstTypeId_StatBreak: AssembleBreak((SAstStat*)ast); break;
			case AstTypeId_StatSkip: AssembleSkip((SAstStat*)ast); break;
			case AstTypeId_StatAssert: AssembleAssert((SAstStatAssert*)ast); break;
			default:
				ASSERT(False);
				break;
		}
		if (((SAst*)ast)->TypeId != AstTypeId_StatBlock)
			SetStatAsm(ast, asms_bottom, PackAsm->Asms->Bottom);
		ptr = ptr->Next;
	}
}

static void AssembleIf(SAstStatIf* ast)
{
	if (ast->Cond == NULL)
	{
		// Optimized code.
		ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
		AssembleBlock(ast->StatBlock);
		ListAdd(PackAsm->Asms, ((SAstStatBreakable*)ast)->BreakPoint);
	}
	else
	{
		SAsmLabel* lbl1 = AsmLabel();
		SAsmLabel* lbl2 = AsmLabel();
		ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
		AssembleExpr(ast->Cond, 0, 0);
		ToValue(ast->Cond, 0, 0);
		ListAdd(PackAsm->Asms, AsmCMP(ValReg(1, RegI[0]), ValImmU(1, 0x00)));
		ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
		AssembleBlock(ast->StatBlock);
		ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
		ListAdd(PackAsm->Asms, lbl1);
		{
			SListNode* ptr = ast->ElIfs->Top;
			while (ptr != NULL)
			{
				SAstStatElIf* elif = (SAstStatElIf*)ptr->Data;
				SAsmLabel* lbl3 = AsmLabel();
				AssembleExpr(elif->Cond, 0, 0);
				ToValue(elif->Cond, 0, 0);
				ListAdd(PackAsm->Asms, AsmCMP(ValReg(1, RegI[0]), ValImmU(1, 0x00)));
				ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl3)->Addr, True))));
				AssembleBlock(elif->StatBlock);
				ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
				ListAdd(PackAsm->Asms, lbl3);
				ptr = ptr->Next;
			}
		}
		if (ast->ElseStatBlock != NULL)
			AssembleBlock(ast->ElseStatBlock);
		ListAdd(PackAsm->Asms, lbl2);
		ListAdd(PackAsm->Asms, ((SAstStatBreakable*)ast)->BreakPoint);
	}
}

static void AssembleSwitch(SAstStatSwitch* ast)
{
	int case_num = ast->Cases->Len;
	SAsmLabel** lbl1 = (SAsmLabel**)Alloc(sizeof(SAsmLabel*) * (size_t)case_num);
	SAsmLabel* lbl2 = AsmLabel();
	int size = GetSize(ast->Cond->Type);
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	{
		int i;
		for (i = 0; i < case_num; i++)
			lbl1[i] = AsmLabel();
	}
	AssembleExpr(ast->Cond, 0, 0);
	ToValue(ast->Cond, 0, 0);
	if (IsRef(ast->Cond->Type))
	{
		GcInc(0);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, RegI[0])));
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(size, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False))));
		GcDec(0, -1, ((SAstStatBreakable*)ast)->BlockVar->Type);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValReg(8, Reg_SI)));
	}
	if (IsFloat(ast->Cond->Type))
		ListAdd(PackAsm->Asms, AsmMOVSD(ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False)), ValReg(4, RegF[0])));
	else
		ListAdd(PackAsm->Asms, AsmMOV(ValMem(size, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False)), ValReg(size, RegI[0])));
	{
		SListNode* ptr = ast->Cases->Top;
		int idx = 0;
		while (ptr != NULL)
		{
			SListNode* ptr2 = ((SAstStatCase*)ptr->Data)->Conds->Top;
			while (ptr2 != NULL)
			{
				SAstExpr** exprs = (SAstExpr**)ptr2->Data;
				Bool signed_;
				if (IsFloat(ast->Cond->Type))
				{
					AssembleExpr(exprs[0], 0, 1);
					ToValue(exprs[0], 0, 1);
				}
				else
				{
					AssembleExpr(exprs[0], 1, 0);
					ToValue(exprs[0], 1, 0);
				}
				if (((SAst*)ast->Cond->Type)->TypeId == AstTypeId_TypeBit || IsChar(ast->Cond->Type))
				{
					ListAdd(PackAsm->Asms, AsmCMP(ValReg(size, RegI[0]), ValReg(size, RegI[1])));
					signed_ = False;
				}
				else if (IsInt(ast->Cond->Type) || IsEnum(ast->Cond->Type))
				{
					ListAdd(PackAsm->Asms, AsmCMP(ValReg(size, RegI[0]), ValReg(size, RegI[1])));
					signed_ = True;
				}
				else if (IsFloat(ast->Cond->Type))
				{
					ListAdd(PackAsm->Asms, AsmCOMISD(ValReg(4, RegF[0]), ValReg(4, RegF[1])));
					signed_ = False;
				}
				else if (IsStr(ast->Cond->Type))
				{
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, RegI[0])));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[2]), ValReg(8, RegI[1])));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[1]), ValReg(8, RegI[0])));
					CallKuinLib(L"_cmpStr");
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DI), ValReg(8, Reg_AX)));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValReg(8, Reg_SI)));
					ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_DI), ValImmU(8, 0x00)));
					signed_ = True;
				}
				else
				{
					SAstArg* tmp_reg_i = MakeTmpVar(8, NULL);
					ASSERT(IsClass(ast->Cond->Type));
					ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_reg_i), False)), ValReg(8, RegI[0])));
					// The second argument.
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[2]), ValReg(8, RegI[1])));
					GcInc(2); // Increment the reference counters of instances passed as arguments.
					ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, 0x08), ValReg(8, RegI[2])));
					// The first argument.
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[1]), ValReg(8, RegI[0])));
					GcInc(1);
					ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, 0x00), ValReg(8, RegI[1])));
					// Call the comparison method.
					ListAdd(PackAsm->Asms, AsmCALL(ValMemS(8, ValReg(8, RegI[1]), NULL, 0x20)));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_reg_i), False))));
					ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_SI), ValImmU(8, 0x00)));
					signed_ = True;
				}
				if (exprs[1] == NULL)
					ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1[idx])->Addr, True))));
				else
				{
					SAsmLabel* lbl3 = AsmLabel();
					if (signed_)
						ListAdd(PackAsm->Asms, AsmJL(ValImm(4, RefValueAddr(((SAsm*)lbl3)->Addr, True))));
					else
						ListAdd(PackAsm->Asms, AsmJB(ValImm(4, RefValueAddr(((SAsm*)lbl3)->Addr, True))));
					if (IsFloat(ast->Cond->Type))
					{
						AssembleExpr(exprs[1], 0, 1);
						ToValue(exprs[1], 0, 1);
					}
					else
					{
						AssembleExpr(exprs[1], 1, 0);
						ToValue(exprs[1], 1, 0);
					}
					if (((SAst*)ast->Cond->Type)->TypeId == AstTypeId_TypeBit || IsChar(ast->Cond->Type) || IsInt(ast->Cond->Type) || IsEnum(ast->Cond->Type))
						ListAdd(PackAsm->Asms, AsmCMP(ValReg(size, RegI[0]), ValReg(size, RegI[1])));
					else if (IsFloat(ast->Cond->Type))
						ListAdd(PackAsm->Asms, AsmCOMISD(ValReg(4, RegF[0]), ValReg(4, RegF[1])));
					else if (IsStr(ast->Cond->Type))
					{
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, RegI[0])));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[2]), ValReg(8, RegI[1])));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[1]), ValReg(8, RegI[0])));
						CallKuinLib(L"_cmpStr");
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DI), ValReg(8, Reg_AX)));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValReg(8, Reg_SI)));
						ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_DI), ValImmU(8, 0x00)));
					}
					else
					{
						SAstArg* tmp_reg_i = MakeTmpVar(8, NULL);
						ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_reg_i), False)), ValReg(8, RegI[0])));
						ASSERT(IsClass(ast->Cond->Type));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[2]), ValReg(8, RegI[1])));
						GcInc(2);
						ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, 0x08), ValReg(8, RegI[2])));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[1]), ValReg(8, RegI[0])));
						GcInc(1);
						ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, 0x00), ValReg(8, RegI[1])));
						ListAdd(PackAsm->Asms, AsmCALL(ValMemS(8, ValReg(8, RegI[1]), NULL, 0x20)));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_reg_i), False))));
						ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_SI), ValImmU(8, 0x00)));
					}
					if (signed_)
						ListAdd(PackAsm->Asms, AsmJLE(ValImm(4, RefValueAddr(((SAsm*)lbl1[idx])->Addr, True))));
					else
						ListAdd(PackAsm->Asms, AsmJBE(ValImm(4, RefValueAddr(((SAsm*)lbl1[idx])->Addr, True))));
					ListAdd(PackAsm->Asms, lbl3);
				}
				ptr2 = ptr2->Next;
			}
			ptr = ptr->Next;
			idx++;
		}
	}
	if (ast->DefaultStatBlock != NULL)
		AssembleBlock(ast->DefaultStatBlock);
	ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
	{
		SListNode* ptr = ast->Cases->Top;
		int idx = 0;
		while (ptr != NULL)
		{
			ListAdd(PackAsm->Asms, lbl1[idx]);
			AssembleBlock(((SAstStatCase*)ptr->Data)->StatBlock);
			if (ptr->Next != NULL)
				ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
			ptr = ptr->Next;
			idx++;
		}
	}
	ListAdd(PackAsm->Asms, lbl2);
	ListAdd(PackAsm->Asms, ((SAstStatBreakable*)ast)->BreakPoint);
	if (IsRef(ast->Cond->Type))
	{
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(size, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False))));
		GcDec(0, -1, ((SAstStatBreakable*)ast)->BlockVar->Type);
		ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False)), ValImmU(8, 0x00)));
	}
}

static void AssembleWhile(SAstStatWhile* ast)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	if (ast->Cond == NULL)
	{
		SAsmLabel* lbl1 = AsmLabel();
		ListAdd(PackAsm->Asms, lbl1);
		AssembleStats(ast->Stats);
		ListAdd(PackAsm->Asms, ((SAstStatSkipable*)ast)->SkipPoint);
		ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
		ListAdd(PackAsm->Asms, ((SAstStatBreakable*)ast)->BreakPoint);
	}
	else if (ast->Skip)
	{
		SAsmLabel* lbl1 = AsmLabel();
		ListAdd(PackAsm->Asms, lbl1);
		AssembleStats(ast->Stats);
		ListAdd(PackAsm->Asms, ((SAstStatSkipable*)ast)->SkipPoint);
		AssembleExpr(ast->Cond, 0, 0);
		ToValue(ast->Cond, 0, 0);
		ListAdd(PackAsm->Asms, AsmCMP(ValReg(1, RegI[0]), ValImmU(1, 0x00)));
		ListAdd(PackAsm->Asms, AsmJNE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
		ListAdd(PackAsm->Asms, ((SAstStatBreakable*)ast)->BreakPoint);
	}
	else
	{
		SAsmLabel* lbl1 = AsmLabel();
		SAsmLabel* lbl2 = AsmLabel();
		ListAdd(PackAsm->Asms, lbl1);
		ListAdd(PackAsm->Asms, ((SAstStatSkipable*)ast)->SkipPoint);
		AssembleExpr(ast->Cond, 0, 0);
		ToValue(ast->Cond, 0, 0);
		ListAdd(PackAsm->Asms, AsmCMP(ValReg(1, RegI[0]), ValImmU(1, 0x00)));
		ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
		AssembleStats(ast->Stats);
		ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
		ListAdd(PackAsm->Asms, lbl2);
		ListAdd(PackAsm->Asms, ((SAstStatBreakable*)ast)->BreakPoint);
	}
}

static void AssembleFor(SAstStatFor* ast)
{
	SAstArg* cond = MakeTmpVar(8, NULL);
	SAsmLabel* lbl1 = AsmLabel();
	SAsmLabel* lbl2 = AsmLabel();
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	AssembleExpr(ast->Start, 0, 0);
	ToValue(ast->Start, 0, 0);
	ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False)), ValReg(8, RegI[0])));
	AssembleExpr(ast->Cond, 0, 0);
	ToValue(ast->Cond, 0, 0);
	ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(cond), False)), ValReg(8, RegI[0])));
	ListAdd(PackAsm->Asms, lbl1);
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[0]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(cond), False))));
	ListAdd(PackAsm->Asms, AsmCMP(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False)), ValReg(8, RegI[0])));
	{
		S64 n;
		ASSERT(((SAst*)ast->Step)->TypeId == AstTypeId_ExprValue);
		n = *(S64*)((SAstExprValue*)ast->Step)->Value;
		if (n > 0)
			ListAdd(PackAsm->Asms, AsmJG(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
		else
		{
			ASSERT(n < 0);
			ListAdd(PackAsm->Asms, AsmJL(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
		}
		AssembleStats(ast->Stats);
		ListAdd(PackAsm->Asms, ((SAstStatSkipable*)ast)->SkipPoint);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValImmS(8, n)));
		ListAdd(PackAsm->Asms, AsmADD(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False)), ValReg(8, Reg_SI)));
		ListAdd(PackAsm->Asms, AsmJO(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
	}
	ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
	ListAdd(PackAsm->Asms, lbl2);
	ListAdd(PackAsm->Asms, ((SAstStatBreakable*)ast)->BreakPoint);
}

static void AssembleTry(SAstStatTry* ast)
{
	SExcptTableTry* scope_catch = NULL;
	SExcptTableTry* scope_finally = NULL;
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ListAdd(PackAsm->Asms, AsmNOP());
	if (ast->FinallyStatBlock != NULL)
	{
		scope_finally = (SExcptTableTry*)Alloc(sizeof(SExcptTableTry));
		scope_finally->Begin = AsmLabel();
		scope_finally->End = AsmLabel();
		scope_finally->CatchFunc = NULL;
		ListAdd(((SExcptTable*)PackAsm->ExcptTables->Bottom->Data)->TryScopes, scope_finally);
		ListAdd(PackAsm->Asms, scope_finally->Begin);
	}
	if (ast->Catches->Len != 0)
	{
		scope_catch = (SExcptTableTry*)Alloc(sizeof(SExcptTableTry));
		scope_catch->Begin = AsmLabel();
		scope_catch->End = AsmLabel();
		scope_catch->CatchFunc = NULL;
		ListAdd(((SExcptTable*)PackAsm->ExcptTables->Bottom->Data)->TryScopes, scope_catch);
		ListAdd(PackAsm->Asms, scope_catch->Begin);
	}
	AssembleBlock(ast->StatBlock);
	if (scope_catch != NULL)
	{
		// 'catch'
		{
			SAsmLabel* lbl1 = AsmLabel();
			ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
			ListAdd(PackAsm->Asms, scope_catch->End);
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_AX), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False))));
			// The statements of 'catch'
			if (ast->Catches->Len == 1)
				AssembleBlock(((SAstStatCatch*)ast->Catches->Top->Data)->StatBlock);
			else
			{
				SAsmLabel** lbl2 = (SAsmLabel**)Alloc(sizeof(SAsmLabel*) * (size_t)(ast->Catches->Len - 1));
				SAsmLabel* lbl3 = AsmLabel();
				{
					SListNode* ptr = ast->Catches->Top;
					int idx = 0;
					while (ptr->Next != NULL)
					{
						SAstStatCatch* catch_ = (SAstStatCatch*)ptr->Data;
						SListNode* ptr2 = catch_->Conds->Top;
						lbl2[idx] = AsmLabel();
						while (ptr2 != NULL)
						{
							SAstExpr** exprs = (SAstExpr**)ptr2->Data;
							if (exprs[1] == NULL)
							{
								S64 n = *(S64*)((SAstExprValue*)exprs[0])->Value;
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_R8), ValImmS(8, n)));
								ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_AX), ValReg(8, Reg_R8)));
								ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl2[idx])->Addr, True))));
							}
							else
							{
								SAsmLabel* lbl4 = AsmLabel();
								S64 n1 = *(S64*)((SAstExprValue*)exprs[0])->Value;
								S64 n2 = *(S64*)((SAstExprValue*)exprs[1])->Value;
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_R8), ValImmS(8, n1)));
								ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_AX), ValReg(8, Reg_R8)));
								ListAdd(PackAsm->Asms, AsmJL(ValImm(4, RefValueAddr(((SAsm*)lbl4)->Addr, True))));
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_R8), ValImmS(8, n2)));
								ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_AX), ValReg(8, Reg_R8)));
								ListAdd(PackAsm->Asms, AsmJLE(ValImm(4, RefValueAddr(((SAsm*)lbl2[idx])->Addr, True))));
								ListAdd(PackAsm->Asms, lbl4);
							}
							ptr2 = ptr2->Next;
						}
						ptr = ptr->Next;
						idx++;
					}
				}
				AssembleBlock(((SAstStatCatch*)ast->Catches->Bottom->Data)->StatBlock);
				ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl3)->Addr, True))));
				{
					SListNode* ptr = ast->Catches->Top;
					int idx = 0;
					while (ptr->Next != NULL)
					{
						ListAdd(PackAsm->Asms, lbl2[idx]);
						AssembleBlock(((SAstStatCatch*)ptr->Data)->StatBlock);
						if (ptr->Next->Next != NULL)
							ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl3)->Addr, True))));
						ptr = ptr->Next;
						idx++;
					}
				}
				ListAdd(PackAsm->Asms, lbl3);
			}
			ListAdd(PackAsm->Asms, ((SAstStatBreakable*)ast)->BreakPoint);
			if (scope_finally != NULL)
				ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False)), ValImmU(8, 0x00)));
			ListAdd(PackAsm->Asms, lbl1);
		}
		// Make the exception function.
		{
			SAstFuncRaw* func = (SAstFuncRaw*)Alloc(sizeof(SAstFuncRaw));
			memset(func, 0, sizeof(SAst));
			((SAst*)func)->TypeId = AstTypeId_FuncRaw;
			((SAst*)func)->AnalyzedCache = (SAst*)func;
			((SAstFunc*)func)->AddrTop = NewAddr();
			((SAstFunc*)func)->AddrBottom = -1;
			((SAstFunc*)func)->PosRowBottom = -1;
			((SAstFunc*)func)->DllName = NULL;
			((SAstFunc*)func)->DllFuncName = NULL;
			((SAstFunc*)func)->FuncAttr = FuncAttr_None;
			((SAstFunc*)func)->Args = ListNew();
			((SAstFunc*)func)->Ret = NULL;
			((SAstFunc*)func)->Stats = ListNew();
			((SAstFunc*)func)->RetPoint = NULL;
			func->Asms = ListNew();
			func->ArgNum = 0;
			func->Header = NULL;
			{
				SAsmLabel* lbl1 = AsmLabel();
				SAsmLabel* lbl2 = AsmLabel();
				ListAdd(func->Asms, AsmMOV(ValReg(8, Reg_AX), ValMemS(8, ValReg(8, Reg_CX), NULL, 0x00)));
				ListAdd(func->Asms, AsmMOV(ValReg(4, Reg_AX), ValMemS(4, ValReg(8, Reg_AX), NULL, 0x00)));
				{
					SListNode* ptr = ast->Catches->Top;
					while (ptr != NULL)
					{
						SAstStatCatch* catch_ = (SAstStatCatch*)ptr->Data;
						SListNode* ptr2 = catch_->Conds->Top;
						while (ptr2 != NULL)
						{
							SAstExpr** exprs = (SAstExpr**)ptr2->Data;
							if (exprs[1] == NULL)
							{
								S64 n = *(S64*)((SAstExprValue*)exprs[0])->Value;
								ListAdd(func->Asms, AsmMOV(ValReg(8, Reg_R8), ValImmS(8, n)));
								ListAdd(func->Asms, AsmCMP(ValReg(8, Reg_AX), ValReg(8, Reg_R8)));
								ListAdd(func->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
							}
							else
							{
								SAsmLabel* lbl3 = AsmLabel();
								S64 n1 = *(S64*)((SAstExprValue*)exprs[0])->Value;
								S64 n2 = *(S64*)((SAstExprValue*)exprs[1])->Value;
								ListAdd(func->Asms, AsmMOV(ValReg(8, Reg_R8), ValImmS(8, n1)));
								ListAdd(func->Asms, AsmCMP(ValReg(8, Reg_AX), ValReg(8, Reg_R8)));
								ListAdd(func->Asms, AsmJL(ValImm(4, RefValueAddr(((SAsm*)lbl3)->Addr, True))));
								ListAdd(func->Asms, AsmMOV(ValReg(8, Reg_R8), ValImmS(8, n2)));
								ListAdd(func->Asms, AsmCMP(ValReg(8, Reg_AX), ValReg(8, Reg_R8)));
								ListAdd(func->Asms, AsmJLE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
								ListAdd(func->Asms, lbl3);
							}
							ptr2 = ptr2->Next;
						}
						ptr = ptr->Next;
					}
				}
				ListAdd(func->Asms, AsmXOR(ValReg(4, Reg_AX), ValReg(4, Reg_AX)));
				ListAdd(func->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
				ListAdd(func->Asms, lbl1);
				ListAdd(func->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_DX), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False)), ValReg(8, Reg_AX)));
				ListAdd(func->Asms, AsmMOV(ValReg(4, Reg_AX), ValImmU(4, 0x01)));
				ListAdd(func->Asms, lbl2);
			}
			scope_catch->CatchFunc = RefLocalFunc((SAstFunc*)func);
		}
	}
	else
		ListAdd(PackAsm->Asms, ((SAstStatBreakable*)ast)->BreakPoint);
	if (scope_finally != NULL)
	{
		// 'finally'
		ListAdd(PackAsm->Asms, AsmNOP());
		ListAdd(PackAsm->Asms, scope_finally->End);
		AssembleBlock(ast->FinallyStatBlock);
		// Throw the exception again.
		{
			SAsmLabel* lbl1 = AsmLabel();
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False))));
			ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_CX), ValImmU(8, 0x00)));
			ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
			ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_DX), ValReg(4, Reg_DX)));
			ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R8), ValReg(4, Reg_R8)));
			ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R9), ValReg(4, Reg_R9)));
			CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"RaiseException");
			ListAdd(PackAsm->Asms, lbl1);
		}
		// Make the exception function.
		{
			SAstFuncRaw* func = (SAstFuncRaw*)Alloc(sizeof(SAstFuncRaw));
			memset(func, 0, sizeof(SAst));
			((SAst*)func)->TypeId = AstTypeId_FuncRaw;
			((SAst*)func)->AnalyzedCache = (SAst*)func;
			((SAstFunc*)func)->AddrTop = NewAddr();
			((SAstFunc*)func)->AddrBottom = -1;
			((SAstFunc*)func)->PosRowBottom = -1;
			((SAstFunc*)func)->DllName = NULL;
			((SAstFunc*)func)->DllFuncName = NULL;
			((SAstFunc*)func)->FuncAttr = FuncAttr_None;
			((SAstFunc*)func)->Args = ListNew();
			((SAstFunc*)func)->Ret = NULL;
			((SAstFunc*)func)->Stats = ListNew();
			((SAstFunc*)func)->RetPoint = NULL;
			func->Asms = ListNew();
			func->ArgNum = 0;
			func->Header = NULL;
			ListAdd(func->Asms, AsmMOV(ValReg(8, Reg_AX), ValMemS(8, ValReg(8, Reg_CX), NULL, 0x00)));
			ListAdd(func->Asms, AsmMOV(ValReg(4, Reg_AX), ValMemS(4, ValReg(8, Reg_AX), NULL, 0x00)));
			ListAdd(func->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_DX), NULL, RefValueAddr(RefLocalVar(((SAstStatBreakable*)ast)->BlockVar), False)), ValReg(8, Reg_AX)));
			ListAdd(func->Asms, AsmMOV(ValReg(4, Reg_AX), ValImmU(4, 0x01)));
			scope_finally->CatchFunc = RefLocalFunc((SAstFunc*)func);
		}
	}
}

static void AssembleThrow(SAstStatThrow* ast)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	AssembleExpr(ast->Code, 0, 0);
	ToValue(ast->Code, 0, 0);
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, Reg_CX), ValReg(4, RegI[0])));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_DX), ValReg(4, Reg_DX)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R8), ValReg(4, Reg_R8)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R9), ValReg(4, Reg_R9)));
	CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"RaiseException");
}

static void AssembleBlock(SAstStatBlock* ast)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	SListNode* asms_bottom = PackAsm->Asms->Bottom;
	AssembleStats(ast->Stats);
	SAsmLabel* break_point = ((SAstStatBreakable*)ast)->BreakPoint;
	if (break_point != NULL)
		ListAdd(PackAsm->Asms, break_point);
	SetStatAsm((SAstStat*)ast, asms_bottom, PackAsm->Asms->Bottom);
}

static void AssembleRet(SAstStatRet* ast)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	if (ast->Value != NULL)
	{
		AssembleExpr(ast->Value, 0, 0);
		ToValue(ast->Value, 0, 0);
	}
	ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)((SAstFunc*)Funcs->Top->Data)->RetPoint)->Addr, True))));
}

static void AssembleDo(SAstStatDo* ast)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	AssembleExpr(ast->Expr, 0, 0);
}

static void AssembleBreak(SAstStat* ast)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)((SAstStatBreakable*)((SAst*)ast)->RefItem)->BreakPoint)->Addr, True))));
}

static void AssembleSkip(SAstStat* ast)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)((SAstStatSkipable*)((SAst*)ast)->RefItem)->SkipPoint)->Addr, True))));
}

static void AssembleAssert(SAstStatAssert* ast)
{
	SAsmLabel* lbl1 = AsmLabel();
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(!Option->Rls);
	AssembleExpr(ast->Cond, 0, 0);
	ToValue(ast->Cond, 0, 0);
	ListAdd(PackAsm->Asms, AsmCMP(ValReg(1, RegI[0]), ValImmU(1, 0x00)));
	ListAdd(PackAsm->Asms, AsmJNE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, Reg_CX), ValImmU(4, EXCPT_DBG_ASSERT_FAILED)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_DX), ValReg(4, Reg_DX)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R8), ValReg(4, Reg_R8)));
	ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_R9), ValReg(4, Reg_R9)));
	CallAPI(PackAsm->Asms, L"KERNEL32.dll", L"RaiseException");
	ListAdd(PackAsm->Asms, lbl1);
}

static void AssembleExpr(SAstExpr* ast, int reg_i, int reg_f)
{
	switch (((SAst*)ast)->TypeId)
	{
		case AstTypeId_Expr1: AssembleExpr1((SAstExpr1*)ast, reg_i, reg_f); break;
		case AstTypeId_Expr2: AssembleExpr2((SAstExpr2*)ast, reg_i, reg_f); break;
		case AstTypeId_Expr3: AssembleExpr3((SAstExpr3*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprNew: AssembleExprNew((SAstExprNew*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprNewArray: AssembleExprNewArray((SAstExprNewArray*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprAs: AssembleExprAs((SAstExprAs*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprToBin: AssembleExprToBin((SAstExprToBin*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprFromBin: AssembleExprFromBin((SAstExprFromBin*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprCall: AssembleExprCall((SAstExprCall*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprArray: AssembleExprArray((SAstExprArray*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprDot: AssembleExprDot((SAstExprDot*)ast, reg_i, reg_f, True); break;
		case AstTypeId_ExprValue: AssembleExprValue((SAstExprValue*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprValueArray: AssembleExprValueArray((SAstExprValueArray*)ast, reg_i, reg_f); break;
		case AstTypeId_ExprRef: AssembleExprRef((SAstExpr*)ast, reg_i, reg_f); break;
		default:
			ASSERT(False);
			break;
	}
}

static void AssembleExpr1(SAstExpr1* ast, int reg_i, int reg_f)
{
	SAstType* type = ast->Child->Type;
	int size = GetSize(type);
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	AssembleExpr(ast->Child, reg_i, reg_f);
	ToValue(ast->Child, reg_i, reg_f);
	switch (ast->Kind)
	{
		case AstExpr1Kind_Plus:
			// Do nothing.
			break;
		case AstExpr1Kind_Minus:
			if (IsFloat(type))
			{
				S64* addr = AddReadonlyData(sizeof(FloatNeg), FloatNeg, True);
				ListAdd(PackAsm->Asms, AsmXORPD(ValReg(4, RegF[reg_f]), ValRIP(4, RefValueAddr(addr, True))));
			}
			else
			{
				ListAdd(PackAsm->Asms, AsmNEG(ValReg(size, RegI[reg_i])));
				if (IsInt(type))
					ChkOverflow();
			}
			break;
		case AstExpr1Kind_Not:
			ASSERT(size == 1);
			ListAdd(PackAsm->Asms, AsmXOR(ValReg(1, RegI[reg_i]), ValImmU(1, 0x01)));
			break;
		case AstExpr1Kind_Copy:
			{
				SAsmLabel* lbl1 = AsmLabel();
				ASSERT(size == 8);
				ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, RegI[reg_i]), ValImmU(8, 0x00)));
				ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
				{
					STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
					ASSERT(((SAst*)type)->TypeId == AstTypeId_TypeArray || ((SAst*)type)->TypeId == AstTypeId_TypeGen || ((SAst*)type)->TypeId == AstTypeId_TypeDict || IsClass(type));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValReg(8, RegI[reg_i])));
					SetTypeId(Reg_DX, type);
					CallKuinLib(L"_copy");
					SetGcInstance(0, -1, type);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
					PopRegs(tmp);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
					ListAdd(PackAsm->Asms, lbl1);
				}
			}
			break;
		case AstExpr1Kind_Len:
			if (IsInt(type))
			{
				SAsmLabel* lbl1 = AsmLabel();
				ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, RegI[reg_i]), ValImmU(8, 0x00)));
				ListAdd(PackAsm->Asms, AsmJGE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
				ListAdd(PackAsm->Asms, AsmNEG(ValReg(8, RegI[reg_i])));
				ListAdd(PackAsm->Asms, lbl1);
			}
			else if (IsFloat(type))
			{
				SAsmLabel* lbl1 = AsmLabel();
				ListAdd(PackAsm->Asms, AsmXORPD(ValReg(4, Reg_XMM14), ValReg(4, Reg_XMM14)));
				ListAdd(PackAsm->Asms, AsmCOMISD(ValReg(4, RegF[reg_f]), ValReg(4, Reg_XMM14)));
				ListAdd(PackAsm->Asms, AsmJAE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
				{
					S64* addr = AddReadonlyData(sizeof(FloatNeg), FloatNeg, True);
					ListAdd(PackAsm->Asms, AsmXORPD(ValReg(4, RegF[reg_f]), ValRIP(4, RefValueAddr(addr, True))));
				}
				ListAdd(PackAsm->Asms, lbl1);
			}
			else
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x08)));
			break;
		default:
			ASSERT(False);
			break;
	}
}

static void AssembleExpr2(SAstExpr2* ast, int reg_i, int reg_f)
{
	SAstArg* tmp_i = NULL;
	SAstArg* tmp_f = NULL;
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	if (ast->Kind == AstExpr2Kind_Or || ast->Kind == AstExpr2Kind_And)
	{
		SAsmLabel* lbl1 = AsmLabel();
		AssembleExpr(ast->Children[0], reg_i, reg_f);
		ToValue(ast->Children[0], reg_i, reg_f);
		ListAdd(PackAsm->Asms, AsmCMP(ValReg(1, RegI[reg_i]), ValImmU(1, 0x00)));
		if (ast->Kind == AstExpr2Kind_Or)
			ListAdd(PackAsm->Asms, AsmJNE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
		else
			ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
		AssembleExpr(ast->Children[1], reg_i, reg_f);
		ToValue(ast->Children[1], reg_i, reg_f);
		ListAdd(PackAsm->Asms, lbl1);
		return;
	}
	{
		int use_reg; // This indicates how to use registers.
		if (ast->Kind == AstExpr2Kind_Assign)
		{
			if (IsFloat(ast->Children[0]->Type))
				use_reg = 0; // Use 'reg_i' and 'reg_f'.
			else
				use_reg = 1; // Use 'reg_i' and 'reg_i + 1'.
		}
		else if (ast->Kind == AstExpr2Kind_Swap)
			use_reg = 1; // Use 'reg_i' and 'reg_i + 1'.
		else
		{
			if (IsFloat(ast->Children[0]->Type))
				use_reg = 2; // Use 'reg_f' and 'reg_f + 1'.
			else
				use_reg = 1; // Use 'reg_i' and 'reg_i + 1'.
		}
		// Save registers.
		{
			if (use_reg == 0 || use_reg == 1)
			{
				if (reg_i + 1 == REG_I_NUM)
				{
					tmp_i = MakeTmpVar(8, NULL);
					ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_i), False)), ValReg(8, RegI[reg_i - 1])));
					reg_i--;
				}
				else
					ASSERT(reg_i + 1 < REG_I_NUM);
			}
			else
			{
				ASSERT(use_reg == 2);
				if (reg_f + 1 == REG_F_NUM)
				{
					tmp_f = MakeTmpVar(8, NULL);
					ListAdd(PackAsm->Asms, AsmMOVSD(ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_f), False)), ValReg(4, RegF[reg_f - 1])));
					reg_f--;
				}
				else
					ASSERT(reg_f + 1 < REG_F_NUM);
			}
		}
		AssembleExpr(ast->Children[0], reg_i, reg_f);
		if (ast->Kind != AstExpr2Kind_Assign && ast->Kind != AstExpr2Kind_Swap)
			ToValue(ast->Children[0], reg_i, reg_f);
		if (use_reg == 0)
		{
			AssembleExpr(ast->Children[1], reg_i + 1, reg_f); // This value is stored in 'reg_f'.
			ToValue(ast->Children[1], reg_i + 1, reg_f);
			ASSERT(ast->Kind != AstExpr2Kind_Swap);
		}
		else if (use_reg == 1)
		{
			AssembleExpr(ast->Children[1], reg_i + 1, reg_f); // This value is stored in 'reg_i + 1'.
			if (ast->Kind != AstExpr2Kind_Swap)
				ToValue(ast->Children[1], reg_i + 1, reg_f);
		}
		else
		{
			ASSERT(use_reg == 2);
			AssembleExpr(ast->Children[1], reg_i, reg_f + 1); // This value is stored in 'reg_f + 1'.
			ToValue(ast->Children[1], reg_i, reg_f + 1);
			ASSERT(ast->Kind != AstExpr2Kind_Swap);
		}
	}
	{
		SAstType* type = ast->Children[0]->Type;
		int size = GetSize(type);
		switch (ast->Kind)
		{
			case AstExpr2Kind_Assign:
				if (IsFloat(type))
				{
					switch (ast->Children[0]->VarKind)
					{
						case AstExprVarKind_LocalVar:
							ListAdd(PackAsm->Asms, AsmMOVSD(ValMemS(4, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00), ValReg(4, RegF[reg_f])));
							break;
						case AstExprVarKind_GlobalVar:
							ListAdd(PackAsm->Asms, AsmMOVSD(ValMemS(4, ValReg(8, RegI[reg_i]), NULL, 0x00), ValReg(4, RegF[reg_f])));
							break;
						case AstExprVarKind_RefVar:
							ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00)));
							ListAdd(PackAsm->Asms, AsmMOVSD(ValMemS(4, ValReg(8, RegI[reg_i]), NULL, 0x00), ValReg(4, RegF[reg_f])));
							break;
						default:
							ASSERT(False);
							break;
					}
				}
				else
				{
					if (IsRef(type))
					{
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, RegI[reg_i])));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DI), ValReg(8, RegI[reg_i + 1])));
						GcInc(reg_i + 1);
						switch (ast->Children[0]->VarKind)
						{
							case AstExprVarKind_LocalVar:
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00)));
								break;
							case AstExprVarKind_GlobalVar:
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00)));
								break;
							case AstExprVarKind_RefVar:
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00)));
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00)));
								break;
							default:
								ASSERT(False);
								break;
						}
						GcDec(reg_i, reg_f - 1, type);
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i + 1]), ValReg(8, Reg_DI)));
					}
					switch (ast->Children[0]->VarKind)
					{
						case AstExprVarKind_LocalVar:
							ListAdd(PackAsm->Asms, AsmMOV(ValMemS(size, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00), ValReg(size, RegI[reg_i + 1])));
							break;
						case AstExprVarKind_GlobalVar:
							ListAdd(PackAsm->Asms, AsmMOV(ValMemS(size, ValReg(8, RegI[reg_i]), NULL, 0x00), ValReg(size, RegI[reg_i + 1])));
							break;
						case AstExprVarKind_RefVar:
							ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, Reg_SP), ValReg(1, RegI[reg_i]), 0x00)));
							ListAdd(PackAsm->Asms, AsmMOV(ValMemS(size, ValReg(8, RegI[reg_i]), NULL, 0x00), ValReg(size, RegI[reg_i + 1])));
							break;
						default:
							ASSERT(False);
							break;
					}
				}
				break;
			case AstExpr2Kind_LT:
			case AstExpr2Kind_GT:
			case AstExpr2Kind_LE:
			case AstExpr2Kind_GE:
			case AstExpr2Kind_Eq:
			case AstExpr2Kind_NEq:
			case AstExpr2Kind_EqRef:
			case AstExpr2Kind_NEqRef:
				{
					SAsmLabel* lbl1 = AsmLabel();
					SAsmLabel* lbl2 = AsmLabel();
					{
						Bool sign;
						EAstExpr2Kind kind = ast->Kind;
						if (IsInt(type) || IsEnum(type))
						{
							ListAdd(PackAsm->Asms, AsmCMP(ValReg(size, RegI[reg_i]), ValReg(size, RegI[reg_i + 1])));
							sign = True;
						}
						else if (((SAst*)type)->TypeId == AstTypeId_TypeBit || IsChar(type) || IsBool(type))
						{
							ListAdd(PackAsm->Asms, AsmCMP(ValReg(size, RegI[reg_i]), ValReg(size, RegI[reg_i + 1])));
							sign = False;
						}
						else if (IsFloat(type))
						{
							ListAdd(PackAsm->Asms, AsmCOMISD(ValReg(4, RegF[reg_f]), ValReg(4, RegF[reg_f + 1])));
							sign = False;
						}
						else
						{
							ASSERT(IsNullable(type) || ((SAst*)type)->TypeId == AstTypeId_TypeNull);
							if (kind == AstExpr2Kind_EqRef || kind == AstExpr2Kind_NEqRef)
							{
								ListAdd(PackAsm->Asms, AsmCMP(ValReg(size, RegI[reg_i]), ValReg(size, RegI[reg_i + 1])));
								sign = False;
							}
							else if (IsStr(type))
							{
								STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, RegI[reg_i + 1])));
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValReg(8, RegI[reg_i])));
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DX), ValReg(8, Reg_SI)));
								CallKuinLib(L"_cmpStr");
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
								PopRegs(tmp);
								ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_SI), ValImmU(8, 0x00)));
								sign = True;
							}
							else
							{
								STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
								ASSERT(IsClass(type));
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, RegI[reg_i + 1])));
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValReg(8, RegI[reg_i])));
								GcInc(1);
								ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, 0x00), ValReg(8, Reg_CX)));
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DX), ValReg(8, Reg_SI)));
								GcInc(2);
								ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, 0x08), ValReg(8, Reg_DX)));
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValMemS(8, ValReg(8, Reg_CX), NULL, 0x08)));
								ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_SI), ValMemS(8, ValReg(8, Reg_SI), NULL, 0x18)));
								ListAdd(PackAsm->Asms, AsmADD(ValReg(8, Reg_SI), ValMemS(8, ValReg(8, Reg_SI), NULL, 0x00)));
								ListAdd(PackAsm->Asms, AsmCALL(ValReg(8, Reg_SI)));
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
								PopRegs(tmp);
								ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_SI), ValImmU(8, 0x00)));
								sign = True;
							}
						}
						if (sign)
						{
							switch (kind)
							{
								case AstExpr2Kind_Eq: ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_NEq: ListAdd(PackAsm->Asms, AsmJNE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_LT: ListAdd(PackAsm->Asms, AsmJL(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_GT: ListAdd(PackAsm->Asms, AsmJG(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_LE: ListAdd(PackAsm->Asms, AsmJLE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_GE: ListAdd(PackAsm->Asms, AsmJGE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								default:
									ASSERT(False);
									break;
							}
						}
						else
						{
							switch (kind)
							{
								case AstExpr2Kind_Eq: ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_NEq: ListAdd(PackAsm->Asms, AsmJNE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_LT: ListAdd(PackAsm->Asms, AsmJB(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_GT: ListAdd(PackAsm->Asms, AsmJA(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_LE: ListAdd(PackAsm->Asms, AsmJBE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_GE: ListAdd(PackAsm->Asms, AsmJAE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_EqRef: ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								case AstExpr2Kind_NEqRef: ListAdd(PackAsm->Asms, AsmJNE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True)))); break;
								default:
									ASSERT(False);
									break;
							}
						}
					}
					ListAdd(PackAsm->Asms, AsmXOR(ValReg(1, RegI[reg_i]), ValReg(1, RegI[reg_i])));
					ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
					ListAdd(PackAsm->Asms, lbl1);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(1, RegI[reg_i]), ValImmU(1, 0x01)));
					ListAdd(PackAsm->Asms, lbl2);
				}
				break;
			case AstExpr2Kind_Cat:
				{
					SAstType* type2 = ((SAstTypeArray*)type)->ItemType;
					int size2 = GetSize(type2);
					STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, RegI[reg_i])));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DI), ValReg(8, RegI[reg_i + 1])));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_AX), ValMemS(8, ValReg(8, Reg_SI), NULL, 0x08))); // Throw an exception if the first value is null.
					ListAdd(PackAsm->Asms, AsmADD(ValReg(8, Reg_AX), ValMemS(8, ValReg(8, Reg_DI), NULL, 0x08))); // Throw an exception if the second value is null.
					if (IsChar(type2))
						ListAdd(PackAsm->Asms, AsmINC(ValReg(8, Reg_AX)));
					ListAdd(PackAsm->Asms, AsmIMUL(ValReg(8, Reg_AX), ValReg(8, Reg_AX), ValImmU(8, size2)));
					ListAdd(PackAsm->Asms, AsmADD(ValReg(8, Reg_AX), ValImmU(8, 0x10)));
					AllocHeap(ValReg(8, Reg_AX));
					ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_AX), NULL, 0x00), ValImmU(8, 0x01)));
					SetGcInstance(0, -1, type);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_R8), ValReg(8, Reg_SI)));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_R9), ValReg(8, Reg_DI)));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValMemS(8, ValReg(8, Reg_R8), NULL, 0x08)));
					ListAdd(PackAsm->Asms, AsmADD(ValReg(8, Reg_CX), ValMemS(8, ValReg(8, Reg_R9), NULL, 0x08)));
					ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_AX), NULL, 0x08), ValReg(8, Reg_CX)));
					ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_DI), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x10)));
					ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_SI), ValMemS(8, ValReg(8, Reg_R8), NULL, 0x10)));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValMemS(8, ValReg(8, Reg_R8), NULL, 0x08)));
					ListAdd(PackAsm->Asms, AsmREPMOVS(ValReg(size2, Reg_AX)));
					ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_SI), ValMemS(8, ValReg(8, Reg_R9), NULL, 0x10)));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValMemS(8, ValReg(8, Reg_R9), NULL, 0x08)));
					if (IsChar(type2))
						ListAdd(PackAsm->Asms, AsmINC(ValReg(8, Reg_CX)));
					ListAdd(PackAsm->Asms, AsmREPMOVS(ValReg(size2, Reg_AX)));
					if (IsRef(type2))
					{
						SAsmLabel* lbl1 = AsmLabel();
						SAsmLabel* lbl2 = AsmLabel();
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x08)));
						ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_DI), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x10)));
						ListAdd(PackAsm->Asms, lbl1);
						ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_SI), ValImmU(8, 0x00)));
						ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[1]), ValMemS(8, ValReg(8, Reg_DI), NULL, 0x00)));
						GcInc(1);
						ListAdd(PackAsm->Asms, AsmDEC(ValReg(8, Reg_SI)));
						ListAdd(PackAsm->Asms, AsmADD(ValReg(8, Reg_DI), ValImmU(8, size2)));
						ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
						ListAdd(PackAsm->Asms, lbl2);
					}
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
					PopRegs(tmp);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
				}
				break;
			case AstExpr2Kind_Add:
				if (IsFloat(type))
					ListAdd(PackAsm->Asms, AsmADDSD(ValReg(4, RegF[reg_f]), ValReg(4, RegF[reg_f + 1])));
				else
				{
					ListAdd(PackAsm->Asms, AsmADD(ValReg(size, RegI[reg_i]), ValReg(size, RegI[reg_i + 1])));
					if (IsInt(type))
						ChkOverflow();
				}
				break;
			case AstExpr2Kind_Sub:
				if (IsFloat(type))
					ListAdd(PackAsm->Asms, AsmSUBSD(ValReg(4, RegF[reg_f]), ValReg(4, RegF[reg_f + 1])));
				else
				{
					ListAdd(PackAsm->Asms, AsmSUB(ValReg(size, RegI[reg_i]), ValReg(size, RegI[reg_i + 1])));
					if (IsInt(type))
						ChkOverflow();
				}
				break;
			case AstExpr2Kind_Mul:
				if (IsFloat(type))
					ListAdd(PackAsm->Asms, AsmMULSD(ValReg(4, RegF[reg_f]), ValReg(4, RegF[reg_f + 1])));
				else
				{
					// TODO: Optimize using 'ADD' and 'SHL'.
					if (size <= 2)
					{
						ASSERT(((SAst*)type)->TypeId == AstTypeId_TypeBit);
						ListAdd(PackAsm->Asms, AsmMOVZX(ValReg(4, RegI[reg_i]), ValReg(size, RegI[reg_i])));
						ListAdd(PackAsm->Asms, AsmMOVZX(ValReg(4, RegI[reg_i + 1]), ValReg(size, RegI[reg_i + 1])));
						ListAdd(PackAsm->Asms, AsmIMUL(ValReg(4, RegI[reg_i]), ValReg(4, RegI[reg_i + 1]), NULL));
					}
					else
						ListAdd(PackAsm->Asms, AsmIMUL(ValReg(size, RegI[reg_i]), ValReg(size, RegI[reg_i + 1]), NULL));
					if (IsInt(type))
						ChkOverflow();
				}
				break;
			case AstExpr2Kind_Div:
			case AstExpr2Kind_Mod:
				if (IsFloat(type))
				{
					if (ast->Kind == AstExpr2Kind_Div)
						ListAdd(PackAsm->Asms, AsmDIVSD(ValReg(4, RegF[reg_f]), ValReg(4, RegF[reg_f + 1])));
					else
					{
						STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
						ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, Reg_XMM0), ValReg(4, RegF[reg_f])));
						ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, Reg_XMM1), ValReg(4, RegF[reg_f + 1])));
						CallKuinLib(L"_mod");
						ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, Reg_XMM14), ValReg(4, Reg_XMM0)));
						PopRegs(tmp);
						ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValReg(4, Reg_XMM14)));
					}
				}
				else
				{
					// TODO: Optimize using 'SHR'.
					STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
					if (size <= 2)
					{
						ASSERT(((SAst*)type)->TypeId == AstTypeId_TypeBit);
						ListAdd(PackAsm->Asms, AsmMOVZX(ValReg(4, Reg_AX), ValReg(size, RegI[reg_i])));
						ListAdd(PackAsm->Asms, AsmMOVZX(ValReg(4, Reg_CX), ValReg(size, RegI[reg_i + 1])));
						ListAdd(PackAsm->Asms, AsmCDQ(ValReg(4, Reg_AX)));
						ListAdd(PackAsm->Asms, AsmIDIV(ValReg(4, Reg_CX)));
					}
					else
					{
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, Reg_AX), ValReg(size, RegI[reg_i])));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, Reg_CX), ValReg(size, RegI[reg_i + 1])));
						if (((SAst*)type)->TypeId == AstTypeId_TypeBit)
						{
							ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_DX), ValReg(4, Reg_DX)));
							ListAdd(PackAsm->Asms, AsmDIV(ValReg(size, Reg_CX)));
						}
						else
						{
							ASSERT(IsInt(type));
							ListAdd(PackAsm->Asms, AsmCDQ(ValReg(size, Reg_AX)));
							ListAdd(PackAsm->Asms, AsmIDIV(ValReg(size, Reg_CX)));
						}
					}
					if (ast->Kind == AstExpr2Kind_Div)
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, Reg_SI), ValReg(size, Reg_AX)));
					else
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, Reg_SI), ValReg(size, Reg_DX)));
					PopRegs(tmp);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i]), ValReg(size, Reg_SI)));
				}
				break;
			case AstExpr2Kind_Pow:
				if (IsInt(type))
				{
					STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, RegI[reg_i + 1])));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValReg(8, RegI[reg_i])));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DX), ValReg(8, Reg_SI)));
					CallKuinLib(L"_powInt");
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
					PopRegs(tmp);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
				}
				else
				{
					STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
					ASSERT(IsFloat(type));
					ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, Reg_XMM0), ValReg(4, RegF[reg_f])));
					ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, Reg_XMM1), ValReg(4, RegF[reg_f + 1])));
					CallKuinLib(L"_powFloat");
					ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, Reg_XMM14), ValReg(4, Reg_XMM0)));
					PopRegs(tmp);
					ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValReg(4, Reg_XMM14)));
				}
				break;
			case AstExpr2Kind_Swap:
				ToRef(ast->Children[0], Reg_SI, RegI[reg_i]);
				ToRef(ast->Children[1], Reg_DI, RegI[reg_i + 1]);
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i]), ValMemS(size, ValReg(8, Reg_SI), NULL, 0x00)));
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i + 1]), ValMemS(size, ValReg(8, Reg_DI), NULL, 0x00)));
				ListAdd(PackAsm->Asms, AsmMOV(ValMemS(size, ValReg(8, Reg_SI), NULL, 0x00), ValReg(size, RegI[reg_i + 1])));
				ListAdd(PackAsm->Asms, AsmMOV(ValMemS(size, ValReg(8, Reg_DI), NULL, 0x00), ValReg(size, RegI[reg_i])));
				break;
			default:
				ASSERT(False);
				break;
		}
	}
	{
		// Restore the registers.
		if (tmp_i != NULL)
		{
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i + 1]), ValReg(8, RegI[reg_i])));
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_i), False))));
		}
		if (tmp_f != NULL)
		{
			ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f + 1]), ValReg(4, RegF[reg_f])));
			ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_f), False))));
		}
	}
}

static void AssembleExpr3(SAstExpr3* ast, int reg_i, int reg_f)
{
	SAsmLabel* lbl1 = AsmLabel();
	SAsmLabel* lbl2 = AsmLabel();
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	AssembleExpr(ast->Children[0], reg_i, reg_f);
	ToValue(ast->Children[0], reg_i, reg_f);
	ListAdd(PackAsm->Asms, AsmCMP(ValReg(1, RegI[reg_i]), ValImmU(1, 0x00)));
	ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
	AssembleExpr(ast->Children[1], reg_i, reg_f);
	ToValue(ast->Children[1], reg_i, reg_f);
	ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
	ListAdd(PackAsm->Asms, lbl1);
	AssembleExpr(ast->Children[2], reg_i, reg_f);
	ToValue(ast->Children[2], reg_i, reg_f);
	ListAdd(PackAsm->Asms, lbl2);
}

static void AssembleExprNew(SAstExprNew* ast, int reg_i, int reg_f)
{
	STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	if (IsClass(ast->ItemType))
	{
		SAstClass* class_ = (SAstClass*)((SAst*)ast->ItemType)->RefItem;
		S64* addr = RefClass(class_);
		AllocHeap(ValImmU(8, 0x10 + (U64)class_->VarSize));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_AX), NULL, 0x00), ValImmU(8, 0x02))); // Set the reference counter to 2 for 'GcInstance' and constructor call.
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_SI), ValRIP(8, RefValueAddr(addr, True))));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_AX), NULL, 0x08), ValReg(8, Reg_SI)));
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_CX), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x10)));
		ClearMem(Reg_CX, (U64)class_->VarSize, Reg_SI);
		SetGcInstance(0, -1, ast->ItemType);
		{
			SAstArg* tmp2 = MakeTmpVar(8, NULL);
			ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp2), False)), ValReg(8, Reg_AX)));
			ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, 0x00), ValReg(8, Reg_AX)));
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_AX), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x08)));
			ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_AX), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x08)));
			ListAdd(PackAsm->Asms, AsmADD(ValReg(8, Reg_AX), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x00)));
			ListAdd(PackAsm->Asms, AsmCALL(ValReg(8, Reg_AX))); // Call 'ctor'.
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp2), False))));
		}
	}
	else
	{
		int size;
		if (((SAst*)ast->ItemType)->TypeId == AstTypeId_TypeGen)
		{
			switch (((SAstTypeGen*)ast->ItemType)->Kind)
			{
				case AstTypeGenKind_List: size = 0x28; break;
				case AstTypeGenKind_Stack: size = 0x18; break;
				case AstTypeGenKind_Queue: size = 0x20; break;
				default:
					ASSERT(False);
					size = 0;
					break;
			}
		}
		else
		{
			ASSERT(((SAst*)ast->ItemType)->TypeId == AstTypeId_TypeDict);
			size = 0x18;
		}
		AllocHeap(ValImmU(8, (U64)size));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_AX), NULL, 0x00), ValImmU(8, 0x01)));
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_CX), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x08)));
		ClearMem(Reg_CX, (U64)(size - 0x08), Reg_SI);
		SetGcInstance(0, -1, ast->ItemType);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
	}
	PopRegs(tmp);
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
}

static void AssembleExprNewArray(SAstExprNewArray* ast, int reg_i, int reg_f)
{
	STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	{
		int len = ast->Idces->Len;
		SAstArg** idx_nums = (SAstArg**)Alloc(sizeof(SAstArg*) * (size_t)len);
		int i;
		// Make consecutive stack variables to store the numbers of elements of the arrays.
		for (i = 0; i < len; i++)
			idx_nums[i] = MakeTmpVar(8, NULL);
		{
			SListNode* ptr = ast->Idces->Top;
			i = 0;
			while (ptr != NULL)
			{
				SAstExpr* expr = (SAstExpr*)ptr->Data;
				AssembleExpr(expr, 0, 0);
				ToValue(expr, 0, 0);
				ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(idx_nums[i]), False)), ValReg(8, RegI[0])));
				ptr = ptr->Next;
				i++;
			}
		}
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValImmU(8, (U64)len)));
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_DX), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(idx_nums[0]), False))));
	}
	SetTypeId(Reg_R8, ast->ItemType);
	CallKuinLib(L"_newArray");
	SetGcInstance(0, -1, ((SAstExpr*)ast)->Type);
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
	PopRegs(tmp);
	ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
}

static void AssembleExprAs(SAstExprAs* ast, int reg_i, int reg_f)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	switch (ast->Kind)
	{
		case AstExprAsKind_As:
			{
				SAstType* t1 = ast->Child->Type;
				SAstType* t2 = ast->ChildType;
				AssembleExpr(ast->Child, reg_i, reg_f);
				ToValue(ast->Child, reg_i, reg_f);
				if (((SAst*)t1)->TypeId == AstTypeId_TypeBit || IsInt(t1) || IsChar(t1) || IsBool(t1) || IsEnum(t1))
				{
					if (IsFloat(t2))
					{
						if (((SAst*)t1)->TypeId == AstTypeId_TypeBit)
						{
							int size = GetSize(t1);
							switch (size)
							{
								case 1:
								case 2:
									ListAdd(PackAsm->Asms, AsmMOVZX(ValReg(4, RegI[reg_i]), ValReg(size, RegI[reg_i])));
									ListAdd(PackAsm->Asms, AsmCVTSI2SD(ValReg(4, RegF[reg_f]), ValReg(4, RegI[reg_i])));
									break;
								case 4:
									ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, RegI[reg_i]), ValReg(4, RegI[reg_i])));
									ListAdd(PackAsm->Asms, AsmCVTSI2SD(ValReg(8, RegF[reg_f]), ValReg(8, RegI[reg_i])));
									break;
								case 8:
									ListAdd(PackAsm->Asms, AsmCVTSI2SD(ValReg(8, RegF[reg_f]), ValReg(8, RegI[reg_i])));
									{
										// Since CVTSI2SD is a signed operation, the compiler needs to correct it.
										SAsmLabel* lbl1 = AsmLabel();
										ListAdd(PackAsm->Asms, AsmTEST(ValReg(8, RegI[reg_i]), ValReg(8, RegI[reg_i])));
										ListAdd(PackAsm->Asms, AsmJNS(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
										{
											S64* addr = AddReadonlyData(sizeof(Real43f0000000000000), Real43f0000000000000, False);
											ListAdd(PackAsm->Asms, AsmADDSD(ValReg(4, RegF[reg_f]), ValRIP(4, RefValueAddr(addr, True))));
										}
										ListAdd(PackAsm->Asms, lbl1);
									}
									break;
								default:
									ASSERT(False);
									break;
							}
						}
						else
						{
							ASSERT(IsInt(t1));
							ListAdd(PackAsm->Asms, AsmCVTSI2SD(ValReg(8, RegF[reg_f]), ValReg(8, RegI[reg_i])));
						}
					}
					else if (IsBool(t2))
					{
						SAsmLabel* lbl1 = AsmLabel();
						ListAdd(PackAsm->Asms, AsmCMP(ValReg(GetSize(t1), RegI[reg_i]), ValImmU(8, 0x00)));
						ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(1, RegI[reg_i]), ValImmU(1, 0x01)));
						ListAdd(PackAsm->Asms, lbl1);
					}
					else
					{
						int size1 = GetSize(t1);
						int size2 = GetSize(t2);
						// Leave the register value as it is when its size is equal to or smaller than that after the casting.
						if (size1 < size2)
						{
							// Note: 'MOVSX' should be used for casting from signed values that are not 8 bytes.
							if (size1 == 4 && size2 == 8)
								ListAdd(PackAsm->Asms, AsmMOV(ValReg(4, RegI[reg_i]), ValReg(4, RegI[reg_i])));
							else
								ListAdd(PackAsm->Asms, AsmMOVZX(ValReg(size2, RegI[reg_i]), ValReg(size1, RegI[reg_i])));
						}
					}
				}
				else if (IsFloat(t1))
				{
					if (IsFloat(t2))
					{
						// Do nothing.
					}
					else
					{
						ASSERT(((SAst*)t2)->TypeId == AstTypeId_TypeBit || IsInt(t2));
						if (((SAst*)t2)->TypeId == AstTypeId_TypeBit && ((SAstTypeBit*)t2)->Size == 8)
						{
							SAsmLabel* lbl1 = AsmLabel();
							{
								S64* addr = AddReadonlyData(sizeof(Real43e0000000000000), Real43e0000000000000, False);
								ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, Reg_XMM14), ValRIP(4, RefValueAddr(addr, True))));
							}
							ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, Reg_SI), ValReg(4, Reg_SI)));
							ListAdd(PackAsm->Asms, AsmCOMISD(ValReg(4, RegF[reg_f]), ValReg(4, Reg_XMM14)));
							ListAdd(PackAsm->Asms, AsmJB(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
							ListAdd(PackAsm->Asms, AsmSUBSD(ValReg(4, RegF[reg_f]), ValReg(4, Reg_XMM14)));
							ListAdd(PackAsm->Asms, AsmCOMISD(ValReg(4, RegF[reg_f]), ValReg(4, Reg_XMM14)));
							ListAdd(PackAsm->Asms, AsmJAE(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
							ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValImmU(8, 0x0000000000000080)));
							ListAdd(PackAsm->Asms, lbl1);
							ListAdd(PackAsm->Asms, AsmCVTSD2SI(ValReg(8, RegI[reg_i]), ValReg(8, RegF[reg_f])));
							ListAdd(PackAsm->Asms, AsmADD(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
						}
						else
							ListAdd(PackAsm->Asms, AsmCVTSD2SI(ValReg(8, RegI[reg_i]), ValReg(8, RegF[reg_f])));
					}
				}
				else
				{
					SAsmLabel* lbl1 = AsmLabel();
					SAsmLabel* lbl2 = AsmLabel();
					SAsmLabel* lbl3 = AsmLabel();
					SAsmLabel* lbl4 = AsmLabel();
					ASSERT(IsClass(t1));
					ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, RegI[reg_i]), ValImmU(8, 0x00)));
					ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl4)->Addr, True))));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, RegI[reg_i])));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x08)));
					ListAdd(PackAsm->Asms, lbl1);
					{
						S64* addr = RefClass((SAstClass*)((SAst*)ast->ChildType)->RefItem);
						ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_DI), ValRIP(8, RefValueAddr(addr, True))));
					}
					ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, RegI[reg_i]), ValReg(8, Reg_DI)));
					ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl3)->Addr, True))));
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DI), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00)));
					ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_DI), ValImmU(8, 0x00)));
					ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
					ListAdd(PackAsm->Asms, AsmADD(ValReg(8, RegI[reg_i]), ValReg(8, Reg_DI)));
					ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
					ListAdd(PackAsm->Asms, lbl2);
					RaiseExcpt(EXCPT_CLASS_CAST_FAILED);
					ListAdd(PackAsm->Asms, lbl3);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
					ListAdd(PackAsm->Asms, lbl4);
				}
			}
			break;
		case AstExprAsKind_Is:
		case AstExprAsKind_NIs:
			{
				SAsmLabel* lbl1 = AsmLabel();
				SAsmLabel* lbl2 = AsmLabel();
				SAsmLabel* lbl3 = AsmLabel();
				SAsmLabel* lbl4 = AsmLabel();
				AssembleExpr(ast->Child, reg_i, reg_f);
				ToValue(ast->Child, reg_i, reg_f);
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x08)));
				{
					S64* addr = RefClass((SAstClass*)((SAst*)ast->ChildType)->RefItem);
					ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_SI), ValRIP(8, RefValueAddr(addr, True))));
				}
				ListAdd(PackAsm->Asms, lbl1);
				ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
				ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DI), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x00)));
				ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_DI), ValImmU(8, 0x00)));
				ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl3)->Addr, True))));
				ListAdd(PackAsm->Asms, AsmADD(ValReg(8, RegI[reg_i]), ValReg(8, Reg_DI)));
				ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
				ListAdd(PackAsm->Asms, lbl2);
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(1, RegI[reg_i]), ValImmU(1, ast->Kind == AstExprAsKind_Is ? 0x01 : 0x00)));
				ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl4)->Addr, True))));
				ListAdd(PackAsm->Asms, lbl3);
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(1, RegI[reg_i]), ValImmU(1, ast->Kind == AstExprAsKind_Is ? 0x00 : 0x01)));
				ListAdd(PackAsm->Asms, lbl4);
			}
			break;
		default:
			ASSERT(False);
			break;
	}
}

static void AssembleExprToBin(SAstExprToBin* ast, int reg_i, int reg_f)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	{
		STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
		AssembleExpr(ast->Child, 0, 0);
		ToValue(ast->Child, 0, 0);
		{
			int size = GetSize(ast->Child->Type);
			SAstArg* tmp_child = MakeTmpVar(size, NULL);
			if (IsFloat(ast->Child->Type))
				ListAdd(PackAsm->Asms, AsmMOVSD(ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_child), False)), ValReg(4, RegF[0])));
			else
				ListAdd(PackAsm->Asms, AsmMOV(ValMem(size, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_child), False)), ValReg(size, RegI[0])));
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_child), False))));
		}
		SetTypeId(Reg_DX, ast->Child->Type);
		CallKuinLib(L"_toBin");
		SetGcInstance(0, -1, ast->ChildType);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
		PopRegs(tmp);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
	}
}

static void AssembleExprFromBin(SAstExprFromBin* ast, int reg_i, int reg_f)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	{
		STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
		SAstArg* tmp_var = MakeTmpVar(8, NULL);
		{
			if (ast->Offset->VarKind == AstExprVarKind_Value)
			{
				SAstArg* tmp_offset = MakeTmpVar(8, NULL);
				ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_offset), False)), ValImmU(8, 0x00)));
				ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, RegI[0]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_offset), False))));
			}
			else
			{
				AssembleExpr(ast->Offset, 0, 0);
				ToRef(ast->Offset, RegI[0], RegI[0]);
			}
			ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_var), False)), ValReg(8, RegI[0])));
		}
		AssembleExpr(ast->Child, 0, 0);
		ToValue(ast->Child, 0, 0);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValReg(8, RegI[0])));
		SetTypeIdForFromBin(Reg_DX, ast->ChildType);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_R8), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_var), False))));
		CallKuinLib(L"_fromBin");
		ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_var), False)), ValReg(8, Reg_AX)));
		{
			SAstType* type = ast->ChildType;
			int size = GetSize(type);
			if (IsClass(type))
				RefClass((SAstClass*)((SAst*)type)->RefItem);
			if (IsRef(type))
				SetGcInstance(0, -1, type);
			PopRegs(tmp);
			if (IsFloat(type))
				ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_var), False))));
			else
				ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i]), ValMem(size, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_var), False))));
		}
	}
}

static void AssembleExprCall(SAstExprCall* ast, int reg_i, int reg_f)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	{
		Bool class_method_call = ((SAst*)ast->Func)->TypeId == AstTypeId_ExprDot && IsClass(((SAstExprDot*)ast->Func)->Var->Type) && ((SAstExprDot*)ast->Func)->ClassItem->Def->TypeId == AstTypeId_Func;
		STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
		SAstArg** tmp_args = (SAstArg**)Alloc(sizeof(SAstArg*) * (size_t)ast->Args->Len);
		{
			int idx = 0;
			SListNode* ptr = ast->Args->Top;
			while (ptr != NULL)
			{
				SAstExprCallArg* arg = (SAstExprCallArg*)ptr->Data;
				if (class_method_call && idx == 0)
				{
					ptr = ptr->Next;
					idx++;
					continue;
				}
				{
					AssembleExpr(arg->Arg, 0, 0);
					if (arg->RefVar)
					{
						ToRef(arg->Arg, RegI[0], RegI[0]);
						tmp_args[idx] = MakeTmpVar(8, NULL);
						if (arg->SkipVar)
						{
							int size = GetSize(arg->Arg->Type);
							ListAdd(PackAsm->Asms, AsmMOV(ValMemS(size, ValReg(8, RegI[0]), NULL, 0x00), ValImmU(size, 0x00)));
						}
						ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_args[idx]), False)), ValReg(8, RegI[0])));
					}
					else
					{
						int size = GetSize(arg->Arg->Type);
						ToValue(arg->Arg, 0, 0);
						tmp_args[idx] = MakeTmpVar(size, NULL);
						if (IsFloat(arg->Arg->Type))
							ListAdd(PackAsm->Asms, AsmMOVSD(ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_args[idx]), False)), ValReg(4, RegF[0])));
						else
							ListAdd(PackAsm->Asms, AsmMOV(ValMem(size, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_args[idx]), False)), ValReg(size, RegI[0])));
					}
				}
				ptr = ptr->Next;
				idx++;
			}
		}
		if (class_method_call)
			AssembleExprDot((SAstExprDot*)ast->Func, 0, 0, False);
		else
			AssembleExpr(ast->Func, 0, 0);
		ToValue(ast->Func, 0, 0);
		{
			int idx = 0;
			SListNode* ptr = ast->Args->Top;
			while (ptr != NULL)
			{
				if (class_method_call && idx == 0)
				{
					// In case of method call, 'me' is not expanded, and store 'me' in the second argument.
					ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, (S64)idx * 8), ValReg(8, RegI[0])));
					ASSERT(IsRef(((SAstExprCallArg*)ptr->Data)->Arg->Type) && !((SAstExprCallArg*)ptr->Data)->RefVar);
					GcInc(0);
					// Expand 'me'.
					ExpandMe((SAstExprDot*)ast->Func, 0);
				}
				else if ((((SAstTypeFunc*)ast->Func->Type)->FuncAttr & FuncAttr_AnyType) != 0 && ptr == ast->Args->Top->Next)
				{
					// Set the second argument of '_any_type' functions to the type of 'me'.
					SetTypeId(RegI[1], ((SAstExprCallArg*)ast->Args->Top->Data)->Arg->Type);
					ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, (S64)idx * 8), ValReg(8, RegI[1])));
				}
				else if ((((SAstTypeFunc*)ast->Func->Type)->FuncAttr & FuncAttr_TakeKeyValue) != 0 && ptr == ast->Args->Top->Next->Next)
				{
					// Set the third argument of '_take_key_value' functions to the type of 'value'.
					ASSERT(((SAst*)((SAstExprCallArg*)ast->Args->Top->Data)->Arg->Type)->TypeId == AstTypeId_TypeDict);
					SetTypeId(RegI[1], ((SAstTypeDict*)((SAstExprCallArg*)ast->Args->Top->Data)->Arg->Type)->ItemTypeValue);
					ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_SP), NULL, (S64)idx * 8), ValReg(8, RegI[1])));
				}
				else
				{
					SAstExprCallArg* arg = (SAstExprCallArg*)ptr->Data;
					int size = arg->RefVar ? 8 : GetSize(arg->Arg->Type);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[1]), ValMem(size, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_args[idx]), False))));
					ListAdd(PackAsm->Asms, AsmMOV(ValMemS(size, ValReg(8, Reg_SP), NULL, (S64)idx * 8), ValReg(size, RegI[1])));
					if (IsRef(arg->Arg->Type) && !arg->RefVar)
						GcInc(1);
				}
				ptr = ptr->Next;
				idx++;
			}
		}
		ListAdd(PackAsm->Asms, AsmCALL(ValReg(8, RegI[0])));
		ListAdd((SList*)StackPeek(RefFuncArgNums), (void*)(S64)ast->Args->Len);
		{
			SAstType* type = ((SAstExpr*)ast)->Type;
			if (type != NULL)
			{
				int size = GetSize(type);
				if (IsClass(type))
					RefClass((SAstClass*)((SAst*)type)->RefItem);
				if (IsFloat(type))
					ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, Reg_XMM14), ValReg(4, Reg_XMM0)));
				else
				{
					if (IsRef(type))
						SetGcInstance(0, -1, type);
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, Reg_SI), ValReg(size, Reg_AX)));
				}
			}
			PopRegs(tmp);
			if (type != NULL)
			{
				int size = GetSize(type);
				if (IsFloat(type))
				{
					if ((((SAstTypeFunc*)ast->Func->Type)->FuncAttr & FuncAttr_RetChild) != 0)
					{
						SAstArg* tmp_i_to_f = MakeTmpVar(8, NULL);
						ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_i_to_f), False)), ValReg(8, Reg_SI)));
						ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValMem(4, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_i_to_f), False))));
					}
					else
						ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValReg(4, Reg_XMM14)));
				}
				else
					ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i]), ValReg(size, Reg_SI)));
			}
		}
	}
}

static void AssembleExprArray(SAstExprArray* ast, int reg_i, int reg_f)
{
	SAstArg* tmp_i = NULL;
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	if (reg_i + 1 == REG_I_NUM)
	{
		tmp_i = MakeTmpVar(8, NULL);
		ListAdd(PackAsm->Asms, AsmMOV(ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_i), False)), ValReg(8, RegI[reg_i - 1])));
		reg_i--;
	}
	else
		ASSERT(reg_i + 1 < REG_I_NUM);
	AssembleExpr(ast->Var, reg_i, reg_f);
	ToValue(ast->Var, reg_i, reg_f);
	AssembleExpr(ast->Idx, reg_i + 1, reg_f);
	ToValue(ast->Idx, reg_i + 1, reg_f);
	if (!Option->Rls)
	{
		// Check whether the index is out of the range.
		SAsmLabel* lbl1 = AsmLabel();
		SAsmLabel* lbl2 = AsmLabel();
		ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, RegI[reg_i + 1]), ValImmU(8, 0x00)));
		ListAdd(PackAsm->Asms, AsmJL(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
		ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, RegI[reg_i + 1]), ValMemS(8, ValReg(8, RegI[reg_i]), NULL, 0x08)));
		ListAdd(PackAsm->Asms, AsmJL(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
		ListAdd(PackAsm->Asms, lbl1);
#if defined(_DEBUG)
		ListAdd(PackAsm->Asms, AsmINT(ValImmU(8, 0x03)));
#endif
		RaiseExcpt(EXCPT_DBG_ARRAY_IDX_OUT_OF_RANGE);
		ListAdd(PackAsm->Asms, lbl2);
	}
	ASSERT(((SAst*)ast->Var->Type)->TypeId == AstTypeId_TypeArray);
	ListAdd(PackAsm->Asms, AsmIMUL(ValReg(8, Reg_SI), ValReg(GetSize(ast->Idx->Type), RegI[reg_i + 1]), ValImmU(8, GetSize(((SAstTypeArray*)ast->Var->Type)->ItemType))));
	ListAdd(PackAsm->Asms, AsmADD(ValReg(8, Reg_SI), ValImmU(8, 0x10)));
	ListAdd(PackAsm->Asms, AsmADD(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
	if (tmp_i != NULL)
	{
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i + 1]), ValReg(8, RegI[reg_i])));
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValMem(8, ValReg(8, Reg_SP), NULL, RefValueAddr(RefLocalVar(tmp_i), False))));
	}
}

static void AssembleExprDot(SAstExprDot* ast, int reg_i, int reg_f, Bool expand_me)
{
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	if (IsClass(ast->Var->Type))
	{
		ASSERT(ast->ClassItem != NULL);
		RefClass((SAstClass*)((SAst*)ast->Var->Type)->RefItem); // The addresses of the members are calculated in this.
		AssembleExpr(ast->Var, reg_i, reg_f);
		ToValue(ast->Var, reg_i, reg_f);
		if (ast->ClassItem->Def->TypeId == AstTypeId_Var)
			ListAdd(PackAsm->Asms, AsmADD(ValReg(8, RegI[reg_i]), ValImmS(8, 0x10 + ast->ClassItem->Addr)));
		else
		{
			ASSERT(ast->ClassItem->Def->TypeId == AstTypeId_Func);
			if (expand_me)
			{
				// In case of method call, 'me' should not be expanded.
				ExpandMe(ast, reg_i);
			}
		}
	}
	else
	{
		ASSERT(((SAst*)ast)->RefItem->TypeId == AstTypeId_ExprRef);
		AssembleExprRef((SAstExpr*)((SAst*)ast)->RefItem, reg_i, reg_f);
	}
}

static void AssembleExprValue(SAstExprValue* ast, int reg_i, int reg_f)
{
	SAstType* type = ((SAstExpr*)ast)->Type;
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	if (IsStr(type))
	{
		STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
		const Char** data = (const Char**)ast->Value;
		size_t len = wcslen(*data);
		AllocHeap(ValImmU(8, (U64)(0x10 + sizeof(Char) * (len + 1))));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_AX), NULL, 0x00), ValImmU(8, 0x01)));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_AX), NULL, 0x08), ValImmU(8, (U64)len)));
		{
			S64* addr = AddReadonlyData((int)(sizeof(Char) * (len + 1)), (const U8*)*data, False);
			ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_SI), ValRIP(8, RefValueAddr(addr, True))));
		}
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_DI), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x10)));
		CopyMem(Reg_DI, (U64)(sizeof(Char) * (len + 1)), Reg_SI, Reg_CX);
		SetGcInstance(0, -1, ((SAstExpr*)ast)->Type);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
		PopRegs(tmp);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
	}
	else if (((SAst*)type)->TypeId == AstTypeId_TypeBit || IsBool(type) || IsChar(type))
	{
		int size = GetSize(((SAstExpr*)ast)->Type);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i]), ValImmU(size, *(U64*)ast->Value)));
	}
	else if (IsInt(type) || IsEnum(type))
	{
		int size = GetSize(((SAstExpr*)ast)->Type);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(size, RegI[reg_i]), ValImmS(size, *(S64*)ast->Value)));
	}
	else if (IsFloat(type))
	{
		S64* addr = AddReadonlyData(8, ast->Value, False);
		ListAdd(PackAsm->Asms, AsmMOVSD(ValReg(4, RegF[reg_f]), ValRIP(4, RefValueAddr(addr, True))));
	}
	else
	{
		ASSERT(((SAst*)type)->TypeId == AstTypeId_TypeNull);
		ListAdd(PackAsm->Asms, AsmXOR(ValReg(4, RegI[reg_i]), ValReg(4, RegI[reg_i])));
	}
}

static void AssembleExprValueArray(SAstExprValueArray* ast, int reg_i, int reg_f)
{
	// Note that constant string values are handled by 'AssembleExprValue'.
	SAstType* type = ((SAstExpr*)ast)->Type;
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	ASSERT(((SAst*)type)->TypeId == AstTypeId_TypeArray);
	{
		STmpVars* tmp = PushRegs(reg_i - 1, reg_f - 1);
		int child_size = GetSize(((SAstTypeArray*)type)->ItemType);
		Bool is_str = IsChar(((SAstTypeArray*)type)->ItemType);
		AllocHeap(ValImmU(8, (U64)(0x10 + child_size * (ast->Values->Len + (is_str ? 1 : 0)))));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_AX), NULL, 0x00), ValImmU(8, 0x01)));
		ListAdd(PackAsm->Asms, AsmMOV(ValMemS(8, ValReg(8, Reg_AX), NULL, 0x08), ValImmU(8, (U64)ast->Values->Len)));
		if (is_str)
			ListAdd(PackAsm->Asms, AsmMOV(ValMemS(child_size, ValReg(8, Reg_AX), NULL, (S64)(0x10 + child_size * ast->Values->Len)), ValImmU(child_size, 0x00)));
		{
			int addr = 0x10;
			Bool is_float = IsFloat(((SAstTypeArray*)type)->ItemType);
			SListNode* ptr = ast->Values->Top;
			while (ptr != NULL)
			{
				AssembleExpr((SAstExpr*)ptr->Data, 1, 0);
				ToValue((SAstExpr*)ptr->Data, 1, 0);
				if (is_float)
					ListAdd(PackAsm->Asms, AsmMOVSD(ValMemS(4, ValReg(8, Reg_AX), NULL, (S64)addr), ValReg(4, RegF[0])));
				else
					ListAdd(PackAsm->Asms, AsmMOV(ValMemS(child_size, ValReg(8, Reg_AX), NULL, (S64)addr), ValReg(child_size, RegI[1])));
				addr += child_size;
				ptr = ptr->Next;
			}
		}
		SetGcInstance(0, -1, type);
		if (IsRef(((SAstTypeArray*)type)->ItemType))
		{
			// Increment the reference counter for each element of the array.
			SAsmLabel* lbl1 = AsmLabel();
			SAsmLabel* lbl2 = AsmLabel();
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_CX), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x08))); // A loop counter.
			ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, Reg_SI), ValMemS(8, ValReg(8, Reg_AX), NULL, 0x10))); // A pointer to the each element.
			ListAdd(PackAsm->Asms, lbl1);
			ListAdd(PackAsm->Asms, AsmCMP(ValReg(8, Reg_CX), ValImmU(8, 0x00)));
			ListAdd(PackAsm->Asms, AsmJE(ValImm(4, RefValueAddr(((SAsm*)lbl2)->Addr, True))));
			ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_DX), ValMemS(8, ValReg(8, Reg_SI), NULL, 0x00)));
			GcInc(2);
			ListAdd(PackAsm->Asms, AsmDEC(ValReg(8, Reg_CX)));
			ListAdd(PackAsm->Asms, AsmADD(ValReg(8, Reg_SI), ValImmU(8, (U64)child_size)));
			ListAdd(PackAsm->Asms, AsmJMP(ValImm(4, RefValueAddr(((SAsm*)lbl1)->Addr, True))));
			ListAdd(PackAsm->Asms, lbl2);
		}
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, Reg_SI), ValReg(8, Reg_AX)));
		PopRegs(tmp);
		ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValReg(8, Reg_SI)));
	}
}

static void AssembleExprRef(SAstExpr* ast, int reg_i, int reg_f)
{
	SAst* ast2 = ((SAst*)ast)->RefItem;
	UNUSED(reg_f);
	ASSERT(((SAst*)ast)->AnalyzedCache != NULL);
	ASSERT(((SAstExpr*)ast)->VarKind != AstExprVarKind_Unknown);
	if (((SAst*)ast2)->TypeId == AstTypeId_Func)
	{
		S64* addr = RefLocalFunc((SAstFunc*)ast2);
		ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, RegI[reg_i]), ValRIP(8, RefValueAddr(addr, True))));
	}
	else
	{
		ASSERT(((SAst*)ast2)->TypeId == AstTypeId_Arg);
		{
			int size = GetSize(((SAstArg*)ast2)->Type);
			switch (((SAstArg*)ast2)->Kind)
			{
				case AstArgKind_Global:
					ASSERT(ast2->Name != NULL && wcscmp(ast2->Name, L"$") != 0);
					{
						S64* addr = AddWritableData(NewStr(NULL, L"%s@%s", ast2->Pos->SrcName, ast2->Name), size);
						((SAstArg*)ast2)->Addr = addr;
						ListAdd(PackAsm->Asms, AsmLEA(ValReg(8, RegI[reg_i]), ValRIP(8, RefValueAddr(addr, True))));
					}
					break;
				case AstArgKind_LocalArg:
				case AstArgKind_LocalVar:
					{
						S64* addr = RefLocalVar((SAstArg*)ast2);
						ListAdd(PackAsm->Asms, AsmMOV(ValReg(8, RegI[reg_i]), ValImm(8, RefValueAddr(addr, False))));
					}
					break;
				default:
					ASSERT(False);
					break;
			}
		}
	}
}
