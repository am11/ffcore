#include "pch.h"
#include "COM/ComAlloc.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Windows/Handles.h"

#if METRO_APP

using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;

namespace ff
{
	// From ThreadPoolShared.cpp:
	Vector<IThreadPool *, 8> &GetAllThreadPools();

	class __declspec(uuid("c5dd1d42-8da9-40f9-8094-cff855c932a6"))
		CThreadPool : public ComBase, public IThreadPool
	{
	public:
		DECLARE_HEADER(CThreadPool);

		void Init();

		// IThreadPool functions

		virtual void Add(IWorkItem *pWork) override;
		virtual bool Wait(IWorkItem *pWork) override;
		virtual bool Cancel(IWorkItem *pWork) override;
		virtual void Flush() override;

		virtual void Suspend() override;
		virtual void Resume() override;

	private:
		struct WorkItemInfo
		{
			ComPtr<IWorkItem>  _work;
			Windows::Foundation::IAsyncAction ^_action;
			WinHandle _event;
		};

		WorkItemInfo *FindInfo(IWorkItem *pWork);

		Mutex _cs;
		List<WorkItemInfo> _running;
	};
}

BEGIN_INTERFACES(ff::CThreadPool)
	HAS_INTERFACE(ff::IThreadPool)
END_INTERFACES()

bool ff::CreateThreadPool(IThreadPool **ppThreadPool, int nMaxThreads)
{
	assertRetVal(ppThreadPool && !nMaxThreads, false);
	*ppThreadPool = nullptr;

	ComPtr<CThreadPool> pPool;
	assertRetVal(SUCCEEDED(ff::ComAllocator<CThreadPool>::CreateInstance(&pPool)), false);
	pPool->Init();

	*ppThreadPool = pPool.Detach();

	return *ppThreadPool != nullptr;
}

ff::CThreadPool::CThreadPool()
{
	LockMutex crit(GCS_THREAD_POOL);
	GetAllThreadPools().Push(this);
}

void ff::CThreadPool::Init()
{
	assert(IsRunningOnMainThread());
}

ff::CThreadPool::~CThreadPool()
{
	Flush();

	LockMutex crit(GCS_THREAD_POOL);
	GetAllThreadPools().Delete(GetAllThreadPools().Find(this));
}

void ff::CThreadPool::Add(IWorkItem *pWork)
{
	assertRet(pWork);

	LockMutex crit(_cs);

	WorkItemInfo *pInfo = &_running.Insert();
	pInfo->_work = pWork;

	pInfo->_action = ThreadPool::RunAsync(ref new WorkItemHandler(
		[=](IAsyncAction^ action)
		{
			pInfo->_work->Run();
		}));

	pInfo->_action->Completed = ref new AsyncActionCompletedHandler(
		[this, pInfo](IAsyncAction^ action, AsyncStatus status)
		{
			PostMainThreadFunction([this, status, pInfo]
			{
				if (status == AsyncStatus::Completed)
				{
					pInfo->_work->InternalOnComplete();
				}
				else
				{
					pInfo->_work->InternalOnCancel();
				}

				if (pInfo->_event != nullptr)
				{
					SetEvent(pInfo->_event);
				}

				pInfo->_action->Close();

				LockMutex crit(_cs);
				_running.Delete(*pInfo);
			});
		});
}

bool ff::CThreadPool::Wait(IWorkItem *pWork)
{
	assertRetVal(IsRunningOnMainThread(), false);

	WorkItemInfo *pInfo = FindInfo(pWork);
	if (pInfo)
	{
		if (!pInfo->_event)
		{
			pInfo->_event = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
		}

		WinHandle doneEvent;
		HANDLE hProcess = GetCurrentProcess();
		assertRetVal(DuplicateHandle(hProcess, pInfo->_event, hProcess, &doneEvent, 0, FALSE, DUPLICATE_SAME_ACCESS), false);

		WaitForHandle(doneEvent);

		assert(!FindInfo(pWork));
	}

	return pInfo != nullptr;
}

bool ff::CThreadPool::Cancel(IWorkItem *pWork)
{
	assert(IsRunningOnMainThread());

	WorkItemInfo *pInfo = FindInfo(pWork);
	if (pInfo && pInfo->_action && pInfo->_action->Status == AsyncStatus::Started)
	{
		pInfo->_action->Cancel();
		return true;
	}

	return false;
}

void ff::CThreadPool::Flush()
{
	assert(IsRunningOnMainThread());

	while (_running.Size())
	{
		Wait(_running.GetFirst()->_work);
	}
}

void ff::CThreadPool::Suspend()
{
	// assert(IsRunningOnMainThread());
	// can't suspend
}

void ff::CThreadPool::Resume()
{
	// assert(IsRunningOnMainThread());
	// can't suspend, so can't resume
}

ff::CThreadPool::WorkItemInfo *ff::CThreadPool::FindInfo(IWorkItem *pWork)
{
	LockMutex crit(_cs);

	for (WorkItemInfo *pInfo = _running.GetFirst(); pInfo; pInfo = _running.GetNext(*pInfo))
	{
		if (pInfo->_work == pWork)
		{
			return pInfo;
		}
	}

	return nullptr;
}

#endif // METRO_APP
