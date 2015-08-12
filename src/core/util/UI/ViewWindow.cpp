#include "pch.h"
#include "App/AppGlobals.h"
#include "App/Commands.h"
#include "COM/ServiceCollection.h"
#include "UI/MainWindow.h"
#include "UI/ViewWindow.h"

ff::ViewWindow::ViewWindow(MainWindow *window, IServiceProvider *contextServices)
	: _mainWindow(window)
	, _viewType(ViewType::UNKNOWN)
	, _active(false)
	, _allowLayout(false)
{
	verify(CreateServiceCollection(window->GetServices(), &_services));
	SetContextServicesInternal(contextServices);
}

ff::ViewWindow::~ViewWindow()
{
}

bool ff::ViewWindow::Create(PWND parent, ViewType type)
{
	assertRetVal(parent, false);
	_viewType = type;

#if !METRO_APP
	ff::String viewName = String::from_acp(typeid(*this).name() + 6); // skip "class "
	assertRetVal(CreateBlank(viewName, parent, WS_CHILD), false);
	_allowLayout = true;
#endif

	assertRetVal((_commandRouter = CreateCommandRouter()) != nullptr, false);
	assertRetVal(Initialize(), false);

	// Don't do Layout() until the view is activated

	return true;
}

void ff::ViewWindow::Layout()
{
#if !METRO_APP
	noAssertRet(_allowLayout);
	assertRet(IsValid());
	noAssertRet(IsActive());

	_allowLayout = false;

	HWND parent = ::GetParent(Handle());
	RectInt windowRect = ff::GetClientRect(parent);

	SetWindowPos(Handle(), HWND_TOP, windowRect, SWP_SHOWWINDOW);
	LayoutChildren();

	_allowLayout = true;
#endif
}

void ff::ViewWindow::LayoutChildren()
{
#if !METRO_APP
	RectInt clientRect = GetClientRect(Handle());
	HWND target = GetChildWindow();

	if (target != nullptr)
	{
		EnsureWindowParent(target, Handle());
		MoveWindow(target, clientRect);
		EnsureWindowVisible(target, true);
	}
#endif
}

void ff::ViewWindow::OnActivated()
{
}

void ff::ViewWindow::OnDeactivated()
{
}

void ff::ViewWindow::OnClosing()
{
}

void ff::ViewWindow::OnClosed()
{
}

void ff::ViewWindow::OnContextServicesChanged()
{
}

void ff::ViewWindow::Activate()
{
	assertRet(IsValid());

	if (!_active)
	{
		_active = true;
		OnActivated();
		Layout();
	}
}

bool ff::ViewWindow::Deactivate(bool hide)
{
	assertRetVal(IsValid(), true);

	if (_active)
	{
		_active = false;
#if !METRO_APP
		if (hide)
		{
			SetWindowPos(Handle(), HWND_TOP, RectInt(0, 0, 0, 0), SWP_HIDEWINDOW | SWP_NOOWNERZORDER);
		}

		if (_mainWindow != nullptr)
		{
			HWND child = GetChildWindow();
			if (child && ff::IsAncestor(child, Handle()))
			{
				_mainWindow->HideSharedViewTarget();
			}
		}
#endif
		OnDeactivated();
	}

	return true;
}

void ff::ViewWindow::WindowActivated()
{
}

void ff::ViewWindow::WindowDeactivated()
{
}

ff::ViewType ff::ViewWindow::GetViewType() const
{
	return _viewType;
}

void ff::ViewWindow::SetViewType(ViewType type)
{
	_viewType = type;
}

ff::IServiceProvider *ff::ViewWindow::GetContextServices() const
{
	return _contextServices;
}

void ff::ViewWindow::SetContextServices(IServiceProvider *contextServices, bool forceRefresh)
{
	if (_contextServices != contextServices || forceRefresh)
	{
		SetContextServicesInternal(contextServices);
		OnContextServicesChanged();
	}
}

REFGUID ff::ViewWindow::GetSingletonId() const
{
	return GUID_NULL;
}

void ff::ViewWindow::SetContextServicesInternal(IServiceProvider *contextServices)
{
	if (_contextServices != nullptr)
	{
		_services->RemoveProvider(_contextServices);
	}

	_contextServices = contextServices;

	if (_contextServices != nullptr)
	{
		_services->AddProvider(_contextServices);
	}
}

void ff::ViewWindow::SetFocus()
{
#if !METRO_APP
	if (IsActive() && _mainWindow->IsActive())
	{
		HWND hwnd = GetChildWindow();
		if (hwnd != nullptr)
		{
			::SetFocus(hwnd);
		}
	}
#endif
}

bool ff::ViewWindow::IsCloseable() const
{
	return true;
}

bool ff::ViewWindow::IsLocked() const
{
	return false;
}

bool ff::ViewWindow::CanClose()
{
	return true;
}

void ff::ViewWindow::Close()
{
	assertRet(IsValid());

	OnClosing();

#if !METRO_APP
	::SendMessage(Handle(), WM_CLOSE, 0, 0);
#endif

	OnClosed();
}

ff::String ff::ViewWindow::GetShortName() const
{
	return ff::String();
}

ff::String ff::ViewWindow::GetFullName() const
{
	return ff::String();
}

ff::String ff::ViewWindow::GetTooltip() const
{
	return GetFullName();
}

bool ff::ViewWindow::IsDirty() const
{
	return false;
}

#if !METRO_APP
HACCEL ff::ViewWindow::GetAcceleratorTable() const
{
	return nullptr;
}
#endif

bool ff::ViewWindow::FrameAdvance()
{
	return true;
}

void ff::ViewWindow::FrameRender(IRenderTarget *target)
{
}

bool ff::ViewWindow::IsValid() const
{
#if METRO_APP
	return _mainWindow != nullptr;
#else
	return _mainWindow != nullptr && (Handle() != nullptr || !UsesSharedTarget());
#endif
}

bool ff::ViewWindow::IsActive() const 
{
	return _active && IsValid();
}

ff::IMainWindow *ff::ViewWindow::GetMainWindow() const
{
	return _mainWindow;
}

ff::ICommandRouter *ff::ViewWindow::GetCommandRouter() const
{
	return _commandRouter;
}

ff::IServiceProvider *ff::ViewWindow::GetServices() const
{
	return _services;
}

ff::IServiceCollection *ff::ViewWindow::GetServiceCollection() const
{
	return _services;
}

ff::AppGlobals &ff::ViewWindow::GetAppGlobals() const
{
	return GetMainWindow()->GetAppGlobals();
}

ff::ProcessGlobals &ff::ViewWindow::GetProcessGlobals() const
{
	return GetAppGlobals().GetProcessGlobals();
}

bool ff::ViewWindow::Initialize()
{
	return true;
}

ff::ComPtr<ff::ICommandRouter> ff::ViewWindow::CreateCommandRouter()
{
	ComPtr<ICommandRouter> router;
	assertRetVal(ff::CreateNullCommandRouter(&router), ComPtr<ICommandRouter>());

	return router;
}

#if METRO_APP

Windows::UI::Core::CoreWindow ^ff::ViewWindow::Handle() const
{
	return GetMainWindow()->GetHandle();
}

#else

bool ff::ViewWindow::ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT &nResult)
{
	return false;
}

bool ff::ViewWindow::FilterMessage(MSG &msg)
{
	return false;
}

LRESULT ff::ViewWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			Layout();
		}
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
	}

	return DoDefault(hwnd, msg, wParam, lParam);
}

bool ff::ViewWindow::UsesSharedTarget() const
{
	return GetViewType() == ViewType::DOCUMENT &&
		_mainWindow->GetDocumentViewTarget() != nullptr &&
		!_mainWindow->IsFullWindowTarget();
}

HWND ff::ViewWindow::GetChildWindow() const
{
	return UsesSharedTarget() ? _mainWindow->GetDocumentViewTarget() : nullptr;
}

bool ff::ViewWindow::OnClose()
{
	return true;
}

void ff::ViewWindow::OnDestroy()
{
}

#endif
