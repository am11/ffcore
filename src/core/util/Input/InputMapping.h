#pragma once

namespace ff
{
	class IInputDevice;
	class IInputEventListener;
	class IProxyInputEventListener;

	// Which device is being used?
	enum InputDevice : BYTE
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_DEVICE_NULL,
		INPUT_DEVICE_KEYBOARD,
		INPUT_DEVICE_MOUSE,
		INPUT_DEVICE_JOYSTICK,
	};

	// What is the user touching on the device?
	enum InputPart : BYTE
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_PART_NULL,
		INPUT_PART_BUTTON, // keyboard, mouse, or joystick
		INPUT_PART_STICK,
		INPUT_PART_DPAD,
		INPUT_PART_TRIGGER,
		INPUT_PART_TEXT, // on keyboard
		INPUT_PART_SPECIAL_BUTTON,
	};

	enum InputPartValue : BYTE
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_VALUE_NULL,
		INPUT_VALUE_PRESSED,
		INPUT_VALUE_X_AXIS, // to get the value of the joystick axis
		INPUT_VALUE_Y_AXIS,
		INPUT_VALUE_LEFT, // pressing the joystick in a certain direction
		INPUT_VALUE_RIGHT,
		INPUT_VALUE_UP,
		INPUT_VALUE_DOWN,
		INPUT_VALUE_TEXT, // string typed on the keyboard
	};

	enum InputPartIndex : BYTE
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_INDEX_LEFT_STICK    = 0,
		INPUT_INDEX_RIGHT_STICK   = 1,

		INPUT_INDEX_LEFT_TRIGGER  = 0,
		INPUT_INDEX_RIGHT_TRIGGER = 1,

		INPUT_INDEX_ANY_BUTTON    = 0xFF,
	};

	// A single thing that the user can press
	struct InputAction
	{
		InputDevice _device;
		InputPart _part;
		InputPartValue _partValue;
		BYTE _partIndex; // which button or stick?
	};

	// Maps actions to an event
	struct InputEventMapping
	{
		InputAction _actions[4]; // up to four (unused actions must be zeroed out)
		Atom _eventID;
		double _holdSeconds;
		double _repeatSeconds;
	};

	// Allows direct access to any input value
	struct InputValueMapping
	{
		InputAction _action;
		Atom _valueID;
	};

	// Sent whenever an InputEventMapping gets triggered (or released)
	struct InputEvent
	{
		Atom _eventID;
		int _count;

		bool IsStart()  const { return _count == 1; }
		bool IsRepeat() const { return _count >  1; }
		bool IsStop()   const { return _count == 0; }
	};

	class __declspec(uuid("53800686-1b66-4256-836a-bdce1b1b2f0b")) __declspec(novtable)
		IInputMapping : public IUnknown
	{
	public:
		virtual void Advance(double deltaTime) = 0;
		virtual void Reset() = 0;

		virtual bool MapEvents(const InputEventMapping *pMappings, size_t nCount) = 0;
		virtual bool MapValues(const InputValueMapping *pMappings, size_t nCount) = 0;

		virtual bool GetMappedEvents(Vector<InputEventMapping> &mappings) const = 0;
		virtual bool GetMappedValues(Vector<InputValueMapping> &mappings) const = 0;

		virtual bool GetMappedEvents(Atom eventID, Vector<InputEventMapping> &mappings) const = 0;
		virtual bool GetMappedValues(Atom valueID, Vector<InputValueMapping> &mappings) const = 0;

		virtual bool Save(IData **ppData) const = 0;
		virtual bool Load(IData *pData) = 0;

		// These must be called between calls to Advance() to find out what happened
		virtual size_t GetEventCount() const = 0;
		virtual const InputEvent *GetEvent(size_t nIndex) const = 0;
		virtual float GetEventProgress(Atom eventID) const = 0; // 1=triggered once, 2=hold time hit twice, etc...
		virtual void ClearEvents() = 0;

		// Gets immediate values
		virtual int GetDigitalValue(Atom valueID) const = 0;
		virtual float GetAnalogValue (Atom valueID) const = 0;
		virtual String GetStringValue (Atom valueID) const = 0;

		// Listeners
		virtual void AddListener(IInputEventListener *pListener) = 0;
		virtual bool AddProxyListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy) = 0;
		virtual bool RemoveListener(IInputEventListener *pListener) = 0;
	};

	UTIL_API bool CreateInputMapping(IInputDevice** ppDevices, size_t nDevices, IInputMapping** ppMapping);

	class __declspec(uuid("7e363dc0-78b6-49e3-bd7a-0dde743e9393")) __declspec(novtable)
		IInputEventListener : public IUnknown
	{
	public:
		virtual void OnInputEvents(IInputMapping *pMapping, const InputEvent *pEvents, size_t nCount) = 0;
	};

	class __declspec(uuid("063acc05-326a-44a6-b9c8-671d84d567be")) __declspec(novtable)
		IProxyInputEventListener : public IInputEventListener
	{
	public:
		// Must call SetOwner(nullptr) when the owner is destroyed
		virtual void SetOwner(IInputEventListener *pOwner) = 0;
	};

	bool CreateProxyInputEventListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy);
}
