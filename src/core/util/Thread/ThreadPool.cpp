#include "pch.h"
#include "COM/ComObject.h"
#include "Globals/GlobalsScope.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Windows/Handles.h"
#include "Windows/WinUtil.h"

#if METRO_APP
bool s_unusedFile_ThreadPool = false;
#else

#include <process.h>

namespace ff
{
	// From ThreadPoolShared.cpp:
	Vector<IThreadPool *, 8> &GetAllThreadPools();

	class __declspec(uuid("58c5d38c-72f2-4b50-a92b-c92f14682f9c"))
		CThreadPool : public ComBase, public IThreadPool
	{
	public:
		DECLARE_HEADER(CThreadPool);

		void Init(int nMaxThreads);

		// IThreadPool functions

		virtual void Add(IWorkItem *pWork) override;
		virtual bool Wait(IWorkItem *pWork) override;
		virtual bool Cancel(IWorkItem *pWork) override;
		virtual void Flush() override;

		virtual void Suspend() override;
		virtual void Resume() override;

		// Functions for the helper threads

		IWorkItem *GetWork();
		void OnComplete(IWorkItem *pWork);

		HANDLE GetWorkReadyEvent() const;
		HANDLE GetKillWorkEvent() const;

	private:
		bool IsWorkCompleted(IWorkItem *pWork);
		bool HasUncompletedWork();
		size_t GetRunningCount();

		void ProcessCompletedWork();
		void WaitForCompletedWork();

		bool EnsureWorkerThread();
		void EndWorkerThreads();

		static unsigned int WINAPI WorkerThread(void *pThreadPoolContext);

		Mutex _cs;

		WinHandle _eventWorkReady;    // _workReady has items
		WinHandle _eventWorkComplete; // _workComplete has items
		WinHandle _eventKillWork;     // loader threads should end ASAP

		Vector<ComPtr<IWorkItem>> _workReady;    // Add() was called
		Vector<ComPtr<IWorkItem>> _workRunning;  // GetWork() was called
		Vector<ComPtr<IWorkItem>> _workComplete; // OnComplete() needs to be called

		Vector<HANDLE> _threads;
		size_t _maxThreads;
	};
}

BEGIN_INTERFACES(ff::CThreadPool)
	HAS_INTERFACE(ff::IThreadPool)
END_INTERFACES()

bool ff::CreateThreadPool(IThreadPool **ppThreadPool, int nMaxThreads)
{
	assertRetVal(ppThreadPool, nullptr);
	*ppThreadPool = nullptr;

	ComPtr<CThreadPool> pPool = new ComObject<CThreadPool>;
	pPool->Init(nMaxThreads);

	*ppThreadPool = pPool.Detach();
	return *ppThreadPool != nullptr;
}

ff::CThreadPool::CThreadPool()
	: _maxThreads(2)
{
	_eventWorkReady = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	_eventWorkComplete = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	_eventKillWork = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	LockMutex crit(GCS_THREAD_POOL);
	GetAllThreadPools().Push(this);
}

void ff::CThreadPool::Init(int nMaxThreads)
{
	assert(IsRunningOnMainThread());

	if (nMaxThreads >= 1 && nMaxThreads <= 32)
	{
		_maxThreads = nMaxThreads;
	}
	else
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);

		_maxThreads = std::min<size_t>(8, si.dwNumberOfProcessors);
	}
}

ff::CThreadPool::~CThreadPool()
{
	EndWorkerThreads();

	LockMutex crit(GCS_THREAD_POOL);
	GetAllThreadPools().DeleteItem(this);
}

static int CompareWorkItems(const ff::ComPtr<ff::IWorkItem> &lhs, const ff::ComPtr<ff::IWorkItem> &rhs)
{
	if (lhs->GetPriority() < rhs->GetPriority())
	{
		return -1;
	}

	if (rhs->GetPriority() < lhs->GetPriority())
	{
		return 1;
	}

	return 0;
}

void ff::CThreadPool::Add(IWorkItem *pWork)
{
	assertRet(pWork);

	if (EnsureWorkerThread())
	{
		LockMutex crit(_cs);
		_workReady.SortInsertFunc(ComPtr<IWorkItem>(pWork), ::CompareWorkItems);
		SetEvent(GetWorkReadyEvent());
	}

	if (IsRunningOnMainThread() && IsProgramShuttingDown())
	{
		// don't let work build up during shutdown (or it could outlive its owner)
		Flush();
	}
}

bool ff::CThreadPool::Wait(IWorkItem *pWork)
{
	assertRetVal(IsRunningOnMainThread(), false);

	ComPtr<IWorkItem> keepAlive = pWork;

	if (!pWork)
	{
		WaitForCompletedWork();
	}

	ProcessCompletedWork();

	while (pWork && !IsWorkCompleted(pWork))
	{
		WaitForCompletedWork();
		ProcessCompletedWork();
	}

	return true;
}

bool ff::CThreadPool::Cancel(IWorkItem *pWork)
{
	size_t nFound = INVALID_SIZE;

	assert(IsRunningOnMainThread());
	assertRetVal(pWork, false);
	{
		LockMutex crit(_cs);

		nFound = _workReady.Find(pWork);

		if (nFound != INVALID_SIZE)
		{
			_workReady.Delete(nFound);
		}
	}

	if (nFound != INVALID_SIZE)
	{
		pWork->InternalOnCancel();
		return true;
	}

	return false;
}

void ff::CThreadPool::Flush()
{
	assert(IsRunningOnMainThread());

	ProcessCompletedWork();

	while (HasUncompletedWork())
	{
		WaitForCompletedWork();
		ProcessCompletedWork();
	}
}

void ff::CThreadPool::Suspend()
{
	assert(IsRunningOnMainThread());

	LockMutex crit(_cs);

	for (size_t i = 0; i < _threads.Size(); i++)
	{
		SuspendThread(_threads[i]);
	}
}

void ff::CThreadPool::Resume()
{
	assert(IsRunningOnMainThread());

	LockMutex crit(_cs);

	for (size_t i = 0; i < _threads.Size(); i++)
	{
		ResumeThread(_threads[i]);
	}
}

// multi-threaded
ff::IWorkItem *ff::CThreadPool::GetWork()
{
	LockMutex crit(_cs);

	IWorkItem *pReturnWork = nullptr;

	if (_maxThreads && !_workReady.IsEmpty())
	{
		ComPtr<IWorkItem> pWork = _workReady.Pop();
		_workRunning.Push(pWork);

		// Don't AddRef the work, I don't want the helper thread to ever
		// be the last to release the work item (it's alive in m_workRunning anyway)
		pReturnWork = pWork;
	}

	if (!pReturnWork)
	{
		// no more work, make sure the worker threads are blocked
		ResetEvent(GetWorkReadyEvent());
	}

	return pReturnWork;
}

// multi-threaded
void ff::CThreadPool::OnComplete(IWorkItem *pWork)
{
	LockMutex crit(_cs);

	ComPtr<IWorkItem> pWorkPtr = pWork;
	size_t nFound = _workRunning.Find(pWorkPtr);

	assertRet(nFound != INVALID_SIZE);

	_workRunning.Delete(nFound);
	_workComplete.Push(pWorkPtr);

	ComPtr<CThreadPool, IThreadPool> pThis = this;
	PostMainThreadFunction([pThis]
	{
		pThis->ProcessCompletedWork();
	});

	SetEvent(_eventWorkComplete);
}

// multi-threaded
HANDLE ff::CThreadPool::GetWorkReadyEvent() const
{
	return _eventWorkReady;
}

// multi-threaded
HANDLE ff::CThreadPool::GetKillWorkEvent() const
{
	return _eventKillWork;
}

bool ff::CThreadPool::IsWorkCompleted(IWorkItem *pWork)
{
	LockMutex crit(_cs);

	ComPtr<IWorkItem> pWorkPtr = pWork;

	return _workReady.Find(pWorkPtr) == INVALID_SIZE &&
		_workRunning.Find(pWorkPtr) == INVALID_SIZE &&
		_workComplete.Find(pWorkPtr) == INVALID_SIZE;
}

bool ff::CThreadPool::HasUncompletedWork()
{
	LockMutex crit(_cs);

	return !_workReady.IsEmpty() ||
		!_workRunning.IsEmpty() ||
		!_workComplete.IsEmpty();
}

size_t ff::CThreadPool::GetRunningCount()
{
	LockMutex crit(_cs);

	return _workRunning.Size() + _workReady.Size();
}

void ff::CThreadPool::ProcessCompletedWork()
{
	ComPtr<IWorkItem> pWork;
	{
		LockMutex crit(_cs);

		if (_workComplete.Size())
		{
			pWork = _workComplete[0];
			_workComplete.Delete(0);
		}

		if (!_workComplete.Size())
		{
			ResetEvent(_eventWorkComplete);
		}
	}

	if (pWork)
	{
		pWork->InternalOnComplete();
	}
}

void ff::CThreadPool::WaitForCompletedWork()
{
	while (GetRunningCount())
	{
		HANDLE eventWorkComplete = _eventWorkComplete;

		if (WAIT_OBJECT_0 == MsgWaitForMultipleObjectsEx(
			1, &eventWorkComplete, INFINITE, QS_ALLEVENTS, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
		{
			break;
		}
		else
		{
			HandleMessages();
		}
	}
}

bool ff::CThreadPool::EnsureWorkerThread()
{
	LockMutex crit(_cs);

	assertRetVal(_maxThreads, false);

	// Create a new thread if all existing threads are busy
	if (GetRunningCount() >= _threads.Size() && _threads.Size() < _maxThreads)
	{
		UINT nThreadID = 0;
		HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, CThreadPool::WorkerThread, this, 0, &nThreadID);
		assert(hThread);

		if (hThread)
		{
			_threads.Push(hThread);

#ifdef _DEBUG
			String threadName = String::format_new(L"Worker %lu for pool %lu",
				_threads.Size() - 1,
				GetAllThreadPools().Find(this));

			SetDebuggerThreadName(threadName, nThreadID);
#endif
		}
	}

	assert(_threads.Size());
	return !_threads.IsEmpty();
}

void ff::CThreadPool::EndWorkerThreads()
{
	// stop new work from starting
	{
		LockMutex crit(_cs);
		_maxThreads = 0;
	}

	while (_workReady.Size())
	{
		// cancel outstanding work

		Vector<ComPtr<IWorkItem>> workReady;
		{
			LockMutex crit(_cs);

			workReady = _workReady;
			_workReady.Clear();
		}

		for (size_t i = 0; i < workReady.Size(); i++)
		{
			workReady[i]->InternalOnCancel();
		}
	}

	SetEvent(GetKillWorkEvent());

	// wait for the threads to finish with their current work items
	{
		Vector<HANDLE> threads;
		{
			LockMutex crit(_cs);
			threads = _threads;
			_threads.Clear();
		}

		while (!threads.IsEmpty())
		{
			DWORD nResult = MsgWaitForMultipleObjectsEx(
				(DWORD)threads.Size(), threads.Data(),
				INFINITE, QS_ALLEVENTS, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);

			if (nResult >= WAIT_OBJECT_0 && nResult < WAIT_OBJECT_0 + threads.Size())
			{
				size_t nThread = nResult - WAIT_OBJECT_0;

				CloseHandle(threads[nThread]);
				threads.Delete(nThread);
			}
			else
			{
				HandleMessages();
			}
		}
	}

	while (_workComplete.Size())
	{
		ProcessCompletedWork();
	}

	assert(_threads.IsEmpty() &&
		_workReady.IsEmpty() &&
		_workRunning.IsEmpty() &&
		_workComplete.IsEmpty());
}

// static WINAPI
unsigned int ff::CThreadPool::WorkerThread(void *pThreadPoolContext)
{
	ff::ThreadGlobalsScope<ff::ThreadGlobals> threadGlobals;
	assertRetVal(threadGlobals.GetGlobals().IsValid(), 1);

	CThreadPool &pool = *(CThreadPool *) pThreadPoolContext;
	HANDLE hWaits[2] = { pool.GetKillWorkEvent(), pool.GetWorkReadyEvent() };

	while (WAIT_OBJECT_0 != WaitForMultipleObjects(2, hWaits, FALSE, INFINITE))
	{
		IWorkItem *pWork = pool.GetWork();

		if (pWork)
		{
			pWork->Run();
			pool.OnComplete(pWork); // don't use pWork after this
		}

		Sleep(0);
	}

	return 0;
}

#endif // !METRO_APP
