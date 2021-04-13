#include "kuin.h"

#pragma comment(lib, "winmm.lib")

#include <locale.h>
#include <Shlobj.h> // 'SHGetSpecialFolderPath'
#include <time.h>

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
	TypeId_Ref = 0x80,
	TypeId_Array = 0x81,
	TypeId_List = 0x82,
	TypeId_Stack = 0x83,
	TypeId_Queue = 0x84,
	TypeId_Dict = 0x85,
	TypeId_Class = 0x86,
} ETypeId;

typedef struct SListPtr
{
	SClass Class;
	void* Ptr;
} SListPtr;

static Bool IsRef(U8 type);
static size_t GetSize(U8 type);
static void GetDictTypes(const U8* type, const U8** child1, const U8** child2);
static void GetDictTypesRecursion(const U8** type);
static void FreeDictRecursion(void* ptr, const U8* child1, const U8* child2);
static Bool IsStr(const U8* type);
static void* CopyDictRecursion(void* node, U8* key_type, U8* item_type);
static void ToBinDictRecursion(void*** buf, void* node, U8* key_type, U8* item_type, const void* root);
static void* CatBin(S64 num, S64 writing_num, void** bins);
static void* AddDictRecursion(void* node, const void* key, const void* item, int cmp_func(const void* a, const void* b), U8* key_type, U8* item_type, Bool* addition);
static int(*GetCmpFunc(const U8* type))(const void* a, const void* b);
static void Copy(void* dst, U8 type, const void* src);
static void* DelDictRecursion(void* node, const void* key, int cmp_func(const void* a, const void* b), U8* key_type, U8* item_type, Bool* deleted);
static Bool ForEachRecursion(void* ptr, const U8* child1, const U8* child2, const void* callback, void* data);
static void ToArrayKeyDictRecursion(U8** buf, size_t key_size, void* node);
static void ToArrayValueDictRecursion(U8** buf, size_t value_size, void* node);
static int CmpInt(const void* a, const void* b);
static int CmpFloat(const void* a, const void* b);
static int CmpChar(const void* a, const void* b);
static int CmpBit8(const void* a, const void* b);
static int CmpBit16(const void* a, const void* b);
static int CmpBit32(const void* a, const void* b);
static int CmpBit64(const void* a, const void* b);
static int CmpStr(const void* a, const void* b);
static void* DictRotateLeft(void* node);
static void* DictRotateRight(void* node);
static void DictFlip(void* node);
static void* DictFixUp(void* node);
static void* DictMoveRedLeft(void* node);
static void* DictMoveRedRight(void* node);
static void* DictDelMinRec(void* node, U8* key_type, U8* item_type);

EXPORT void _init(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);

	setlocale(LC_ALL, "");

	// Initialize the COM library and the timer.
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) // 'STA'
	{
		// Maybe the COM library is already initialized in some way.
		ASSERT(False);
	}
	timeBeginPeriod(1);
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
		text = L"User defined exception.";
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
			case 0xe9170000: text = L"Assertion failed."; break;
			case 0xe9170001: text = L"Class cast failed."; break;
			case 0xe9170002: text = L"Array index out of range."; break;
			case 0xe9170004: text = L"Invalid comparison."; break;
			case 0xe9170006: text = L"Argument outside the domain."; break;
			case 0xe9170007: text = L"File open failed."; break;
			case 0xe9170008: text = L"Invalid data format."; break;
			case 0xe9170009: text = L"Device initialization failed."; break;
			case 0xe917000a: text = L"Inoperable state."; break;
		}
	}
	swprintf(str, 1024, L"An exception '0x%08X' occurred.\r\n\r\n> %s", (U32)excpt, text);
	MessageBox(0, str, NULL, 0);
#else
	UNUSED(excpt);
#endif
}

#if defined(DBG)
EXPORT void _pause(void)
{
	system("pause");
}
#endif

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
				void* ptr2 = *(void**)((U8*)ptr + 0x10);
				if (ptr2 != NULL)
					FreeDictRecursion(ptr2, child1, child2);
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
				return result;
			}
		case TypeId_Dict:
			{
				U8* child1;
				U8* child2;
				GetDictTypes(type, &child1, &child2);
				U8* result = (U8*)AllocMem(0x18);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = *(S64*)((U8*)me_ + 0x08);
				((S64*)result)[2] = 0;
				if (*(void**)((U8*)me_ + 0x10) != NULL)
					*(void**)(result + 0x10) = CopyDictRecursion(*(void**)((U8*)me_ + 0x10), child1, child2);
				return result;
			}
		default:
			ASSERT(*type == TypeId_Class);
			return CopyClassAsm(me_);
	}
}

EXPORT void* _toBin(const void* me_, const U8* type, const void* root)
{
	if (IsRef(*type) && me_ == NULL || *type == TypeId_Func)
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
					bins[i] = _toBin(value, type + 1, root);
					ptr += size;
				}
				return CatBin(len, len, bins);
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
					bins[i + 1] = _toBin(value, type + 1, root);
					if (ptr_cur == ptr)
						idx = i;
					ptr = *(void**)((U8*)ptr + 0x08);
				}
				void* bin = AllocMem(0x10 + 0x08);
				((S64*)bin)[0] = DefaultRefCntOpe;
				((S64*)bin)[1] = 0x08;
				((S64*)bin)[2] = idx;
				bins[0] = bin;
				return CatBin(len + 1, len, bins);
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
					bins[i] = _toBin(value, type + 1, root);
					ptr = *(void**)ptr;
				}
				return CatBin(len, len, bins);
			}
		case TypeId_Dict:
			{
				U8* child1;
				U8* child2;
				GetDictTypes(type, &child1, &child2);
				S64 len = *(S64*)((U8*)me_ + 0x08);
				void** bins = (void**)AllocMem(sizeof(void*) * (size_t)len * 2); // 'key' + 'value' per node.
				void** ptr = bins;
				if (len != 0)
					ToBinDictRecursion(&ptr, *(void**)((U8*)me_ + 0x10), child1, child2, root);
				return CatBin(len * 2, len, bins);
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
			{
				ASSERT(*type == TypeId_Class);
				ASSERT(root != NULL);
				void** bins = (void**)AllocMem(sizeof(void*) * 2);
				{
					U8* ptr = (U8*)AllocMem(0x18);
					((S64*)ptr)[0] = DefaultRefCntOpe;
					((S64*)ptr)[1] = 0x08;
					((S64*)ptr)[2] = (S64*)*(void**)((U8*)me_ + 0x08) - (S64*)root;
					bins[0] = ptr;
				}
				bins[1] = ToBinClassAsm(me_);
				return CatBin(2, -1, bins);
			}
	}
}

EXPORT void* _fromBin(const U8* me_, const U8* type, S64* idx, const void* root)
{
	void* result = NULL;
	if (IsRef(*type) && *(S64*)(me_ + 0x10 + *idx) == -1)
	{
		*idx += 8;
		return NULL;
	}
	switch (*type)
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
				size_t size = GetSize(type[1]);
				Bool is_str = IsStr(type);
				result = AllocMem(0x10 + size * (size_t)(len + (is_str ? 1 : 0)));
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = len;
				if (is_str)
					*(Char*)((U8*)result + 0x10 + size * (size_t)len) = L'\0';
				U8* ptr = (U8*)result + 0x10;
				S64 i;
				for (i = 0; i < len; i++)
				{
					void* value = _fromBin(me_, type + 1, idx, root);
					memcpy(ptr, &value, size);
					ptr += size;
				}
			}
			break;
		case TypeId_List:
			{
				S64 idx_cur;
				S64 len = *(S64*)(me_ + 0x10 + *idx);
				*idx += 8;
				idx_cur = *(S64*)(me_ + 0x10 + *idx);
				*idx += 8;
				size_t size = GetSize(type[1]);
				result = AllocMem(0x28);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = len;
				((S64*)result)[2] = 0;
				((S64*)result)[3] = 0;
				((S64*)result)[4] = 0;
				S64 i;
				for (i = 0; i < len; i++)
				{
					U8* node = (U8*)AllocMem(0x10 + size);
					if (idx_cur == i)
						((void**)result)[4] = node;
					void* value = _fromBin(me_, type + 1, idx, root);
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
			break;
		case TypeId_Stack:
			{
				S64 len = *(S64*)(me_ + 0x10 + *idx);
				*idx += 8;
				size_t size = GetSize(type[1]);
				void* top = NULL;
				void* bottom = NULL;
				result = AllocMem(0x18);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = len;
				((S64*)result)[2] = 0;
				S64 i;
				for (i = 0; i < len; i++)
				{
					U8* node = (U8*)AllocMem(0x08 + size);
					void* value = _fromBin(me_, type + 1, idx, root);
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
			break;
		case TypeId_Queue:
			{
				S64 len = *(S64*)(me_ + 0x10 + *idx);
				*idx += 8;
				size_t size = GetSize(type[1]);
				result = AllocMem(0x20);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = len;
				((S64*)result)[2] = 0;
				((S64*)result)[3] = 0;
				S64 i;
				for (i = 0; i < len; i++)
				{
					U8* node = (U8*)AllocMem(0x08 + size);
					void* value = _fromBin(me_, type + 1, idx, root);
					memcpy(node + 0x08, &value, size);
					*(void**)node = NULL;
					if (*(void**)((U8*)result + 0x18) == NULL)
						*(void**)((U8*)result + 0x10) = node;
					else
						*(void**)*(void**)((U8*)result + 0x18) = node;
					*(void**)((U8*)result + 0x18) = node;
				}
			}
			break;
		case TypeId_Dict:
			{
				U8* child1;
				U8* child2;
				GetDictTypes(type, &child1, &child2);
				S64 len = *(S64*)(me_ + 0x10 + *idx);
				*idx += 8;
				result = AllocMem(0x18);
				((S64*)result)[0] = DefaultRefCntOpe;
				((S64*)result)[1] = len;
				((S64*)result)[2] = 0;
				S64 i;
				Bool is_key_ref = IsRef(*child1);
				Bool is_value_ref = IsRef(*child2);
				Bool addition;
				for (i = 0; i < len; i++)
				{
					void* key = _fromBin(me_, child1, idx, root);
					ASSERT(!(is_key_ref && key == NULL));
					if (is_key_ref)
					{
						ASSERT(*(S64*)key == DefaultRefCntOpe);
						*(S64*)key = 0;
					}
					void* value = _fromBin(me_, child2, idx, root);
					if (is_value_ref && value != NULL)
					{
						ASSERT(*(S64*)value == DefaultRefCntOpe);
						*(S64*)value = 0;
					}
					*(void**)((U8*)result + 0x10) = AddDictRecursion(*(void**)((U8*)result + 0x10), key, value, GetCmpFunc(child1), child1, child2, &addition);
					ASSERT(addition);
					ASSERT(!(is_key_ref && *(S64*)key == 0));
					*(Bool*)((U8*)*(void**)((U8*)result + 0x10) + 0x10) = False;
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
			ASSERT(*type == TypeId_Class);
			ASSERT(root != NULL);
			S64 kind = *(S64*)(me_ + 0x10 + *idx);
			*idx += 8;
			result = FromBinClassAsm((S64*)root + kind, me_, idx);
			break;
	}
	return result;
}

EXPORT S64 _powInt(S64 a, S64 b)
{
	switch (b)
	{
		case 0LL:
			return 1LL;
		case 1LL:
			return a;
		case 2LL:
			return a * a;
	}
	if (a == 1LL)
		return 1LL;
	if (a == -1LL)
		return (b & 1LL) == 0LL ? 1LL : -1LL;
	if (b < 0LL)
		return 0LL;
	S64 r = 1LL;
	for (; ; )
	{
		if ((b & 1LL) == 1LL)
			r *= a;
		b >>= 1LL;
		if (b == 0LL)
			break;
		a *= a;
	}
	return r;
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
	THROWDBG(*nums < 0, 0xe917000b);
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

EXPORT U8* _dictValueType(const U8* type)
{
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	return child2;
}

EXPORT void _addDict(void* me_, const U8* type, const void* key, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	Bool addition;
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	THROWDBG(IsRef(*child1) && key == NULL, EXCPT_ACCESS_VIOLATION); // 'key' must not be 'null'.
	*(void**)((U8*)me_ + 0x10) = AddDictRecursion(*(void**)((U8*)me_ + 0x10), key, item, GetCmpFunc(child1), child1, child2, &addition);
	if (addition)
		(*(S64*)((U8*)me_ + 0x08))++;
	*(Bool*)((U8*)*(void**)((U8*)me_ + 0x10) + 0x10) = False;
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

EXPORT void _addStack(void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* node = (U8*)AllocMem(0x08 + GetSize(type[1]));
	Copy(node + 0x08, type[1], &item);
	*(void**)node = *(void**)((U8*)me_ + 0x10);
	*(void**)((U8*)me_ + 0x10) = node;
	(*(S64*)((U8*)me_ + 0x08))++;
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

EXPORT void _del(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(void**)((U8*)me_ + 0x20) == NULL, 0xe917000a);
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

EXPORT void _delDict(void* me_, const U8* type, const void* key)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	THROWDBG(IsRef(*child1) && key == NULL, EXCPT_ACCESS_VIOLATION); // 'key' must not be 'null'.
	Bool deleted = False;
	*(void**)((U8*)me_ + 0x10) = DelDictRecursion(*(void**)((U8*)me_ + 0x10), key, GetCmpFunc(child1), child1, child2, &deleted);
	if (*(void**)((U8*)me_ + 0x10) != NULL)
		*(Bool*)((U8*)*(void**)((U8*)me_ + 0x10) + 0x10) = False;
	if (deleted)
		(*(S64*)((U8*)me_ + 0x08))--;
}

EXPORT void _delNext(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(void**)((U8*)me_ + 0x20) == NULL, 0xe917000a);
	void* ptr = *(void**)((U8*)me_ + 0x20);
	void* next = *(void**)((U8*)ptr + 0x08);
	THROWDBG(next == NULL, 0xe917000a);
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

EXPORT S64 _endian(const void* me_, const U8* type)
{
	switch (*type)
	{
		case TypeId_Bit8:
			return (S64)(U64) * (U8*)&me_;
		case TypeId_Bit16:
			{
				U16 n = *(U16*)&me_;
				return (S64)(U64)(((n & 0x00ff) << 8) | ((n & 0xFF00) >> 8));
			}
		case TypeId_Bit32:
			{
				U32 n = *(U32*)&me_;
				n = ((n & 0x00ff00ff) << 8) | ((n & 0xff00ff00) >> 8);
				n = ((n & 0x0000ffff) << 16) | ((n & 0xffff0000) >> 16);
				return (S64)(U64)n;
			}
		case TypeId_Bit64:
			{
				U64 n = *(U64*)&me_;
				n = ((n & 0x00ff00ff00ff00ff) << 8) | ((n & 0xff00ff00ff00ff00) >> 8);
				n = ((n & 0x0000ffff0000ffff) << 16) | ((n & 0xffff0000ffff0000) >> 16);
				n = ((n & 0x00000000ffffffff) << 32) | ((n & 0xffffffff00000000) >> 32);
				return (S64)n;
			}
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT Bool _exist(void* me_, const U8* type, const void* key)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	THROWDBG(IsRef(*child1) && key == NULL, EXCPT_ACCESS_VIOLATION); // 'key' must not be 'null'.
	int(*cmp_func)(const void* a, const void* b) = GetCmpFunc(child1);
	const void* node = *(void**)((U8*)me_ + 0x10);
	while (node != NULL)
	{
		int cmp = cmp_func(&key, (void**)((U8*)node + 0x18));
		if (cmp == 0)
			return True;
		if (cmp < 0)
			node = *(void**)node;
		else
			node = *(void**)((U8*)node + 0x08);
	}
	return False;
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
			void* ptr2 = *(void**)ptr;
			if (ptr2 != NULL)
			{
				(*(S64*)ptr2)--;
				if (*(S64*)ptr2 == 0)
					_freeSet(ptr2, type + 1);
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

EXPORT S64 _findArray(const void* me_, const U8* type, const void* item, S64 start)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	ASSERT(cmp != NULL);
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
		if (cmp(&value, &item) == 0)
			return i;
		ptr += size;
	}
	return -1;
}

EXPORT S64 _findBin(const void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	ASSERT(cmp != NULL);
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
		cmp2 = cmp(&item, &value);
		if (cmp2 < 0)
			max = mid - 1;
		else if (cmp2 > 0)
			min = mid + 1;
		else
			return mid;
	}
	return -1;
}

EXPORT S64 _findLastArray(const void* me_, const U8* type, const void* item, S64 start)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	ASSERT(cmp != NULL);
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
		if (cmp(&value, &item) == 0)
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
	ASSERT(cmp != NULL);
	for (; ; )
	{
		void* cur = *(void**)((U8*)me_ + 0x20);
		if (cur == NULL)
			return False;
		void* value = NULL;
		memcpy(&value, (U8*)cur + 0x10, size);
		if (cmp(&value, &item) == 0)
			return True;
		*(void**)((U8*)me_ + 0x20) = *(void**)cur;
	}
}

EXPORT Bool _findList(const void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	ASSERT(cmp != NULL);
	for (; ; )
	{
		void* cur = *(void**)((U8*)me_ + 0x20);
		if (cur == NULL)
			return False;
		void* value = NULL;
		memcpy(&value, (U8*)cur + 0x10, size);
		if (cmp(&value, &item) == 0)
			return True;
		*(void**)((U8*)me_ + 0x20) = *(void**)((U8*)cur + 0x08);
	}
}

EXPORT Bool _forEach(void* me_, const U8* type, const void* callback, void* data)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(callback == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	void* ptr = *(void**)((U8*)me_ + 0x10);
	if (ptr == NULL)
		return True;
	return ForEachRecursion(ptr, child1, child2, callback, data);
}

EXPORT void* _getDict(void* me_, const U8* type, const void* key, Bool* existed)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	U8* child1;
	U8* child2;
	GetDictTypes(type, &child1, &child2);
	THROWDBG(IsRef(*child1) && key == NULL, EXCPT_ACCESS_VIOLATION); // 'key' must not be 'null'.
	int(*cmp_func)(const void* a, const void* b) = GetCmpFunc(child1);
	const void* node = *(void**)((U8*)me_ + 0x10);
	while (node != NULL)
	{
		int cmp = cmp_func(&key, (void**)((U8*)node + 0x18));
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
	*existed = False;
	return NULL;
}

EXPORT void* _getList(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(void**)((U8*)me_ + 0x20) == NULL, 0xe917000a);
	void* result = NULL;
	Copy(&result, type[1], (U8*)*(void**)((U8*)me_ + 0x20) + 0x10);
	return result;
}

EXPORT void* _getOffset(void* me_, const U8* type, S64 offset)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* ptr = *(void**)((U8*)me_ + 0x20);
	S64 i;
	if (offset >= 0)
	{
		for (i = 0; i < offset; i++)
		{
			THROWDBG(ptr == NULL, 0xe917000a);
			ptr = *(void**)((U8*)ptr + 0x08);
		}
	}
	else
	{
		for (i = 0; i > offset; i--)
		{
			THROWDBG(ptr == NULL, 0xe917000a);
			ptr = *(void**)ptr;
		}
	}
	THROWDBG(ptr == NULL, 0xe917000a);
	void* result = NULL;
	Copy(&result, type[1], (U8*)ptr + 0x10);
	return result;
}

EXPORT SClass* _getPtr(void* me_, const U8* type, SClass* me2)
{
	UNUSED(type);
	SListPtr* ptr = (SListPtr*)me2;
	ptr->Ptr = *(void**)((U8*)me_ + 0x20);
	return me2;
}

EXPORT void* _getQueue(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* node = *(void**)((U8*)me_ + 0x10);
	THROWDBG(node == NULL, 0xe917000a);
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

EXPORT void* _getStack(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* node = *(void**)((U8*)me_ + 0x10);
	THROWDBG(node == NULL, 0xe917000a);
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

EXPORT void _head(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	UNUSED(type);
	*(void**)((U8*)me_ + 0x20) = *(void**)((U8*)me_ + 0x10);
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

EXPORT void _ins(void* me_, const U8* type, const void* item)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(*(void**)((U8*)me_ + 0x20) == NULL, 0xe917000a);
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

EXPORT void* _max(const void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	ASSERT(cmp != NULL);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	U8* ptr = (U8*)me_ + 0x10;
	void* item = NULL;
	S64 i;
	for (i = 0; i < len; i++)
	{
		void* value = NULL;
		memcpy(&value, ptr, size);
		if (i == 0 || cmp(&item, &value) < 0)
			item = value;
		ptr += size;
	}
	void* result = NULL;
	Copy(&result, type[1], &item);
	return result;
}

EXPORT void* _min(const void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	size_t size = GetSize(type[1]);
	int(*cmp)(const void* a, const void* b) = GetCmpFunc(type + 1);
	ASSERT(cmp != NULL);
	S64 len = *(S64*)((U8*)me_ + 0x08);
	U8* ptr = (U8*)me_ + 0x10;
	void* item = NULL;
	S64 i;
	for (i = 0; i < len; i++)
	{
		void* value = NULL;
		memcpy(&value, ptr, size);
		if (i == 0 || cmp(&item, &value) > 0)
			item = value;
		ptr += size;
	}
	void* result = NULL;
	Copy(&result, type[1], &item);
	return result;
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

EXPORT Bool _nan(double me_)
{
	return isnan(me_);
}

EXPORT void _next(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* ptr = *(void**)((U8*)me_ + 0x20);
	if (ptr == NULL)
		return;
	UNUSED(type);
	*(void**)((U8*)me_ + 0x20) = *(void**)((U8*)ptr + 0x08);
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

EXPORT void* _peek(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* node = *(void**)((U8*)me_ + 0x10);
	THROWDBG(node == NULL, EXCPT_ACCESS_VIOLATION);
	void* result = NULL;
	Copy(&result, type[1], (U8*)node + 0x08);
	return result;
}

EXPORT void _prev(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	void* ptr = *(void**)((U8*)me_ + 0x20);
	if (ptr == NULL)
		return;
	UNUSED(type);
	*(void**)((U8*)me_ + 0x20) = *(void**)ptr;
}

EXPORT void* _repeat(const void* me_, const U8* type, S64 len)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(len < 0, 0xe9170006);
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

EXPORT S64 _sar(const void* me_, const U8* type, S64 n)
{
	// In Visual C++, shifting of signed type is guaranteed to be an arithmetic shift.
	switch (*type)
	{
		case TypeId_Bit8:
			THROWDBG(n < 0 || n >= 8, 0xe9170006);
			return (S64)((S8) * (U8*)&me_ >> n);
		case TypeId_Bit16:
			THROWDBG(n < 0 || n >= 16, 0xe9170006);
			return (S64)((S16) * (U16*)&me_ >> n);
		case TypeId_Bit32:
			THROWDBG(n < 0 || n >= 32, 0xe9170006);
			return (S64)((S32) * (U32*)&me_ >> n);
		case TypeId_Bit64:
			THROWDBG(n < 0 || n >= 64, 0xe9170006);
			return ((S64) * (U64*)&me_ >> n);
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT void _setPtr(void* me_, const U8* type, SClass* ptr)
{
	UNUSED(type);
	SListPtr* ptr2 = (SListPtr*)ptr;
	*(void**)((U8*)me_ + 0x20) = ptr2->Ptr;
}

EXPORT S64 _shl(const void* me_, const U8* type, S64 n)
{
	switch (*type)
	{
		case TypeId_Bit8:
			THROWDBG(n < 0 || n >= 8, 0xe9170006);
			return (S64)(U64)(U8)(*(U8*)&me_ << n);
		case TypeId_Bit16:
			THROWDBG(n < 0 || n >= 16, 0xe9170006);
			return (S64)(U64)(U16)(*(U16*)&me_ << n);
		case TypeId_Bit32:
			THROWDBG(n < 0 || n >= 32, 0xe9170006);
			return (S64)(U64)(U32)(*(U32*)&me_ << n);
		case TypeId_Bit64:
			THROWDBG(n < 0 || n >= 64, 0xe9170006);
			return (S64)(*(U64*)&me_ << n);
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
		case TypeId_Bit8:
			THROWDBG(n < 0 || n >= 8, 0xe9170006);
			return (S64)(U64)(*(U8*)&me_ >> n);
		case TypeId_Bit16:
			THROWDBG(n < 0 || n >= 16, 0xe9170006);
			return (S64)(U64)(*(U16*)&me_ >> n);
		case TypeId_Bit32:
			THROWDBG(n < 0 || n >= 32, 0xe9170006);
			return (S64)(U64)(*(U32*)&me_ >> n);
		case TypeId_Bit64:
			THROWDBG(n < 0 || n >= 64, 0xe9170006);
			return (S64)(*(U64*)&me_ >> n);
		default:
			ASSERT(False);
			return 0;
	}
}

EXPORT void _sort(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	int(*cmp)(const void*, const void*) = GetCmpFunc(type + 1);
	ASSERT(cmp != NULL);
	qsort((U8*)me_ + 0x10, (size_t) * (S64*)((U8*)me_ + 0x08), GetSize(type[1]), cmp);
}

EXPORT void* _sub(const void* me_, const U8* type, S64 start, S64 len)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	S64 len2 = *(S64*)((U8*)me_ + 0x08);
	if (len == -1)
		len = len2 - start;
	THROWDBG(start < 0 || len < 0 || start + len > len2, 0xe9170006);
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

EXPORT void _tail(void* me_, const U8* type)
{
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	UNUSED(type);
	*(void**)((U8*)me_ + 0x20) = *(void**)((U8*)me_ + 0x18);
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

EXPORT double _toFloat(const U8* me_, Bool* success)
{
	Char* ptr;
	THROWDBG(me_ == NULL, EXCPT_ACCESS_VIOLATION);
	double value = wcstod((const Char*)(me_ + 0x10), &ptr);
	*success = *ptr == L'\0';
	return value;
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
	U8* result = (U8*)AllocMem(0x10 + sizeof(Char) * (size_t)(len + 1));
	((S64*)result)[0] = DefaultRefCntFunc;
	((S64*)result)[1] = (S64)len;
	memcpy(result + 0x10, str, sizeof(Char) * (size_t)(len + 1));
	return result;
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

static Bool IsRef(U8 type)
{
	return type >= TypeId_Ref;
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

static void ToBinDictRecursion(void*** buf, void* node, U8* key_type, U8* item_type, const void* root)
{
	if (*(void**)node != NULL)
		ToBinDictRecursion(buf, *(void**)node, key_type, item_type, root);
	{
		void* key = NULL;
		memcpy(&key, (U8*)node + 0x18, GetSize(*key_type));
		**buf = _toBin(key, key_type, root);
		(*buf)++;
	}
	{
		void* value = NULL;
		memcpy(&value, (U8*)node + 0x20, GetSize(*item_type));
		**buf = _toBin(value, item_type, root);
		(*buf)++;
	}
	if (*(void**)((U8*)node + 0x08) != NULL)
		ToBinDictRecursion(buf, *(void**)((U8*)node + 0x08), key_type, item_type, root);
}

static void* CatBin(S64 num, S64 writing_num, void** bins)
{
	S64 len = 0;
	S64 i;
	for (i = 0; i < num; i++)
		len += *(S64*)((U8*)bins[i] + 0x08);
	U8* result = (U8*)AllocMem((writing_num == -1 ? 0x10 : 0x18) + (size_t)len);
	((S64*)result)[0] = DefaultRefCntOpe;
	((S64*)result)[1] = (writing_num == -1 ? 0x00 : 0x08) + len;
	if (writing_num != -1)
		((S64*)result)[2] = writing_num;
	{
		U8* ptr = result + (writing_num == -1 ? 0x10 : 0x18);
		for (i = 0; i < num; i++)
		{
			size_t size = (size_t) * (S64*)((U8*)bins[i] + 0x08);
			memcpy(ptr, (U8*)bins[i] + 0x10, size);
			ptr += size;
			ASSERT(*(S64*)bins[i] == DefaultRefCntOpe);
			FreeMem(bins[i]);
		}
	}
	FreeMem(bins);
	return result;
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
	int cmp = cmp_func(&key, (void**)((U8*)node + 0x18));
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
	// '*addition' should have been set so far.
	return DictFixUp(node);
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

static void* DelDictRecursion(void* node, const void* key, int cmp_func(const void* a, const void* b), U8* key_type, U8* item_type, Bool* deleted)
{
	if (node == NULL)
		return NULL;
	if (cmp_func(&key, (void**)((U8*)node + 0x18)) < 0)
	{
		if (*(void**)node != NULL && !*(Bool*)((U8*)*(void**)node + 0x10) && !(*(void**)*(void**)node != NULL && *(Bool*)((U8*)*(void**)*(void**)node + 0x10)))
			node = DictMoveRedLeft(node);
		*(void**)node = DelDictRecursion(*(void**)node, key, cmp_func, key_type, item_type, deleted);
	}
	else
	{
		if (*(void**)node != NULL && *(Bool*)((U8*)*(void**)node + 0x10))
			node = DictRotateRight(node);
		if (*(void**)((U8*)node + 0x08) != NULL && !*(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10) && !(*(void**)*(void**)((U8*)node + 0x08) != NULL && *(Bool*)((U8*)*(void**)*(void**)((U8*)node + 0x08) + 0x10)))
			node = DictMoveRedRight(node);
		if (cmp_func(&key, (void**)((U8*)node + 0x18)) == 0)
		{
			*deleted = True;
			if (*(void**)((U8*)node + 0x08) == NULL)
			{
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
				return NULL;
			}
			void* ptr = *(void**)((U8*)node + 0x08);
			while (*(void**)ptr != NULL)
				ptr = *(void**)ptr;
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
			*(void**)((U8*)node + 0x18) = NULL;
			Copy((U8*)node + 0x18, *key_type, (U8*)ptr + 0x18);
			Copy((U8*)node + 0x20, *item_type, (U8*)ptr + 0x20);
			*(void**)((U8*)node + 0x08) = DictDelMinRec(*(void**)((U8*)node + 0x08), key_type, item_type);
		}
		else
			*(void**)((U8*)node + 0x08) = DelDictRecursion(*(void**)((U8*)node + 0x08), key, cmp_func, key_type, item_type, deleted);
	}
	return DictFixUp(node);
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

static int CmpInt(const void* a, const void* b)
{
	S64 a2 = *(S64*)a;
	S64 b2 = *(S64*)b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpFloat(const void* a, const void* b)
{
	double a2 = *(double*)a;
	double b2 = *(double*)b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpChar(const void* a, const void* b)
{
	Char a2 = *(Char*)a;
	Char b2 = *(Char*)b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpBit8(const void* a, const void* b)
{
	U8 a2 = *(U8*)a;
	U8 b2 = *(U8*)b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpBit16(const void* a, const void* b)
{
	U16 a2 = *(U16*)a;
	U16 b2 = *(U16*)b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpBit32(const void* a, const void* b)
{
	U32 a2 = *(U32*)a;
	U32 b2 = *(U32*)b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpBit64(const void* a, const void* b)
{
	U64 a2 = *(U64*)a;
	U64 b2 = *(U64*)b;
	return a2 > b2 ? 1 : (a2 < b2 ? -1 : 0);
}

static int CmpStr(const void* a, const void* b)
{
	const Char* a2 = (const Char*)((U8*)*(void**)a + 0x10);
	const Char* b2 = (const Char*)((U8*)*(void**)b + 0x10);
	return wcscmp(a2, b2);
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

static void DictFlip(void* node)
{
	*(Bool*)((U8*)node + 0x10) = !*(Bool*)((U8*)node + 0x10);
	*(Bool*)((U8*)*(void**)node + 0x10) = !*(Bool*)((U8*)*(void**)node + 0x10);
	*(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10) = !*(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10);
}

static void* DictFixUp(void* node)
{
	if (*(void**)((U8*)node + 0x08) != NULL && *(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10))
		node = DictRotateLeft(node);
	if (*(void**)node != NULL && *(Bool*)((U8*)*(void**)node + 0x10) && *(void**)*(void**)node != NULL && *(Bool*)((U8*)*(void**)*(void**)node + 0x10))
		node = DictRotateRight(node);
	if (*(void**)node != NULL && *(Bool*)((U8*)*(void**)node + 0x10) && *(void**)((U8*)node + 0x08) != NULL && *(Bool*)((U8*)*(void**)((U8*)node + 0x08) + 0x10))
		DictFlip(node);
	return node;
}

static void* DictMoveRedLeft(void* node)
{
	DictFlip(node);
	if (*(void**)*(void**)((U8*)node + 0x08) != NULL && *(Bool*)((U8*)*(void**)*(void**)((U8*)node + 0x08) + 0x10))
	{
		*(void**)((U8*)node + 0x08) = DictRotateRight(*(void**)((U8*)node + 0x08));
		node = DictRotateLeft(node);
		DictFlip(node);
	}
	return node;
}

static void* DictMoveRedRight(void* node)
{
	DictFlip(node);
	if (*(void**)*(void**)node != NULL && *(Bool*)((U8*)*(void**)*(void**)node + 0x10))
	{
		node = DictRotateRight(node);
		DictFlip(node);
	}
	return node;
}

static void* DictDelMinRec(void* node, U8* key_type, U8* item_type)
{
	if (*(void**)node == NULL)
	{
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
		return NULL;
	}
	if (!*(Bool*)((U8*)*(void**)node + 0x10) && !(*(void**)*(void**)node != NULL && *(Bool*)((U8*)*(void**)*(void**)node + 0x10)))
		node = DictMoveRedLeft(node);
	*(void**)node = DictDelMinRec(*(void**)node, key_type, item_type);
	return DictFixUp(node);
}
