#pragma once

#include "Input/InputDevice.h"
#include "Windows/WinUtil.h"

namespace ff
{
	class __declspec(uuid("dc258f87-ba84-4b7a-b75a-3ce57ad23dc9")) __declspec(novtable)
		IMouseDevice : public IInputDevice
	{
	public:
		virtual PWND GetWindow() const = 0;
		virtual bool IsInWindow() const = 0;
		virtual PointFloat GetPos() const = 0; // in window coordinates
		virtual PointFloat GetRelativePos() const = 0; // since last Advance()
		virtual float GetDpiScale() const = 0;

		virtual bool GetButton(int vkButton) const = 0;
		virtual int GetButtonClickCount(int vkButton) const = 0; // since the last Advance()
		virtual int GetButtonDoubleClickCount(int vkButton) const = 0; // since the last Advance()
		virtual PointFloat GetWheelScroll() const = 0; // since the last Advance()
	};

	UTIL_API bool CreateMouseDevice(PWND hwnd, IMouseDevice** ppInput);
}
