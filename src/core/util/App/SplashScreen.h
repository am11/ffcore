#pragma once

#include "Windows/Handles.h"
#include "Windows/WinUtil.h"

#if METRO_APP

namespace ff
{
	class AppGlobals;

	class SplashScreen
	{
	public:
		SplashScreen();
		~SplashScreen();

		bool Create(AppGlobals &app);
		void Close();
		bool IsShowing() const;
		void SetStatusText(StringRef text);

	private:
	};
}

#else

namespace ff
{
	class AppGlobals;

	class SplashScreen : Dialog
	{
	public:
		SplashScreen();
		~SplashScreen();

		bool Create(AppGlobals &app);
		void Close();
		bool IsShowing() const;
		void SetStatusText(StringRef text);

	protected:
		virtual INT_PTR DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		virtual void PostNcDestroy();

	private:
		static unsigned int WINAPI StaticWorkerThread(void *context);
		unsigned int WorkerThread();
		void OnEraseBackground(HWND hwnd, HDC hdc);

		BitmapHandle _bitmap;
		HINSTANCE _bitmapInstance;
		UINT _bitmapResource;
		String _status;
		Mutex _statusCs;
		bool _created;
	};
}

#endif
