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
	THROWDBG(pattern == NULL, EXCPT_ACCESS_VIOLATION);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	me2->Pattern = static_cast<wsregex*>(AllocMem(sizeof(wsregex)));
	new(me2->Pattern)wsregex();
	try
	{
		*me2->Pattern = wsregex::compile(reinterpret_cast<const Char*>(pattern + 0x10));
	}
	catch (...)
	{
		me2->Pattern->~wsregex();
		FreeMem(me2->Pattern);
		me2->Pattern = NULL;
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

EXPORT_CPP void* _regexFind(SClass* me_, S64* pos, const U8* text, S64 start)
{
	THROWDBG(pos == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	wsmatch results;
	if (start < -1 || (S64)text2.size() <= start)
	{
		*pos = -1;
		return NULL;
	}
	if (start == -1)
		start = 0;
	try
	{
		const std::wstring& text3 = text2.substr(start);
		if (regex_search(text3, results, *me2->Pattern))
		{
			*pos = start + static_cast<S64>(results.position(0));
			return ResultsToArray(results);
		}
		else
		{
			*pos = -1;
			return NULL;
		}
	}
	catch (...)
	{
		*pos = -1;
		return NULL;
	}
}

EXPORT_CPP void* _regexFindLast(SClass* me_, S64* pos, const U8* text, S64 start)
{
	THROWDBG(pos == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	{
		S64 size = (S64)text2.size();
		if (start < -1 || size <= start)
		{
			*pos = -1;
			return NULL;
		}
		if (start == -1)
			start = size - 1;
	}
	try
	{
		const std::wstring& text3 = text2.substr(0, start + 1);
		wsregex_iterator iter1(text3.begin(), text3.end(), *me2->Pattern);
		wsregex_iterator iter2;
		if (iter1 == iter2)
		{
			*pos = -1;
			return NULL;
		}
		wsregex_iterator iter = iter1;
		while (iter1 != iter2)
		{
			iter = iter1;
			iter1++;
		}
		*pos = static_cast<S64>(iter->position(0));
		return ResultsToArray(*iter);
	}
	catch (...)
	{
		*pos = -1;
		return NULL;
	}
}

EXPORT_CPP void* _regexMatch(SClass* me_, const U8* text)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	wsmatch results;
	try
	{
		if (regex_match(text2, results, *me2->Pattern))
			return ResultsToArray(results);
		else
			return NULL;
	}
	catch (...)
	{
		return NULL;
	}
}

EXPORT_CPP void* _regexFindAll(SClass* me_, U8** pos, const U8* text)
{
	THROWDBG(pos == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	try
	{
		wsregex_iterator iter1(text2.begin(), text2.end(), *me2->Pattern);
		wsregex_iterator iter2;
		if (iter1 == iter2)
		{
			*pos = NULL;
			return NULL;
		}
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
		*pos = NULL;
		return NULL;
	}
}

EXPORT_CPP void* _regexReplace(SClass* me_, const U8* text, const U8* newText, Bool all)
{
	THROWDBG(text == NULL, EXCPT_ACCESS_VIOLATION);
	THROWDBG(newText == NULL, EXCPT_ACCESS_VIOLATION);
	SRegexPattern* me2 = reinterpret_cast<SRegexPattern*>(me_);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	std::wstring new_text2 = reinterpret_cast<const Char*>(newText + 0x10);
	std::wstring result;
	try
	{
		result = regex_replace(text2, *me2->Pattern, new_text2, regex_constants::format_perl | (all ? regex_constants::format_default : regex_constants::format_first_only));
	}
	catch (...)
	{
		result = text2;
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
