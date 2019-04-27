#include "asm.h"

#include "mem.h"
#include "util.h"

static void DumpVal(FILE* file_ptr, const SVal* val);
static Bool IsXMM(const SValReg* reg);
static Bool IsAX(const SVal* val);
static Bool IsMem(const SVal* val);
static Bool IsRXReg(const SValReg* reg);
static Bool IsSPLReg(const SValReg* reg);
static Bool IsXMMReg(const SValReg* reg);
static U8 GetRegCode(const SValReg* reg);
static void REX(FILE* file_ptr, const SVal* a, const SVal* b);
static U64 GetRefValue(SRefValue* value, FILE* file_ptr, SList* ref_value_list);
static void WriteValMemS(FILE* file_ptr, SList* ref_value_list, const SVal* a, U8 b);
static void WriteValMem(FILE* file_ptr, SList* ref_value_list, const SVal* a, const SVal* b);
static void WriteAXImm(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, const SVal* b);
static void WriteRegImm(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, const SVal* b, U8 rate);
static void WriteRegImm2(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, const SVal* b, U8 rate);
static void WriteMemReg(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, const SVal* b);
static void WriteMemOnly(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, U8 rate);

SRefValue* RefValue(U64 value)
{
	SRefValue* ref_value = (SRefValue*)Alloc(sizeof(SRefValue));
	ref_value->TypeId = RefValueTypeId_RefValue;
	ref_value->Value = value;
	return ref_value;
}

SRefValue* RefValueAddr(S64* addr, Bool relative)
{
	SRefValueAddr* ref_value = (SRefValueAddr*)Alloc(sizeof(SRefValueAddr));
	((SRefValue*)ref_value)->TypeId = RefValueTypeId_Addr;
	((SRefValue*)ref_value)->Value = 0xffffffff;
	ref_value->Pos = -1;
	ref_value->Bottom = -1;
	ref_value->Addr = addr;
	ref_value->Relative = relative;
	return (SRefValue*)ref_value;
}

SVal* ValImm(int size, SRefValue* value)
{
	SValImm* val = (SValImm*)Alloc(sizeof(SValImm));
	ASSERT(value != NULL);
	((SVal*)val)->TypeId = ValTypeId_Imm;
	((SVal*)val)->Size = size;
	val->Value = value;
	return (SVal*)val;
}

SVal* ValImmS(int size, S64 value)
{
	SValImm* val = (SValImm*)Alloc(sizeof(SValImm));
	((SVal*)val)->TypeId = ValTypeId_Imm;
	if (size == 8 && value == (S64)(S32)value)
		size = 4;
	if (size == 4 && value == (S64)(S16)value)
		size = 2;
	if (size == 2 && value == (S64)(S8)value)
		size = 1;
	((SVal*)val)->Size = size;
	val->Value = RefValue((U64)value);
	return (SVal*)val;
}

SVal* ValImmU(int size, U64 value)
{
	SValImm* val = (SValImm*)Alloc(sizeof(SValImm));
	((SVal*)val)->TypeId = ValTypeId_Imm;
	if (size == 8 && value == (U64)(U32)value)
		size = 4;
	if (size == 4 && value == (U64)(U16)value)
		size = 2;
	if (size == 2 && value == (U64)(U8)value)
		size = 1;
	((SVal*)val)->Size = size;
	val->Value = RefValue(value);
	return (SVal*)val;
}

SVal* ValReg(int size, EReg reg)
{
	SValReg* val = (SValReg*)Alloc(sizeof(SValReg));
	((SVal*)val)->TypeId = ValTypeId_Reg;
	if (size == 1)
	{
		switch (reg)
		{
			case Reg_SP: reg = Reg_SPL; break;
			case Reg_BP: reg = Reg_BPL; break;
			case Reg_SI: reg = Reg_SIL; break;
			case Reg_DI: reg = Reg_DIL; break;
		}
	}
	((SVal*)val)->Size = size;
	val->Reg = reg;
	return (SVal*)val;
}

SVal* ValMem(int size, SVal* base, SVal* idx, SRefValue* disp)
{
	SValMem* val = (SValMem*)Alloc(sizeof(SValMem));
	ASSERT(disp != NULL);
	ASSERT(base == NULL || base->TypeId == ValTypeId_Reg && !IsXMM((SValReg*)base));
	ASSERT(idx == NULL || idx->TypeId == ValTypeId_Reg && !IsXMM((SValReg*)idx));
	((SVal*)val)->TypeId = ValTypeId_Mem;
	ASSERT(!(base != NULL && IsXMM((SValReg*)base) || idx != NULL && IsXMM((SValReg*)idx))); // 'XMM' should not be specified here.
	((SVal*)val)->Size = size;
	val->Base = (SValReg*)base;
	val->Idx = (SValReg*)idx;
	val->Disp = disp;
	return (SVal*)val;
}

SVal* ValMemS(int size, SVal* base, SVal* idx, S64 disp)
{
	SValMem* val = (SValMem*)Alloc(sizeof(SValMem));
	ASSERT(base == NULL || base->TypeId == ValTypeId_Reg && !IsXMM((SValReg*)base));
	ASSERT(idx == NULL || idx->TypeId == ValTypeId_Reg && !IsXMM((SValReg*)idx));
	((SVal*)val)->TypeId = ValTypeId_Mem;
	ASSERT(!(base != NULL && IsXMM((SValReg*)base) || idx != NULL && IsXMM((SValReg*)idx))); // 'XMM' should not be specified here.
	((SVal*)val)->Size = size;
	val->Base = (SValReg*)base;
	val->Idx = (SValReg*)idx;
	val->Disp = RefValue((U64)disp);
	return (SVal*)val;
}

SVal* ValRIP(int size, SRefValue* disp)
{
	SValRIP* val = (SValRIP*)Alloc(sizeof(SValRIP));
	((SVal*)val)->TypeId = ValTypeId_RIP;
	((SVal*)val)->Size = size;
	val->Disp = disp;
	return (SVal*)val;
}

SAsmLabel* AsmLabel(void)
{
	SAsmLabel* asm_ = (SAsmLabel*)Alloc(sizeof(SAsmLabel));
	((SAsm*)asm_)->TypeId = AsmTypeId_Label;
	((SAsm*)asm_)->Addr = NewAddr();
#if defined(_DEBUG)
	static int cnt = 0;
	asm_->Cnt = cnt;
	cnt++;
#endif
	return asm_;
}

SAsmMachine* AsmMachine(int data_size, U8* data)
{
	SAsmMachine* asm_ = (SAsmMachine*)Alloc(sizeof(SAsmMachine));
	((SAsm*)asm_)->TypeId = AsmTypeId_Machine;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->DataSize = data_size;
	asm_->Data = data;
	return asm_;
}

SAsmADDGroup* AsmADD(SVal* a, SVal* b)
{
	SAsmADDGroup* asm_ = (SAsmADDGroup*)Alloc(sizeof(SAsmADDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_ADD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Rate = 0x00;
	return asm_;
}

SAsmADDSDGroup* AsmADDSD(SVal* a, SVal* b)
{
	SAsmADDSDGroup* asm_ = (SAsmADDSDGroup*)Alloc(sizeof(SAsmADDSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_ADDSD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = True;
	asm_->Code = 0x58;
	return asm_;
}

SAsmADDGroup* AsmAND(SVal* a, SVal* b)
{
	SAsmADDGroup* asm_ = (SAsmADDGroup*)Alloc(sizeof(SAsmADDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_AND;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Rate = 0x04;
	return asm_;
}

SAsm1* AsmCALL(SVal* a)
{
	SAsm1* asm_ = (SAsm1*)Alloc(sizeof(SAsm1));
	((SAsm*)asm_)->TypeId = AsmTypeId_CALL;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	return asm_;
}

SAsm1* AsmCDQ(SVal* a)
{
	SAsm1* asm_ = (SAsm1*)Alloc(sizeof(SAsm1));
	((SAsm*)asm_)->TypeId = AsmTypeId_CDQ;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	return asm_;
}

SAsmADDGroup* AsmCMP(SVal* a, SVal* b)
{
	SAsmADDGroup* asm_ = (SAsmADDGroup*)Alloc(sizeof(SAsmADDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_CMP;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Rate = 0x07;
	return asm_;
}

SAsmCOMISDGroup* AsmCOMISD(SVal* a, SVal* b)
{
	SAsmCOMISDGroup* asm_ = (SAsmCOMISDGroup*)Alloc(sizeof(SAsmCOMISDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_COMISD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = True;
	asm_->Code = 0x2f;
	return asm_;
}

SAsmADDSDGroup* AsmCVTSD2SI(SVal* a, SVal* b)
{
	SAsmADDSDGroup* asm_ = (SAsmADDSDGroup*)Alloc(sizeof(SAsmADDSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_CVTSD2SI;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = True;
	asm_->Code = 0x2d;
	return asm_;
}

SAsmADDSDGroup* AsmCVTSD2SS(SVal* a, SVal* b)
{
	SAsmADDSDGroup* asm_ = (SAsmADDSDGroup*)Alloc(sizeof(SAsmADDSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_CVTSD2SS;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = True;
	asm_->Code = 0x5a;
	return asm_;
}

SAsmADDSDGroup* AsmCVTSI2SD(SVal* a, SVal* b)
{
	SAsmADDSDGroup* asm_ = (SAsmADDSDGroup*)Alloc(sizeof(SAsmADDSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_CVTSI2SD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = True;
	asm_->Code = 0x2a;
	return asm_;
}

SAsmADDSDGroup* AsmCVTSS2SD(SVal* a, SVal* b)
{
	SAsmADDSDGroup* asm_ = (SAsmADDSDGroup*)Alloc(sizeof(SAsmADDSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_CVTSS2SD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = False;
	asm_->Code = 0x5a;
	return asm_;
}

SAsmINCGroup* AsmDEC(SVal* a)
{
	SAsmINCGroup* asm_ = (SAsmINCGroup*)Alloc(sizeof(SAsmINCGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_DEC;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0xfe;
	asm_->Rate = 0x01;
	return asm_;
}

SAsmINCGroup* AsmDIV(SVal* a)
{
	SAsmINCGroup* asm_ = (SAsmINCGroup*)Alloc(sizeof(SAsmINCGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_DIV;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0xf6;
	asm_->Rate = 0x06;
	return asm_;
}

SAsmADDSDGroup* AsmDIVSD(SVal* a, SVal* b)
{
	SAsmADDSDGroup* asm_ = (SAsmADDSDGroup*)Alloc(sizeof(SAsmADDSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_DIVSD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = True;
	asm_->Code = 0x5e;
	return asm_;
}

SAsmINCGroup* AsmIDIV(SVal* a)
{
	SAsmINCGroup* asm_ = (SAsmINCGroup*)Alloc(sizeof(SAsmINCGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_DIV;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0xf6;
	asm_->Rate = 0x07;
	return asm_;
}

SAsm3* AsmIMUL(SVal* a, SVal* b, SVal* c)
{
	SAsm3* asm_ = (SAsm3*)Alloc(sizeof(SAsm3));
	((SAsm*)asm_)->TypeId = AsmTypeId_IMUL;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->C = c;
	return asm_;
}

SAsmINCGroup* AsmINC(SVal* a)
{
	SAsmINCGroup* asm_ = (SAsmINCGroup*)Alloc(sizeof(SAsmINCGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_INC;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0xfe;
	asm_->Rate = 0x00;
	return asm_;
}

SAsm1* AsmINT(SVal* a)
{
	SAsm1* asm_ = (SAsm1*)Alloc(sizeof(SAsm1));
	((SAsm*)asm_)->TypeId = AsmTypeId_INT;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	return asm_;
}

SAsmJEGroup* AsmJA(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JA;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x77;
	return asm_;
}

SAsmJEGroup* AsmJAE(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JAE;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x73;
	return asm_;
}

SAsmJEGroup* AsmJB(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JB;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x72;
	return asm_;
}

SAsmJEGroup* AsmJBE(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JBE;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x76;
	return asm_;
}

SAsmJEGroup* AsmJE(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JE;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x74;
	return asm_;
}

SAsmJEGroup* AsmJG(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JG;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x7f;
	return asm_;
}

SAsmJEGroup* AsmJGE(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JGE;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x7d;
	return asm_;
}

SAsmJEGroup* AsmJL(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JL;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x7c;
	return asm_;
}

SAsmJEGroup* AsmJLE(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JLE;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x7e;
	return asm_;
}

SAsm1* AsmJMP(SVal* a)
{
	SAsm1* asm_ = (SAsm1*)Alloc(sizeof(SAsm1));
	((SAsm*)asm_)->TypeId = AsmTypeId_JMP;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	return asm_;
}

SAsmJEGroup* AsmJNE(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JNE;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x75;
	return asm_;
}

SAsmJEGroup* AsmJNO(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JNO;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x71;
	return asm_;
}

SAsmJEGroup* AsmJNS(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JNS;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x79;
	return asm_;
}

SAsmJEGroup* AsmJO(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JO;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x70;
	return asm_;
}

SAsmJEGroup* AsmJS(SVal* a)
{
	SAsmJEGroup* asm_ = (SAsmJEGroup*)Alloc(sizeof(SAsmJEGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_JS;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0x78;
	return asm_;
}

SAsm1* AsmLDMXCSR(SVal* a)
{
	SAsm1* asm_ = (SAsm1*)Alloc(sizeof(SAsm1));
	((SAsm*)asm_)->TypeId = AsmTypeId_LDMXCSR;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	return asm_;
}

SAsm2* AsmLEA(SVal* a, SVal* b)
{
	SAsm2* asm_ = (SAsm2*)Alloc(sizeof(SAsm2));
	((SAsm*)asm_)->TypeId = AsmTypeId_LEA;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	return asm_;
}

SAsm2* AsmMOV(SVal* a, SVal* b)
{
	SAsm2* asm_ = (SAsm2*)Alloc(sizeof(SAsm2));
	((SAsm*)asm_)->TypeId = AsmTypeId_MOV;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	return asm_;
}

SAsmMOVSDGroup* AsmMOVSD(SVal* a, SVal* b)
{
	SAsmMOVSDGroup* asm_ = (SAsmMOVSDGroup*)Alloc(sizeof(SAsmMOVSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_MOVSD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Code = 0xf2;
	asm_->CodeRM = 0x10;
	asm_->CodeMR = 0x11;
	return asm_;
}

SAsmMOVSDGroup* AsmMOVSS(SVal* a, SVal* b)
{
	SAsmMOVSDGroup* asm_ = (SAsmMOVSDGroup*)Alloc(sizeof(SAsmMOVSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_MOVSS;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Code = 0xf3;
	asm_->CodeRM = 0x10;
	asm_->CodeMR = 0x11;
	return asm_;
}

SAsm2* AsmMOVSX(SVal* a, SVal* b)
{
	SAsm2* asm_ = (SAsm2*)Alloc(sizeof(SAsm2));
	((SAsm*)asm_)->TypeId = AsmTypeId_MOVSX;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	return asm_;
}

SAsm2* AsmMOVUPS(SVal* a, SVal* b)
{
	SAsm2* asm_ = (SAsm2*)Alloc(sizeof(SAsm2));
	((SAsm*)asm_)->TypeId = AsmTypeId_MOVUPS;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	return asm_;
}

SAsm2* AsmMOVZX(SVal* a, SVal* b)
{
	SAsm2* asm_ = (SAsm2*)Alloc(sizeof(SAsm2));
	((SAsm*)asm_)->TypeId = AsmTypeId_MOVZX;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	return asm_;
}

SAsmADDSDGroup* AsmMULSD(SVal* a, SVal* b)
{
	SAsmADDSDGroup* asm_ = (SAsmADDSDGroup*)Alloc(sizeof(SAsmADDSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_MULSD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = True;
	asm_->Code = 0x59;
	return asm_;
}

SAsmINCGroup* AsmNEG(SVal* a)
{
	SAsmINCGroup* asm_ = (SAsmINCGroup*)Alloc(sizeof(SAsmINCGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_NEG;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0xf6;
	asm_->Rate = 0x03;
	return asm_;
}

SAsm* AsmNOP(void)
{
	SAsm* asm_ = (SAsm*)Alloc(sizeof(SAsm));
	((SAsm*)asm_)->TypeId = AsmTypeId_NOP;
	((SAsm*)asm_)->Addr = NewAddr();
	return asm_;
}

SAsmINCGroup* AsmNOT(SVal* a)
{
	SAsmINCGroup* asm_ = (SAsmINCGroup*)Alloc(sizeof(SAsmINCGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_NOT;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0xf6;
	asm_->Rate = 0x02;
	return asm_;
}

SAsmADDGroup* AsmOR(SVal* a, SVal* b)
{
	SAsmADDGroup* asm_ = (SAsmADDGroup*)Alloc(sizeof(SAsmADDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_OR;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Rate = 0x01;
	return asm_;
}

SAsmREPGroup* AsmREPMOVS(SVal* a)
{
	SAsmREPGroup* asm_ = (SAsmREPGroup*)Alloc(sizeof(SAsmREPGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_REPMOVS;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0xa4;
	return asm_;
}

SAsmREPGroup* AsmREPSTOS(SVal* a)
{
	SAsmREPGroup* asm_ = (SAsmREPGroup*)Alloc(sizeof(SAsmREPGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_REPSTOS;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->Code = 0xaa;
	return asm_;
}

SAsm* AsmRET(void)
{
	SAsm* asm_ = (SAsm*)Alloc(sizeof(SAsm));
	((SAsm*)asm_)->TypeId = AsmTypeId_RET;
	((SAsm*)asm_)->Addr = NewAddr();
	return asm_;
}

SAsmSHLGroup* AsmSAR(SVal* a, SVal* b)
{
	SAsmSHLGroup* asm_ = (SAsmSHLGroup*)Alloc(sizeof(SAsmSHLGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_SAR;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Rate = 0x07;
	return asm_;
}

SAsmSHLGroup* AsmSHL(SVal* a, SVal* b)
{
	SAsmSHLGroup* asm_ = (SAsmSHLGroup*)Alloc(sizeof(SAsmSHLGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_SHL;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Rate = 0x04;
	return asm_;
}

SAsmSHLGroup* AsmSHR(SVal* a, SVal* b)
{
	SAsmSHLGroup* asm_ = (SAsmSHLGroup*)Alloc(sizeof(SAsmSHLGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_SHR;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Rate = 0x05;
	return asm_;
}

SAsmADDGroup* AsmSUB(SVal* a, SVal* b)
{
	SAsmADDGroup* asm_ = (SAsmADDGroup*)Alloc(sizeof(SAsmADDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_SUB;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Rate = 0x05;
	return asm_;
}

SAsmADDSDGroup* AsmSUBSD(SVal* a, SVal* b)
{
	SAsmADDSDGroup* asm_ = (SAsmADDSDGroup*)Alloc(sizeof(SAsmADDSDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_SUBSD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = True;
	asm_->Code = 0x5c;
	return asm_;
}

SAsm2* AsmTEST(SVal* a, SVal* b)
{
	SAsm2* asm_ = (SAsm2*)Alloc(sizeof(SAsm2));
	((SAsm*)asm_)->TypeId = AsmTypeId_TEST;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	return asm_;
}

SAsmADDGroup* AsmXOR(SVal* a, SVal* b)
{
	SAsmADDGroup* asm_ = (SAsmADDGroup*)Alloc(sizeof(SAsmADDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_XOR;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Rate = 0x06;
	return asm_;
}

SAsmCOMISDGroup* AsmXORPD(SVal* a, SVal* b)
{
	SAsmCOMISDGroup* asm_ = (SAsmCOMISDGroup*)Alloc(sizeof(SAsmCOMISDGroup));
	((SAsm*)asm_)->TypeId = AsmTypeId_XORPD;
	((SAsm*)asm_)->Addr = NewAddr();
	asm_->A = a;
	asm_->B = b;
	asm_->Double = True;
	asm_->Code = 0x57;
	return asm_;
}

void WriteAsmBin(FILE* file_ptr, SList* ref_value_list, S64 addr, const SAsm* asm_)
{
	EAsmTypeId type_id = asm_->TypeId;
	*asm_->Addr = addr;
	if ((type_id & AsmTypeId_ADDGroup) == AsmTypeId_ADDGroup)
	{
		SAsmADDGroup* asm2 = (SAsmADDGroup*)asm_;
		if (IsAX(asm2->A) && asm2->B->TypeId == ValTypeId_Imm)
			WriteAXImm(file_ptr, ref_value_list, (U8)(asm2->Rate * 0x08 + 0x04), asm2->A, asm2->B);
		else if (IsMem(asm2->A) && asm2->B->TypeId == ValTypeId_Imm)
		{
			if (asm2->A->Size != 1 && asm2->B->Size == 1)
				WriteRegImm2(file_ptr, ref_value_list, 0x83, asm2->A, asm2->B, asm2->Rate);
			else
				WriteRegImm(file_ptr, ref_value_list, 0x80, asm2->A, asm2->B, asm2->Rate);
		}
		else if (IsMem(asm2->A) && asm2->B->TypeId == ValTypeId_Reg)
			WriteMemReg(file_ptr, ref_value_list, (U8)(asm2->Rate * 0x08), asm2->A, asm2->B);
		else
		{
			ASSERT(asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B));
			WriteMemReg(file_ptr, ref_value_list, (U8)(asm2->Rate * 0x08 + 0x02), asm2->B, asm2->A);
		}
	}
	else if ((type_id & AsmTypeId_ADDSDGroup) == AsmTypeId_ADDSDGroup)
	{
		SAsmADDSDGroup* asm2 = (SAsmADDSDGroup*)asm_;
		ASSERT(asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B));
		fputc(asm2->Double ? 0xf2 : 0xf3, file_ptr);
		REX(file_ptr, asm2->B, asm2->A);
		fputc(0x0f, file_ptr);
		fputc(asm2->Code, file_ptr);
		WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
	}
	else if ((type_id & AsmTypeId_INCGroup) == AsmTypeId_INCGroup)
	{
		SAsmINCGroup* asm2 = (SAsmINCGroup*)asm_;
		ASSERT(IsMem(asm2->A));
		WriteMemOnly(file_ptr, ref_value_list, asm2->Code, asm2->A, asm2->Rate);
	}
	else if ((type_id & AsmTypeId_JEGroup) == AsmTypeId_JEGroup)
	{
		SAsmJEGroup* asm2 = (SAsmJEGroup*)asm_;
		ASSERT(asm2->A->TypeId == ValTypeId_Imm);
		if (asm2->A->Size == 1)
		{
			fputc(asm2->Code, file_ptr);
			{
				U64 addr2 = GetRefValue(((SValImm*)asm2->A)->Value, file_ptr, ref_value_list);
				fwrite(&addr2, 1, 1, file_ptr);
			}
		}
		else
		{
			ASSERT(asm2->A->Size <= 4);
			fputc(0x0f, file_ptr);
			fputc((U8)(asm2->Code + 0x10), file_ptr);
			{
				U64 addr2 = GetRefValue(((SValImm*)asm2->A)->Value, file_ptr, ref_value_list);
				fwrite(&addr2, 1, 4, file_ptr);
			}
		}
	}
	else if ((type_id & AsmTypeId_MOVSDGroup) == AsmTypeId_MOVSDGroup)
	{
		SAsmMOVSDGroup* asm2 = (SAsmMOVSDGroup*)asm_;
		fputc(asm2->Code, file_ptr);
		if (asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B))
		{
			REX(file_ptr, asm2->B, asm2->A);
			fputc(0x0f, file_ptr);
			fputc(asm2->CodeRM, file_ptr);
			WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
		}
		else
		{
			ASSERT(IsMem(asm2->A) && asm2->B->TypeId == ValTypeId_Reg);
			REX(file_ptr, asm2->A, asm2->B);
			fputc(0x0f, file_ptr);
			fputc(asm2->CodeMR, file_ptr);
			WriteValMem(file_ptr, ref_value_list, asm2->A, asm2->B);
		}
	}
	else if ((type_id & AsmTypeId_REPGroup) == AsmTypeId_REPGroup)
	{
		SAsmREPGroup* asm2 = (SAsmREPGroup*)asm_;
		fputc(0xf3, file_ptr);
		ASSERT(IsAX(asm2->A));
		REX(file_ptr, asm2->A, NULL);
		if (asm2->A->Size == 1)
			fputc(asm2->Code, file_ptr);
		else
		{
			ASSERT(asm2->A->Size == 2 || asm2->A->Size == 4 || asm2->A->Size == 8);
			fputc((U8)(asm2->Code + 1), file_ptr);
		}
	}
	else if ((type_id & AsmTypeId_SHLGroup) == AsmTypeId_SHLGroup)
	{
		SAsmSHLGroup* asm2 = (SAsmSHLGroup*)asm_;
		REX(file_ptr, asm2->A, asm2->B);
		if (IsMem(asm2->A) && asm2->B->TypeId == ValTypeId_Imm && asm2->B->Size == 1)
		{
			U64 addr2 = GetRefValue(((SValImm*)asm2->B)->Value, file_ptr, ref_value_list);
			if (addr2 == 1)
			{
				if (asm2->A->Size == 1)
					fputc(0xd0, file_ptr);
				else
				{
					ASSERT(asm2->A->Size == 2 || asm2->A->Size == 4 || asm2->A->Size == 8);
					fputc(0xd1, file_ptr);
				}
				WriteValMemS(file_ptr, ref_value_list, asm2->A, asm2->Rate);
			}
			else
			{
				if (asm2->A->Size == 1)
					fputc(0xc0, file_ptr);
				else
				{
					ASSERT(asm2->A->Size == 2 || asm2->A->Size == 4 || asm2->A->Size == 8);
					fputc(0xc1, file_ptr);
				}
				WriteValMemS(file_ptr, ref_value_list, asm2->A, asm2->Rate);
				ASSERT(addr2 == (U64)(U8)addr2);
				fputc((U8)addr2, file_ptr);
			}
		}
		else
		{
			ASSERT(IsMem(asm2->A) && asm2->B->TypeId == ValTypeId_Reg && ((SValReg*)asm2->B)->Reg == Reg_CX && asm2->B->Size == 1);
			if (asm2->A->Size == 1)
				fputc(0xd2, file_ptr);
			else
			{
				ASSERT(asm2->A->Size == 2 || asm2->A->Size == 4 || asm2->A->Size == 8);
				fputc(0xd3, file_ptr);
			}
			WriteValMemS(file_ptr, ref_value_list, asm2->A, asm2->Rate);
		}
	}
	else if ((type_id & AsmTypeId_COMISDGroup) == AsmTypeId_COMISDGroup)
	{
		SAsmCOMISDGroup* asm2 = (SAsmCOMISDGroup*)asm_;
		ASSERT(asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B));
		if (asm2->Double)
			fputc(0x66, file_ptr);
		REX(file_ptr, asm2->B, asm2->A);
		fputc(0x0f, file_ptr);
		fputc(asm2->Code, file_ptr);
		WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
	}
	else
	{
		switch (type_id)
		{
			case AsmTypeId_Label:
				break;
			case AsmTypeId_Machine:
				{
					int len = ((SAsmMachine*)asm_)->DataSize;
					const U8* data = ((SAsmMachine*)asm_)->Data;
					fwrite(data, 1, (size_t)len, file_ptr);
				}
				break;
			case AsmTypeId_CALL:
				{
					SAsm1* asm2 = (SAsm1*)asm_;
					if (asm2->A->TypeId == ValTypeId_Imm && asm2->A->Size <= 4)
					{
						fputc(0xe8, file_ptr);
						{
							U64 addr2 = GetRefValue(((SValImm*)asm2->A)->Value, file_ptr, ref_value_list);
							fwrite(&addr2, 1, 4, file_ptr);
						}
					}
					else
					{
						ASSERT(IsMem(asm2->A) && asm2->A->Size == 8);
						{
							int tmp = asm2->A->Size;
							asm2->A->Size = 4;
							REX(file_ptr, asm2->A, NULL);
							asm2->A->Size = tmp;
						}
						fputc(0xff, file_ptr);
						WriteValMemS(file_ptr, ref_value_list, asm2->A, 0x02);
					}
				}
				break;
			case AsmTypeId_CDQ:
				{
					SAsm1* asm2 = (SAsm1*)asm_;
					ASSERT(IsAX(asm2->A));
					REX(file_ptr, asm2->A, NULL);
					fputc(0x99, file_ptr);
				}
				break;
			case AsmTypeId_IMUL:
				{
					SAsm3* asm2 = (SAsm3*)asm_;
					if (IsMem(asm2->A) && asm2->B == NULL)
					{
						ASSERT(asm2->C == NULL);
						WriteMemOnly(file_ptr, ref_value_list, 0xf6, asm2->A, 0x05);
					}
					else
					{
						ASSERT(asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B));
						REX(file_ptr, asm2->B, asm2->A);
						if (asm2->C == NULL)
						{
							ASSERT(asm2->A->Size == 2 && asm2->B->Size == 2 || asm2->A->Size == 4 && asm2->B->Size == 4 || asm2->A->Size == 8 && asm2->B->Size == 8);
							fputc(0x0f, file_ptr);
							fputc(0xaf, file_ptr);
							WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
						}
						else
						{
							ASSERT(asm2->C->TypeId == ValTypeId_Imm);
							{
								U64 addr2;
								if (asm2->C->Size == 1)
								{
									ASSERT(asm2->A->Size == 2 && asm2->B->Size == 2 || asm2->A->Size == 4 && asm2->B->Size == 4 || asm2->A->Size == 8 && asm2->B->Size == 8);
									fputc(0x6b, file_ptr);
									WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
									addr2 = GetRefValue(((SValImm*)asm2->C)->Value, file_ptr, ref_value_list);
									fwrite(&addr2, 1, 1, file_ptr);
								}
								else
								{
									fputc(0x69, file_ptr);
									WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
									addr2 = GetRefValue(((SValImm*)asm2->C)->Value, file_ptr, ref_value_list);
									if (asm2->A->Size == 2 && asm2->B->Size == 2 && asm2->C->Size == 2)
										fwrite(&addr2, 1, 2, file_ptr);
									else
									{
										ASSERT((asm2->A->Size == 4 && asm2->B->Size == 4 || asm2->A->Size == 8 && asm2->B->Size == 8) && asm2->C->Size == 4);
										fwrite(&addr2, 1, 4, file_ptr);
									}
								}
							}
						}
					}
				}
				break;
			case AsmTypeId_INT:
				{
					SAsm1* asm2 = (SAsm1*)asm_;
					ASSERT(asm2->A->TypeId == ValTypeId_Imm && asm2->A->Size == 1);
					{
						SRefValue* value = ((SValImm*)asm2->A)->Value;
						if (value->Value == 0x03)
							fputc(0xcc, file_ptr);
						else
						{
							U64 addr2;
							fputc(0xcd, file_ptr);
							addr2 = GetRefValue(value, file_ptr, ref_value_list);
							fwrite(&addr2, 1, 1, file_ptr);
						}
					}
				}
				break;
			case AsmTypeId_JMP:
				{
					SAsm1* asm2 = (SAsm1*)asm_;
					if (asm2->A->TypeId == ValTypeId_Imm)
					{
						U64 addr2;
						if (asm2->A->Size == 1)
						{
							fputc(0xeb, file_ptr);
							addr2 = GetRefValue(((SValImm*)asm2->A)->Value, file_ptr, ref_value_list);
							fwrite(&addr2, 1, 1, file_ptr);
						}
						else
						{
							ASSERT(asm2->A->Size == 4);
							fputc(0xe9, file_ptr);
							addr2 = GetRefValue(((SValImm*)asm2->A)->Value, file_ptr, ref_value_list);
							fwrite(&addr2, 1, 4, file_ptr);
						}
					}
					else
					{
						ASSERT(IsMem(asm2->A) && asm2->A->Size == 8);
						fputc(0xff, file_ptr);
						WriteValMemS(file_ptr, ref_value_list, asm2->A, 0x04);
					}
				}
				break;
			case AsmTypeId_LDMXCSR:
				{
					SAsm1* asm2 = (SAsm1*)asm_;
					ASSERT(asm2->A->TypeId == ValTypeId_Mem && asm2->A->Size == 4);
					REX(file_ptr, asm2->A, NULL);
					fputc(0x0f, file_ptr);
					fputc(0xae, file_ptr);
					WriteValMemS(file_ptr, ref_value_list, asm2->A, 0x02);
				}
				break;
			case AsmTypeId_LEA:
				{
					SAsm2* asm2 = (SAsm2*)asm_;
					ASSERT(asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B));
					REX(file_ptr, asm2->B, asm2->A);
					ASSERT(asm2->A->Size == 2 && asm2->B->Size == 2 || asm2->A->Size == 4 && asm2->B->Size == 4 || asm2->A->Size == 8 && asm2->B->Size == 8);
					fputc(0x8d, file_ptr);
					WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
				}
				break;
			case AsmTypeId_MOV:
				{
					SAsm2* asm2 = (SAsm2*)asm_;
					if (IsMem(asm2->A) && asm2->B->TypeId == ValTypeId_Reg)
						WriteMemReg(file_ptr, ref_value_list, 0x88, asm2->A, asm2->B);
					else if (asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B))
						WriteMemReg(file_ptr, ref_value_list, 0x8a, asm2->B, asm2->A);
					else if (asm2->A->TypeId == ValTypeId_Reg && asm2->B->TypeId == ValTypeId_Imm)
					{
						REX(file_ptr, asm2->A, asm2->B);
						{
							U8 code = (U8)(0xb0 + GetRegCode((SValReg*)asm2->A));
							U64 addr2;
							if (asm2->A->Size == 1 && asm2->B->Size == 1)
							{
								fputc(code, file_ptr);
								addr2 = GetRefValue(((SValImm*)asm2->B)->Value, file_ptr, ref_value_list);
								fwrite(&addr2, 1, 1, file_ptr);
							}
							else if (asm2->A->Size == 2 && asm2->B->Size <= 2)
							{
								fputc((U8)(code + 0x08), file_ptr);
								addr2 = GetRefValue(((SValImm*)asm2->B)->Value, file_ptr, ref_value_list);
								fwrite(&addr2, 1, 2, file_ptr);
							}
							else if (asm2->A->Size == 4 && asm2->B->Size <= 4)
							{
								fputc((U8)(code + 0x08), file_ptr);
								addr2 = GetRefValue(((SValImm*)asm2->B)->Value, file_ptr, ref_value_list);
								fwrite(&addr2, 1, 4, file_ptr);
							}
							else
							{
								ASSERT(asm2->A->Size == 8 && asm2->B->Size <= 8);
								fputc((U8)(code + 0x08), file_ptr);
								addr2 = GetRefValue(((SValImm*)asm2->B)->Value, file_ptr, ref_value_list);
								fwrite(&addr2, 1, 8, file_ptr);
							}
						}
					}
					else
					{
						ASSERT(IsMem(asm2->A) && asm2->B->TypeId == ValTypeId_Imm);
						WriteRegImm(file_ptr, ref_value_list, 0xc6, asm2->A, asm2->B, 0x00);
					}
				}
				break;
			case AsmTypeId_MOVSX:
				{
					SAsm2* asm2 = (SAsm2*)asm_;
					ASSERT(asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B));
					{
						int tmp = asm2->B->Size;
						asm2->B->Size = asm2->A->Size;
						REX(file_ptr, asm2->B, asm2->A);
						asm2->B->Size = tmp;
					}
					if ((asm2->A->Size == 2 || asm2->A->Size == 4 || asm2->A->Size == 8) && asm2->B->Size == 1)
					{
						fputc(0x0f, file_ptr);
						fputc(0xbe, file_ptr);
						WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
					}
					else if ((asm2->A->Size == 4 || asm2->A->Size == 8) && asm2->B->Size == 2)
					{
						fputc(0x0f, file_ptr);
						fputc(0xbf, file_ptr);
						WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
					}
					else
					{
						ASSERT(asm2->A->Size == 8 && asm2->B->Size == 4);
						fputc(0x63, file_ptr);
						WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
					}
				}
				break;
			case AsmTypeId_MOVUPS:
				{
					SAsm2* asm2 = (SAsm2*)asm_;
					if (asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B))
					{
						fputc(0x0f, file_ptr);
						fputc(0x10, file_ptr);
						WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
					}
					else
					{
						ASSERT(IsMem(asm2->A) && asm2->B->TypeId == ValTypeId_Reg);
						fputc(0x0f, file_ptr);
						fputc(0x11, file_ptr);
						WriteValMem(file_ptr, ref_value_list, asm2->A, asm2->B);
					}
				}
				break;
			case AsmTypeId_MOVZX:
				{
					SAsm2* asm2 = (SAsm2*)asm_;
					ASSERT(asm2->A->TypeId == ValTypeId_Reg && IsMem(asm2->B));
					{
						int tmp = asm2->B->Size;
						asm2->B->Size = asm2->A->Size;
						REX(file_ptr, asm2->B, asm2->A);
						asm2->B->Size = tmp;
					}
					fputc(0x0f, file_ptr);
					if ((asm2->A->Size == 2 || asm2->A->Size == 4 || asm2->A->Size == 8) && asm2->B->Size == 1)
						fputc(0xb6, file_ptr);
					else
					{
						ASSERT((asm2->A->Size == 4 || asm2->A->Size == 8) && asm2->B->Size == 2);
						fputc(0xb7, file_ptr);
					}
					WriteValMem(file_ptr, ref_value_list, asm2->B, asm2->A);
				}
				break;
			case AsmTypeId_NOP:
				fputc(0x90, file_ptr);
				break;
			case AsmTypeId_RET:
				fputc(0xc3, file_ptr);
				break;
			case AsmTypeId_TEST:
				{
					SAsm2* asm2 = (SAsm2*)asm_;
					if (IsAX(asm2->A) && asm2->B->TypeId == ValTypeId_Imm)
						WriteAXImm(file_ptr, ref_value_list, 0xa8, asm2->A, asm2->B);
					else
					{
						ASSERT(IsMem(asm2->A));
						if (asm2->B->TypeId == ValTypeId_Imm)
							WriteRegImm(file_ptr, ref_value_list, 0xf6, asm2->A, asm2->B, 0x00);
						else
						{
							ASSERT(asm2->B->TypeId == ValTypeId_Reg);
							WriteMemReg(file_ptr, ref_value_list, 0x84, asm2->A, asm2->B);
						}
					}
				}
				break;
			default:
				ASSERT(False);
				break;
		}
	}
}

void Dump2(const Char* path, const SList* asms)
{
	FILE* file_ptr = _wfopen(path, L"w, ccs=UTF-8");
	SListNode* ptr = asms->Top;
	while (ptr != NULL)
	{
		SAsm* asm_ = (SAsm*)ptr->Data;
		switch (asm_->TypeId)
		{
			case AsmTypeId_Label:
#if defined(_DEBUG)
				fwprintf(file_ptr, L"%08XH: (label=%d)\n", (U32)(U64)asm_->Addr, ((SAsmLabel*)asm_)->Cnt);
#else
				fwprintf(file_ptr, L"%08XH:\n", (U32)(U64)asm_->Addr);
#endif
				ptr = ptr->Next;
				continue;
			case AsmTypeId_Machine:
				fwprintf(file_ptr, L"  Machine\n");
				{
					SAsmMachine* asm2 = (SAsmMachine*)asm_;
					int i;
					for (i = 0; i < asm2->DataSize; i++)
					{
						if (i % 16 == 0)
							fwprintf(file_ptr, L"    %02X", asm2->Data[i]);
						else
							fwprintf(file_ptr, L" %02X", asm2->Data[i]);
						if (i % 16 == 15 || i == asm2->DataSize - 1)
							fwprintf(file_ptr, L"\n");
					}
				}
				ptr = ptr->Next;
				continue;
			case AsmTypeId_ADD: fwprintf(file_ptr, L"  ADD"); break;
			case AsmTypeId_ADDSD: fwprintf(file_ptr, L"  ADDSD"); break;
			case AsmTypeId_AND: fwprintf(file_ptr, L"  AND"); break;
			case AsmTypeId_CALL: fwprintf(file_ptr, L"  CALL"); break;
			case AsmTypeId_CDQ: fwprintf(file_ptr, L"  CDQ"); break;
			case AsmTypeId_CMP: fwprintf(file_ptr, L"  CMP"); break;
			case AsmTypeId_COMISD: fwprintf(file_ptr, L"  COMISD"); break;
			case AsmTypeId_CVTSD2SI: fwprintf(file_ptr, L"  CVTSD2SI"); break;
			case AsmTypeId_CVTSD2SS: fwprintf(file_ptr, L"  CVTSD2SS"); break;
			case AsmTypeId_CVTSI2SD: fwprintf(file_ptr, L"  CVTSI2SD"); break;
			case AsmTypeId_CVTSS2SD: fwprintf(file_ptr, L"  CVTSS2SD"); break;
			case AsmTypeId_DEC: fwprintf(file_ptr, L"  DEC"); break;
			case AsmTypeId_DIV: fwprintf(file_ptr, L"  DIV"); break;
			case AsmTypeId_DIVSD: fwprintf(file_ptr, L"  DIVSD"); break;
			case AsmTypeId_IDIV: fwprintf(file_ptr, L"  IDIV"); break;
			case AsmTypeId_IMUL: fwprintf(file_ptr, L"  IMUL"); break;
			case AsmTypeId_INC: fwprintf(file_ptr, L"  INC"); break;
			case AsmTypeId_INT: fwprintf(file_ptr, L"  INT"); break;
			case AsmTypeId_JA: fwprintf(file_ptr, L"  JA"); break;
			case AsmTypeId_JAE: fwprintf(file_ptr, L"  JAE"); break;
			case AsmTypeId_JB: fwprintf(file_ptr, L"  JB"); break;
			case AsmTypeId_JBE: fwprintf(file_ptr, L"  JBE"); break;
			case AsmTypeId_JE: fwprintf(file_ptr, L"  JE"); break;
			case AsmTypeId_JG: fwprintf(file_ptr, L"  JG"); break;
			case AsmTypeId_JGE: fwprintf(file_ptr, L"  JGE"); break;
			case AsmTypeId_JL: fwprintf(file_ptr, L"  JL"); break;
			case AsmTypeId_JLE: fwprintf(file_ptr, L"  JLE"); break;
			case AsmTypeId_JMP: fwprintf(file_ptr, L"  JMP"); break;
			case AsmTypeId_JNE: fwprintf(file_ptr, L"  JNE"); break;
			case AsmTypeId_JNO: fwprintf(file_ptr, L"  JNO"); break;
			case AsmTypeId_JNS: fwprintf(file_ptr, L"  JNS"); break;
			case AsmTypeId_JO: fwprintf(file_ptr, L"  JO"); break;
			case AsmTypeId_JS: fwprintf(file_ptr, L"  JS"); break;
			case AsmTypeId_LDMXCSR: fwprintf(file_ptr, L"  LDMXCSR"); break;
			case AsmTypeId_LEA: fwprintf(file_ptr, L"  LEA"); break;
			case AsmTypeId_MOV: fwprintf(file_ptr, L"  MOV"); break;
			case AsmTypeId_MOVSD: fwprintf(file_ptr, L"  MOVSD"); break;
			case AsmTypeId_MOVSS: fwprintf(file_ptr, L"  MOVSS"); break;
			case AsmTypeId_MOVSX: fwprintf(file_ptr, L"  MOVSX"); break;
			case AsmTypeId_MOVUPS: fwprintf(file_ptr, L"  MOVUPS"); break;
			case AsmTypeId_MOVZX: fwprintf(file_ptr, L"  MOVZX"); break;
			case AsmTypeId_MULSD: fwprintf(file_ptr, L"  MULSD"); break;
			case AsmTypeId_NEG: fwprintf(file_ptr, L"  NEG"); break;
			case AsmTypeId_NOP: fwprintf(file_ptr, L"  NOP"); break;
			case AsmTypeId_NOT: fwprintf(file_ptr, L"  NOT"); break;
			case AsmTypeId_OR: fwprintf(file_ptr, L"  OR"); break;
			case AsmTypeId_REPMOVS: fwprintf(file_ptr, L"  REPMOVS"); break;
			case AsmTypeId_REPSTOS: fwprintf(file_ptr, L"  REPSTOS"); break;
			case AsmTypeId_RET: fwprintf(file_ptr, L"  RET"); break;
			case AsmTypeId_SAR: fwprintf(file_ptr, L"  SAR"); break;
			case AsmTypeId_SHL: fwprintf(file_ptr, L"  SHL"); break;
			case AsmTypeId_SHR: fwprintf(file_ptr, L"  SHR"); break;
			case AsmTypeId_SUB: fwprintf(file_ptr, L"  SUB"); break;
			case AsmTypeId_SUBSD: fwprintf(file_ptr, L"  SUBSD"); break;
			case AsmTypeId_TEST: fwprintf(file_ptr, L"  TEST"); break;
			case AsmTypeId_XOR: fwprintf(file_ptr, L"  XOR"); break;
			case AsmTypeId_XORPD: fwprintf(file_ptr, L"  XORPD"); break;
			default:
				ASSERT(False);
				break;
		}
		if ((asm_->TypeId & AsmTypeId_ADDGroup) == AsmTypeId_ADDGroup)
		{
			SAsmADDGroup* asm2 = (SAsmADDGroup*)asm_;
			fwprintf(file_ptr, L" ");
			DumpVal(file_ptr, asm2->A);
			fwprintf(file_ptr, L",");
			DumpVal(file_ptr, asm2->B);
		}
		else if ((asm_->TypeId & AsmTypeId_ADDSDGroup) == AsmTypeId_ADDSDGroup)
		{
			SAsmADDSDGroup* asm2 = (SAsmADDSDGroup*)asm_;
			fwprintf(file_ptr, L" ");
			DumpVal(file_ptr, asm2->A);
			fwprintf(file_ptr, L",");
			DumpVal(file_ptr, asm2->B);
		}
		else if ((asm_->TypeId & AsmTypeId_INCGroup) == AsmTypeId_INCGroup)
		{
			SAsmINCGroup* asm2 = (SAsmINCGroup*)asm_;
			fwprintf(file_ptr, L" ");
			DumpVal(file_ptr, asm2->A);
		}
		else if ((asm_->TypeId & AsmTypeId_JEGroup) == AsmTypeId_JEGroup)
		{
			SAsmJEGroup* asm2 = (SAsmJEGroup*)asm_;
			fwprintf(file_ptr, L" ");
			DumpVal(file_ptr, asm2->A);
		}
		else if ((asm_->TypeId & AsmTypeId_MOVSDGroup) == AsmTypeId_MOVSDGroup)
		{
			SAsmMOVSDGroup* asm2 = (SAsmMOVSDGroup*)asm_;
			fwprintf(file_ptr, L" ");
			DumpVal(file_ptr, asm2->A);
			fwprintf(file_ptr, L",");
			DumpVal(file_ptr, asm2->B);
		}
		else if ((asm_->TypeId & AsmTypeId_REPGroup) == AsmTypeId_REPGroup)
		{
			SAsmREPGroup* asm2 = (SAsmREPGroup*)asm_;
			fwprintf(file_ptr, L" ");
			DumpVal(file_ptr, asm2->A);
		}
		else if ((asm_->TypeId & AsmTypeId_SHLGroup) == AsmTypeId_SHLGroup)
		{
			SAsmSHLGroup* asm2 = (SAsmSHLGroup*)asm_;
			fwprintf(file_ptr, L" ");
			DumpVal(file_ptr, asm2->A);
			fwprintf(file_ptr, L",");
			DumpVal(file_ptr, asm2->B);
		}
		else if ((asm_->TypeId & AsmTypeId_COMISDGroup) == AsmTypeId_COMISDGroup)
		{
			SAsmCOMISDGroup* asm2 = (SAsmCOMISDGroup*)asm_;
			fwprintf(file_ptr, L" ");
			DumpVal(file_ptr, asm2->A);
			fwprintf(file_ptr, L",");
			DumpVal(file_ptr, asm2->B);
		}
		else
		{
			switch (asm_->TypeId)
			{
				case AsmTypeId_NOP:
				case AsmTypeId_RET:
					// Do nothing.
					break;
				case AsmTypeId_CALL:
				case AsmTypeId_CDQ:
				case AsmTypeId_INT:
				case AsmTypeId_JMP:
				case AsmTypeId_LDMXCSR:
					{
						SAsm1* asm2 = (SAsm1*)asm_;
						fwprintf(file_ptr, L" ");
						DumpVal(file_ptr, asm2->A);
					}
					break;
				case AsmTypeId_LEA:
				case AsmTypeId_MOV:
				case AsmTypeId_MOVSX:
				case AsmTypeId_MOVUPS:
				case AsmTypeId_MOVZX:
				case AsmTypeId_TEST:
					{
						SAsm2* asm2 = (SAsm2*)asm_;
						fwprintf(file_ptr, L" ");
						DumpVal(file_ptr, asm2->A);
						fwprintf(file_ptr, L",");
						DumpVal(file_ptr, asm2->B);
					}
					break;
				case AsmTypeId_IMUL:
					{
						SAsm3* asm2 = (SAsm3*)asm_;
						fwprintf(file_ptr, L" ");
						DumpVal(file_ptr, asm2->A);
						fwprintf(file_ptr, L",");
						DumpVal(file_ptr, asm2->B);
						if (asm2->C != NULL)
						{
							fwprintf(file_ptr, L",");
							DumpVal(file_ptr, asm2->C);
						}
					}
					break;
				default:
					ASSERT(False);
					break;
			}
		}
		fwprintf(file_ptr, L"\n");
		ptr = ptr->Next;
	}
	fclose(file_ptr);
}

static void DumpVal(FILE* file_ptr, const SVal* val)
{
	switch (val->TypeId)
	{
		case ValTypeId_Imm:
			{
				SValImm* val2 = (SValImm*)val;
				if (val2->Value->TypeId == RefValueTypeId_Addr)
					fwprintf(file_ptr, L"(%08XH)", (U32)(U64)((SRefValueAddr*)val2->Value)->Addr);
				else
				{
					U64 addr = val2->Value->Value;
					switch (val->Size)
					{
						case 1: fwprintf(file_ptr, L"%02XH", (U8)addr); break;
						case 2: fwprintf(file_ptr, L"%04XH", (U16)addr); break;
						case 4: fwprintf(file_ptr, L"%08XH", (U32)addr); break;
						case 8: fwprintf(file_ptr, L"%016I64XH", addr); break;
						default:
							ASSERT(False);
							break;
					}
				}
			}
			break;
		case ValTypeId_Reg:
			{
				SValReg* val2 = (SValReg*)val;
				switch (val2->Reg)
				{
					case Reg_AX:
					case Reg_CX:
					case Reg_DX:
					case Reg_BX:
					case Reg_SP:
					case Reg_BP:
					case Reg_SI:
					case Reg_DI:
					case Reg_SPL:
					case Reg_BPL:
					case Reg_SIL:
					case Reg_DIL:
						if (val->Size == 1)
						{
							switch (val2->Reg)
							{
								case Reg_AX: fwprintf(file_ptr, L"AL"); break;
								case Reg_CX: fwprintf(file_ptr, L"CL"); break;
								case Reg_DX: fwprintf(file_ptr, L"DL"); break;
								case Reg_BX: fwprintf(file_ptr, L"BL"); break;
								case Reg_SP: fwprintf(file_ptr, L"SPL"); break;
								case Reg_BP: fwprintf(file_ptr, L"BPL"); break;
								case Reg_SI: fwprintf(file_ptr, L"SIL"); break;
								case Reg_DI: fwprintf(file_ptr, L"DIL"); break;
								case Reg_SPL: fwprintf(file_ptr, L"SPL"); break;
								case Reg_BPL: fwprintf(file_ptr, L"BPL"); break;
								case Reg_SIL: fwprintf(file_ptr, L"SIL"); break;
								case Reg_DIL: fwprintf(file_ptr, L"DIL"); break;
								default:
									ASSERT(False);
									break;
							}
						}
						else
						{
							if (val->Size == 8)
								fwprintf(file_ptr, L"R");
							else if (val->Size == 4)
								fwprintf(file_ptr, L"E");
							switch (val2->Reg)
							{
								case Reg_AX: fwprintf(file_ptr, L"AX"); break;
								case Reg_CX: fwprintf(file_ptr, L"CX"); break;
								case Reg_DX: fwprintf(file_ptr, L"DX"); break;
								case Reg_BX: fwprintf(file_ptr, L"BX"); break;
								case Reg_SP: fwprintf(file_ptr, L"SP"); break;
								case Reg_BP: fwprintf(file_ptr, L"BP"); break;
								case Reg_SI: fwprintf(file_ptr, L"SI"); break;
								case Reg_DI: fwprintf(file_ptr, L"DI"); break;
								default:
									ASSERT(False);
									break;
							}
						}
						break;
					case Reg_R8:
					case Reg_R9:
					case Reg_R10:
					case Reg_R11:
					case Reg_R12:
					case Reg_R13:
					case Reg_R14:
					case Reg_R15:
						switch (val2->Reg)
						{
							case Reg_R8: fwprintf(file_ptr, L"R8"); break;
							case Reg_R9: fwprintf(file_ptr, L"R9"); break;
							case Reg_R10: fwprintf(file_ptr, L"R10"); break;
							case Reg_R11: fwprintf(file_ptr, L"R11"); break;
							case Reg_R12: fwprintf(file_ptr, L"R12"); break;
							case Reg_R13: fwprintf(file_ptr, L"R13"); break;
							case Reg_R14: fwprintf(file_ptr, L"R14"); break;
							case Reg_R15: fwprintf(file_ptr, L"R15"); break;
							default:
								ASSERT(False);
								break;
						}
						if (val->Size == 1)
							fwprintf(file_ptr, L"B");
						else if (val->Size == 2)
							fwprintf(file_ptr, L"W");
						else if (val->Size == 4)
							fwprintf(file_ptr, L"D");
						break;
					default:
						switch (val2->Reg)
						{
							case Reg_XMM0: fwprintf(file_ptr, L"XMM0"); break;
							case Reg_XMM1: fwprintf(file_ptr, L"XMM1"); break;
							case Reg_XMM2: fwprintf(file_ptr, L"XMM2"); break;
							case Reg_XMM3: fwprintf(file_ptr, L"XMM3"); break;
							case Reg_XMM4: fwprintf(file_ptr, L"XMM4"); break;
							case Reg_XMM5: fwprintf(file_ptr, L"XMM5"); break;
							case Reg_XMM6: fwprintf(file_ptr, L"XMM6"); break;
							case Reg_XMM7: fwprintf(file_ptr, L"XMM7"); break;
							case Reg_XMM8: fwprintf(file_ptr, L"XMM8"); break;
							case Reg_XMM9: fwprintf(file_ptr, L"XMM9"); break;
							case Reg_XMM10: fwprintf(file_ptr, L"XMM10"); break;
							case Reg_XMM11: fwprintf(file_ptr, L"XMM11"); break;
							case Reg_XMM12: fwprintf(file_ptr, L"XMM12"); break;
							case Reg_XMM13: fwprintf(file_ptr, L"XMM13"); break;
							case Reg_XMM14: fwprintf(file_ptr, L"XMM14"); break;
							case Reg_XMM15: fwprintf(file_ptr, L"XMM15"); break;
							default:
								ASSERT(False);
						}
						break;
				}
			}
			break;
		case ValTypeId_Mem:
			{
				SValMem* val2 = (SValMem*)val;
				switch (val->Size)
				{
					case 1: fwprintf(file_ptr, L"BYTE PTR ["); break;
					case 2: fwprintf(file_ptr, L"WORD PTR ["); break;
					case 4: fwprintf(file_ptr, L"DWORD PTR ["); break;
					case 8: fwprintf(file_ptr, L"QWORD PTR ["); break;
					default:
						ASSERT(False);
						break;
				}
				{
					Bool wrote = False;
					if (val2->Base != NULL)
					{
						DumpVal(file_ptr, (SVal*)val2->Base);
						wrote = True;
					}
					if (val2->Idx != NULL)
					{
						if (wrote)
							fwprintf(file_ptr, L"+");
						DumpVal(file_ptr, (SVal*)val2->Idx);
						wrote = True;
					}
					if (val2->Disp != NULL)
					{
						if (wrote)
							fwprintf(file_ptr, L"+");
						if (val2->Disp->TypeId == RefValueTypeId_Addr)
							fwprintf(file_ptr, L"(%08XH)", (U32)(U64)((SRefValueAddr*)val2->Disp)->Addr);
						else
						{
							U64 addr = val2->Disp->Value;
							if (addr == (U64)(U8)addr)
								fwprintf(file_ptr, L"%02XH", (U8)addr);
							else if (addr == (U64)(U16)addr)
								fwprintf(file_ptr, L"%04XH", (U16)addr);
							else if (addr == (U64)(U32)addr)
								fwprintf(file_ptr, L"%08XH", (U32)addr);
							else
								fwprintf(file_ptr, L"%016I64XH", addr);
						}
					}
				}
				fwprintf(file_ptr, L"]");
			}
			break;
		case ValTypeId_RIP:
			{
				SValRIP* val2 = (SValRIP*)val;
				if (val2->Disp->TypeId == RefValueTypeId_Addr)
					fwprintf(file_ptr, L"(%08XH)", (U32)(U64)((SRefValueAddr*)val2->Disp)->Addr);
				else
				{
					U64 addr = val2->Disp->Value;
					switch (val->Size)
					{
						case 1: fwprintf(file_ptr, L"%02XH", (U8)addr); break;
						case 2: fwprintf(file_ptr, L"%04XH", (U16)addr); break;
						case 4: fwprintf(file_ptr, L"%08XH", (U32)addr); break;
						case 8: fwprintf(file_ptr, L"%016I64XH", addr); break;
						default:
							ASSERT(False);
							break;
					}
				}
			}
			break;
		default:
			ASSERT(False);
	}
}

static Bool IsXMM(const SValReg* reg)
{
	return (reg->Reg & 0xf8) == 0x18 || (reg->Reg & 0xf8) == 0x20;
}

static Bool IsAX(const SVal* val)
{
	return val->TypeId == ValTypeId_Reg && ((SValReg*)val)->Reg == Reg_AX;
}

static Bool IsMem(const SVal* val)
{
	switch (val->TypeId)
	{
		case ValTypeId_Reg:
		case ValTypeId_Mem:
		case ValTypeId_RIP:
			return True;
	}
	return False;
}

static Bool IsRXReg(const SValReg* reg)
{
	return (reg->Reg & 0xf8) == 0x08 || ((reg->Reg & 0xf8) == 0x20);
}

static Bool IsSPLReg(const SValReg* reg)
{
	return (reg->Reg & 0xf8) == 0x10;
}

static Bool IsXMMReg(const SValReg* reg)
{
	return (reg->Reg & 0xf8) == 0x18 || (reg->Reg & 0xf8) == 0x20;
}

static U8 GetRegCode(const SValReg* reg)
{
	return (U8)(reg->Reg & 0x07);
}

static void REX(FILE* file_ptr, const SVal* a, const SVal* b)
{
	U8 bin = 0x40;
	Bool prefix = False;
	if (a->TypeId == ValTypeId_Reg)
	{
		const SValReg* a2 = (const SValReg*)a;
		if (IsRXReg(a2))
		{
			bin |= 0x01;
			prefix = True;
		}
		else if (IsSPLReg(a2))
			prefix = True;
		else if (b != NULL && b->TypeId == ValTypeId_Reg && IsSPLReg((SValReg*)b))
		{
			ASSERT(!IsSPLReg(a2));
			prefix = True;
		}
	}
	else if (a->TypeId == ValTypeId_Mem)
	{
		const SValMem* a2 = (const SValMem*)a;
		if (a2->Base != NULL)
		{
			if (IsRXReg(a2->Base))
			{
				bin |= 0x01;
				prefix = True;
			}
			else if (IsSPLReg(a2->Base))
				prefix = True;
			else if (b != NULL && b->TypeId == ValTypeId_Reg && IsSPLReg((SValReg*)b))
			{
				ASSERT(!IsSPLReg(a2->Base));
				prefix = True;
			}
		}
		if (a2->Idx != NULL && IsRXReg(a2->Idx))
		{
			bin |= 0x02;
			prefix = True;
		}
	}
	if (b != NULL && b->TypeId == ValTypeId_Reg && IsRXReg((SValReg*)b))
	{
		bin |= 0x04;
		prefix = True;
	}
	if (a->Size == 8)
	{
		bin |= 0x08;
		prefix = True;
	}
	if (a->Size == 2)
		fputc(0x66, file_ptr);
	if (prefix)
		fputc(bin, file_ptr);
}

static U64 GetRefValue(SRefValue* value, FILE* file_ptr, SList* ref_value_list)
{
	if (value->TypeId == RefValueTypeId_Addr)
	{
		// 'ref_value_list' is set to 'NULL' when the address is to be updated later, not now.
		if (ref_value_list != NULL)
		{
			((SRefValueAddr*)value)->Pos = _ftelli64(file_ptr) - 0x0400 + 0x1000;
			ListAdd(ref_value_list, value);
		}
		return 0xffffffff;
	}
	ASSERT(value->TypeId == RefValueTypeId_RefValue);
	return value->Value;
}

static void WriteValMemS(FILE* file_ptr, SList* ref_value_list, const SVal* a, U8 b)
{
	if (a->TypeId == ValTypeId_Reg)
		fputc((U8)((3 << 6) | (b << 3) | GetRegCode((SValReg*)a)), file_ptr);
	else if (a->TypeId == ValTypeId_Mem)
	{
		SValMem* a2 = (SValMem*)a;
		int disp_range = 0;
		S64 disp = (S64)GetRefValue(a2->Disp, file_ptr, NULL);
		if (a2->Base == NULL)
		{
			U8 idx;
			fputc((U8)((0 << 6) | (b << 3) | 4), file_ptr);
			if (a2->Idx == NULL)
				idx = 4;
			else
			{
				ASSERT(a2->Idx->Reg != Reg_SP);
				idx = GetRegCode(a2->Idx);
			}
			fputc((U8)((0 << 6) | (idx << 3) | 5), file_ptr);
			disp_range = 4;
			ASSERT(disp == (S64)(S32)disp);
		}
		else
		{
			U8 mod;
			if (disp == 0)
				mod = 0;
			else if (disp == (S64)(S8)disp)
				mod = 1;
			else
			{
				ASSERT(disp == (S64)(S32)disp || disp == 0xffffffff);
				mod = 2;
			}
			if (mod == 0 && (a2->Base->Reg == Reg_BP || a2->Base->Reg == Reg_R13))
				mod = 1;
			disp_range = (int)mod;
			if (a2->Idx == NULL)
			{
				if (a2->Base->Reg == Reg_SP || a2->Base->Reg == Reg_R12)
				{
					fputc((U8)((mod << 6) | (b << 3) | 4), file_ptr);
					fputc((U8)((0 << 6) | (4 << 3) | GetRegCode(a2->Base)), file_ptr);
				}
				else
					fputc((U8)((mod << 6) | (b << 3) | GetRegCode(a2->Base)), file_ptr);
			}
			else
			{
				U8 scale = 0;
				ASSERT(a2->Idx->Reg != Reg_SP);
				fputc((U8)((mod << 6) | (b << 3) | 4), file_ptr);
				switch (((SVal*)a2->Idx)->Size)
				{
					case 1: scale = 0; break;
					case 2: scale = 1; break;
					case 4: scale = 2; break;
					case 8: scale = 3; break;
					default:
						ASSERT(False);
						break;
				}
				fputc((U8)((scale << 6) | (GetRegCode(a2->Idx) << 3) | GetRegCode(a2->Base)), file_ptr);
			}
		}
		GetRefValue(a2->Disp, file_ptr, ref_value_list);
		if (disp_range == 1)
			fputc((U8)(S8)disp, file_ptr);
		else if (disp_range == 2)
		{
			U32 disp2 = (U32)(S32)disp;
			fwrite(&disp2, 1, 4, file_ptr);
		}
	}
	else
	{
		ASSERT(a->TypeId == ValTypeId_RIP);
		fputc((U8)((0 << 6) | (b << 3) | 5), file_ptr);
		{
			S64 disp = (S64)GetRefValue(((SValRIP*)a)->Disp, file_ptr, ref_value_list);
			U32 disp2 = (U32)(S32)disp;
			ASSERT(disp == (S64)(S32)disp || disp == 0xffffffff);
			fwrite(&disp2, 1, 4, file_ptr);
		}
	}
}

static void WriteValMem(FILE* file_ptr, SList* ref_value_list, const SVal* a, const SVal* b)
{
	ASSERT(b->TypeId == ValTypeId_Reg);
	WriteValMemS(file_ptr, ref_value_list, a, GetRegCode((SValReg*)b));
}

static void WriteAXImm(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, const SVal* b)
{
	U64 addr;
	ASSERT(b->TypeId == ValTypeId_Imm);
	REX(file_ptr, a, b);
	if (a->Size == 1 && b->Size == 1)
	{
		fputc(code, file_ptr);
		addr = GetRefValue(((SValImm*)b)->Value, file_ptr, ref_value_list);
		fwrite(&addr, 1, 1, file_ptr);
	}
	else if (a->Size == 2 && b->Size <= 2)
	{
		fputc((U8)(code + 1), file_ptr);
		addr = GetRefValue(((SValImm*)b)->Value, file_ptr, ref_value_list);
		fwrite(&addr, 1, 2, file_ptr);
	}
	else
	{
		ASSERT((a->Size == 4 || a->Size == 8) && b->Size <= 4);
		fputc((U8)(code + 1), file_ptr);
		addr = GetRefValue(((SValImm*)b)->Value, file_ptr, ref_value_list);
		fwrite(&addr, 1, 4, file_ptr);
	}
}

static void WriteRegImm(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, const SVal* b, U8 rate)
{
	U64 addr;
	ASSERT(b->TypeId == ValTypeId_Imm);
	REX(file_ptr, a, b);
	if (a->Size == 1 && b->Size == 1)
	{
		fputc(code, file_ptr);
		WriteValMemS(file_ptr, ref_value_list, a, rate);
		addr = GetRefValue(((SValImm*)b)->Value, file_ptr, ref_value_list);
		fwrite(&addr, 1, 1, file_ptr);
	}
	else if (a->Size == 2 && b->Size <= 2)
	{
		fputc((U8)(code + 1), file_ptr);
		WriteValMemS(file_ptr, ref_value_list, a, rate);
		addr = GetRefValue(((SValImm*)b)->Value, file_ptr, ref_value_list);
		fwrite(&addr, 1, 2, file_ptr);
	}
	else
	{
		ASSERT((a->Size == 4 || a->Size == 8) && b->Size <= 4);
		fputc((U8)(code + 1), file_ptr);
		WriteValMemS(file_ptr, ref_value_list, a, rate);
		addr = GetRefValue(((SValImm*)b)->Value, file_ptr, ref_value_list);
		fwrite(&addr, 1, 4, file_ptr);
	}
}

static void WriteRegImm2(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, const SVal* b, U8 rate)
{
	U64 addr;
	ASSERT(b->TypeId == ValTypeId_Imm);
	REX(file_ptr, a, b);
	ASSERT((a->Size == 2 || a->Size == 4 || a->Size == 8) && b->Size == 1);
	fputc(code, file_ptr);
	WriteValMemS(file_ptr, ref_value_list, a, rate);
	addr = GetRefValue(((SValImm*)b)->Value, file_ptr, ref_value_list);
	fwrite(&addr, 1, 1, file_ptr);
}

static void WriteMemReg(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, const SVal* b)
{
	ASSERT(b->TypeId == ValTypeId_Reg);
	REX(file_ptr, a, b);
	if (a->Size == 1 && b->Size == 1)
		fputc(code, file_ptr);
	else
	{
		ASSERT(a->Size == 2 && b->Size == 2 || a->Size == 4 && b->Size == 4 || a->Size == 8 && b->Size == 8);
		fputc((U8)(code + 1), file_ptr);
	}
	WriteValMem(file_ptr, ref_value_list, a, b);
}

static void WriteMemOnly(FILE* file_ptr, SList* ref_value_list, U8 code, const SVal* a, U8 rate)
{
	REX(file_ptr, a, NULL);
	if (a->Size == 1)
		fputc(code, file_ptr);
	else
	{
		ASSERT(a->Size == 2 || a->Size == 4 || a->Size == 8);
		fputc((U8)(code + 1), file_ptr);
	}
	WriteValMemS(file_ptr, ref_value_list, a, rate);
}
