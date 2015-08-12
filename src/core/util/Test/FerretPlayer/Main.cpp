#include "pch.h"
#include <Globals/MetroProcessGlobals.h>
#include <Globals/GlobalsScope.h>
#include <MainUtilInclude.h>
#include <Module/MetroModule.h>
#include <String/StringUtil.h>

#include "App.xaml.h"

namespace FerretPlayer
{
	// {65673519-C5AF-4BF8-AF82-4E6889093D5B}
	static const GUID s_moduleId = { 0x65673519, 0xc5af, 0x4bf8, { 0xaf, 0x82, 0x4e, 0x68, 0x89, 0x9, 0x3d, 0x5b } };
	static ff::StaticString s_moduleName(L"FerretPlayer");

	static ff::ModuleFactory CreateThisModule(s_moduleName, s_moduleId, ff::GetDelayLoadInstance, ff::GetModuleStartup,
	[]()
	{
		return std::unique_ptr<ff::Module>(
			new ff::WinModuleType(s_moduleName, s_moduleId, ff::GetDelayLoadInstance()));
	});
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

[Platform::MTAThread]
int main(Platform::Array<Platform::String ^> ^args)
{
	HINSTANCE instance = reinterpret_cast<HINSTANCE>(&__ImageBase);

	ff::SetCommandLineArgs(args);
	ff::SetDelayLoadInstance(instance);
	ff::SetMainModule(FerretPlayer::s_moduleName, FerretPlayer::s_moduleId, instance);

	Windows::UI::Xaml::Application::Start(
		ref new Windows::UI::Xaml::ApplicationInitializationCallback(
			[](Windows::UI::Xaml::ApplicationInitializationCallbackParams ^args)
			{
				auto app = ref new ::FerretPlayer::App();
			}));

	return 0;
}
