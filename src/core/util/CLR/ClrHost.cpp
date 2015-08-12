#include "pch.h"
#include "CLR/ClrHost.h"
#include "CLR/HostControl.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Module/Module.h"
#include "Resource/util-resource.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

#if METRO_APP

static bool s_unusedFile_ClrHost = false;

#else

static ff::ComPtr<ICLRMetaHost> s_metaHost;
static ff::ComPtr<ICLRRuntimeInfo> s_clrInfo;
static ff::ComPtr<ICLRRuntimeHost> s_clrHost;
static ff::ComPtr<ICLRControl> s_clrControl;
static ff::ComPtr<ICLRGCManager> s_gcManager;
static ff::INativeHostControl *s_hostControl = nullptr;

static ff::String GetClrVersion(ICLRMetaHost *metaHost, ff::StringRef versionOrAssembly)
{
	ff::String version;
	assertRetVal(versionOrAssembly.size(), version);

	if (!_wcsicmp(ff::GetPathExtension(versionOrAssembly).c_str(), L"dll"))
	{
		ff::String assembly = ff::GetExecutablePath();
		ff::StripPathTail(assembly);
		ff::AppendPathTail(assembly, versionOrAssembly);

		if (ff::FileExists(assembly))
		{
			wchar_t versionBuf[32];
			DWORD count = _countof(versionBuf);
			assertRetVal(SUCCEEDED(metaHost->GetVersionFromFile(assembly.c_str(), versionBuf, &count)), version);
			version = versionBuf;
		}
		else
		{
			assertSz(false, ff::String::format_new(L"Managed DLL doesn't exist: %s", assembly.c_str()).c_str());
		}
	}
	else if (versionOrAssembly[0] == 'v')
	{
		version = versionOrAssembly;
	}
	else
	{
		assertRetVal(false, version);
	}

	return version;
}

static bool GetAppConfig(ff::IDataFile **file)
{
	assertRetVal(file, false);
	const char *configXml =
		"<?xml version='1.0' encoding='utf-8'?>" "\r\n"
		"<configuration>" "\r\n"
		"  <runtime>" "\r\n"
		"    <assemblyBinding xmlns='urn:schemas-microsoft-com:asm.v1'>" "\r\n"
		"    </assemblyBinding>" "\r\n"
		"  </runtime>" "\r\n"
		"</configuration>" "\r\n";

	ff::ComPtr<ff::IDataFile> configFile;
	{
		// Read the config file out of a resource and save it into a temp file
		ff::ComPtr<ff::IDataWriter> configWriter;
		ff::ComPtr<ff::IData> configData;
		assertRetVal(ff::CreateTempDataFile(&configFile), false);
		assertRetVal(ff::CreateDataWriter(configFile, 0, &configWriter), false);
		assertRetVal(configWriter->Write(configXml, strlen(configXml)), false);
	}

	*file = configFile.Detach();
	return *file != nullptr;
}

ff::INativeHostControl *ff::ClrStartup(StringRef versionOrAssembly)
{
	// Use my own empty config file
	ComPtr<IDataFile> configFile;
	assertRetVal(GetAppConfig(&configFile), false);

	return ClrStartup(configFile->GetPath(), versionOrAssembly);
}

ff::INativeHostControl *ff::ClrStartup(StringRef configFile, StringRef versionOrAssembly)
{
	ff::ComPtr<ICLRMetaHost> metaHost;
	ff::ComPtr<ICLRRuntimeInfo> clrInfo;
	ff::ComPtr<ICLRRuntimeHost> clrHost;
	ff::ComPtr<ICLRControl> clrControl;
	ff::ComPtr<ICLRGCManager> gcManager;
	ff::ComPtr<ff::INativeHostControl> hostControl;

	assertRetVal(SUCCEEDED(CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (void **)&metaHost)), nullptr);

	assertRetVal(SUCCEEDED(metaHost->GetRuntime(GetClrVersion(metaHost, versionOrAssembly).c_str(), IID_ICLRRuntimeInfo, (void **)&clrInfo)), nullptr);
	assertRetVal(SUCCEEDED(clrInfo->GetInterface(CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost, (void **)&clrHost)), nullptr);
	assertRetVal(SUCCEEDED(clrInfo->SetDefaultStartupFlags(STARTUP_CONCURRENT_GC | STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST, configFile.c_str())), nullptr);

	assertRetVal(SUCCEEDED(CreateHostControl(&hostControl)), nullptr);
	assertRetVal(SUCCEEDED(clrHost->SetHostControl(hostControl)), nullptr);
	assertRetVal(SUCCEEDED(clrHost->GetCLRControl(&clrControl)), nullptr);
	assertRetVal(SUCCEEDED(clrControl->GetCLRManager(IID_ICLRGCManager, (void **)&gcManager)), nullptr);

	assertRetVal(SUCCEEDED(clrHost->Start()), nullptr);
	assertRetVal(hostControl->Startup(), nullptr);

	s_metaHost = metaHost;
	s_clrInfo = clrInfo;
	s_clrHost = clrHost;
	s_clrControl = clrControl;
	s_gcManager = gcManager;
	s_hostControl = hostControl.Detach();

	return s_hostControl;
}

void ff::ClrShutdown()
{
	if (s_hostControl)
	{
		s_hostControl->Shutdown();
		s_hostControl = nullptr; // never deleted
	}

	// s_clrHost->Stop(); // this would kill the process
	s_gcManager = nullptr;
	s_clrControl = nullptr;
	s_clrHost = nullptr;
	s_clrInfo = nullptr;
	s_metaHost = nullptr;
}

bool ff::IsClrRunning()
{
	return s_hostControl != nullptr;
}

#endif // !METRO_APP
