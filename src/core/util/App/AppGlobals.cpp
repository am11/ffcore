#include "pch.h"
#include "App/AppGlobals.h"
#include "App/EventTimer.h"
#include "Audio/AudioDevice.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Dict/DictPersist.h"
#include "COM/ServiceCollection.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Resource/util-resource.h"
#include "String/StringUtil.h"
#include "UI/MainWindow.h"
#include "Windows/FileUtil.h"

void LogWithTime(ff::Log &log, unsigned int id)
{
	ff::String text = ff::GetThisModule().GetFormattedString(IDS_APP_LOG_WITH_TIME,
		ff::GetDateAsString().c_str(),
		ff::GetTimeAsString().c_str(),
		id,
		ff::GetThisModule().GetString(id).c_str());
	log.TraceLineF(text.c_str());
}

static bool VerifyCpuSupport(ff::Log &log)
{
	if (!XMVerifyCPUSupport())
	{
		log.TraceLine(ff::GetThisModule().GetString(IDS_ERR_DIRECTXMATH).c_str());
		return false;
	}

	return true;
}

ff::AppGlobals::AppGlobals(ProcessGlobals &processGlobals)
	: _processGlobals(processGlobals)
	, _advancingGame(0)
	, _renderingGame(0)
	, _lock(0)
	, _automation(false)
	, _firstRun(false)
	, _valid(false)
{
	_mru.SetListener(this);

	verify(CreateServiceCollection(_processGlobals.GetServices(), &_services));
}

ff::AppGlobals::~AppGlobals()
{
}

bool ff::AppGlobals::WinMain()
{
	ScanCommandLine();
	noAssertRetVal(CanStartup(), false);

	bool success = Startup();
	if (success)
	{
		Run();
	}

#if METRO_APP
	if (!success)
#endif
	{
		Shutdown();
	}

	return success;
}

#if METRO_APP

bool ff::AppGlobals::Show(StringRef args)
{
	return false;
}

bool ff::AppGlobals::Close()
{
	return false;
}

#else

bool ff::AppGlobals::Show(StringRef args)
{
	auto window = GetLastActiveWindow();
	if (window != nullptr)
	{
		window->Activate();
	}

	return true;
}

bool ff::AppGlobals::Close()
{
	for (size_t i = ff::PreviousSize(GetWindowCount()); i != ff::INVALID_SIZE; i = ff::PreviousSize(i))
	{
		GetWindow(i)->Close();
	}

	return true;
}

#endif

ff::String ff::AppGlobals::GetCompanyName() const
{
	return GetThisModule().GetString(IDS_APP_COMPANY);
}

ff::String ff::AppGlobals::GetAppName() const
{
	return GetProcessGlobals().GetModules().GetMain()->GetName();
}

REFGUID ff::AppGlobals::GetAppId() const
{
	return GetProcessGlobals().GetModules().GetMain()->GetId();
}

bool ff::AppGlobals::IsValid() const
{
	return _valid;
}

bool ff::AppGlobals::IsFirstRun() const
{
	return _firstRun;
}

bool ff::AppGlobals::IsAdvancing() const
{
	return _advancingGame != 0;
}

bool ff::AppGlobals::IsRendering() const
{
	return _renderingGame != 0;
}

bool ff::AppGlobals::IsQuitting() const
{
	return GotQuitMessage() || CanQuit();
}

bool ff::AppGlobals::IsAutomation() const
{
	return _automation;
}

ff::ProcessGlobals &ff::AppGlobals::GetProcessGlobals() const
{
	return _processGlobals;
}

ff::Dict &ff::AppGlobals::GetOptions()
{
	return GetNamedOptions(GetEmptyString());
}

ff::Dict &ff::AppGlobals::GetNamedOptions(StringRef name)
{
	if (name.empty() && !_appOptionsName.empty())
	{
		return GetNamedOptions(_appOptionsName);
	}

	BucketIter namedIter = _namedOptions.Get(name);
	if (namedIter == INVALID_ITER)
	{
		namedIter = _namedOptions.SetKey(name, Dict(_defaultOptions));

		// TODO: Look on disk
#if 0
		BucketIter cachedIter = _namedOptionsCache.Get(name);
		if (cachedIter != INVALID_ITER)
		{
			PackageNode node = _namedOptionsCache.ValueAt(cachedIter);
			_namedOptionsCache.DeletePos(cachedIter);
			cachedIter = INVALID_ITER;

			verify(LoadDict(node, *dict));
			DumpDict(name, *dict, &GetLog(), _appOptionsName == name, true);
		}
#endif
	}

	return _namedOptions.ValueAt(namedIter);
}

const ff::AppGlobalTime &ff::AppGlobals::GetGlobalTime() const
{
	return _globalTime;
}

const ff::AppFrameTime &ff::AppGlobals::GetFrameTime() const
{
	return _frameTime;
}

const ff::MostRecentlyUsed &ff::AppGlobals::GetMru() const
{
	return _mru;
}

ff::MostRecentlyUsed &ff::AppGlobals::GetMru()
{
	return _mru;
}

ff::Log &ff::AppGlobals::GetLog() const
{
	return _processGlobals.GetLog();
}

ff::IServiceProvider *ff::AppGlobals::GetServices() const
{
	return _services;
}

ff::IServiceCollection *ff::AppGlobals::GetServiceCollection() const
{
	return _services;
}

double ff::AppGlobals::GetTimeScale() const
{
	return 1.0;
}

size_t ff::AppGlobals::GetMaxAdvances() const
{
#ifdef _DEBUG
	if (GetTimeScale() <= 1.0 && IsDebuggerPresent())
	{
		return 1;
	}
#endif

	return 4;
}

const ff::Module &ff::AppGlobals::GetModule() const
{
	return GetThisModule();
}

class __declspec(uuid("f14a59b2-9ab9-466f-8f41-1bbd7b79b66a"))
	ResetStatusText : public ff::ComBase, public IUnknown
{
public:
	DECLARE_HEADER(ResetStatusText);

	void SetOldText(ff::AppGlobals *app, ff::String oldText);

private:
	ff::AppGlobals *_app;
	ff::String _oldText;
};

BEGIN_INTERFACES(ResetStatusText)
END_INTERFACES()

ResetStatusText::ResetStatusText()
	: _app(nullptr)
{
}

ResetStatusText::~ResetStatusText()
{
	if (_app)
	{
		_app->SetStatusText(_oldText);
	}
}

void ResetStatusText::SetOldText(ff::AppGlobals *app, ff::String oldText)
{
	_app = app;
	_oldText = oldText;
}

ff::String ff::AppGlobals::SetStatusText(StringRef text, IUnknown **resetObj)
{
	String oldStatus = _status;
	_status = text.size() ? text : GetThisModule().GetString(IDS_STATUS_READY);
	_splashScreen.SetStatusText(text);

	if (resetObj)
	{
		*resetObj = nullptr;

		if (_status != oldStatus)
		{
			ff::ComPtr<ResetStatusText> obj;
			if (SUCCEEDED(ff::ComAllocator<ResetStatusText>::CreateInstance(&obj)))
			{
				obj->SetOldText(this, oldStatus);
				*resetObj = obj.Detach();
			}
		}
	}

	return oldStatus;
}

ff::String ff::AppGlobals::GetStatusText() const
{
	return _status;
}

ff::String ff::AppGlobals::GetUserDataDirectory(StringRef subDir) const
{
	String path = GetLocalUserDirectory();

#if !METRO_APP
	AppendPathTail(path, GetCompanyName());
	AppendPathTail(path, GetAppName());
#endif

	if (subDir.size())
	{
		AppendPathTail(path, subDir);
	}

	assertRetVal(DirectoryExists(path) || CreateDirectory(path), ff::String());

	return path;
}

ff::String ff::AppGlobals::GetOptionsFile() const
{
	String path = GetUserDataDirectory();
	assertRetVal(!path.empty(), path);

	String name = GetThisModule().GetFormattedString(
		IDS_APP_OPTIONS_FILE,
		GetAppName().c_str(),
		GetThisModule().GetBuildArch());

	AppendPathTail(path, name);

	return path;
}

ff::String ff::AppGlobals::GetScoresFile() const
{
	String path = GetUserDataDirectory();
	assertRetVal(!path.empty(), path);

	String name = GetThisModule().GetFormattedString(
		IDS_APP_SCORES_FILE,
		GetAppName().c_str(),
		GetThisModule().GetBuildArch());

	AppendPathTail(path, name);

	return path;
}

ff::String ff::AppGlobals::GetLogFile() const
{
	return _logFile;
}

void ff::AppGlobals::SaveState()
{
	noAssertRet(IsValid());
	LogWithTime(GetLog(), IDS_APP_SAVING_STATE);
	SaveOptions();
	SaveScores();
}

void ff::AppGlobals::RestoreState()
{
	assertRet(IsValid());
	LogWithTime(GetLog(), IDS_APP_RESTORING_STATE);
}

size_t ff::AppGlobals::GetWindowCount() const
{
	return _mainWindowsVector.Size();
}

std::shared_ptr<ff::IMainWindow> ff::AppGlobals::GetWindow(size_t index) const
{
	assertRetVal(index < GetWindowCount(), std::shared_ptr<IMainWindow>());
	return _mainWindowsVector[index];
}

ff::Vector<std::shared_ptr<ff::IMainWindow>> ff::AppGlobals::GetOrderedWindows() const
{
	Vector<std::shared_ptr<IMainWindow>> windows;
	windows.Reserve(_mainWindowsOrder.Size());

	for (auto &window: _mainWindowsOrder)
	{
		windows.Push(window);
	}

	return windows;
}

std::shared_ptr<ff::IMainWindow> ff::AppGlobals::OpenNewWindow(StringRef typeName, IServiceProvider *contextServices)
{
	LogWithTime(GetLog(), IDS_APP_WINDOW_CREATING);

	std::shared_ptr<IMainWindow> window = NewWindow(typeName, contextServices);
	assertRetVal(window != nullptr, std::shared_ptr<IMainWindow>());

	_mainWindowsOrder.InsertFirst(window);
	_mainWindowsVector.Push(window);

	return window;
}

bool ff::AppGlobals::RequestFrameUpdate(IMainWindow *window, bool vsync)
{
	std::shared_ptr<IMainWindow> activeWindow = GetActiveWindow();
	if (activeWindow != nullptr && activeWindow.get() == window && activeWindow->IsVisible())
	{
		if (FrameAdvance(activeWindow.get()))
		{
			// did something change?
			if (_frameTime._advances)
			{
				FrameRender(activeWindow.get());
				FramePresent(activeWindow.get(), vsync);
			}
			else if (vsync)
			{
				FrameVsync(activeWindow.get());
			}
		}

		FrameCleanup(activeWindow.get());

		return true;
	}

	return false;
}

bool ff::AppGlobals::CanStartup()
{
	return true;
}

bool ff::AppGlobals::Startup()
{
	assertRetVal(!IsValid(), false);

	SetStatusText(GetThisModule().GetString(IDS_STATUS_LOADING));
	_splashScreen.Create(*this);

	InitializeTempDirectory();
	InternalInitializeDefaultOptions();
	InitializeLog();
	LogWithTime(GetLog(), IDS_APP_STARTUP);
	assertRetVal(VerifyCpuSupport(GetLog()), false);
	LoadOptions();
	LoadScores();
	noAssertRetVal(ParseCommandLine(), false);
	assertRetVal(InternalInitialize(), false);
	assertRetVal(Initialize(), false);
	assertRetVal(InitializeMainWindow(), false);
	LogWithTime(GetLog(), IDS_APP_STARTUP_DONE);
	SetStatusText(GetEmptyString());

	_splashScreen.Close();
	_valid = true;

	return true;
}

void ff::AppGlobals::Shutdown()
{
	LogWithTime(GetLog(), IDS_APP_SHUTDOWN);
	SaveState();
	Cleanup();
	InternalCleanup();
}

#if METRO_APP

void ff::AppGlobals::Run()
{
}

#else

void ff::AppGlobals::Run()
{
	LogWithTime(GetLog(), IDS_APP_RUN_MAIN_LOOP);

	EventTimer eventTimer; // waits for time to pass (when not using vsync)
	bool eventTimerSet = false;
	bool eventTimerFail = false;
	bool active = false;
	bool modal = false;

	while (!IsQuitting())
	{
		bool wasActive = active;
		bool wasModal = modal;

		std::shared_ptr<IMainWindow> activeWindow = GetActiveWindow();
		active = activeWindow != nullptr;
		modal = activeWindow != nullptr && activeWindow->IsModal();

		if (active && !modal)
		{
			// Update the event timer if the vsync option changed
			bool vsync = eventTimerFail || GetOptions().GetBool(OPTION_GRAPH_VSYNC_ON);
			if (!vsync && !eventTimerSet)
			{
				UINT frameMs = (UINT)(1.0 / _globalTime._framesPerSecond * 1000.0);
				UINT resolutionMs = (frameMs < 32) ? 4 : 8;
				vsync = !eventTimer.Start(frameMs, resolutionMs, false);
				eventTimerSet = !vsync;
				eventTimerFail = !eventTimerSet;
			}

			// Never use the event timer again if it failed to start
			if (vsync && eventTimerSet)
			{
				eventTimer.Stop();
				eventTimerSet = false;
			}

			// Pump messages
			if (vsync ? ff::HandleMessages(this) : eventTimer.Wait(this))
			{
				FrameMessagesHandled(activeWindow.get());

				// Reset timers when focus is set
				if (!wasActive || wasModal)
				{
					eventTimer.Resume();
					FrameResetTimer(activeWindow.get());
				}

				if (FrameAdvance(activeWindow.get()))
				{
					// did something change?
					if (_frameTime._advances)
					{
						FrameRender(activeWindow.get());
						FramePresent(activeWindow.get(), vsync);
					}
					else if (vsync)
					{
						FrameVsync(activeWindow.get());
					}
				}

				FrameCleanup(activeWindow.get());
			}
		}
		else // The game isn't active, so just pump messages
		{
			eventTimer.Pause();
			ff::WaitForMessage(nullptr, 1000);
		}
	}
}

#endif // METRO_APP

void ff::AppGlobals::FrameMessagesHandled(IMainWindow *window)
{
}

void ff::AppGlobals::FrameResetTimer(IMainWindow *window)
{
	_frameTimer.Tick();
	_frameTimer.StoreLastTickTime();
}

bool ff::AppGlobals::FrameAdvance(IMainWindow *window)
{
	_advancingGame++;

	_frameTimer.SetTimeScale(GetTimeScale());
	_globalTime._absoluteSeconds = _frameTimer.GetSeconds();
	_globalTime._bankSeconds += _frameTimer.Tick();

	_frameTime._advances = 0;
	_frameTime._flipTime = _frameTimer.GetLastTickStoredRawTime();

	const double idealAdvanceTime = _globalTime._deltaSeconds;
	const size_t maxAdvances = GetMaxAdvances();
	bool result = true;

	while (result && _globalTime._bankSeconds >= idealAdvanceTime)
	{
		if (_frameTime._advances >= maxAdvances)
		{
			// The game is running way too slow
			_globalTime._bankSeconds = std::fmod(_globalTime._bankSeconds, idealAdvanceTime);
			_globalTime._bankScale = _globalTime._bankSeconds / idealAdvanceTime;
			_globalTime._bankScaleF = static_cast<float>(_globalTime._bankScale);
			break;
		}

		_globalTime._absoluteSeconds += idealAdvanceTime;
		_globalTime._bankSeconds -= idealAdvanceTime;
		_globalTime._bankScale = _globalTime._bankSeconds / idealAdvanceTime;
		_globalTime._bankScaleF = static_cast<float>(_globalTime._bankScale);
		_globalTime._advances++;
		_frameTime._advances++;
		_frameTime._vsyncs = 1;

		if (_frameTime._advances > 1)
		{
			result = ff::HandleMessages(this);
		}

		if (result)
		{
			result = window->FrameAdvance();
		}

		_frameTime._advanceTime[_frameTime._advances - 1] = _frameTimer.GetCurrentStoredRawTime();
	}

	_advancingGame--;

	return result && !IsQuitting();
}

void ff::AppGlobals::FrameRender(IMainWindow *window)
{
	_renderingGame++;

	window->FrameRender();

	_frameTime._renderTime = _frameTimer.GetCurrentStoredRawTime();
	_frameTime._vsyncs++;

	_renderingGame--;
}

void ff::AppGlobals::FramePresent(IMainWindow *window, bool vsync)
{
	// Don't wait for vsync if the game is running slow (nAdvances > 1)
	window->FramePresent(vsync && _frameTime._advances <= 1);
}

void ff::AppGlobals::FrameVsync(IMainWindow *window)
{
	if (window->FrameVsync())
	{
		_frameTime._vsyncs++;
	}
}

void ff::AppGlobals::FrameCleanup(IMainWindow *window)
{
	window->FrameCleanup();

	_destroyedWindows.Clear(); // don't use window after this
}

void ff::AppGlobals::ScanCommandLine()
{
	// Check if the app is supposed to start up in automation mode
	Vector<String> tokens = ff::TokenizeCommandLine();
	for (const String &token : tokens)
	{
		if (token == L"/automation" || token == L"/Automation")
		{
			_automation = true;
		}
	}
}

bool ff::AppGlobals::ParseCommandLine()
{
	GetLog().TraceLine(GetThisModule().GetFormattedString(
		IDS_APP_PARSE_COMMAND_LINE, ff::GetCommandLine().c_str()).c_str());

	List<String> tokenList = List<String>::FromVector(TokenizeCommandLine());
	if (!tokenList.IsEmpty())
	{
		// ignore the executable path which is always first on the command line
		tokenList.Delete(*tokenList.GetFirst());
	}

	bool showHelp = false;

	while (!tokenList.IsEmpty())
	{
		bool valid = true;
		String token = CanonicalizeString(*tokenList.GetFirst());
		size_t oldSize = tokenList.Size();

		if (token == L"/?")
		{
			showHelp = true;

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_HELP).c_str());
		}
		else if (token == L"/sound:on")
		{
			GetOptions().SetBool(OPTION_SOUND_MASTER_ON, true);

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_SOUND_ON).c_str());
		}
		else if (token == L"/sound:off")
		{
			GetOptions().SetBool(OPTION_SOUND_MASTER_ON, false);

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_SOUND_OFF).c_str());
		}
		else if (token == L"/vsync:on")
		{
			GetOptions().SetBool(OPTION_GRAPH_VSYNC_ON, true);

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_VSYNC_ON).c_str());
		}
		else if (token == L"/vsync:off")
		{
			GetOptions().SetBool(OPTION_GRAPH_VSYNC_ON, false);

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_VSYNC_OFF).c_str());
		}
		else if (token == L"/fps:on")
		{
			GetOptions().SetBool(OPTION_GRAPH_SHOW_FPS_ON, true);

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_FPS_ON).c_str());
		}
		else if (token == L"/fps:off")
		{
			GetOptions().SetBool(OPTION_GRAPH_SHOW_FPS_ON, false);

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_FPS_OFF).c_str());
		}
		else if (token.length() > 9 && !wcsncmp(token.c_str(), L"/padding:", 9) && isdigit(token[9]))
		{
			int overscan = _ttoi(token.c_str() + 9);

			if (overscan >= 0 && overscan < 128)
			{
				GetOptions().SetRect(OPTION_WINDOW_PADDING,
					RectInt(overscan, overscan, overscan, overscan));

				GetLog().TraceLine(GetThisModule().GetFormattedString(IDS_APP_CL_PADDING, overscan).c_str());
			}
			else
			{
				valid = false;
			}
		}
		else if (token.length() > 9 && !wcsncmp(token.c_str(), L"/buffers:", 9) && isdigit(token[9]))
		{
			int buffers = _ttoi(token.c_str() + 9);

			if (buffers >= 1 && buffers <= 4)
			{
				GetOptions().SetInt(OPTION_GRAPH_BACK_BUFFERS, buffers);

				GetLog().TraceLine(GetThisModule().GetFormattedString(IDS_APP_CL_BUFFERS, buffers).c_str());
			}
			else
			{
				valid = false;
			}
		}
		else if (token == L"/full" || token == L"/fullscreen")
		{
			GetOptions().SetBool(OPTION_WINDOW_FULL_SCREEN, true);

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_FS_ON).c_str());
		}
		else if (token == L"/window")
		{
			GetOptions().SetBool(OPTION_WINDOW_FULL_SCREEN, false);

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_FS_OFF).c_str());
		}
		else if (token == L"/clearscores")
		{
			ClearScores();

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_CLEAR_SCORES).c_str());
		}
		else if (token == L"/default")
		{
			ClearOptions();

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_DEFAULT_OPTIONS).c_str());
		}
		else if (token == L"/firstrun")
		{
			_firstRun = true;

			ClearOptions();

			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_FIRST_RUN).c_str());
		}
		else if (token == L"/automation" || token == L"/Automation")
		{
			// ScanCommandLine should've noticed this
			assert(_automation);
			GetLog().TraceLine(GetThisModule().GetString(IDS_APP_CL_AUTOMATION).c_str());
		}
		else if (token.find(L"-servername:") == 0)
		{
			GetLog().TraceLine(GetThisModule().GetFormattedString(
				IDS_APP_CL_WIN8_SERVER, token.c_str() + wcslen(L"-servername:")).c_str());
		}
		else if (!HandleCommandLineArgument(tokenList))
		{
			valid = false;
		}

		if (!valid)
		{
			GetLog().TraceLine(GetThisModule().GetFormattedString(
				IDS_APP_CL_BAD_PARAM, tokenList.GetFirst()->c_str()).c_str());
		}

		if (oldSize == tokenList.Size())
		{
			tokenList.Delete(*tokenList.GetFirst());
		}
	}

	return !showHelp || ShowHelp();
}

bool ff::AppGlobals::HandleCommandLineArgument(List<String> &args)
{
	return false;
}

bool ff::AppGlobals::ShowHelp()
{
	return true;
}

void ff::AppGlobals::InitializeDefaultOptions(Dict &dict)
{
}

void ff::AppGlobals::InitializeLog()
{
	for (size_t i = 0; i < 16; i++)
	{
		String fullPath = GetUserDataDirectory();
		String name = i ? String::format_new(L"log%lu.txt", i) : String(L"log.txt");
		AppendPathTail(fullPath, name);

		ComPtr<IDataFile> file;
		ComPtr<IDataWriter> writer;
		if ((!ff::FileExists(fullPath) || ff::DeleteFile(fullPath)) &&
			ff::CreateDataFile(fullPath, false, &file) &&
			ff::CreateDataWriter(file, 0, &writer))
		{
			_logFile = fullPath;
			_logWriter = writer;

			WriteUnicodeBOM(writer);
			GetLog().AddWriter(writer);

			break;
		}
	}
}

bool ff::AppGlobals::InitializeMainWindow()
{
	if (!IsAutomation())
	{
		assertRetVal(OpenNewWindow(GetEmptyString(), nullptr) != nullptr, false);
	}

	return true;
}

bool ff::AppGlobals::Initialize()
{
	return true;
}

void ff::AppGlobals::LockApp()
{
	::InterlockedIncrement(&_lock);
}

void ff::AppGlobals::UnlockApp()
{
	if (::InterlockedDecrement(&_lock) < 0)
	{
		assert(false);
		::InterlockedIncrement(&_lock);
	}
}

bool ff::AppGlobals::CanQuit() const
{
	return _lock == 0 && GetWindowCount() == 0;
}

void ff::AppGlobals::Cleanup()
{
}

UINT ff::AppGlobals::GetSplashScreenBitmapResource() const
{
	return 0;
}

std::shared_ptr<ff::IMainWindow> ff::AppGlobals::NewWindow(StringRef typeName, IServiceProvider *contextServices)
{
	std::shared_ptr<MainWindow> window = std::make_shared<MainWindow>(*this, this, contextServices);
	assertRetVal(window != nullptr && window->Create(), std::shared_ptr<IMainWindow>());

	return window;
}

std::shared_ptr<ff::IMainWindow> ff::AppGlobals::GetActiveWindow()
{
	for (const auto &mainWindow: _mainWindowsOrder)
	{
		if (mainWindow->IsValid() && mainWindow->IsActive())
		{
			return mainWindow;
		}
	}

	return std::shared_ptr<IMainWindow>();
}

std::shared_ptr<ff::IMainWindow> ff::AppGlobals::GetLastActiveWindow()
{
	for (const auto &mainWindow: _mainWindowsOrder)
	{
		if (mainWindow->IsValid())
		{
			return mainWindow;
		}
	}

	return std::shared_ptr<IMainWindow>();
}

void ff::AppGlobals::InitializeTempDirectory()
{
	DWORD id = ::GetCurrentProcessId();
	String name = GetAppName();

	Vector<String> tempDirs;
	Vector<String> tempFiles;

	ff::SetTempSubDirectory(name);
	if (ff::GetDirectoryContents(ff::GetTempDirectory(), tempDirs, tempFiles))
	{
		// Look for temp directories that aren't owned by any running process, and delete them

		for (String &tempDir: tempDirs)
		{
			// Parse the process ID from the temp directory name

			int parsedChars = 0;
			DWORD tempId = 0;
			if (_snwscanf_s(tempDir.c_str(), tempDir.length(), L"%u%n", &tempId, &parsedChars) == 1 &&
				parsedChars == static_cast<int>(tempDir.length()))
			{
				// Check if the process is still running
#if !METRO_APP
				DWORD exitCode = 0;
				WinHandle handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, tempId);
				if (handle == nullptr ||
					!::GetExitCodeProcess(handle, &exitCode) ||
					exitCode != STILL_ACTIVE)
#endif
				{
					String unusedTempPath = ff::GetTempDirectory();
					ff::AppendPathTail(unusedTempPath, tempDir);
					ff::DeleteDirectory(unusedTempPath, false);
				}
			}
		}
	}

	String subDir = String::format_new(L"%s\\%u", name.c_str(), id);
	ff::SetTempSubDirectory(subDir);
}

void ff::AppGlobals::InternalInitializeDefaultOptions()
{
	_defaultOptions.SetBool(OPTION_APP_USE_DIRECT3D, true);
	_defaultOptions.SetBool(OPTION_APP_USE_JOYSTICKS, true);
	_defaultOptions.SetBool(OPTION_APP_USE_MAIN_WINDOW_KEYBOARD, true);
	_defaultOptions.SetBool(OPTION_APP_USE_MAIN_WINDOW_MOUSE, true);
	_defaultOptions.SetBool(OPTION_APP_USE_MAIN_WINDOW_TOUCH, true);
	_defaultOptions.SetBool(OPTION_APP_USE_RENDER_2D, true);
	_defaultOptions.SetBool(OPTION_APP_USE_RENDER_DEPTH, true);
	_defaultOptions.SetBool(OPTION_APP_USE_RENDER_MAIN_WINDOW, true);
	_defaultOptions.SetBool(OPTION_APP_USE_XAUDIO, true);
	_defaultOptions.SetBool(OPTION_APP_USE_XINPUT, true);

	_defaultOptions.SetDouble(OPTION_GAME_ADVANCES_PER_SECOND, 60.0);
	_defaultOptions.SetDouble(OPTION_GRAPH_FRAMES_PER_SECOND, 60.0);
	_defaultOptions.SetInt(OPTION_GRAPH_BACK_BUFFERS, 1);
	_defaultOptions.SetInt(OPTION_GRAPH_MULTI_SAMPLES, 1);
	_defaultOptions.SetBool(OPTION_GRAPH_VSYNC_ON, true);

	_defaultOptions.SetBool(OPTION_SOUND_MASTER_ON, true);
	_defaultOptions.SetDouble(OPTION_SOUND_MASTER_VOLUME, 1);
	_defaultOptions.SetDouble(OPTION_SOUND_EFFECTS_VOLUME, 1);
	_defaultOptions.SetDouble(OPTION_SOUND_MUSIC_VOLUME, 1);
	_defaultOptions.SetBool(OPTION_GRAPH_SHOW_FPS_ON, false);

	_defaultOptions.SetPoint(OPTION_WINDOW_DEFAULT_CLIENT_SIZE, PointInt(0, 0));
	_defaultOptions.SetBool(OPTION_WINDOW_FULL_SCREEN, false);
	_defaultOptions.SetBool(OPTION_WINDOW_MAXIMIZED, false);
	_defaultOptions.SetRect(OPTION_WINDOW_PADDING, RectInt(0, 0, 0, 0));
	_defaultOptions.SetRect(OPTION_WINDOW_POSITION, RectInt(0, 0, 0, 0));

	InitializeDefaultOptions(_defaultOptions);
}

bool ff::AppGlobals::InternalInitialize()
{
	_globalTime._framesPerSecond = GetOptions().GetDouble(OPTION_GRAPH_FRAMES_PER_SECOND);
	_globalTime._framesPerSecond = std::max<double>(_globalTime._framesPerSecond,  1);
	_globalTime._framesPerSecond = std::min<double>(_globalTime._framesPerSecond,  120);

	_globalTime._advancesPerSecond = GetOptions().GetDouble(OPTION_GAME_ADVANCES_PER_SECOND);
	_globalTime._advancesPerSecond = std::max<double>(_globalTime._advancesPerSecond, 1);
	_globalTime._advancesPerSecond = std::min<double>(_globalTime._advancesPerSecond, 120);

	_globalTime._deltaSeconds = 1.0 / _globalTime._advancesPerSecond;;

	_frameTime._freq = _frameTimer.GetRawFreq();
	_frameTime._freqD = _frameTimer.GetRawFreqD();

	_frameTimer.Reset();

	return true;
}

void ff::AppGlobals::InternalCleanup()
{
	LogWithTime(GetLog(), IDS_APP_CLEANUP);

	if (_services != nullptr)
	{
		_services->RemoveAllServices();
	}

	if (_logWriter != nullptr)
	{
		GetLog().RemoveWriter(_logWriter);
		_logWriter = nullptr;
	}
}

void ff::AppGlobals::LoadOptions()
{
	_appOptionsName = GetAppName();

	String fileName = GetOptionsFile();
	assertRet(!fileName.empty());

	if (FileExists(fileName))
	{
		GetLog().TraceLine(GetThisModule().GetFormattedString(IDS_APP_LOADING_OPTIONS, fileName.c_str()).c_str());

		LoadMru();
		CleanLoadedOptions();
	}
}

void ff::AppGlobals::SaveOptions()
{
	String fileName = GetOptionsFile();
	assertRet(!fileName.empty());

	GetLog().TraceLine(GetThisModule().GetFormattedString(IDS_APP_SAVING_OPTIONS, fileName.c_str()).c_str());

	CleanSavingOptions();
	SaveMru();
	// TODO:
	// SaveNamedOptions(fileName.c_str());
}

void ff::AppGlobals::ClearOptions()
{
	_namedOptions.Clear();
	GetOptions().Clear();
}

void ff::AppGlobals::CleanLoadedOptions()
{
	GetOptions().SetValue(OPTION_GRAPH_SHOW_FPS_ON, nullptr);
}

void ff::AppGlobals::CleanSavingOptions()
{
}

void ff::AppGlobals::LoadMru()
{
	ValuePtr paths = GetOptions().GetValue(OPTION_MRU_PATHS, true);
	ValuePtr names = GetOptions().GetValue(OPTION_MRU_NAMES, true);
	ValuePtr pinned = GetOptions().GetValue(OPTION_MRU_PINNED, true);

	if (paths && paths->IsType(Value::Type::StringVector) &&
		names && names->IsType(Value::Type::StringVector) &&
		pinned && pinned->IsType(Value::Type::IntVector) &&
		paths->AsStringVector().Size() == pinned->AsIntVector().Size() &&
		paths->AsStringVector().Size() == names->AsStringVector().Size())
	{
		for (size_t i = PreviousSize(paths->AsStringVector().Size()); i != INVALID_SIZE; i = PreviousSize(i))
		{
			GetMru().Add(
				paths->AsStringVector()[i],
				names->AsStringVector()[i],
				pinned->AsIntVector()[i] ? true : false);
		}
	}
}

void ff::AppGlobals::SaveMru()
{
	ValuePtr paths, names, pinned;
	Value::CreateStringVector(&paths);
	Value::CreateStringVector(&names);
	Value::CreateIntVector(&pinned);

	for (size_t i = 0; i < GetMru().GetCount(); i++)
	{
		const RecentlyUsedItem &item = GetMru().GetItem(i);
		paths->AsStringVector().Push(item._path);
		names->AsStringVector().Push(item._name);
		pinned->AsIntVector().Push(item._pinned);
	}

	GetOptions().SetValue(OPTION_MRU_PATHS, paths);
	GetOptions().SetValue(OPTION_MRU_NAMES, names);
	GetOptions().SetValue(OPTION_MRU_PINNED, pinned);
}

void ff::AppGlobals::LoadScores()
{
}

void ff::AppGlobals::SaveScores()
{
}

void ff::AppGlobals::ClearScores()
{
}

void ff::AppGlobals::OnWindowCreated(IMainWindow &window)
{
}

void ff::AppGlobals::OnWindowInitialized(IMainWindow &window)
{
	LogWithTime(GetLog(), IDS_APP_WINDOW_CREATED);
}

void ff::AppGlobals::OnWindowActivated(IMainWindow &window)
{
	FrameResetTimer(&window);

	for (const auto &mainWindow: _mainWindowsOrder)
	{
		if (mainWindow.get() == &window)
		{
			_mainWindowsOrder.MoveToFront(mainWindow);
			break;
		}
	}
}

void ff::AppGlobals::OnWindowDeactivated(IMainWindow &window)
{
}

bool ff::AppGlobals::OnWindowClosing(IMainWindow &window)
{
	LogWithTime(GetLog(), IDS_APP_WINDOW_CLOSE);

	return true;
}

void ff::AppGlobals::OnWindowDestroyed(IMainWindow &window)
{
	LogWithTime(GetLog(), IDS_APP_WINDOW_DESTROY);

	for (const auto &mainWindow: _mainWindowsOrder)
	{
		if (mainWindow.get() == &window)
		{
			_destroyedWindows.Push(mainWindow);
			_mainWindowsOrder.Delete(mainWindow);
			_mainWindowsVector.DeleteItem(mainWindow);
			break;
		}
	}
}

void ff::AppGlobals::OnWindowModalStart(IMainWindow &window)
{
}

void ff::AppGlobals::OnWindowModalEnd(IMainWindow &window)
{
}

void ff::AppGlobals::OnWindowSuspending(IMainWindow &window)
{
	LogWithTime(GetLog(), IDS_APP_SUSPEND);
}

void ff::AppGlobals::OnWindowResumed(IMainWindow &window)
{
	LogWithTime(GetLog(), IDS_APP_RESUME);
}

void ff::AppGlobals::OnMruChanged(const MostRecentlyUsed &mru)
{
	if (IsValid())
	{
		for (const auto &window: _mainWindowsOrder)
		{
			if (window->IsValid())
			{
				auto listener = std::dynamic_pointer_cast<IMostRecentlyUsedListener>(window);
				if (listener != nullptr)
				{
					listener->OnMruChanged(mru);
				}
			}
		}
	}
}

#if !METRO_APP

bool ff::AppGlobals::FilterMessage(MSG &msg)
{
	std::shared_ptr<IMainWindow> targetWindow;

	if (msg.hwnd != nullptr)
	{
		for (const auto &window: _mainWindowsOrder)
		{
			HWND hwnd = window->IsValid() ? window->GetHandle() : nullptr;
			if (hwnd && (hwnd == msg.hwnd || ::IsChild(hwnd, msg.hwnd)))
			{
				targetWindow = window;
				break;
			}
		}
	}

	if (targetWindow == nullptr)
	{
		targetWindow = GetActiveWindow();
	}

	if (targetWindow != nullptr && targetWindow->FilterMessage(msg))
	{
		return true;
	}

	return false;
}

#endif
