#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/JoystickDevice.h"
#include "Module/Module.h"
#include "Resource/util-resource.h"

size_t ff::GetMaxXboxJoystickCount()
{
	return 4;
}

static float AnalogShortToFloat(SHORT sh)
{
	return sh / ((sh < 0) ? 32768.0f : 32767.0f);
}

namespace ff
{
	class __declspec(uuid("a46186b2-1e05-4a3b-bce0-547b42c2bcf5"))
		XboxJoystick : public ComBase, public IJoystickDevice
	{
	public:
		DECLARE_HEADER(XboxJoystick);

		bool Init(size_t nIndex);

		// IInputDevice
		virtual void Advance() override;
		virtual String GetName() const override;
		virtual void SetName(StringRef name) override;
		virtual bool IsConnected() const override;

		// IJoystickDevice
		virtual size_t GetStickCount() const override;
		virtual PointFloat GetStickPos(size_t nStick, bool bDigital) const override;
		virtual RectInt GetStickPressCount(size_t nStick) const override;
		virtual String GetStickName(size_t nStick) const override;

		virtual size_t GetDPadCount() const override;
		virtual PointInt GetDPadPos(size_t nDPad) const override;
		virtual RectInt GetDPadPressCount(size_t nDPad) const override;
		virtual String GetDPadName(size_t nDPad) const override;

		virtual size_t GetButtonCount() const override;
		virtual bool GetButton(size_t nButton) const override;
		virtual int GetButtonPressCount(size_t nButton) const override;
		virtual String GetButtonName(size_t nButton) const override;

		virtual size_t GetTriggerCount() const override;
		virtual float GetTrigger(size_t nTrigger, bool bDigital) const override;
		virtual int GetTriggerPressCount(size_t nTrigger) const override;
		virtual String GetTriggerName(size_t nTrigger) const override;

		virtual bool HasSpecialButton(ESpecialJoystickButton button) const override;
		virtual bool GetSpecialButton(ESpecialJoystickButton button) const override;
		virtual int GetSpecialButtonPressCount(ESpecialJoystickButton button) const override;
		virtual String GetSpecialButtonName(ESpecialJoystickButton button) const override;

	private:
		void CheckPresses(WORD prevButtons);
		static size_t ChooseRandomConnectionCount();

		BYTE &GetPressed(int vk);
		BYTE GetPressed(int vk) const;
		bool IsButtonPressed(WORD buttonFlag) const;

		static const size_t s_nMaxButtons = VK_PAD_RTHUMB_DOWNLEFT + 1 - VK_PAD_A;

		XINPUT_STATE _state;
		String _name;
		DWORD _index;
		size_t _checkConnected;
		bool _connected;
		float _triggerPressing[2];
		PointFloat _stickPressing[2];
		BYTE _pressed[s_nMaxButtons];
	};
}

BEGIN_INTERFACES(ff::XboxJoystick)
	HAS_INTERFACE(ff::IJoystickDevice)
	HAS_INTERFACE(ff::IInputDevice)
END_INTERFACES()

bool ff::CreateXboxJoystick(size_t nIndex, IJoystickDevice** device)
{
	assertRetVal(device, false);
	*device = nullptr;

	ComPtr<XboxJoystick> pDevice;
	assertRetVal(SUCCEEDED(ComAllocator<XboxJoystick>::CreateInstance(&pDevice)), false);
	assertRetVal(pDevice->Init(nIndex), false);

	*device = pDevice.Detach();
	return true;
}

ff::XboxJoystick::XboxJoystick()
	: _index(0)
	, _checkConnected(0)
	, _connected(true)
{
	assert(_countof(_pressed) == 56);

	ZeroObject(_state);
	ZeroObject(_triggerPressing);
	ZeroObject(_stickPressing);
	ZeroObject(_pressed);
}

ff::XboxJoystick::~XboxJoystick()
{
}

bool ff::XboxJoystick::Init(size_t nIndex)
{
	assertRetVal(nIndex >= 0 && nIndex < GetMaxXboxJoystickCount(), false);

	_index     = (DWORD)nIndex;
	_connected = (XInputGetState(_index, &_state) == ERROR_SUCCESS);
	_checkConnected = ChooseRandomConnectionCount();

	return true;
}

void ff::XboxJoystick::Advance()
{
	ZeroObject(_pressed);

	XINPUT_STATE prevState = _state;
	DWORD hr = (_connected || _checkConnected == 0)
		? XInputGetState(_index, &_state)
		: ERROR_DEVICE_NOT_CONNECTED;

	if (hr == ERROR_SUCCESS)
	{
		_connected = true;
		CheckPresses(prevState.Gamepad.wButtons);
	}
	else
	{
		ZeroObject(_state);
		ZeroObject(_triggerPressing);
		ZeroObject(_stickPressing);

		if (_connected || _checkConnected == 0)
		{
			_connected = false;
			_checkConnected = ChooseRandomConnectionCount();
		}
		else
		{
			_checkConnected--;
		}
	}
}

void ff::XboxJoystick::CheckPresses(WORD prevButtons)
{
	WORD cb = _state.Gamepad.wButtons;

	if (cb)
	{
		GetPressed(VK_PAD_A)            = (cb & XINPUT_GAMEPAD_A)              && !(prevButtons & XINPUT_GAMEPAD_A);
		GetPressed(VK_PAD_B)            = (cb & XINPUT_GAMEPAD_B)              && !(prevButtons & XINPUT_GAMEPAD_B);
		GetPressed(VK_PAD_X)            = (cb & XINPUT_GAMEPAD_X)              && !(prevButtons & XINPUT_GAMEPAD_X);
		GetPressed(VK_PAD_Y)            = (cb & XINPUT_GAMEPAD_Y)              && !(prevButtons & XINPUT_GAMEPAD_Y);
		GetPressed(VK_PAD_RSHOULDER)    = (cb & XINPUT_GAMEPAD_RIGHT_SHOULDER) && !(prevButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
		GetPressed(VK_PAD_LSHOULDER)    = (cb & XINPUT_GAMEPAD_LEFT_SHOULDER)  && !(prevButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
		GetPressed(VK_PAD_DPAD_UP)      = (cb & XINPUT_GAMEPAD_DPAD_UP)        && !(prevButtons & XINPUT_GAMEPAD_DPAD_UP);
		GetPressed(VK_PAD_DPAD_DOWN)    = (cb & XINPUT_GAMEPAD_DPAD_DOWN)      && !(prevButtons & XINPUT_GAMEPAD_DPAD_DOWN);
		GetPressed(VK_PAD_DPAD_LEFT)    = (cb & XINPUT_GAMEPAD_DPAD_LEFT)      && !(prevButtons & XINPUT_GAMEPAD_DPAD_LEFT);
		GetPressed(VK_PAD_DPAD_RIGHT)   = (cb & XINPUT_GAMEPAD_DPAD_RIGHT)     && !(prevButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
		GetPressed(VK_PAD_START)        = (cb & XINPUT_GAMEPAD_START)          && !(prevButtons & XINPUT_GAMEPAD_START);
		GetPressed(VK_PAD_BACK)         = (cb & XINPUT_GAMEPAD_BACK)           && !(prevButtons & XINPUT_GAMEPAD_BACK);
		GetPressed(VK_PAD_LTHUMB_PRESS) = (cb & XINPUT_GAMEPAD_LEFT_THUMB)     && !(prevButtons & XINPUT_GAMEPAD_LEFT_THUMB);
		GetPressed(VK_PAD_RTHUMB_PRESS) = (cb & XINPUT_GAMEPAD_RIGHT_THUMB)    && !(prevButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
	}

	if (GetPressed(VK_PAD_START))
	{
		int i = 0;
		i = i;
	}

	float t0 = _state.Gamepad.bLeftTrigger  / 255.0f;
	float t1 = _state.Gamepad.bRightTrigger / 255.0f;

	float x0 =  AnalogShortToFloat(_state.Gamepad.sThumbLX);
	float y0 = -AnalogShortToFloat(_state.Gamepad.sThumbLY);
	float x1 =  AnalogShortToFloat(_state.Gamepad.sThumbRX);
	float y1 = -AnalogShortToFloat(_state.Gamepad.sThumbRY);

	const float rmax = 0.55f;
	const float rmin = 0.50f;

	// Left trigger press
	if      (t0 >= rmax)        { GetPressed(VK_PAD_LTRIGGER) = (_triggerPressing[0] == 0); _triggerPressing[0] = 1; }
	else if (t0 <= rmin)        { _triggerPressing[0] = 0; }

	// Right trigger press
	if      (t1 >= rmax)        { GetPressed(VK_PAD_RTRIGGER) = (_triggerPressing[1] == 0); _triggerPressing[1] = 1; }
	else if (t1 <= rmin)        { _triggerPressing[1] = 0; }

	// Left stick X press
	if      (x0 >=  rmax)       { GetPressed(VK_PAD_LTHUMB_RIGHT) = (_stickPressing[0].x !=  1); _stickPressing[0].x =  1; }
	else if (x0 <= -rmax)       { GetPressed(VK_PAD_LTHUMB_LEFT)  = (_stickPressing[0].x != -1); _stickPressing[0].x = -1; }
	else if (fabsf(x0) <= rmin) { _stickPressing[0].x =  0; }

	// Left stick Y press
	if      (y0 >=  rmax)       { GetPressed(VK_PAD_LTHUMB_DOWN) = (_stickPressing[0].y !=  1); _stickPressing[0].y =  1; }
	else if (y0 <= -rmax)       { GetPressed(VK_PAD_LTHUMB_UP)   = (_stickPressing[0].y != -1); _stickPressing[0].y = -1; }
	else if (fabsf(y0) <= rmin) { _stickPressing[0].y =  0; }

	// Right stick X press
	if      (x1 >=  rmax)       { GetPressed(VK_PAD_RTHUMB_RIGHT) = (_stickPressing[1].x !=  1); _stickPressing[1].x =  1; }
	else if (x1 <= -rmax)       { GetPressed(VK_PAD_RTHUMB_LEFT)  = (_stickPressing[1].x != -1); _stickPressing[1].x = -1; }
	else if (fabsf(x1) <= rmin) { _stickPressing[1].x =  0; }

	// Right stick Y press
	if      (y1 >=  rmax)       { GetPressed(VK_PAD_RTHUMB_DOWN) = (_stickPressing[1].y !=  1); _stickPressing[1].y =  1; }
	else if (y1 <= -rmax)       { GetPressed(VK_PAD_RTHUMB_UP)   = (_stickPressing[1].y != -1); _stickPressing[1].y = -1; }
	else if (fabsf(y1) <= rmin) { _stickPressing[1].y =  0; }
}

size_t ff::XboxJoystick::ChooseRandomConnectionCount()
{
	return 60 + std::rand() % 60;
}

ff::String ff::XboxJoystick::GetName() const
{
	return _name.empty()
		? GetThisModule().GetFormattedString(IDS_XBOX_JOYSTICK_NAME, _index + 1)
		: _name;
}

void ff::XboxJoystick::SetName(StringRef name)
{
	_name = name;
}

bool ff::XboxJoystick::IsConnected() const
{
	return _connected;
}

size_t ff::XboxJoystick::GetStickCount() const
{
	return 2;
}

ff::PointFloat ff::XboxJoystick::GetStickPos(size_t nStick, bool bDigital) const
{
	PointFloat pos(0, 0);

	if (bDigital)
	{
		switch (nStick)
		{
		case 0: return _stickPressing[0]; break;
		case 1: return _stickPressing[1]; break;
		default: assert(false);
		}
	}
	else
	{
		switch (nStick)
		{
		case 0:
			pos.x =  AnalogShortToFloat(_state.Gamepad.sThumbLX);
			pos.y = -AnalogShortToFloat(_state.Gamepad.sThumbLY);
			break;

		case 1:
			pos.x =  AnalogShortToFloat(_state.Gamepad.sThumbRX);
			pos.y = -AnalogShortToFloat(_state.Gamepad.sThumbRY);
			break;

		default:
			assert(false);
		}
	}

	return pos;
}

ff::RectInt ff::XboxJoystick::GetStickPressCount(size_t nStick) const
{
	RectInt press(0, 0, 0, 0);

	switch (nStick)
	{
	case 0:
		press.left   = GetPressed(VK_PAD_LTHUMB_LEFT);
		press.right  = GetPressed(VK_PAD_LTHUMB_RIGHT);
		press.top    = GetPressed(VK_PAD_LTHUMB_UP);
		press.bottom = GetPressed(VK_PAD_LTHUMB_DOWN);
		break;

	case 1:
		press.left   = GetPressed(VK_PAD_RTHUMB_LEFT);
		press.right  = GetPressed(VK_PAD_RTHUMB_RIGHT);
		press.top    = GetPressed(VK_PAD_RTHUMB_UP);
		press.bottom = GetPressed(VK_PAD_RTHUMB_DOWN);
		break;

	default:
		assert(false);
	}

	return press;
}

ff::String ff::XboxJoystick::GetStickName(size_t nStick) const
{
	switch (nStick)
	{
	case 0: return GetThisModule().GetString(IDS_XBOX_PAD_LEFT);
	case 1: return GetThisModule().GetString(IDS_XBOX_PAD_RIGHT);
	default: assertRetVal(false, GetThisModule().GetString(IDS_STICK_UNKNOWN));
	}
}

size_t ff::XboxJoystick::GetDPadCount() const
{
	return 1;
}

ff::PointInt ff::XboxJoystick::GetDPadPos(size_t nDPad) const
{
	PointInt pos(0, 0);

	if (!nDPad)
	{
		pos.x = (_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? -1 : ((_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 1 : 0);
		pos.y = (_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)   ? -1 : ((_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  ? 1 : 0);
	}

	return pos;
}

ff::RectInt ff::XboxJoystick::GetDPadPressCount(size_t nDPad) const
{
	RectInt press(0, 0, 0, 0);

	if (!nDPad)
	{
		press.left   = GetPressed(VK_PAD_DPAD_LEFT);
		press.right  = GetPressed(VK_PAD_DPAD_RIGHT);
		press.top    = GetPressed(VK_PAD_DPAD_UP);
		press.bottom = GetPressed(VK_PAD_DPAD_DOWN);
	}

	return press;
}

ff::String ff::XboxJoystick::GetDPadName(size_t nDPad) const
{
	switch (nDPad)
	{
	case 0: return GetThisModule().GetString(IDS_XBOX_PAD_DIGITAL);
	default: assertRetVal(false, GetThisModule().GetString(IDS_STICK_UNKNOWN));
	}
}

size_t ff::XboxJoystick::GetButtonCount() const
{
	return 10;
}

bool ff::XboxJoystick::GetButton(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return IsButtonPressed(XINPUT_GAMEPAD_A);
	case 1: return IsButtonPressed(XINPUT_GAMEPAD_B);
	case 2: return IsButtonPressed(XINPUT_GAMEPAD_X);
	case 3: return IsButtonPressed(XINPUT_GAMEPAD_Y);
	case 4: return IsButtonPressed(XINPUT_GAMEPAD_BACK);
	case 5: return IsButtonPressed(XINPUT_GAMEPAD_START);
	case 6: return IsButtonPressed(XINPUT_GAMEPAD_LEFT_SHOULDER);
	case 7: return IsButtonPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER);
	case 8: return IsButtonPressed(XINPUT_GAMEPAD_LEFT_THUMB);
	case 9: return IsButtonPressed(XINPUT_GAMEPAD_RIGHT_THUMB);
	default: assertRetVal(false, false);
	}
}

int ff::XboxJoystick::GetButtonPressCount(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return GetPressed(VK_PAD_A);
	case 1: return GetPressed(VK_PAD_B);
	case 2: return GetPressed(VK_PAD_X);
	case 3: return GetPressed(VK_PAD_Y);
	case 4: return GetPressed(VK_PAD_BACK);
	case 5: return GetPressed(VK_PAD_START);
	case 6: return GetPressed(VK_PAD_LSHOULDER);
	case 7: return GetPressed(VK_PAD_RSHOULDER);
	case 8: return GetPressed(VK_PAD_LTHUMB_PRESS);
	case 9: return GetPressed(VK_PAD_RTHUMB_PRESS);
	default: assertRetVal(false, 0);
	}
}

ff::String ff::XboxJoystick::GetButtonName(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return GetThisModule().GetString(IDS_XBOX_BUTTON_A);
	case 1: return GetThisModule().GetString(IDS_XBOX_BUTTON_B);
	case 2: return GetThisModule().GetString(IDS_XBOX_BUTTON_X);
	case 3: return GetThisModule().GetString(IDS_XBOX_BUTTON_Y);
	case 4: return GetThisModule().GetString(IDS_XBOX_BUTTON_BACK);
	case 5: return GetThisModule().GetString(IDS_XBOX_BUTTON_START);
	case 6: return GetThisModule().GetString(IDS_XBOX_BUTTON_LSHOULDER);
	case 7: return GetThisModule().GetString(IDS_XBOX_BUTTON_RSHOULDER);
	case 8: return GetThisModule().GetString(IDS_XBOX_BUTTON_LTHUMB);
	case 9: return GetThisModule().GetString(IDS_XBOX_BUTTON_RTHUMB);
	default: assertRetVal(false, String());
	}
}

size_t ff::XboxJoystick::GetTriggerCount() const
{
	return 2;
}

float ff::XboxJoystick::GetTrigger(size_t nTrigger, bool bDigital) const
{
	if (bDigital)
	{
		switch (nTrigger)
		{
		case 0: return _triggerPressing[0];
		case 1: return _triggerPressing[1];
		default: assertRetVal(false, 0);
		}
	}
	else
	{
		switch (nTrigger)
		{
		case 0: return _state.Gamepad.bLeftTrigger / 255.0f;
		case 1: return _state.Gamepad.bRightTrigger / 255.0f;
		default: assertRetVal(false, 0);
		}
	}
}

int ff::XboxJoystick::GetTriggerPressCount(size_t nTrigger) const
{
	switch (nTrigger)
	{
	case 0: return GetPressed(VK_PAD_LTRIGGER);
	case 1: return GetPressed(VK_PAD_RTRIGGER);
	default: assertRetVal(false, 0);
	}
}

ff::String ff::XboxJoystick::GetTriggerName(size_t nTrigger) const
{
	switch (nTrigger)
	{
	case 0: return GetThisModule().GetString(IDS_XBOX_LTRIGGER);
	case 1: return GetThisModule().GetString(IDS_XBOX_RTRIGGER);
	default: assertRetVal(false, String());
	}
}

BYTE &ff::XboxJoystick::GetPressed(int vk)
{
	return _pressed[vk - VK_PAD_A];
}

BYTE ff::XboxJoystick::GetPressed(int vk) const
{
	return _pressed[vk - VK_PAD_A];
}

bool ff::XboxJoystick::IsButtonPressed(WORD buttonFlag) const
{
	return (_state.Gamepad.wButtons & buttonFlag) == buttonFlag;
}

bool ff::XboxJoystick::HasSpecialButton(ESpecialJoystickButton button) const
{
	switch (button)
	{
	case JOYSTICK_BUTTON_BACK:
	case JOYSTICK_BUTTON_START:
	case JOYSTICK_BUTTON_LEFT_BUMPER:
	case JOYSTICK_BUTTON_RIGHT_BUMPER:
	case JOYSTICK_BUTTON_LEFT_STICK:
	case JOYSTICK_BUTTON_RIGHT_STICK:
	case JOYSTICK_BUTTON_A:
	case JOYSTICK_BUTTON_B:
	case JOYSTICK_BUTTON_X:
	case JOYSTICK_BUTTON_Y:
		return true;
	}

	return false;
}

bool ff::XboxJoystick::GetSpecialButton(ESpecialJoystickButton button) const
{
	switch (button)
	{
	case JOYSTICK_BUTTON_BACK:
		return GetButton(4);

	case JOYSTICK_BUTTON_START:
		return GetButton(5);

	case JOYSTICK_BUTTON_LEFT_BUMPER:
		return GetButton(6);

	case JOYSTICK_BUTTON_RIGHT_BUMPER:
		return GetButton(7);

	case JOYSTICK_BUTTON_LEFT_STICK:
		return GetButton(9);

	case JOYSTICK_BUTTON_RIGHT_STICK:
		return GetButton(10);

	case JOYSTICK_BUTTON_A:
		return GetButton(0);

	case JOYSTICK_BUTTON_B:
		return GetButton(1);

	case JOYSTICK_BUTTON_X:
		return GetButton(2);

	case JOYSTICK_BUTTON_Y:
		return GetButton(3);
	}

	assertRetVal(false, false);
}

int ff::XboxJoystick::GetSpecialButtonPressCount(ESpecialJoystickButton button) const
{
	switch (button)
	{
	case JOYSTICK_BUTTON_BACK:
		return GetButtonPressCount(4);

	case JOYSTICK_BUTTON_START:
		return GetButtonPressCount(5);

	case JOYSTICK_BUTTON_LEFT_BUMPER:
		return GetButtonPressCount(6);

	case JOYSTICK_BUTTON_RIGHT_BUMPER:
		return GetButtonPressCount(7);

	case JOYSTICK_BUTTON_LEFT_STICK:
		return GetButtonPressCount(9);

	case JOYSTICK_BUTTON_RIGHT_STICK:
		return GetButtonPressCount(10);

	case JOYSTICK_BUTTON_A:
		return GetButtonPressCount(0);

	case JOYSTICK_BUTTON_B:
		return GetButtonPressCount(1);

	case JOYSTICK_BUTTON_X:
		return GetButtonPressCount(2);

	case JOYSTICK_BUTTON_Y:
		return GetButtonPressCount(3);
	}

	assertRetVal(false, 0);
}

ff::String ff::XboxJoystick::GetSpecialButtonName(ESpecialJoystickButton button) const
{
	switch (button)
	{
	case JOYSTICK_BUTTON_BACK:
		return GetButtonName(4);

	case JOYSTICK_BUTTON_START:
		return GetButtonName(5);

	case JOYSTICK_BUTTON_LEFT_BUMPER:
		return GetButtonName(6);

	case JOYSTICK_BUTTON_RIGHT_BUMPER:
		return GetButtonName(7);

	case JOYSTICK_BUTTON_LEFT_STICK:
		return GetButtonName(9);

	case JOYSTICK_BUTTON_RIGHT_STICK:
		return GetButtonName(10);

	case JOYSTICK_BUTTON_A:
		return GetButtonName(0);

	case JOYSTICK_BUTTON_B:
		return GetButtonName(1);

	case JOYSTICK_BUTTON_X:
		return GetButtonName(2);

	case JOYSTICK_BUTTON_Y:
		return GetButtonName(3);
	}

	assertRetVal(false, String());
}
