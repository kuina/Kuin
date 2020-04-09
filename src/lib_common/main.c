// LibCommon.dll
//
// (C)Kuina-chan
//

#include "main.h"

#include "lib.h"
#include "rnd.h"

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

typedef struct SBinList
{
	const void* Bin;
	struct SBinList* Next;
} SBinList;

typedef struct SListPtr
{
	SClass Class;
	void* Ptr;
} SListPtr;

extern Char ResRoot[KUIN_MAX_PATH + 1];

static Bool IsRef(U8 type);
static size_t GetSize(U8 type);
static S64 Add(S64 a, S64 b);
static S64 Sub(S64 a, S64 b);
static S64 Mul(S64 a, S64 b);
static void GetDictTypes(const U8* type, const U8** child1, const U8** child2);
static void GetDictTypesRecursion(const U8** type);
static void FreeDictRecursion(void* ptr, const U8* child1, const U8* child2);
static Bool IsStr(const U8* type);
static Bool IsSpace(Char c);
static void Copy(void* dst, U8 type, const void* src);
static void* AddDictRecursion(void* node, const void* key, const void* item, int cmp_func(const void* a, const void* b), U8* key_type, U8* item_type, Bool* addition);
static void* DelDictRecursion(void* node, const void* key, int cmp_func(const void* a, const void* b), U8* key_type, U8* item_type, Bool* deleted, Bool* balanced);
static void* DelDictBalanceLeftRecursion(void* node, Bool* balanced);
static void* DelDictBalanceRightRecursion(void* node, Bool* balanced);
static void* DictRotateLeft(void* node);
static void* DictRotateRight(void* node);
static void* CopyDictRecursion(void* node, U8* key_type, U8* item_type);
static void ToBinDictRecursion(void*** buf, void* node, U8* key_type, U8* item_type);
static void* FromBinDictRecursion(const U8* key_type, const U8* item_type, const U8* bin, S64* idx, const void* type_class);
static void ToArrayKeyDictRecursion(U8** buf, size_t key_size, void* node);
static void ToArrayValueDictRecursion(U8** buf, size_t value_size, void* node);
static int(*GetCmpFunc(const U8* type))(const void* a, const void* b);
static int CmpInt(const void* a, const void* b);
static int CmpFloat(const void* a, const void* b);
static int CmpChar(const void* a, const void* b);
static int CmpBit8(const void* a, const void* b);
static int CmpBit16(const void* a, const void* b);
static int CmpBit32(const void* a, const void* b);
static int CmpBit64(const void* a, const void* b);
static int CmpStr(const void* a, const void* b);
static void* CatBin(int num, void** bins);
static void MergeSortArray(void* me_, const U8* type, Bool asc);
static void MergeSortList(void* me_, const U8* type, Bool asc);
static Bool ForEachRecursion(void* ptr, const U8* child1, const U8* child2, const void* callback, void* data);

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

#if defined(DBG)
	SetCurrentDirectory(EnvVars.ResRoot);
#endif

	// Initialize the COM library and the timer.
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) // 'STA'
	{
		// Maybe the COM library is already initialized in some way.
		ASSERT(False);
	}
	timeBeginPeriod(1);

	LibInit();
}

EXPORT void _fin(void)
{
	// Finalize the COM library and the timer.
	CoUninitialize();
	timeEndPeriod(1);
}

EXPORT void _err(S64 excpt)
{
#if defined(DBG)
	Char str[1024];
	const Char* text = L"Unknown exception.";
	if (0x00000000 <= excpt && excpt <= 0x0000ffff)
	{
		text = L"User defined exception.";
	}
	else
	{
		switch (excpt)
		{
			case EXCPT_ACCESS_VIOLATION: text = L"Access violation."; break;
			case 0xc0000017: text = L"No memory."; break;
			case 0xc0000090: text = L"Float invalid operation."; break;
			case 0xc0000094: text = L"Integer division by zero."; break;
			case 0xc00000fd: text = L"Stack overflow."; break;
			case 0xc000013a: text = L"Ctrl-C exit."; break;
			case EXCPT_DBG_ASSERT_FAILED: text = L"Assertion failed."; break;
			case EXCPT_CLASS_CAST_FAILED: text = L"Class cast failed."; break;
			case EXCPT_DBG_ARRAY_IDX_OUT_OF_RANGE: text = L"Array index out of range."; break;
			case EXCPT_DBG_INT_OVERFLOW: text = L"Integer overflow."; break;
			case EXCPT_INVALID_CMP: text = L"Invalid comparison."; break;
			case 0xe9170005: text = L"Invalid operation on standard library class."; break;
			case EXCPT_DBG_ARG_OUT_DOMAIN: text = L"Argument outside the domain."; break;
			case EXCPT_FILE_READ_FAILED: text = L"File reading failed."; break;
			case EXCPT_INVALID_DATA_FMT: text = L"Invalid data format."; break;
			case EXCPT_DEVICE_INIT_FAILED: text = L"Device initialization failed."; break;
			case EXCPT_DBG_INOPERABLE_STATE: text = L"Inoperable state."; break;
}
	}
	swprintf(str, 1024, L"An exception '0x%08X' occurred.\r\n\r\n> %s", (U32)excpt, text);
	MessageBox(0, str, NULL, 0);
#else
	UNUSED(excpt);
#endif
}

EXPORT void _freeSet(void* ptr, const U8* type)
{
	switch (*type)
	{
		case TypeId_Array:
			type++;
			if (IsRef(*type))
			{
				S64 len = ((S64*)ptr)[1];
				void** ptr2 = (void**)((U8*)ptr + 0x10);
				while (len != 0)
				{
					if (*ptr2 != NULL)
					{
						(*(S64*)*ptr2)--;
						if (*(S64*)*ptr2 == 0)
							_freeSet(*ptr2, type);
					}
					ptr2++;
					len--;
				}
			}
			FreeMem(ptr);
			break;
		case TypeId_List:
			type++;
			{
				void* ptr2 = *(void**)((U8*)ptr + 0x10);
				while (ptr2 != NULL)
				{
					void* ptr3 = ptr2;
					if (IsRef(*type))
					{
						void* ptr4 = *(void**)((U8*)ptr2 + 0x10);
						if (ptr4 != NULL)
						{
							(*(S64*)ptr4)--;
							if (*(S64*)ptr4 == 0)
								_freeSet(ptr4, type);
						}
					}
					ptr2 = *(void**)((U8*)ptr2 + 0x08);
					FreeMem(ptr3);
				}
			}
			FreeMem(ptr);
			break;
		case TypeId_Stack:
			type++;
			{
				void* ptr2 = *(void**)((U8*)ptr + 0x10);
				while (ptr2 != NULL)
				{
					void* ptr3 = ptr2;
					if (IsRef(*type))
					{
						void* ptr4 = *(void**)((U8*)ptr2 + 0x08);
						if (ptr4 != NULL)
						{
							(*(S64*)ptr4)--;
							if (*(S64*)ptr4 == 0)
								_freeSet(ptr4, type);
						}
					}
					ptr2 = *(void**)ptr2;
					FreeMem(ptr3);
				}
			}
			FreeMem(ptr);
			break;
		case TypeId_Queue:
			type++;
			{
				void* ptr2 = *(void**)((U8*)ptr + 0x10);
				while (ptr2 != NULL)
				{
					void* ptr3 = ptr2;
					if (IsRef(*type))
					{
						void* ptr4 = *(void**)((U8*)ptr2 + 0x08);
						if (ptr4 != NULL)
						{
							(*(S64*)ptr4)--;
							if (*(S64*)ptr4 == 0)
								_freeSet(ptr4, type);
						}
					}
					ptr2 = *(void**)ptr2;
					FreeMem(ptr3);
				}
			}
			FreeMem(ptr);
			break;
		case TypeId_Dict:
			{
				U8* child1;
				U8* child2;
				GetDictTypes(type, &child1, &child2);
				{
					void* ptr2 = *(void**)((U8*)ptr + 0x10);
					if (ptr2 != NULL)
						FreeDictRecursion(ptr2, child1, child2);
				}
				FreeMem(ptr);
			}
			break;
		default:
			ASSERT(*type == TypeId_Class);
			*(S64*)ptr = 2;
			DtorClassAsm(ptr);
			FreeMem(ptr);
			break;
	}
}

EXPORT void* _copy(const void* me_, const U8* type)
{
	ASSERT(me_ != NULL);
	switch (*type)
	{
		case TypeId_Array:
			{
				size_t size = GetSize(type[1]);
				S64 len = *(S64*)((U8*)me_ + 0x08);
				Bool is_str = IsStr(type);
				U8* result = (U8*)AllocMem(0x10 + size * (size_t)(len + (is_str ? 1 : 0)));
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = len;
				if (IsRef(type[1]))
				{
					S64 i;
					void** src = (void**)((U8*)me_ + 0x10);
					void** dst = (void**)(result + 0x10);
					for (i = 0; i < len; i++)
					{
						if (*src == NULL)
							*dst = NULL;
						else
							*dst = _copy(*src, type + 1);
						src++;
						dst++;
					}
				}
				else
					memcpy(result + 0x10, (U8*)me_ + 0x10, size * (size_t)(len + (is_str ? 1 : 0)));
				return result;
			}
		case TypeId_List:
			{
				size_t size = GetSize(type[1]);
				Bool is_ref = IsRef(type[1]);
				U8* result = (U8*)AllocMem(0x28);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = *(S64*)((U8*)me_ + 0x08);
				((S64*)result)[2] = 0;
				((S64*)result)[3] = 0;
				((S64*)result)[4] = 0;
				{
					void* src = *(void**)((U8*)me_ + 0x10);
					void* ptr_cur = *(void**)((U8*)me_ + 0x20);
					while (src != NULL)
					{
						U8* node = (U8*)AllocMem(0x10 + size);
						if (ptr_cur == src)
							((void**)result)[4] = node;
						if (is_ref)
						{
							if (*(void**)((U8*)src + 0x10) == NULL)
								*(void**)(node + 0x10) = NULL;
							else
								*(void**)(node + 0x10) = _copy(*(void**)((U8*)src + 0x10), type + 1);
						}
						else
							memcpy(node + 0x10, (U8*)src + 0x10, size);
						*(void**)(node + 0x08) = NULL;
						if (*(void**)(result + 0x10) == NULL)
						{
							*(void**)node = NULL;
							*(void**)(result + 0x10) = node;
							*(void**)(result + 0x18) = node;
						}
						else
						{
							*(void**)node = *(void**)(result + 0x18);
							*(void**)((U8*)*(void**)(result + 0x18) + 0x08) = node;
							*(void**)(result + 0x18) = node;
						}
						src = *(void**)((U8*)src + 0x08);
					}
				}
				return result;
			}
		case TypeId_Stack:
			{
				size_t size = GetSize(type[1]);
				Bool is_ref = IsRef(type[1]);
				void* top = NULL;
				void* bottom = NULL;
				U8* result = (U8*)AllocMem(0x18);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = *(S64*)((U8*)me_ + 0x08);
				((S64*)result)[2] = 0;
				{
					void* src = *(void**)((U8*)me_ + 0x10);
					while (src != NULL)
					{
						U8* node = (U8*)AllocMem(0x08 + size);
						if (is_ref)
						{
							if (*(void**)((U8*)src + 0x08) == NULL)
								*(void**)(node + 0x08) = NULL;
							else
								*(void**)(node + 0x08) = _copy(*(void**)((U8*)src + 0x08), type + 1);
						}
						else
							memcpy(node + 0x08, (U8*)src + 0x08, size);
						*(void**)node = NULL;
						if (top == NULL)
						{
							top = node;
							bottom = node;
						}
						else
						{
							*(void**)bottom = node;
							bottom = node;
						}
						src = *(void**)src;
					}
					*(void**)(result + 0x10) = top;
				}
				return result;
			}
		case TypeId_Queue:
			{
				size_t size = GetSize(type[1]);
				Bool is_ref = IsRef(type[1]);
				U8* result = (U8*)AllocMem(0x20);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = *(S64*)((U8*)me_ + 0x08);
				((S64*)result)[2] = 0;
				((S64*)result)[3] = 0;
				{
					void* src = *(void**)((U8*)me_ + 0x10);
					while (src != NULL)
					{
						U8* node = (U8*)AllocMem(0x08 + size);
						if (is_ref)
						{
							if (*(void**)((U8*)src + 0x08) == NULL)
								*(void**)(node + 0x08) = NULL;
							else
								*(void**)(node + 0x08) = _copy(*(void**)((U8*)src + 0x08), type + 1);
						}
						else
							memcpy(node + 0x08, (U8*)src + 0x08, size);
						*(void**)node = NULL;
						if (*(void**)(result + 0x18) == NULL)
							*(void**)(result + 0x10) = node;
						else
							*(void**)*(void**)(result + 0x18) = node;
						*(void**)(result + 0x18) = node;
						src = *(void**)src;
					}
				}
				return result;
			}
		case TypeId_Dict:
			{
				U8* child1;
				U8* child2;
				GetDictTypes(type, &child1, &child2);
				{
					U8* result = (U8*)AllocMem(0x18);
					((S64*)result)[0] = DefaultRefCntOpe;
					((S64*)result)[1] = *(S64*)((U8*)me_ + 0x08);
					((S64*)result)[2] = 0;
					if (*(void**)((U8*)me_ + 0x10) != NULL)
						*(void**)(result + 0x10) = CopyDictRecursion(*(void**)((U8*)me_ + 0x10), child1, child2);
					return result;
				}
			}
		default:
			ASSERT(*type == TypeId_Class);
			return CopyClassAsm(me_);
	}
}

EXPORT void* _toBin(const void* me_, const U8* type)
{
	if (IsRef(*type) && me_ == NULL)
	{
		U8* result = (U8*)AllocMem(0x10 + 0x08);
		((S64*)result)[0] = DefaultRefCntOpe;
		((S64*)result)[1] = 0x08;
		((S64*)result)[2] = -1;
		return result;
	}
	switch (*type)
	{
		case TypeId_Int:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x08);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x08;
				*(S64*)(result + 0x10) = *(S64*)&me_;
				return result;
			}
		case TypeId_Float:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x08);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x08;
				*(double*)(result + 0x10) = *(double*)&me_;
				return result;
			}
		case TypeId_Char:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x02);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x02;
				*(Char*)(result + 0x10) = *(Char*)&me_;
				return result;
			}
		case TypeId_Bool:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x01);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x01;
				*(Bool*)(result + 0x10) = *(Bool*)&me_;
				return result;
			}
		case TypeId_Bit8:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x01);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x01;
				*(U8*)(result + 0x10) = *(U8*)&me_;
				return result;
			}
		case TypeId_Bit16:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x02);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x02;
				*(U16*)(result + 0x10) = *(U16*)&me_;
				return result;
			}
		case TypeId_Bit32:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x04);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x04;
				*(U32*)(result + 0x10) = *(U32*)&me_;
				return result;
			}
		case TypeId_Bit64:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x08);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x08;
				*(U64*)(result + 0x10) = *(U64*)&me_;
				return result;
			}
		case TypeId_Array:
			{
				S64 len = *(S64*)((U8*)me_ + 0x08);
				void** bins = (void**)AllocMem(sizeof(void*) * (size_t)len);
				size_t size = GetSize(type[1]);
				U8* ptr = (U8*)me_ + 0x10;
				S64 i;
				for (i = 0; i < len; i++)
				{
					void* value = NULL;
					memcpy(&value, ptr, size);
					bins[i] = _toBin(value, type + 1);
					ptr += size;
				}
				return CatBin((int)len, bins);
			}
		case TypeId_List:
			{
				S64 len = *(S64*)((U8*)me_ + 0x08);
				void** bins = (void**)AllocMem(sizeof(void*) * (size_t)(len + 1));
				size_t size = GetSize(type[1]);
				void* ptr = *(void**)((U8*)me_ + 0x10);
				void* ptr_cur = *(void**)((U8*)me_ + 0x20);
				S64 idx = -1;
				S64 i;
				for (i = 0; i < len; i++)
				{
					void* value = NULL;
					memcpy(&value, (U8*)ptr + 0x10, size);
					bins[i + 1] = _toBin(value, type + 1);
					if (ptr_cur == ptr)
						idx = i;
					ptr = *(void**)((U8*)ptr + 0x08);
				}
				{
					void* bin = AllocMem(0x10 + 0x08);
					((S64*)bin)[0] = DefaultRefCntOpe;
					((S64*)bin)[1] = 0x08;
					((S64*)bin)[2] = idx;
					bins[0] = bin;
				}
				return CatBin((int)len + 1, bins);
			}
		case TypeId_Stack:
		case TypeId_Queue:
			{
				S64 len = *(S64*)((U8*)me_ + 0x08);
				void** bins = (void**)AllocMem(sizeof(void*) * (size_t)len);
				size_t size = GetSize(type[1]);
				void* ptr = *(void**)((U8*)me_ + 0x10);
				S64 i;
				for (i = 0; i < len; i++)
				{
					void* value = NULL;
					memcpy(&value, (U8*)ptr + 0x08, size);
					bins[i] = _toBin(value, type + 1);
					ptr = *(void**)ptr;
				}
				return CatBin((int)len, bins);
			}
		case TypeId_Dict:
			{
				U8* child1;
				U8* child2;
				GetDictTypes(type, &child1, &child2);
				{
					S64 len = *(S64*)((U8*)me_ + 0x08);
					void** bins = (void**)AllocMem(sizeof(void*) * (size_t)len * 3); // 'key' + 'value' + 'info' per node.
					void** ptr = bins;
					if (len != 0)
						ToBinDictRecursion(&ptr, *(void**)((U8*)me_ + 0x10), child1, child2);
					return CatBin((int)len * 3, bins);
				}
			}
		case TypeId_Func:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x08);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x08;
				*(S64*)(result + 0x10) = 0;
				return result;
			}
		case TypeId_Enum:
			{
				U8* result = (U8*)AllocMem(0x10 + 0x08);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = 0x08;
				*(S64*)(result + 0x10) = *(S64*)&me_;
				return result;
			}
		default:
			ASSERT(*type == TypeId_Class);
			return ToBinClassAsm(me_);
	}
}

EXPORT void* _fromBin(const U8* me_, const void** type, S64* idx)
{
	const U8* type2 = (const U8*)type[0];
	void* result = NULL;
	if (IsRef(*type2) && *(S64*)(me_ + 0x10 + *idx) == -1)
	{
		*idx += 8;
		return NULL;
	}
	switch (*type2)
	{
		case TypeId_Int:
			*(S64*)&result = *(S64*)(me_ + 0x10 + *idx);
			*idx += 8;
			break;
		case TypeId_Float:
			*(double*)&result = *(double*)(me_ + 0x10 + *idx);
			*idx += 8;
			break;
		case TypeId_Char:
			*(Char*)&result = *(Char*)(me_ + 0x10 + *idx);
			*idx += 2;
			break;
		case TypeId_Bool:
			*(Bool*)&result = *(Bool*)(me_ + 0x10 + *idx);
			*idx += 1;
			break;
		case TypeId_Bit8:
			*(U8*)&result = *(U8*)(me_ + 0x10 + *idx);
			*idx += 1;
			break;
		case TypeId_Bit16:
			*(U16*)&result = *(U16*)(me_ + 0x10 + *idx);
			*idx += 2;
			break;
		case TypeId_Bit32:
			*(U32*)&result = *(U32*)(me_ + 0x10 + *idx);
			*idx += 4;
			break;
		case TypeId_Bit64:
			*(U64*)&result = *(U64*)(me_ + 0x10 + *idx);
			*idx += 8;
			break;
		case TypeId_Array:
			{
				S64 len = *(S64*)(me_ + 0x10 + *idx);
				*idx += 8;
				{
					size_t size = GetSize(type2[1]);
					Bool is_str = IsStr(type2);
					result = AllocMem(0x10 + size * (size_t)(len + (is_str ? 1 : 0)));
					((S64*)result)[0] = DefaultRefCntOpe;
					((S64*)result)[1] = len;
					if (is_str)
						*(Char*)((U8*)result + 0x10 + size * (size_t)len) = L'\0';
					{
						U8* ptr = (U8*)result + 0x10;
						S64 i;
						for (i = 0; i < len; i++)
						{
							const void* type3[2];
							void* value;
							type3[0] = type2 + 1;
							type3[1] = type[1];
							value = _fromBin(me_, type3, idx);;
							memcpy(ptr, &value, size);
							ptr += size;
						}
					}
				}
			}
			break;
		case TypeId_List:
			{
				S64 idx_cur;
				S64 len = *(S64*)(me_ + 0x10 + *idx) - 1;
				*idx += 8;
				idx_cur = *(S64*)(me_ + 0x10 + *idx);
				*idx += 8;
				{
					size_t size = GetSize(type2[1]);
					result = AllocMem(0x28);
					((S64*)result)[0] = DefaultRefCntOpe;
					((S64*)result)[1] = len;
					((S64*)result)[2] = 0;
					((S64*)result)[3] = 0;
					((S64*)result)[4] = 0;
					{
						S64 i;
						for (i = 0; i < len; i++)
						{
							U8* node = (U8*)AllocMem(0x10 + size);
							const void* type3[2];
							void* value;
							if (idx_cur == i)
								((void**)result)[4] = node;
							type3[0] = type2 + 1;
							type3[1] = type[1];
							value = _fromBin(me_, type3, idx);
							memcpy(node + 0x10, &value, size);
							*(void**)(node + 0x08) = NULL;
							if (*(void**)((U8*)result + 0x10) == NULL)
							{
								*(void**)node = NULL;
								*(void**)((U8*)result + 0x10) = node;
								*(void**)((U8*)result + 0x18) = node;
							}
							else
							{
								*(void**)node = *(void**)((U8*)result + 0x18);
								*(void**)((U8*)*(void**)((U8*)result + 0x18) + 0x08) = node;
								*(void**)((U8*)result + 0x18) = node;
							}
						}
					}
				}
			}
			break;
		case TypeId_Stack:
			{
				S64 len = *(S64*)(me_ + 0x10 + *idx);
				*idx += 8;
				{
					size_t size = GetSize(type2[1]);
					void* top = NULL;
					void* bottom = NULL;
					result = AllocMem(0x18);
					((S64*)result)[0] = DefaultRefCntOpe;
					((S64*)result)[1] = len;
					((S64*)result)[2] = 0;
					{
						S64 i;
						for (i = 0; i < len; i++)
						{
							U8* node = (U8*)AllocMem(0x08 + size);
							const void* type3[2];
							void* value;
							type3[0] = type2 + 1;
							type3[1] = type[1];
							value = _fromBin(me_, type3, idx);
							memcpy(node + 0x08, &value, size);
							*(void**)node = NULL;
							if (top == NULL)
							{
								top = node;
								bottom = node;
							}
							else
							{
								*(void**)bottom = node;
								bottom = node;
							}
						}
						*(void**)((U8*)result + 0x10) = top;
					}
				}
			}
			break;
		case TypeId_Queue:
			{
				S64 len = *(S64*)(me_ + 0x10 + *idx);
				*idx += 8;
				{
					size_t size = GetSize(type2[1]);
					result = AllocMem(0x20);
					((S64*)result)[0] = DefaultRefCntOpe;
					((S64*)result)[1] = len;
					((S64*)result)[2] = 0;
					((S64*)result)[3] = 0;
					{
						S64 i;
						for (i = 0; i < len; i++)
						{
							U8* node = (U8*)AllocMem(0x08 + size);
							const void* type3[2];
							void* value;
							type3[0] = type2 + 1;
							type3[1] = type[1];
							value = _fromBin(me_, type3, idx);
							memcpy(node + 0x08, &value, size);
							*(void**)node = NULL;
							if (*(void**)((U8*)result + 0x18) == NULL)
								*(void**)((U8*)result + 0x10) = node;
							else
								*(void**)*(void**)((U8*)result + 0x18) = node;
							*(void**)((U8*)result + 0x18) = node;
						}
					}
				}
			}
			break;
		case TypeId_Dict:
			{
				U8* child1;
				U8* child2;
				GetDictTypes(type2, &child1, &child2);
				{
					S64 len = *(S64*)(me_ + 0x10 + *idx);
					*idx += 8;
					{
						result = AllocMem(0x18);
						((S64*)result)[0] = DefaultRefCntOpe;
						((S64*)result)[1] = len;
						((S64*)result)[2] = 0;
						if (len != 0)
							*(void**)((U8*)result + 0x10) = FromBinDictRecursion(child1, child2, me_, idx, type[1]);
					}
				}
			}
			break;
		case TypeId_Func:
			result = NULL;
			*idx += 8;
		case TypeId_Enum:
			*(S64*)&result = *(S64*)(me_ + 0x10 + *idx);
			*idx += 8;
			break;
		default:
			ASSERT(*type2 == TypeId_Class);
			if (*(S64*)(me_ + 0x10 + *idx) != 0)
				THROW(EXCPT_ACCESS_VIOLATION); // TODO: What is this code?
			*idx += 8;
			{
				const void** type3 = (const void**)type[1];
				result = FromBinClassAsm(type3[type2[1]], me_, idx);
			}
			break;
	}
	return result;
}

EXPORT S64 _powInt(S64 n, S64 m)
{
	switch (m)
	{
		case 0:
			return 1;
		case 1:
			return n;
		case 2:
			return Mul(n, n);
		default:
			if (m < 0)
			{
				THROWDBG(n == 0, EXCPT_DBG_INT_OVERFLOW);
				return n == 1 ? 1 : 0;
			}
			if (m == 0)
				return 1;
			{
				S64 result = 1;
				for (; ; )
				{
					if ((m & 1) == 1)
						result = Mul(result, n);
					m >>= 1;
					if (m == 0)
						break;
					n = Mul(n, n);
				}
				return result;
			}
			break;
	}
}

EXPORT double _powFloat(double n, double m)
{
	return pow(n, m);
}

EXPORT double _mod(double n, double m)
{
	return fmod(n, m);
}

EXPORT S64 _cmpStr(const U8* a, const U8* b)
{
	return (S64)wcscmp((Char*)(a + 0x10), (Char*)(b + 0x10));
}

EXPORT void* _newArray(S64 len, S64* nums, const U8* type)
{
	THROWDBG(*nums < 0, EXCPT_DBG_ARRAY_IDX_OUT_OF_RANGE);
	{
		size_t size = len == 1 ? GetSize(*type) : 8;
		Bool is_str = len == 1 && *type == TypeId_Char;
		U8* result = (U8*)AllocMem(0x10 + size * (size_t)(*nums + (is_str ? 1 : 0)));
		((S64*)result)[0] = DefaultRefCntOpe;
		((S64*)result)[1] = *nums;
		ASSERT(len >= 1);
		if (len != 1)
		{
			S64 i;
			U8* ptr = result + 0x10;
			for (i = 0; i < *nums; i++)
			{
				*(void**)ptr = _newArray(len - 1, nums + 1, type);
				ptr = ptr + 8;
			}
		}
		else
			memset(result + 0x10, 0, size * (size_t)(*nums + (is_str ? 1 : 0)));
		return result;
	}
}

EXPORT U8* _toStr(const void* me_, const U8* type)
{
	Char str[33];
	int len;
	switch (*type)
	{
		case TypeId_Int: len = swprintf(str, 33, L"%I64d", *(S64*)&me_); break;
		case TypeId_Float: len = swprintf(str, 33, L"%.16g", *(double*)&me_); break;
		case TypeId_Char:
			len = 1;
			str[0] = *(Char*)&me_;
			str[1] = L'\0';
			break;
		case TypeId_Bool:
			if (*(Bool*)&me_)
			{
				len = 4;
				wcscpy(str, L"true");
			}
			else
			{
				len = 5;
				wcscpy(str, L"false");
			}
			break;
		case TypeId_Bit8: len = swprintf(str, 33, L"0x%02X", *(U8*)&me_); break;
		case TypeId_Bit16: len = swprintf(str, 33, L"0x%04X", *(U16*)&me_); break;
		case TypeId_Bit32: len = swprintf(str, 33, L"0x%08X", *(U32*)&me_); break;
		case TypeId_Bit64: len = swprintf(str, 33, L"0x%016I64X", *(U64*)&me_); break;
		case TypeId_Array:
			ASSERT(type[1] == TypeId_Char);
			return (U8*)me_;
			break;
		default:
			len = 0;
			ASSERT(False);
			break;
	}
	ASSERT(len < 33);
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (S64)len;
		memcpy(result + 0x10, str, sizeof(Char) * (size_t)(len + 1));
		return result;
	}
}

EXPORT U8* _toStrFmtInt(S64 me_, const U8* fmt)
{
	THROWDBG(fmt == NULL, EXCPT_ACCESS_VIOLATION);
	const Char* fmt2 = (const Char*)(fmt + 0x10);
	int src_len = (int)((S64*)fmt)[1];
	Char dst[33];
	int dst_ptr = 0;
	int src_ptr = 0;

	dst[dst_ptr] = L'%';
	dst_ptr++;
	if (src_ptr < src_len && (fmt2[src_ptr] == L'+' || fmt2[src_ptr] == L' '))
	{
		dst[dst_ptr] = fmt2[src_ptr];
		dst_ptr++;
		if (dst_ptr + 4 >= 33)
			return NULL;
		src_ptr++;
	}
	if (src_ptr < src_len && (fmt2[src_ptr] == L'-' || fmt2[src_ptr] == L'0'))
	{
		dst[dst_ptr] = fmt2[src_ptr];
		dst_ptr++;
		if (dst_ptr + 4 >= 33)
			return NULL;
		src_ptr++;
	}
	if (src_ptr < src_len && L'1' <= fmt2[src_ptr] && fmt2[src_ptr] <= L'9')
	{
		dst[dst_ptr] = fmt2[src_ptr];
		dst_ptr++;
		if (dst_ptr + 4 >= 33)
			return NULL;
		src_ptr++;
		while (src_ptr < src_len && L'0' <= fmt2[src_ptr] && fmt2[src_ptr] <= L'9')
		{
			dst[dst_ptr] = fmt2[src_ptr];
			dst_ptr++;
			if (dst_ptr + 4 >= 33)
				return NULL;
			src_ptr++;
		}
	}
	if (!(src_ptr < src_len && (fmt2[src_ptr] == L'd' || fmt2[src_ptr] == L'x' || fmt2[src_ptr] == L'X' || fmt2[src_ptr] == L'u' || fmt2[src_ptr] == L'o')))
		return NULL;
	dst[dst_ptr] = L'I';
	dst_ptr++;
	dst[dst_ptr] = L'6';
	dst_ptr++;
	dst[dst_ptr] = L'4';
	dst_ptr++;
	dst[dst_ptr] = fmt2[src_ptr];
	dst_ptr++;
	dst[dst_ptr] = L'\0';
	dst_ptr++;
	ASSERT(src_ptr + 1 == src_len);

	Char str[65];
	int len = swprintf(str, 65, dst, me_);
	ASSERT(len < 65);
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (S64)len;
		memcpy(result + 0x10, str, sizeof(Char) * (size_t)(len + 1));
		return result;
	}
}

EXPORT U8* _toStrFmtFloat(double me_, const U8* fmt)
{
	THROWDBG(fmt == NULL, EXCPT_ACCESS_VIOLATION);
	const Char* fmt2 = (const Char*)(fmt + 0x10);
	int src_len = (int)((S64*)fmt)[1];
	Char dst[33];
	int dst_ptr = 0;
	int src_ptr = 0;

	dst[dst_ptr] = L'%';
	dst_ptr++;
	if (src_ptr < src_len && (fmt2[src_ptr] == L'+' || fmt2[src_ptr] == L' '))
	{
		dst[dst_ptr] = fmt2[src_ptr];
		dst_ptr++;
		if (dst_ptr + 1 >= 33)
			return NULL;
		src_ptr++;
	}
	if (src_ptr < src_len && (fmt2[src_ptr] == L'-' || fmt2[src_ptr] == L'0'))
	{
		dst[dst_ptr] = fmt2[src_ptr];
		dst_ptr++;
		if (dst_ptr + 1 >= 33)
			return NULL;
		src_ptr++;
	}
	if (src_ptr < src_len && L'1' <= fmt2[src_ptr] && fmt2[src_ptr] <= L'9')
	{
		dst[dst_ptr] = fmt2[src_ptr];
		dst_ptr++;
		if (dst_ptr + 1 >= 33)
			return NULL;
		src_ptr++;
		while (src_ptr < src_len && L'0' <= fmt2[src_ptr] && fmt2[src_ptr] <= L'9')
		{
			dst[dst_ptr] = fmt2[src_ptr];
			dst_ptr++;
			if (dst_ptr + 1 >= 33)
				return NULL;
			src_ptr++;
		}
	}
	if (src_ptr < src_len && fmt2[src_ptr] == L'.')
	{
		dst[dst_ptr] = fmt2[src_ptr];
		dst_ptr++;
		if (dst_ptr + 1 >= 33)
			return NULL;
		src_ptr++;
		if (!(src_ptr < src_len && L'0' <= fmt2[src_ptr] && fmt2[src_ptr] <= L'9'))
			return NULL;
		dst[dst_ptr] = fmt2[src_ptr];
		dst_ptr++;
		if (dst_ptr + 1 >= 33)
			return NULL;
		src_ptr++;
		while (src_ptr < src_len && L'0' <= fmt2[src_ptr] && fmt2[src_ptr] <= L'9')
		{
			dst[dst_ptr] = fmt2[src_ptr];
			dst_ptr++;
			if (dst_ptr + 1 >= 33)
				return NULL;
			src_ptr++;
		}
	}
	if (!(src_ptr < src_len && (fmt2[src_ptr] == L'f' || fmt2[src_ptr] == L'e' || fmt2[src_ptr] == L'E' || fmt2[src_ptr] == L'g' || fmt2[src_ptr] == L'G' || fmt2[src_ptr] == L'a' || fmt2[src_ptr] == L'A')))
		return NULL;
	dst[dst_ptr] = fmt2[src_ptr];
	dst_ptr++;
	dst[dst_ptr] = L'\0';
	dst_ptr++;
	ASSERT(src_ptr + 1 == src_len);

	Char str[65];
	int len = swprintf(str, 65, dst, me_);
	ASSERT(len < 65);
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = (S64)len;
		memcpy(result + 0x10, str, sizeof(Char) * (size_t)(len + 1));
		return result;
	}
}

EXPORT S64 _absInt(S64 me_)
{
	return me_ >= 0 ? me_ : Sub(0, me_);
}

EXPORT double _absFloat(double me_)
{
	return me_ >= 0.0 ? me_ : -me_;
}

EXPORT S64 _signInt(S64 me_)
{
	return me_ > 0 ? 1 : (me_ < 0 ? -1 : 0);
}

EXPORT double _signFloat(double me_)
{
	return me_ > 0.0 ? 1.0 : (me_ < 0.0 ? -1.0 : 0.0);
}

EXPORT S64 _clampInt(S64 me_, S64 min, S64 max)
{
	THROWDBG(min > max, EXCPT_DBG_ARG_OUT_DOMAIN);
	if (me_ < min)
		return min;
	if (me_ > max)
		return max;
	return me_;
}

EXPORT double _clampFloat(double me_, double min, double max)
{
	THROWDBG(min > max, EXCPT_DBG_ARG_OUT_DOMAIN);
	if (me_ < min)
		return min;
	if (me_ > max)
		return max;
	return me_;
}

EXPORT S64 _clampMinInt(S64 me_, S64 min)
{
	return me_ < min ? min : me_;
}

EXPORT double _clampMinFloat(double me_, double min)
{
	return me_ < min ? min : me_;
}

EXPORT S64 _clampMaxInt(S64 me_, S64 max)
{
	return me_ > max ? max : me_;
}

EXPORT double _clampMaxFloat(double me_, double max)
{
	return me_ > max ? max : me_;
}

EXPORT Char _offset(Char me_, int n)
{
	return (Char)((int)me_ + n);
}

EXPORT S64 _or(const void* me_, const U8* type, const void* n)
{
	switch (*type)
	{
		case TypeId_Bit8: return (S64)(U64)(*(U8*)&me_ | *(U8*)&n);
		case TypeId_Bit16: return (S64)(U64)(*(U16*)&me_ | *(U16*)&n);
		case TypeId_Bit32: return (S64)(U64)(*(U32*)&me_ | *(U32*)&n);
		case TypeId_Bit64: return (S64)(*(U64*)&me_ | *(U64*)&n);
		case TypeId_Enum: return (S64)(*(U64*)&me_ | *(U64*)&n);
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT S64 _and(const void* me_, const U8* type, const void* n)
{
	switch (*type)
	{
		case TypeId_Bit8: return (S64)(U64)(*(U8*)&me_ & *(U8*)&n);
		case TypeId_Bit16: return (S64)(U64)(*(U16*)&me_ & *(U16*)&n);
		case TypeId_Bit32: return (S64)(U64)(*(U32*)&me_ & *(U32*)&n);
		case TypeId_Bit64: return (S64)(*(U64*)&me_ & *(U64*)&n);
		case TypeId_Enum: return (S64)(*(U64*)&me_ & *(U64*)&n);
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT S64 _xor(const void* me_, const U8* type, const void* n)
{
	switch (*type)
	{
		case TypeId_Bit8: return (S64)(U64)(*(U8*)&me_ ^ *(U8*)&n);
		case TypeId_Bit16: return (S64)(U64)(*(U16*)&me_ ^ *(U16*)&n);
		case TypeId_Bit32: return (S64)(U64)(*(U32*)&me_ ^ *(U32*)&n);
		case TypeId_Bit64: return (S64)(*(U64*)&me_ ^ *(U64*)&n);
		case TypeId_Enum: return (S64)(*(U64*)&me_ ^ *(U64*)&n);
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT S64 _not(const void* me_, const U8* type)
{
	switch (*type)
	{
		case TypeId_Bit8: return (S64)(U64)(~*(U8*)&me_);
		case TypeId_Bit16: return (S64)(U64)(~*(U16*)&me_);
		case TypeId_Bit32: return (S64)(U64)(~*(U32*)&me_);
		case TypeId_Bit64: return (S64)(~*(U64*)&me_);
		case TypeId_Enum: return (S64)(~*(U64*)&me_);
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT S64 _shl(const void* me_, const U8* type, S64 n)
{
	switch (*type)
	{
		case TypeId_Bit8: return (S64)(U64)(*(U8*)&me_ << n);
		case TypeId_Bit16: return (S64)(U64)(*(U16*)&me_ << n);
		case TypeId_Bit32: return (S64)(U64)(*(U32*)&me_ << n);
		case TypeId_Bit64: return (S64)(*(U64*)&me_ << n);
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT S64 _shr(const void* me_, const U8* type, S64 n)
{
	// In Visual C++, shifting of unsigned type is guaranteed to be a logical shift.
	switch (*type)
	{
		case TypeId_Bit8: return (S64)(U64)(*(U8*)&me_ >> n);
		case TypeId_Bit16: return (S64)(U64)(*(U16*)&me_ >> n);
		case TypeId_Bit32: return (S64)(U64)(*(U32*)&me_ >> n);
		case TypeId_Bit64: return (S64)(*(U64*)&me_ >> n);
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT S64 _sar(const void* me_, const U8* type, S64 n)
{
	// In Visual C++, shifting of signed type is guaranteed to be an arithmetic shift.
	switch (*type)
	{
		case TypeId_Bit8: return (S64)((S8)*(U8*)&me_ >> n);
		case TypeId_Bit16: return (S64)((S16)*(U16*)&me_ >> n);
		case TypeId_Bit32: return (S64)((S32)*(U32*)&me_ >> n);
		case TypeId_Bit64: return ((S64)*(U64*)&me_ >> n);
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT S64 _endian(const void* me_, const U8* type)
{
	switch (*type)
	{
		case TypeId_Bit8: return (S64)(U64)(SwapEndianU8(*(U8*)&me_));
		case TypeId_Bit16: return (S64)(U64)(SwapEndianU16(*(U16*)&me_));
		case TypeId_Bit32: return (S64)(U64)(SwapEndianU32(*(U32*)&me_));
		case TypeId_Bit64: return (S64)(SwapEndianU64(*(U64*)&me_));
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT void* _sub(const void* me_, const U8* type, S64 start, S64 len)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len2 = *(S64*)((U8*)me_ + 0x08);
	if (len == -1)
		len = len2 - start;
	THROWDBG(start < 0 || len < 0 || start + len > len2, EXCPT_DBG_ARRAY_IDX_OUT_OF_RANGE);
	{
		Bool is_str = IsStr(type);
		size_t size = GetSize(type[1]);
		U8* result = (U8*)AllocMem(0x10 + size * (size_t)(len + (is_str ? 1 : 0)));
		const U8* src = (U8*)me_ + 0x10 + size * (size_t)start;
		((S64*)result)[0] = 1; // Return values of '_ret_me' functions are not automatically incremented.
		((S64*)result)[1] = len;
		memcpy(result + 0x10, src, size * (size_t)len);
		if (IsRef(type[1]))
		{
			void** ptr = (void**)(result + 0x10);
			S64 i;
			for (i = 0; i < len; i++)
			{
				if (*ptr != NULL)
					(*(S64*)*ptr)++;
				ptr++;
			}
		}
		if (is_str)
			*(Char*)(result + 0x10 + size * (size_t)len) = L'\0';
		return result;
	}
}

EXPORT void _reverse(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	U8* a = (U8*)me_ + 0x10;
	U8* b = (U8*)me_ + 0x10 + (size_t)(len - 1) * size;
	void* tmp;
	S64 i;
	for (i = 0; i < len / 2; i++)
	{
		memcpy(&tmp, a, size);
		memcpy(a, b, size);
		memcpy(b, &tmp, size);
		a += size;
		b -= size;
	}
}

EXPORT void _shuffle(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	U8* a = (U8*)me_ + 0x10;
	U8* b;
	void* tmp;
	S64 i;
	for (i = 0; i < len - 1; i++)
	{
		S64 r = _rnd(i, len - 1);
		if (i == r)
			continue;
		b = (U8*)me_ + 0x10 + (size_t)r * size;
		memcpy(&tmp, a, size);
		memcpy(a, b, size);
		memcpy(b, &tmp, size);
		a += size;
	}
}

EXPORT void _sortArray(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	MergeSortArray(me_, type, True);
}

EXPORT void _sortDescArray(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	MergeSortArray(me_, type, False);
}

EXPORT void _sortList(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	MergeSortList(me_, type, True);
}

EXPORT void _sortDescList(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	MergeSortList(me_, type, False);
}

EXPORT S64 _findArray(const void* me_, const U8* type, const void* item, S64 start)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	if (cmp == NULL)
		THROW(EXCPT_INVALID_CMP);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	if (start < -1 || len <= start)
		return -1;
	if (start == -1)
		start = 0;
	U8* ptr = (U8*)me_ + 0x10 + size * start;
	S64 i;
	for (i = start; i < len; i++)
	{
		void* value = NULL;
		memcpy(&value, ptr, size);
		if (cmp(value, item) == 0)
			return i;
		ptr += size;
	}
	return -1;
}

EXPORT Bool _findList(const void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	if (cmp == NULL)
		THROW(EXCPT_INVALID_CMP);
	for (; ; )
	{
		void* cur = *(void**)((U8*)me_ + 0x20);
		if (cur == NULL)
			break;
		void* value = NULL;
		memcpy(&value, (U8*)cur + 0x10, size);
		if (cmp(value, item) == 0)
			return True;
		*(void**)((U8*)me_ + 0x20) = *(void**)((U8*)cur + 0x08);
	}
	return False;
}

EXPORT S64 _findLastArray(const void* me_, const U8* type, const void* item, S64 start)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	if (cmp == NULL)
		THROW(EXCPT_INVALID_CMP);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	if (start < -1 || len <= start)
		return -1;
	if (start == -1)
		start = len - 1;
	U8* ptr = (U8*)me_ + 0x10 + size * start;
	S64 i;
	for (i = start; i >= 0; i--)
	{
		void* value = NULL;
		memcpy(&value, ptr, size);
		if (cmp(value, item) == 0)
			return i;
		ptr -= size;
	}
	return -1;
}

EXPORT Bool _findLastList(const void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	if (cmp == NULL)
		THROW(EXCPT_INVALID_CMP);
	for (; ; )
	{
		void* cur = *(void**)((U8*)me_ + 0x20);
		if (cur == NULL)
			break;
		void* value = NULL;
		memcpy(&value, (U8*)cur + 0x10, size);
		if (cmp(value, item) == 0)
			return True;
		*(void**)((U8*)me_ + 0x20) = *(void**)cur;
	}
	return False;
}

EXPORT S64 _findBin(const void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	if (cmp == NULL)
		THROW(EXCPT_INVALID_CMP);
	{
		S64 len = *(S64*)((U8*)me_ + 0x08);
		U8* ptr = (U8*)me_ + 0x10;
		S64 min = 0;
		S64 max = len - 1;
		while (min <= max)
		{
			S64 mid = (min + max) / 2;
			void* value = NULL;
			S64 cmp2;
			memcpy(&value, ptr + size * (size_t)mid, size);
			cmp2 = cmp(item, value);
			if (cmp2 < 0)
				max = mid - 1;
			else if (cmp2 > 0)
				min = mid + 1;
			else
				return mid;
		}
	}
	return -1;
}

EXPORT void _fill(void* me_, const U8* type, const void* value)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	U8* ptr = (U8*)me_ + 0x10;
	S64 i;
	if (IsRef(type[1]))
	{
		if (value != NULL)
			*(S64*)value += len;
		for (i = 0; i < len; i++)
		{
			{
				void* ptr2 = *(void**)ptr;
				if (ptr2 != NULL)
				{
					(*(S64*)ptr2)--;
					if (*(S64*)ptr2 == 0)
						_freeSet(ptr2, type + 1);
				}
			}
			memcpy(ptr, &value, size);
			ptr += size;
		}
	}
	else
	{
		for (i = 0; i < len; i++)
		{
			memcpy(ptr, &value, size);
			ptr += size;
		}
	}
}

EXPORT void* _min(const void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	if (cmp == NULL)
		THROW(EXCPT_INVALID_CMP);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	U8* ptr = (U8*)me_ + 0x10;
	void* item = NULL;
	S64 i;
	for (i = 0; i < len; i++)
	{
		void* value = NULL;
		memcpy(&value, ptr, size);
		if (i == 0 || cmp(item, value) > 0)
			item = value;
		ptr += size;
	}
	void* result = NULL;
	Copy(&result, type[1], &item);
	return result;
}

EXPORT void* _max(const void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	if (cmp == NULL)
		THROW(EXCPT_INVALID_CMP);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	U8* ptr = (U8*)me_ + 0x10;
	void* item = NULL;
	S64 i;
	for (i = 0; i < len; i++)
	{
		void* value = NULL;
		memcpy(&value, ptr, size);
		if (i == 0 || cmp(item, value) < 0)
			item = value;
		ptr += size;
	}
	void* result = NULL;
	Copy(&result, type[1], &item);
	return result;
}

EXPORT void* _repeat(const void* me_, const U8* type, S64 len)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	Bool is_str = type[1] == TypeId_Char;
	size_t len2 = ((const S64*)me_)[1];
	U8* result = (U8*)AllocMem(0x10 + size * (len2 * (size_t)len + (is_str ? 1 : 0)));
	((S64*)result)[0] = DefaultRefCntOpe;
	((S64*)result)[1] = (S64)len2 * len;
	if (is_str)
		((Char*)(result + 0x10))[(S64)len2 * len] = L'\0';
	U8* ptr = result + 0x10;
	S64 i;
	size_t j;
	if (IsRef(type[1]))
	{
		void** ptr2 = (void**)((const U8*)me_ + 0x10);
		for (j = 0; j < len2; j++)
		{
			*(S64*)*ptr2 += len;
			ptr2++;
		}
	}
	for (i = 0; i < len; i++)
	{
		U8* ptr2 = (U8*)((const U8*)me_ + 0x10);
		for (j = 0; j < len2; j++)
		{
			memcpy(ptr, ptr2, size);
			ptr += size;
			ptr2 += size;
		}
	}
	return result;
}

EXPORT S64 _toInt(const U8* me_, Bool* success)
{
	Char* ptr;
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	const Char* str = (const Char*)(me_ + 0x10);
	S64 value;
	if (str[0] == '0' && str[1] == 'x')
		value = wcstoll(str + 2, &ptr, 16);
	else
		value = wcstoll(str, &ptr, 10);
	*success = *ptr == L'\0';
	return value;
}

EXPORT double _toFloat(const U8* me_, Bool* success)
{
	Char* ptr;
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	double value = wcstod((const Char*)(me_ + 0x10), &ptr);
	*success = *ptr == L'\0';
	return value;
}

EXPORT U64 _toBit64(const U8* me_, Bool* success)
{
	Char* ptr;
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	const Char* str = (const Char*)(me_ + 0x10);
	U64 value;
	if (str[0] == '0' && str[1] == 'x')
		value = wcstoull(str + 2, &ptr, 16);
	else
		value = wcstoull(str, &ptr, 10);
	*success = *ptr == L'\0';
	return value;
}

EXPORT void* _lower(const U8* me_)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len = *(S64*)(me_ + 0x08);
	U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(len + 1));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = len;
	{
		Char* dst = (Char*)(result + 0x10);
		const Char* src = (Char*)(me_ + 0x10);
		while (len > 0)
		{
			if (L'A' <= *src && *src <= L'Z')
				*dst = *src - L'A' + L'a';
			else
				*dst = *src;
			dst++;
			src++;
			len--;
		}
		*dst = L'\0';
	}
	return result;
}

EXPORT void* _upper(const U8* me_)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len = *(S64*)(me_ + 0x08);
	U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(len + 1));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = len;
	{
		Char* dst = (Char*)(result + 0x10);
		const Char* src = (Char*)(me_ + 0x10);
		while (len > 0)
		{
			if (L'a' <= *src && *src <= L'z')
				*dst = *src - L'a' + L'A';
			else
				*dst = *src;
			dst++;
			src++;
			len--;
		}
		*dst = L'\0';
	}
	return result;
}

EXPORT void* _trim(const U8* me_)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len = *(S64*)(me_ + 0x08);
	const Char* first = (Char*)(me_ + 0x10);
	const Char* last = first + len - 1;
	while (len > 0 && IsSpace(*first))
	{
		first++;
		len--;
	}
	while (len > 0 && IsSpace(*last))
	{
		last--;
		len--;
	}
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = len;
		ASSERT(len == last - first + 1);
		memcpy(result + 0x10, first, sizeof(Char) * (size_t)len);
		*(Char*)(result + 0x10 + sizeof(Char) * (size_t)len) = L'\0';
		return result;
	}
}

EXPORT void* _trimLeft(const U8* me_)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len = *(S64*)(me_ + 0x08);
	const Char* first = (Char*)(me_ + 0x10);
	while (len > 0 && IsSpace(*first))
	{
		first++;
		len--;
	}
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = len;
		memcpy(result + 0x10, first, sizeof(Char) * (size_t)len);
		*(Char*)(result + 0x10 + sizeof(Char) * (size_t)len) = L'\0';
		return result;
	}
}

EXPORT void* _trimRight(const U8* me_)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len = *(S64*)(me_ + 0x08);
	const Char* last = (Char*)(me_ + 0x10) + len - 1;
	while (len > 0 && IsSpace(*last))
	{
		last--;
		len--;
	}
	{
		U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(len + 1));
		((S64*)result)[0] = DefaultRefCntFunc;
		((S64*)result)[1] = len;
		memcpy(result + 0x10, me_ + 0x10, sizeof(Char) * (size_t)len);
		*(Char*)(result + 0x10 + sizeof(Char) * (size_t)len) = L'\0';
		return result;
	}
}

EXPORT void* _split(const U8* me_, const U8* delimiter)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(delimiter == NULL, EXCPT_ACCESS_VIOLATION);
	SBinList* top = NULL;
	SBinList* bottom = NULL;
	size_t len = 0;
	const Char* str = (const Char*)(me_ + 0x10);
	size_t delimiter_len = *(const S64*)(delimiter + 0x08);
	const Char* delimiter2 = (const Char*)(delimiter + 0x10);
	const Char* ptr = str;
	Bool end_flag = False;
	U8* result;
	while (!end_flag)
	{
		ptr = wcsstr(ptr, delimiter2);
		if (ptr == NULL)
		{
			ptr = str + wcslen(str);
			end_flag = True;
		}
		{
			SBinList* node = (SBinList*)AllocMem(sizeof(SBinList));
			node->Next = NULL;
			node->Bin = ptr;
			if (top == NULL)
			{
				top = node;
				bottom = node;
			}
			else
			{
				bottom->Next = node;
				bottom = node;
			}
			len++;
		}
		ptr += delimiter_len;
	}
	result = (U8*)AllocMem(0x10 + sizeof(void*) * len);
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = len;
	ASSERT(len != 0);
	{
		SBinList* ptr2 = top;
		const Char* prev = str;
		void** ptr3 = (void**)(result + 0x10);
		while (ptr2 != NULL)
		{
			SBinList* ptr4 = ptr2;
			size_t len2 = (const Char*)ptr4->Bin - prev;
			U8* element = (U8*)AllocMem(0x10 + sizeof(Char) * (len2 + 1));
			((S64*)element)[0] = 1;
			((S64*)element)[1] = len2;
			memcpy(element + 0x10, prev, sizeof(Char) * len2);
			*(Char*)(element + 0x10 + sizeof(Char) * len2) = L'\0';
			*ptr3 = element;
			ptr3++;
			prev = (const Char*)ptr4->Bin + delimiter_len;
			ptr2 = ptr2->Next;
			FreeMem(ptr4);
		}
	}
	return result;
}

EXPORT void* _join(const U8* me_, const U8* delimiter)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(delimiter == NULL, EXCPT_ACCESS_VIOLATION);
	S64 delimiter_len = *(const S64*)(delimiter + 0x08);
	S64 array_len = *(const S64*)(me_ + 0x08);
	if (array_len == 0)
	{
		U8* blank = (U8*)AllocMem(0x10 + sizeof(Char));
		((S64*)blank)[0] = DefaultRefCntFunc;
		((S64*)blank)[1] = 0;
		*(Char*)(blank + 0x10) = L'\0';
		return blank;
	}
	const void** array_ = (const void**)(me_ + 0x10);
	S64 total_len = delimiter_len * (array_len - 1);
	U8* result;
	{
		S64 i;
		for (i = 0; i < array_len; i++)
		{
			S64 child_len = *(const S64*)((const U8*)array_[i] + 0x08);
			total_len += child_len;
		}
	}
	result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(total_len + 1));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = total_len;
	{
		U8* ptr = result + 0x10;
		S64 i;
		for (i = 0; i < array_len; i++)
		{
			S64 child_len = *(const S64*)((const U8*)array_[i] + 0x08);
			memcpy(ptr, (const U8*)array_[i] + 0x10, sizeof(Char) * (size_t)child_len);
			ptr += sizeof(Char) * (size_t)child_len;
			if (i != array_len - 1)
			{
				memcpy(ptr, (const U8*)delimiter + 0x10, sizeof(Char) * (size_t)delimiter_len);
				ptr += sizeof(Char) * (size_t)delimiter_len;
			}
		}
		*(Char*)ptr = L'\0';
	}
	return result;
}

EXPORT void* _replace(const U8* me_, const U8* old, const U8* new_)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(old == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(new_ == NULL, EXCPT_ACCESS_VIOLATION);
	SBinList* top = NULL;
	SBinList* bottom = NULL;
	size_t len = 0;
	size_t str_len = *(const S64*)(me_ + 0x08);
	const Char* str = (const Char*)(me_ + 0x10);
	size_t old_len = *(const S64*)(old + 0x08);
	const Char* old2 = (const Char*)(old + 0x10);
	size_t new_len = *(const S64*)(new_ + 0x08);
	const Char* new2 = (const Char*)(new_ + 0x10);
	const Char* ptr = str;
	Bool end_flag = False;
	U8* result;
	while (!end_flag)
	{
		ptr = wcsstr(ptr, old2);
		if (ptr == NULL || old_len == 0)
		{
			ptr = str + str_len;
			end_flag = True;
		}
		{
			SBinList* node = (SBinList*)AllocMem(sizeof(SBinList));
			node->Next = NULL;
			node->Bin = ptr;
			if (top == NULL)
			{
				top = node;
				bottom = node;
			}
			else
			{
				bottom->Next = node;
				bottom = node;
			}
			len++;
		}
		ptr += old_len;
	}
	size_t result_len = str_len - (len - 1) * old_len + (len - 1) * new_len;
	result = (U8*)AllocMem(0x10 + sizeof(Char) * (result_len + 1));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = result_len;
	{
		SBinList* ptr2 = top;
		const Char* prev = str;
		Char* ptr3 = (Char*)(result + 0x10);
		ptr3[result_len] = L'\0';
		while (ptr2 != NULL)
		{
			SBinList* ptr4 = ptr2;
			size_t len2 = (const Char*)ptr4->Bin - prev;
			memcpy(ptr3, prev, sizeof(Char) * len2);
			ptr3 += len2;
			if (ptr2->Next != NULL)
			{
				memcpy(ptr3, new2, sizeof(Char) * new_len);
				ptr3 += new_len;
			}
			prev = (const Char*)ptr4->Bin + old_len;
			ptr2 = ptr2->Next;
			FreeMem(ptr4);
		}
	}
	return result;
}

EXPORT S64 _findStr(const U8* me_, const U8* pattern, S64 start)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(pattern == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len1 = ((const S64*)me_)[1];
	if (start < -1 || len1 <= start)
		return -1;
	if (start == -1)
		start = 0;
	const Char* result = wcsstr((const Char*)(me_ + 0x10) + start, (const Char*)(pattern + 0x10));
	return result == NULL ? -1 : (S64)(result - (const Char*)(me_ + 0x10));
}

EXPORT S64 _findStrLast(const U8* me_, const U8* pattern, S64 start)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(pattern == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len1 = ((const S64*)me_)[1];
	S64 len2 = ((const S64*)pattern)[1];
	const Char* ptr1 = (const Char*)(me_ + 0x10);
	const Char* ptr2 = (const Char*)(pattern + 0x10);
	S64 i;
	if (start < -1 || len1 <= start)
		return -1;
	if (start == -1 || start > len1 - len2)
		start = len1 - len2;
	for (i = start; i >= 0; i--)
	{
		if (wcsncmp(ptr1 + i, ptr2, (size_t)len2) == 0)
			return i;
	}
	return -1;
}

EXPORT S64 _findStrEx(const U8* me_, const U8* pattern, S64 start, Bool fromLast, Bool ignoreCase, Bool wholeWord)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(pattern == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len1 = ((const S64*)me_)[1];
	S64 len2 = ((const S64*)pattern)[1];
	const Char* ptr1 = (const Char*)(me_ + 0x10);
	const Char* ptr2 = (const Char*)(pattern + 0x10);
	S64 i;
	int(*func)(const Char*, const Char*, size_t) = ignoreCase ? _wcsnicmp : wcsncmp;
	if (start < -1 || len1 <= start)
		return -1;
	if (fromLast)
	{
		if (start == -1 || start > len1 - len2)
			start = len1 - len2;
		for (i = start; i >= 0; i--)
		{
			if (func(ptr1 + i, ptr2, (size_t)len2) == 0)
			{
				if (wholeWord)
				{
					if (i > 0)
					{
						Char c = ptr1[i - 1];
						if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_')
							continue;
					}
					if (i + len2 < len1)
					{
						Char c = ptr1[i + len2];
						if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_')
							continue;
					}
				}
				return i;
			}
		}
	}
	else
	{
		if (start == -1)
			start = 0;
		for (i = start; i <= len1 - len2; i++)
		{
			if (func(ptr1 + i, ptr2, (size_t)len2) == 0)
			{
				if (wholeWord)
				{
					if (i > 0)
					{
						Char c = ptr1[i - 1];
						if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_')
							continue;
					}
					if (i + len2 < len1)
					{
						Char c = ptr1[i + len2];
						if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || L'0' <= c && c <= L'9' || c == L'_')
							continue;
					}
				}
				return i;
			}
		}
	}
	return -1;
}

EXPORT void _addList(void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* node = (U8*)AllocMem(0x10 + GetSize(type[1]));
	Copy(node + 0x10, type[1], &item);
	*(void**)(node + 0x08) = NULL;
	if (*(void**)((U8*)me_ + 0x10) == NULL)
	{
		*(void**)node = NULL;
		*(void**)((U8*)me_ + 0x10) = node;
		*(void**)((U8*)me_ + 0x18) = node;
	}
	else
	{
		*(void**)node = *(void**)((U8*)me_ + 0x18);
		*(void**)((U8*)(*(void**)((U8*)me_ + 0x18)) + 0x08) = node;
		*(void**)((U8*)me_ + 0x18) = node;
	}
	(*(S64*)((U8*)me_ + 0x08))++;
}

EXPORT void _addStack(void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* node = (U8*)AllocMem(0x08 + GetSize(type[1]));
	Copy(node + 0x08, type[1], &item);
	*(void**)node = *(void**)((U8*)me_ + 0x10);
	*(void**)((U8*)me_ + 0x10) = node;
	(*(S64*)((U8*)me_ + 0x08))++;
}

EXPORT void _addQueue(void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* node = (U8*)AllocMem(0x08 + GetSize(type[1]));
	Copy(node + 0x08, type[1], &item);
	*(void**)node = NULL;
	if (*(void**)((U8*)me_ + 0x18) == NULL)
		*(void**)((U8*)me_ + 0x10) = node;
	else
		*(void**)*(void**)((U8*)me_ + 0x18) = node;
	*(void**)((U8*)me_ + 0x18) = node;
	(*(S64*)((U8*)me_ + 0x08))++;
}

EXPORT void _addDict(void* me_, const U8* type, const U8* value_type, const void* key, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	Bool addition;
	U8* child1;
	U8* child2;
	UNUSED(value_type);
	GetDictTypes(type, &child1, &child2);
	THROWDBG(IsRef(*child1) && key == NULL, EXCPT_ACCESS_VIOLATION); // 'key' must not be 'null'.
	*(void**)((U8*)me_ + 0x10) = AddDictRecursion(*(void**)((U8*)me_ + 0x10), key, item, GetCmpFunc(child1), child1, child2, &addition);
	if (addition)
		(*(S64*)((U8*)me_ + 0x08))++;
	*(Bool*)((U8*)*(void**)((U8*)me_ + 0x10) + 0x10) = False;
}

EXPORT void* _getList(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(void**)((U8*)me_ + 0x20) == NULL, EXCPT_ACCESS_VIOLATION);
	void* result = NULL;
	Copy(&result, type[1], (U8*)*(void**)((U8*)me_ + 0x20) + 0x10);
	return result;
}

EXPORT void* _getStack(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* node = *(void**)((U8*)me_ + 0x10);
	THROWDBG(node == NULL, EXCPT_ACCESS_VIOLATION);
	void* result = NULL;
	Copy(&result, type[1], (U8*)node + 0x08);
	*(void**)((U8*)me_ + 0x10) = *(void**)node;
	(*(S64*)((U8*)me_ + 0x08))--;
	if (IsRef(type[1]))
	{
		void* ptr2 = *(void**)((U8*)node + 0x08);
		if (ptr2 != NULL)
		{
			(*(S64*)ptr2)--;
			if (*(S64*)ptr2 == 0)
				_freeSet(ptr2, type + 1);
		}
	}
	FreeMem(node);
	return result;
}

EXPORT void* _getQueue(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* node = *(void**)((U8*)me_ + 0x10);
	THROWDBG(node == NULL, EXCPT_ACCESS_VIOLATION);
	void* result = NULL;
	Copy(&result, type[1], (U8*)node + 0x08);
	*(void**)((U8*)me_ + 0x10) = *(void**)node;
	if (*(void**)node == NULL)
		*(void**)((U8*)me_ + 0x18) = NULL;
	(*(S64*)((U8*)me_ + 0x08))--;
	if (IsRef(type[1]))
	{
		void* ptr2 = *(void**)((U8*)node + 0x08);
		if (ptr2 != NULL)
		{
			(*(S64*)ptr2)--;
			if (*(S64*)ptr2 == 0)
				_freeSet(ptr2, type + 1);
		}
	}
	FreeMem(node);
	return result;
}

EXPORT void* _getDict(void* me_, const U8* type, const void* key, Bool* existed)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	THROWDBG(IsRef(*child1) && key == NULL, EXCPT_ACCESS_VIOLATION); // 'key' must not be 'null'.
	{
		int(*cmp_func)(const void* a, const void* b) = GetCmpFunc(child1);
		const void* node = *(void**)((U8*)me_ + 0x10);
		while (node != NULL)
		{
			int cmp = cmp_func(key, *(void**)((U8*)node + 0x18));
			if (cmp == 0)
			{
				void* result = NULL;
				Copy(&result, *child2, (U8*)node + 0x20);
				*existed = True;
				return result;
			}
			if (cmp < 0)
				node = *(void**)node;
			else
				node = *(void**)((U8*)node + 0x08);
		}
	}
	*existed = False;
	return NULL;
}

EXPORT void* _getOffset(void* me_, const U8* type, S64 offset)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* ptr = *(void**)((U8*)me_ + 0x20);
	S64 i;
	if (offset >= 0)
	{
		for (i = 0; i < offset; i++)
			ptr = *(void**)((U8*)ptr + 0x08);
	}
	else
	{
		for (i = 0; i > offset; i--)
			ptr = *(void**)ptr;
	}
	THROWDBG(ptr == NULL, EXCPT_ACCESS_VIOLATION);
	{
		void* result = NULL;
		Copy(&result, type[1], (U8*)ptr + 0x10);
		return result;
	}
}

EXPORT void _head(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	UNUSED(type);
	*(void**)((U8*)me_ + 0x20) = *(void**)((U8*)me_ + 0x10);
}

EXPORT void _tail(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	UNUSED(type);
	*(void**)((U8*)me_ + 0x20) = *(void**)((U8*)me_ + 0x18);
}

EXPORT void _next(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* ptr = *(void**)((U8*)me_ + 0x20);
	THROWDBG(ptr == NULL, EXCPT_ACCESS_VIOLATION);
	UNUSED(type);
	*(void**)((U8*)me_ + 0x20) = *(void**)((U8*)ptr + 0x08);
}

EXPORT void _prev(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* ptr = *(void**)((U8*)me_ + 0x20);
	THROWDBG(ptr == NULL, EXCPT_ACCESS_VIOLATION);
	UNUSED(type);
	*(void**)((U8*)me_ + 0x20) = *(void**)ptr;
}

EXPORT void _moveOffset(void* me_, const U8* type, S64 offset)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* ptr = *(void**)((U8*)me_ + 0x20);
	S64 i;
	UNUSED(type);
	if (offset >= 0)
	{
		for (i = 0; i < offset; i++)
		{
			if (ptr == NULL)
				break;
			ptr = *(void**)((U8*)ptr + 0x08);
		}
	}
	else
	{
		for (i = 0; i > offset; i--)
		{
			if (ptr == NULL)
				break;
			ptr = *(void**)ptr;
		}
	}
	*(void**)((U8*)me_ + 0x20) = ptr;
}

EXPORT Bool _term(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	UNUSED(type);
	return *(void**)((U8*)me_ + 0x20) == NULL;
}

EXPORT Bool _termOffset(void* me_, const U8* type, S64 offset)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* ptr = *(void**)((U8*)me_ + 0x20);
	S64 i;
	UNUSED(type);
	if (ptr == NULL)
		return True;
	if (offset >= 0)
	{
		for (i = 0; i < offset; i++)
		{
			ptr = *(void**)((U8*)ptr + 0x08);
			if (ptr == NULL)
				return True;
		}
	}
	else
	{
		for (i = 0; i > offset; i--)
		{
			ptr = *(void**)ptr;
			if (ptr == NULL)
				return True;
		}
	}
	return False;
}

EXPORT void _del(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(void**)((U8*)me_ + 0x20) == NULL, EXCPT_ACCESS_VIOLATION);
	{
		void* ptr = *(void**)((U8*)me_ + 0x20);
		void* next = *(void**)((U8*)ptr + 0x08);
		void* prev = *(void**)ptr;
		if (prev == NULL)
			*(void**)((U8*)me_ + 0x10) = next;
		else
			*(void**)((U8*)prev + 0x08) = next;
		if (next == NULL)
			*(void**)((U8*)me_ + 0x18) = prev;
		else
			*(void**)next = prev;
		*(void**)((U8*)me_ + 0x20) = next;
		(*(S64*)((U8*)me_ + 0x08))--;
		if (IsRef(type[1]))
		{
			void* ptr2 = *(void**)((U8*)ptr + 0x10);
			if (ptr2 != NULL)
			{
				(*(S64*)ptr2)--;
				if (*(S64*)ptr2 == 0)
					_freeSet(ptr2, type + 1);
			}
		}
		FreeMem(ptr);
	}
}

EXPORT void _delNext(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(void**)((U8*)me_ + 0x20) == NULL, EXCPT_ACCESS_VIOLATION);
	{
		void* ptr = *(void**)((U8*)me_ + 0x20);
		void* next = *(void**)((U8*)ptr + 0x08);
		THROWDBG(next == NULL, EXCPT_ACCESS_VIOLATION);
		void* next_next = *(void**)((U8*)next + 0x08);
		*(void**)((U8*)ptr + 0x08) = next_next;
		if (next_next == NULL)
			*(void**)((U8*)me_ + 0x18) = ptr;
		else
			*(void**)next_next = ptr;
		(*(S64*)((U8*)me_ + 0x08))--;
		if (IsRef(type[1]))
		{
			void* ptr2 = *(void**)((U8*)next + 0x10);
			if (ptr2 != NULL)
			{
				(*(S64*)ptr2)--;
				if (*(S64*)ptr2 == 0)
					_freeSet(ptr2, type + 1);
			}
		}
		FreeMem(next);
	}
}

EXPORT void _ins(void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(void**)((U8*)me_ + 0x20) == NULL, EXCPT_ACCESS_VIOLATION);
	{
		void* ptr = *(void**)((U8*)me_ + 0x20);
		U8* node = (U8*)AllocMem(0x10 + GetSize(type[1]));
		Copy(node + 0x10, type[1], &item);
		if (*(void**)ptr == NULL)
			*(void**)((U8*)me_ + 0x10) = node;
		else
			*(void**)((U8*)*(void**)ptr + 0x08) = node;
		*(void**)((U8*)node + 0x08) = ptr;
		*(void**)node = *(void**)ptr;
		*(void**)ptr = node;
		(*(S64*)((U8*)me_ + 0x08))++;
	}
}

EXPORT void* _toArray(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	Bool is_str = type[1] == TypeId_Char;
	U8* result = (U8*)AllocMem(0x10 + size * (size_t)(len + (is_str ? 1 : 0)));
	((S64*)result)[0] = 1;
	((S64*)result)[1] = len;
	if (len != 0)
	{
		void* src = *(void**)((U8*)me_ + 0x10);
		U8* dst = result + 0x10;
		while (src != NULL)
		{
			Copy(dst, type[1], (U8*)src + 0x10);
			src = *(void**)((U8*)src + 0x08);
			dst += size;
		}
	}
	if (is_str)
		((Char*)(0x10 + result))[len] = L'\0';
	return result;
}

EXPORT void* _toArrayKey(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	Bool is_str = *child1 == TypeId_Char;
	size_t size = GetSize(*child1);
	U8* result = (U8*)AllocMem(0x10 + size * (size_t)(len + (is_str ? 1 : 0)));
	((S64*)result)[0] = 1;
	((S64*)result)[1] = len;
	if (len != 0)
	{
		U8* ptr = result + 0x10;
		ToArrayKeyDictRecursion(&ptr, size, *(void**)((U8*)me_ + 0x10));
	}
	if (IsRef(*child1))
	{
		S64 i;
		void** ptr = (void**)(result + 0x10);
		for (i = 0; i < len; i++)
		{
			if (ptr[i] != NULL)
				(*(S64*)ptr[i])++;
		}
	}
	if (is_str)
		((Char*)(0x10 + result))[len] = L'\0';
	return result;
}

EXPORT void* _toArrayValue(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	Bool is_str = *child2 == TypeId_Char;
	size_t size = GetSize(*child2);
	U8* result = (U8*)AllocMem(0x10 + size * (size_t)(len + (is_str ? 1 : 0)));
	((S64*)result)[0] = 1;
	((S64*)result)[1] = len;
	if (len != 0)
	{
		U8* ptr = result + 0x10;
		ToArrayValueDictRecursion(&ptr, size, *(void**)((U8*)me_ + 0x10));
	}
	if (IsRef(*child2))
	{
		S64 i;
		void** ptr = (void**)(result + 0x10);
		for (i = 0; i < len; i++)
		{
			if (ptr[i] != NULL)
				(*(S64*)ptr[i])++;
		}
	}
	if (is_str)
		((Char*)(0x10 + result))[len] = L'\0';
	return result;
}

EXPORT void* _peek(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* node = *(void**)((U8*)me_ + 0x10);
	THROWDBG(node == NULL, EXCPT_ACCESS_VIOLATION);
	void* result = NULL;
	Copy(&result, type[1], (U8*)node + 0x08);
	return result;
}

EXPORT Bool _exist(void* me_, const U8* type, const void* key)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	THROWDBG(IsRef(*child1) && key == NULL, EXCPT_ACCESS_VIOLATION); // 'key' must not be 'null'.
	{
		int(*cmp_func)(const void* a, const void* b) = GetCmpFunc(child1);
		const void* node = *(void**)((U8*)me_ + 0x10);
		while (node != NULL)
		{
			int cmp = cmp_func(key, *(void**)((U8*)node + 0x18));
			if (cmp == 0)
				return True;
			if (cmp < 0)
				node = *(void**)node;
			else
				node = *(void**)((U8*)node + 0x08);
		}
	}
	return False;
}

EXPORT Bool _forEach(void* me_, const U8* type, const void* callback, void* data)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(callback == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	{
		void* ptr = *(void**)((U8*)me_ + 0x10);
		if (ptr == NULL)
			return True;
		return ForEachRecursion(ptr, child1, child2, callback, data);
	}
}

EXPORT void _delDict(void* me_, const U8* type, const void* key)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	THROWDBG(IsRef(*child1) && key == NULL, EXCPT_ACCESS_VIOLATION); // 'key' must not be 'null'.
	{
		Bool deleted = False;
		Bool balanced = False;
		*(void**)((U8*)me_ + 0x10) = DelDictRecursion(*(void**)((U8*)me_ + 0x10), key, GetCmpFunc(child1), child1, child2, &deleted, &balanced);
		if (deleted)
			(*(S64*)((U8*)me_ + 0x08))--;
	}
}

EXPORT S64 _idx(void* me_, const U8* type)
{
	UNUSED(type);
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* cur = *(void**)((U8*)me_ + 0x20);
	void* ptr = *(void**)((U8*)me_ + 0x10);
	S64 idx = 0;
	if (cur == NULL)
		return -1;
	while (ptr != NULL)
	{
		if (ptr == cur)
			return idx;
		idx++;
		ptr = *(void**)((U8*)ptr + 0x08);
	}
	return -1;
}

EXPORT SClass* _getPtr(void* me_, const U8* type, SClass* me2)
{
	SListPtr* ptr = (SListPtr*)me2;
	ptr->Ptr = *(void**)((U8*)me_ + 0x20);
	return me2;
}

EXPORT void _setPtr(void* me_, const U8* type, SClass* ptr)
{
	SListPtr* ptr2 = (SListPtr*)ptr;
	*(void**)((U8*)me_ + 0x20) = ptr2->Ptr;
}

static Bool IsRef(U8 type)
{
	return type == TypeId_Array || type == TypeId_List || type == TypeId_Stack || type == TypeId_Queue || type == TypeId_Dict || type == TypeId_Class;
}

static size_t GetSize(U8 type)
{
	switch (type)
	{
		case TypeId_Int: return 8;
		case TypeId_Float: return 8;
		case TypeId_Char: return 2;
		case TypeId_Bool: return 1;
		case TypeId_Bit8: return 1;
		case TypeId_Bit16: return 2;
		case TypeId_Bit32: return 4;
		case TypeId_Bit64: return 8;
		case TypeId_Array: return 8;
		case TypeId_List: return 8;
		case TypeId_Stack: return 8;
		case TypeId_Queue: return 8;
		case TypeId_Dict: return 8;
		case TypeId_Func: return 8;
		case TypeId_Enum: return 8;
		case TypeId_Class: return 8;
	}
	ASSERT(False);
	return 0;
}

static S64 Add(S64 a, S64 b)
{
#if defined(DBG)
	if (AddAsm(&a, b))
		THROW(EXCPT_DBG_INT_OVERFLOW);
	return a;
#else
	return a + b;
#endif
}

static S64 Sub(S64 a, S64 b)
{
#if defined(DBG)
	if (SubAsm(&a, b))
		THROW(EXCPT_DBG_INT_OVERFLOW);
	return a;
#else
	return a - b;
#endif
}

static S64 Mul(S64 a, S64 b)
{
#if defined(DBG)
	if (MulAsm(&a, b))
		THROW(EXCPT_DBG_INT_OVERFLOW);
	return a;
#else
	return a * b;
#endif
}

static void GetDictTypes(const U8* type, const U8** child1, const U8** child2)
{
	const U8* type2 = type;
	type2++;
	*child1 = type2;
	GetDictTypesRecursion(&type2);
	type2++;
	*child2 = type2;
}

static void GetDictTypesRecursion(const U8** type)
{
	switch (**type)
	{
		case TypeId_Int:
		case TypeId_Float:
		case TypeId_Char:
		case TypeId_Bool:
		case TypeId_Bit8:
		case TypeId_Bit16:
		case TypeId_Bit32:
		case TypeId_Bit64:
		case TypeId_Func:
		case TypeId_Enum:
		case TypeId_Class:
			break;
		case TypeId_Array:
		case TypeId_List:
		case TypeId_Stack:
		case TypeId_Queue:
			(*type)++;
			GetDictTypesRecursion(type);
			break;
		case TypeId_Dict:
			(*type)++;
			GetDictTypesRecursion(type);
			(*type)++;
			GetDictTypesRecursion(type);
			(*type)++;
			break;
		default:
			ASSERT(False);
	}
}

static void FreeDictRecursion(void* ptr, const U8* child1, const U8* child2)
{
	if (*(void**)ptr != NULL)
		FreeDictRecursion(*(void**)ptr, child1, child2);
	if (*(void**)((U8*)ptr + 0x08) != NULL)
		FreeDictRecursion(*(void**)((U8*)ptr + 0x08), child1, child2);
	if (IsRef(*child1))
	{
		void* ptr2 = *(void**)((U8*)ptr + 0x18);
		if (ptr2 != NULL)
		{
			(*(S64*)ptr2)--;
			if (*(S64*)ptr2 == 0)
				_freeSet(ptr2, child1);
		}
	}
	if (IsRef(*child2))
	{
		void* ptr2 = *(void**)((U8*)ptr + 0x20);
		if (ptr2 != NULL)
		{
			(*(S64*)ptr2)--;
			if (*(S64*)ptr2 == 0)
				_freeSet(ptr2, child2);
		}
	}
	FreeMem(ptr);
}

static Bool IsStr(const U8* type)
{
	return type[0] == TypeId_Array && type[1] == TypeId_Char;
}

static Bool IsSpace(Char c)
{
	return 0x09 <= c && c <= 0x0d || c == L' ' || c == 0xa0;
}

static void Copy(void* dst, U8 type, const void* src)
{
	switch (GetSize(type))
	{
		case 1: *(U8*)dst = *(const U8*)src; break;
		case 2: *(U16*)dst = *(const U16*)src; break;
		case 4: *(U32*)dst = *(const U32*)src; break;
		case 8: *(U64*)dst = *(const U64*)src; break;
		default:
			ASSERT(False);
			break;
	}
	if (IsRef(type) && *(void**)dst != NULL)
		(*(S64*)*(void**)dst)++;
}

static void* AddDictRecursion(void* node, const void* key, const void* item, int cmp_func(const void* a, const void* b), U8* key_type, U8* item_type, Bool* addition)
{
	if (node == NULL)
	{
		void* n = AllocMem(0x20 + GetSize(*item_type));
		*(void**)((U8*)n + 0x18) = NULL;
		Copy((U8*)n + 0x18, *key_type, &key);
		Copy((U8*)n + 0x20, *item_type, &item);
		*(U64*)((U8*)n + 0x10) = (U64)True;
		*(void**)n = NULL;
		*(void**)((U8*)n + 0x08) = NULL;
		*addition = True;
		return n;
	}
	{
		int cmp = cmp_func(key, *(void**)((U8*)node + 0x18));
		if (cmp == 0)
		{
			void** ptr = (void**)((U8*)node + 0x20);
			if (IsRef(*item_type) && *ptr != NULL)
			{
				if (item != NULL)
					(*(S64*)item)++;
				(*(S64*)*ptr)--;
				if (*(S64*)*ptr == 0)
					_freeSet(*ptr, item_type);
			}
			*ptr = (void*)item;
			*addition = False;
			return node;
		}
		if (cmp < 0)
			*(void**)node = AddDictRecursion(*(void**)node, key, item, cmp_func, key_type, item_type, addition);
		else
			*(void**)((U8*)node + 0x08) = AddDictRecursion(*(void**)((U8*)node + 0x08), key, item, cmp_func, key_type, item_type, addition);
	}
	if (*(void**)((U8*)node + 0x08) != NULL && *(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10))
		node = DictRotateLeft(node);
	if (*(void**)node != NULL && *(Bool*)((U8*)*(void**)node + 0x10) && *(void**)*(void**)node != NULL && *(Bool*)((U8*)*(void**)*(void**)node + 0x10))
	{
		node = DictRotateRight(node);
		*(Bool*)((U8*)node + 0x10) = True;
		*(Bool*)((U8*)*(void**)node + 0x10) = False;
		*(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10) = False;
	}
	// '*addition' should have been set so far.
	return node;
}

static void* DelDictRecursion(void* node, const void* key, int cmp_func(const void* a, const void* b), U8* key_type, U8* item_type, Bool* deleted, Bool* balanced)
{
	if (node == NULL)
	{
		*balanced = True;
		return NULL;
	}
	int cmp = cmp_func(key, *(void**)((U8*)node + 0x18));
	if (cmp == 0)
	{
		if (*(void**)node == NULL && *(void**)((U8*)node + 0x08) == NULL)
		{
			*balanced = *(Bool*)((U8*)node + 0x10);

			if (IsRef(*key_type))
			{
				void* ptr2 = *(void**)((U8*)node + 0x18);
				if (ptr2 != NULL)
				{
					(*(S64*)ptr2)--;
					if (*(S64*)ptr2 == 0)
						_freeSet(ptr2, key_type);
				}
			}
			if (IsRef(*item_type))
			{
				void* ptr2 = *(void**)((U8*)node + 0x20);
				if (ptr2 != NULL)
				{
					(*(S64*)ptr2)--;
					if (*(S64*)ptr2 == 0)
						_freeSet(ptr2, item_type);
				}
			}
			FreeMem(node);
			*deleted = True;

			return NULL;
		}
		if (*(void**)((U8*)node + 0x08) == NULL)
		{
			*(Bool*)((U8*)*(void**)node + 0x10) = False;
			*balanced = True;
			void* result = *(void**)node;

			if (IsRef(*key_type))
			{
				void* ptr2 = *(void**)((U8*)node + 0x18);
				if (ptr2 != NULL)
				{
					(*(S64*)ptr2)--;
					if (*(S64*)ptr2 == 0)
						_freeSet(ptr2, key_type);
				}
			}
			if (IsRef(*item_type))
			{
				void* ptr2 = *(void**)((U8*)node + 0x20);
				if (ptr2 != NULL)
				{
					(*(S64*)ptr2)--;
					if (*(S64*)ptr2 == 0)
						_freeSet(ptr2, item_type);
				}
			}
			FreeMem(node);
			*deleted = True;

			return result;
		}
		{
			void* ptr = *(void**)((U8*)node + 0x08);
			while (*(void**)ptr != NULL)
				ptr = *(void**)ptr;
			*(void**)((U8*)node + 0x18) = NULL;
			ASSERT(node != ptr);
			if (IsRef(*key_type))
			{
				void* ptr2 = *(void**)((U8*)node + 0x18);
				if (ptr2 != NULL)
				{
					(*(S64*)ptr2)--;
					if (*(S64*)ptr2 == 0)
						_freeSet(ptr2, key_type);
				}
			}
			if (IsRef(*item_type))
			{
				void* ptr2 = *(void**)((U8*)node + 0x20);
				if (ptr2 != NULL)
				{
					(*(S64*)ptr2)--;
					if (*(S64*)ptr2 == 0)
						_freeSet(ptr2, item_type);
				}
			}
			Copy((U8*)node + 0x18, *key_type, (U8*)ptr + 0x18);
			Copy((U8*)node + 0x20, *item_type, (U8*)ptr + 0x20);
		}
		*(void**)((U8*)node + 0x08) = DelDictRecursion(*(void**)((U8*)node + 0x08), *(void**)((U8*)node + 0x18), cmp_func, key_type, item_type, deleted, balanced);
		return DelDictBalanceRightRecursion(node, balanced);
	}
	if (cmp < 0)
	{
		*(void**)node = DelDictRecursion(*(void**)node, key, cmp_func, key_type, item_type, deleted, balanced);
		return DelDictBalanceLeftRecursion(node, balanced);
	}
	*(void**)((U8*)node + 0x08) = DelDictRecursion(*(void**)((U8*)node + 0x08), key, cmp_func, key_type, item_type, deleted, balanced);
	return DelDictBalanceRightRecursion(node, balanced);
}

static void* DelDictBalanceLeftRecursion(void* node, Bool* balanced)
{
	if (balanced)
		return node;
	if (*(void**)((U8*)node + 0x08) != NULL && *(void**)*(void**)((U8*)node + 0x08) != NULL && !*(Bool*)((U8*)*(void**)*(void**)((U8*)node + 0x08) + 0x10))
	{
		node = DictRotateLeft(node);
		if (!*(Bool*)((U8*)node + 0x10))
			return node;
		*(Bool*)((U8*)node + 0x10) = False;
	}
	else
	{
		*(void**)((U8*)node + 0x08) = DictRotateRight(*(void**)((U8*)node + 0x08));
		node = DictRotateLeft(node);
		*(Bool*)((U8*)*(void**)node + 0x10) = False;
		*(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10) = False;
	}
	*balanced = True;
	return node;
}

static void* DelDictBalanceRightRecursion(void* node, Bool* balanced)
{
	if (balanced)
		return node;
	if (*(void**)node != NULL && *(void**)*(void**)node != NULL && !*(Bool*)((U8*)*(void**)*(void**)node + 0x10))
	{
		if (!*(Bool*)((U8*)*(void**)node + 0x10))
		{
			*(Bool*)((U8*)*(void**)node + 0x10) = True;
			if (!*(Bool*)((U8*)node + 0x10))
				return node;
			*(Bool*)((U8*)node + 0x10) = False;
		}
		else if (*(void**)node != NULL && *(void**)((U8*)*(void**)node + 0x08) != NULL && *(void**)*(void**)((U8*)*(void**)node + 0x08) != NULL && !*(Bool*)((U8*)*(void**)*(void**)((U8*)*(void**)node + 0x08) + 0x10))
		{
			node = DictRotateRight(node);
			*(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10) = False;
			*(Bool*)((U8*)*(void**)*(void**)((U8*)node + 0x08) + 0x10) = True;
		}
		else
		{
			*(void**)node = DictRotateLeft(*(void**)node);
			node = DictRotateRight(node);
			*(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10) = False;
			*(Bool*)((U8*)*(void**)((U8*)*(void**)node + 0x08) + 0x10) = False;
		}
	}
	else
	{
		node = DictRotateRight(node);
		*(Bool*)((U8*)*(void**)node + 0x10) = False;
		*(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10) = False;
	}
	*balanced = True;
	return node;
}

static void* DictRotateLeft(void* node)
{
	void* r = *(void**)((U8*)node + 0x08);
	*(void**)((U8*)node + 0x08) = *(void**)r;
	*(void**)r = node;
	*(Bool*)((U8*)r + 0x10) = *(Bool*)((U8*)node + 0x10);
	*(Bool*)((U8*)node + 0x10) = True;
	return r;
}

static void* DictRotateRight(void* node)
{
	void* l = *(void**)node;
	*(void**)node = *(void**)((U8*)l + 0x08);
	*(void**)((U8*)l + 0x08) = node;
	*(Bool*)((U8*)l + 0x10) = *(Bool*)((U8*)node + 0x10);
	*(Bool*)((U8*)node + 0x10) = True;
	return l;
}

static void* CopyDictRecursion(void* node, U8* key_type, U8* item_type)
{
	U8* result = (U8*)AllocMem(0x20 + GetSize(*item_type));
	if (*(void**)node == NULL)
		*(void**)result = NULL;
	else
		*(void**)result = CopyDictRecursion(*(void**)node, key_type, item_type);
	if (*(void**)((U8*)node + 0x08) == NULL)
		*(void**)(result + 0x08) = NULL;
	else
		*(void**)(result + 0x08) = CopyDictRecursion(*(void**)((U8*)node + 0x08), key_type, item_type);
	*(void**)(result + 0x10) = *(void**)((U8*)node + 0x10);
	if (IsRef(*key_type))
	{
		if (*(void**)((U8*)node + 0x18) == NULL)
			*(void**)(result + 0x18) = NULL;
		else
			*(void**)(result + 0x18) = _copy(*(void**)((U8*)node + 0x18), key_type);
	}
	else
	{
		*(void**)(result + 0x18) = NULL;
		memcpy(result + 0x18, (U8*)node + 0x18, GetSize(*key_type));
	}
	if (IsRef(*item_type))
	{
		if (*(void**)((U8*)node + 0x20) == NULL)
			*(void**)(result + 0x20) = NULL;
		else
			*(void**)(result + 0x20) = _copy(*(void**)((U8*)node + 0x20), item_type);
	}
	else
		memcpy(result + 0x20, (U8*)node + 0x20, GetSize(*item_type));
	return result;
}

static void ToBinDictRecursion(void*** buf, void* node, U8* key_type, U8* item_type)
{
	{
		void* key = NULL;
		memcpy(&key, (U8*)node + 0x18, GetSize(*key_type));
		**buf = _toBin(key, key_type);
		(*buf)++;
	}
	{
		void* value = NULL;
		memcpy(&value, (U8*)node + 0x20, GetSize(*item_type));
		**buf = _toBin(value, item_type);
		(*buf)++;
	}
	{
		U8* info = (U8*)AllocMem(0x11);
		((S64*)info)[0] = DefaultRefCntOpe;
		((S64*)info)[1] = 1;
		{
			U8 flag = 0;
			if (*(void**)node != NULL)
				flag |= 0x01;
			if (*(void**)((U8*)node + 0x08) != NULL)
				flag |= 0x02;
			if ((Bool)*(S64*)((U8*)node + 0x10))
				flag |= 0x04;
			*(info + 0x10) = flag;
		}
		**buf = info;
		(*buf)++;
	}
	if (*(void**)node != NULL)
		ToBinDictRecursion(buf, *(void**)node, key_type, item_type);
	if (*(void**)((U8*)node + 0x08) != NULL)
		ToBinDictRecursion(buf, *(void**)((U8*)node + 0x08), key_type, item_type);
}

static void* FromBinDictRecursion(const U8* key_type, const U8* item_type, const U8* bin, S64* idx, const void* type_class)
{
	U8* node = (U8*)AllocMem(0x20 + GetSize(*item_type));
	{
		const void* type[2];
		void* key;
		type[0] = key_type;
		type[1] = type_class;
		key = _fromBin(bin, type, idx);
		*(void**)(node + 0x18) = NULL;
		memcpy(node + 0x18, &key, GetSize(*key_type));
	}
	{
		const void* type[2];
		void* value;
		type[0] = item_type;
		type[1] = type_class;
		value = _fromBin(bin, type, idx);
		memcpy(node + 0x20, &value, GetSize(*item_type));
	}
	{
		U8 info = *(bin + 0x10 + *idx);
		(*idx)++;
		if ((info & 0x01) != 0)
			*(void**)node = FromBinDictRecursion(key_type, item_type, bin, idx, type_class);
		else
			*(void**)node = NULL;
		if ((info & 0x02) != 0)
			*(void**)(node + 0x08) = FromBinDictRecursion(key_type, item_type, bin, idx, type_class);
		else
			*(void**)(node + 0x08) = NULL;
		*(S64*)(node + 0x10) = (info & 0x04) != 0 ? 1 : 0;
	}
	return node;
}

static void ToArrayKeyDictRecursion(U8** buf, size_t key_size, void* node)
{
	if (*(void**)node != NULL)
		ToArrayKeyDictRecursion(buf, key_size, *(void**)node);
	memcpy(*buf, (U8*)node + 0x18, key_size);
	(*buf) += key_size;
	if (*(void**)((U8*)node + 0x08) != NULL)
		ToArrayKeyDictRecursion(buf, key_size, *(void**)((U8*)node + 0x08));
}

static void ToArrayValueDictRecursion(U8** buf, size_t value_size, void* node)
{
	if (*(void**)node != NULL)
		ToArrayValueDictRecursion(buf, value_size, *(void**)node);
	memcpy(*buf, (U8*)node + 0x20, value_size);
	(*buf) += value_size;
	if (*(void**)((U8*)node + 0x08) != NULL)
		ToArrayValueDictRecursion(buf, value_size, *(void**)((U8*)node + 0x08));
}

static int(*GetCmpFunc(const U8* type))(const void* a, const void* b)
{
	switch (*type)
	{
		case TypeId_Int: return CmpInt;
		case TypeId_Float: return CmpFloat;
		case TypeId_Char: return CmpChar;
		case TypeId_Bit8: return CmpBit8;
		case TypeId_Bit16: return CmpBit16;
		case TypeId_Bit32: return CmpBit32;
		case TypeId_Bit64: return CmpBit64;
		case TypeId_Array:
			if (type[1] != TypeId_Char)
				return NULL;
			return CmpStr;
		case TypeId_Enum: return CmpInt;
		case TypeId_Class: return CmpClassAsm;
		default:
			return NULL;
	}
}

static int CmpInt(const void* a, const void* b)
{
	S64 a2 = *(S64*)&a;
	S64 b2 = *(S64*)&b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpFloat(const void* a, const void* b)
{
	double a2 = *(double*)&a;
	double b2 = *(double*)&b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpChar(const void* a, const void* b)
{
	Char a2 = *(Char*)&a;
	Char b2 = *(Char*)&b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpBit8(const void* a, const void* b)
{
	U8 a2 = *(U8*)&a;
	U8 b2 = *(U8*)&b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpBit16(const void* a, const void* b)
{
	U16 a2 = *(U16*)&a;
	U16 b2 = *(U16*)&b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpBit32(const void* a, const void* b)
{
	U32 a2 = *(U32*)&a;
	U32 b2 = *(U32*)&b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpBit64(const void* a, const void* b)
{
	U64 a2 = *(U64*)&a;
	U64 b2 = *(U64*)&b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpStr(const void* a, const void* b)
{
	const Char* a2 = (const Char*)((U8*)a + 0x10);
	const Char* b2 = (const Char*)((U8*)b + 0x10);
	return wcscmp(a2, b2);
}

static void* CatBin(int num, void** bins)
{
	S64 len = 0;
	int i;
	for (i = 0; i < num; i++)
		len += *(S64*)((U8*)bins[i] + 0x08);
	{
		U8* result = (U8*)AllocMem(0x18 + (size_t)len);
		((S64*)result)[0] = DefaultRefCntOpe;
		((S64*)result)[1] = 0x08 + len;
		((S64*)result)[2] = (S64)num;
		{
			U8* ptr = result + 0x18;
			for (i = 0; i < num; i++)
			{
				size_t size = (size_t)*(S64*)((U8*)bins[i] + 0x08);
				memcpy(ptr, (U8*)bins[i] + 0x10, size);
				ptr += size;
				ASSERT(*(S64*)bins[i] == DefaultRefCntOpe);
				FreeMem(bins[i]);
			}
		}
		FreeMem(bins);
		return result;
	}
}

static void MergeSortArray(void* me_, const U8* type, Bool asc)
{
	size_t size = GetSize(type[1]);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	U8* a = (U8*)me_ + 0x10;
	U8* b = (U8*)AllocMem((size_t)len * size);
	U8* a2 = a;
	U8* b2 = b;
	S64 n = 1;
	S64 i, j;
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	if (cmp == NULL)
		THROW(EXCPT_INVALID_CMP);
	while (n < len)
	{
		for (i = 0; i < len; i += n * 2)
		{
			S64 p = i;
			S64 q = i + n;
			for (j = i; j < i + n * 2; j++)
			{
				if (p >= i + n || p >= len)
				{
					if (q >= len)
						break;
					memcpy(b2 + j * size, a2 + q * size, size);
					q++;
				}
				else if (q >= i + n * 2 || q >= len)
				{
					memcpy(b2 + j * size, a2 + p * size, size);
					p++;
				}
				else
				{
					int cmp_result;
					void* value_p = NULL;
					void* value_q = NULL;
					memcpy(&value_p, a2 + p * size, size);
					memcpy(&value_q, a2 + q * size, size);
					if (asc)
						cmp_result = cmp(value_p, value_q);
					else
						cmp_result = -cmp(value_p, value_q);
					if (cmp_result <= 0)
					{
						memcpy(b2 + j * size, &value_p, size);
						p++;
					}
					else
					{
						memcpy(b2 + j * size, &value_q, size);
						q++;
					}
				}
			}
		}
		{
			U8* tmp = a2;
			a2 = b2;
			b2 = tmp;
		}
		n *= 2;
	}
	if (a != a2)
		memcpy(a, b, (size_t)len * size);
	FreeMem(b);
}

static void MergeSortList(void* me_, const U8* type, Bool asc)
{
	size_t size = GetSize(type[1]);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	void** a = (void**)AllocMem((size_t)len * 8);
	void** b = (void**)AllocMem((size_t)len * 8);
	void** a2 = a;
	void** b2 = b;
	S64 n = 1;
	S64 i, j;
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	if (cmp == NULL)
		THROW(EXCPT_INVALID_CMP);
	{
		void* ptr = *(void**)((U8*)me_ + 0x10);
		for (i = 0; i < len; i++)
		{
			a[i] = (U8*)ptr + 0x10;
			b[i] = NULL;
			ptr = *(void**)((U8*)ptr + 0x08);
		}
	}
	while (n < len)
	{
		for (i = 0; i < len; i += n * 2)
		{
			S64 p = i;
			S64 q = i + n;
			for (j = i; j < i + n * 2; j++)
			{
				if (p >= i + n || p >= len)
				{
					if (q >= len)
						break;
					b2[j] = a2[q];
					q++;
				}
				else if (q >= i + n * 2 || q >= len)
				{
					b2[j] = a2[p];
					p++;
				}
				else
				{
					int cmp_result;
					void* value_p = NULL;
					void* value_q = NULL;
					memcpy(&value_p, a2[p], size);
					memcpy(&value_q, a2[q], size);
					if (asc)
						cmp_result = cmp(value_p, value_q);
					else
						cmp_result = -cmp(value_p, value_q);
					if (cmp_result <= 0)
					{
						b2[j] = a2[p];
						p++;
					}
					else
					{
						b2[j] = a2[q];
						q++;
					}
				}
			}
		}
		{
			void** tmp = a2;
			a2 = b2;
			b2 = tmp;
		}
		n *= 2;
	}
	{
		void* ptr = *(void**)((U8*)me_ + 0x10);
		for (i = 0; i < len; i++)
			memcpy(b2 + i, a2[i], size);
		for (i = 0; i < len; i++)
		{
			memcpy((U8*)ptr + 0x10, b2 + i, size);
			ptr = *(void**)((U8*)ptr + 0x08);
		}
	}
	FreeMem(a);
	FreeMem(b);
}

static Bool ForEachRecursion(void* ptr, const U8* child1, const U8* child2, const void* callback, void* data)
{
	if (*(void**)ptr != NULL)
	{
		if (!ForEachRecursion(*(void**)ptr, child1, child2, callback, data))
			return False;
	}
	{
		void* arg1 = *(void**)((U8*)ptr + 0x18);
		void* arg2 = *(void**)((U8*)ptr + 0x20);
		if (IsRef(*child1) && arg1 != NULL)
			(*(S64*)arg1)++;
		if (IsRef(*child2) && arg2 != NULL)
			(*(S64*)arg2)++;
		if (data != NULL)
			(*(S64*)data)++;
		if (!(Bool)(U64)Call3Asm(arg1, arg2, data, (void*)callback))
			return False;
	}
	if (*(void**)((U8*)ptr + 0x08) != NULL)
	{
		if (!ForEachRecursion(*(void**)((U8*)ptr + 0x08), child1, child2, callback, data))
			return False;
	}
	return True;
}
