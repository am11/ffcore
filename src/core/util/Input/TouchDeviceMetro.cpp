#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/PointerListener.h"
#include "Input/TouchDevice.h"
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
	class __declspec(uuid("a0689bfc-ad2f-41c0-a779-766c0ac0ca73"))
		TouchDevice
		: public ComBase
		, public ITouchDevice
		, public IPointerEventListener
	{
	public:
		DECLARE_HEADER(TouchDevice);

		bool Init(CoreWindow ^window);
		void Destroy();

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

		// IPointerEventListener
		virtual void OnPointerEntered(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerExited(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerMoved(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerPressed(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerReleased(CoreWindow ^sender, PointerEventArgs ^args) override;
		virtual void OnPointerCaptureLost(CoreWindow ^sender, PointerEventArgs ^args) override;

	private:
		struct InternalTouchInfo
		{
			PointerPoint ^point;
		};

		InternalTouchInfo *FindTouchInfo(PointerPoint ^point);

		Agile<CoreWindow> _window;
		PointerEvents ^_events;
		Vector<InternalTouchInfo> _touches;
		Vector<InternalTouchInfo> _pendingTouches;
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
bool ff::CreateTouchDevice(CoreWindow ^window, ITouchDevice **ppInput)
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
{
	LockMutex crit(s_deviceCS);
	s_allTouchDevices.Push(this);
}

ff::TouchDevice::~TouchDevice()
{
	Destroy();

	LockMutex crit(s_deviceCS);
	s_allTouchDevices.Delete(s_allTouchDevices.Find(this));

	if (!s_allTouchDevices.Size())
	{
		s_allTouchDevices.Reduce();
	}
}

bool ff::TouchDevice::Init(CoreWindow ^window)
{
	assertRetVal(window, false);
	_window = window;
	_events = ref new PointerEvents(this, window);

	return true;
}

void ff::TouchDevice::Destroy()
{
	if (_events != nullptr)
	{
		_events->Destroy();
		_events = nullptr;
	}
}

void ff::TouchDevice::Advance()
{
	_touches = _pendingTouches;
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
	return true;
}

ff::PWND ff::TouchDevice::GetWindow() const
{
	return MemberToWindow(_window);
}

float ff::TouchDevice::GetDpiScale() const
{
	return _events->GetDpiScale();
}

void ff::TouchDevice::Reset()
{
	_touches.Clear();
}

size_t ff::TouchDevice::GetTouchCount() const
{
	return _touches.Size();
}

ff::TouchType ff::TouchDevice::GetTouchType(size_t index) const
{
	assertRetVal(index < _touches.Size(), TouchType::TOUCH_TYPE_NONE);

	switch (_touches[index].point->PointerDevice->PointerDeviceType)
	{
	case Windows::Devices::Input::PointerDeviceType::Mouse:
		return TouchType::TOUCH_TYPE_MOUSE;

	case Windows::Devices::Input::PointerDeviceType::Pen:
		return TouchType::TOUCH_TYPE_PEN;

	case Windows::Devices::Input::PointerDeviceType::Touch:
		return TouchType::TOUCH_TYPE_FINGER;

	default:
		return TouchType::TOUCH_TYPE_NONE;
	}
}

ff::PointFloat ff::TouchDevice::GetTouchPos(size_t index) const
{
	assertRetVal(index < _touches.Size(), PointFloat(0, 0));

	Point pos = _touches[index].point->Position;

	return PointFloat(
		pos.X * _events->GetDpiScale(),
		pos.Y * _events->GetDpiScale());
}

void ff::TouchDevice::OnPointerEntered(CoreWindow ^sender, PointerEventArgs ^args)
{
}

void ff::TouchDevice::OnPointerExited(CoreWindow ^sender, PointerEventArgs ^args)
{
	OnPointerReleased(sender, args);
}

void ff::TouchDevice::OnPointerMoved(CoreWindow ^sender, PointerEventArgs ^args)
{
	PointerPoint ^point = args->CurrentPoint;
	InternalTouchInfo *info = FindTouchInfo(point);

	if (info != nullptr)
	{
		info->point = point;

		// Log::DebugTraceF(L"*** Moved touch: %u\n", point->PointerId);
	}
}

void ff::TouchDevice::OnPointerPressed(CoreWindow ^sender, PointerEventArgs ^args)
{
	PointerPoint ^point = args->CurrentPoint;
	InternalTouchInfo *info = FindTouchInfo(point);

	if (info != nullptr)
	{
		assert(false);
		info->point = point;
	}
	else
	{
		InternalTouchInfo newInfo;
		newInfo.point = point;
		_pendingTouches.Push(newInfo);

		// Log::DebugTraceF(L"*** New touch: %u\n", point->PointerId);
	}
}

void ff::TouchDevice::OnPointerReleased(CoreWindow ^sender, PointerEventArgs ^args)
{
	PointerPoint ^point = args->CurrentPoint;
	InternalTouchInfo *info = FindTouchInfo(point);

	if (info != nullptr)
	{
		_pendingTouches.Delete(info - _pendingTouches.Data());

		// Log::DebugTraceF(L"*** Deleted touch: %u\n", point->PointerId);
	}
}

void ff::TouchDevice::OnPointerCaptureLost(CoreWindow ^sender, PointerEventArgs ^args)
{
	OnPointerReleased(sender, args);
}

ff::TouchDevice::InternalTouchInfo *ff::TouchDevice::FindTouchInfo(PointerPoint ^point)
{
	for (size_t i = 0; i < _pendingTouches.Size(); i++)
	{
		if (point->PointerId == _pendingTouches[i].point->PointerId)
		{
			return &_pendingTouches[i];
		}
	}

	return nullptr;
}

#endif // METRO_APP
