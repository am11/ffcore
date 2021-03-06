#include "pch.h"
#include "String/StringUtil.h"

namespace ff
{
	namespace details
	{
		class EmptyString : public SharedStringVectorAllocator::SharedStringVector
		{
		public:
			EmptyString()
			{
				DisableRefs();
				Push(L'\0');
			}
		};
	}
}

// STATIC_DATA (object)
static ff::details::EmptyString s_empty;

ff::StringRef ff::GetEmptyString()
{
	// STATIC_DATA (object)
	static const String s_emptyString;

	return s_emptyString;
}

ff::String::String()
	: _str(&s_empty)
{
}

ff::String::String(const String &rhs)
	: _str(rhs._str)
{
	_str->AddRef();
}

ff::String::String(const String &rhs, size_t pos, size_t count)
	: _str(&s_empty)
{
	assign(rhs, pos, count);
}

ff::String::String(String &&rhs)
	: _str(rhs._str)
{
	rhs._str = &s_empty;
}

ff::String::String(const wchar_t *rhs, size_t count)
	: _str(&s_empty)
{
	assign(rhs, count);
}

ff::String::String(size_t count, wchar_t ch)
	: _str(&s_empty)
{
	assign(count, ch);
}

ff::String::String(const wchar_t *start, const wchar_t *end)
	: _str(&s_empty)
{
	assign(start, end);
}

ff::String::~String()
{
	_str->Release();
}

ff::String &ff::String::operator=(const String &rhs)
{
	return assign(rhs);
}

ff::String &ff::String::operator=(String &&rhs)
{
	return assign(std::move(rhs));
}

ff::String &ff::String::operator=(const wchar_t *rhs)
{
	return assign(rhs);
}

ff::String &ff::String::operator=(wchar_t ch)
{
	return assign(1, ch);
}

ff::String ff::String::operator+(const String &rhs) const
{
	String ret = *this;
	ret.append(rhs);
	return ret;
}

ff::String ff::String::operator+(String &&rhs) const
{
	String ret = *this;
	ret.append(std::move(rhs));
	return ret;
}

ff::String ff::String::operator+(const wchar_t *rhs) const
{
	String ret = *this;
	ret.append(rhs);
	return ret;
}

ff::String ff::String::operator+(wchar_t ch) const
{
	String ret = *this;
	ret.append(1, ch);
	return ret;
}

ff::String &ff::String::operator+=(const String &rhs)
{
	return append(rhs);
}

ff::String &ff::String::operator+=(String &&rhs)
{
	return append(std::move(rhs));
}

ff::String &ff::String::operator+=(const wchar_t *rhs)
{
	return append(rhs);
}

ff::String &ff::String::operator+=(wchar_t ch)
{
	return append(1, ch);
}

bool ff::String::operator==(const String &rhs) const
{
	return compare(rhs) == 0;
}

bool ff::String::operator==(const wchar_t *rhs) const
{
	return compare(rhs) == 0;
}

bool ff::String::operator==(wchar_t ch) const
{
	return compare(&ch, 1) == 0;
}

bool ff::String::operator!=(const String &rhs) const
{
	return compare(rhs) != 0;
}

bool ff::String::operator!=(const wchar_t *rhs) const
{
	return compare(rhs) != 0;
}

bool ff::String::operator!=(wchar_t ch) const
{
	return compare(&ch, 1) != 0;
}

bool ff::String::operator<(const String &rhs) const
{
	return compare(rhs) < 0;
}

bool ff::String::operator<(const wchar_t *rhs) const
{
	return compare(rhs) < 0;
}

bool ff::String::operator<(wchar_t ch) const
{
	return compare(&ch, 1) < 0;
}

bool ff::String::operator<=(const String &rhs) const
{
	return compare(rhs) <= 0;
}

bool ff::String::operator<=(const wchar_t *rhs) const
{
	return compare(rhs) <= 0;
}

bool ff::String::operator<=(wchar_t ch) const
{
	return compare(&ch, 1) <= 0;
}

bool ff::String::operator>(const String &rhs) const
{
	return compare(rhs) > 0;
}

bool ff::String::operator>(const wchar_t *rhs) const
{
	return compare(rhs) > 0;
}

bool ff::String::operator>(wchar_t ch) const
{
	return compare(&ch, 1) > 0;
}

bool ff::String::operator>=(const String &rhs) const
{
	return compare(rhs) >= 0;
}

bool ff::String::operator>=(const wchar_t *rhs) const
{
	return compare(rhs) >= 0;
}

bool ff::String::operator>=(wchar_t ch) const
{
	return compare(&ch, 1) >= 0;
}

ff::String &ff::String::assign(const String &rhs, size_t pos, size_t count)
{
	return replace((size_t)0, size(), rhs, pos, count);
}

ff::String &ff::String::assign(String &&rhs)
{
	std::swap(_str, rhs._str);
	rhs.clear();
	return *this;
}

ff::String &ff::String::assign(const wchar_t *rhs, size_t count)
{
	return replace((size_t)0, size(), rhs, count);
}

ff::String &ff::String::assign(size_t count, wchar_t ch)
{
	return replace((size_t)0, size(), count, ch);
}

ff::String &ff::String::assign(const wchar_t *start, const wchar_t *end)
{
	return replace((size_t)0, size(), start, end - start);
}

ff::String &ff::String::append(const String &rhs, size_t pos, size_t count)
{
	return replace(size(), (size_t)0, rhs, pos, count);
}

ff::String &ff::String::append(const wchar_t *rhs, size_t count)
{
	return replace(size(), (size_t)0, rhs, count);
}

ff::String &ff::String::append(size_t count, wchar_t ch)
{
	return replace(size(), (size_t)0, count, ch);
}

ff::String &ff::String::append(const wchar_t *start, const wchar_t *end)
{
	return replace(size(), (size_t)0, start, end - start);
}

ff::String &ff::String::insert(size_t pos, const String &rhs, size_t rhs_pos, size_t count)
{
	return replace(pos, (size_t)0, rhs, rhs_pos, count);
}

ff::String &ff::String::insert(size_t pos, const wchar_t *rhs, size_t count)
{
	return replace(pos, (size_t)0, rhs, count);
}

ff::String &ff::String::insert(size_t pos, size_t count, wchar_t ch)
{
	return replace(pos, (size_t)0, count, ch);
}

const wchar_t *ff::String::insert(const wchar_t *pos, wchar_t ch)
{
	size_t i = pos - c_str();
	return replace(i, (size_t)0, 1, ch).c_str() + i + 1;
}

const wchar_t *ff::String::insert(const wchar_t *pos, const wchar_t *start, const wchar_t *end)
{
	size_t i = pos - c_str();
	size_t count = end - start;
	return replace(i, (size_t)0, start, count).c_str() + i + count;
}

const wchar_t *ff::String::insert(const wchar_t *pos, size_t count, wchar_t ch)
{
	size_t i = pos - c_str();
	return replace(i, (size_t)0, count, ch).c_str() + i + count;
}

void ff::String::push_back(wchar_t ch)
{
	replace(size(), (size_t)0, 1, ch);
}

void ff::String::pop_back()
{
	replace(size() - 1, 1, (size_t)0, '\0');
}

const wchar_t *ff::String::erase(const wchar_t *start, const wchar_t *end)
{
	size_t i = start - c_str();
	return replace(i, end - start, (size_t)0, '\0').c_str() + i;
}

const wchar_t *ff::String::erase(const wchar_t *pos)
{
	size_t i = pos - c_str();
	return replace(i, 1, (size_t)0, '\0').c_str() + i;
}

ff::String &ff::String::erase(size_t pos, size_t count)
{
	return replace(pos, count, (size_t)0, '\0');
}

ff::String &ff::String::replace(size_t pos, size_t count, const wchar_t *rhs, size_t rhs_count)
{
	if (rhs == nullptr)
	{
		rhs = L"";
	}

	if (rhs_count == INVALID_SIZE)
	{
		rhs_count = wcslen(rhs);
	}

	if (rhs_count > 0 && rhs < c_str() + size() && rhs + rhs_count > c_str())
	{
		// copying chars from INSIDE this string
		return replace(pos, count, String(rhs, rhs_count), 0, rhs_count);
	}

	if (count == INVALID_SIZE || pos + count > size())
	{
		count = size() - pos;
	}

	if (pos == 0 && count == size())
	{
		// special case when the whole string is replaced, no need to clone the storage
		clear();
		count = 0;
	}

	make_editable();

	if (rhs_count > count)
	{
		_str->InsertDefault(pos, rhs_count - count);
	}
	else if (count > rhs_count)
	{
		_str->Delete(pos, count - rhs_count);
	}

	if (rhs_count > 0)
	{
		memcpy(_str->Data(pos), rhs, rhs_count * sizeof(wchar_t));
	}

	return *this;
}

ff::String &ff::String::replace(size_t pos, size_t count, const String &rhs, size_t rhs_pos, size_t rhs_count)
{
	if (rhs_count == INVALID_SIZE || rhs_pos + rhs_count > rhs.size())
	{
		rhs_count = rhs.size() - rhs_pos;
	}

	if (count == INVALID_SIZE || pos + count > size())
	{
		count = size() - pos;
	}

	if (pos == 0 && rhs_pos == 0 && count == size() && rhs_count == rhs.size())
	{
		if (this != &rhs)
		{
			_str->Release();
			_str = rhs._str;
			_str->AddRef();
		}

		return *this;
	}

	return replace(pos, count, rhs.c_str() + rhs_pos, rhs_count);
}

ff::String &ff::String::replace(size_t pos, size_t count, size_t ch_count, wchar_t ch)
{
	if (count == INVALID_SIZE || pos + count > size())
	{
		count = size() - pos;
	}

	if (count > 0 || ch_count > 0)
	{
		make_editable();

		if (ch_count > count)
		{
			_str->InsertDefault(pos, ch_count - count);
		}
		else if (count > ch_count)
		{
			_str->Delete(pos, count - ch_count);
		}

		for (size_t i = 0; i < ch_count; i++)
		{
			_str->SetAt(pos + i, ch);
		}
	}

	return *this;
}

ff::String &ff::String::replace(const wchar_t *start, const wchar_t *end, const wchar_t *rhs, size_t rhs_count)
{
	return replace(start - c_str(), end - start, rhs, rhs_count);
}

ff::String &ff::String::replace(const wchar_t *start, const wchar_t *end, const String &rhs)
{
	return replace(start - c_str(), end - start, rhs, (size_t)0, rhs.size());
}

ff::String &ff::String::replace(const wchar_t *start, const wchar_t *end, size_t ch_count, wchar_t ch)
{
	return replace(start - c_str(), end - start, ch_count, ch);
}

ff::String &ff::String::replace(const wchar_t *start, const wchar_t *end, const wchar_t *start2, const wchar_t *end2)
{
	return replace(start - c_str(), end - start, start2, end2 - start2);
}

ff::String::iterator ff::String::begin()
{
	make_editable();
	return _str->begin();
}

ff::String::const_iterator ff::String::begin() const
{
	return _str->cbegin();
}

ff::String::const_iterator ff::String::cbegin() const
{
	return _str->cbegin();
}

ff::String::iterator ff::String::end()
{
	make_editable();
	return _str->end();
}

ff::String::const_iterator ff::String::end() const
{
	return _str->cend();
}

ff::String::const_iterator ff::String::cend() const
{
	return _str->cend();
}

ff::String::reverse_iterator ff::String::rbegin()
{
	make_editable();
	return _str->rbegin();
}

ff::String::const_reverse_iterator ff::String::rbegin() const
{
	return _str->crbegin();
}

ff::String::const_reverse_iterator ff::String::crbegin() const
{
	return _str->crbegin();
}

ff::String::reverse_iterator ff::String::rend()
{
	make_editable();
	return _str->rend();
}

ff::String::const_reverse_iterator ff::String::rend() const
{
	return _str->crend();
}

ff::String::const_reverse_iterator ff::String::crend() const
{
	return _str->crend();
}

wchar_t &ff::String::front()
{
	make_editable();
	return _str->GetAt(0);
}

const wchar_t &ff::String::front() const
{
	return _str->GetAt(0);
}

wchar_t &ff::String::back()
{
	make_editable();
	return _str->GetAt(size() - 1);
}

const wchar_t &ff::String::back() const
{
	return _str->GetAt(size() - 1);
}

wchar_t &ff::String::at(size_t pos)
{
	make_editable();
	return _str->GetAt(pos);
}

const wchar_t &ff::String::at(size_t pos) const
{
	return _str->GetAt(pos);
}

wchar_t &ff::String::operator[](size_t pos)
{
	make_editable();
	return _str->GetAt(pos);
}

const wchar_t &ff::String::operator[](size_t pos) const
{
	return _str->GetAt(pos);
}

const wchar_t *ff::String::c_str() const
{
	return _str->ConstData();
}

const wchar_t *ff::String::data() const
{
	return _str->ConstData();
}

size_t ff::String::length() const
{
	return _str->Size() - 1; // don't include null char
}

size_t ff::String::size() const
{
	return _str->Size() - 1; // don't include null char
}

bool ff::String::empty() const
{
	return _str->Size() == 1;
}

size_t ff::String::max_size() const
{
	// allows size() to be cast to an int
	return 0x7FFFFFFF;
}

size_t ff::String::capacity() const
{
	return _str->Allocated() - 1;
}

void ff::String::clear()
{
	if (_str != &s_empty)
	{
		_str->Release();
		_str = &s_empty;
	}
}

void ff::String::resize(size_t count)
{
	if (count > size())
	{
		make_editable();
		_str->InsertDefault(size(), count - size());
	}
	else if (count < size())
	{
		replace(count, size() - count, (size_t)0, '\0');
	}
}

void ff::String::resize(size_t count, wchar_t ch)
{
	if (count > size())
	{
		replace(size(), (size_t)0, count - size(), ch);
	}
	else if (count < size())
	{
		replace(count, size() - count, (size_t)0, '\0');
	}
}

void ff::String::reserve(size_t alloc)
{
	make_editable();
	_str->Reserve(alloc + 1);
}

void ff::String::shrink_to_fit()
{
	make_editable();
	_str->Reduce();
}

size_t ff::String::copy(wchar_t * out, size_t count, size_t pos)
{
	if (count == INVALID_SIZE || pos + count > size())
	{
		count = size() - pos;
	}

	memmove(out, c_str() + pos, count * sizeof(wchar_t));
	return count;
}

void ff::String::swap(String &rhs)
{
	std::swap(_str, rhs._str);
}

ff::String ff::String::substr(size_t pos, size_t count) const
{
	return String(*this, pos, count);
}

bool ff::String::format(const wchar_t *format, ...)
{
	va_list args;
	va_start(args, format);
	bool result = format_v(format, args);
	va_end(args);

	return result;
}

bool ff::String::format_v(const wchar_t *format, va_list args)
{
	int count = _vsctprintf(format, args);
	assertRetVal(count >= 0, false);

	resize(count);
	_vsnwprintf_s(&*begin(), size() + 1, _TRUNCATE, format, args);

	return true;
}

// static
ff::String ff::String::format_new(const wchar_t *format, ...)
{
	va_list args;
	va_start(args, format);
	String str = format_new_v(format, args);
	va_end(args);

	return str;
}

// static
ff::String ff::String::format_new_v(const wchar_t *format, va_list args)
{
	String str;
	str.format_v(format, args);
	return str;
}

// static
ff::String ff::String::from_acp(const char *str)
{
	return StringFromACP(str);
}

// static
ff::String ff::String::from_static(const wchar_t *str, size_t len)
{
	String result;
	assertRetVal(str, result);
	noAssertRetVal(*str, result);

	result.make_editable();
	result._str->SetStaticData(str, ((len == npos) ? wcslen(str) : len) + 1);
	return result;
}

BSTR ff::String::bstr() const
{
	return ::SysAllocStringLen(c_str(), static_cast<UINT>(length()));
}

#if METRO_APP
Platform::String ^ff::String::pstring() const
{
	return ref new Platform::String(c_str(), static_cast<unsigned int>(length()));
}

// static
ff::String ff::String::from_pstring(Platform::String ^str)
{
	return str != nullptr
		? String(str->Data(), str->Length())
		: String();
}
#endif

size_t ff::String::find(const String &rhs, size_t pos) const
{
	return find(rhs.c_str(), pos, rhs.size());
}

size_t ff::String::find(const wchar_t *rhs, size_t pos, size_t count) const
{
	if (count == INVALID_SIZE)
	{
		count = wcslen(rhs);
	}

	if (count > 0 && count <= size())
	{
		const wchar_t *lhs = c_str() + pos;
		for (size_t last = size() - count; pos <= last; pos++, lhs++)
		{
			size_t i = 0;
			for (; i < count; i++)
			{
				if (lhs[i] != rhs[i])
				{
					break;
				}
			}

			if (i == count)
			{
				return pos;
			}
		}
	}

	return INVALID_SIZE;
}

size_t ff::String::find(wchar_t ch, size_t pos) const
{
	return find(&ch, pos, 1);
}

size_t ff::String::rfind(const String &rhs, size_t pos) const
{
	return rfind(rhs.c_str(), pos, rhs.size());
}

size_t ff::String::rfind(const wchar_t *rhs, size_t pos, size_t count) const
{
	if (size() > 0)
	{
		if (pos == INVALID_SIZE)
		{
			pos = size() - 1;
		}

		if (count == INVALID_SIZE)
		{
			count = wcslen(rhs);
		}

		if (count > 0 && count <= size())
		{
			if (pos > size() - count)
			{
				pos = size() - count;
			}

			const wchar_t *lhs = c_str() + pos;
			for (; pos != INVALID_SIZE; pos = PreviousSize(pos), lhs--)
			{
				size_t i = 0;
				for (; i < count; i++)
				{
					if (lhs[i] != rhs[i])
					{
						break;
					}
				}

				if (i == count)
				{
					return pos;
				}
			}
		}
	}

	return INVALID_SIZE;
}

size_t ff::String::rfind(wchar_t ch, size_t pos) const
{
	return rfind(&ch, pos, 1);
}

size_t ff::String::find_first_of(const String &rhs, size_t pos) const
{
	return find_first_of(rhs.c_str(), pos, rhs.size());
}

size_t ff::String::find_first_of(const wchar_t *rhs, size_t pos, size_t count) const
{
	if (count == INVALID_SIZE)
	{
		count = wcslen(rhs);
	}

	const wchar_t *lhs = c_str() + pos;
	for (size_t endPos = size(); pos < endPos; pos++, lhs++)
	{
		for (size_t i = 0; i < count; i++)
		{
			if (*lhs == rhs[i])
			{
				return pos;
			}
		}
	}

	return INVALID_SIZE;
}

size_t ff::String::find_first_of(wchar_t ch, size_t pos) const
{
	return find_first_of(&ch, pos, 1);
}

size_t ff::String::find_last_of(const String &rhs, size_t pos) const
{
	return find_last_of(rhs.c_str(), pos, rhs.size());
}

size_t ff::String::find_last_of(const wchar_t *rhs, size_t pos, size_t count) const
{
	if (size() > 0)
	{
		if (pos == INVALID_SIZE)
		{
			pos = size() - 1;
		}

		if (count == INVALID_SIZE)
		{
			count = wcslen(rhs);
		}

		const wchar_t *lhs = c_str() + pos;
		for (; pos != INVALID_SIZE; pos = PreviousSize(pos), lhs--)
		{
			for (size_t i = 0; i < count; i++)
			{
				if (*lhs == rhs[i])
				{
					return pos;
				}
			}
		}
	}

	return INVALID_SIZE;
}

size_t ff::String::find_last_of(wchar_t ch, size_t pos) const
{
	return find_last_of(&ch, pos, 1);
}

size_t ff::String::find_first_not_of(const String &rhs, size_t pos) const
{
	return find_first_not_of(rhs.c_str(), pos, rhs.size());
}

size_t ff::String::find_first_not_of(const wchar_t *rhs, size_t pos, size_t count) const
{
	if (count == INVALID_SIZE)
	{
		count = wcslen(rhs);
	}

	const wchar_t *lhs = c_str() + pos;
	for (size_t endPos = size(); pos < endPos; pos++, lhs++)
	{
		size_t i = 0;
		for (; i < count; i++)
		{
			if (*lhs == rhs[i])
			{
				break;
			}
		}

		if (i == count)
		{
			return pos;
		}
	}

	return INVALID_SIZE;
}

size_t ff::String::find_first_not_of(wchar_t ch, size_t pos) const
{
	return find_first_not_of(&ch, pos, 1);
}

size_t ff::String::find_last_not_of(const String &rhs, size_t pos) const
{
	return find_last_not_of(rhs.c_str(), pos, rhs.size());
}

size_t ff::String::find_last_not_of(const wchar_t *rhs, size_t pos, size_t count) const
{
	if (size() > 0)
	{
		if (pos == INVALID_SIZE)
		{
			pos = size() - 1;
		}

		if (count == INVALID_SIZE)
		{
			count = wcslen(rhs);
		}

		const wchar_t *lhs = c_str() + pos;
		for (; pos != INVALID_SIZE; pos = PreviousSize(pos), lhs--)
		{
			size_t i = 0;
			for (; i < count; i++)
			{
				if (*lhs == rhs[i])
				{
					break;
				}
			}

			if (i == count)
			{
				return pos;
			}
		}
	}

	return INVALID_SIZE;
}

size_t ff::String::find_last_not_of(wchar_t ch, size_t pos) const
{
	return find_last_not_of(&ch, pos, 1);
}

int ff::String::compare(const String &rhs, size_t rhs_pos, size_t rhs_count) const
{
	return compare(0, size(), rhs.c_str(), rhs.size());
}

int ff::String::compare(size_t pos, size_t count, const String &rhs, size_t rhs_pos, size_t rhs_count) const
{
	if (rhs_count == INVALID_SIZE || rhs_pos + rhs_count > rhs.size())
	{
		rhs_count = rhs.size() - rhs_pos;
	}

	return compare(pos, count, rhs.c_str() + rhs_pos, rhs_count);
}

int ff::String::compare(const wchar_t *rhs, size_t rhs_count) const
{
	return compare(0, size(), rhs, rhs_count);
}

int ff::String::compare(size_t pos, size_t count, const wchar_t *rhs, size_t rhs_count) const
{
	if (count == INVALID_SIZE || pos + count > size())
	{
		count = size() - pos;
	}

	if (rhs_count == INVALID_SIZE)
	{
		rhs_count = wcslen(rhs);
	}

	const wchar_t *lhs = c_str() + pos;
	size_t compareCount = std::min(count, rhs_count);

	for (size_t i = 0; i < compareCount; i++)
	{
		int diff = static_cast<int>(lhs[i]) - static_cast<int>(rhs[i]);
		if (diff != 0)
		{
			return diff;
		}
	}

	return static_cast<int>(count) - static_cast<int>(rhs_count);
}

void ff::String::make_editable()
{
	SharedStringVectorAllocator::SharedStringVector::GetUnshared(&_str);
}

ff::StaticString::StaticString(const wchar_t *sz, size_t len)
{
	Initialize(sz, len + 1);
}

const ff::StringRef ff::StaticString::GetString() const
{
	return *(String *)&_str;
}

ff::StaticString::operator ff::StringRef() const
{
	return GetString();
}

void ff::StaticString::Initialize(const wchar_t *sz, size_t lenWithNull)
{
	assert(lenWithNull > 0 && !sz[lenWithNull - 1]);

	_str = &_data;
	_data.DisableRefs();
	_data.SetStaticData(sz, lenWithNull);
}
