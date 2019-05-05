#include "math_boost.h"

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/format.hpp>
#include <complex>
#include <codecvt>

using namespace boost::multiprecision;
using namespace boost::math::constants;

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

struct SComplex
{
	SClass Class;
	std::complex<double>* Value;
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
	THROWDBG(value == NULL, EXCPT_ACCESS_VIOLATION);
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

EXPORT_CPP SClass* _bigIntMod(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value %= *reinterpret_cast<SBigInt*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _bigIntModInt(SClass* me_, S64 value)
{
	*reinterpret_cast<SBigInt*>(me_)->Value %= value;
	return me_;
}

EXPORT_CPP SClass* _bigIntPowInt(SClass* me_, S64 value)
{
	U32 value2 = static_cast<U32>(value);
	THROWDBG(value != static_cast<S64>(value2), EXCPT_DBG_ARG_OUT_DOMAIN);
	*reinterpret_cast<SBigInt*>(me_)->Value = pow(*reinterpret_cast<SBigInt*>(me_)->Value, value2);
	return me_;
}

EXPORT_CPP SClass* _bigIntAbs(SClass* me_)
{
	*reinterpret_cast<SBigInt*>(me_)->Value = abs(*reinterpret_cast<SBigInt*>(me_)->Value);
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
	{
		me2->Value->~cpp_dec_float_100();
		FreeMem(me2->Value);
	}
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
	THROWDBG(value == NULL, EXCPT_ACCESS_VIOLATION);
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

EXPORT_CPP SClass* _bigFloatMod(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = fmod(*reinterpret_cast<SBigFloat*>(me_)->Value, *reinterpret_cast<SBigFloat*>(value)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatModFloat(SClass* me_, double value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = fmod(*reinterpret_cast<SBigFloat*>(me_)->Value, value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatPow(SClass* me_, SClass* value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = pow(*reinterpret_cast<SBigFloat*>(me_)->Value, *reinterpret_cast<SBigFloat*>(value)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatPowFloat(SClass* me_, double value)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = pow(*reinterpret_cast<SBigFloat*>(me_)->Value, value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatExp(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = exp(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatLn(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = log(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatSqrt(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = sqrt(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatFloor(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = floor(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatCeil(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = ceil(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatCos(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = cos(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatSin(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = sin(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatTan(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = tan(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatAcos(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = acos(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatAsin(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = asin(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatAtan(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = atan(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatCosh(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = cosh(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatSinh(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = sinh(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatTanh(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = tanh(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatAcosh(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = acosh(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatAsinh(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = asinh(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatAtanh(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = atanh(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatAbs(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = abs(*reinterpret_cast<SBigFloat*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _bigFloatPi(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = pi<cpp_dec_float_100>();
	return me_;
}

EXPORT_CPP SClass* _bigFloatE(SClass* me_)
{
	*reinterpret_cast<SBigFloat*>(me_)->Value = e<cpp_dec_float_100>();
	return me_;
}

EXPORT_CPP SClass* _makeComplex(SClass* me_)
{
	SComplex* me2 = reinterpret_cast<SComplex*>(me_);
	me2->Value = static_cast<std::complex<double>*>(AllocMem(sizeof(std::complex<double>)));
	new(me2->Value)std::complex<double>();
	return me_;
}

EXPORT_CPP void _complexDtor(SClass* me_)
{
	SComplex* me2 = reinterpret_cast<SComplex*>(me_);
	if (me2->Value != NULL)
	{
		me2->Value->~complex<double>();
		FreeMem(me2->Value);
	}
}

EXPORT_CPP void* _complexToStr(SClass* me_)
{
	SComplex* me2 = reinterpret_cast<SComplex*>(me_);
	double re = me2->Value->real();
	double im = me2->Value->imag();
	std::wstring result =
		im == 0.0f ? (boost::wformat(L"%g") % re).str() :
		(re == 0.0f ? (boost::wformat(L"%gi") % im).str() : (boost::wformat(L"%g%+gi") % re % im).str());
	size_t len = result.size();
	U8* result2 = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
	(reinterpret_cast<S64*>(result2))[0] = DefaultRefCntFunc;
	(reinterpret_cast<S64*>(result2))[1] = static_cast<S64>(len);
	memcpy(result2 + 0x10, result.c_str(), sizeof(Char) * (len + 1));
	return result2;
}

EXPORT_CPP double _complexRe(SClass* me_)
{
	return reinterpret_cast<SComplex*>(me_)->Value->real();
}

EXPORT_CPP double _complexIm(SClass* me_)
{
	return reinterpret_cast<SComplex*>(me_)->Value->imag();
}

EXPORT_CPP SClass* _complexSet(SClass* me_, double re, double im)
{
	SComplex* me2 = reinterpret_cast<SComplex*>(me_);
	me2->Value->real(re);
	me2->Value->imag(im);
	return me_;
}

EXPORT_CPP SClass* _complexAdd(SClass* me_, SClass* value)
{
	*reinterpret_cast<SComplex*>(me_)->Value += *reinterpret_cast<SComplex*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _complexSub(SClass* me_, SClass* value)
{
	*reinterpret_cast<SComplex*>(me_)->Value -= *reinterpret_cast<SComplex*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _complexMul(SClass* me_, SClass* value)
{
	*reinterpret_cast<SComplex*>(me_)->Value *= *reinterpret_cast<SComplex*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _complexDiv(SClass* me_, SClass* value)
{
	*reinterpret_cast<SComplex*>(me_)->Value /= *reinterpret_cast<SComplex*>(value)->Value;
	return me_;
}

EXPORT_CPP SClass* _complexPow(SClass* me_, SClass* value)
{
	*reinterpret_cast<SComplex*>(me_)->Value = pow(*reinterpret_cast<SComplex*>(me_)->Value, *reinterpret_cast<SComplex*>(value)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexExp(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = exp(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexLn(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = log(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexSqrt(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = sqrt(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexCos(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = cos(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexSin(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = sin(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexTan(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = tan(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexAcos(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = acos(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexAsin(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = asin(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexAtan(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = atan(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexCosh(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = cosh(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexSinh(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = sinh(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexTanh(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = tanh(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexAcosh(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = acosh(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexAsinh(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = asinh(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexAtanh(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = atanh(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}

EXPORT_CPP SClass* _complexAbs(SClass* me_)
{
	*reinterpret_cast<SComplex*>(me_)->Value = abs(*reinterpret_cast<SComplex*>(me_)->Value);
	return me_;
}
