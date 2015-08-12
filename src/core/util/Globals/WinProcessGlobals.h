#pragma once
#if !METRO_APP

#include "Globals/ProcessGlobals.h"

namespace ff
{
	class WinProcessGlobals : public ProcessGlobals
	{
	public:
		UTIL_API WinProcessGlobals();
		UTIL_API virtual ~WinProcessGlobals();

		UTIL_API virtual bool Startup() override;
		UTIL_API virtual void Shutdown() override;
	};

	typedef WinProcessGlobals WinProcessGlobalsType;
	UTIL_API WinProcessGlobals *GetPlatformProcessGlobals();
}

#endif !METRO_APP
