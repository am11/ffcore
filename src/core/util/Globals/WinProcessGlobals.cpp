#include "pch.h"
#include "Globals/WinProcessGlobals.h"

#if !METRO_APP

ff::WinProcessGlobals *GetPlatformProcessGlobals()
{
	return static_cast<ff::WinProcessGlobals *>(ff::ProcessGlobals::Get());
}

ff::WinProcessGlobals::WinProcessGlobals()
{
}

ff::WinProcessGlobals::~WinProcessGlobals()
{
}

bool ff::WinProcessGlobals::Startup()
{
	assertRetVal(SUCCEEDED(::OleInitialize(nullptr)), false);

	assertRetVal(ff::ProcessGlobals::Startup(), false);

	INITCOMMONCONTROLSEX cc;
	cc.dwSize = sizeof(cc);
	cc.dwICC  = ICC_WIN95_CLASSES;
	verify(::InitCommonControlsEx(&cc));

	return true;
}

void ff::WinProcessGlobals::Shutdown()
{
	ff::ProcessGlobals::Shutdown();

	::OleUninitialize();
}

#endif // !METRO_APP
