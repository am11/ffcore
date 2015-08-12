#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/GlobalDirectInput.h"
#include "Input/JoystickDevice.h"
#include "Input/JoystickInput.h"

namespace ff
{
	class __declspec(uuid("3292c799-eb7a-4337-b2c2-87861ec1a03f"))
		JoystickInput : public ComBase, public IJoystickInput
	{
	public:
		DECLARE_HEADER(JoystickInput);

		bool Init(PWND hwnd, bool bAllowXInput);

		// IJoystickInput
		virtual void Advance() override;
		virtual void Reset() override;
		virtual PWND GetWindow() const override;

		virtual size_t GetCount() const override;
		virtual IJoystickDevice* GetJoystick(size_t nJoy) const override;

	private:
		MPWND _hwnd;
		bool _allowXInput;
		Vector<ComPtr<IJoystickDevice>> _joysticks;

#if !METRO_APP
		GlobalDirectInput _input;
#endif
	};
}

BEGIN_INTERFACES(ff::JoystickInput)
	HAS_INTERFACE(ff::IJoystickInput)
END_INTERFACES()

bool ff::CreateJoystickInput(PWND hwnd, bool bAllowXInput, IJoystickInput** ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;

	ComPtr<JoystickInput> pInput;
	assertRetVal(SUCCEEDED(ComAllocator<JoystickInput>::CreateInstance(&pInput)), false);
	assertRetVal(pInput->Init(hwnd, bAllowXInput), false);

	*ppInput = pInput.Detach();
	return true;
}

ff::JoystickInput::JoystickInput()
	: _hwnd(nullptr)
	, _allowXInput(true)
{
}

ff::JoystickInput::~JoystickInput()
{
}

#if !METRO_APP

struct CallbackData
{
	CallbackData(HWND hwnd, bool bAllowXInput, ff::GlobalDirectInput &directInput)
		: _hwnd(hwnd)
		, _xinputCount(0)
		, _allowXInput(bAllowXInput)
		, _directInput(directInput)
		{ }

	bool FoundAnyXboxControllers() const { return _xinputCount > 0; }

	ff::Vector<ff::ComPtr<ff::IJoystickDevice>> _joysticks;
	size_t _xinputCount;
	HWND _hwnd;
	bool _allowXInput;
	ff::GlobalDirectInput &_directInput;
};

static BOOL CALLBACK DeviceEnumCallback(LPCDIDEVICEINSTANCE pInstance, LPVOID pCookie)
{
	assertRetVal(pInstance && pCookie, DIENUM_STOP);

	CallbackData &data = *(CallbackData *)pCookie;

	if (pInstance->dwDevType & (DI8DEVTYPE_JOYSTICK | DI8DEVTYPE_GAMEPAD))
	{
		ff::ComPtr<ff::IJoystickDevice> pDevice;

		if (data._allowXInput && data._directInput.IsXInputDevice(pInstance->guidProduct))
		{
			data._xinputCount++;
		}
		else
		{
			verify(CreateLegacyJoystick(data._hwnd, pInstance->guidInstance, &pDevice));
			pDevice->SetName(ff::String(pInstance->tszInstanceName));
		}

		if (pDevice)
		{
			data._joysticks.Push(pDevice);
		}
	}

	return DIENUM_CONTINUE;
}

#endif // !METRO_APP

bool ff::JoystickInput::Init(PWND hwnd, bool bAllowXInput)
{
	_hwnd = hwnd;

#if !METRO_APP
	assertRetVal(hwnd && _input.GetInput(), false);

	_allowXInput = bAllowXInput;

	CallbackData data(hwnd, bAllowXInput, _input);
	assertRetVal(SUCCEEDED(_input.GetInput()->EnumDevices(DI8DEVCLASS_GAMECTRL, DeviceEnumCallback, &data, DIEDFL_ALLDEVICES)), false); // DIEDFL_ATTACHEDONLY

	_joysticks = data._joysticks;

	if (_allowXInput) // && data.FoundAnyXboxControllers())
#endif
	{
		for (DWORD i = 0; i < GetMaxXboxJoystickCount(); i++)
		{
			XINPUT_CAPABILITIES caps;
			ZeroObject(caps);
			DWORD result = XInputGetCapabilities(i, 0, &caps);

			if (result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED)
			{
				ComPtr<IJoystickDevice> pDevice;
				assertRetVal(CreateXboxJoystick(i, &pDevice), false);
				_joysticks.Push(pDevice);
			}
		}
	}

	return true;
}

void ff::JoystickInput::Advance()
{
	for (size_t i = 0; i < _joysticks.Size(); i++)
	{
		_joysticks[i]->Advance();
	}
}

void ff::JoystickInput::Reset()
{
	_joysticks.Clear();
	verify(Init(MemberToWindow(_hwnd), _allowXInput));
}

ff::PWND ff::JoystickInput::GetWindow() const
{
	return MemberToWindow(_hwnd);
}

size_t ff::JoystickInput::GetCount() const
{
	return _joysticks.Size();
}

ff::IJoystickDevice *ff::JoystickInput::GetJoystick(size_t nJoy) const
{
	assertRetVal(nJoy >= 0 && nJoy < _joysticks.Size(), nullptr);
	return _joysticks[nJoy];
}
