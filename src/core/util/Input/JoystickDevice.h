#pragma once

#include "Input/InputDevice.h"

namespace ff
{
	enum ESpecialJoystickButton
	{
		JOYSTICK_BUTTON_START,
		JOYSTICK_BUTTON_BACK,

		JOYSTICK_BUTTON_LEFT_BUMPER,
		JOYSTICK_BUTTON_RIGHT_BUMPER,

		JOYSTICK_BUTTON_LEFT_STICK,
		JOYSTICK_BUTTON_RIGHT_STICK,

		JOYSTICK_BUTTON_A,
		JOYSTICK_BUTTON_B,
		JOYSTICK_BUTTON_X,
		JOYSTICK_BUTTON_Y,
	};

	class __declspec(uuid("f3ff5369-b610-4139-89cf-beda1cc4add0")) __declspec(novtable)
		IJoystickDevice : public IInputDevice
	{
	public:
		virtual size_t GetStickCount() const = 0;
		virtual PointFloat GetStickPos(size_t nStick, bool bDigital) const = 0;
		virtual RectInt GetStickPressCount(size_t nStick) const = 0;
		virtual String GetStickName(size_t nStick) const = 0;

		virtual size_t GetDPadCount() const = 0;
		virtual PointInt GetDPadPos(size_t nDPad) const = 0;
		virtual RectInt GetDPadPressCount(size_t nDPad) const = 0;
		virtual String GetDPadName(size_t nDPad) const = 0;

		virtual size_t GetButtonCount() const = 0;
		virtual bool GetButton(size_t nButton) const = 0;
		virtual int GetButtonPressCount(size_t nButton) const = 0;
		virtual String GetButtonName(size_t nButton) const = 0;

		virtual size_t GetTriggerCount() const = 0;
		virtual float GetTrigger(size_t nTrigger, bool bDigital) const = 0;
		virtual int GetTriggerPressCount(size_t nTrigger) const = 0;
		virtual String GetTriggerName(size_t nTrigger) const = 0;

		virtual bool HasSpecialButton(ESpecialJoystickButton button) const = 0;
		virtual bool GetSpecialButton(ESpecialJoystickButton button) const = 0;
		virtual int GetSpecialButtonPressCount(ESpecialJoystickButton button) const = 0;
		virtual String GetSpecialButtonName(ESpecialJoystickButton button) const = 0;
	};

#if !METRO_APP
	bool CreateLegacyJoystick(HWND hwnd, REFGUID instanceID, IJoystickDevice** device);
#endif

	bool CreateXboxJoystick(size_t nIndex, IJoystickDevice** device);
	size_t GetMaxXboxJoystickCount();
}
