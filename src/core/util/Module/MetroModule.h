#pragma once
#if METRO_APP

#include "Module/Module.h"

namespace ff
{
	class MetroModule : public Module
	{
	public:
		UTIL_API MetroModule(StringRef name, REFGUID id, HINSTANCE instance);

		virtual bool GetAsset(unsigned int id, IDataReader **data) const override;
		virtual String GetString(unsigned int id) const override;
	};

	typedef MetroModule WinModuleType;
}

#endif // METRO_APP
