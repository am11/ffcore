#pragma once

namespace ff
{
	/// A simple way to pass BSTRs to Windows APIs that need one
	class SysString
	{
	public:
		UTIL_API SysString();
		UTIL_API SysString(StringRef str);
		UTIL_API ~SysString();

		UTIL_API operator BSTR() const;
		UTIL_API BSTR *operator &();

	private:
		BSTR _bstr;
	};
}
