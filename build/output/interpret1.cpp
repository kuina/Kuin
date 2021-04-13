#include "kuin_interpreter.h"

#define AUXILIARY_BUF_SIZE (0x8000)

enum EAlignmentToken
{
	AlignmentToken_None = 0x01,
	AlignmentToken_Value = 0x02,
	AlignmentToken_Ope2 = 0x04,
	AlignmentToken_Pr = 0x08,
	AlignmentToken_Comma = 0x10,
};

enum ECharColor
{
	CharColor_None,
	CharColor_Identifier,
	CharColor_Global,
	CharColor_Reserved,
	CharColor_Number,
	CharColor_Str,
	CharColor_Char,
	CharColor_LineComment,
	CharColor_Comment,
	CharColor_Symbol,
	CharColor_Err,
};

static wchar_t Interpret1Buf[0x10 + AUXILIARY_BUF_SIZE + 1];
bool isReserved(Array_<char16_t>*);

static void InterpretImpl1Color(int* ptr, int str_level, const wchar_t* str, uint8_t* color, int64_t comment_level, uint64_t flags);
static void InterpretImpl1Align(int* ptr_buf, int* ptr_str, wchar_t* buf, const wchar_t* str, int64_t* comment_level, uint64_t* flags, int64_t* tab_context, int64_t cursor_x, int64_t* new_cursor_x, wchar_t* add_end);
static void InterpretImpl1AlignRecursion(int* ptr_buf, int* ptr_str, int str_level, int type_level, wchar_t* buf, const wchar_t* str, int64_t* comment_level, uint64_t* flags, int64_t* tab_context, EAlignmentToken* prev, int64_t cursor_x, int64_t* new_cursor_x);
static void InterpretImpl1Write(int* ptr, wchar_t* buf, wchar_t c);
static void Interpret1Impl1UpdateCursor(int64_t cursor_x, int64_t* new_cursor_x, int* ptr_str, int* ptr_buf);

bool InterpretImpl1(void* str, void* color, void* comment_level, void* flags, int64_t line, void* me, void* replace_func, int64_t cursor_x, int64_t cursor_y, int64_t* new_cursor_x, int64_t old_line, int64_t new_line)
{
	if (line == -1)
	{
		int64_t line_len = *(int64_t*)((uint8_t*)str + 0x08);
		int64_t comment_level_context = 0;
		uint64_t flags_context = 0;
		int64_t tab_context = 0;
		int64_t i;
		wchar_t add_end[32];
		add_end[0] = L'\0';
		for (i = 0; i < line_len; i++)
		{
			void** str2 = (void**)((uint8_t*)str + 0x10 + 0x08 * (size_t)i);
			void** color2 = (void**)((uint8_t*)color + 0x10 + 0x08 * (size_t)i);
			int64_t* comment_level2 = (int64_t*)((uint8_t*)comment_level + 0x10 + 0x08 * (size_t)i);
			uint64_t* flags2 = (uint64_t*)((uint8_t*)flags + 0x10 + 0x08 * (size_t)i);

			const wchar_t* str3 = (wchar_t*)((uint8_t*)*str2 + 0x10);
			wchar_t* dst_str = Interpret1Buf + 0x08;

			int64_t len;
			{
				int ptr_buf = 0;
				int ptr_str = 0;
				InterpretImpl1Align(&ptr_buf, &ptr_str, dst_str, (wchar_t*)((uint8_t*)*str2 + 0x10), &comment_level_context, &flags_context, &tab_context, cursor_x, i == cursor_y ? new_cursor_x : nullptr, i == old_line ? add_end : nullptr);
				len = (int64_t)ptr_buf;
			}
			if (add_end[0] != L'\0' && tab_context == 0)
				add_end[0] = L'\0';

			if (wcscmp(str3, dst_str) != 0)
			{
				((int64_t*)Interpret1Buf)[0] = 2;
				((int64_t*)Interpret1Buf)[1] = len;
				if (me != nullptr)
					(*(int64_t*)me)++;
				Call3Asm(me, (void*)i, (void*)Interpret1Buf, replace_func);

				str2 = (void**)((uint8_t*)str + 0x10 + 0x08 * (size_t)i);
				color2 = (void**)((uint8_t*)color + 0x10 + 0x08 * (size_t)i);
				comment_level2 = (int64_t*)((uint8_t*)comment_level + 0x10 + 0x08 * (size_t)i);
				flags2 = (uint64_t*)((uint8_t*)flags + 0x10 + 0x08 * (size_t)i);
			}
			*comment_level2 = comment_level_context;
			*flags2 = (*flags2 & 0x02) | flags_context;

			{
				int ptr = 0;
				InterpretImpl1Color(&ptr, 0, (wchar_t*)((uint8_t*)*str2 + 0x10), (uint8_t*)*color2 + 0x10, *comment_level2, *flags2);
			}
		}
		if (add_end[0] != L'\0')
		{
			if (me != nullptr)
				(*(int64_t*)me)++;
			Call3Asm(me, (void*)(-1 - new_line), (void*)add_end, replace_func);
			return false;
		}
	}
	else
	{
		void* str2 = *(void**)((uint8_t*)str + 0x10 + 0x08 * (size_t)line);
		const wchar_t* str3 = (wchar_t*)((uint8_t*)str2 + 0x10);
		uint8_t* color2 = (uint8_t*)*(void**)((uint8_t*)color + 0x10 + 0x08 * (size_t)line) + 0x10;
		int64_t comment_level2 = *(int64_t*)((uint8_t*)comment_level + 0x10 + 0x08 * (size_t)line);
		uint64_t flags2 = *(uint64_t*)((uint8_t*)flags + 0x10 + 0x08 * (size_t)line);
		int ptr = 0;
		InterpretImpl1Color(&ptr, 0, str3, color2, comment_level2, flags2);
	}
	return true;
}

static void InterpretImpl1Color(int* ptr, int str_level, const wchar_t* str, uint8_t* color, int64_t comment_level, uint64_t flags)
{
	while (str[*ptr] != L'\0')
	{
		const wchar_t c = str[*ptr];
		if (comment_level <= 0)
		{
			if (str_level > 0 && c == L'}')
			{
				break;
			}
			else if (c == L' ' || c == L'\t')
			{
				color[*ptr] = CharColor_None;
				(*ptr)++;
			}
			else if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || c == L'@' || c == L'\\' || c == L'%' && (L'a' <= str[*ptr + 1] && str[*ptr + 1] <= L'z' || L'A' <= str[*ptr + 1] && str[*ptr + 1] <= L'Z' || str[*ptr + 1] == L'_'))
			{
				bool at = false;
				int begin = *ptr;
				int end;
				do
				{
					if (str[*ptr] == L'@')
						at = true;
					(*ptr)++;
				} while (L'a' <= str[*ptr] && str[*ptr] <= L'z' || L'A' <= str[*ptr] && str[*ptr] <= L'Z' || str[*ptr] == L'_' || L'0' <= str[*ptr] && str[*ptr] <= L'9' || str[*ptr] == L'@' || str[*ptr] == L'\\');
				end = *ptr;
				{
					uint8_t new_color = (uint8_t)(at ? CharColor_Global : CharColor_Identifier);
					int word_len = end - begin;
					if (!at && word_len <= 16)
					{
						wchar_t word[17];
						wcsncpy_s(word, str + begin, word_len);
						word[word_len] = L'\0';
						auto* word2 = new_(Array_<char16_t>)();
						word2->L = word_len;
						word2->B = reinterpret_cast<char16_t*>(word);
						if (isReserved(word2))
							new_color = (uint8_t)CharColor_Reserved;
					}
					int i;
					for (i = begin; i < end; i++)
						color[i] = new_color;
				}
			}
			else if (L'0' <= c && c <= L'9')
			{
				uint8_t new_color = (uint8_t)CharColor_Number;
				do
				{
					color[*ptr] = new_color;
					(*ptr)++;
				} while (L'0' <= str[*ptr] && str[*ptr] <= L'9' || L'A' <= str[*ptr] && str[*ptr] <= L'F' || str[*ptr] == L'x' || str[*ptr] == L'.');
				if (str[*ptr] == L'e')
				{
					color[*ptr] = new_color;
					(*ptr)++;
					if (str[*ptr] == L'+' || str[*ptr] == L'-')
					{
						color[*ptr] = new_color;
						(*ptr)++;
						while (L'0' <= str[*ptr] && str[*ptr] <= L'9')
						{
							color[*ptr] = new_color;
							(*ptr)++;
						}
					}
				}
				else if (str[*ptr] == L'b')
				{
					color[*ptr] = new_color;
					(*ptr)++;
					while (L'0' <= str[*ptr] && str[*ptr] <= L'9')
					{
						color[*ptr] = new_color;
						(*ptr)++;
					}
				}
			}
			else if (c == L'"')
			{
				do
				{
					if (str[*ptr] == L'\\')
					{
						color[*ptr] = CharColor_Str;
						(*ptr)++;
						if (str[*ptr] == L'\0')
							break;
						if (str[*ptr] == L'{')
						{
							color[*ptr] = CharColor_Str;
							(*ptr)++;
							InterpretImpl1Color(ptr, str_level + 1, str, color, comment_level, flags);
							continue;
						}
					}
					color[*ptr] = CharColor_Str;
					(*ptr)++;
					if (str[*ptr] == L'"')
					{
						color[*ptr] = CharColor_Str;
						(*ptr)++;
						break;
					}
				} while (str[*ptr] != L'\0');
			}
			else if (c == L'\'')
			{
				do
				{
					if (str[*ptr] == L'\\')
					{
						color[*ptr] = CharColor_Char;
						(*ptr)++;
						if (str[*ptr] == L'\0')
							break;
					}
					color[*ptr] = CharColor_Char;
					(*ptr)++;
					if (str[*ptr] == L'\'')
					{
						color[*ptr] = CharColor_Char;
						(*ptr)++;
						break;
					}
				} while (str[*ptr] != L'\0');
			}
			else if (c == L'{')
			{
				color[*ptr] = CharColor_Comment;
				(*ptr)++;
				comment_level++;
			}
			else if (c == L';')
			{
				do
				{
					color[*ptr] = CharColor_LineComment;
					(*ptr)++;
				} while (str[*ptr] != L'\0');
			}
			else
			{
				color[*ptr] = CharColor_Symbol;
				(*ptr)++;
			}
		}
		else
		{
			if (c == L'"')
			{
				do
				{
					if (str[*ptr] == L'\\')
					{
						color[*ptr] = CharColor_Comment;
						(*ptr)++;
						if (str[*ptr] == L'\0')
							break;
					}
					color[*ptr] = CharColor_Comment;
					(*ptr)++;
					if (str[*ptr] == L'"')
					{
						color[*ptr] = CharColor_Comment;
						(*ptr)++;
						break;
					}
				} while (str[*ptr] != L'\0');
			}
			else if (c == L'\'')
			{
				do
				{
					if (str[*ptr] == L'\\')
					{
						color[*ptr] = CharColor_Comment;
						(*ptr)++;
						if (str[*ptr] == L'\0')
							break;
					}
					color[*ptr] = CharColor_Comment;
					(*ptr)++;
					if (str[*ptr] == L'\'')
					{
						color[*ptr] = CharColor_Comment;
						(*ptr)++;
						break;
					}
				} while (str[*ptr] != L'\0');
			}
			else if (c == L';')
			{
				do
				{
					color[*ptr] = CharColor_Comment;
					(*ptr)++;
				} while (str[*ptr] != L'\0');
			}
			else
			{
				if (c == L'{')
					comment_level++;
				else if (c == L'}')
					comment_level--;
				color[*ptr] = CharColor_Comment;
				(*ptr)++;
			}
		}
	}
}

static void InterpretImpl1Align(int* ptr_buf, int* ptr_str, wchar_t* buf, const wchar_t* str, int64_t* comment_level, uint64_t* flags, int64_t* tab_context, int64_t cursor_x, int64_t* new_cursor_x, wchar_t* add_end)
{
	EAlignmentToken prev = AlignmentToken_None;
	if (*comment_level <= 0)
	{
		int access_public = -1;
		int access_override = -1;
		bool access_override2 = false;
		int64_t enum_depth = -1;
		while (str[*ptr_str] != L'\0')
		{
			const wchar_t c = str[*ptr_str];
			if (c == L' ' || c == L'\t')
			{
				(*ptr_str)++;
				continue;
			}
			if (c == L'+')
			{
				access_public = *ptr_str;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				(*ptr_str)++;
				continue;
			}
			if (c == L'*')
			{
				if (access_override != -1)
					access_override2 = true;
				access_override = *ptr_str;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				(*ptr_str)++;
				continue;
			}
			if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || c == L'@' || c == L'\\')
			{
				prev = AlignmentToken_Value;
				int begin = *ptr_str;
				int end;
				do
				{
					(*ptr_str)++;
				} while (L'a' <= str[*ptr_str] && str[*ptr_str] <= L'z' || L'A' <= str[*ptr_str] && str[*ptr_str] <= L'Z' || str[*ptr_str] == L'_' || L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9' || str[*ptr_str] == L'@' || str[*ptr_str] == L'\\');
				end = *ptr_str;
				if (str[begin] == L'e' && str[begin + 1] == L'n' && str[begin + 2] == L'd' && *ptr_str == begin + 3)
				{
					if (enum_depth == *tab_context)
						enum_depth = -1;
					(*tab_context)--;
					if (*tab_context < 0)
						*tab_context = 0;
				}
				else if (str[begin] == L'e' && str[begin + 1] == L'l' && str[begin + 2] == L'i' && str[begin + 3] == L'f' && *ptr_str == begin + 4 ||
					str[begin] == L'e' && str[begin + 1] == L'l' && str[begin + 2] == L's' && str[begin + 3] == L'e' && *ptr_str == begin + 4 ||
					str[begin] == L'd' && str[begin + 1] == L'e' && str[begin + 2] == L'f' && str[begin + 3] == L'a' && str[begin + 4] == L'u' && str[begin + 5] == L'l' && str[begin + 6] == L't' && *ptr_str == begin + 7 ||
					str[begin] == L'f' && str[begin + 1] == L'i' && str[begin + 2] == L'n' && str[begin + 3] == L'a' && str[begin + 4] == L'l' && str[begin + 5] == L'l' && str[begin + 6] == L'y' && *ptr_str == begin + 7)
				{
					(*tab_context)--;
					if (*tab_context < 0)
						*tab_context = 0;
				}
				else if (str[begin] == L'c' && str[begin + 1] == L'a' && str[begin + 2] == L's' && str[begin + 3] == L'e' && *ptr_str == begin + 4 ||
					str[begin] == L'c' && str[begin + 1] == L'a' && str[begin + 2] == L't' && str[begin + 3] == L'c' && str[begin + 4] == L'h' && *ptr_str == begin + 5)
				{
					prev = AlignmentToken_Comma;
					(*tab_context)--;
					if (*tab_context < 0)
						*tab_context = 0;
				}
				if (enum_depth != -1)
					(*flags) |= 2;
				else
					(*flags) &= ~2;
				{
					int64_t i;
					for (i = 0; i < *tab_context; i++)
						InterpretImpl1Write(ptr_buf, buf, L'\t');
				}
				{
					if (access_public != -1)
					{
						if (new_cursor_x != nullptr && cursor_x == (int64_t)access_public)
							*new_cursor_x = (int64_t)*ptr_buf;
						InterpretImpl1Write(ptr_buf, buf, L'+');
					}
					if (access_override != -1)
					{
						if (new_cursor_x != nullptr && cursor_x == (int64_t)access_override)
							*new_cursor_x = (int64_t)*ptr_buf;
						InterpretImpl1Write(ptr_buf, buf, L'*');
						if (access_override2)
							InterpretImpl1Write(ptr_buf, buf, L'*');
					}
					int i;
					for (i = begin; i < end; i++)
					{
						if (new_cursor_x != nullptr && cursor_x == (int64_t)i)
							*new_cursor_x = (int64_t)*ptr_buf;
						InterpretImpl1Write(ptr_buf, buf, str[i]);
					}
				}
				bool is_enum = false;
				if (str[begin] == L'e' && str[begin + 1] == L'n' && str[begin + 2] == L'u' && str[begin + 3] == L'm' && *ptr_str == begin + 4)
				{
					enum_depth = *tab_context;
					is_enum = true;
				}
				if (is_enum ||
					str[begin] == L'f' && str[begin + 1] == L'u' && str[begin + 2] == L'n' && str[begin + 3] == L'c' && *ptr_str == begin + 4 ||
					str[begin] == L'c' && str[begin + 1] == L'l' && str[begin + 2] == L'a' && str[begin + 3] == L's' && str[begin + 4] == L's' && *ptr_str == begin + 5 ||
					str[begin] == L'i' && str[begin + 1] == L'f' && *ptr_str == begin + 2 ||
					str[begin] == L's' && str[begin + 1] == L'w' && str[begin + 2] == L'i' && str[begin + 3] == L't' && str[begin + 4] == L'c' && str[begin + 5] == L'h' && *ptr_str == begin + 6 ||
					str[begin] == L'w' && str[begin + 1] == L'h' && str[begin + 2] == L'i' && str[begin + 3] == L'l' && str[begin + 4] == L'e' && *ptr_str == begin + 5 ||
					str[begin] == L'f' && str[begin + 1] == L'o' && str[begin + 2] == L'r' && *ptr_str == begin + 3 ||
					str[begin] == L't' && str[begin + 1] == L'r' && str[begin + 2] == L'y' && *ptr_str == begin + 3 ||
					str[begin] == L'b' && str[begin + 1] == L'l' && str[begin + 2] == L'o' && str[begin + 3] == L'c' && str[begin + 4] == L'k' && *ptr_str == begin + 5)
				{
					(*tab_context)++;
					if (add_end != nullptr)
					{
						((int64_t*)add_end)[0] = 2;
						((int64_t*)add_end)[1] = (int64_t)(0x05 + end - begin);
						add_end[0x08] = L'\n';
						add_end[0x09] = L'e';
						add_end[0x0a] = L'n';
						add_end[0x0b] = L'd';
						add_end[0x0c] = L' ';
						int i;
						for (i = begin; i < end; i++)
							add_end[0x0d + i - begin] = str[i];
						add_end[0x0d + end - begin] = L'\0';
					}
				}
				else if (str[begin] == L'e' && str[begin + 1] == L'l' && str[begin + 2] == L'i' && str[begin + 3] == L'f' && *ptr_str == begin + 4 ||
					str[begin] == L'e' && str[begin + 1] == L'l' && str[begin + 2] == L's' && str[begin + 3] == L'e' && *ptr_str == begin + 4 ||
					str[begin] == L'c' && str[begin + 1] == L'a' && str[begin + 2] == L's' && str[begin + 3] == L'e' && *ptr_str == begin + 4 ||
					str[begin] == L'd' && str[begin + 1] == L'e' && str[begin + 2] == L'f' && str[begin + 3] == L'a' && str[begin + 4] == L'u' && str[begin + 5] == L'l' && str[begin + 6] == L't' && *ptr_str == begin + 7 ||
					str[begin] == L'c' && str[begin + 1] == L'a' && str[begin + 2] == L't' && str[begin + 3] == L'c' && str[begin + 4] == L'h' && *ptr_str == begin + 5 ||
					str[begin] == L'f' && str[begin + 1] == L'i' && str[begin + 2] == L'n' && str[begin + 3] == L'a' && str[begin + 4] == L'l' && str[begin + 5] == L'l' && str[begin + 6] == L'y' && *ptr_str == begin + 7)
				{
					(*tab_context)++;
				}
			}
			break;
		}
		if (prev == AlignmentToken_None)
		{
			int64_t i;
			for (i = 0; i < *tab_context; i++)
				InterpretImpl1Write(ptr_buf, buf, L'\t');
			if (new_cursor_x != nullptr)
				*new_cursor_x = (int64_t)*ptr_buf;
		}
	}
	else
	{
		if (new_cursor_x != nullptr)
			*new_cursor_x = (int64_t)*ptr_buf;
	}
	InterpretImpl1AlignRecursion(ptr_buf, ptr_str, 0, 0, buf, str, comment_level, flags, tab_context, &prev, cursor_x, new_cursor_x);
	buf[*ptr_buf] = L'\0';
	if (new_cursor_x != nullptr && cursor_x >= (int64_t)*ptr_str)
		*new_cursor_x = (int64_t)*ptr_buf;
}

static void InterpretImpl1AlignRecursion(int* ptr_buf, int* ptr_str, int str_level, int type_level, wchar_t* buf, const wchar_t* str, int64_t* comment_level, uint64_t* flags, int64_t* tab_context, EAlignmentToken* prev, int64_t cursor_x, int64_t* new_cursor_x)
{
	while (str[*ptr_str] != L'\0')
	{
		const wchar_t c = str[*ptr_str];
		if (*comment_level <= 0)
		{
			if (str_level > 0 && c == L'}')
			{
				*prev = AlignmentToken_None;
				break;
			}
			else if (type_level > 0 && c == L'>')
			{
				*prev = AlignmentToken_Pr;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
				(*ptr_str)++;
				break;
			}
			else if (c == L' ' || c == L'\t')
			{
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				(*ptr_str)++;
			}
			else if (L'a' <= c && c <= L'z' || L'A' <= c && c <= L'Z' || c == L'_' || c == L'@' || c == L'\\' || c == L'%' && (L'a' <= str[*ptr_str + 1] && str[*ptr_str + 1] <= L'z' || L'A' <= str[*ptr_str + 1] && str[*ptr_str + 1] <= L'Z' || str[*ptr_str + 1] == L'_'))
			{
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_Value;
				int begin = *ptr_str;
				do
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				} while (L'a' <= str[*ptr_str] && str[*ptr_str] <= L'z' || L'A' <= str[*ptr_str] && str[*ptr_str] <= L'Z' || str[*ptr_str] == L'_' || L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9' || str[*ptr_str] == L'@' || str[*ptr_str] == L'\\');
				if (str[begin] == L'f' && str[begin + 1] == L'u' && str[begin + 2] == L'n' && str[begin + 3] == L'c' && *ptr_str == begin + 4 ||
					str[begin] == L'l' && str[begin + 1] == L'i' && str[begin + 2] == L's' && str[begin + 3] == L't' && *ptr_str == begin + 4 ||
					str[begin] == L's' && str[begin + 1] == L't' && str[begin + 2] == L'a' && str[begin + 3] == L'c' && str[begin + 4] == L'k' && *ptr_str == begin + 5 ||
					str[begin] == L'q' && str[begin + 1] == L'u' && str[begin + 2] == L'e' && str[begin + 3] == L'u' && str[begin + 4] == L'e' && *ptr_str == begin + 5 ||
					str[begin] == L'd' && str[begin + 1] == L'i' && str[begin + 2] == L'c' && str[begin + 3] == L't' && *ptr_str == begin + 4)
				{
					while (str[*ptr_str] != L'\0')
					{
						if (str[*ptr_str] == L' ' || str[*ptr_str] == L'\t')
						{
							Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
							(*ptr_str)++;
							continue;
						}
						if (str[*ptr_str] == L'<')
						{
							*prev = AlignmentToken_None;
							Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
							InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
							(*ptr_str)++;
							InterpretImpl1AlignRecursion(ptr_buf, ptr_str, str_level, type_level + 1, buf, str, comment_level, flags, tab_context, prev, cursor_x, new_cursor_x);
						}
						break;
					}
				}
			}
			else if (L'0' <= c && c <= L'9')
			{
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_Value;
				do
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				} while (L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9' || L'A' <= str[*ptr_str] && str[*ptr_str] <= L'F' || str[*ptr_str] == L'x' || str[*ptr_str] == L'.');
				if (str[*ptr_str] == L'e')
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'+' || str[*ptr_str] == L'-')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						while (L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9')
						{
							Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
							InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
							(*ptr_str)++;
						}
					}
				}
				else if (str[*ptr_str] == L'b')
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					while (L'0' <= str[*ptr_str] && str[*ptr_str] <= L'9')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
					}
				}
			}
			else if (c == L'"')
			{
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_None;
				do
				{
					if (str[*ptr_str] == L'\\')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						if (str[*ptr_str] == L'\0')
							break;
						if (str[*ptr_str] == L'{')
						{
							Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
							InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
							(*ptr_str)++;
							InterpretImpl1AlignRecursion(ptr_buf, ptr_str, str_level + 1, type_level, buf, str, comment_level, flags, tab_context, prev, cursor_x, new_cursor_x);
							continue;
						}
					}
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'"')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						break;
					}
				} while (str[*ptr_str] != L'\0');
				*prev = AlignmentToken_Value;
			}
			else if (c == L'\'')
			{
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_None;
				do
				{
					if (str[*ptr_str] == L'\\')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						if (str[*ptr_str] == L'\0')
							break;
					}
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'\'')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						break;
					}
				} while (str[*ptr_str] != L'\0');
				*prev = AlignmentToken_Value;
			}
			else if (c == L'{')
			{
				if (*prev != AlignmentToken_None)
					InterpretImpl1Write(ptr_buf, buf, L' ');
				*prev = AlignmentToken_None;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
				(*ptr_str)++;
				(*comment_level)++;
			}
			else if (c == L';')
			{
				*prev = AlignmentToken_None;
				do
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				} while (str[*ptr_str] != L'\0');
			}
			else
			{
				bool is_ope2 = false;
				int len = 1;
				if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
				{
					switch (c)
					{
						case L'^':
						case L'*':
						case L'/':
						case L'%':
						case L'+':
						case L'-':
						case L'~':
						case L'&':
						case L'|':
							is_ope2 = true;
							len = 1;
							break;
						case L'$':
							is_ope2 = true;
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'>':
								case L'<':
									len = 2;
									break;
							}
							break;
						case L'=':
							is_ope2 = true;
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'&':
								case L'$':
									len = 2;
									break;
							}
							break;
						case L'<':
							is_ope2 = true;
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'>':
									len = 2;
									switch (str[*ptr_str + 2])
									{
										case L'&':
										case L'$':
											len = 3;
											break;
									}
									break;
								case L'=':
									len = 2;
									break;
							}
							break;
						case L'>':
							is_ope2 = true;
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'=':
									len = 2;
									break;
							}
							break;
						case L':':
							switch (str[*ptr_str + 1])
							{
								case L'$':
								case L':':
								case L'+':
								case L'-':
								case L'*':
								case L'/':
								case L'%':
								case L'^':
								case L'~':
									is_ope2 = true;
									len = 2;
									break;
							}
							break;
					}
				}
				if (is_ope2)
				{
					if (*prev != AlignmentToken_None)
						InterpretImpl1Write(ptr_buf, buf, L' ');
					*prev = AlignmentToken_Ope2;
				}
				else
				{
					if ((*prev & (AlignmentToken_Ope2 | AlignmentToken_Comma)) != 0)
						InterpretImpl1Write(ptr_buf, buf, L' ');
					switch (c)
					{
						case L'(':
						case L'[':
						case L'.':
							len = 1;
							*prev = AlignmentToken_None;
							break;
						case L')':
						case L']':
							len = 1;
							*prev = AlignmentToken_Pr;
							break;
						case L',':
							len = 1;
							*prev = AlignmentToken_Comma;
							break;
						case L':':
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L':':
									if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
										InterpretImpl1Write(ptr_buf, buf, L' ');
									*prev = AlignmentToken_Ope2;
									len = 2;
									break;
								default:
									*prev = AlignmentToken_Comma;
									break;
							}
							break;
						case L'+':
						case L'-':
						case L'!':
						case L'^':
						case L'*':
							if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
								InterpretImpl1Write(ptr_buf, buf, L' ');
							len = 1;
							*prev = AlignmentToken_None;
							break;
						case L'#':
							if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
								InterpretImpl1Write(ptr_buf, buf, L' ');
							len = 1;
							switch (str[*ptr_str + 1])
							{
								case L'#':
									len = 2;
									break;
							}
							*prev = AlignmentToken_None;
							break;
						case L'?':
							switch (str[*ptr_str + 1])
							{
								case L'(':
									if ((*prev & (AlignmentToken_Value | AlignmentToken_Pr)) != 0)
										InterpretImpl1Write(ptr_buf, buf, L' ');
									len = 2;
									break;
							}
							*prev = AlignmentToken_None;
							break;
						default:
							*prev = AlignmentToken_None;
							break;
					}
				}
				int i;
				for (i = 0; i < len; i++)
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				}
			}
		}
		else
		{
			*prev = AlignmentToken_None;
			if (c == L'"')
			{
				do
				{
					if (str[*ptr_str] == L'\\')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						if (str[*ptr_str] == L'\0')
							break;
					}
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'"')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						break;
					}
				} while (str[*ptr_str] != L'\0');
			}
			else if (c == L'\'')
			{
				do
				{
					if (str[*ptr_str] == L'\\')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						if (str[*ptr_str] == L'\0')
							break;
					}
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
					if (str[*ptr_str] == L'\'')
					{
						Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
						InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
						(*ptr_str)++;
						break;
					}
				} while (str[*ptr_str] != L'\0');
			}
			else if (c == L';')
			{
				do
				{
					Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
					InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
					(*ptr_str)++;
				} while (str[*ptr_str] != L'\0');
			}
			else
			{
				if (c == L'{')
					(*comment_level)++;
				else if (c == L'}')
					(*comment_level)--;
				Interpret1Impl1UpdateCursor(cursor_x, new_cursor_x, ptr_str, ptr_buf);
				InterpretImpl1Write(ptr_buf, buf, str[*ptr_str]);
				(*ptr_str)++;
			}
		}
	}
}

static void InterpretImpl1Write(int* ptr, wchar_t* buf, wchar_t c)
{
	if (*ptr < AUXILIARY_BUF_SIZE)
	{
		buf[*ptr] = c;
		(*ptr)++;
	}
}

static void Interpret1Impl1UpdateCursor(int64_t cursor_x, int64_t* new_cursor_x, int* ptr_str, int* ptr_buf)
{
	if (new_cursor_x != nullptr && cursor_x == (int64_t)*ptr_str)
		*new_cursor_x = (int64_t)*ptr_buf;
}
