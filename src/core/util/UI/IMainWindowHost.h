#pragma once

namespace ff
{
	class IMainWindow;

	class IMainWindowHost
	{
	public:
		virtual ~IMainWindowHost() { }

		virtual void OnWindowCreated(IMainWindow &window) = 0;
		virtual void OnWindowInitialized(IMainWindow &window) = 0;
		virtual void OnWindowActivated(IMainWindow &window) = 0;
		virtual void OnWindowDeactivated(IMainWindow &window) = 0;
		virtual bool OnWindowClosing(IMainWindow &window) = 0;
		virtual void OnWindowDestroyed(IMainWindow &window) = 0;
		virtual void OnWindowModalStart(IMainWindow &window) = 0;
		virtual void OnWindowModalEnd(IMainWindow &window) = 0;
		virtual void OnWindowSuspending(IMainWindow &window) = 0;
		virtual void OnWindowResumed(IMainWindow &window) = 0;
	};
}
