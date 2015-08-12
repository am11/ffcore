#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/TouchDevice.h"
#include "Module/Module.h"
#include "Resource/util-resource.h"

#if !METRO_APP

namespace ff
{
	class __declspec(uuid("34fe65c2-5bb9-492e-ab5a-f4d21bdba6fb"))
		TouchDevice
		: public ComBase
		, public ITouchDevice
	{
	public:
		DECLARE_HEADER(TouchDevice);

		bool Init(HWND window);

		// IInputDevice
		virtual void Advance() override;
		virtual String GetName() const override;
		virtual void SetName(StringRef name) override;
		virtual bool IsConnected() const override;

		// ITouchDevice
		virtual PWND GetWindow() const override;
		virtual float GetDpiScale() const override;
		virtual void Reset() override;
		virtual size_t GetTouchCount() const override;
		virtual TouchType GetTouchType(size_t index) const override;
		virtual PointFloat GetTouchPos(size_t index) const override;

	private:
		HWND _window;
	};
}

BEGIN_INTERFACES(ff::TouchDevice)
	HAS_INTERFACE(ff::ITouchDevice)
	HAS_INTERFACE(ff::IInputDevice)
END_INTERFACES()

// STATIC_DATA (object)
static ff::Mutex s_deviceCS;
static ff::Vector<ff::TouchDevice*, 8> s_allTouchDevices;

// static
bool ff::CreateTouchDevice(HWND window, ITouchDevice **ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;
	{
		LockMutex crit(s_deviceCS);

		for (size_t i = 0; i < s_allTouchDevices.Size(); i++)
		{
			if (s_allTouchDevices[i]->GetWindow() == window)
			{
				*ppInput = GetAddRef(s_allTouchDevices[i]);
				return true;
			}
		}
	}

	ComPtr<TouchDevice> pInput;
	assertRetVal(SUCCEEDED(ComAllocator<TouchDevice>::CreateInstance(&pInput)), false);
	assertRetVal(pInput->Init(window), false);

	*ppInput = pInput.Detach();
	return true;
}

ff::TouchDevice::TouchDevice()
	: _window(nullptr)
{
	LockMutex crit(s_deviceCS);
	s_allTouchDevices.Push(this);
}

ff::TouchDevice::~TouchDevice()
{
	LockMutex crit(s_deviceCS);
	s_allTouchDevices.Delete(s_allTouchDevices.Find(this));

	if (!s_allTouchDevices.Size())
	{
		s_allTouchDevices.Reduce();
	}
}

bool ff::TouchDevice::Init(HWND window)
{
	assertRetVal(window, false);
	_window = window;

	return true;
}

void ff::TouchDevice::Advance()
{
}

ff::String ff::TouchDevice::GetName() const
{
	return GetThisModule().GetString(IDS_TOUCH_NAME);
}

void ff::TouchDevice::SetName(StringRef name)
{
}

bool ff::TouchDevice::IsConnected() const
{
	return false;
}

ff::PWND ff::TouchDevice::GetWindow() const
{
	return _window;
}

float ff::TouchDevice::GetDpiScale() const
{
	return 1;
}

void ff::TouchDevice::Reset()
{
}

size_t ff::TouchDevice::GetTouchCount() const
{
	return 0;
}

ff::TouchType ff::TouchDevice::GetTouchType(size_t index) const
{
	assertRetVal(false, TouchType::TOUCH_TYPE_NONE);
}

ff::PointFloat ff::TouchDevice::GetTouchPos(size_t index) const
{
	assertRetVal(false, PointFloat(0, 0));
}

#endif // !METRO_APP
