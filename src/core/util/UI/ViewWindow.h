#pragma once

#include "UI/IViewWindow.h"

namespace ff
{
	class IServiceCollection;
	class IServiceProvider;
	class MainWindow;
	class ProcessGlobals;

	class ViewWindow
		: public IViewWindowInternal
#if !METRO_APP
		, public CustomWindow
#endif
	{
	public:
		UTIL_API ViewWindow(MainWindow *window, IServiceProvider *contextServices);
		UTIL_API virtual ~ViewWindow();

		UTIL_API virtual bool Create(PWND parent, ViewType type);
		UTIL_API virtual void Layout() override;
		UTIL_API virtual void Activate() override;
		UTIL_API virtual bool Deactivate(bool hide) override;
		UTIL_API virtual void WindowActivated() override;
		UTIL_API virtual void WindowDeactivated() override;
		UTIL_API virtual void SetFocus() override;
		UTIL_API virtual bool IsCloseable() const override;
		UTIL_API virtual bool IsLocked() const override;
		UTIL_API virtual bool CanClose() override;
		UTIL_API virtual void Close() override;
		UTIL_API virtual ff::String GetShortName() const override;
		UTIL_API virtual ff::String GetFullName() const override;
		UTIL_API virtual ff::String GetTooltip() const override;
		UTIL_API virtual bool IsDirty() const override;
#if !METRO_APP
		UTIL_API virtual HACCEL GetAcceleratorTable() const override;
#else
		UTIL_API Windows::UI::Core::CoreWindow ^Handle() const;
#endif
		// These are called by the main window while the app is running (active view only)
		UTIL_API virtual bool FrameAdvance() override;
		UTIL_API virtual void FrameRender(IRenderTarget *target) override;

		UTIL_API virtual bool IsValid() const override;
		UTIL_API virtual bool IsActive() const override;

		UTIL_API virtual IMainWindow *GetMainWindow() const override;
		UTIL_API virtual ICommandRouter *GetCommandRouter() const override;
		UTIL_API virtual IServiceProvider *GetServices() const override;
		UTIL_API virtual ViewType GetViewType() const override;
		UTIL_API virtual void SetViewType(ViewType type) override;
		UTIL_API virtual IServiceProvider *GetContextServices() const override;
		UTIL_API virtual void SetContextServices(IServiceProvider *contextServices, bool forceRefresh) override;
		UTIL_API virtual REFGUID GetSingletonId() const override;

		UTIL_API AppGlobals &GetAppGlobals() const;
		UTIL_API ProcessGlobals &GetProcessGlobals() const;

#if !METRO_APP
		// IWindowProcListener
		UTIL_API virtual bool ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT &nResult) override;

		// IThreadMessageFilter
		UTIL_API virtual bool FilterMessage(MSG &msg) override;
#endif

	protected:
#if !METRO_APP
		// Window messages
		UTIL_API virtual bool OnClose();
		UTIL_API virtual void OnDestroy();

		UTIL_API virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
		UTIL_API virtual bool UsesSharedTarget() const;
		UTIL_API virtual HWND GetChildWindow() const;
#endif
		UTIL_API virtual void LayoutChildren();
		UTIL_API virtual void OnActivated();
		UTIL_API virtual void OnDeactivated();
		UTIL_API virtual void OnClosing();
		UTIL_API virtual void OnClosed();
		UTIL_API virtual void OnContextServicesChanged();
		UTIL_API IServiceCollection *GetServiceCollection() const;

		// Initialization
		UTIL_API virtual bool Initialize();
		UTIL_API virtual ComPtr<ICommandRouter> CreateCommandRouter();

	private:
		void SetContextServicesInternal(IServiceProvider *contextServices);

		MainWindow *_mainWindow;
		ComPtr<IServiceCollection> _services;
		ComPtr<IServiceProvider> _contextServices;
		ComPtr<ICommandRouter> _commandRouter;
		ViewType _viewType;
		bool _active;
		bool _allowLayout;
	};
}
