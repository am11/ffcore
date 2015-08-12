#include "pch.h"
#include "Input/PointerListener.h"

#if METRO_APP

using namespace Platform;
using namespace Windows::Devices::Input;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;

ff::PointerEvents::PointerEvents(IPointerEventListener *pParent, CoreWindow ^window)
	: _window(window)
	, _parent(pParent)
	, _displayInfo(DisplayInformation::GetForCurrentView())
{
	_dpiScale = _displayInfo->LogicalDpi / 96;

	_tokens[0] = window->PointerEntered     += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ff::PointerEvents::OnPointerEntered);
	_tokens[1] = window->PointerExited      += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ff::PointerEvents::OnPointerExited);
	_tokens[2] = window->PointerMoved       += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ff::PointerEvents::OnPointerMoved);
	_tokens[3] = window->PointerPressed     += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ff::PointerEvents::OnPointerPressed);
	_tokens[4] = window->PointerReleased    += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ff::PointerEvents::OnPointerReleased);
	_tokens[5] = window->PointerCaptureLost += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &ff::PointerEvents::OnPointerCaptureLost);

	_tokens[6] = _displayInfo->DpiChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &ff::PointerEvents::OnLogicalDpiChanged);
}

ff::PointerEvents::~PointerEvents()
{
	Destroy();
}

void ff::PointerEvents::Destroy()
{
	CoreWindow ^window = _window.Get();

	if (window != nullptr)
	{
		window->PointerEntered     -= _tokens[0];
		window->PointerExited      -= _tokens[1];
		window->PointerMoved       -= _tokens[2];
		window->PointerPressed     -= _tokens[3];
		window->PointerReleased    -= _tokens[4];
		window->PointerCaptureLost -= _tokens[5];

		_displayInfo->DpiChanged -= _tokens[6];

		_window.Release();
		_displayInfo = nullptr;
		ZeroObject(_tokens);
	}
}

float ff::PointerEvents::GetDpiScale() const
{
	return _dpiScale;
}

void ff::PointerEvents::OnPointerEntered(CoreWindow ^sender, PointerEventArgs ^args)
{
	_parent->OnPointerEntered(sender, args);
}

void ff::PointerEvents::OnPointerExited(CoreWindow ^sender, PointerEventArgs ^args)
{
	_parent->OnPointerExited(sender, args);
}

void ff::PointerEvents::OnPointerMoved(CoreWindow ^sender, PointerEventArgs ^args)
{
	_parent->OnPointerMoved(sender, args);
}

void ff::PointerEvents::OnPointerPressed(CoreWindow ^sender, PointerEventArgs ^args)
{
	_parent->OnPointerPressed(sender, args);
}

void ff::PointerEvents::OnPointerReleased(CoreWindow ^sender, PointerEventArgs ^args)
{
	_parent->OnPointerReleased(sender, args);
}

void ff::PointerEvents::OnPointerCaptureLost(CoreWindow ^sender, PointerEventArgs ^args)
{
	_parent->OnPointerCaptureLost(sender, args);
}

void ff::PointerEvents::OnLogicalDpiChanged(DisplayInformation ^displayInfo, Object ^sender)
{
	_dpiScale = _displayInfo->LogicalDpi / 96;
}

#endif // METRO_APP
