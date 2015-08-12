#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/KeyboardDevice.h"
#include "Module/Module.h"
#include "Resource/util-resource.h"

#if METRO_APP

using namespace Platform;
using namespace Windows::Devices::Input;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;

namespace ff
{
	class __declspec(uuid("12896528-8ff4-491c-98fd-527c1640e578"))
		KeyboardDevice : public ComBase, public IKeyboardDevice
	{
	public:
		DECLARE_HEADER(KeyboardDevice);

		bool Init(CoreWindow ^window);
		void Destroy();

		// IInputDevice
		virtual void Advance() override;
		virtual String GetName() const override;
		virtual void SetName(StringRef name) override;
		virtual bool IsConnected() const override;

		// IKeyboardDevice
		virtual PWND GetWindow() const override;
		virtual bool GetKey(int vk) const override;
		virtual int GetKeyPressCount(int vk) const override;
		virtual String GetChars() const override;
		virtual void Disable() override;
		virtual void Enable() override;

	private:
		ref class KeyEvents
		{
		internal:
			KeyEvents(KeyboardDevice *pParent);

		public:
			virtual ~KeyEvents();

			void Destroy();

		private:
			void OnCharacterReceived(CoreWindow ^sender, CharacterReceivedEventArgs ^args);
			void OnKeyDown(CoreWindow ^sender, KeyEventArgs ^args);
			void OnKeyUp(CoreWindow ^sender, KeyEventArgs ^args);

			KeyboardDevice *m_parent;
			Windows::Foundation::EventRegistrationToken m_tokens[3];
		};

		void OnCharacterReceived(CoreWindow ^sender, CharacterReceivedEventArgs ^args);
		void OnKeyDown(CoreWindow ^sender, KeyEventArgs ^args);
		void OnKeyUp(CoreWindow ^sender, KeyEventArgs ^args);

		Agile<CoreWindow> _window;
		KeyEvents ^_events;
		BYTE _keys[256];
		BYTE _presses[256];
		BYTE _pressesPending[256];
		String _text;
		String _textPending;
		int _disabled;
	};
}

BEGIN_INTERFACES(ff::KeyboardDevice)
	HAS_INTERFACE(ff::IKeyboardDevice)
	HAS_INTERFACE(ff::IInputDevice)
END_INTERFACES()

ff::KeyboardDevice::KeyEvents::KeyEvents(KeyboardDevice *pParent)
	: m_parent(pParent)
{
	m_tokens[0] = m_parent->_window->CharacterReceived += ref new TypedEventHandler<CoreWindow^, CharacterReceivedEventArgs^>(this, &KeyEvents::OnCharacterReceived);
	m_tokens[1] = m_parent->_window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &KeyEvents::OnKeyDown);
	m_tokens[2] = m_parent->_window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &KeyEvents::OnKeyUp);
}

ff::KeyboardDevice::KeyEvents::~KeyEvents()
{
	Destroy();
}

void ff::KeyboardDevice::KeyEvents::Destroy()
{
	m_parent->_window->CharacterReceived -= m_tokens[0];
	m_parent->_window->KeyDown -= m_tokens[1];
	m_parent->_window->KeyUp -= m_tokens[2];
}

void ff::KeyboardDevice::KeyEvents::OnCharacterReceived(CoreWindow ^sender, CharacterReceivedEventArgs ^args)
{
	m_parent->OnCharacterReceived(sender, args);
}

void ff::KeyboardDevice::KeyEvents::OnKeyDown(CoreWindow ^sender, KeyEventArgs ^args)
{
	m_parent->OnKeyDown(sender, args);
}

void ff::KeyboardDevice::KeyEvents::OnKeyUp(CoreWindow ^sender, KeyEventArgs ^args)
{
	m_parent->OnKeyUp(sender, args);
}

// STATIC_DATA (object)
static ff::Mutex s_deviceCS;
static ff::Vector<ff::KeyboardDevice*, 8> s_allKeyInputs;

// static
bool ff::CreateKeyboardDevice(PWND hwnd, IKeyboardDevice** ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;
	{
		LockMutex crit(s_deviceCS);

		for (size_t i = 0; i < s_allKeyInputs.Size(); i++)
		{
			if (s_allKeyInputs[i]->GetWindow() == hwnd)
			{
				*ppInput = GetAddRef(s_allKeyInputs[i]);
				return true;
			}
		}
	}

	ComPtr<KeyboardDevice> pInput;
	assertRetVal(SUCCEEDED(ComAllocator<KeyboardDevice>::CreateInstance(&pInput)), false);
	assertRetVal(pInput->Init(hwnd), false);

	*ppInput = pInput.Detach();
	return true;
}

ff::KeyboardDevice::KeyboardDevice()
	: _disabled(0)
{
	ZeroObject(_keys);
	ZeroObject(_presses);
	ZeroObject(_pressesPending);

	LockMutex crit(s_deviceCS);
	s_allKeyInputs.Push(this);
}


ff::KeyboardDevice::~KeyboardDevice()
{
	Destroy();

	LockMutex crit(s_deviceCS);
	s_allKeyInputs.Delete(s_allKeyInputs.Find(this));

	if (!s_allKeyInputs.Size())
	{
		s_allKeyInputs.Reduce();
	}
}

bool ff::KeyboardDevice::Init(PWND window)
{
	assertRetVal(window, false);
	_window = window;
	_events = ref new KeyEvents(this);

	return true;
}

void ff::KeyboardDevice::Destroy()
{
	if (_events)
	{
		_events->Destroy();
		_events = nullptr;
	}
}

void ff::KeyboardDevice::Advance()
{
	_text = _textPending;
	_textPending.clear();

	CopyMemory(_presses, _pressesPending, sizeof(_presses));
	ZeroObject(_pressesPending);
}

ff::String ff::KeyboardDevice::GetName() const
{
	return ff::GetThisModule().GetString(IDS_KEYBOARD_NAME);
}

void ff::KeyboardDevice::SetName(StringRef name)
{
}

bool ff::KeyboardDevice::IsConnected() const
{
	return true;
}

ff::PWND ff::KeyboardDevice::GetWindow() const
{
	return _window.Get();
}

bool ff::KeyboardDevice::GetKey(int vk) const
{
	assert(vk >= 0 && vk < _countof(_keys));
	return _keys[vk] != 0;
}

int ff::KeyboardDevice::GetKeyPressCount(int vk) const
{
	assert(vk >= 0 && vk < _countof(_presses));
	return (int)_presses[vk];
}

ff::String ff::KeyboardDevice::GetChars() const
{
	return _text;
}

void ff::KeyboardDevice::Disable()
{
	_disabled++;
}

void ff::KeyboardDevice::Enable()
{
	assertRet(_disabled > 0);
	_disabled--;
}

void ff::KeyboardDevice::OnCharacterReceived(CoreWindow ^sender, CharacterReceivedEventArgs ^args)
{
	if (!_disabled)
	{
		wchar_t ch = (wchar_t)args->KeyCode;
		_textPending.append(1, ch);
	}
}

void ff::KeyboardDevice::OnKeyDown(CoreWindow ^sender, KeyEventArgs ^args)
{
	if (!_disabled)
	{
		int vk = (int)args->VirtualKey;

		if (vk >= 0 && vk < _countof(_pressesPending) && _pressesPending[vk] != 0xFF)
		{
			_pressesPending[vk]++;
		}

		if (vk >= 0 && vk < _countof(_keys))
		{
			_keys[vk] = true;
		}
	}
}

void ff::KeyboardDevice::OnKeyUp(CoreWindow ^sender, KeyEventArgs ^args)
{
	if (!_disabled)
	{
		int vk = (int)args->VirtualKey;

		if (vk >= 0 && vk < _countof(_keys))
		{
			_keys[vk] = false;
		}
	}
}

#endif
