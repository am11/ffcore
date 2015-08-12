#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Input/InputMapping.h"
#include "Input/JoystickDevice.h"
#include "Input/KeyboardDevice.h"
#include "Input/MouseDevice.h"

namespace ff
{
	class IJoystickDevice;
	class IKeyboardDevice;
	class IMouseDevice;

	class __declspec(uuid("d1dee57f-337b-49d6-9608-c69bbbce4367"))
		CInputMapping : public ComBase, public IInputMapping
	{
	public:
		DECLARE_HEADER(CInputMapping);

		bool Init(IInputDevice** ppDevices, size_t nDevices);

		// IInputMapping

		virtual void Advance(double deltaTime) override;
		virtual void Reset() override;

		virtual bool MapEvents(const InputEventMapping *pMappings, size_t nCount) override;
		virtual bool MapValues(const InputValueMapping *pMappings, size_t nCount) override;

		virtual bool GetMappedEvents(Vector<InputEventMapping> &mappings) const override;
		virtual bool GetMappedValues(Vector<InputValueMapping> &mappings) const override;

		virtual bool GetMappedEvents(Atom eventID, Vector<InputEventMapping> &mappings) const override;
		virtual bool GetMappedValues(Atom valueID, Vector<InputValueMapping> &mappings) const override;

		virtual bool Save(IData **ppData) const override;
		virtual bool Load(IData *pData) override;

		virtual size_t            GetEventCount() const override;
		virtual const InputEvent* GetEvent(size_t nIndex) const override;
		virtual float             GetEventProgress(Atom eventID) const override;
		virtual void              ClearEvents() override;

		virtual int     GetDigitalValue(Atom valueID) const override;
		virtual float   GetAnalogValue (Atom valueID) const override;
		virtual String GetStringValue (Atom valueID) const override;

		virtual void AddListener     (IInputEventListener *pListener) override;
		virtual bool AddProxyListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy) override;
		virtual bool RemoveListener  (IInputEventListener *pListener) override;

	private:
		int     GetDigitalValue(const InputAction &action, int *pPressCount) const;
		float   GetAnalogValue (const InputAction &action, bool bForDigital) const;
		String GetStringValue (const InputAction &action) const;

		struct InputEventMappingInfo : public InputEventMapping
		{
			double _holdingSeconds;
			int _eventCount;
			bool _holding;
		};

		void PushStartEvent(InputEventMappingInfo &info);
		void PushStopEvent (InputEventMappingInfo &info);

		// Raw user input
		Vector<ComPtr<IKeyboardDevice>> _keys;
		Vector<ComPtr<IMouseDevice>> _mice;
		Vector<ComPtr<IJoystickDevice>> _joys;

		Vector<InputEventMappingInfo> _eventMappings;
		Vector<InputValueMapping> _valueMappings;
		Vector<InputEvent> _currentEvents;

		Map<Atom, size_t> _eventToInfo;
		Map<Atom, size_t> _valueToInfo;

		typedef Vector<ComPtr<IInputEventListener>> ListenerVector;
		typedef SharedObject<ListenerVector> SharedListenerVector;
		SmartPtr<SharedListenerVector> _listeners;
	};

	bool CreateInputMapping(IInputDevice** ppDevices, size_t nDevices, IInputMapping** ppMapping)
	{
		assertRetVal(ppMapping, false);
		*ppMapping = nullptr;

		ComPtr<CInputMapping> pMapping;
		assertRetVal(SUCCEEDED(ComAllocator<CInputMapping>::CreateInstance(&pMapping)), false);
		assertRetVal(pMapping->Init(ppDevices, nDevices), false);

		*ppMapping = pMapping.Detach();
		return true;
	}

	class __declspec(uuid("d3bdf49d-1e6b-43ed-95d2-99f924170bfa"))
	CProxyInputEventListener : public ComBase, public IProxyInputEventListener
	{
	public:
		DECLARE_HEADER(CProxyInputEventListener);

		// IProxyInputEventListener
		virtual void SetOwner(IInputEventListener *pOwner) override;

		// IInputEventListener
		virtual void OnInputEvents(IInputMapping *pMapping, const InputEvent *pEvents, size_t nCount) override;

	private:
		IInputEventListener *_owner;
	};

	bool CreateProxyInputEventListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy)
	{
		assertRetVal(pListener && ppProxy, false);

		ComPtr<CProxyInputEventListener> pProxy;
		assertRetVal(SUCCEEDED(ComAllocator<CProxyInputEventListener>::CreateInstance(&pProxy)), false);
		pProxy->SetOwner(pListener);

		*ppProxy = pProxy.Detach();
		return true;
	}
}

BEGIN_INTERFACES(ff::CInputMapping)
	HAS_INTERFACE(ff::IInputMapping)
END_INTERFACES()

BEGIN_INTERFACES(ff::CProxyInputEventListener)
	HAS_INTERFACE(ff::IInputEventListener)
	HAS_INTERFACE(ff::IProxyInputEventListener)
END_INTERFACES()

ff::CProxyInputEventListener::CProxyInputEventListener()
	: _owner(nullptr)
{
}

ff::CProxyInputEventListener::~CProxyInputEventListener()
{
	assert(!_owner);
}

void ff::CProxyInputEventListener::SetOwner(IInputEventListener *pOwner)
{
	_owner = pOwner;
}

void ff::CProxyInputEventListener::OnInputEvents(IInputMapping *pMapping, const InputEvent *pEvents, size_t nCount)
{
	if (_owner)
	{
		_owner->OnInputEvents(pMapping, pEvents, nCount);
	}
}

ff::CInputMapping::CInputMapping()
{
}


ff::CInputMapping::~CInputMapping()
{
}

bool ff::CInputMapping::Init(IInputDevice** ppDevices, size_t nDevices)
{
	assertRetVal(!nDevices || ppDevices, false);

	for (size_t i = 0; i < nDevices; i++)
	{
		ComPtr<IKeyboardDevice> pKey;
		ComPtr<IMouseDevice>    pMouse;
		ComPtr<IJoystickDevice> pJoy;

		if (pKey.QueryFrom(ppDevices[i]))
		{
			_keys.Push(pKey);
		}

		if (pMouse.QueryFrom(ppDevices[i]))
		{
			_mice.Push(pMouse);
		}

		if (pJoy.QueryFrom(ppDevices[i]))
		{
			_joys.Push(pJoy);
		}
	}

	return true;
}

void ff::CInputMapping::Advance(double deltaTime)
{
	ClearEvents();

	for (size_t i = 0; i < _eventMappings.Size(); i++)
	{
		InputEventMappingInfo &info = _eventMappings[i];

		bool bStillHolding = true;
		int  nTriggerCount = 0;

		// Check each required button that needs to be pressed to trigger the current action
		for (size_t h = 0; h < _countof(info._actions) && info._actions[h]._device != INPUT_DEVICE_NULL; h++)
		{
			int nCurTriggerCount = 0;
			int nCurValue = GetDigitalValue(info._actions[h], &nCurTriggerCount);

			nTriggerCount = h ? std::min(nTriggerCount, nCurTriggerCount) : nCurTriggerCount;

			if (!nCurValue)
			{
				bStillHolding = false;
			}
		}

		for (int h = 0; h < nTriggerCount; h++)
		{
			if (info._eventCount > 0)
			{
				PushStopEvent(info);
			}

			info._holding = true;

			if (deltaTime / nTriggerCount >= info._holdSeconds)
			{
				PushStartEvent(info);
			}
		}

		if (info._holding)
		{
			if (!bStillHolding)
			{
				PushStopEvent(info);
			}
			else if (!nTriggerCount)
			{
				info._holdingSeconds += deltaTime;

				double holdTime = (info._holdingSeconds - info._holdSeconds);

				if (holdTime >= 0)
				{
					int totalEvents = 1;

					if (info._repeatSeconds > 0)
					{
						totalEvents += (int)floor(holdTime / info._repeatSeconds);
					}

					while (info._eventCount < totalEvents)
					{
						PushStartEvent(info);
					}
				}
			}
		}
	}

	if (_currentEvents.Size())
	{
#if !METRO_APP
		// The user is doing something, tell Windows not to go to sleep (mostly for joysticks)
		SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
#endif
		// Tell listeners about the input events
		SmartPtr<SharedListenerVector> listeners = _listeners;
		if (listeners != nullptr)
		{
			for (const auto &listener: *listeners.Object())
			{
				listener->OnInputEvents(this, _currentEvents.Data(), _currentEvents.Size());
			}
		}
	}
}

void ff::CInputMapping::Reset()
{
	ClearEvents();

	_eventMappings.Clear();
	_valueMappings.Clear();
	_eventToInfo.Clear();
	_valueToInfo.Clear();
}

bool ff::CInputMapping::MapEvents(const InputEventMapping *pMappings, size_t nCount)
{
	bool status = true;

	_eventMappings.Reserve(_eventMappings.Size() + nCount);

	for (size_t i = 0; i < nCount; i++)
	{
		InputEventMappingInfo info;
		ZeroObject(info);

		CopyMemory(&info, &pMappings[i], sizeof(pMappings[i]));

		_eventMappings.Push(info);
		_eventToInfo.Insert(info._eventID, _eventMappings.Size() - 1);
	}

	return status;
}

bool ff::CInputMapping::MapValues(const InputValueMapping *pMappings, size_t nCount)
{
	bool status = true;

	_valueMappings.Reserve(_valueMappings.Size() + nCount);

	for (size_t i = 0; i < nCount; i++)
	{
		_valueMappings.Push(pMappings[i]);
		_valueToInfo.Insert(pMappings[i]._valueID, _valueMappings.Size() - 1);
	}

	return status;
}

bool ff::CInputMapping::GetMappedEvents(Vector<InputEventMapping> &mappings) const
{
	mappings.Resize(_eventMappings.Size());

	for (size_t i = 0; i < _eventMappings.Size(); i++)
	{
		mappings[i] = _eventMappings[i];
	}

	return !mappings.IsEmpty();
}

bool ff::CInputMapping::GetMappedValues(Vector<InputValueMapping> &mappings) const
{
	mappings = _valueMappings;

	return !mappings.IsEmpty();
}

bool ff::CInputMapping::GetMappedEvents(Atom eventID, Vector<InputEventMapping> &mappings) const
{
	mappings.Clear();

	for (BucketIter i = _eventToInfo.Get(eventID); i != INVALID_ITER; i = _eventToInfo.GetNext(i))
	{
		mappings.Push(_eventMappings[_eventToInfo.ValueAt(i)]);
	}

	return !mappings.IsEmpty();
}

bool ff::CInputMapping::GetMappedValues(Atom valueID, Vector<InputValueMapping> &mappings) const
{
	mappings.Clear();

	for (BucketIter i = _valueToInfo.Get(valueID); i != INVALID_ITER; i = _valueToInfo.GetNext(i))
	{
		mappings.Push(_valueMappings[_valueToInfo.ValueAt(i)]);
	}

	return !mappings.IsEmpty();
}

bool ff::CInputMapping::Save(IData **ppData) const
{
	assertRetVal(ppData, false);

	ComPtr<IDataVector> pData;
	ComPtr<IDataWriter> pWriter;
	assertRetVal(CreateDataWriter(&pData, &pWriter), false);

	DWORD nVersion = 0;
	assertRetVal(SaveData(pWriter, nVersion), false);

	DWORD nSize = (DWORD)_eventMappings.Size();
	assertRetVal(SaveData(pWriter, nSize), false);

	for (size_t i = 0; i < nSize; i++)
	{
		InputEventMapping info = _eventMappings[i];
		assertRetVal(SaveData(pWriter, info), false);
	}

	nSize = (DWORD)_valueMappings.Size();
	assertRetVal(SaveData(pWriter, nSize), false);

	if (nSize)
	{
		assertRetVal(SaveBytes(pWriter, _valueMappings.Data(), _valueMappings.ByteSize()), false);
	}

	*ppData = pData.Detach();
	return true;
}

bool ff::CInputMapping::Load(IData *pData)
{
	assertRetVal(pData, false);

	ComPtr<IDataReader> pReader;
	assertRetVal(CreateDataReader(pData, 0, &pReader), false);

	DWORD nVersion = 0;
	assertRetVal(LoadData(pReader, nVersion), false);
	assertRetVal(nVersion == 0, false);

	DWORD nSize = 0;
	assertRetVal(LoadData(pReader, nSize), false);

	if (nSize)
	{
		Vector<InputEventMapping> mappings;
		mappings.Resize(nSize);

		assertRetVal(LoadBytes(pReader, mappings.Data(), mappings.ByteSize()), false);
		assertRetVal(MapEvents(mappings.Data(), mappings.Size()), false);
	}

	assertRetVal(LoadData(pReader, nSize), false);

	if (nSize)
	{
		Vector<InputValueMapping> mappings;
		mappings.Resize(nSize);

		assertRetVal(LoadBytes(pReader, mappings.Data(), mappings.ByteSize()), false);
		assertRetVal(MapValues(mappings.Data(), mappings.Size()), false);
	}

	return true;
}

size_t ff::CInputMapping::GetEventCount() const
{
	return _currentEvents.Size();
}

const ff::InputEvent *ff::CInputMapping::GetEvent(size_t nIndex) const
{
	assertRetVal(nIndex >= 0 && nIndex < _currentEvents.Size(), nullptr);
	return &_currentEvents[nIndex];
}

float ff::CInputMapping::GetEventProgress(Atom eventID) const
{
	double ret = 0;

	for (BucketIter i = _eventToInfo.Get(eventID); i != INVALID_ITER; i = _eventToInfo.GetNext(i))
	{
		const InputEventMappingInfo &info = _eventMappings[_eventToInfo.ValueAt(i)];

		if (info._holding)
		{
			double val = 1;

			if (info._holdingSeconds < info._holdSeconds)
			{
				// from 0 to 1
				val = info._holdingSeconds / info._holdSeconds;
			}
			else if (info._repeatSeconds > 0)
			{
				// from 1 to N, using repeat count
				val = (info._holdingSeconds - info._holdSeconds) / info._repeatSeconds + 1.0;
			}
			else if (info._holdSeconds > 0)
			{
				// from 1 to N, using the original hold time as the repeat time
				val = info._holdingSeconds / info._holdSeconds;
			}

			// Can only return one value, so choose the largest
			if (val > ret)
			{
				ret = val;
			}
		}
	}

	return (float)ret;
}

void ff::CInputMapping::ClearEvents()
{
	_currentEvents.Clear();
}

int ff::CInputMapping::GetDigitalValue(Atom valueID) const
{
	int ret = 0;

	for (BucketIter i = _valueToInfo.Get(valueID); i != INVALID_ITER; i = _valueToInfo.GetNext(i))
	{
		const InputAction &action = _valueMappings[_valueToInfo.ValueAt(i)]._action;

		int val = GetDigitalValue(action, nullptr);

		// Can only return one value, so choose the largest
		if (abs(val) > abs(ret))
		{
			ret = val;
		}
	}

	return ret;
}

float ff::CInputMapping::GetAnalogValue(Atom valueID) const
{
	float ret = 0;

	for (BucketIter i = _valueToInfo.Get(valueID); i != INVALID_ITER; i = _valueToInfo.GetNext(i))
	{
		const InputAction &action = _valueMappings[_valueToInfo.ValueAt(i)]._action;

		float val = GetAnalogValue(action, false);

		// Can only return one value, so choose the largest
		if (fabsf(val) > fabsf(ret))
		{
			ret = val;
		}
	}

	return ret;
}

ff::String ff::CInputMapping::GetStringValue(Atom valueID) const
{
	String ret;

	for (BucketIter i = _valueToInfo.Get(valueID); i != INVALID_ITER; i = _valueToInfo.GetNext(i))
	{
		const InputAction &action = _valueMappings[_valueToInfo.ValueAt(i)]._action;

		ret += GetStringValue(action);
	}

	return ret;
}

void ff::CInputMapping::AddListener(IInputEventListener *pListener)
{
	assertRet(pListener);
	GetUnsharedObject(_listeners);
	_listeners->Push(pListener);
}

bool ff::CInputMapping::AddProxyListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy)
{
	assertRetVal(CreateProxyInputEventListener(pListener, ppProxy), false);
	AddListener(*ppProxy);

	return true;
}

bool ff::CInputMapping::RemoveListener(IInputEventListener *pListener)
{
	assertRetVal(pListener, false);

	GetUnsharedObject(_listeners);
	if (_listeners != nullptr)
	{
		for (const auto &listener: *_listeners.Object())
		{
			if (listener == pListener)
			{
				_listeners->DeleteItem(listener);
				return true;
			}
		}
	}

	assertRetVal(false, false);
}

int ff::CInputMapping::GetDigitalValue(const InputAction &action, int *pPressCount) const
{
	int nFakePressCount;
	pPressCount = pPressCount ? pPressCount : &nFakePressCount;
	*pPressCount = 0;

	switch (action._device)
	{
	case INPUT_DEVICE_KEYBOARD:
		if (action._part == INPUT_PART_BUTTON && action._partValue == INPUT_VALUE_PRESSED)
		{
			int nRet = 0;

			for (size_t i = 0; i < _keys.Size(); i++)
			{
				*pPressCount += _keys[i]->GetKeyPressCount(action._partIndex);

				if (_keys[i]->GetKey(action._partIndex))
				{
					nRet = 1;
				}
			}

			return nRet;
		}
		else if (action._part == INPUT_PART_TEXT && action._partValue == INPUT_VALUE_TEXT)
		{
			for (size_t i = 0; i < _keys.Size(); i++)
			{
				if (_keys[i]->GetChars().size())
				{
					*pPressCount = 1;
					return 1;
				}
			}
		}
		break;

	case INPUT_DEVICE_MOUSE:
		if (action._part == INPUT_PART_BUTTON && action._partValue == INPUT_VALUE_PRESSED)
		{
			int nRet = 0;

			for (size_t i = 0; i < _mice.Size(); i++)
			{
				*pPressCount += _mice[i]->GetButtonClickCount(action._partIndex);

				if (_mice[i]->GetButton(action._partIndex))
				{
					nRet = 1;
				}
			}

			return nRet;
		}
		break;

	case INPUT_DEVICE_JOYSTICK:
		if (action._part == INPUT_PART_BUTTON && action._partValue == INPUT_VALUE_PRESSED)
		{
			int nRet = 0;

			for (size_t i = 0; i < _joys.Size(); i++)
			{
				if (_joys[i]->IsConnected())
				{
					size_t nButtonCount = _joys[i]->GetButtonCount();
					bool   bAllButtons  = (action._partIndex == INPUT_INDEX_ANY_BUTTON);

					if (bAllButtons || action._partIndex < nButtonCount)
					{
						size_t nFirst = bAllButtons ? 0 : action._partIndex;
						size_t nEnd   = bAllButtons ? nButtonCount : nFirst + 1;

						for (size_t h = nFirst; h < nEnd; h++)
						{
							*pPressCount += _joys[i]->GetButtonPressCount(h);

							if (_joys[i]->GetButton(h))
							{
								nRet = 1;
							}
						}
					}
				}
			}

			return nRet;
		}
		else if (action._part == INPUT_PART_SPECIAL_BUTTON && action._partValue == INPUT_VALUE_PRESSED)
		{
			int nRet = 0;
			ESpecialJoystickButton type = (ESpecialJoystickButton)action._partIndex;

			for (size_t i = 0; i < _joys.Size(); i++)
			{
				if (_joys[i]->IsConnected() && _joys[i]->HasSpecialButton(type))
				{
					*pPressCount += _joys[i]->GetSpecialButtonPressCount(type);

					if (_joys[i]->GetSpecialButton(type))
					{
						nRet = 1;
					}
				}
			}

			return nRet;
		}
		else if (action._part == INPUT_PART_TRIGGER)
		{
			if (action._partValue == INPUT_VALUE_PRESSED)
			{
				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetTriggerCount())
					{
						*pPressCount += _joys[i]->GetTriggerPressCount(action._partIndex);
					}
				}
			}

			return GetAnalogValue(action, true) >= 0.5f ? 1 : 0;
		}
		else if (action._part == INPUT_PART_STICK)
		{
			if (action._partValue == INPUT_VALUE_LEFT ||
				action._partValue == INPUT_VALUE_RIGHT ||
				action._partValue == INPUT_VALUE_UP ||
				action._partValue == INPUT_VALUE_DOWN)
			{
				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetStickCount())
					{
						RectInt dirs = _joys[i]->GetStickPressCount(action._partIndex);

						switch (action._partValue)
						{
						case INPUT_VALUE_LEFT:  *pPressCount += dirs.left;   break;
						case INPUT_VALUE_RIGHT: *pPressCount += dirs.right;  break;
						case INPUT_VALUE_UP:    *pPressCount += dirs.top;    break;
						case INPUT_VALUE_DOWN:  *pPressCount += dirs.bottom; break;
						}
					}
				}
			}

			return GetAnalogValue(action, true) >= 0.5f ? 1 : 0;
		}
		else if (action._part == INPUT_PART_DPAD)
		{
			if (action._partValue == INPUT_VALUE_LEFT ||
				action._partValue == INPUT_VALUE_RIGHT ||
				action._partValue == INPUT_VALUE_UP ||
				action._partValue == INPUT_VALUE_DOWN)
			{
				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetDPadCount())
					{
						RectInt dirs = _joys[i]->GetDPadPressCount(action._partIndex);

						switch (action._partValue)
						{
						case INPUT_VALUE_LEFT:  *pPressCount += dirs.left;   break;
						case INPUT_VALUE_RIGHT: *pPressCount += dirs.right;  break;
						case INPUT_VALUE_UP:    *pPressCount += dirs.top;    break;
						case INPUT_VALUE_DOWN:  *pPressCount += dirs.bottom; break;
						}
					}
				}
			}

			return GetAnalogValue(action, true) >= 0.5f ? 1 : 0;
		}
		break;
	}

	return 0;
}

float ff::CInputMapping::GetAnalogValue(const InputAction &action, bool bForDigital) const
{
	switch (action._device)
	{
	case INPUT_DEVICE_KEYBOARD:
	case INPUT_DEVICE_MOUSE:
		return GetDigitalValue(action, nullptr) ? 1.0f : 0.0f;

	case INPUT_DEVICE_JOYSTICK:
		if (action._part == INPUT_PART_BUTTON)
		{
			return GetDigitalValue(action, nullptr) ? 1.0f : 0.0f;
		}
		else if (action._part == INPUT_PART_TRIGGER && action._partValue == INPUT_VALUE_PRESSED)
		{
			float val = 0;

			for (size_t i = 0; i < _joys.Size(); i++)
			{
				if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetTriggerCount())
				{
					float newVal = _joys[i]->GetTrigger(action._partIndex, bForDigital);

					if (fabsf(newVal) > fabsf(val))
					{
						val = newVal;
					}
				}
			}

			return val;
		}
		else if (action._part == INPUT_PART_STICK)
		{
			if (action._partValue == INPUT_VALUE_X_AXIS ||
				action._partValue == INPUT_VALUE_Y_AXIS ||
				action._partValue == INPUT_VALUE_LEFT ||
				action._partValue == INPUT_VALUE_RIGHT ||
				action._partValue == INPUT_VALUE_UP ||
				action._partValue == INPUT_VALUE_DOWN)
			{
				PointFloat posXY(0, 0);

				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetStickCount())
					{
						PointFloat newXY = _joys[i]->GetStickPos(action._partIndex, bForDigital);

						if (fabsf(newXY.x) > fabsf(posXY.x))
						{
							posXY.x = newXY.x;
						}

						if (fabsf(newXY.y) > fabsf(posXY.y))
						{
							posXY.y = newXY.y;
						}
					}
				}

				switch (action._partValue)
				{
				case INPUT_VALUE_X_AXIS: return posXY.x;
				case INPUT_VALUE_Y_AXIS: return posXY.y;
				case INPUT_VALUE_LEFT:   return (posXY.x < 0) ? -posXY.x : 0.0f;
				case INPUT_VALUE_RIGHT:  return (posXY.x > 0) ?  posXY.x : 0.0f;
				case INPUT_VALUE_UP:     return (posXY.y < 0) ? -posXY.y : 0.0f;
				case INPUT_VALUE_DOWN:   return (posXY.y > 0) ?  posXY.y : 0.0f;
				}
			}
		}
		else if (action._part == INPUT_PART_DPAD)
		{
			if (action._partValue == INPUT_VALUE_X_AXIS ||
				action._partValue == INPUT_VALUE_Y_AXIS ||
				action._partValue == INPUT_VALUE_LEFT ||
				action._partValue == INPUT_VALUE_RIGHT ||
				action._partValue == INPUT_VALUE_UP ||
				action._partValue == INPUT_VALUE_DOWN)
			{
				PointInt posXY(0, 0);

				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetDPadCount())
					{
						PointInt newXY = _joys[i]->GetDPadPos(action._partIndex);

						if (abs(newXY.x) > abs(posXY.x))
						{
							posXY.x = newXY.x;
						}

						if (abs(newXY.y) > abs(posXY.y))
						{
							posXY.y = newXY.y;
						}
					}
				}

				switch (action._partValue)
				{
				case INPUT_VALUE_X_AXIS: return (float)posXY.x;
				case INPUT_VALUE_Y_AXIS: return (float)posXY.y;
				case INPUT_VALUE_LEFT:   return (posXY.x < 0) ? 1.0f : 0.0f;
				case INPUT_VALUE_RIGHT:  return (posXY.x > 0) ? 1.0f : 0.0f;
				case INPUT_VALUE_UP:     return (posXY.y < 0) ? 1.0f : 0.0f;
				case INPUT_VALUE_DOWN:   return (posXY.y > 0) ? 1.0f : 0.0f;
				}
			}
		}
		break;
	}

	return 0;
}

ff::String ff::CInputMapping::GetStringValue(const InputAction &action) const
{
	String szText;

	if (action._device == INPUT_DEVICE_KEYBOARD && action._part == INPUT_PART_TEXT)
	{
		for (size_t i = 0; i < _keys.Size(); i++)
		{
			if (szText.empty())
			{
				szText = _keys[i]->GetChars();
			}
			else
			{
				szText += _keys[i]->GetChars();
			}
		}

		return szText;
	}

	return szText;
}

void ff::CInputMapping::PushStartEvent(InputEventMappingInfo &info)
{
	InputEvent ev;
	ev._eventID = info._eventID;
	ev._count   = ++info._eventCount;

	_currentEvents.Push(ev);
}

void ff::CInputMapping::PushStopEvent(InputEventMappingInfo &info)
{
	bool bEvent = (info._eventCount > 0);

	info._holdingSeconds = 0;
	info._eventCount     = 0;
	info._holding       = false;

	if (bEvent)
	{
		InputEvent ev;
		ev._eventID = info._eventID;
		ev._count   = 0;

		_currentEvents.Push(ev);
	}
}
