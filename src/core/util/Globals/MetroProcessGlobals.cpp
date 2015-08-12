#include "pch.h"
#include "Globals/MetroProcessGlobals.h"

#if METRO_APP

ff::MetroProcessGlobals *ff::GetPlatformProcessGlobals()
{
	return static_cast<MetroProcessGlobals *>(ProcessGlobals::Get());
}

ff::MetroProcessGlobals::MetroProcessGlobals()
{
}

ff::MetroProcessGlobals::~MetroProcessGlobals()
{
}

bool ff::MetroProcessGlobals::Startup()
{
	assertRetVal(ff::ProcessGlobals::Startup(), false);

	return true;
}

void ff::MetroProcessGlobals::Shutdown()
{
	return ff::ProcessGlobals::Shutdown();
}

#endif // METRO_APP
