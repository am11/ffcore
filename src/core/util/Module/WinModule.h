#pragma once
#if !METRO_APP

#include "Module/Module.h"

namespace ff
{
	class WinModule : public Module
	{
	public:
		UTIL_API WinModule(StringRef name, REFGUID id, HINSTANCE instance);

		virtual bool GetAsset(unsigned int id, IDataReader **data) const override;
		virtual String GetString(unsigned int id) const override;

	private:
	};

	typedef WinModule WinModuleType;
}

#endif // !METRO_APP
