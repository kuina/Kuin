#pragma once

#include "..\common.h"

#include "list.h"

typedef enum ERefValueTypeId
{
	RefValueTypeId_RefValue = 0x00,
	RefValueTypeId_Addr = 0x01,
} ERefValueTypeId;

typedef struct SRefValue
{
	ERefValueTypeId TypeId;
	U64 Value;
} SRefValue;

typedef struct SRefValueAddr
{
	SRefValue RefValue;
	S64 Pos;
	S64 Bottom; // The address at the end of operation.
	S64* Addr;
	Bool Relative;
} SRefValueAddr;

typedef enum EValTypeId
{
	ValTypeId_Val = 0x00,
	ValTypeId_Imm = 0x01,
	ValTypeId_Reg = 0x02,
	ValTypeId_Mem = 0x03,
	ValTypeId_RIP = 0x04,
} EValTypeId;

typedef struct SVal
{
	EValTypeId TypeId;
	int Size;
} SVal;

typedef struct SValImm
{
	SVal Val;
	SRefValue* Value;
} SValImm;

typedef enum EReg
{
	Reg_AX = 0x00 | 0,
	Reg_CX = 0x00 | 1,
	Reg_DX = 0x00 | 2,
	Reg_BX = 0x00 | 3,
	Reg_SP = 0x00 | 4,
	Reg_BP = 0x00 | 5,
	Reg_SI = 0x00 | 6,
	Reg_DI = 0x00 | 7,
	Reg_R8 = 0x08 | 0,
	Reg_R9 = 0x08 | 1,
	Reg_R10 = 0x08 | 2,
	Reg_R11 = 0x08 | 3,
	Reg_R12 = 0x08 | 4,
	Reg_R13 = 0x08 | 5,
	Reg_R14 = 0x08 | 6,
	Reg_R15 = 0x08 | 7,
	Reg_SPL = 0x10 | 4,
	Reg_BPL = 0x10 | 5,
	Reg_SIL = 0x10 | 6,
	Reg_DIL = 0x10 | 7,
	Reg_XMM0 = 0x18 | 0,
	Reg_XMM1 = 0x18 | 1,
	Reg_XMM2 = 0x18 | 2,
	Reg_XMM3 = 0x18 | 3,
	Reg_XMM4 = 0x18 | 4,
	Reg_XMM5 = 0x18 | 5,
	Reg_XMM6 = 0x18 | 6,
	Reg_XMM7 = 0x18 | 7,
	Reg_XMM8 = 0x20 | 0,
	Reg_XMM9 = 0x20 | 1,
	Reg_XMM10 = 0x20 | 2,
	Reg_XMM11 = 0x20 | 3,
	Reg_XMM12 = 0x20 | 4,
	Reg_XMM13 = 0x20 | 5,
	Reg_XMM14 = 0x20 | 6,
	Reg_XMM15 = 0x20 | 7,
} EReg;

typedef struct SValReg
{
	SVal Val;
	EReg Reg;
} SValReg;

typedef struct SValMem
{
	SVal Val;
	SValReg* Base;
	SValReg* Idx;
	SRefValue* Disp;
} SValMem;

typedef struct SValRIP
{
	SVal Val;
	SRefValue* Disp;
} SValRIP;

typedef enum EAsmTypeId
{
	AsmTypeId_Asm = 0x00,
	AsmTypeId_Label = 0x01,
	AsmTypeId_Machine = 0x02,
	AsmTypeId_ADDGroup = 0x0100,
	AsmTypeId_ADDSDGroup = 0x0200,
	AsmTypeId_INCGroup = 0x0400,
	AsmTypeId_JEGroup = 0x0800,
	AsmTypeId_MOVSDGroup = 0x1000,
	AsmTypeId_REPGroup = 0x2000,
	AsmTypeId_SHLGroup = 0x4000,
	AsmTypeId_COMISDGroup = 0x8000,
	AsmTypeId_ADD = AsmTypeId_ADDGroup | 0x01,
	AsmTypeId_ADDSD = AsmTypeId_ADDSDGroup | 0x01,
	AsmTypeId_AND = AsmTypeId_ADDGroup | 0x02,
	AsmTypeId_CALL = 0x03,
	AsmTypeId_CDQ = 0x04,
	AsmTypeId_CMP = AsmTypeId_ADDGroup | 0x03,
	AsmTypeId_COMISD = AsmTypeId_COMISDGroup | 0x01,
	AsmTypeId_CVTSD2SI = AsmTypeId_ADDSDGroup | 0x02,
	AsmTypeId_CVTSD2SS = AsmTypeId_ADDSDGroup | 0x03,
	AsmTypeId_CVTSI2SD = AsmTypeId_ADDSDGroup | 0x04,
	AsmTypeId_CVTSS2SD = AsmTypeId_ADDSDGroup | 0x05,
	AsmTypeId_DEC = AsmTypeId_INCGroup | 0x01,
	AsmTypeId_DIV = AsmTypeId_INCGroup | 0x02,
	AsmTypeId_DIVSD = AsmTypeId_ADDSDGroup | 0x06,
	AsmTypeId_IDIV = AsmTypeId_INCGroup | 0x03,
	AsmTypeId_IMUL = 0x05,
	AsmTypeId_INC = AsmTypeId_INCGroup | 0x04,
	AsmTypeId_INT = 0x06,
	AsmTypeId_JA = AsmTypeId_JEGroup | 0x01,
	AsmTypeId_JAE = AsmTypeId_JEGroup | 0x02,
	AsmTypeId_JB = AsmTypeId_JEGroup | 0x03,
	AsmTypeId_JBE = AsmTypeId_JEGroup | 0x04,
	AsmTypeId_JE = AsmTypeId_JEGroup | 0x05,
	AsmTypeId_JG = AsmTypeId_JEGroup | 0x06,
	AsmTypeId_JGE = AsmTypeId_JEGroup | 0x07,
	AsmTypeId_JL = AsmTypeId_JEGroup | 0x08,
	AsmTypeId_JLE = AsmTypeId_JEGroup | 0x09,
	AsmTypeId_JMP = 0x07,
	AsmTypeId_JNE = AsmTypeId_JEGroup | 0x0a,
	AsmTypeId_JNO = AsmTypeId_JEGroup | 0x0b,
	AsmTypeId_JNS = AsmTypeId_JEGroup | 0x0c,
	AsmTypeId_JO = AsmTypeId_JEGroup | 0x0d,
	AsmTypeId_JS = AsmTypeId_JEGroup | 0x0e,
	AsmTypeId_LDMXCSR = 0x08,
	AsmTypeId_LEA = 0x09,
	AsmTypeId_MOV = 0x0a,
	AsmTypeId_MOVSD = AsmTypeId_MOVSDGroup | 0x01,
	AsmTypeId_MOVSS = AsmTypeId_MOVSDGroup | 0x02,
	AsmTypeId_MOVSX = 0x0b,
	AsmTypeId_MOVUPS = 0x0c,
	AsmTypeId_MOVZX = 0x0d,
	AsmTypeId_MULSD = AsmTypeId_ADDSDGroup | 0x07,
	AsmTypeId_NEG = AsmTypeId_INCGroup | 0x05,
	AsmTypeId_NOP = 0x0e,
	AsmTypeId_NOT = AsmTypeId_INCGroup | 0x06,
	AsmTypeId_OR = AsmTypeId_ADDGroup | 0x04,
	AsmTypeId_REPMOVS = AsmTypeId_REPGroup | 0x01,
	AsmTypeId_REPSTOS = AsmTypeId_REPGroup | 0x02,
	AsmTypeId_RET = 0x0f,
	AsmTypeId_SAR = AsmTypeId_SHLGroup | 0x01,
	AsmTypeId_SHL = AsmTypeId_SHLGroup | 0x02,
	AsmTypeId_SHR = AsmTypeId_SHLGroup | 0x03,
	AsmTypeId_SUB = AsmTypeId_ADDGroup | 0x05,
	AsmTypeId_SUBSD = AsmTypeId_ADDSDGroup | 0x08,
	AsmTypeId_TEST = 0x10,
	AsmTypeId_XOR = AsmTypeId_ADDGroup | 0x06,
	AsmTypeId_XORPD = AsmTypeId_COMISDGroup | 0x02,
} EAsmTypeId;

typedef struct SAsm
{
	EAsmTypeId TypeId;
	S64* Addr;
} SAsm;

typedef struct SAsmLabel
{
	SAsm Asm;
#if defined(_DEBUG)
	int Cnt;
#endif
} SAsmLabel;

typedef struct SAsmMachine
{
	SAsm Asm;
	int DataSize;
	U8* Data;
} SAsmMachine;

typedef struct SAsmADDGroup
{
	SAsm Asm;
	SVal* A;
	SVal* B;
	U8 Rate;
} SAsmADDGroup;

typedef struct SAsmADDSDGroup
{
	SAsm Asm;
	SVal* A;
	SVal* B;
	Bool Double;
	U8 Code;
} SAsmADDSDGroup;

typedef struct SAsmINCGroup
{
	SAsm Asm;
	SVal* A;
	U8 Code;
	U8 Rate;
} SAsmINCGroup;

typedef struct SAsmJEGroup
{
	SAsm Asm;
	SVal* A;
	U8 Code;
} SAsmJEGroup;

typedef struct SAsmMOVSDGroup
{
	SAsm Asm;
	SVal* A;
	SVal* B;
	U8 Code;
	U8 CodeRM;
	U8 CodeMR;
} SAsmMOVSDGroup;

typedef struct SAsmREPGroup
{
	SAsm Asm;
	SVal* A;
	U8 Code;
} SAsmREPGroup;

typedef struct SAsmSHLGroup
{
	SAsm Asm;
	SVal* A;
	SVal* B;
	U8 Rate;
} SAsmSHLGroup;

typedef struct SAsmCOMISDGroup
{
	SAsm Asm;
	SVal* A;
	SVal* B;
	Bool Double;
	U8 Code;
} SAsmCOMISDGroup;

typedef struct SAsm1
{
	SAsm Asm;
	SVal* A;
} SAsm1;

typedef struct SAsm2
{
	SAsm Asm;
	SVal* A;
	SVal* B;
} SAsm2;

typedef struct SAsm3
{
	SAsm Asm;
	SVal* A;
	SVal* B;
	SVal* C;
} SAsm3;

SRefValue* RefValue(U64 value);
SRefValue* RefValueAddr(S64* addr, Bool relative);
SVal* ValImm(int size, SRefValue* value);
SVal* ValImmS(int size, S64 value);
SVal* ValImmU(int size, U64 value);
SVal* ValReg(int size, EReg reg);
SVal* ValMem(int size, SVal* base, SVal* idx, SRefValue* disp);
SVal* ValMemS(int size, SVal* base, SVal* idx, S64 disp);
SVal* ValRIP(int size, SRefValue* disp);
SAsmLabel* AsmLabel(void);
SAsmMachine* AsmMachine(int data_size, U8* data);
SAsmADDGroup* AsmADD(SVal* a, SVal* b);
SAsmADDSDGroup* AsmADDSD(SVal* a, SVal* b);
SAsmADDGroup* AsmAND(SVal* a, SVal* b);
SAsm1* AsmCALL(SVal* a);
SAsm1* AsmCDQ(SVal* a);
SAsmADDGroup* AsmCMP(SVal* a, SVal* b);
SAsmCOMISDGroup* AsmCOMISD(SVal* a, SVal* b);
SAsmADDSDGroup* AsmCVTSD2SI(SVal* a, SVal* b);
SAsmADDSDGroup* AsmCVTSD2SS(SVal* a, SVal* b);
SAsmADDSDGroup* AsmCVTSI2SD(SVal* a, SVal* b);
SAsmADDSDGroup* AsmCVTSS2SD(SVal* a, SVal* b);
SAsmINCGroup* AsmDEC(SVal* a);
SAsmINCGroup* AsmDIV(SVal* a);
SAsmADDSDGroup* AsmDIVSD(SVal* a, SVal* b);
SAsmINCGroup* AsmIDIV(SVal* a);
SAsm3* AsmIMUL(SVal* a, SVal* b, SVal* c);
SAsmINCGroup* AsmINC(SVal* a);
SAsm1* AsmINT(SVal* a);
SAsmJEGroup* AsmJA(SVal* a);
SAsmJEGroup* AsmJAE(SVal* a);
SAsmJEGroup* AsmJB(SVal* a);
SAsmJEGroup* AsmJBE(SVal* a);
SAsmJEGroup* AsmJE(SVal* a);
SAsmJEGroup* AsmJG(SVal* a);
SAsmJEGroup* AsmJGE(SVal* a);
SAsmJEGroup* AsmJL(SVal* a);
SAsmJEGroup* AsmJLE(SVal* a);
SAsm1* AsmJMP(SVal* a);
SAsmJEGroup* AsmJNE(SVal* a);
SAsmJEGroup* AsmJNO(SVal* a);
SAsmJEGroup* AsmJNS(SVal* a);
SAsmJEGroup* AsmJO(SVal* a);
SAsmJEGroup* AsmJS(SVal* a);
SAsm1* AsmLDMXCSR(SVal* a);
SAsm2* AsmLEA(SVal* a, SVal* b);
SAsm2* AsmMOV(SVal* a, SVal* b);
SAsmMOVSDGroup* AsmMOVSD(SVal* a, SVal* b);
SAsmMOVSDGroup* AsmMOVSS(SVal* a, SVal* b);
SAsm2* AsmMOVSX(SVal* a, SVal* b);
SAsm2* AsmMOVUPS(SVal* a, SVal* b);
SAsm2* AsmMOVZX(SVal* a, SVal* b);
SAsmADDSDGroup* AsmMULSD(SVal* a, SVal* b);
SAsmINCGroup* AsmNEG(SVal* a);
SAsm* AsmNOP(void);
SAsmINCGroup* AsmNOT(SVal* a);
SAsmADDGroup* AsmOR(SVal* a, SVal* b);
SAsmREPGroup* AsmREPMOVS(SVal* a);
SAsmREPGroup* AsmREPSTOS(SVal* a);
SAsm* AsmRET(void);
SAsmSHLGroup* AsmSAR(SVal* a, SVal* b);
SAsmSHLGroup* AsmSHL(SVal* a, SVal* b);
SAsmSHLGroup* AsmSHR(SVal* a, SVal* b);
SAsmADDGroup* AsmSUB(SVal* a, SVal* b);
SAsmADDSDGroup* AsmSUBSD(SVal* a, SVal* b);
SAsm2* AsmTEST(SVal* a, SVal* b);
SAsmADDGroup* AsmXOR(SVal* a, SVal* b);
SAsmCOMISDGroup* AsmXORPD(SVal* a, SVal* b);
void WriteAsmBin(FILE* file_ptr, SList* ref_value_list, S64 addr, const SAsm* asm_);
void Dump2(const Char* path, const SList* asms);
