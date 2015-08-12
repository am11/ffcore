#include "pch.h"
#include "App/Idle.h"
#include "App/Timer.h"
#include "COM/ComAlloc.h"
#include "COM/ComObject.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Windows/WinUtil.h"

class __declspec(uuid("ba3bde5e-b216-4113-a236-aaa62da75068"))
	IdleMaster : public ff::ComBase, public ff::IIdleMaster
{
public:
	DECLARE_HEADER(IdleMaster);

	void OnTimer();
	void MessageFilter(MSG &msg);

	// IIdleMaster
	virtual void Add(ff::IWorkItem *work) override;
	virtual bool Remove(ff::IWorkItem *work) override;

	virtual void ForceIdle() override;
	virtual void KickIdle() override;

	virtual size_t GetIdleTime() override;
	virtual void SetIdleTime(size_t nMS) override;

private:
	void EnsureIdleTimer();
	void KillIdleTimer();
	void ResetIdleTimer();

	ff::Mutex _mutex;
	ff::Vector<ff::ComPtr<ff::IWorkItem>> _work;
	UINT_PTR _event;
	UINT_PTR _timerId;
	size_t _idleTime;
	UINT _lastMouseMessage;
	POINT _lastMousePos;
};

BEGIN_INTERFACES(IdleMaster)
	HAS_INTERFACE(ff::IIdleMaster)
END_INTERFACES()

bool ff::CreateIdleMaster(IIdleMaster **obj)
{
	ComPtr<IdleMaster, IIdleMaster> myObj;
	assertRetVal(SUCCEEDED(ComAllocator<IdleMaster>::CreateInstance(&myObj)), false);

	*obj = myObj.Detach();
	return *obj != nullptr;
}

IdleMaster::IdleMaster()
	: _event(ff::CreateGlobalTimerEventID())
	, _timerId(0)
	, _idleTime(100)
	, _lastMouseMessage(WM_NULL)
{
	_lastMousePos.x = -1;
	_lastMousePos.y = -1;
}

IdleMaster::~IdleMaster()
{
	KillIdleTimer();
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

void IdleMaster::Add(ff::IWorkItem *work)
{
	assertRet(work);

	ff::LockMutex crit(_mutex);

	_work.SortInsertFunc(ff::ComPtr<ff::IWorkItem>(work), CompareWorkItems);

	KickIdle();
}

bool IdleMaster::Remove(ff::IWorkItem *work)
{
	assertRetVal(work, false);
	{
		ff::LockMutex crit(_mutex);

		size_t nFound = _work.Find(work);
		assertRetVal(nFound != ff::INVALID_SIZE, false);

		_work.Delete(nFound);

		if (!_work.Size())
		{
			KillIdleTimer();
		}
	}

	work->InternalOnCancel();

	return true;
}

void IdleMaster::ForceIdle()
{
	ff::Vector<ff::ComPtr<ff::IWorkItem>> work;

	KillIdleTimer();

	// Copy the current work items
	{
		ff::LockMutex crit(_mutex);
		work = _work;
	}

	for (size_t i = ff::PreviousSize(work.Size()); i != ff::INVALID_SIZE; i = ff::PreviousSize(i))
	{
		work[i]->Run();
		work[i]->InternalOnComplete();
	}
}

void IdleMaster::KickIdle()
{
	ff::LockMutex crit(_mutex);

	if (_work.Size())
	{
		EnsureIdleTimer();
	}
}

size_t IdleMaster::GetIdleTime()
{
	return _idleTime;
}

void IdleMaster::SetIdleTime(size_t nMS)
{
	_idleTime = nMS;
}

static void CALLBACK OnIdleTimer(HWND hwnd, UINT nMsg, UINT_PTR nEvent, DWORD nTime)
{
	ff::ComPtr<IdleMaster> idleMaster;
	if (idleMaster.QueryFrom(ff::ProcessGlobals::Get()->GetIdleMaster()))
	{
		idleMaster->OnTimer();
	}
}

static bool IdleMessageFilter(MSG &msg)
{
	ff::ComPtr<IdleMaster> idleMaster;
	if (idleMaster.QueryFrom(ff::ProcessGlobals::Get()->GetIdleMaster()))
	{
		idleMaster->MessageFilter(msg);
	}

	return false;
}

void IdleMaster::EnsureIdleTimer()
{
	if (!_timerId)
	{
#if METRO_APP
		_timerId = 1;

		ff::GetMainThreadDispatcher()->RunIdleAsync(
			ref new Windows::UI::Core::IdleDispatchedHandler(
				[](Windows::UI::Core::IdleDispatchedHandlerArgs^ args)
				{
					OnIdleTimer(0, 0, 0, 0);
				}));
#else
		_timerId = SetTimer(nullptr, _event, (UINT)_idleTime, OnIdleTimer);
		ff::AddMessageFilter(IdleMessageFilter);
#endif
	}

	assert(_timerId);
}

void IdleMaster::KillIdleTimer()
{
#if !METRO_APP
	if (_timerId)
	{
		verify(KillTimer(nullptr, _timerId));
		_timerId = 0;

		ff::RemoveMessageFilter(IdleMessageFilter);
	}
#endif
}

void IdleMaster::ResetIdleTimer()
{
	EnsureIdleTimer();
}

void IdleMaster::OnTimer()
{
	ForceIdle();
}

void IdleMaster::MessageFilter(MSG &msg)
{
	assert(_timerId);

	bool bResetIdle = true;

	switch (msg.message)
	{
	case WM_PAINT:
	case 0x0118: // WM_SYSTIMER
		bResetIdle = false;
		break;

	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
		if (_lastMouseMessage == msg.message &&
			_lastMousePos.x == msg.pt.x &&
			_lastMousePos.y == msg.pt.y)
		{
			bResetIdle = false;
		}
		else
		{
			_lastMouseMessage = msg.message;
			_lastMousePos = msg.pt;
		}
		break;
	}

	if (bResetIdle)
	{
		// A non-idle message came in
		ResetIdleTimer();
	}
}
