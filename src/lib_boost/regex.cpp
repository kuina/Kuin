#include "regex.h"

#include <boost/xpressive/xpressive.hpp>

using namespace boost::xpressive;

typedef struct SRegexPattern
{
	SClass Class;
	wsregex* Pattern;
} SRegexPattern;

static void* ResultsToArray(const wsmatch& results);

EXPORT_CPP SClass* _makeRegex(SClass* me_, const U8* pattern)
{
	THROWDBG(pattern == NULL, 0xc0000005);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	me2->Pattern = static_cast<wsregex*>(AllocMem(sizeof(wsregex)));
	new(me2->Pattern)wsregex();
	try
	{
		*me2->Pattern = wsregex::compile(reinterpret_cast<const Char*>(pattern + 0x10));
	}
	catch (...)
	{
		me2->Pattern = NULL;
		THROWDBG(True, 0xe9170006);
		return NULL;
	}
	return me_;
}

EXPORT_CPP void _regexDtor(SClass* me_)
{
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	if (me2->Pattern != NULL)
	{
		me2->Pattern->~wsregex();
		FreeMem(me2->Pattern);
	}
}

EXPORT_CPP void* _regexFind(SClass* me_, S64* pos, const U8* text)
{
	THROWDBG(pos == NULL, 0xc0000005);
	THROWDBG(text == NULL, 0xc0000005);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	wsmatch results;
	Bool found;
	try
	{
		found = regex_search(text2, results, *me2->Pattern);
	}
	catch (...)
	{
		THROWDBG(True, 0xe9170006);
		*pos = -1;
		return NULL;
	}
	if (found)
	{
		*pos = static_cast<S64>(results.position(0));
		return ResultsToArray(results);
	}
	else
	{
		*pos = -1;
		return NULL;
	}
}

EXPORT_CPP void* _regexMatch(SClass* me_, S64* pos, const U8* text)
{
	THROWDBG(pos == NULL, 0xc0000005);
	THROWDBG(text == NULL, 0xc0000005);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	wsmatch results;
	Bool found;
	try
	{
		found = regex_match(text2, results, *me2->Pattern);
	}
	catch (...)
	{
		THROWDBG(True, 0xe9170006);
		*pos = -1;
		return NULL;
	}
	if (found)
	{
		*pos = static_cast<S64>(results.position(0));
		return ResultsToArray(results);
	}
	else
	{
		*pos = -1;
		return NULL;
	}
}

EXPORT_CPP void* _regexFindAll(SClass* me_, U8** pos, const U8* text)
{
	THROWDBG(pos == NULL, 0xc0000005);
	THROWDBG(text == NULL, 0xc0000005);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	wsmatch results;
	Bool found;
	try
	{
		wsregex_iterator iter1(text2.begin(), text2.end(), *me2->Pattern);
		wsregex_iterator iter2;
		size_t len = static_cast<size_t>(std::distance(iter1, iter2));
		*pos = static_cast<U8*>(AllocMem(0x10 + sizeof(S64) * len));
		(reinterpret_cast<S64*>(*pos))[0] = 1;
		(reinterpret_cast<S64*>(*pos))[1] = static_cast<S64>(len);
		U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(void*) * len));
		(reinterpret_cast<S64*>(result))[0] = DefaultRefCntFunc;
		(reinterpret_cast<S64*>(result))[1] = static_cast<S64>(len);
		int cnt = 0;
		S64* pos2 = reinterpret_cast<S64*>((*pos) + 0x10);
		void** result2 = reinterpret_cast<void**>(result + 0x10);
		while (iter1 != iter2)
		{
			pos2[cnt] = static_cast<S64>(iter1->position(0));
			void* array_ = ResultsToArray(*iter1);
			(*(static_cast<S64*>(array_)))++;
			result2[cnt] = array_;
			++iter1;
			cnt++;
		}
		return result;
	}
	catch (...)
	{
		THROWDBG(True, 0xe9170006);
		*pos = NULL;
		return NULL;
	}
}

EXPORT_CPP void* _regexReplace(SClass* me_, const U8* text, const U8* newText, Bool all)
{
	THROWDBG(text == NULL, 0xc0000005);
	THROWDBG(newText == NULL, 0xc0000005);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	std::wstring new_text2 = reinterpret_cast<const Char*>(newText + 0x10);
	std::wstring result;
	Bool found;
	try
	{
		result = regex_replace(text2, *me2->Pattern, new_text2, all ? regex_constants::format_all : regex_constants::format_first_only);
	}
	catch (...)
	{
		THROWDBG(True, 0xe9170006);
		return NULL;
	}
	size_t len = result.size();
	U8* result2 = static_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len + 1)));
	(reinterpret_cast<S64*>(result2))[0] = DefaultRefCntFunc;
	(reinterpret_cast<S64*>(result2))[1] = static_cast<S64>(len);
	memcpy(result2 + 0x10, result.c_str(), sizeof(Char) * (len + 1));
	return result2;
}

static void* ResultsToArray(const wsmatch& results)
{
	size_t len = results.size();
	U8* result = static_cast<U8*>(AllocMem(0x10 + sizeof(void*) * len));
	(reinterpret_cast<S64*>(result))[0] = DefaultRefCntFunc;
	(reinterpret_cast<S64*>(result))[1] = static_cast<S64>(len);
	int i;
	U8** ptr = reinterpret_cast<U8**>(result + 0x10);
	for (i = 0; i < static_cast<int>(len); i++)
	{
		const std::wstring& result_str = results[i];
		size_t len2 = result_str.size();
		ptr[i] = reinterpret_cast<U8*>(AllocMem(0x10 + sizeof(Char) * (len2 + 1)));
		(reinterpret_cast<S64*>(ptr[i]))[0] = 1;
		(reinterpret_cast<S64*>(ptr[i]))[1] = static_cast<S64>(len2);
		memcpy(ptr[i] + 0x10, result_str.c_str(), sizeof(Char) * (len2 + 1));
	}
	return result;
}
