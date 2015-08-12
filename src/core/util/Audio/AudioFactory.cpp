#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioFactory.h"
#include "COM/ComAlloc.h"
#include "COM/ComListener.h"
#include "Globals/ProcessGlobals.h"

#if !METRO_APP
#include <SetupAPI.h>
#endif

namespace ff
{
	class __declspec(uuid("b1c52643-fed3-458a-b998-3036c344ced9"))
		AudioFactory
			: public ComBase
			, public IAudioFactory
			, public IComListener
	{
	public:
		DECLARE_HEADER(AudioFactory);

		bool Init();

		// IComListener
		virtual void OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj) override;
		virtual void OnDestruct(REFGUID catid, REFGUID clsid, IUnknown *pObj) override;

		// IAudioFactory
		virtual IXAudio2 *GetAudio() override;

		virtual bool CreateDefaultDevice(IAudioDevice **device) override;
		virtual bool CreateDevice(StringRef name, size_t channels, size_t sampleRate, IAudioDevice **device) override;

		virtual size_t GetDeviceCount() const override;
		virtual IAudioDevice *GetDevice(size_t nIndex) const override;

	private:
		Mutex _mutex;
		ComPtr<IProxyComListener> _listener;
		ComPtr<IXAudio2> _audio;
		Vector<IAudioDevice *> _devices;
		IAudioDevice *_defaultDevice;
		bool _startedMF;
	};
}

BEGIN_INTERFACES(ff::AudioFactory)
	HAS_INTERFACE(ff::IAudioFactory)
	HAS_INTERFACE(ff::IComListener)
END_INTERFACES()

bool ff::CreateAudioFactory(IAudioFactory **obj)
{
	assertRetVal(obj, false);

	ComPtr<AudioFactory, IAudioFactory> myObj;
	assertHrRetVal(ComAllocator<AudioFactory>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(), false);

	*obj = myObj.Detach();
	return true;
}

ff::AudioFactory::AudioFactory()
	: _defaultDevice(nullptr)
	, _startedMF(false)
{
	verify(CreateProxyComListener(this, &_listener));
}

ff::AudioFactory::~AudioFactory()
{
	assert(_devices.IsEmpty());

	_listener->SetOwner(nullptr);
	_audio = nullptr;

	if (_startedMF)
	{
		_startedMF = false;
		::MFShutdown();
	}
}

bool ff::AudioFactory::Init()
{
	assertHrRetVal(::MFStartup(MF_VERSION), false);
	_startedMF = true;

	return true;
}

IXAudio2 *ff::AudioFactory::GetAudio()
{
	if (!_audio)
	{
		LockMutex crit(_mutex);

		if (!_audio)
		{
#if !METRO_APP
			// Force SetupAPI.dll to be loaded before the XAudio DLL.
			// Otherwise, on shutdown, SetupAPI can be unloaded early, causing a failure
			// while MMDevAPI.dll is in DllMain, and no other DLLs get DllMain called.
			// http://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/d15e5b4a-b7cd-42d4-905a-f8ed501d857d
			SetupGetNonInteractiveMode();
#endif
			if (GetThisModule().IsDebugBuild())
			{
				XAudio2Create(&_audio);

				XAUDIO2_DEBUG_CONFIGURATION dc;
				ZeroObject(dc);
				dc.TraceMask = XAUDIO2_LOG_ERRORS;
				dc.BreakMask = XAUDIO2_LOG_ERRORS;
				_audio->SetDebugConfiguration(&dc);
			}

			if (!_audio)
			{
				assertRetVal(SUCCEEDED(XAudio2Create(&_audio)), nullptr);
			}
		}
	}

	return _audio;
}

bool ff::AudioFactory::CreateDefaultDevice(IAudioDevice **device)
{
	if (_defaultDevice)
	{
		*device = GetAddRef(_defaultDevice);
	}
	else
	{
		assertRetVal(CreateDevice(GetEmptyString(), 0, 0, device), false);
		_defaultDevice = *device;
	}

	return true;
}

bool ff::AudioFactory::CreateDevice(StringRef deviceId, size_t channels, size_t sampleRate, IAudioDevice **device)
{
	assertRetVal(device && GetAudio(), false);
	*device = nullptr;

	ComPtr<IAudioDevice> pDevice;
	assertRetVal(CreateAudioDevice(GetAudio(), deviceId, channels, sampleRate, &pDevice), false);

	_devices.Push(pDevice);
	AddComListener(pDevice.Interface(), _listener);

	*device = pDevice.Detach();
	return true;
}


size_t ff::AudioFactory::GetDeviceCount() const
{
	return _devices.Size();
}

ff::IAudioDevice *ff::AudioFactory::GetDevice(size_t nIndex) const
{
	assertRetVal(nIndex >= 0 && nIndex < _devices.Size(), nullptr);
	return _devices[nIndex];
}

void ff::AudioFactory::OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	assert(false);
}

void ff::AudioFactory::OnDestruct(REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	ComPtr<IAudioDevice> pDevice;
	assertRet(pDevice.QueryFrom(pObj));

	if (pDevice == _defaultDevice)
	{
		_defaultDevice = nullptr;
	}

	for (size_t i = 0; i < _devices.Size(); i++)
	{
		if (_devices[i] == pDevice)
		{
			_devices.Delete(i);
			return;
		}
	}

	assertRet(false);
}
