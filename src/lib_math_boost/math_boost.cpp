#include "math_boost.h"

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#include <codecvt>

using namespace boost::multiprecision;

struct SBigInt
{
	SClass Class;
	cpp_int* Value;
};

struct SBigFloat
{
	SClass Class;
	cpp_dec_float_100* Value;
};

static std::wstring_convert<std::codecvt_utf8<Char>, Char> Converter;

EXPORT_CPP SClass* _makeBigInt(SClass* me_)
{
	SBigInt* me2 = reinterpret_cast<SBigInt*>(me_);
	me2->Value = static_cast<cpp_int*>(AllocMem(sizeof(cpp_int)));
	new(me2->Value)cpp_int();
	return me_;
}

EXPORT_CPP void _bigIntDtor(SClass* me_)
{
	SBigInt* me2 = reinterpret_cast<SBigInt*>(me_);
	if (me2->Value != NULL)
		FreeMem(me2->Value);
}

EXPORT_CPP S64 _bigIntCmp(SClass* me_, SClass* t)
{
	SBigInt* me2 = reinterpret_cast<SBigInt*>(me_);
	SBigInt* t2 = reinterpret_cast<SBigInt*>(t);
	return *me2->Value > *t2->Value ? 1 : (*me2->Value < *t2->Value ? -1 : 0);
}

EXPORT_CPP Bool _bigIntFromStr(SClass* me_, const void* value)
{
	SBigInt* me2 = reinterpret_cast<SBigInt*>(me_);
	THROWDBG(value == NULL, 0xc0000005);
	try
	{
		me2->Value->assign(Converter.to_bytes(static_cast<const Char*>(value) + 0x08));
	}
	catch (...)
	{
		return False;
	}
	return True;
}

EXPORT_CPP void* _bigIntToStr(SClass* me_)
{
	SBigInt* me2 = reinterpret_cast<SBigInt*>(me_);
	std::wstring result = Converter.from_bytes(me2->Value->str());
	size_t len = result.size();
	U8* result2 = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
	(reinterpret_cast<S64*>(result2))[0] = DefaultRefCntFunc;
	(reinterpret_cast<S64*>(result2))[1] = static_cast<S64>(len);
	memcpy(result2 + 0x10, result.c_str(), sizeof(Char) * (len + 1));
	return result2;
}

EXPORT_CPP void _bigIntFromInt(SClass* me_, S64 value)
{
	SBigInt* me2 = reinterpret_cast<SBigInt*>(me_);
	*me2->Value = value;
}

EXPORT_CPP S64 _bigIntToInt(SClass* me_)
{
	SBigInt* me2 = reinterpret_cast<SBigInt*>(me_);
	return static_cast<S64>(*me2->Value);
}

EXPORT_CPP SClass* _bigIntAdd(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value += *reinterpret_cast<SBigInt*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _bigIntAddInt(SClass* me_, S64 value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value += value;
	return me_;
}

EXPORT_CPP SClass* _bigIntSub(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value -= *reinterpret_cast<SBigInt*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _bigIntSubInt(SClass* me_, S64 value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value -= value;
	return me_;
}

EXPORT_CPP SClass* _bigIntMul(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value *= *reinterpret_cast<SBigInt*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _bigIntMulInt(SClass* me_, S64 value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value *= value;
	return me_;
}

EXPORT_CPP SClass* _bigIntDiv(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value /= *reinterpret_cast<SBigInt*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _bigIntDivInt(SClass* me_, S64 value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value /= value;
	return me_;
}

EXPORT_CPP SClass* _makeBigFloat(SClass* me_)
{
	SBigFloat* me2 = reinterpret_cast<SBigFloat*>(me_);
	me2->Value = static_cast<cpp_dec_float_100*>(AllocMem(sizeof(cpp_dec_float_100)));
	new(me2->Value)cpp_dec_float_100();
	return me_;
}

EXPORT_CPP void _bigFloatDtor(SClass* me_)
{
	SBigFloat* me2 = reinterpret_cast<SBigFloat*>(me_);
	if (me2->Value != NULL)
		FreeMem(me2->Value);
}

EXPORT_CPP S64 _bigFloatCmp(SClass* me_, SClass* t)
{
	SBigFloat* me2 = reinterpret_cast<SBigFloat*>(me_);
	SBigFloat* t2 = reinterpret_cast<SBigFloat*>(t);
	return *me2->Value > *t2->Value ? 1 : (*me2->Value < *t2->Value ? -1 : 0);
}

EXPORT_CPP Bool _bigFloatFromStr(SClass* me_, const void* value)
{
	SBigFloat* me2 = reinterpret_cast<SBigFloat*>(me_);
	THROWDBG(value == NULL, 0xc0000005);
	try
	{
		me2->Value->assign(Converter.to_bytes(static_cast<const Char*>(value) + 0x08));
	}
	catch (...)
	{
		return False;
	}
	return True;
}

EXPORT_CPP void* _bigFloatToStr(SClass* me_)
{
	SBigFloat* me2 = reinterpret_cast<SBigFloat*>(me_);
	std::wstring result = Converter.from_bytes(me2->Value->str());
	size_t len = result.size();
	U8* result2 = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
	(reinterpret_cast<S64*>(result2))[0] = DefaultRefCntFunc;
	(reinterpret_cast<S64*>(result2))[1] = static_cast<S64>(len);
	memcpy(result2 + 0x10, result.c_str(), sizeof(Char) * (len + 1));
	return result2;
}

EXPORT_CPP void _bigFloatFromFloat(SClass* me_, double value)
{
	SBigFloat* me2 = reinterpret_cast<SBigFloat*>(me_);
	*me2->Value = value;
}

EXPORT_CPP double _bigFloatToFloat(SClass* me_)
{
	SBigFloat* me2 = reinterpret_cast<SBigFloat*>(me_);
	return static_cast<double>(*me2->Value);
}

EXPORT_CPP SClass* _bigFloatAdd(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value += *reinterpret_cast<SBigFloat*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _bigFloatAddFloat(SClass* me_, double value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value += value;
	return me_;
}

EXPORT_CPP SClass* _bigFloatSub(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value -= *reinterpret_cast<SBigFloat*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _bigFloatSubFloat(SClass* me_, double value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value -= value;
	return me_;
}

EXPORT_CPP SClass* _bigFloatMul(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value *= *reinterpret_cast<SBigFloat*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _bigFloatMulFloat(SClass* me_, double value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value *= value;
	return me_;
}

EXPORT_CPP SClass* _bigFloatDiv(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value /= *reinterpret_cast<SBigFloat*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _bigFloatDivFloat(SClass* me_, double value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value /= value;
	return me_;
}
