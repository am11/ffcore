#pragma once

#if !METRO_APP

namespace ff
{
	class IThreadMessageFilter;

	// Wrapper for high-resolution multimedia timers
	class EventTimer
	{
	public:
		UTIL_API EventTimer();
		UTIL_API ~EventTimer();

		UTIL_API bool IsValid() const; // Can this timer even be used?
		UTIL_API bool GetResolutionRange(UINT &nMin, UINT &nMax);

		UTIL_API bool Start(UINT nDelay, UINT nResolution, bool bPaused); // in milliseconds (if paused, must call Resume)
		UTIL_API bool Running() const; // Was the timer started?
		UTIL_API size_t Fired() const; // How many times has the timer fired since the last wait?
		UTIL_API bool Wait(IThreadMessageFilter *filter = nullptr); // Wait for the timer to fire. Returns false if WM_QUIT was received.
		UTIL_API void Pause(); // Call this when you aren't going to call Wait() for a while
		UTIL_API void Resume(); // Start again with the previous delay values
		UTIL_API void Stop(); // Invalidates this timer (cannot Resume anymore, must call Start)

	private:
		static void CALLBACK OnTimerCallback(UINT nID, UINT, DWORD_PTR pUser, DWORD_PTR, DWORD_PTR);
		void OnTimer();

		Mutex _cs;
		HANDLE _event;
		size_t _fired;
		UINT _id;
		UINT _delay;
		UINT _resolution;
		bool _valid;
	};
}

#endif // !METRO_APP
