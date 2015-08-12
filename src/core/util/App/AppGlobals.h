#pragma once

#include "App/AppTime.h"
#include "App/MostRecentlyUsed.h"
#include "App/SplashScreen.h"
#include "App/Timer.h"
#include "Dict/Dict.h"
#include "UI/IMainWindowHost.h"
#include "Windows/WinUtil.h"

namespace ff
{
	class IDataWriter;
	class IMainWindow;
	class IServiceCollection;
	class IServiceProvider;
	class Log;
	class Module;
	class ProcessGlobals;

	class AppGlobals
		: public IMainWindowHost
		, public IMostRecentlyUsedListener
#if !METRO_APP
		, public IThreadMessageFilter
#endif
	{
	public:
		UTIL_API AppGlobals(ProcessGlobals &processGlobals);
		UTIL_API virtual ~AppGlobals();

		// Starts running the app
		UTIL_API virtual bool WinMain();
		UTIL_API virtual bool Show(StringRef args);
		UTIL_API virtual bool Close();

		// App information
		UTIL_API virtual String GetCompanyName() const;
		UTIL_API virtual String GetAppName() const;
		UTIL_API virtual REFGUID GetAppId() const;

		// State checks
		UTIL_API bool IsValid() const;
		UTIL_API bool IsFirstRun() const;
		UTIL_API bool IsAdvancing() const;
		UTIL_API bool IsRendering() const;
		UTIL_API bool IsQuitting() const;
		UTIL_API bool IsAutomation() const;

		// Properties
		UTIL_API ProcessGlobals &GetProcessGlobals() const;
		UTIL_API Dict &GetOptions();
		UTIL_API Dict &GetNamedOptions(StringRef name);
		UTIL_API const AppGlobalTime &GetGlobalTime() const;
		UTIL_API const AppFrameTime &GetFrameTime() const;
		UTIL_API const MostRecentlyUsed &GetMru() const;
		UTIL_API MostRecentlyUsed &GetMru();
		UTIL_API Log &GetLog() const;
		UTIL_API IServiceProvider *GetServices() const;
		UTIL_API virtual double GetTimeScale() const;
		UTIL_API virtual size_t GetMaxAdvances() const;
		UTIL_API virtual const Module &GetModule() const;
		UTIL_API String SetStatusText(StringRef text, IUnknown **resetObj = nullptr);
		UTIL_API String GetStatusText() const;

		// Data files
		UTIL_API virtual String GetUserDataDirectory(StringRef subDir = GetEmptyString()) const;
		UTIL_API virtual String GetOptionsFile() const;
		UTIL_API virtual String GetScoresFile() const;
		UTIL_API virtual String GetLogFile() const;

		// Resources
		UTIL_API virtual UINT GetSplashScreenBitmapResource() const;

		// App state
		UTIL_API virtual void SaveState();
		UTIL_API virtual void RestoreState();

		// Main windows
		UTIL_API size_t GetWindowCount() const;
		UTIL_API std::shared_ptr<IMainWindow> GetWindow(size_t index) const;
		UTIL_API Vector<std::shared_ptr<IMainWindow>> GetOrderedWindows() const;
		UTIL_API std::shared_ptr<IMainWindow> GetActiveWindow();
		UTIL_API std::shared_ptr<IMainWindow> GetLastActiveWindow();
		UTIL_API std::shared_ptr<IMainWindow> OpenNewWindow(StringRef typeName, IServiceProvider *contextServices);
		UTIL_API bool RequestFrameUpdate(IMainWindow *window, bool vsync);

		// IMainWindowHost
		UTIL_API virtual void OnWindowCreated(IMainWindow &window) override;
		UTIL_API virtual void OnWindowInitialized(IMainWindow &window) override;
		UTIL_API virtual void OnWindowActivated(IMainWindow &window) override;
		UTIL_API virtual void OnWindowDeactivated(IMainWindow &window) override;
		UTIL_API virtual bool OnWindowClosing(IMainWindow &window) override;
		UTIL_API virtual void OnWindowDestroyed(IMainWindow &window) override;
		UTIL_API virtual void OnWindowModalStart(IMainWindow &window) override;
		UTIL_API virtual void OnWindowModalEnd(IMainWindow &window) override;
		UTIL_API virtual void OnWindowSuspending(IMainWindow &window) override;
		UTIL_API virtual void OnWindowResumed(IMainWindow &window) override;

		// IMostRecentlyUsedListener
		UTIL_API virtual void OnMruChanged(const MostRecentlyUsed &mru) override;

#if !METRO_APP
		// IThreadMessageFilter
		UTIL_API virtual bool FilterMessage(MSG &msg) override;
#endif

	protected:
		// Called from WinMain
		UTIL_API virtual bool CanStartup();
		UTIL_API virtual bool Startup();
		UTIL_API virtual void Shutdown();
		UTIL_API virtual void Run();

		// Called from Run
		UTIL_API virtual void FrameMessagesHandled(IMainWindow *window);
		UTIL_API virtual void FrameResetTimer(IMainWindow *window);
		UTIL_API virtual bool FrameAdvance(IMainWindow *window);
		UTIL_API virtual void FrameRender(IMainWindow *window);
		UTIL_API virtual void FramePresent(IMainWindow *window, bool vsync);
		UTIL_API virtual void FrameVsync(IMainWindow *window);
		UTIL_API virtual void FrameCleanup(IMainWindow *window);

		// Command line arguments
		UTIL_API virtual void ScanCommandLine();
		UTIL_API virtual bool ParseCommandLine();
		UTIL_API virtual bool HandleCommandLineArgument(List<String> &args);
		UTIL_API virtual bool ShowHelp();

		// Init/Quit
		UTIL_API virtual void InitializeTempDirectory();
		UTIL_API virtual void InitializeDefaultOptions(Dict &dict);
		UTIL_API virtual void InitializeLog();
		UTIL_API virtual bool InitializeMainWindow();
		UTIL_API virtual bool Initialize();
		UTIL_API virtual void Cleanup();

		// Locking the app running
		UTIL_API virtual void LockApp();
		UTIL_API virtual void UnlockApp();
		UTIL_API virtual bool CanQuit() const;

		// Main windows
		UTIL_API virtual std::shared_ptr<IMainWindow> NewWindow(StringRef typeName, IServiceProvider *contextServices);

		// Options
		UTIL_API virtual void LoadOptions();
		UTIL_API virtual void SaveOptions();
		UTIL_API virtual void ClearOptions();
		UTIL_API virtual void LoadMru();
		UTIL_API virtual void SaveMru();
		UTIL_API virtual void CleanLoadedOptions();
		UTIL_API virtual void CleanSavingOptions();

		// High scores
		UTIL_API virtual void LoadScores();
		UTIL_API virtual void SaveScores();
		UTIL_API virtual void ClearScores();

		// Services
		UTIL_API IServiceCollection *GetServiceCollection() const;

	private:
		void InternalInitializeDefaultOptions();
		bool InternalInitialize();
		void InternalCleanup();

		ProcessGlobals &_processGlobals;
		ComPtr<IServiceCollection> _services;
		ComPtr<IDataWriter> _logWriter;
		List<std::shared_ptr<IMainWindow>> _mainWindowsOrder;
		Vector<std::shared_ptr<IMainWindow>> _mainWindowsVector;
		Vector<std::shared_ptr<IMainWindow>> _destroyedWindows;
		String _appOptionsName;
		Map<String, Dict> _namedOptions;
		SplashScreen _splashScreen;
		MostRecentlyUsed _mru;
		Dict _defaultOptions;
		Timer _frameTimer;
		AppGlobalTime _globalTime;
		AppFrameTime _frameTime;
		String _logFile;
		String _status;
		int _advancingGame;
		int _renderingGame;
		long _lock;
		bool _automation;
		bool _firstRun;
		bool _valid;
	};
}
