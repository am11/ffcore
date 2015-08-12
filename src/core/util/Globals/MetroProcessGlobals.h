#pragma once
#if METRO_APP

#include "Globals/ProcessGlobals.h"

namespace ff
{
	class MetroProcessGlobals : public ProcessGlobals
	{
	public:
		UTIL_API MetroProcessGlobals();
		UTIL_API virtual ~MetroProcessGlobals();

		UTIL_API virtual bool Startup() override;
		UTIL_API virtual void Shutdown() override;
	};

	typedef MetroProcessGlobals WinProcessGlobalsType;
	UTIL_API MetroProcessGlobals *GetPlatformProcessGlobals();
}

#endif // METRO_APP
