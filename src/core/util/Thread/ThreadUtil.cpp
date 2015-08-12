#include "pch.h"
#include "Module/Module.h"
#include "Thread/ThreadUtil.h"
#include "Windows/Handles.h"
#include "Windows/WinUtil.h"

#if METRO_APP

// STATIC_DATA (object)
static Platform::Agile<Windows::UI::Core::CoreWindow> s_mainThreadWindow;
static Platform::Agile<Windows::UI::Xaml::Window> s_mainThreadWindowXaml;
static Windows::UI::Core::CoreDispatcher ^s_mainThreadDispatcher;
static Windows::UI::Core::DispatchedHandler ^s_flushMainThreadFuncsHandler;

Windows::UI::Core::CoreWindow ^ff::GetMainThreadWindow()
{
	return s_mainThreadWindow.Get();
}

Windows::UI::Xaml::Window ^ff::GetMainThreadWindowXaml()
{
	return s_mainThreadWindowXaml.Get();
}

Windows::UI::Core::CoreDispatcher ^ff::GetMainThreadDispatcher()
{
	return s_mainThreadDispatcher;
}

static bool HasMainThreadWindow()
{
	return s_mainThreadWindow != nullptr;
}

#else

class CMainThreadMessageWindow : public ff::CustomWindow
{
protected:
	virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
};

LRESULT CMainThreadMessageWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_USER:
	case WM_DESTROY:
		ff::FlushMainThreadFunctions();
		break;
	}

	return DoDefault(hwnd, msg, wParam, lParam);
}

// STATIC_DATA (object)
static CMainThreadMessageWindow s_mainThreadWindow;

static bool HasMainThreadWindow()
{
	return s_mainThreadWindow.Handle() != nullptr;
}

#endif // METRO_APP

struct FuncEntry
{
	std::function<void ()> _func;
	ff::SmartPtr<ff::Module> _module;
	DWORD _threadId;
};

// STATIC_DATA (object,pod)
static ff::PoolAllocator<FuncEntry> s_funcEntryPool;
static ff::Vector<FuncEntry *> s_mainThreadFuncs;
static ff::WinHandle s_mainThreadFuncPending;
static ff::WinHandle s_neverSetEvent;
static DWORD s_mainThreadId = 0;
static bool s_didInitMainThread = false;

void ff::StartupMainThread()
{
	assert(s_mainThreadId == 0);

	s_mainThreadId = ::GetCurrentThreadId();
	s_mainThreadFuncPending = ::CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
	s_neverSetEvent = ::CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);

#if METRO_APP
	s_mainThreadWindow = Windows::UI::Core::CoreWindow::GetForCurrentThread();
	s_mainThreadWindowXaml = Windows::UI::Xaml::Window::Current;

	s_mainThreadDispatcher = (s_mainThreadWindowXaml != nullptr)
		? s_mainThreadWindowXaml->Dispatcher
		: s_mainThreadWindow->Dispatcher;

	s_flushMainThreadFuncsHandler = ref new Windows::UI::Core::DispatchedHandler([]
	{
		FlushMainThreadFunctions();
	});
#else
	verify(s_mainThreadWindow.CreateMessageWindow());
#endif

	s_didInitMainThread = true;

	assert(HasMainThreadWindow());
}

void ff::ShutdownMainThread()
{
	assertRet(IsRunningOnMainThread());

	if (HasMainThreadWindow())
	{
#if METRO_APP
		s_mainThreadWindow = nullptr;
		s_mainThreadWindowXaml = nullptr;
		s_mainThreadDispatcher = nullptr;
		s_flushMainThreadFuncsHandler = nullptr;
#else
		DestroyWindow(s_mainThreadWindow.Handle());
#endif
	}

	s_mainThreadId = 0;
	s_mainThreadFuncPending.Close();
	s_neverSetEvent.Close();
	s_didInitMainThread = false;
}

DWORD ff::GetMainThreadID()
{
	return s_mainThreadId;
}

bool ff::IsRunningOnMainThread()
{
	return s_didInitMainThread && GetMainThreadID() == GetCurrentThreadId();
}

bool ff::IsEventSet(HANDLE hEvent)
{
	assertRetVal(hEvent, true);

	return WaitForSingleObjectEx(hEvent, 0, FALSE) == WAIT_OBJECT_0;
}

static bool WaitForHandleMainThread(HANDLE handle, DWORD timeout = INFINITE)
{
	const HANDLE handles[] = { handle, s_mainThreadFuncPending };

	while (true)
	{
		switch (WaitForMultipleObjectsEx(_countof(handles), handles, FALSE, timeout, TRUE))
		{
		case WAIT_OBJECT_0:
			return true;

		case WAIT_OBJECT_0 + 1:
			ff::FlushMainThreadFunctions();
			break;

		case WAIT_IO_COMPLETION:
			break;

		case WAIT_ABANDONED_0:
		case WAIT_ABANDONED_0 + 1:
		case WAIT_TIMEOUT:
		case WAIT_FAILED:
			return false;
		}
	}
}

static bool WaitForHandleWorkerThread(HANDLE handle, DWORD timeout = INFINITE)
{
	while (true)
	{
		switch (WaitForSingleObjectEx(handle, timeout, TRUE))
		{
		case WAIT_OBJECT_0:
			return true;

		case WAIT_ABANDONED:
		case WAIT_TIMEOUT:
		case WAIT_FAILED:
			return false;

		default:
		case WAIT_IO_COMPLETION:
			break;
		}
	}
}

bool ff::WaitForHandle(HANDLE handle)
{
	noAssertRetVal(handle, false);

	if (ff::IsRunningOnMainThread())
	{
		return WaitForHandleMainThread(handle);
	}
	else
	{
		return WaitForHandleWorkerThread(handle);
	}
}

void ff::Sleep(size_t ms)
{
	if (s_neverSetEvent)
	{
		if (IsRunningOnMainThread())
		{
			WaitForHandleMainThread(s_neverSetEvent, (DWORD)ms);
		}
		else
		{
			WaitForHandleWorkerThread(s_neverSetEvent, (DWORD)ms);
		}
	}
}

void ff::FlushMainThreadFunctions()
{
	assertRet(IsRunningOnMainThread());

#if !METRO_APP
	DWORD nResult = MsgWaitForMultipleObjectsEx(0, nullptr, 0, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
	assert(nResult != WAIT_FAILED);
#endif

	Vector<FuncEntry *> entries;
	{
		LockMutex crit(GCS_THREAD_UTIL);

		if (s_mainThreadFuncs.Size())
		{
			entries = s_mainThreadFuncs;
			s_mainThreadFuncs.Clear();
			ResetEvent(s_mainThreadFuncPending);
		}
	}

	for (size_t i = 0; i < entries.Size(); i++)
	{
		FuncEntry *entry = entries[i];
		entry->_func();
		entry->_module = nullptr;
		s_funcEntryPool.Delete(entry);
	}
}

void ff::PostMainThreadFunction(std::function<void ()> func, Module *module)
{
	assertRet(HasMainThreadWindow());

	FuncEntry *entry = s_funcEntryPool.New();
	entry->_func = func;
	entry->_threadId = GetMainThreadID();
	entry->_module = module;

	LockMutex crit(GCS_THREAD_UTIL);
	s_mainThreadFuncs.Push(entry);
	
	if (s_mainThreadFuncs.Size() == 1)
	{
		SetEvent(s_mainThreadFuncPending);

#if METRO_APP
		s_mainThreadDispatcher->RunAsync(
			Windows::UI::Core::CoreDispatcherPriority::High,
			s_flushMainThreadFuncsHandler);
#else
		PostMessage(s_mainThreadWindow.Handle(), WM_USER, 0, 0);
#endif
	}
}

void ff::SetDebuggerThreadName(StringRef name, DWORD nThreadID)
{
#ifdef _DEBUG
	if (!IsDebuggerPresent())
	{
		return;
	}

	CHAR szNameACP[512] = "";
	WideCharToMultiByte(CP_ACP, 0, name.c_str(), -1, szNameACP, _countof(szNameACP), nullptr, nullptr);

	typedef struct tagTHREADNAME_INFO
	{
		ULONG_PTR dwType;     // must be 0x1000
		const char *szName;     // pointer to name (in user addr space)
		ULONG_PTR dwThreadID; // thread ID (-1=caller thread)
		ULONG_PTR dwFlags;    // reserved for future use, must be zero
	} THREADNAME_INFO;

	THREADNAME_INFO info;
	info.dwType     = 0x1000;
	info.szName     = szNameACP;
	info.dwThreadID = nThreadID ? nThreadID : GetCurrentThreadId();
	info.dwFlags    = 0;

	__try
	{
		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{
	}

#endif // _DEBUG
}
