#pragma once

#include "Input/InputDevice.h"
#include "Windows/WinUtil.h"

namespace ff
{
	enum class TouchType
	{
		TOUCH_TYPE_NONE,
		TOUCH_TYPE_MOUSE,
		TOUCH_TYPE_FINGER,
		TOUCH_TYPE_PEN,
	};

	class __declspec(uuid("81921d08-adbc-4832-bd1d-a0587c3af12b")) __declspec(novtable)
		ITouchDevice : public IInputDevice
	{
	public:
		virtual PWND GetWindow() const = 0;
		virtual float GetDpiScale() const = 0;
		virtual void Reset() = 0;

		virtual size_t GetTouchCount() const = 0;
		virtual TouchType GetTouchType(size_t index) const = 0;
		virtual PointFloat GetTouchPos(size_t index) const = 0;
	};

	UTIL_API bool CreateTouchDevice(PWND hwnd, ITouchDevice** ppInput);
}
