#include "pch.h"
#include "String/StringUtil.h"
#include "String/SysString.h"

ff::SysString::SysString()
	: _bstr(nullptr)
{
}

ff::SysString::SysString(StringRef str)
	: _bstr(ff::StringToBSTR(str))
{
}

ff::SysString::~SysString()
{
	if (_bstr)
	{
		::SysFreeString(_bstr);
	}
}

ff::SysString::operator BSTR() const
{
	return _bstr;
}

BSTR *ff::SysString::operator &()
{
	assert(_bstr == nullptr);
	return &_bstr;
}
