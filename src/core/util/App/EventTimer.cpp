#include "pch.h"
#include "App/EventTimer.h"
#include "Windows/WinUtil.h"

#if METRO_APP
bool s_unusedFile_EventTimer = false;
#else

// static
void CALLBACK ff::EventTimer::OnTimerCallback(UINT nID, UINT, DWORD_PTR pUser, DWORD_PTR, DWORD_PTR)
{
	// this function is called on a helper thread

	EventTimer *pTimer = (EventTimer*)pUser;
	pTimer->OnTimer();
}

ff::EventTimer::EventTimer()
	: _valid(false)
	, _fired(0)
	, _delay(0)
	, _resolution(0)
	, _id(0)
	, _event(nullptr)
{
}

ff::EventTimer::~EventTimer()
{
	Stop();
}

bool ff::EventTimer::IsValid() const
{
	return _valid;
}

bool ff::EventTimer::GetResolutionRange(UINT &nMin, UINT &nMax)
{
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(tc)) != TIMERR_NOERROR)
	{
		return false;
	}

	nMin = tc.wPeriodMin;
	nMax = tc.wPeriodMax;

	return true;
}

bool ff::EventTimer::Start(UINT nDelay, UINT nResolution, bool bPaused)
{
	LockMutex crit(_cs);

	Stop();

	UINT nMin, nMax;
	if (!GetResolutionRange(nMin, nMax))
	{
		return false;
	}

	// fix the resolution
	nResolution = std::max(nResolution, nMin);
	nResolution = std::min(nResolution, nMax);

	if (timeBeginPeriod(nResolution) != TIMERR_NOERROR)
	{
		return false;
	}

	_valid      = true;
	_delay      = nDelay;
	_resolution = nResolution;

	_event = CreateEvent(
		nullptr,  // security
		FALSE, // manual reset
		FALSE, // signaled
		nullptr); // name

	assertRetVal(_event, false);

	if (!bPaused)
	{
		Resume();
	}

	return true;
}

bool ff::EventTimer::Running() const
{
	return _id != 0;
}

size_t ff::EventTimer::Fired() const
{
	return _fired;
}

bool ff::EventTimer::Wait(IThreadMessageFilter *filter)
{
	if (!HandleMessages(filter))
	{
		return false;
	}

	if (Running())
	{
		while (!Fired() && WAIT_OBJECT_0 != MsgWaitForMultipleObjectsEx(
			1, &_event, INFINITE, QS_ALLEVENTS, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE))
		{
			HandleMessages(filter);
		}

		// _event and _fired must be updated together
		{
			LockMutex crit(_cs);

			ResetEvent(_event);
			_fired = 0;
		}
	}

	return true;
}

void ff::EventTimer::Pause()
{
	if (_id)
	{
		LockMutex crit(_cs);

		if (_id)
		{
			timeKillEvent(_id);
			_id = 0;
			_fired = 0;
		}
	}
}

void ff::EventTimer::Resume()
{
	if (!_id && _event)
	{
		LockMutex crit(_cs);

		if (!_id && _event)
		{
			_id = timeSetEvent(
				_delay,
				_resolution,
				EventTimer::OnTimerCallback,
				(DWORD_PTR)this,
				TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
		}
	}
}

void ff::EventTimer::Stop()
{
	LockMutex crit(_cs);

	Pause();

	if (_valid)
	{
		timeEndPeriod(_resolution);
		_valid = false;
	}

	if (_event)
	{
		CloseHandle(_event);
		_event = nullptr;
	}
}

void ff::EventTimer::OnTimer()
{
	// this function is called on a helper thread

	LockMutex crit(_cs);

	if (Running())
	{
		_fired++;
		SetEvent(_event);
	}
}

#endif // !METRO_APP
