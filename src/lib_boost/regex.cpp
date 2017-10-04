#include "regex.h"

#include <boost/xpressive/xpressive.hpp>

using namespace boost::xpressive;

static void* ResultsToArray(const wsmatch& results);

EXPORT_CPP void* _find(S64* pos, const U8* text, const U8* pattern)
{
	THROWDBG(pos == NULL, 0xc0000005);
	THROWDBG(text == NULL, 0xc0000005);
	THROWDBG(pattern == NULL, 0xc0000005);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	wsregex pattern2;
	wsmatch results;
	Bool found;
	try
	{
		pattern2 = wsregex::compile(reinterpret_cast<const Char*>(pattern + 0x10));
		found = regex_search(text2, results, pattern2);
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

EXPORT_CPP void* _match(S64* pos, const U8* text, const U8* pattern)
{
	THROWDBG(pos == NULL, 0xc0000005);
	THROWDBG(text == NULL, 0xc0000005);
	THROWDBG(pattern == NULL, 0xc0000005);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	wsregex pattern2;
	wsmatch results;
	Bool found;
	try
	{
		pattern2 = wsregex::compile(reinterpret_cast<const Char*>(pattern + 0x10));
		found = regex_match(text2, results, pattern2);
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

EXPORT_CPP void* _all(U8** pos, const U8* text, const U8* pattern)
{
	THROWDBG(pos == NULL, 0xc0000005);
	THROWDBG(text == NULL, 0xc0000005);
	THROWDBG(pattern == NULL, 0xc0000005);
	std::wstring text2 = reinterpret_cast<const Char*>(text + 0x10);
	wsregex pattern2;
	wsmatch results;
	Bool found;
	try
	{
		pattern2 = wsregex::compile(reinterpret_cast<const Char*>(pattern + 0x10));
		wsregex_iterator iter1(text2.begin(), text2.end(), pattern2);
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
