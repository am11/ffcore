#include "pch.h"
#include "Globals/ThreadGlobals.h"
#include "Thread/ThreadUtil.h"

static __declspec(thread) ff::ThreadGlobals *s_threadGlobals = nullptr;

ff::ThreadGlobals::ThreadGlobals()
	: _state(State::UNSTARTED)
	, _id(GetCurrentThreadId())
{
	assert(s_threadGlobals == nullptr);
	s_threadGlobals = this;
}

ff::ThreadGlobals::~ThreadGlobals()
{
	assert(s_threadGlobals == this && IsShuttingDown());
	s_threadGlobals = nullptr;
}

ff::ThreadGlobals *ff::ThreadGlobals::Get()
{
	assert(s_threadGlobals);
	return s_threadGlobals;
}

bool ff::ThreadGlobals::Startup()
{
	_state = State::STARTED;

	return IsValid();
}

void ff::ThreadGlobals::Shutdown()
{
	CallShutdownFunctions();
}

unsigned int ff::ThreadGlobals::ThreadId() const
{
	return _id;
}

bool ff::ThreadGlobals::IsValid() const
{
	switch (_state)
	{
	case State::UNSTARTED:
	case State::FAILED:
		return false;

	default:
		return true;
	}
}

bool ff::ThreadGlobals::IsShuttingDown() const
{
	return _state == State::SHUT_DOWN;
}

void ff::ThreadGlobals::AtShutdown(std::function<void()> func)
{
	assert(!IsShuttingDown());

	LockMutex lock(_mutex);
	_shutdownFunctions.Insert(std::move(func));
}

void ff::ThreadGlobals::CallShutdownFunctions()
{
	_state = State::SHUT_DOWN;

	Vector<std::function<void()>> shutdownFunctions;
	{
		LockMutex lock(_mutex);
		shutdownFunctions.Reserve(_shutdownFunctions.Size());

		while (!_shutdownFunctions.IsEmpty())
		{
			shutdownFunctions.Push(std::move(*_shutdownFunctions.GetLast()));
			_shutdownFunctions.DeleteLast();
		}
	}

	for (const auto &func: shutdownFunctions)
	{
		func();
	}
}

void ff::AtThreadShutdown(std::function<void()> func)
{
	assertRet(s_threadGlobals != nullptr);

	if (s_threadGlobals->IsShuttingDown())
	{
		assertSz(false, L"Why register a thread shutdown function during shutdown?");
		func();
	}
	else
	{
		s_threadGlobals->AtShutdown(func);
	}
}
