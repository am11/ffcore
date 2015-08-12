#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/MouseDevice.h"
#include "Input/PointerListener.h"
#include "Module/Module.h"
#include "Resource/util-resource.h"

#if METRO_APP

using namespace Platform;
using namespace Windows::Devices::Input;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;

namespace ff
{
	class __declspec(uuid("08dfcb6a-7d69-4e5a-aed3-628dd89a3e8d"))
		MouseDevice
			: public ComBase
			, public IMouseDevice
			, public IPointerEventListener
	{
	public:
		DECLARE_HEADER(MouseDevice);

		bool Init(CoreWindow ^window);
		void Destroy();

		// IInputDevice
		virtual void Advance() override;
		virtual String GetName() const override;
		virtual void SetName(StringRef name) override;
		virtual bool IsConnected() const override;

		// IMouseDevice
		virtual PWND GetWindow() const override;
		virtual bool IsInWindow() const override;
		virtual PointFloat GetPos() const override;
		virtual PointFloat GetRelativePos() const override;
		virtual float GetDpiScale() const override;

		virtual bool GetButton(int vkButton) const override;
		virtual int GetButtonClickCount(int vkButton) const override;
		virtual int GetButtonDoubleClickCount(int vkButton) const override;
		virtual PointFloat GetWheelScroll() const override;

		// IPointerEventListener
		virtual void OnPointerEntered(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerExited(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerMoved(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerPressed(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerReleased(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerCaptureLost(CoreWindow ^sender, PointerEventArgs ^args) override;

	private:
		bool IsKeyDown(Windows::System::VirtualKey vk);

		struct MouseInfo
		{
			PointFloat _pos;
			PointFloat _posRelative;
			PointFloat _wheel;
			bool _buttons[6];
			BYTE _clicks[6];
			BYTE _doubleClicks[6];
		};

		Agile<CoreWindow> _window;
		PointerEvents ^_events;
		MouseInfo _state;
		MouseInfo _statePending;
		bool _insideWindow;
	};
}

BEGIN_INTERFACES(ff::MouseDevice)
	HAS_INTERFACE(ff::IMouseDevice)
	HAS_INTERFACE(ff::IInputDevice)
END_INTERFACES()

// STATIC_DATA (object)
static ff::Mutex s_deviceCS;
static ff::Vector<ff::MouseDevice*, 8> s_allMouseDevices;

// static
bool ff::CreateMouseDevice(CoreWindow ^window, IMouseDevice **ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;
	{
		LockMutex crit(s_deviceCS);

		for (size_t i = 0; i < s_allMouseDevices.Size(); i++)
		{
			if (s_allMouseDevices[i]->GetWindow() == window)
			{
				*ppInput = GetAddRef(s_allMouseDevices[i]);
				return true;
			}
		}
	}

	ComPtr<MouseDevice> pInput;
	assertRetVal(SUCCEEDED(ComAllocator<MouseDevice>::CreateInstance(&pInput)), false);
	assertRetVal(pInput->Init(window), false);

	*ppInput = pInput.Detach();
	return true;
}

ff::MouseDevice::MouseDevice()
	: _insideWindow(true)
{
	ZeroObject(_state);
	ZeroObject(_statePending);

	LockMutex crit(s_deviceCS);
	s_allMouseDevices.Push(this);
}

ff::MouseDevice::~MouseDevice()
{
	Destroy();

	LockMutex crit(s_deviceCS);
	s_allMouseDevices.Delete(s_allMouseDevices.Find(this));

	if (!s_allMouseDevices.Size())
	{
		s_allMouseDevices.Reduce();
	}
}

bool ff::MouseDevice::Init(CoreWindow ^window)
{
	assertRetVal(window, false);
	_window = window;

	_state._pos.x = _window->PointerPosition.X;
	_state._pos.y = _window->PointerPosition.Y;
	_statePending._pos = _state._pos;
	_events = ref new PointerEvents(this, window);

	return true;
}

void ff::MouseDevice::Destroy()
{
	if (_events != nullptr)
	{
		_events->Destroy();
		_events = nullptr;
	}
}

bool ff::MouseDevice::IsKeyDown(Windows::System::VirtualKey vk)
{
	CoreVirtualKeyStates state = _window->GetKeyState(vk);
	return (state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
}

void ff::MouseDevice::Advance()
{
	_state = _statePending;

	ZeroObject(_statePending);
	_statePending._pos = _state._pos;

	_state._buttons[VK_LBUTTON]  = IsKeyDown(Windows::System::VirtualKey::LeftButton);
	_state._buttons[VK_RBUTTON]  = IsKeyDown(Windows::System::VirtualKey::RightButton);
	_state._buttons[VK_MBUTTON]  = IsKeyDown(Windows::System::VirtualKey::MiddleButton);
	_state._buttons[VK_XBUTTON1] = IsKeyDown(Windows::System::VirtualKey::XButton1);
	_state._buttons[VK_XBUTTON2] = IsKeyDown(Windows::System::VirtualKey::XButton1);
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

ff::PWND ff::MouseDevice::GetWindow() const
{
	return MemberToWindow(_window);
}

bool ff::MouseDevice::IsInWindow() const
{
	return _insideWindow;
}

ff::PointFloat ff::MouseDevice::GetPos() const
{
	return _state._pos * _events->GetDpiScale();
}

ff::PointFloat ff::MouseDevice::GetRelativePos() const
{
	return _state._posRelative * _events->GetDpiScale();
}

float ff::MouseDevice::GetDpiScale() const
{
	return _events->GetDpiScale();
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

void ff::MouseDevice::OnPointerEntered(CoreWindow ^sender, PointerEventArgs ^args)
{
	_insideWindow = true;
}

void ff::MouseDevice::OnPointerExited(CoreWindow ^sender, PointerEventArgs ^args)
{
	_insideWindow = false;
}

void ff::MouseDevice::OnPointerMoved(CoreWindow ^sender, PointerEventArgs ^args)
{
	PointerPoint ^point = args->CurrentPoint;

	_statePending._pos.x = point->Position.X;
	_statePending._pos.y = point->Position.Y;
	_statePending._posRelative = _statePending._pos - _state._pos;
}

void ff::MouseDevice::OnPointerPressed(CoreWindow ^sender, PointerEventArgs ^args)
{
	PointerPoint ^point = args->CurrentPoint;
	int nPresses = 0;
	int vkButton = 0;

	switch (point->Properties->PointerUpdateKind)
	{
	case PointerUpdateKind::LeftButtonPressed:
		vkButton = VK_LBUTTON;
		nPresses = 1;
		break;

	case PointerUpdateKind::LeftButtonReleased:
		vkButton = VK_LBUTTON;
		break;

	case PointerUpdateKind::RightButtonPressed:
		vkButton = VK_RBUTTON;
		nPresses = 1;
		break;

	case PointerUpdateKind::RightButtonReleased:
		vkButton = VK_RBUTTON;
		break;

	case PointerUpdateKind::XButton1Pressed:
		vkButton = VK_XBUTTON1;
		nPresses = 1;
		break;

	case PointerUpdateKind::XButton1Released:
		vkButton = VK_XBUTTON1;
		break;

	case PointerUpdateKind::XButton2Pressed:
		vkButton = VK_XBUTTON2;
		nPresses = 1;
		break;

	case PointerUpdateKind::XButton2Released:
		vkButton = VK_XBUTTON2;
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
	}

	OnPointerMoved(sender, args);
}

void ff::MouseDevice::OnPointerReleased(CoreWindow ^sender, PointerEventArgs ^args)
{
	OnPointerPressed(sender, args);
}

void ff::MouseDevice::OnPointerCaptureLost(CoreWindow ^sender, PointerEventArgs ^args)
{
}

#endif // METRO_APP
