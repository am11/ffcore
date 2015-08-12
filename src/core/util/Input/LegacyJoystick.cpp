#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/GlobalDirectInput.h"
#include "Input/JoystickDevice.h"
#include "Module/Module.h"
#include "Resource/util-resource.h"

#if !METRO_APP

// STATIC_DATA(pod)
static const int JOY_AXIS_LIMIT = 1024;
static const int JOY_AXIS_MIDDLE = 563; // 0.55
static const int JOY_AXIS_RELEASE = 460; // 0.45
static const float JOY_AXIS_LIMIT_F = (float)JOY_AXIS_LIMIT;

namespace ff
{
	class __declspec(uuid("80783f12-e700-4af7-9a7d-a4e23a85b91b"))
		LegacyJoystick : public ComBase, public IJoystickDevice
	{
	public:
		DECLARE_HEADER(LegacyJoystick);

		bool Init(HWND hwnd, REFGUID instanceID);
		BOOL CALLBACK ObjectEnumCallback(const DIDEVICEOBJECTINSTANCE *pObject);

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
		void UpdateStickPressing(size_t nStick, int posX, int posY);

		GlobalDirectInput _input;
		ComPtr<IDirectInputDevice8> _device;
		GUID _instanceID;
		String _name;
		DIJOYSTATE _state;
		bool _connected;
		bool _dPad;
		bool _rightStick;
		size_t _buttons;
		PointFloat _stickPressing[3];
		RectInt _stickPressed[3];
		BYTE _buttonPressed[32];
	};
}

BEGIN_INTERFACES(ff::LegacyJoystick)
	HAS_INTERFACE(ff::IJoystickDevice)
	HAS_INTERFACE(ff::IInputDevice)
END_INTERFACES()

bool ff::CreateLegacyJoystick(HWND hwnd, REFGUID instanceID, IJoystickDevice** device)
{
	assertRetVal(device, false);
	*device = nullptr;

	ComPtr<LegacyJoystick> pDevice;
	assertRetVal(SUCCEEDED(ComAllocator<LegacyJoystick>::CreateInstance(&pDevice)), false);
	assertRetVal(pDevice->Init(hwnd, instanceID), false);

	*device = pDevice.Detach();
	return true;
}

static BOOL CALLBACK ObjectEnumCallback(const DIDEVICEOBJECTINSTANCE *pObject, void *pCookie)
{
	ff::LegacyJoystick *pDevice = (ff::LegacyJoystick*)pCookie;
	assertRetVal(pDevice, DIENUM_STOP);

	return pDevice->ObjectEnumCallback(pObject);
}

// Set the range for every axis on the device
BOOL ff::LegacyJoystick::ObjectEnumCallback(const DIDEVICEOBJECTINSTANCE *pObject)
{
	assertRetVal(pObject, DIENUM_STOP);

	if (pObject->dwType & DIDFT_ABSAXIS)
	{
		if (pObject->guidType == GUID_RxAxis || pObject->guidType == GUID_RyAxis)
		{
			_rightStick = true;
		}

		DIPROPRANGE propRange;
		propRange.diph.dwSize       = sizeof(DIPROPRANGE);
		propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		propRange.diph.dwHow        = DIPH_BYID;
		propRange.diph.dwObj        = pObject->dwType; // Specify the enumerated axis
		propRange.lMin              = -JOY_AXIS_LIMIT;
		propRange.lMax              = +JOY_AXIS_LIMIT;

		// Set the range for the axis
		if (FAILED(_device->SetProperty(DIPROP_RANGE, &propRange.diph)))
		{
			return DIENUM_STOP;
		}
	}
	else if (pObject->dwType & DIDFT_POV)
	{
		_dPad = true;
	}
	else if (pObject->dwType & DIDFT_BUTTON)
	{
		if (_buttons < _countof(_buttonPressed))
		{
			_buttons++;
		}
	}

	return DIENUM_CONTINUE;
}


static ff::PointInt AngleToPosition(DWORD angle)
{
	ff::PointInt pos(0, 0);

	if (LOWORD(angle) != 0xFFFF)
	{
		angle /= 100;

		if (angle >= 45 && angle <= 135)
		{
			pos.x = 1;
		}
		else if (angle >= 225 && angle <= 315)
		{
			pos.x = -1;
		}

		if (angle >= 315 || angle <= 45)
		{
			pos.y = -1;
		}
		else if (angle >= 135 && angle <= 225)
		{
			pos.y = 1;
		}
	}

	return pos;
}

ff::LegacyJoystick::LegacyJoystick()
	: _connected(true)
	, _dPad(false)
	, _rightStick(false)
	, _buttons(0)
	, _instanceID(GUID_NULL)
{
	ZeroObject(_state);
	ZeroObject(_stickPressing);
	ZeroObject(_stickPressed);
	ZeroObject(_buttonPressed);
}

ff::LegacyJoystick::~LegacyJoystick()
{
}

bool ff::LegacyJoystick::Init(HWND hwnd, REFGUID instanceID)
{
	_instanceID = instanceID;

	assertRetVal(SUCCEEDED(_input.CreateDevice(hwnd, instanceID, &c_dfDIJoystick, 256, &_device)), false);
	assertRetVal(SUCCEEDED(_device->EnumObjects(::ObjectEnumCallback, this, DIDFT_ALL)), false);

	_device->GetDeviceState(sizeof(DIJOYSTATE), &_state);

	return true;
}

void ff::LegacyJoystick::Advance()
{
	ZeroObject(_stickPressed);
	ZeroObject(_buttonPressed);

	if (_input.Poll(_device))
	{
		_connected = true;

		DIDEVICEOBJECTDATA data;
		DWORD nCount = 1;

		while (SUCCEEDED(_device->GetDeviceData(sizeof(data), &data, &nCount, 0)) && nCount == 1)
		{
			if (data.dwOfs == DIJOFS_X || data.dwOfs == DIJOFS_Y)
			{
				if (data.dwOfs == DIJOFS_X)
				{
					_state.lX = *(int*)&data.dwData;
				}
				else
				{
					_state.lY = *(int*)&data.dwData;
				}

				UpdateStickPressing(0, _state.lX, _state.lY);
			}
			else if (data.dwOfs == DIJOFS_RX || data.dwOfs == DIJOFS_RY)
			{
				if (data.dwOfs == DIJOFS_RX)
				{
					_state.lRx = *(int*)&data.dwData;
				}
				else
				{
					_state.lRy = *(int*)&data.dwData;
				}

				UpdateStickPressing(1, _state.lRx, _state.lRy);
			}
			else if (data.dwOfs == DIJOFS_POV(0))
			{
				_state.rgdwPOV[0] = data.dwData;
				PointInt pos = AngleToPosition(_state.rgdwPOV[0]);

				UpdateStickPressing(2, pos.x * JOY_AXIS_LIMIT, pos.y * JOY_AXIS_LIMIT);
			}
			else if (data.dwOfs >= DIJOFS_BUTTON0 && data.dwOfs <= DIJOFS_BUTTON31)
			{
				size_t nButton = data.dwOfs - DIJOFS_BUTTON0;

				if ((data.dwData & 0x80) && _buttonPressed[nButton] != 0xFF)
				{
					_buttonPressed[nButton]++;
				}
			}
		}

		_device->GetDeviceState(sizeof(DIJOYSTATE), &_state);
	}
	else
	{
		_connected = (_input.GetInput()->GetDeviceStatus(_instanceID) == DI_OK);

		ZeroObject(_state);
		_state.rgdwPOV[0] = (DWORD)-1;

		ZeroObject(_stickPressing);
	}
}

ff::String ff::LegacyJoystick::GetName() const
{
	return _name.empty()
		? GetThisModule().GetString(IDS_LEGACY_JOYSTICK_NAME)
		: _name;
}

void ff::LegacyJoystick::SetName(StringRef name)
{
	_name = name;
}

bool ff::LegacyJoystick::IsConnected() const
{
	return _connected;
}

void ff::LegacyJoystick::UpdateStickPressing(size_t nStick, int posX, int posY)
{
	PointFloat prevPos = _stickPressing[nStick];

	if (prevPos.x < 0 && posX > -JOY_AXIS_RELEASE)
	{
		_stickPressing[nStick].x = 0;
	}
	else if (prevPos.x > 0 && posX < JOY_AXIS_RELEASE)
	{
		_stickPressing[nStick].x = 0;
	}

	if (prevPos.y < 0 && posY >= -JOY_AXIS_RELEASE)
	{
		_stickPressing[nStick].y = 0;
	}
	else if (prevPos.y > 0 && posY <= JOY_AXIS_RELEASE)
	{
		_stickPressing[nStick].y = 0;
	}

	if (posX < -JOY_AXIS_MIDDLE && prevPos.x >= 0)
	{
		_stickPressing[nStick].x = -1;
		_stickPressed[nStick].left++;
	}
	else if (posX > JOY_AXIS_MIDDLE && prevPos.x <= 0)
	{
		_stickPressing[nStick].x = 1;
		_stickPressed[nStick].right++;
	}

	if (posY < -JOY_AXIS_MIDDLE && prevPos.y >= 0)
	{
		_stickPressing[nStick].y = -1;
		_stickPressed[nStick].top++;
	}
	else if (posY > JOY_AXIS_MIDDLE && prevPos.y <= 0)
	{
		_stickPressing[nStick].y = 1;
		_stickPressed[nStick].bottom++;
	}
}

size_t ff::LegacyJoystick::GetStickCount() const
{
	return _rightStick ? 2 : 1;
}

ff::PointFloat ff::LegacyJoystick::GetStickPos(size_t nStick, bool bDigital) const
{
	PointInt pos(0, 0);

	if (nStick < GetStickCount())
	{
		if (bDigital)
		{
			return _stickPressing[nStick];
		}

		switch (nStick)
		{
		case 0: pos.SetPoint(_state.lX,  _state.lY);  break;
		case 1: pos.SetPoint(_state.lRx, _state.lRy); break;
		}
	}

	return PointFloat(pos.x / JOY_AXIS_LIMIT_F, pos.y / JOY_AXIS_LIMIT_F);
}

ff::RectInt ff::LegacyJoystick::GetStickPressCount(size_t nStick) const
{
	assertRetVal(nStick < GetStickCount(), RectInt(0, 0, 0, 0));
	return _stickPressed[nStick];
}

ff::String ff::LegacyJoystick::GetStickName(size_t nStick) const
{
	String szName;

	if (nStick < GetStickCount())
	{
		switch (nStick)
		{
		case 0: szName = GetThisModule().GetString(IDS_STICK0); break;
		case 1: szName = GetThisModule().GetString(IDS_STICK1); break;
		}
	}

	if (szName.empty())
	{
		szName = GetThisModule().GetString(IDS_STICK_UNKNOWN);
	}

	return szName;
}

size_t ff::LegacyJoystick::GetDPadCount() const
{
	return _dPad ? 1 : 0;
}

ff::PointInt ff::LegacyJoystick::GetDPadPos(size_t nDPad) const
{
	PointInt pos(0, 0);

	if (!nDPad)
	{
		pos = AngleToPosition(_state.rgdwPOV[0]);
	}

	return pos;
}

ff::RectInt ff::LegacyJoystick::GetDPadPressCount(size_t nDPad) const
{
	assertRetVal(!nDPad, RectInt(0, 0, 0, 0));
	return _stickPressed[2];
}

ff::String ff::LegacyJoystick::GetDPadName(size_t nDPad) const
{
	String szName;

	if (!nDPad)
	{
		szName = GetThisModule().GetString(IDS_STICK_POV);
	}

	if (szName.empty())
	{
		szName = GetThisModule().GetString(IDS_STICK_UNKNOWN);
	}

	return szName;
}

size_t ff::LegacyJoystick::GetButtonCount() const
{
	return _buttons;
}

bool ff::LegacyJoystick::GetButton(size_t nButton) const
{
	assertRetVal(nButton >= 0 && nButton < _buttons, false);
	return (_state.rgbButtons[nButton] & 0x80) != 0;
}

int ff::LegacyJoystick::GetButtonPressCount(size_t nButton) const
{
	assertRetVal(nButton >= 0 && nButton < _buttons, false);
	return _buttonPressed[nButton];
}

ff::String ff::LegacyJoystick::GetButtonName(size_t nButton) const
{
	return GetThisModule().GetFormattedString(IDS_BUTTON_NAME, nButton);
}

size_t ff::LegacyJoystick::GetTriggerCount() const
{
	return 0;
}

float ff::LegacyJoystick::GetTrigger(size_t nTrigger, bool bDigital) const
{
	assertRetVal(false, 0);
}

int ff::LegacyJoystick::GetTriggerPressCount(size_t nTrigger) const
{
	assertRetVal(false, 0);
}

ff::String ff::LegacyJoystick::GetTriggerName(size_t nTrigger) const
{
	return GetThisModule().GetString(IDS_TRIGGER_UNKNOWN);
}

bool ff::LegacyJoystick::HasSpecialButton(ESpecialJoystickButton button) const
{
	return false;
}

bool ff::LegacyJoystick::GetSpecialButton(ESpecialJoystickButton button) const
{
	assertRetVal(false, false);
}

int ff::LegacyJoystick::GetSpecialButtonPressCount(ESpecialJoystickButton button) const
{
	assertRetVal(false, 0);
}

ff::String ff::LegacyJoystick::GetSpecialButtonName(ESpecialJoystickButton button) const
{
	assertRetVal(false, String());
}

#endif
