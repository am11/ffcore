#pragma once

#include "Windows/WinUtil.h"

namespace ff
{
	class AppGlobals;
	class ICommandRouter;
	class IMainWindow;
	class IRenderTarget;
	class IServiceProvider;

	enum class ViewType : size_t
	{
		UNKNOWN,
		DOCUMENT,
		DOCUMENT_TOOLBAR, // above document
		DOCUMENT_TABS, // above document toolbar
		DOCUMENT_STATUS, // below document
		DOCKED_TOP, // toolbar
		DOCKED_BOTTOM, // status bar
		DOCKED_LEFT, // left of document, not top or bottom views
		DOCKED_RIGHT, // right of document, not top or bottom views

		VIEW_TYPE_COUNT,
		FIRST_CUSTOM,
		LAST_CUSTOM = 1000,
	};

	class IViewWindow
	{
	public:
		virtual ~IViewWindow() { }

		virtual bool FrameAdvance() = 0;
		virtual void FrameRender(IRenderTarget *target) = 0;

		virtual bool IsValid() const = 0;
		virtual bool IsActive() const = 0;

		virtual IMainWindow *GetMainWindow() const = 0;
		virtual ICommandRouter *GetCommandRouter() const = 0;
		virtual IServiceProvider *GetServices() const = 0;
		virtual ViewType GetViewType() const = 0;
		virtual void SetViewType(ViewType type) = 0;
		virtual IServiceProvider *GetContextServices() const = 0;
		virtual void SetContextServices(IServiceProvider *contextServices, bool forceRefresh) = 0;
		virtual REFGUID GetSingletonId() const = 0;
	};

	class IViewWindowInternal
		: public IViewWindow
#if !METRO_APP
		, public IWindowProcListener
		, public IThreadMessageFilter
#endif
	{
	public:
		virtual void Layout() = 0;
		virtual void Activate() = 0;
		virtual bool Deactivate(bool hide) = 0;
		virtual void WindowActivated() = 0;
		virtual void WindowDeactivated() = 0;
		virtual void SetFocus() = 0;
		virtual bool IsCloseable() const = 0;
		virtual bool IsLocked() const = 0;
		virtual bool CanClose() = 0;
		virtual void Close() = 0;
		virtual ff::String GetShortName() const = 0;
		virtual ff::String GetFullName() const = 0;
		virtual ff::String GetTooltip() const = 0;
		virtual bool IsDirty() const = 0;
#if !METRO_APP
		virtual HACCEL GetAcceleratorTable() const = 0;
#endif
	};
}
