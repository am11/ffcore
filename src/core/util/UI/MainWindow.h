#pragma once

#include "App/Commands.h"
#include "App/MostRecentlyUsed.h"
#include "Resource/ResourceContext.h"
#include "UI/IMainWindow.h"
#include "UI/IViewWindow.h"
#include "Windows/Handles.h"
#include "Windows/WinUtil.h"

namespace ff
{
#if METRO_APP
	using namespace Platform;
	using namespace Windows::ApplicationModel;
	using namespace Windows::Foundation;
	using namespace Windows::UI::Core;

	ref class MainWindowEvents;
#endif

	class EntityDomain;
	class IMainWindowHost;
	class IServiceCollection;
	class IServiceProvider;
	class IViewWindow;
	class IViewWindowInternal;
	class Dict;
	class ProcessGlobals;

	class MainWindow
		: public IMainWindow
		, public ICommandGroupListener
		, public IMostRecentlyUsedListener
		, public IResourceContext
#if !METRO_APP
		, public IWindowProcListener
		, public CustomWindow
#endif
	{
	public:
		UTIL_API MainWindow(AppGlobals &app, IMainWindowHost *host, IServiceProvider *contextServices);
		UTIL_API virtual ~MainWindow();

#if METRO_APP
		UTIL_API Windows::UI::Core::CoreWindow ^Handle() const;
#endif

		UTIL_API virtual bool Create(PWND attachWindow = nullptr);
		UTIL_API virtual void Layout();
		UTIL_API virtual bool Activate() override;
		UTIL_API virtual void Close() override;
		UTIL_API virtual PWND Recycle() override;

		// These are called by the AppGlobals while the app is running
		UTIL_API virtual bool FrameAdvance() override;
		UTIL_API virtual void FrameRender() override;
		UTIL_API virtual void FramePresent(bool vsync) override;
		UTIL_API virtual bool FrameVsync() override;
		UTIL_API virtual void FrameCleanup() override;
		// Only for special cases where an external message pump isn't advancing the game
		UTIL_API virtual void PostRequestForFrameUpdate(bool vsync);

		UTIL_API virtual bool IsValid() const override;
		UTIL_API virtual bool IsActive() const override;
		UTIL_API virtual bool IsVisible() const override;
		UTIL_API virtual bool IsModal() const override;
		UTIL_API virtual void PushModal();
		UTIL_API virtual void PopModal();
		UTIL_API virtual void CancelModal();

		UTIL_API virtual AppGlobals &GetAppGlobals() const override;
		UTIL_API virtual ICommandRouter *GetCommandRouter() const override;
		UTIL_API virtual IServiceProvider *GetServices() const override;
		UTIL_API virtual PWND GetHandle() const override;
		UTIL_API virtual IGraphDevice *GetWindowGraph() const override;
		UTIL_API virtual IAudioDevice *GetWindowAudio() const override;

		UTIL_API bool LoadResource(StringRef name, REFGUID iid, void **obj);
		template<typename T> bool LoadResource(StringRef name, T **obj);
		UTIL_API Dict &GetWindowOptions() const;
		UTIL_API ProcessGlobals &GetProcessGlobals() const;
		UTIL_API ICommandGroups *GetCommandGroups() const;
#if !METRO_APP
		UTIL_API ff::String GetTitleText() const;
		UTIL_API void UpdateWindowTitleText();
#endif
		// IResourceContext
		UTIL_API virtual IRenderTargetWindow *GetTarget() const override;
		UTIL_API virtual IRenderDepth *GetDepth() const override;
		UTIL_API virtual IGraphDevice *GetGraph() const override;
		UTIL_API virtual IAudioDevice *GetAudio() const override;
		UTIL_API virtual IMouseDevice *GetMouse() const override;
		UTIL_API virtual ITouchDevice *GetTouch() const override;
		UTIL_API virtual IKeyboardDevice *GetKeys() const override;
		UTIL_API virtual IJoystickInput *GetJoysticks() const override;
		UTIL_API virtual I2dRenderer *Get2dRender() const override;
		UTIL_API virtual I2dEffect *Get2dEffect() const override;

		// Views
		UTIL_API size_t GetViewCount(ViewType type) const;
		UTIL_API std::shared_ptr<IViewWindow> GetView(ViewType type, size_t index) const;
		UTIL_API std::shared_ptr<IViewWindow> GetActiveView(ViewType type) const;
		UTIL_API Vector<std::shared_ptr<IViewWindow>> GetOrderedViews(ViewType type) const;
		UTIL_API std::shared_ptr<IViewWindow> OpenNewView(ViewType type, StringRef typeName, IServiceProvider *contextServices);
		UTIL_API std::shared_ptr<IViewWindow> FindSingletonView(ViewType type, REFGUID id) const;
		UTIL_API bool ActivateView(IViewWindow *view);
		UTIL_API bool CloseView(IViewWindow *view, bool forceClose = false);
		UTIL_API bool CloseAllViews(ViewType type, bool forceClose = false);
		UTIL_API bool MoveView(IViewWindow *view, size_t index);
		UTIL_API PWND GetDocumentViewParent();
		UTIL_API PWND GetDocumentViewTarget() const;
		UTIL_API ViewType GetCurrentFocusViewType() const;
		UTIL_API ViewType GetPreviousFocusViewType() const;
#if !METRO_APP
		UTIL_API void HideSharedViewTarget();
		UTIL_API void ShowViews(ViewType type, bool show);
#endif
		// View tab commands
		UTIL_API virtual void OnCloseTab();
		UTIL_API virtual void OnNextTab();
		UTIL_API virtual void OnPrevTab();

		// Full screen
		UTIL_API bool ToggleFullScreen();
		UTIL_API bool IsFullScreen() const;
		UTIL_API bool IsFullWindowTarget() const;
		UTIL_API bool CanSetFullScreen() const;
		UTIL_API bool SetFullScreen(bool fullScreen);
		UTIL_API virtual void OnSetFullScreen(bool fullScreen);

		// DPI ("dip" is device independent pixel, "pixel" is device dependent)
		UTIL_API PointInt DipToPixel(const PointInt &dip);
		UTIL_API PointDouble DipToPixel(const PointDouble &dip);
		UTIL_API PointInt PixelToDip(const PointInt &pixel);
		UTIL_API PointDouble PixelToDip(const PointDouble &pixel);

		UTIL_API RectInt DipToPixel(const RectInt &dip);
		UTIL_API RectDouble DipToPixel(RectDouble &dip);
		UTIL_API RectInt PixelToDip(const RectInt &pixel);
		UTIL_API RectDouble PixelToDip(const RectDouble &pixel);

		UTIL_API int DipToPixelX(int dip);
		UTIL_API int DipToPixelY(int dip);
		UTIL_API double DipToPixelX(double dip);
		UTIL_API double DipToPixelY(double dip);
		UTIL_API int PixelToDipX(int pixel);
		UTIL_API int PixelToDipY(int pixel);
		UTIL_API double PixelToDipX(double pixel);
		UTIL_API double PixelToDipY(double pixel);

		// Commands
		UTIL_API virtual bool ExecuteCommand(DWORD id);
		UTIL_API virtual void PostExecuteCommand(DWORD id);
		UTIL_API virtual void OnCommandExecuted(DWORD id);

		// ICommandGroupListener
		UTIL_API virtual void UpdateCommands(const DWORD *ids, size_t count) override;
		UTIL_API virtual void UpdateGroups(const DWORD *groups, size_t count) override;
		UTIL_API virtual void OnGroupInvalidated(DWORD id) override;

		// IMostRecentlyUsedListener
		UTIL_API virtual void OnMruChanged(const MostRecentlyUsed &mru) override;

#if !METRO_APP
		// IWindowProcListener
		UTIL_API virtual bool ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT &nResult) override;

		// IThreadMessageFilter
		UTIL_API virtual bool FilterMessage(MSG &msg) override;
#endif

	protected:
#if !METRO_APP
		UTIL_API virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
		UTIL_API virtual void OnSize(WPARAM wParam);
		UTIL_API virtual void SavePosition();
		UTIL_API virtual RectInt GetDefaultWindowPosition() const;
#endif
		UTIL_API virtual bool CanClose();
		UTIL_API virtual bool CanRecycle();
		UTIL_API virtual void OnClosing();
		UTIL_API virtual void OnEndSession();
		UTIL_API virtual bool CanEndSession(bool askUser);
		UTIL_API virtual void OnActivated();
		UTIL_API virtual void OnDeactivated();
		UTIL_API virtual void OnViewOpened(IViewWindowInternal *view);
		UTIL_API virtual void OnViewClosed(IViewWindowInternal *view);
		UTIL_API virtual void OnViewMoved(IViewWindowInternal *view, size_t oldIndex, size_t newIndex);
		UTIL_API virtual void OnViewActivated(IViewWindowInternal *view);
		UTIL_API virtual void OnViewDeactivated(IViewWindowInternal *view);
		UTIL_API virtual bool OnAccelerator(DWORD id);

		// View windows
		typedef List<std::shared_ptr<IViewWindowInternal>> InternalViewList;
		typedef Vector<std::shared_ptr<IViewWindowInternal>> InternalViewVector;

		struct InternalViewInfo
		{
			InternalViewInfo();
			InternalViewInfo(const InternalViewInfo &rhs);
			InternalViewInfo(InternalViewInfo &&rhs);
			InternalViewInfo &operator=(const InternalViewInfo &rhs);

			ViewType _type;
			InternalViewVector _views;
			InternalViewList _viewOrder;
			MPWND _parentHandle;
#if !METRO_APP
			ListenedWindow _parentWindow;
#endif
		};

		UTIL_API virtual ff::PWND GetViewParent(ViewType type, bool onlyVisibleChild = false);
		UTIL_API virtual std::shared_ptr<IViewWindowInternal> OpenNewView(PWND parent, ViewType type, StringRef typeName, IServiceProvider *contextServices);
		UTIL_API virtual std::shared_ptr<IViewWindowInternal> NewView(PWND parent, ViewType type, StringRef typeName, IServiceProvider *contextServices);
		UTIL_API virtual void OnFocusViewTypeChanged(ViewType previousType, ViewType newType);

		UTIL_API virtual std::shared_ptr<IViewWindowInternal> GetActiveViewInternal(ViewType type) const;
		UTIL_API virtual InternalViewInfo *GetInternalViewInfo(ViewType type);
		UTIL_API const InternalViewInfo *GetInternalViewInfo(ViewType type) const;
		UTIL_API ViewType GetFreeCustomViewType() const;
		UTIL_API ViewType FindViewTypeForWindow(PWND window) const;
		UTIL_API void CheckViewFocus();

#if !METRO_APP
		UTIL_API virtual bool CreateDocumentViewTarget(CustomWindow &sharedViewTarget);
		UTIL_API virtual bool CreateViewParent(ViewType type, CustomWindow &window);
		UTIL_API virtual void LayoutNow();
		UTIL_API virtual void LayoutActiveViews(ViewType type);
		UTIL_API virtual bool UseFullScreenViews() const;
		UTIL_API virtual bool AllowFullScreen() const;

		// View cursor
		UTIL_API virtual HCURSOR GetCursorViewTarget() const;
		UTIL_API virtual bool UseCursorFullScreen() const;
		UTIL_API virtual bool UseCursorWindowed() const;
#endif

		// Initialization
		UTIL_API virtual bool Initialize();
		UTIL_API virtual bool InitializeView();
		UTIL_API virtual bool InitializeWindowOptions(Dict &dict);
		UTIL_API virtual bool InitializePosition();
		UTIL_API virtual bool InitializeVisibility();
		UTIL_API virtual bool InitializeAcceleratorTable();
		UTIL_API virtual bool InitializeGraphics();
		UTIL_API virtual bool InitializeRenderTarget();
		UTIL_API virtual bool InitializeDepthBuffer();
		UTIL_API virtual bool InitializeRender2d();
		UTIL_API virtual bool InitializeKeyboard();
		UTIL_API virtual bool InitializeMouse();
		UTIL_API virtual bool InitializeTouch();
		UTIL_API virtual bool InitializeJoysticks();
		UTIL_API virtual bool InitializeAudio();
		UTIL_API virtual void Cleanup();
#if METRO_APP
		UTIL_API virtual bool InitializeMetroWindow(PWND attachWindow);
#endif

		// Commands
		UTIL_API virtual ComPtr<ICommandRouter> CreateCommandRouter();
		UTIL_API virtual ComPtr<ICommandGroups> CreateCommandGroups();
		UTIL_API virtual bool InitializeCommandGroups(ICommandGroups *groups);

		// Services
		UTIL_API IServiceCollection *GetServiceCollection() const;

		// For creating the window class
		UTIL_API virtual String GetClassName() const;
#if !METRO_APP
		UTIL_API virtual HINSTANCE GetClassInstance() const;
		UTIL_API virtual DWORD GetClassStyle() const;
		UTIL_API virtual HCURSOR GetClassCursor() const;
		UTIL_API virtual HBRUSH GetClassBackgroundBrush() const;
		UTIL_API virtual UINT GetClassMenu() const;
		UTIL_API virtual HICON GetClassIcon(int size) const;
		UTIL_API virtual String GetDefaultText() const;
		UTIL_API virtual DWORD GetDefaultStyle() const;
		UTIL_API virtual DWORD GetDefaultExStyle() const;
		UTIL_API virtual HMENU GetDefaultMenu() const;
		UTIL_API virtual UINT GetAcceleratorTable() const;

		// Window
		UTIL_API virtual ff::String ComputeTitleText();
		UTIL_API virtual void OnTitleTextChanged(ff::String value);
#endif

	private:
		bool ActivateView(IViewWindow *view, bool tabOrder);
		void ResetTabOrder();
		void CheckViewFocusNow();
		void RefreshPosition();

		void OnDpiChanged();
		void OnSuspended(bool suspend);
		void OnActivate(bool active);
		bool OnClose(bool force = false);
		void OnDestroy();
#if !METRO_APP
		void OnEnable(bool enabled);
		void OnSetFocus();
		bool OnSetCursorViewTarget();
		bool OnPaintViewTarget();
#endif
		ComPtr<IGraphDevice> _graph;
		ComPtr<IRenderTargetWindow> _renderTarget;
		ComPtr<IRenderDepth> _renderDepth;
		ComPtr<IMouseDevice> _mouse;
		ComPtr<ITouchDevice> _touch;
		ComPtr<IKeyboardDevice> _keyboard;
		ComPtr<IJoystickInput> _joysticks;
		ComPtr<IAudioDevice> _audio;
		ComPtr<I2dRenderer> _render2d;
		ComPtr<I2dEffect> _effect2d;
		ComPtr<IServiceCollection> _services;

		AppGlobals &_app;
		IMainWindowHost *_host;
		ComPtr<ICommandRouter> _commandRouter;
		ComPtr<ICommandGroups> _commandGroups;

		ViewType _currentFocusViewType;
		ViewType _previousFocusViewType;
		Map<ViewType, InternalViewInfo> _viewInfo;
		const std::shared_ptr<IViewWindowInternal> *_currentTabOrderDocumentView;

		String _windowClassName;
		size_t _modal;
		PointInt _dpi; // 96 is normal
		PointDouble _dpiScaleDipToPixel; // 1.0 is normal
		bool _active;
		bool _suspended;
		bool _destroyed;
		bool _allowLayout;
		bool _audioPaused;

#if METRO_APP
		friend ref class MainWindowEvents;

		Agile<CoreWindow> _coreWindow;
		MainWindowEvents ^_coreWindowEvents;
#else
		HWND _previousFocusWindow;
		String _titleText;
		AccelHandle _acceleratorTable;
		ListenedWindow _documentViewTarget;
#endif
	};

	template<typename T>
	bool MainWindow::LoadResource(StringRef name, T **obj)
	{
		return LoadResource(name, __uuidof(T), (void **)obj);
	}
}
