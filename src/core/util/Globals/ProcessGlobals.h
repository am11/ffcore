#pragma once

#include "App/Log.h"
#include "Globals/ThreadGlobals.h"
#include "Module/Modules.h"
#include "String/StringCache.h"
#include "String/StringManager.h"

namespace ff
{
	class IAsyncDataLoader;
	class IAudioFactory;
	class IGraphicFactory;
	class IIdleMaster;
	class IServiceCollection;
	class IThreadPool;

	class ProcessGlobals : public ThreadGlobals
	{
	public:
		UTIL_API ProcessGlobals();
		UTIL_API virtual ~ProcessGlobals();

		UTIL_API static ProcessGlobals *Get();

		virtual bool Startup() override;
		virtual void Shutdown() override;
#if !METRO_APP
		UTIL_API virtual void KillProcess(unsigned int code);
#endif
		UTIL_API Log &GetLog();
		UTIL_API Modules &GetModules();
		UTIL_API StringManager &GetStringManager();
		UTIL_API StringCache *GetStringCache();

		UTIL_API IAsyncDataLoader *GetAsyncDataLoader();
		UTIL_API IAudioFactory *GetAudioFactory();
		UTIL_API IGraphicFactory *GetGraphicFactory();
		UTIL_API IIdleMaster *GetIdleMaster();
		UTIL_API IServiceCollection *GetServices();
		UTIL_API IThreadPool *GetThreadPool();

	private:
		Log _log;
		StringManager _stringManager;
		Modules _modules;
		StringCache _stringCache;

		ComPtr<IAsyncDataLoader> _asyncDataLoader;
		ComPtr<IAudioFactory> _audioFactory;
		ComPtr<IGraphicFactory> _graphicFactory;
		ComPtr<IIdleMaster> _idleMaster;
		ComPtr<IServiceCollection> _services;
		ComPtr<IThreadPool> _threadPool;
	};

	UTIL_API bool DidProgramStart();
	UTIL_API bool IsProgramRunning();
	UTIL_API bool IsProgramShuttingDown();
	UTIL_API void AtProgramShutdown(std::function<void()> func);
}
