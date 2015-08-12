#pragma once

namespace ff
{
	class ThreadGlobals;

	class GlobalsScope
	{
	public:
		UTIL_API GlobalsScope(ThreadGlobals &globals);
		UTIL_API ~GlobalsScope();

	private:
		ThreadGlobals &_globals;
	};

	// T must be ThreadGlobals or something derived from it
	template<class T>
	class ThreadGlobalsScope
	{
	public:
		ThreadGlobalsScope();
		~ThreadGlobalsScope();

		T &GetGlobals();

	private:
		T _globals;
		GlobalsScope _scope;
	};
}

template<class T>
ff::ThreadGlobalsScope<T>::ThreadGlobalsScope()
	: _scope(_globals)
{
}

template<class T>
ff::ThreadGlobalsScope<T>::~ThreadGlobalsScope()
{
}

template<class T>
T &ff::ThreadGlobalsScope<T>::GetGlobals()
{
	return _globals;
}
