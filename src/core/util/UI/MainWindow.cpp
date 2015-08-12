#include "pch.h"
#include "App/AppGlobals.h"
#include "App/Log.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioFactory.h"
#include "COM/ServiceCollection.h"
#include "Dict/Dict.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/2D/2dEffect.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Input/JoystickInput.h"
#include "Input/KeyboardDevice.h"
#include "Input/MouseDevice.h"
#include "Input/TouchDevice.h"
#include "Module/Module.h"
#include "Resource/util-resource.h"
#include "String/StringUtil.h"
#include "Thread/ThreadUtil.h"
#include "UI/MainWindow.h"
#include "UI/IMainWindowHost.h"
#include "UI/ViewWindow.h"

static const int DEFAULT_DPI = 96;
static const double DEFAULT_DPI_D = 96.0;

#if !METRO_APP
static const UINT WM_APP_LAYOUT_NOW = ff::CreateAppMessage();
static const UINT WM_APP_CHECK_FOCUS_NOW = ff::CreateAppMessage();
static const UINT WM_APP_UPDATE_TITLE_NOW = ff::CreateAppMessage();
static const UINT WM_APP_FRAME_UPDATE_NOW = ff::CreateAppMessage();
static const UINT WM_APP_REFRESH_POSITION = ff::CreateAppMessage();
#endif

ff::MainWindow::InternalViewInfo::InternalViewInfo()
	: _type(ViewType::UNKNOWN)
	, _parentHandle(nullptr)
{
}

ff::MainWindow::InternalViewInfo::InternalViewInfo(const InternalViewInfo &rhs)
	: _type(rhs._type)
	, _parentHandle(nullptr)
{
	// no need to use this
	assert(false);
}

ff::MainWindow::InternalViewInfo::InternalViewInfo(InternalViewInfo &&rhs)
	: _type(rhs._type)
	, _views(std::move(rhs._views))
	, _viewOrder(std::move(rhs._viewOrder))
	, _parentHandle(rhs._parentHandle)
#if !METRO_APP
	, _parentWindow(std::move(rhs._parentWindow))
#endif
{
	rhs._type = ViewType::UNKNOWN;
	rhs._parentHandle = nullptr;
}

ff::MainWindow::InternalViewInfo &ff::MainWindow::InternalViewInfo::operator=(const InternalViewInfo &rhs)
{
	// no need to use this
	assertRetVal(false, *this);
}

#if METRO_APP

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Xaml::Media;

namespace ff
{
	ref class MainWindowEvents
	{
	internal:
		MainWindowEvents(ff::MainWindow *parent);

	public:
		virtual ~MainWindowEvents();

		void Destroy();

	private:
		void OnSuspending(Object ^sender, SuspendingEventArgs ^args);
		void OnResuming(Object ^sender, Object ^arg);
		void OnActivated(CoreWindow ^sender, WindowActivatedEventArgs ^args);
		void OnVisible(CoreWindow ^sender, VisibilityChangedEventArgs ^args);
		void OnClosed(CoreWindow ^sender, CoreWindowEventArgs ^args);
		void OnSizeChanged(CoreWindow ^sender, WindowSizeChangedEventArgs ^args);
		void OnRendering(Object ^sender, Object ^args);
		void OnLogicalDpiChanged(DisplayInformation ^displayInfo, Object ^sender);
		void UpdateWindow();

		MainWindow *_parent;
		DisplayInformation ^_displayInfo;
		Windows::Foundation::EventRegistrationToken _tokens[7];
		Windows::Foundation::EventRegistrationToken _renderingToken;
		bool _firstRender;
	};
}

ff::MainWindowEvents::MainWindowEvents(ff::MainWindow *parent)
	: _parent(parent)
	, _firstRender(true)
	, _displayInfo(DisplayInformation::GetForCurrentView())
{
	CoreWindow ^window = _parent->Handle();

	_tokens[0] = CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs ^>(this, &MainWindowEvents::OnSuspending);
	_tokens[1] = CoreApplication::Resuming += ref new EventHandler<Object ^>(this, &MainWindowEvents::OnResuming);
	_tokens[2] = window->Activated += ref new TypedEventHandler<CoreWindow ^, WindowActivatedEventArgs ^>(this, &MainWindowEvents::OnActivated);
	_tokens[3] = window->VisibilityChanged += ref new TypedEventHandler<CoreWindow ^, VisibilityChangedEventArgs ^>(this, &MainWindowEvents::OnVisible);
	_tokens[4] = window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &MainWindowEvents::OnClosed);
	_tokens[5] = window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &MainWindowEvents::OnSizeChanged);
	_tokens[6] = _displayInfo->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &MainWindowEvents::OnLogicalDpiChanged);
}

ff::MainWindowEvents::~MainWindowEvents()
{
	Destroy();
}

void ff::MainWindowEvents::Destroy()
{
	if (_parent)
	{
		CoreWindow ^window = _parent->Handle();

		CoreApplication::Suspending -= _tokens[0];
		CoreApplication::Resuming -= _tokens[1];
		window->Activated -= _tokens[2];
		window->VisibilityChanged -= _tokens[3];
		window->Closed -= _tokens[4];
		window->SizeChanged -= _tokens[5];
		_displayInfo->DpiChanged -= _tokens[6];

		_parent = nullptr;
		_displayInfo = nullptr;
	}

	if (_renderingToken.Value != 0)
	{
		CompositionTarget::Rendering -= _renderingToken;
		_renderingToken.Value = 0;
	}
}

void ff::MainWindowEvents::OnSuspending(Object ^sender, SuspendingEventArgs ^args)
{
	SuspendingDeferral ^deferral = args->SuspendingOperation->GetDeferral();

	ff::PostMainThreadFunction([=]
	{
		if (_parent)
		{
			_parent->OnSuspended(true);
		}

		deferral->Complete();
	});
}

void ff::MainWindowEvents::OnResuming(Object ^sender, Object ^arg)
{
	if (_parent)
	{
		_parent->OnSuspended(false);
	}
}

void ff::MainWindowEvents::OnActivated(CoreWindow ^sender, WindowActivatedEventArgs ^args)
{
	if (_parent)
	{
		_parent->OnActivate(args->WindowActivationState != CoreWindowActivationState::Deactivated);

		UpdateWindow();
	}
}

void ff::MainWindowEvents::OnVisible(CoreWindow ^sender, VisibilityChangedEventArgs ^args)
{
	if (_parent)
	{
		UpdateWindow();
	}
}

void ff::MainWindowEvents::OnClosed(CoreWindow ^sender, CoreWindowEventArgs ^args)
{
	if (_parent)
	{
		_parent->OnDestroy();
		Destroy();
	}
}

void ff::MainWindowEvents::OnSizeChanged(CoreWindow ^sender, WindowSizeChangedEventArgs ^args)
{
	if (_parent)
	{
		_parent->Layout();
	}
}

void ff::MainWindowEvents::OnRendering(Object ^sender, Object ^args)
{
	if (_parent && _parent->GetAppGlobals().RequestFrameUpdate(_parent, false))
	{
		if (_firstRender)
		{
			_firstRender = false;
			_parent->Activate();
		}
	}
}

void ff::MainWindowEvents::OnLogicalDpiChanged(DisplayInformation ^displayInfo, Object ^sender)
{
	if (_parent)
	{
		_parent->OnDpiChanged();
	}
}

void ff::MainWindowEvents::UpdateWindow()
{
	if (_parent)
	{
		if (_parent->IsVisible())
		{
			if (_renderingToken.Value == 0)
			{
				_renderingToken = CompositionTarget::Rendering += ref new EventHandler<Object ^>(this, &MainWindowEvents::OnRendering);
			}
		}
		else
		{
			if (_renderingToken.Value != 0)
			{
				CompositionTarget::Rendering -= _renderingToken;
				_renderingToken.Value = 0;
			}
		}

		ff::PostMainThreadFunction([this]
		{
			if (_parent && (!_parent->IsActive() || !_parent->IsVisible()))
			{
				// TODO: Trigger event to pause the game?
			}
		});
	}
}

#endif

// From AppGlobals.cpp
void LogWithTime(ff::Log &log, unsigned int id);

ff::MainWindow::MainWindow(AppGlobals &app, IMainWindowHost *host, IServiceProvider *contextServices)
	: _app(app)
	, _host(host)
	, _modal(0)
	, _dpi(DEFAULT_DPI, DEFAULT_DPI)
	, _dpiScaleDipToPixel(1.0, 1.0)
	, _active(ff::GetThisModule().IsMetroBuild())
	, _currentFocusViewType(ViewType::UNKNOWN)
	, _previousFocusViewType(ViewType::UNKNOWN)
	, _currentTabOrderDocumentView(nullptr)
	, _suspended(false)
	, _allowLayout(false)
	, _audioPaused(false)
	, _destroyed(false)
#if !METRO_APP
	, _previousFocusWindow(nullptr)
#endif
{
	verify(CreateServiceCollection(app.GetServices(), &_services));

	if (contextServices != nullptr)
	{
		_services->AddProvider(contextServices);
	}

#if !METRO_APP
	_documentViewTarget.SetListener(this);
#endif

	OnDpiChanged();
}

ff::MainWindow::~MainWindow()
{
#if METRO_APP
	if (_coreWindowEvents != nullptr)
	{
		_coreWindowEvents->Destroy();
		_coreWindowEvents = nullptr;
	}
#else
	_documentViewTarget.SetListener(nullptr);

	for (auto &i: _viewInfo)
	{
		i.GetEditableValue()._parentWindow.SetListener(nullptr);
	}
#endif

	if (_commandGroups)
	{
		_commandGroups->SetListener(nullptr);
	}
}

#if METRO_APP
Windows::UI::Core::CoreWindow ^ff::MainWindow::Handle() const
{
	return _coreWindow.Get();
}
#endif

bool ff::MainWindow::Create(PWND attachWindow)
{
	assertRetVal(_host != nullptr && Handle() == nullptr, false);

	PWND pwnd = attachWindow;
	_windowClassName = String::from_acp(typeid(*this).name() + 6); // skip "class "

#if METRO_APP
	if (pwnd == nullptr)
	{
		pwnd = Windows::UI::Core::CoreWindow::GetForCurrentThread();
	}

	assertRetVal(InitializeMetroWindow(pwnd), false);
#else
	_titleText = ComputeTitleText();

	if (pwnd == nullptr)
	{
		// Create a new window

		assertRetVal(CustomWindow::CreateClass(
			GetClassName(),
			GetClassStyle(),
			GetClassInstance(),
			GetClassCursor(),
			GetClassBackgroundBrush(),
			GetClassMenu(),
			GetClassIcon(::GetSystemMetrics(SM_CXICON)),
			GetClassIcon(::GetSystemMetrics(SM_CXSMICON))), false);

		assertRetVal(__super::Create(
			GetClassName(),
			GetDefaultText(),
			nullptr, // parent
			GetDefaultStyle(),
			GetDefaultExStyle(),
			0, 0, 0, 0, // size
			GetClassInstance(),
			GetDefaultMenu()), false);

		pwnd = _hwnd;
	}
	else
	{
		assertRetVal(::GetWindow(pwnd, GW_CHILD) == nullptr, false);
		assertRetVal(Attach(pwnd) && pwnd == _hwnd, false);
		::SetWindowText(pwnd, _titleText.c_str());
	}
#endif

	assertRetVal(InitializeWindowOptions(GetWindowOptions()), false);
	assertRetVal(InitializePosition(), false);
	assertRetVal((_commandRouter = CreateCommandRouter()) != nullptr, false);
	assertRetVal((_commandGroups = CreateCommandGroups()) != nullptr, false);
#if !METRO_APP
	assertRetVal(CreateDocumentViewTarget(_documentViewTarget), false);
#endif
	assertRetVal(InitializeAcceleratorTable(), false);
	assertRetVal(InitializeGraphics(), false);
	assertRetVal(InitializeRenderTarget(), false);
	assertRetVal(InitializeDepthBuffer(), false);
	assertRetVal(InitializeRender2d(), false);
	assertRetVal(InitializeKeyboard(), false);
	assertRetVal(InitializeMouse(), false);
	assertRetVal(InitializeTouch(), false);
	assertRetVal(InitializeJoysticks(), false);
	assertRetVal(InitializeAudio(), false);
	assertRetVal(Initialize(), false);
	assertRetVal(InitializeView(), false);
	assertRetVal(InitializeVisibility(), false);

	_host->OnWindowInitialized(*this);

	return true;
}

#if !METRO_APP

bool ff::MainWindow::ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT &nResult)
{
	if (hwnd == GetDocumentViewTarget())
	{
		std::shared_ptr<IViewWindowInternal> view = GetActiveViewInternal(ViewType::DOCUMENT);
		if (view != nullptr && view->ListenWindowProc(hwnd, msg, wParam, lParam, nResult))
		{
			return true;
		}

		switch (msg)
		{
		case WM_SETCURSOR:
			if (hwnd == (HWND)wParam && LOWORD(lParam) == HTCLIENT)
			{
				if (OnSetCursorViewTarget())
				{
					nResult = 1;
					return true;
				}
			}
			break;

		case WM_PAINT:
			if (OnPaintViewTarget())
			{
				nResult = 0;
				return true;
			}
			break;
		}
	}
	else
	{
		bool checkViews = false;

		switch (msg)
		{
		case WM_SIZE:
			checkViews = (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED);
			break;

		case WM_SHOWWINDOW:
			checkViews = (lParam == 0);
			break;

		case WM_DESTROY:
			checkViews = true;
			break;
		}

		if (checkViews)
		{
			for (auto &i: _viewInfo)
			{
				ff::ViewType type = i.GetKey();
				const InternalViewInfo &info = i.GetValue();

				if (info._parentHandle == hwnd)
				{
					if (msg == WM_DESTROY)
					{
						CloseAllViews(type, true);
					}
					else if (::GetParent(hwnd) == Handle())
					{
						Layout();
					}
					else
					{
						LayoutActiveViews(type);
					}

					break;
				}
			}
		}
	}

	return false;
}

bool ff::MainWindow::FilterMessage(MSG &msg)
{
	switch (msg.message)
	{
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (msg.wParam == VK_CONTROL)
		{
			ResetTabOrder();
		}
		break;
	}

	if (_acceleratorTable && ::TranslateAccelerator(Handle(), _acceleratorTable, &msg))
	{
		return true;
	}

	auto focusView = GetActiveViewInternal(_previousFocusViewType);
	if (focusView != nullptr)
	{
		if (focusView->GetAcceleratorTable() &&
			::TranslateAccelerator(Handle(), focusView->GetAcceleratorTable(), &msg))
		{
			return true;
		}

		if (focusView->FilterMessage(msg))
		{
			return true;
		}
	}

	// Try the rest of the views for ALT key presses
	switch (msg.message)
	{
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
		for (auto &i: _viewInfo)
		{
			ViewType type = i.GetKey();
			if (type != _previousFocusViewType)
			{
				auto view = GetActiveViewInternal(type);
				if (view != nullptr)
				{
					if (view->GetAcceleratorTable() &&
						::TranslateAccelerator(Handle(), view->GetAcceleratorTable(), &msg))
					{
						return true;
					}

					if (view->FilterMessage(msg))
					{
						return true;
					}
				}
			}
		}
		break;
	}

	return false;
}

LRESULT ff::MainWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT listenResult = 0;
	if (ListenWindowProc(hwnd, msg, wParam, lParam, listenResult))
	{
		return listenResult;
	}

	switch (msg)
	{
	case WM_ACTIVATE:
		OnActivate(LOWORD(wParam) != WA_INACTIVE);
		break;

	case WM_SETFOCUS:
		OnSetFocus();
		break;

	case WM_SIZE:
		OnSize(wParam);
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			Layout();
		}
		break;

	case WM_SHOWWINDOW:
		if (lParam == 0)
		{
			Layout();
		}
		break;

	case WM_ENTERMENULOOP:
	case WM_ENTERSIZEMOVE:
		PushModal();
		break;

	case WM_EXITMENULOOP:
	case WM_EXITSIZEMOVE:
		PopModal();
		break;

	case WM_ENABLE:
		OnEnable(wParam != FALSE);
		break;

	case WM_CREATE:
		_host->OnWindowCreated(*this);
		break;

	case WM_CLOSE:
		if (!OnClose())
		{
			return 0;
		}
		break;

	case WM_DESTROY:
		OnDestroy();
		break;

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case 0:
			if (ExecuteCommand(LOWORD(wParam)))
			{
				return 0;
			}
			break;

		case 1:
			if (OnAccelerator(LOWORD(wParam)))
			{
				return 0;
			}
			break;
		}
		break;

	case WM_POWERBROADCAST:
		switch (wParam)
		{
		case PBT_APMBATTERYLOW:
		case PBT_APMPOWERSTATUSCHANGE:
		case PBT_APMQUERYSUSPEND: // Don't want to prevent sleep
		case PBT_APMQUERYSUSPENDFAILED:
		case PBT_APMRESUMEAUTOMATIC: // No user interaction yet
			break;

		case PBT_APMRESUMECRITICAL: // Back from critical suspension (like very low power)
		case PBT_APMRESUMESUSPEND:  // We're running normal again
			OnSuspended(false);
			break;

		case PBT_APMSUSPEND:
			OnSuspended(true);
			break;
		}
		break;

	case WM_ENDSESSION:
		OnEndSession();
		break;

	case WM_QUERYENDSESSION:
		if (!(lParam & ENDSESSION_CRITICAL))
		{
			bool askUser = !(lParam & ENDSESSION_CLOSEAPP);
			if (!CanEndSession(askUser))
			{
				return FALSE;
			}
		}
		break;

	default:
		if (msg >= WM_APP && msg <= 0xBFFF)
		{
			if (msg == WM_APP_LAYOUT_NOW)
			{
				LayoutNow();
			}
			else if (msg == WM_APP_CHECK_FOCUS_NOW)
			{
				CheckViewFocusNow();
			}
			else if (msg == WM_APP_UPDATE_TITLE_NOW)
			{
				ff::String titleText = ComputeTitleText();

				if (titleText != _titleText)
				{
					_titleText = titleText;
					::SetWindowText(Handle(), _titleText.c_str());
					OnTitleTextChanged(_titleText);
				}
			}
			else if (msg == WM_APP_FRAME_UPDATE_NOW)
			{
				GetAppGlobals().RequestFrameUpdate(this, wParam != 0);
			}
			else if (msg == WM_APP_REFRESH_POSITION)
			{
				RefreshPosition();
			}
		}
		break;
	}

	return DoDefault(hwnd, msg, wParam, lParam);
}

void ff::MainWindow::OnSize(WPARAM wParam)
{
}

bool ff::MainWindow::CreateDocumentViewTarget(CustomWindow &sharedViewTarget)
{
	if (!UseFullScreenViews())
	{
		static StaticString name(L"Shared target");
		assertRetVal(sharedViewTarget.CreateBlank(name, Handle(), WS_CHILD), false);
	}

	return true;
}

bool ff::MainWindow::CreateViewParent(ViewType type, CustomWindow &window)
{
	if (type == ViewType::DOCUMENT)
	{
		if (!UseFullScreenViews())
		{
			static StaticString name(L"Document parent");
			assertRetVal(window.CreateBlank(name, Handle(), WS_CHILD | WS_VISIBLE), false);
		}

		return true;
	}

	assertRetVal(false, false);
}

void ff::MainWindow::SavePosition()
{
	assertRet(Handle());

	bool fullScreen = IsFullScreen();
	GetWindowOptions().SetBool(OPTION_WINDOW_FULL_SCREEN, fullScreen);

	if (fullScreen)
	{
		RectInt rect = GetDefaultWindowPosition();
		GetWindowOptions().SetBool(OPTION_WINDOW_MAXIMIZED, false);
		GetWindowOptions().SetRect(OPTION_WINDOW_POSITION, rect);
	}
	else
	{
		WINDOWPLACEMENT wp;
		ZeroObject(wp);
		wp.length = sizeof(wp);

		if (GetWindowPlacement(Handle(), &wp))
		{
			GetWindowOptions().SetBool(OPTION_WINDOW_MAXIMIZED,
				(wp.showCmd == SW_SHOWMAXIMIZED) ||
				(wp.showCmd == SW_SHOWMINIMIZED && (wp.flags & WPF_RESTORETOMAXIMIZED)));

			GetWindowOptions().SetRect(OPTION_WINDOW_POSITION, RectInt(wp.rcNormalPosition));
		}
	}
}

ff::RectInt ff::MainWindow::GetDefaultWindowPosition() const
{
	RectInt workArea = GetWorkArea(Handle());
	RectInt rect(0, 0, workArea.Width() * 3 / 4, workArea.Height() * 3 / 4);
	rect.CenterWithin(workArea);

	return rect;
}

ff::String ff::MainWindow::GetTitleText() const
{
	return _titleText;
}

void ff::MainWindow::UpdateWindowTitleText()
{
	ff::PostMessageOnce(Handle(), WM_APP_UPDATE_TITLE_NOW, 0, 0);
}

#endif // !METRO_APP

void ff::MainWindow::Cleanup()
{
}

bool ff::MainWindow::Initialize()
{
	return true;
}

bool ff::MainWindow::InitializeView()
{
	std::shared_ptr<IViewWindow> view = OpenNewView(ViewType::DOCUMENT, GetEmptyString(), nullptr);
	assertRetVal(view != nullptr, false);
	assertRetVal(ActivateView(view.get()), false);

	return true;
}

bool ff::MainWindow::InitializeWindowOptions(Dict &dict)
{
	return true;
}

bool ff::MainWindow::InitializePosition()
{
#if !METRO_APP
	if (ff::IsWindowVisibleStyle(Handle()))
	{
		// already in place
		return true;
	}

	// Save the position of the last window of the same type. That can be used as a good starting position.
	bool firstWindow = true;
	for (size_t i = PreviousSize(GetAppGlobals().GetWindowCount()); i != INVALID_SIZE; i = PreviousSize(i))
	{
		IMainWindow *otherWindow = GetAppGlobals().GetWindow(i).get();
		if (typeid(*otherWindow) == typeid(*this))
		{
			MainWindow *otherMainWindow = dynamic_cast<MainWindow *>(otherWindow);
			otherMainWindow->SavePosition();
			firstWindow = false;
			break;
		}
	}

	const PointInt minSize(64, 64);
	RectInt windowRect = GetWindowOptions().GetRect(OPTION_WINDOW_POSITION);
	if (windowRect.IsEmpty())
	{
		RectInt overscan = GetWindowOptions().GetRect(OPTION_WINDOW_PADDING);
		PointInt tempSize = GetWindowOptions().GetPoint(OPTION_WINDOW_DEFAULT_CLIENT_SIZE);
		RectInt defArea = GetWorkArea(nullptr);

		if (tempSize.x < minSize.x && tempSize.y < minSize.y)
		{
			tempSize.SetPoint(defArea.Width() * 3 / 4, defArea.Height() * 3 / 4);
		}

		RectInt tempRect(0, 0,
			tempSize.x + overscan.left + overscan.right,
			tempSize.y + overscan.top  + overscan.bottom);

		PointInt size = WindowSizeFromClientSize(Handle(), tempRect.Size());

		windowRect.SetRect(PointInt(0, 0), size);
		windowRect.CenterWithin(defArea);
	}

	// Make sure the window rect fits within the work area
	RectInt workArea = GetWorkArea(windowRect);

	windowRect.right = std::max(windowRect.right, windowRect.left + minSize.x);
	windowRect.right = std::min(windowRect.right, windowRect.left + workArea.Width());

	windowRect.bottom = std::max(windowRect.bottom, windowRect.top + minSize.y);
	windowRect.bottom = std::min(windowRect.bottom, windowRect.top + workArea.Height());

	if (!firstWindow)
	{
		int offset = ::GetSystemMetrics(SM_CYCAPTION);
		windowRect.Offset(offset, offset);
	}

	if (!windowRect.IsInside(workArea))
	{
		windowRect.CenterWithin(workArea);
	}

	MoveWindow(Handle(), windowRect);
#endif

	return true;
}

bool ff::MainWindow::InitializeVisibility()
{
#if METRO_APP
	// Might need to delay showing the window until the first frame is rendered
	Activate();
#else
	_allowLayout = true;

	if (!ff::IsWindowVisibleStyle(Handle()))
	{
		bool fullScreen = !IsFullWindowTarget() && GetWindowOptions().GetBool(OPTION_WINDOW_FULL_SCREEN);
		bool maximized = !fullScreen && GetWindowOptions().GetBool(OPTION_WINDOW_MAXIMIZED);

		if (fullScreen)
		{
			SetFullScreen(true);
		}

		::ShowWindow(Handle(), maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
	}
	else
	{
		_active = (::GetActiveWindow() == Handle());
		Layout();
	}

	::SetForegroundWindow(Handle());
#endif

	return true;
}

bool ff::MainWindow::InitializeAcceleratorTable()
{
#if !METRO_APP
	UINT id = GetAcceleratorTable();
	if (id)
	{
		_acceleratorTable = ::LoadAccelerators(GetClassInstance(), MAKEINTRESOURCE(id));
		return _acceleratorTable != nullptr;
	}
#endif

	return true;
}

bool ff::MainWindow::InitializeGraphics()
{
	if (GetWindowOptions().GetBool(OPTION_APP_USE_DIRECT3D))
	{
		LogWithTime(GetAppGlobals().GetLog(), IDS_APP_INIT_DIRECT3D);

		ComPtr<IDXGIAdapterX> card;
#if !METRO_APP
		if (!GetProcessGlobals().GetGraphicFactory()->GetAdapterForWindow(Handle(), &card, nullptr))
		{
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_DIRECT3D);
			return false;
		}
#endif
		if (!GetProcessGlobals().GetGraphicFactory()->CreateDevice(card, &_graph))
		{
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_DIRECT3D);
			return false;
		}
	}

	return true;
}

bool ff::MainWindow::InitializeRenderTarget()
{
	if (GetGraph() && GetWindowOptions().GetBool(OPTION_APP_USE_RENDER_MAIN_WINDOW))
	{
		LogWithTime(GetAppGlobals().GetLog(), IDS_APP_INIT_RENDER_TARGET);

		size_t backBuffers = GetWindowOptions().GetInt(OPTION_GRAPH_BACK_BUFFERS);
		size_t multiSamples = GetWindowOptions().GetInt(OPTION_GRAPH_MULTI_SAMPLES);

		bool fullScreen = IsFullWindowTarget() && GetWindowOptions().GetBool(OPTION_WINDOW_FULL_SCREEN);

		if (!CreateRenderTargetWindow(GetGraph(), GetDocumentViewTarget(), fullScreen,
			DXGI_FORMAT_UNKNOWN, backBuffers, multiSamples, &_renderTarget))
		{
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_RENDER_TARGET);
			return false;
		}
	}

	return true;
}

bool ff::MainWindow::InitializeDepthBuffer()
{
	if (GetGraph() && GetWindowOptions().GetBool(OPTION_APP_USE_RENDER_DEPTH))
	{
		LogWithTime(GetAppGlobals().GetLog(), IDS_APP_INIT_DEPTH_BUFFER);

		size_t multiSamples = GetWindowOptions().GetInt(OPTION_GRAPH_MULTI_SAMPLES);
		PointInt size = GetTarget() ? GetTarget()->GetSize() : GetClientRect(GetDocumentViewTarget()).Size();

		if (!CreateRenderDepth(GetGraph(), size, DXGI_FORMAT_UNKNOWN, multiSamples, &_renderDepth))
		{
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_DEPTH_BUFFER);
			return false;
		}
	}

	if (GetTarget() && GetDepth())
	{
		GetTarget()->SetDepth(GetDepth());
	}

	return true;
}

bool ff::MainWindow::InitializeRender2d()
{
	if (GetGraph() && GetWindowOptions().GetBool(OPTION_APP_USE_RENDER_2D))
	{
		LogWithTime(GetAppGlobals().GetLog(), IDS_APP_INIT_RENDER_2D);

		if (!Create2dRenderer(GetGraph(), &_render2d) ||
			!CreateDefault2dEffect(GetGraph(), &_effect2d))
		{
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_RENDER_2D);
			return false;
		}
	}

	return true;
}

bool ff::MainWindow::InitializeKeyboard()
{
	if (GetWindowOptions().GetBool(OPTION_APP_USE_MAIN_WINDOW_KEYBOARD))
	{
		LogWithTime(GetAppGlobals().GetLog(), IDS_APP_INIT_KEYBOARD);

		if (!CreateKeyboardDevice(GetDocumentViewTarget(), &_keyboard))
		{
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_KEYBOARD);
			return false;
		}
	}

	return true;
}

bool ff::MainWindow::InitializeMouse()
{
	if (GetWindowOptions().GetBool(OPTION_APP_USE_MAIN_WINDOW_MOUSE))
	{
		LogWithTime(GetAppGlobals().GetLog(), IDS_APP_INIT_MOUSE);

		if (!CreateMouseDevice(GetDocumentViewTarget(), &_mouse))
		{
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_MOUSE);
			return false;
		}
	}

	return true;
}

bool ff::MainWindow::InitializeTouch()
{
	if (GetWindowOptions().GetBool(OPTION_APP_USE_MAIN_WINDOW_TOUCH))
	{
		LogWithTime(GetAppGlobals().GetLog(), IDS_APP_INIT_TOUCH);

		if (!CreateTouchDevice(GetDocumentViewTarget(), &_touch))
		{
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_TOUCH);
			return false;
		}
	}

	return true;
}

bool ff::MainWindow::InitializeJoysticks()
{
	if (GetWindowOptions().GetBool(OPTION_APP_USE_JOYSTICKS))
	{
		LogWithTime(GetAppGlobals().GetLog(), IDS_APP_INIT_JOYSTICKS);

		bool allowXInput = GetWindowOptions().GetBool(OPTION_APP_USE_XINPUT);

		if (!CreateJoystickInput(GetDocumentViewTarget(), allowXInput, &_joysticks))
		{
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_DIRECTINPUT);
			return false;
		}
	}

	return true;
}

bool ff::MainWindow::InitializeAudio()
{
	if (GetWindowOptions().GetBool(OPTION_APP_USE_XAUDIO))
	{
		LogWithTime(GetAppGlobals().GetLog(), IDS_APP_INIT_AUDIO);

		if (!GetProcessGlobals().GetAudioFactory()->CreateDefaultDevice(&_audio))
		{
			// Not a fatal error
			LogWithTime(GetAppGlobals().GetLog(), IDS_ERR_AUDIO);
		}
	}

	if (GetAudio())
	{
		float masterVolume = GetWindowOptions().GetFloat(OPTION_SOUND_MASTER_VOLUME);
		float effectsVolume = GetWindowOptions().GetFloat(OPTION_SOUND_EFFECTS_VOLUME);
		float musicVolume = GetWindowOptions().GetFloat(OPTION_SOUND_MUSIC_VOLUME);

		GetAudio()->SetVolume(ff::AudioVoiceType::MASTER, masterVolume);
		GetAudio()->SetVolume(ff::AudioVoiceType::EFFECTS, effectsVolume);
		GetAudio()->SetVolume(ff::AudioVoiceType::MUSIC, musicVolume);
	}

	return true;
}

ff::ComPtr<ff::ICommandRouter> ff::MainWindow::CreateCommandRouter()
{
	ComPtr<ICommandRouter> router;
	assertRetVal(ff::CreateNullCommandRouter(&router), ComPtr<ICommandRouter>());

	return router;
}

ff::ComPtr<ff::ICommandGroups> ff::MainWindow::CreateCommandGroups()
{
	ComPtr<ICommandGroups> groups;
	assertRetVal(ff::CreateCommandGroups(this, &groups), ComPtr<ICommandGroups>());
	assertRetVal(InitializeCommandGroups(groups), ComPtr<ICommandGroups>());

	return groups;
}

bool ff::MainWindow::InitializeCommandGroups(ICommandGroups *groups)
{
	groups->InvalidateAll();
	return true;
}

ff::String ff::MainWindow::GetClassName() const
{
	assert(!_windowClassName.empty());
	return _windowClassName;
}

#if METRO_APP

bool ff::MainWindow::InitializeMetroWindow(PWND attachWindow)
{
	assertRetVal(_coreWindow == nullptr, false);

	_coreWindow = attachWindow;
	_coreWindowEvents = ref new MainWindowEvents(this);

	PointerVisualizationSettings ^pointerSettings = PointerVisualizationSettings::GetForCurrentView();
	pointerSettings->IsBarrelButtonFeedbackEnabled = false;
	pointerSettings->IsContactFeedbackEnabled = false;

	return true;
}

#else

HINSTANCE ff::MainWindow::GetClassInstance() const
{
	return GetThisModule().GetInstance();
}

DWORD ff::MainWindow::GetClassStyle() const
{
	return CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
}

HCURSOR ff::MainWindow::GetClassCursor() const
{
	return ::LoadCursor(nullptr, IDC_ARROW);
}

HBRUSH ff::MainWindow::GetClassBackgroundBrush() const
{
	return GetStockBrush(NULL_BRUSH);
}

UINT ff::MainWindow::GetClassMenu() const
{
	return 0;
}

HICON ff::MainWindow::GetClassIcon(int size) const
{
	return nullptr;
}

ff::String ff::MainWindow::GetDefaultText() const
{
	return _titleText;
}

DWORD ff::MainWindow::GetDefaultStyle() const
{
	return WS_OVERLAPPEDWINDOW;
}

DWORD ff::MainWindow::GetDefaultExStyle() const
{
	return 0;
}

HMENU ff::MainWindow::GetDefaultMenu() const
{
	return nullptr;
}

UINT ff::MainWindow::GetAcceleratorTable() const
{
	return 0;
}

ff::String ff::MainWindow::ComputeTitleText()
{
	return _app.GetAppName();
}

void ff::MainWindow::OnTitleTextChanged(ff::String value)
{
}

HCURSOR ff::MainWindow::GetCursorViewTarget() const
{
	return ::LoadCursor(nullptr, IDC_ARROW);
}

bool ff::MainWindow::UseFullScreenViews() const
{
	return false;
}

bool ff::MainWindow::AllowFullScreen() const
{
	return true;
}

bool ff::MainWindow::UseCursorFullScreen() const
{
	return false;
}

bool ff::MainWindow::UseCursorWindowed() const
{
	return !UseFullScreenViews();
}

void ff::MainWindow::LayoutNow()
{
	assertRet(IsValid());
	noAssertRet(_allowLayout);

	_allowLayout = false;

	ff::PointInt size = ff::GetClientRect(Handle()).Size();
	ff::RectInt area(0, 0, size.x, size.y);

	HDWP defer = ::BeginDeferWindowPos(8);
	HWND topHandle = GetViewParent(ff::ViewType::DOCKED_TOP, true);
	HWND bottomHandle = GetViewParent(ff::ViewType::DOCKED_BOTTOM, true);
	HWND leftHandle = GetViewParent(ff::ViewType::DOCKED_LEFT, true);
	HWND rightHandle = GetViewParent(ff::ViewType::DOCKED_RIGHT, true);
	HWND tabsHandle = GetViewParent(ff::ViewType::DOCUMENT_TABS, true);
	HWND toolbarHandle = GetViewParent(ff::ViewType::DOCUMENT_TOOLBAR, true);
	HWND statusHandle = GetViewParent(ff::ViewType::DOCUMENT_STATUS, true);
	HWND documentHandle = GetViewParent(ff::ViewType::DOCUMENT, true);

	if (topHandle)
	{
		int height = ff::GetWindowRect(topHandle).Height();
		ff::RectInt rect(0, area.top, size.x, area.top + height);
		ff::DeferMoveWindow(defer, topHandle, rect);

		area.top += height;
		area.EnsurePositiveSize();
	}

	if (bottomHandle)
	{
		int height = ff::GetWindowRect(bottomHandle).Height();
		int top = std::max(area.top, area.bottom - height);
		ff::RectInt rect(0, top, size.x, top + height);
		ff::DeferMoveWindow(defer, bottomHandle, rect);

		area.bottom = top;
	}

	if (leftHandle)
	{
		int width = ff::GetWindowRect(leftHandle).Width();
		ff::RectInt rect(0, area.top, width, area.bottom);
		ff::DeferMoveWindow(defer, leftHandle, rect);

		area.left += width;
		area.EnsurePositiveSize();
	}

	if (rightHandle)
	{
		int width = ff::GetWindowRect(rightHandle).Width();
		int left = std::max(area.left, area.right - width);
		ff::RectInt rect(left, area.top, left + width, area.bottom);
		ff::DeferMoveWindow(defer, rightHandle, rect);

		area.right = left;
	}

	if (tabsHandle)
	{
		int height = ff::GetWindowRect(tabsHandle).Height();
		ff::RectInt rect(area.left, area.top, area.right, area.top + height);
		ff::DeferMoveWindow(defer, tabsHandle, rect);

		area.top += height;
		area.EnsurePositiveSize();
	}

	if (toolbarHandle)
	{
		int height = ff::GetWindowRect(toolbarHandle).Height();
		ff::RectInt rect(area.left, area.top, area.right, area.top + height);
		ff::DeferMoveWindow(defer, toolbarHandle, rect);

		area.top += height;
		area.EnsurePositiveSize();
	}

	if (statusHandle)
	{
		int height = ff::GetWindowRect(statusHandle).Height();
		ff::RectInt rect(area.left, area.bottom - height, area.right, area.bottom);
		ff::DeferMoveWindow(defer, statusHandle, rect);

		area.bottom -= height;
		area.EnsurePositiveSize();
	}

	if (documentHandle)
	{
		ff::DeferMoveWindow(defer, documentHandle, area);

		area.left = area.right;
		area.top = area.bottom;
	}

	::EndDeferWindowPos(defer);

	if (topHandle)
	{
		LayoutActiveViews(ViewType::DOCKED_TOP);
	}

	if (bottomHandle)
	{
		LayoutActiveViews(ViewType::DOCKED_BOTTOM);
	}

	if (leftHandle)
	{
		LayoutActiveViews(ViewType::DOCKED_LEFT);
	}

	if (rightHandle)
	{
		LayoutActiveViews(ViewType::DOCKED_RIGHT);
	}

	if (tabsHandle)
	{
		LayoutActiveViews(ViewType::DOCUMENT_TABS);
	}

	if (toolbarHandle)
	{
		LayoutActiveViews(ViewType::DOCUMENT_TOOLBAR);
	}

	if (statusHandle)
	{
		LayoutActiveViews(ViewType::DOCUMENT_STATUS);
	}

	if (documentHandle || IsFullWindowTarget())
	{
		LayoutActiveViews(ViewType::DOCUMENT);
	}

	std::shared_ptr<ff::IViewWindowInternal> activeView = GetActiveViewInternal(ViewType::DOCUMENT);
	if (activeView == nullptr)
	{
		EnsureWindowParent(GetDocumentViewTarget(), GetDocumentViewParent());
		SetWindowPos(GetDocumentViewTarget(), HWND_TOP, ff::GetClientRect(GetDocumentViewParent()), SWP_SHOWWINDOW);
	}

	_allowLayout = true;
}

void ff::MainWindow::LayoutActiveViews(ViewType type)
{
	InternalViewInfo *info = GetInternalViewInfo(type);
	if (info != nullptr)
	{
		for (auto &view: info->_views)
		{
			if (view->IsActive())
			{
				view->Layout();
			}
		}
	}
}

#endif

ff::ViewType ff::MainWindow::FindViewTypeForWindow(PWND window) const
{
#if METRO_APP
	return ViewType::DOCUMENT;
#else
	ViewType type = ViewType::UNKNOWN;

	if (window)
	{
		HWND parent = nullptr;

		for (auto &i: _viewInfo)
		{
			const InternalViewInfo &info = i.GetValue();

			if (info._parentHandle != nullptr && ::IsChild(info._parentHandle, window))
			{
				if (parent == nullptr || ::IsChild(parent, info._parentHandle))
				{
					parent = info._parentHandle;
					type = info._type;
				}
			}
		}
	}

	return type;
#endif
}

bool ff::MainWindow::IsValid() const
{
	return Handle() != nullptr;
}

bool ff::MainWindow::IsActive() const
{
	return _active && IsValid();
}

bool ff::MainWindow::IsVisible() const
{
	return IsValid() && GetTarget() && !GetTarget()->IsHidden();
}

bool ff::MainWindow::IsModal() const
{
#if !METRO_APP
	if (_hwnd)
	{
		if (_modal)
		{
			return true;
		}

		HWND hCapture = ::GetCapture();
		if (hCapture && !::IsChild(_hwnd, hCapture))
		{
			return true;
		}
	}
#endif

	return false;
}

void ff::MainWindow::PushModal()
{
	if (++_modal == 1)
	{
		_host->OnWindowModalStart(*this);
	}
}

void ff::MainWindow::PopModal()
{
	if (_modal > 0 && !--_modal)
	{
		_host->OnWindowModalEnd(*this);
	}
}

void ff::MainWindow::CancelModal()
{
	if (_modal > 0)
	{
		_modal = 0;
		_host->OnWindowModalEnd(*this);
	}
}

ff::AppGlobals &ff::MainWindow::GetAppGlobals() const
{
	return _app;
}

ff::ICommandRouter *ff::MainWindow::GetCommandRouter() const
{
	return _commandRouter;
}

ff::IServiceProvider *ff::MainWindow::GetServices() const
{
	return _services;
}

ff::IServiceCollection *ff::MainWindow::GetServiceCollection() const
{
	return _services;
}

ff::PWND ff::MainWindow::GetHandle() const
{
	return Handle();
}

ff::IGraphDevice *ff::MainWindow::GetWindowGraph() const
{
	return _graph;
}

ff::IAudioDevice *ff::MainWindow::GetWindowAudio() const
{
	return _audio;
}

ff::Dict &ff::MainWindow::GetWindowOptions() const
{
	return GetAppGlobals().GetNamedOptions(GetClassName());
}

ff::ProcessGlobals &ff::MainWindow::GetProcessGlobals() const
{
	return _app.GetProcessGlobals();
}

ff::ICommandGroups *ff::MainWindow::GetCommandGroups() const
{
	return _commandGroups;
}

ff::IRenderTargetWindow *ff::MainWindow::GetTarget() const
{
	return _renderTarget;
}

ff::IRenderDepth *ff::MainWindow::GetDepth() const
{
	return _renderDepth;
}

ff::IGraphDevice *ff::MainWindow::GetGraph() const
{
	return _graph;
}

ff::IAudioDevice *ff::MainWindow::GetAudio() const
{
	return _audio;
}

ff::IMouseDevice *ff::MainWindow::GetMouse() const
{
	return _mouse;
}

ff::ITouchDevice *ff::MainWindow::GetTouch() const
{
	return _touch;
}

ff::IKeyboardDevice *ff::MainWindow::GetKeys() const
{
	return _keyboard;
}

ff::IJoystickInput *ff::MainWindow::GetJoysticks() const
{
	return _joysticks;
}

ff::I2dRenderer *ff::MainWindow::Get2dRender() const
{
	return _render2d;
}

ff::I2dEffect *ff::MainWindow::Get2dEffect() const
{
	return _effect2d;
}

size_t ff::MainWindow::GetViewCount(ViewType type) const
{
	const InternalViewInfo *info = GetInternalViewInfo(type);
	return info ? info->_views.Size() : 0;
}

std::shared_ptr<ff::IViewWindow> ff::MainWindow::GetView(ViewType type, size_t index) const
{
	const InternalViewInfo *info = GetInternalViewInfo(type);
	assertRetVal(info && index < info->_views.Size(), std::shared_ptr<IViewWindow>());
	return info->_views[index];
}

std::shared_ptr<ff::IViewWindow> ff::MainWindow::GetActiveView(ViewType type) const
{
	return GetActiveViewInternal(type);
}

ff::Vector<std::shared_ptr<ff::IViewWindow>> ff::MainWindow::GetOrderedViews(ViewType type) const
{
	Vector<std::shared_ptr<IViewWindow>> views;
	const InternalViewInfo *info = GetInternalViewInfo(type);

	if (info != nullptr)
	{
		views.Reserve(info->_viewOrder.Size());

		for (auto &view: info->_viewOrder)
		{
			views.Push(view);
		}
	}

	return views;
}

std::shared_ptr<ff::IViewWindow> ff::MainWindow::OpenNewView(ViewType type, StringRef typeName, IServiceProvider *contextServices)
{
	std::shared_ptr<IViewWindow> view;
	PWND parent = GetViewParent(type);
	assertRetVal(parent != nullptr, view);

	view = OpenNewView(parent, type, typeName, contextServices);
	return view;
}

std::shared_ptr<ff::IViewWindow> ff::MainWindow::FindSingletonView(ViewType type, REFGUID id) const
{
	const InternalViewInfo *info = GetInternalViewInfo(type);
	if (info != nullptr && id != GUID_NULL)
	{
		for (auto &view: info->_views)
		{
			if (view->GetSingletonId() == id)
			{
				return view;
			}
		}
	}

	return std::shared_ptr<IViewWindow>();
}

bool ff::MainWindow::ActivateView(IViewWindow *view)
{
	return ActivateView(view, false);
}

bool ff::MainWindow::ActivateView(IViewWindow *view, bool tabOrder)
{
	assertRetVal(view && view->GetMainWindow() == this, false);
	bool result = false;

	ViewType type = view->GetViewType();
	InternalViewInfo *info = GetInternalViewInfo(type);
	assertRetVal(info, false);

	std::shared_ptr<IViewWindowInternal> activeView = GetActiveViewInternal(type);
	if (activeView.get() == view)
	{
		// already active
		if (type == ViewType::DOCUMENT)
		{
			activeView->SetFocus();
		}

		result = true;
	}
	else
	{
		for (auto &internalView: info->_viewOrder)
		{
			if (internalView.get() == view)
			{
				if (activeView == nullptr || activeView->Deactivate(true))
				{
					if (activeView != nullptr)
					{
						OnViewDeactivated(activeView.get());
					}
#if !METRO_APP
					if (::GetParent(GetDocumentViewTarget()) == GetDocumentViewParent())
					{
						SetWindowPos(GetDocumentViewTarget(), HWND_TOP, RectInt(0, 0, 0, 0), SWP_HIDEWINDOW);
					}
#endif
					if (!tabOrder)
					{
						info->_viewOrder.MoveToFront(internalView);
					}

					internalView->Activate();

					if (type == ViewType::DOCUMENT)
					{
						internalView->SetFocus();
					}

					result = true;

					OnViewActivated(internalView.get());
				}

				break;
			}
		}
	}

	return result;
}

void ff::MainWindow::ResetTabOrder()
{
	if (_currentTabOrderDocumentView != nullptr)
	{
		InternalViewInfo *info = GetInternalViewInfo(ViewType::DOCUMENT);
		assertRet(info);

		info->_viewOrder.MoveToFront(*_currentTabOrderDocumentView);
		_currentTabOrderDocumentView = nullptr;
	}
}

void ff::MainWindow::OnCloseTab()
{
	auto activeView = GetActiveViewInternal(ViewType::DOCUMENT);
	if (activeView != nullptr && activeView->IsCloseable())
	{
		CloseView(activeView.get());
	}
}

void ff::MainWindow::OnNextTab()
{
	const InternalViewInfo *info = GetInternalViewInfo(ViewType::DOCUMENT);
	noAssertRet(info);

	if (_currentTabOrderDocumentView == nullptr)
	{
		_currentTabOrderDocumentView = info->_viewOrder.GetFirst();
	}

	if (_currentTabOrderDocumentView != nullptr)
	{
		_currentTabOrderDocumentView = info->_viewOrder.GetNext(*_currentTabOrderDocumentView);
	}

	if (_currentTabOrderDocumentView == nullptr)
	{
		_currentTabOrderDocumentView = info->_viewOrder.GetFirst();
	}

	if (_currentTabOrderDocumentView != nullptr)
	{
		ActivateView(_currentTabOrderDocumentView->get(), true);
	}
}

void ff::MainWindow::OnPrevTab()
{
	const InternalViewInfo *info = GetInternalViewInfo(ViewType::DOCUMENT);
	noAssertRet(info);

	if (_currentTabOrderDocumentView == nullptr)
	{
		_currentTabOrderDocumentView = info->_viewOrder.GetLast();
	}

	if (_currentTabOrderDocumentView != nullptr)
	{
		_currentTabOrderDocumentView = info->_viewOrder.GetPrev(*_currentTabOrderDocumentView);
	}

	if (_currentTabOrderDocumentView == nullptr)
	{
		_currentTabOrderDocumentView = info->_viewOrder.GetLast();
	}

	if (_currentTabOrderDocumentView != nullptr)
	{
		ActivateView(_currentTabOrderDocumentView->get(), true);
	}
}

bool ff::MainWindow::CloseView(IViewWindow *view, bool forceClose)
{
	ResetTabOrder();

	assertRetVal(view && view->GetMainWindow() == this, false);
	bool result = false;
	bool wasActive = false;

	ViewType type = view->GetViewType();
	InternalViewInfo *info = GetInternalViewInfo(type);
	assertRetVal(info, false);

	std::shared_ptr<IViewWindowInternal> activeView = GetActiveViewInternal(type);
	for (auto &internalView: info->_views)
	{
		if (internalView.get() == view)
		{
			if (!forceClose && !internalView->CanClose())
			{
				return false;
			}

			if (internalView->IsActive())
			{
				if (!internalView->Deactivate(false))
				{
					return false;
				}

				OnViewDeactivated(internalView.get());
				wasActive = true;
			}

			internalView->Close();
			info->_views.DeleteItem(internalView);
			result = true;
			break;
		}
	}

	if (result)
	{
		for (auto &internalView: info->_viewOrder)
		{
			if (internalView.get() == view)
			{
				auto sharedInternalView = internalView;
				info->_viewOrder.Delete(internalView);
				OnViewClosed(sharedInternalView.get());
				break;
			}
		}

		if (wasActive)
		{
			activeView = GetActiveViewInternal(type);
			if (activeView == nullptr && !info->_viewOrder.IsEmpty())
			{
				verify(ActivateView(info->_viewOrder.GetFirst()->get()));
			}
		}

		Layout();
	}

	return result;
}

bool ff::MainWindow::CloseAllViews(ViewType type, bool forceClose)
{
	ResetTabOrder();

	InternalViewList allViews;
	for (auto &i: _viewInfo)
	{
		if (i.GetKey() == type || type == ViewType::VIEW_TYPE_COUNT)
		{
			for (auto &view: i.GetValue()._views)
			{
				if (!forceClose && !view->CanClose())
				{
					return false;
				}

				allViews.Insert(view);
			}

			i.GetEditableValue()._viewOrder.Clear();
			i.GetEditableValue()._views.Clear();
		}
	}

	for (auto &view: allViews)
	{
		if (view->IsValid() && view->IsActive())
		{
			verify(view->Deactivate(false));
			OnViewDeactivated(view.get());
		}
	}

	for (auto &view: allViews)
	{
		if (view->IsValid())
		{
			view->Close();
			OnViewClosed(view.get());
		}
	}

	Layout();

	return true;
}

bool ff::MainWindow::MoveView(IViewWindow *view, size_t index)
{
	ResetTabOrder();

	assertRetVal(view && view->GetMainWindow() == this, false);

	ViewType type = view->GetViewType();
	InternalViewInfo *info = GetInternalViewInfo(type);
	assertRetVal(info, false);

	size_t i = 0;
	for (std::shared_ptr<IViewWindowInternal> internalView: info->_views)
	{
		if (internalView.get() == view)
		{
			size_t index2 = index;
			if (index2 > i)
			{
				index2--;
			}

			info->_views.Delete(i);
			info->_views.Insert(index2, internalView);

			OnViewMoved(internalView.get(), i, index2);
			break;
		}

		i++;
	}

	return false;
}

#if !METRO_APP
void ff::MainWindow::HideSharedViewTarget()
{
	if (GetDocumentViewParent() != GetDocumentViewTarget())
	{
		EnsureWindowParent(GetDocumentViewTarget(), GetDocumentViewParent());
		SetWindowPos(GetDocumentViewTarget(), HWND_TOP, RectInt(0, 0, 0, 0), SWP_HIDEWINDOW);
	}
}
#endif

#if !METRO_APP
void ff::MainWindow::ShowViews(ViewType type, bool show)
{
	InternalViewInfo *info = GetInternalViewInfo(type);
	noAssertRet(info);

	HWND hwnd = GetViewParent(type);
	assertRet(hwnd);

	ff::EnsureWindowVisible(hwnd, show);
}
#endif

ff::ViewType ff::MainWindow::GetCurrentFocusViewType() const
{
	return _currentFocusViewType;
}

ff::ViewType ff::MainWindow::GetPreviousFocusViewType() const
{
	return _previousFocusViewType;
}

ff::PWND ff::MainWindow::GetDocumentViewParent()
{
	return GetViewParent(ViewType::DOCUMENT);
}

ff::PWND ff::MainWindow::GetDocumentViewTarget() const
{
#if METRO_APP
	return Handle();
#else
	return _documentViewTarget.Handle() ? _documentViewTarget.Handle() : Handle();
#endif
}

bool ff::MainWindow::ToggleFullScreen()
{
	return SetFullScreen(!IsFullScreen());
}

bool ff::MainWindow::IsFullScreen() const
{
	bool fullScreen = false;

	if (IsFullWindowTarget())
	{
		fullScreen = GetTarget() && GetTarget()->IsFullScreen();
	}
#if !METRO_APP
	else if (Handle())
	{
		fullScreen = !ff::WindowHasStyle(Handle(), WS_OVERLAPPEDWINDOW);
	}
#endif

	return fullScreen;
}

bool ff::MainWindow::IsFullWindowTarget() const
{
	return GetDocumentViewTarget() == Handle();
}

bool ff::MainWindow::CanSetFullScreen() const
{
	bool canSet = false;

#if !METRO_APP
	if (AllowFullScreen())
#endif
	{
		if (IsFullWindowTarget())
		{
			canSet = true;
		}
#if !METRO_APP
		else if (Handle())
		{
			canSet = true;
		}
#endif
	}

	return canSet;
}

bool ff::MainWindow::SetFullScreen(bool fullScreen)
{
	if (!CanSetFullScreen())
	{
		return false;
	}

	bool status = false;
	bool oldFullScreen = IsFullScreen();

	if (IsFullWindowTarget())
	{
		status = GetTarget() && GetTarget()->SetFullScreen(fullScreen);
	}
#if !METRO_APP
	else if (fullScreen && !Activate())
	{
		status = false;
	}
	else
	{
		// Update the window style and position to match full/windowed screen
		LONG style = ::GetWindowLong(Handle(), GWL_STYLE);

		if (fullScreen)
		{
			style &= ~WS_OVERLAPPEDWINDOW;
		}
		else
		{
			style |= WS_OVERLAPPEDWINDOW;
		}

		::SetWindowLong(Handle(), GWL_STYLE, style);

		RectInt pos = fullScreen
			? ff::GetMonitorArea(Handle())
			: GetDefaultWindowPosition();

		bool topMost = fullScreen;
#ifdef _DEBUG
		topMost = !::IsDebuggerPresent();
#endif
		::SetWindowPos(Handle(),
			topMost ? HWND_TOPMOST : HWND_NOTOPMOST,
			pos.left, pos.top, pos.Width(), pos.Height(),
			SWP_FRAMECHANGED | (!topMost ? SWP_NOACTIVATE : 0));

		status = true;
	}
#endif

	if (status)
	{
		bool newFullScreen = IsFullScreen();
		if (oldFullScreen != newFullScreen)
		{
			OnSetFullScreen(newFullScreen);
		}
	}

	return status;
}

void ff::MainWindow::OnSetFullScreen(bool fullScreen)
{
}

ff::PointInt ff::MainWindow::DipToPixel(const PointInt &dip)
{
	return PointInt(
		::MulDiv(dip.x, _dpi.x, DEFAULT_DPI),
		::MulDiv(dip.y, _dpi.y, DEFAULT_DPI));
}

ff::PointDouble ff::MainWindow::DipToPixel(const PointDouble &dip)
{
	return PointDouble(
		dip.x * _dpiScaleDipToPixel.x,
		dip.y * _dpiScaleDipToPixel.y);
}

ff::PointInt ff::MainWindow::PixelToDip(const PointInt &pixel)
{
	return PointInt(
		::MulDiv(pixel.x, DEFAULT_DPI, _dpi.x),
		::MulDiv(pixel.y, DEFAULT_DPI, _dpi.y));
}

ff::PointDouble ff::MainWindow::PixelToDip(const PointDouble &pixel)
{
	return PointDouble(
		pixel.x / _dpiScaleDipToPixel.x,
		pixel.y / _dpiScaleDipToPixel.y);
}

ff::RectInt ff::MainWindow::DipToPixel(const RectInt &dip)
{
	return RectInt(
		::MulDiv(dip.left, _dpi.x, DEFAULT_DPI),
		::MulDiv(dip.top, _dpi.y, DEFAULT_DPI),
		::MulDiv(dip.right, _dpi.x, DEFAULT_DPI),
		::MulDiv(dip.bottom, _dpi.y, DEFAULT_DPI));
}

ff::RectDouble ff::MainWindow::DipToPixel(RectDouble &dip)
{
	return RectDouble(
		dip.left * _dpiScaleDipToPixel.x,
		dip.top * _dpiScaleDipToPixel.y,
		dip.right * _dpiScaleDipToPixel.x,
		dip.bottom * _dpiScaleDipToPixel.y);
}

ff::RectInt ff::MainWindow::PixelToDip(const RectInt &pixel)
{
	return RectInt(
		::MulDiv(pixel.left, DEFAULT_DPI, _dpi.x),
		::MulDiv(pixel.top, DEFAULT_DPI, _dpi.y),
		::MulDiv(pixel.right, DEFAULT_DPI, _dpi.x),
		::MulDiv(pixel.bottom, DEFAULT_DPI, _dpi.y));
}

ff::RectDouble ff::MainWindow::PixelToDip(const RectDouble &pixel)
{
	return RectDouble(
		pixel.left / _dpiScaleDipToPixel.x,
		pixel.top / _dpiScaleDipToPixel.y,
		pixel.right / _dpiScaleDipToPixel.x,
		pixel.bottom / _dpiScaleDipToPixel.y);
}

int ff::MainWindow::DipToPixelX(int dip)
{
	return ::MulDiv(dip, _dpi.x, DEFAULT_DPI);
}

int ff::MainWindow::DipToPixelY(int dip)
{
	return ::MulDiv(dip, _dpi.y, DEFAULT_DPI);
}

double ff::MainWindow::DipToPixelX(double dip)
{
	return dip * _dpiScaleDipToPixel.x;
}

double ff::MainWindow::DipToPixelY(double dip)
{
	return dip * _dpiScaleDipToPixel.y;
}

int ff::MainWindow::PixelToDipX(int pixel)
{
	return ::MulDiv(pixel, DEFAULT_DPI, _dpi.x);
}

int ff::MainWindow::PixelToDipY(int pixel)
{
	return ::MulDiv(pixel, DEFAULT_DPI, _dpi.y);
}

double ff::MainWindow::PixelToDipX(double pixel)
{
	return pixel / _dpiScaleDipToPixel.x;
}

double ff::MainWindow::PixelToDipY(double pixel)
{
	return pixel / _dpiScaleDipToPixel.y;
}

std::shared_ptr<ff::IViewWindowInternal> ff::MainWindow::OpenNewView(PWND parent, ViewType type, StringRef typeName, IServiceProvider *contextServices)
{
	std::shared_ptr<IViewWindowInternal> view;
	assertRetVal(parent != nullptr, view);

	InternalViewInfo *info = GetInternalViewInfo(type);
	assertRetVal(info, view);

	view = NewView(parent, type, typeName, contextServices);
	assertRetVal(view != nullptr, view);

	ResetTabOrder();

	info->_viewOrder.Insert(view);
	info->_views.Push(view);

	OnViewOpened(view.get());

	return view;
}

std::shared_ptr<ff::IViewWindowInternal> ff::MainWindow::NewView(PWND parent, ViewType type, StringRef typeName, IServiceProvider *contextServices)
{
	// This is just an empty default window
	std::shared_ptr<ViewWindow> view = std::make_shared<ViewWindow>(this, contextServices);
	assertRetVal(view != nullptr && view->Create(parent, type), std::shared_ptr<IViewWindowInternal>());

	return view;
}

void ff::MainWindow::CheckViewFocus()
{
#if METRO_APP
	CheckViewFocusNow();
#else
	ff::RepostMessageOnce(Handle(), WM_APP_CHECK_FOCUS_NOW, 0, 0);
#endif
}

void ff::MainWindow::CheckViewFocusNow()
{
#if METRO_APP
	PWND focusWindow = Handle();
#else
	PWND focusWindow = ::GetFocus();
#endif
	ViewType type = FindViewTypeForWindow(focusWindow);

	if (type == ViewType::DOCUMENT_TABS ||
		type == ViewType::DOCUMENT_TOOLBAR ||
		type == ViewType::DOCUMENT_STATUS)
	{
		type = ViewType::DOCUMENT;
	}

	if (type != _currentFocusViewType)
	{
		if (type != ViewType::UNKNOWN)
		{
			_previousFocusViewType = type;
		}

		ViewType previousType = _currentFocusViewType;
		_currentFocusViewType = type;
		OnFocusViewTypeChanged(previousType, type);
	}
}

void ff::MainWindow::RefreshPosition()
{
#if !METRO_APP
	// CRenderTargetWindow::DeviceTopWindowProc can deal with full window targets
	if (!IsFullWindowTarget() && IsFullScreen())
	{
		// Make sure the window is actually covering the full screen
		RectInt pos = ff::GetMonitorArea(Handle());
		if (ff::GetWindowRect(Handle()) != pos)
		{
			ff::MoveWindow(Handle(), pos);
		}
	}
#endif
}

void ff::MainWindow::OnFocusViewTypeChanged(ViewType previousType, ViewType newType)
{
}

std::shared_ptr<ff::IViewWindowInternal> ff::MainWindow::GetActiveViewInternal(ViewType type) const
{
	std::shared_ptr<IViewWindowInternal> view;
	const InternalViewInfo *info = GetInternalViewInfo(type);
	noAssertRetVal(info, view);

	for (auto &curView: info->_viewOrder)
	{
		if (curView->IsActive())
		{
			view = curView;
			break;
		}
	}

	return view;
}

ff::MainWindow::InternalViewInfo *ff::MainWindow::GetInternalViewInfo(ViewType type)
{
	BucketIter iter = _viewInfo.Get(type);
	noAssertRetVal(iter != INVALID_ITER, nullptr);
	InternalViewInfo *info = &_viewInfo.ValueAt(iter);
	return info;
}

ff::ViewType ff::MainWindow::GetFreeCustomViewType() const
{
	for (size_t i = (size_t)ViewType::LAST_CUSTOM; ; i++)
	{
		ViewType type = (ViewType)i;

		if (_viewInfo.Get(type) == INVALID_ITER)
		{
			return type;
		}
	}
}

const ff::MainWindow::InternalViewInfo *ff::MainWindow::GetInternalViewInfo(ViewType type) const
{
	ff::MainWindow *editThis = const_cast<ff::MainWindow *>(this);
	InternalViewInfo *info = editThis->GetInternalViewInfo(type);
	return info;
}

ff::PWND ff::MainWindow::GetViewParent(ViewType type, bool onlyVisibleChild)
{
	InternalViewInfo *info = GetInternalViewInfo(type);
	if (info == nullptr && !onlyVisibleChild)
	{
		BucketIter iter = _viewInfo.Insert(std::move(type), InternalViewInfo());
		info = &_viewInfo.ValueAt(iter);
		info->_type = type;

#if METRO_APP
		info->_parentHandle = Handle();
#else
		info->_parentWindow.SetListener(this);
		CustomWindow &parentWindow = info->_parentWindow;
		assertRetVal(CreateViewParent(type, parentWindow), nullptr);

		if (!parentWindow.Handle())
		{
			// A window wasn't created. That's only valid for full screen document views.
			if (type != ViewType::DOCUMENT || !UseFullScreenViews())
			{
				_viewInfo.DeletePos(iter);
				assertRetVal(false, nullptr);
			}
		}

		info->_parentHandle = parentWindow.Handle() ? parentWindow.Handle() : Handle();
		Layout();
#endif
	}

	if (!info)
	{
		assert(onlyVisibleChild);
		return nullptr;
	}

#if !METRO_APP
	if (info->_parentHandle && onlyVisibleChild)
	{
		if (!ff::IsWindowVisibleStyle(info->_parentHandle) ||
			::GetParent(info->_parentHandle) != Handle())
		{
			return nullptr;
		}
	}
#endif

	assert(info->_parentHandle != nullptr);
	return MemberToWindow(info->_parentHandle);
}

bool ff::MainWindow::ExecuteCommand(DWORD id)
{
	ff::ComPtr<ff::ICommandHandler> handler;
	ff::ComPtr<ff::ICommandExecuteHandler> action;

	if (GetCommandRouter()->FindCommandHandler(id, &handler) &&
		action.QueryFrom(handler) &&
		handler->CommandIsEnabled(id))
	{
		action->CommandOnExecuted(id);
		OnCommandExecuted(id);
		return true;
	}

	return false;
}

void ff::MainWindow::PostExecuteCommand(DWORD id)
{
#if METRO_APP
	ExecuteCommand(id); // uhh, not really posting a message, might be bad
#else
	::PostMessage(Handle(), WM_COMMAND,  MAKEWPARAM(id, 0), 0);
#endif
}

void ff::MainWindow::OnCommandExecuted(DWORD id)
{
	// This could be smarter...
	GetCommandGroups()->InvalidateAll();
}

void ff::MainWindow::UpdateCommands(const DWORD *ids, size_t count)
{
}

void ff::MainWindow::UpdateGroups(const DWORD *groups, size_t count)
{
}

void ff::MainWindow::OnGroupInvalidated(DWORD id)
{
}

void ff::MainWindow::OnMruChanged(const MostRecentlyUsed &mru)
{
	GetCommandGroups()->InvalidateAll();
}

void ff::MainWindow::Layout()
{
#if !METRO_APP
	assertRet(IsValid());
	noAssertRet(_allowLayout);

	ff::RepostMessageOnce(Handle(), WM_APP_LAYOUT_NOW, 0, 0);
#endif
}

bool ff::MainWindow::Activate()
{
	assertRetVal(IsValid(), false);

#if METRO_APP
	Handle()->Activate();
#else
	if (::IsIconic(Handle()))
	{
		::ShowWindow(Handle(), SW_SHOWNORMAL);
	}

	if (!::SetForegroundWindow(Handle()))
	{
		::SetActiveWindow(Handle());
	}
#endif

	return IsActive();
}

void ff::MainWindow::Close()
{
	assertRet(IsValid());

	// Can't close metro windows
#if !METRO_APP
	::PostMessage(Handle(), WM_CLOSE, 0, 0);
#endif
}

ff::PWND ff::MainWindow::Recycle()
{
#if !METRO_APP
	if (CanRecycle() && OnClose(false))
	{
		OnDestroy();
		ff::DestroyChildWindows(Handle());
		return Detach();
	}
#endif
	return nullptr;
}

bool ff::MainWindow::FrameAdvance()
{
	bool result = true;

	if (GetMouse())
	{
		GetMouse()->Advance();
	}

	if (GetTouch())
	{
		GetTouch()->Advance();
	}

	if (GetKeys())
	{
		GetKeys()->Advance();
	}

	if (GetJoysticks())
	{
		GetJoysticks()->Advance();
	}

	if (!_audioPaused && GetAudio())
	{
		GetAudio()->AdvanceEffects();
	}

	// Could advance all active views if it's needed someday
	std::shared_ptr<IViewWindowInternal> view = GetActiveViewInternal(ViewType::DOCUMENT);
	if (view != nullptr)
	{
		result = view->FrameAdvance();
	}

	return result;
}

void ff::MainWindow::FrameRender()
{
	assertRet(GetGraph() && GetGraph()->GetContext());

	if (GetTarget())
	{
		GetTarget()->Clear();

		if (GetDepth())
		{
			GetDepth()->Clear();
		}

		// Could render all active views if it's needed someday
		std::shared_ptr<IViewWindowInternal> view = GetActiveViewInternal(ViewType::DOCUMENT);
		if (view != nullptr)
		{
			view->FrameRender(GetTarget());
		}
	}
}

void ff::MainWindow::FramePresent(bool vsync)
{
	if (GetTarget())
	{
		GetTarget()->Present(vsync);
	}
}

bool ff::MainWindow::FrameVsync()
{
	if (GetTarget())
	{
		GetTarget()->WaitForVsync();
		return true;
	}

	return false;
}

void ff::MainWindow::FrameCleanup()
{
#if !METRO_APP
	HWND focusWindow = ::GetFocus();
	if (_previousFocusWindow != focusWindow)
	{
		_previousFocusWindow = focusWindow;
		CheckViewFocus();
	}
#endif
}

void ff::MainWindow::PostRequestForFrameUpdate(bool vsync)
{
#if METRO_APP
	GetAppGlobals().RequestFrameUpdate(this, vsync);
#else
	ff::PostMessageOnce(Handle(), WM_APP_FRAME_UPDATE_NOW, vsync ? 1 : 0, 0);
#endif
}

bool ff::MainWindow::CanClose()
{
	for (auto &i: _viewInfo)
	{
		for (auto &view: i.GetValue()._views)
		{
			if (!view->CanClose())
			{
				return false;
			}
		}
	}

	return true;
}

bool ff::MainWindow::CanRecycle()
{
	return true;
}

void ff::MainWindow::OnClosing()
{
}

void ff::MainWindow::OnEndSession()
{
}

bool ff::MainWindow::CanEndSession(bool askUser)
{
	return true;
}

void ff::MainWindow::OnActivated()
{
#if !METRO_APP
	PostMessageOnce(Handle(), WM_APP_REFRESH_POSITION, 0, 0);
#endif

	for (auto &i: _viewInfo)
	{
		for (auto &view: i.GetValue()._views)
		{
			if (view->IsActive())
			{
				view->WindowActivated();
			}
		}
	}

	if (_audioPaused)
	{
		_audioPaused = false;

		if (GetAudio())
		{
			GetAudio()->ResumeEffects();
		}
	}
}

void ff::MainWindow::OnDeactivated()
{
	if (!_audioPaused)
	{
		_audioPaused = true;

		if (GetAudio())
		{
			GetAudio()->PauseEffects();
		}
	}

	for (auto &i: _viewInfo)
	{
		for (auto &view: i.GetValue()._views)
		{
			if (view->IsActive())
			{
				view->WindowDeactivated();
			}
		}
	}

#if !METRO_APP
	// CRenderTargetWindow::DeviceTopWindowProc can deal with full window targets
	if (!IsFullWindowTarget() && IsFullScreen())
	{
		// Keep the window full screen, but hide it
		::ShowWindow(Handle(), SW_SHOWMINIMIZED);
	}
#endif
}

void ff::MainWindow::OnViewOpened(IViewWindowInternal *view)
{
}

void ff::MainWindow::OnViewClosed(IViewWindowInternal *view)
{
}

void ff::MainWindow::OnViewMoved(IViewWindowInternal *view, size_t oldIndex, size_t newIndex)
{
}

void ff::MainWindow::OnViewActivated(IViewWindowInternal *view)
{
}

void ff::MainWindow::OnViewDeactivated(IViewWindowInternal *view)
{
}

bool ff::MainWindow::OnAccelerator(DWORD id)
{
	return ExecuteCommand(id);
}

void ff::MainWindow::OnActivate(bool active)
{
	OnSuspended(_suspended && !active);

	if (!_active != !active)
	{
		_active = active;

		if (_active)
		{
			OnActivated();

			_host->OnWindowActivated(*this);
		}
		else
		{
			OnDeactivated();

			_host->OnWindowDeactivated(*this);
		}

		CheckViewFocus();
	}

	if (_active)
	{
		CancelModal();
	}
}

bool ff::MainWindow::OnClose(bool force)
{
#if !METRO_APP
	SavePosition();
#endif

	if (!force && !CanClose())
	{
		return false;
	}

	OnClosing();

	return _host->OnWindowClosing(*this);
}

void ff::MainWindow::OnDestroy()
{
	_allowLayout = false;
	_destroyed = true;

	CloseAllViews(ViewType::VIEW_TYPE_COUNT, true);
	Cleanup();

	_host->OnWindowDestroyed(*this);
}

void ff::MainWindow::OnDpiChanged()
{
#if METRO_APP
	float dpi = DisplayInformation::GetForCurrentView()->LogicalDpi;
	_dpi.SetPoint(static_cast<int>(dpi), static_cast<int>(dpi));
	_dpiScaleDipToPixel.SetPoint(dpi / DEFAULT_DPI_D, dpi / DEFAULT_DPI_D);
#else
	HDC hdc = ::GetDC(nullptr);
	if (hdc)
	{
		_dpi.SetPoint(::GetDeviceCaps(hdc, LOGPIXELSX), ::GetDeviceCaps(hdc, LOGPIXELSY));
		_dpiScaleDipToPixel.SetPoint(_dpi.x / DEFAULT_DPI_D, _dpi.y / DEFAULT_DPI_D);
		::ReleaseDC(nullptr, hdc);
	}
#endif
}

void ff::MainWindow::OnSuspended(bool suspend)
{
	if (!suspend != !_suspended)
	{
		_suspended = suspend;

		if (_suspended)
		{
			if (GetAudio())
			{
				GetAudio()->Stop();
			}

			_host->OnWindowSuspending(*this);
		}
		else
		{
			if (GetAudio())
			{
				GetAudio()->Start();
			}

			_host->OnWindowResumed(*this);
		}
	}
}

#if !METRO_APP

void ff::MainWindow::OnEnable(bool enabled)
{
	if (enabled)
	{
		PopModal();
	}
	else
	{
		PushModal();
	}
}

void ff::MainWindow::OnSetFocus()
{
	std::shared_ptr<IViewWindowInternal> view = GetActiveViewInternal(ViewType::DOCUMENT);
	if (view != nullptr)
	{
		view->SetFocus();
	}
}

bool ff::MainWindow::OnSetCursorViewTarget()
{
	noAssertRetVal(IsActive() && !IsModal(), false);

	bool fullScreen = IsFullScreen();
	HCURSOR cursor = nullptr;

	if ((fullScreen && UseCursorFullScreen()) ||
		(!fullScreen && UseCursorWindowed()))
	{
		cursor = GetCursorViewTarget();
	}

	::SetCursor(cursor);

	return true;
}

bool ff::MainWindow::OnPaintViewTarget()
{
	assertRetVal(GetDocumentViewTarget(), false);

	PAINTSTRUCT ps;
	if (::BeginPaint(GetDocumentViewTarget(), &ps))
	{
		::EndPaint(GetDocumentViewTarget(), &ps);
	}

	if (IsModal() || GetViewCount(ViewType::DOCUMENT) == 0)
	{
		FrameRender();
		FramePresent(false);
	}

	return true;
}

#endif
