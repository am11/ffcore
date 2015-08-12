#pragma once

#include "Windows/WinUtil.h"

namespace ff
{
	class AppGlobals;
	class IAudioDevice;
	class IGraphDevice;
	class IServiceProvider;

	class IMainWindow
#if !METRO_APP
		: public IThreadMessageFilter
#endif
	{
	public:
		virtual ~IMainWindow() { }

		virtual bool Activate() = 0;
		virtual void Close() = 0;
		virtual PWND Recycle() = 0;

		virtual bool FrameAdvance() = 0;
		virtual void FrameRender() = 0;
		virtual void FramePresent(bool vsync) = 0;
		virtual bool FrameVsync() = 0;
		virtual void FrameCleanup() = 0;

		virtual bool IsValid() const = 0;
		virtual bool IsActive() const = 0;
		virtual bool IsVisible() const = 0;
		virtual bool IsModal() const = 0;

		virtual AppGlobals &GetAppGlobals() const = 0;
		virtual ICommandRouter *GetCommandRouter() const = 0;
		virtual IServiceProvider *GetServices() const = 0;
		virtual PWND GetHandle() const = 0;
		virtual IGraphDevice *GetWindowGraph() const = 0;
		virtual IAudioDevice *GetWindowAudio() const = 0;
	};
}
