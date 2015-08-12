#include "pch.h"
#include "App/Idle.h"
#include "Audio/AudioFactory.h"
#include "COM/ServiceCollection.h"
#include "Data/AsyncDataLoader.h"
#include "Globals/ProcessGlobals.h"
#include "Globals/ProcessStartup.h"
#include "Graph/GraphFactory.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"

static ff::ProcessGlobals *s_processGlobals = nullptr;
static bool s_programShutDown = false;

bool ff::DidProgramStart()
{
	return s_processGlobals != nullptr && s_processGlobals->IsValid();
}

bool ff::IsProgramRunning()
{
	return s_processGlobals != nullptr && s_processGlobals->IsValid() && !s_processGlobals->IsShuttingDown();
}

bool ff::IsProgramShuttingDown()
{
	return s_programShutDown || (s_processGlobals != nullptr && s_processGlobals->IsShuttingDown());
}

void ff::AtProgramShutdown(std::function<void()> func)
{
	assertRet(s_processGlobals != nullptr);

	if (s_processGlobals->IsShuttingDown())
	{
		assertSz(false, L"Why register a program shutdown function during shutdown?");
		func();
	}
	else
	{
		s_processGlobals->AtShutdown(func);
	}
}

ff::ProcessGlobals::ProcessGlobals()
{
	assert(s_processGlobals == nullptr);
	s_processGlobals = this;
	s_programShutDown = false;
}

ff::ProcessGlobals::~ProcessGlobals()
{
	assert(s_processGlobals == this && !s_programShutDown);
	s_processGlobals = nullptr;
	s_programShutDown = true;
}

ff::ProcessGlobals *ff::ProcessGlobals::Get()
{
	assert(s_processGlobals);
	return s_processGlobals;
}

bool ff::ProcessGlobals::Startup()
{
	HookCrtMemAlloc();

	// Init random numbers
	SYSTEMTIME time;
	GetSystemTime(&time);
	std::srand((DWORD)(HashFunc(time) & 0xFFFFFFFF));

	assertRetVal(ff::ThreadGlobals::Startup(), false);

	StartupMainThread();

	assertRetVal(CreateAsyncDataLoader(&_asyncDataLoader), false);
	assertRetVal(CreateAudioFactory(&_audioFactory), false);
	assertRetVal(CreateGraphicFactory(&_graphicFactory), false);
	assertRetVal(CreateIdleMaster(&_idleMaster), false);
	assertRetVal(CreateServiceCollection(&_services), false);
	assertRetVal(CreateThreadPool(&_threadPool), false);

	ProcessStartup::OnStartup(*this);

	return true;
}

void ff::ProcessGlobals::Shutdown()
{
	FlushAllThreadPoolWork();
	ShutdownMainThread();

	ff::ThreadGlobals::Shutdown();

	_asyncDataLoader = nullptr;
	_audioFactory = nullptr;
	_graphicFactory = nullptr;
	_idleMaster = nullptr;
	_services = nullptr;
	_threadPool = nullptr;

	_modules.Clear();
	_stringCache.Clear();

	ProgramShutdown::OnShutdown();

	ComBaseEx::DumpComObjects();
	UnhookCrtMemAlloc();
}

#if !METRO_APP
void ff::ProcessGlobals::KillProcess(unsigned int code)
{
	_state = State::SHUT_DOWN;
	::ExitProcess(code);
}
#endif

ff::Log &ff::ProcessGlobals::GetLog()
{
	return _log;
}

ff::Modules &ff::ProcessGlobals::GetModules()
{
	return _modules;
}

ff::StringManager &ff::ProcessGlobals::GetStringManager()
{
	return _stringManager;
}

ff::StringCache *ff::ProcessGlobals::GetStringCache()
{
	return &_stringCache;
}

ff::IAsyncDataLoader *ff::ProcessGlobals::GetAsyncDataLoader()
{
	return _asyncDataLoader;
}

ff::IAudioFactory *ff::ProcessGlobals::GetAudioFactory()
{
	return _audioFactory;
}

ff::IGraphicFactory *ff::ProcessGlobals::GetGraphicFactory()
{
	return _graphicFactory;
}

ff::IIdleMaster *ff::ProcessGlobals::GetIdleMaster()
{
	return _idleMaster;
}

ff::IServiceCollection *ff::ProcessGlobals::GetServices()
{
	return _services;
}

ff::IThreadPool *ff::ProcessGlobals::GetThreadPool()
{
	return _threadPool;
}
