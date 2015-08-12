#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/MouseDevice.h"
#include "Module/Module.h"
#include "Resource/util-resource.h"

#if !METRO_APP

namespace ff
{
	class __declspec(uuid("a2f6e2e1-d9e6-41e0-bd16-49396abcdc26"))
		MouseDevice : public IMouseDevice, public ComBase
	{
	public:
		DECLARE_HEADER(MouseDevice);

		bool Init(HWND hwnd);
		void Destroy();

		// IInputDevice
		virtual void    Advance() override;
		virtual String GetName() const override;
		virtual void    SetName(StringRef name) override;
		virtual bool    IsConnected() const override;

		// IMouseDevice
		virtual HWND GetWindow() const override;
		virtual bool IsInWindow() const override;
		virtual PointFloat GetPos() const override;
		virtual PointFloat GetRelativePos() const override;
		virtual float GetDpiScale() const override;

		virtual bool GetButton(int vkButton) const override;
		virtual int GetButtonClickCount(int vkButton) const override;
		virtual int GetButtonDoubleClickCount(int vkButton) const override;
		virtual PointFloat GetWheelScroll() const override;

	private:
		static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void OnMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		struct MouseInfo
		{
			PointFloat _pos;
			PointFloat _posRelative;
			PointFloat _wheel;
			bool _buttons[6];
			BYTE _clicks[6];
			BYTE _doubleClicks[6];
		};

		HWND _hwnd;
		HWND _hoverWnd;
		WNDPROC _oldWindowProc;

		MouseInfo _state;
		MouseInfo _statePending;
	};
}

BEGIN_INTERFACES(ff::MouseDevice)
	HAS_INTERFACE(ff::IMouseDevice)
	HAS_INTERFACE(ff::IInputDevice)
END_INTERFACES()

// STATIC_DATA (object)
static ff::Vector<ff::MouseDevice*, 8> s_allMouseDevices; // only access this on the main thread

// static
bool ff::CreateMouseDevice(HWND hwnd, IMouseDevice** ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;

	for (size_t i = 0; i < s_allMouseDevices.Size(); i++)
	{
		if (s_allMouseDevices[i]->GetWindow() == hwnd)
		{
			*ppInput = GetAddRef(s_allMouseDevices[i]);
			return true;
		}
	}

	ComPtr<MouseDevice> pInput;
	assertRetVal(SUCCEEDED(ComAllocator<MouseDevice>::CreateInstance(&pInput)), false);
	assertRetVal(pInput->Init(hwnd), false);

	*ppInput = pInput.Detach();
	return true;
}

ff::MouseDevice::MouseDevice()
	: _hwnd(nullptr)
	, _hoverWnd(nullptr)
	, _oldWindowProc(nullptr)
{
	ZeroObject(_state);
	ZeroObject(_statePending);

	s_allMouseDevices.Push(this);
}

ff::MouseDevice::~MouseDevice()
{
	Destroy();

	s_allMouseDevices.Delete(s_allMouseDevices.Find(this));

	if (!s_allMouseDevices.Size())
	{
		s_allMouseDevices.Reduce();
	}
}

bool ff::MouseDevice::Init(HWND hwnd)
{
	assertRetVal(hwnd, false);

	_hwnd           = hwnd;
	_oldWindowProc = SubclassWindow(_hwnd, StaticWindowProc);

	PointInt ipos = ScreenToClient(hwnd, GetCursorPos());
	_state._pos.SetPoint((float)ipos.x, (float)ipos.y);
	_statePending._pos = _state._pos;

	return true;
}

void ff::MouseDevice::Destroy()
{
	if (_hwnd && _oldWindowProc)
	{
		SubclassWindow(_hwnd, _oldWindowProc);

		_oldWindowProc = nullptr;
		_hwnd           = nullptr;
	}
}

void ff::MouseDevice::Advance()
{
	_state = _statePending;

	ZeroObject(_statePending);
	_statePending._pos = _state._pos;

	_state._buttons[VK_LBUTTON] = ff::IsKeyDown(VK_LBUTTON);
	_state._buttons[VK_RBUTTON] = ff::IsKeyDown(VK_RBUTTON);
	_state._buttons[VK_MBUTTON] = ff::IsKeyDown(VK_MBUTTON);
	_state._buttons[VK_XBUTTON1] = ff::IsKeyDown(VK_XBUTTON1);
	_state._buttons[VK_XBUTTON2] = ff::IsKeyDown(VK_XBUTTON2);
}

ff::String ff::MouseDevice::GetName() const
{
	return GetThisModule().GetString(IDS_MOUSE_NAME);
}

void ff::MouseDevice::SetName(StringRef name)
{
}

bool ff::MouseDevice::IsConnected() const
{
	return true;
}

HWND ff::MouseDevice::GetWindow() const
{
	return _hwnd;
}

bool ff::MouseDevice::IsInWindow() const
{
	return _hoverWnd == _hwnd;
}

ff::PointFloat ff::MouseDevice::GetPos() const
{
	return _state._pos;
}

ff::PointFloat ff::MouseDevice::GetRelativePos() const
{
	return _state._posRelative;
}

float ff::MouseDevice::GetDpiScale() const
{
	return 1;
}

static bool IsValidButton(int vkButton)
{
	switch (vkButton)
	{
	case VK_LBUTTON:
	case VK_RBUTTON:
	case VK_MBUTTON:
	case VK_XBUTTON1:
	case VK_XBUTTON2:
		return true;

	default:
		assertRetVal(false, false);
	}
}

bool ff::MouseDevice::GetButton(int vkButton) const
{
	return IsValidButton(vkButton) ? _state._buttons[vkButton] : false;
}

int ff::MouseDevice::GetButtonClickCount(int vkButton) const
{
	return IsValidButton(vkButton) ? _state._clicks[vkButton] : 0;
}

int ff::MouseDevice::GetButtonDoubleClickCount(int vkButton) const
{
	return IsValidButton(vkButton) ? _state._doubleClicks[vkButton] : 0;
}

ff::PointFloat ff::MouseDevice::GetWheelScroll() const
{
	return _state._wheel;
}

// static
LRESULT ff::MouseDevice::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	for (size_t i = 0; i < s_allMouseDevices.Size(); i++)
	{
		if (s_allMouseDevices[i]->GetWindow() == hwnd)
		{
			return s_allMouseDevices[i]->WindowProc(hwnd, msg, wParam, lParam);
		}
	}

	assert(false);
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT ff::MouseDevice::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pOldWndProc = _oldWindowProc;

	if (msg == WM_DESTROY || msg == ff::CustomWindow::GetDetachMessage())
	{
		Destroy();
	}
	else if ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_MOUSELEAVE)
	{
		OnMouseMessage(hwnd, msg, wParam, lParam);
	}

	return pOldWndProc
		? CallWindowProc(pOldWndProc, hwnd, msg, wParam, lParam)
		: DefWindowProc(hwnd, msg, wParam, lParam);
}

void ff::MouseDevice::OnMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	bool bSetHoverWnd = false;
	int  nPresses = 0;
	int  vkButton = 0;

	switch (msg)
	{
	case WM_LBUTTONDOWN:
		vkButton = VK_LBUTTON;
		nPresses = 1;
		break;

	case WM_LBUTTONUP:
		vkButton = VK_LBUTTON;
		break;

	case WM_LBUTTONDBLCLK:
		vkButton = VK_LBUTTON;
		nPresses = 2;
		break;

	case WM_RBUTTONDOWN:
		vkButton = VK_RBUTTON;
		nPresses = 1;
		break;

	case WM_RBUTTONUP:
		vkButton = VK_RBUTTON;
		break;

	case WM_RBUTTONDBLCLK:
		vkButton = VK_RBUTTON;
		nPresses = 2;
		break;

	case WM_MBUTTONDOWN:
		vkButton = VK_MBUTTON;
		nPresses = 1;
		break;

	case WM_MBUTTONUP:
		vkButton = VK_MBUTTON;
		break;

	case WM_MBUTTONDBLCLK:
		vkButton = VK_MBUTTON;
		nPresses = 2;
		break;

	case WM_XBUTTONDOWN:
		switch (GET_XBUTTON_WPARAM(wParam))
		{
		case 1:
			vkButton = VK_XBUTTON1;
			nPresses = 1;
			break;

		case 2:
			vkButton = VK_XBUTTON2;
			nPresses = 1;
			break;
		}
		break;

	case WM_XBUTTONUP:
		switch (GET_XBUTTON_WPARAM(wParam))
		{
		case 1:
			vkButton = VK_XBUTTON1;
			break;

		case 2:
			vkButton = VK_XBUTTON2;
			break;
		}
		break;

	case WM_XBUTTONDBLCLK:
		switch (GET_XBUTTON_WPARAM(wParam))
		{
		case 1:
			vkButton = VK_XBUTTON1;
			nPresses = 2;
			break;

		case 2:
			vkButton = VK_XBUTTON2;
			nPresses = 2;
			break;
		}
		break;

	case WM_MOUSEWHEEL:
		_statePending._wheel.y += GET_WHEEL_DELTA_WPARAM(wParam);
		break;

	case WM_MOUSEHWHEEL:
		_statePending._wheel.x += GET_WHEEL_DELTA_WPARAM(wParam);
		break;

	case WM_MOUSEMOVE:
		vkButton = -1;
		break;

	case WM_MOUSELEAVE:
		_hoverWnd = nullptr;
		break;
	}

	if (vkButton)
	{
		switch (nPresses)
		{
		case 2:
			if (_statePending._doubleClicks[vkButton] != 0xFF)
			{
				_statePending._doubleClicks[vkButton]++;
			}
			__fallthrough;

		case 1:
			if (_statePending._clicks[vkButton] != 0xFF)
			{
				_statePending._clicks[vkButton]++;
			}
			break;
		}

		PointInt ipos = ToPoint(lParam);
		_statePending._pos.SetPoint((float)ipos.x, (float)ipos.y);
		_statePending._posRelative = _statePending._pos - _state._pos;
		bSetHoverWnd = true;
	}

	if (bSetHoverWnd && hwnd != _hoverWnd)
	{
		if (_hoverWnd)
		{
			TRACKMOUSEEVENT tme;
			ZeroObject(tme);
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_CANCEL | TME_LEAVE;
			tme.hwndTrack = _hoverWnd;

			_hoverWnd = nullptr;

			verify(TrackMouseEvent(&tme));
		}

		if (hwnd)
		{
			TRACKMOUSEEVENT tme;
			ZeroObject(tme);
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hwnd;

			_hoverWnd = hwnd;

			verify(TrackMouseEvent(&tme));
		}
	}
}

#endif // !METRO_APP
