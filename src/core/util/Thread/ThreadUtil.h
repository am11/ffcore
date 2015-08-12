#pragma once

namespace ff
{
	class Module;

	UTIL_API DWORD GetMainThreadID();
	UTIL_API bool IsRunningOnMainThread();
	UTIL_API bool IsEventSet(HANDLE hEvent);
	UTIL_API bool WaitForHandle(HANDLE handle); // allows PostMainThreadFunction to work while waiting
	UTIL_API void Sleep(size_t ms);

#if METRO_APP
	UTIL_API Windows::UI::Core::CoreWindow ^GetMainThreadWindow();
	UTIL_API Windows::UI::Xaml::Window ^GetMainThreadWindowXaml();
	UTIL_API Windows::UI::Core::CoreDispatcher ^GetMainThreadDispatcher();
#endif

	UTIL_API void FlushMainThreadFunctions();
	UTIL_API void PostMainThreadFunction(std::function<void ()> func, Module *module = nullptr);

	UTIL_API void SetDebuggerThreadName(StringRef name, DWORD nThreadID = 0);

	void StartupMainThread();
	void ShutdownMainThread();
}
