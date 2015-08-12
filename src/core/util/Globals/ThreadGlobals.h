#pragma once

namespace ff
{
	class ThreadGlobals
	{
	public:
		UTIL_API ThreadGlobals();
		UTIL_API ~ThreadGlobals();

		UTIL_API static ff::ThreadGlobals *Get();

		UTIL_API virtual bool Startup();
		UTIL_API virtual void Shutdown();

		UTIL_API unsigned int ThreadId() const;
		UTIL_API bool IsValid() const;
		UTIL_API bool IsShuttingDown() const;
		UTIL_API void AtShutdown(std::function<void()> func);

	protected:
		void CallShutdownFunctions();

		enum class State
		{
			UNSTARTED,
			STARTED,
			FAILED,
			SHUT_DOWN,
		} _state;

	private:
		Mutex _mutex;
		unsigned int _id;
		List<std::function<void()>> _shutdownFunctions;
	};

	UTIL_API void AtThreadShutdown(std::function<void()> func);
}
