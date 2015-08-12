#include "pch.h"
#include "Globals/ProcessGlobals.h"

// STATIC_DATA (object) - CRITICAL SECTIONS
static ff::Mutex s_staticMutex[ff::GCS_COUNT];
ff::Mutex &ff::GetGlobalMutex(GlobalMutex type)
{
	assert(IsProgramRunning());
	return s_staticMutex[type];
}

ff::Mutex::Mutex(bool lockable)
{
	::InitializeCriticalSectionEx(&_mutex, 3000, 0);
	_lockable = lockable ? &_mutex : nullptr;
}

ff::Mutex::~Mutex()
{
	if (_lockable == &_mutex)
	{
		::DeleteCriticalSection(&_mutex);
	}
}

void ff::Mutex::Enter() const
{
	assert(DidProgramStart());

	if (_lockable != nullptr)
	{
		::EnterCriticalSection(_lockable);
	}
}

bool ff::Mutex::TryEnter() const
{
	assert(DidProgramStart());

	return _lockable != nullptr && ::TryEnterCriticalSection(_lockable) != FALSE;
}

void ff::Mutex::Leave() const
{
	if (_lockable != nullptr)
	{
		::LeaveCriticalSection(_lockable);
	}
}

bool ff::Mutex::IsLockable() const
{
	return _lockable != nullptr;
}

void ff::Mutex::SetLockable(bool lockable)
{
	_lockable = lockable ? &_mutex : nullptr;
}

ff::LockMutex::LockMutex(const Mutex &mutex)
	: _mutex(IsProgramRunning() ? &mutex : nullptr)
{
	if (_mutex != nullptr)
	{
		_mutex->Enter();
	}
}

ff::LockMutex::LockMutex(GlobalMutex type)
	: _mutex(IsProgramRunning() ? &GetGlobalMutex(type) : nullptr)
{
	if (_mutex != nullptr)
	{
		_mutex->Enter();
	}
}

ff::LockMutex::~LockMutex()
{
	if (_mutex != nullptr)
	{
		_mutex->Leave();
	}
}
