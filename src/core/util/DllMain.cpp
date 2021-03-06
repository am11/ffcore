#include "pch.h"
#include "MainUtilInclude.h"
#include "Module/MetroModule.h"
#include "Module/WinModule.h"
#include "Resource/util-resource.h"

#if !METRO_APP
#include <FerretFace.Interop_i.c>
#endif

// {0C55D6ED-16E8-404C-9E11-211A81AA74F8}
static const GUID s_moduleId = {0x0c55d6ed,0x16e8,0x404c,{0x9e,0x11,0x21,0x1a,0x81,0xaa,0x74,0xf8}};
static ff::StaticString s_moduleName(L"util");

static ff::ModuleFactory CreateThisModule(s_moduleName, s_moduleId, ff::GetDelayLoadInstance, ff::GetModuleStartup,
[]()
{
	return std::unique_ptr<ff::Module>(
		new ff::WinModuleType(s_moduleName, s_moduleId, ff::GetDelayLoadInstance()));
});

#ifndef _LIB

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		ff::SetDelayLoadInstance(instance);
		ff::SetThisModule(s_moduleName, s_moduleId, instance);
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

#endif
