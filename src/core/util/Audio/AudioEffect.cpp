#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioBuffer.h"
#include "Audio/AudioEffect.h"
#include "Audio/AudioPlaying.h"
#include "Audio/DestroyVoice.h"
#include "COM/ComAlloc.h"
#include "Data/DataPersist.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/Data/GraphCategory.h"
#include "Module/ModuleFactory.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"

class AudioEffectPlaying;

class __declspec(uuid("86764238-1c06-418c-bb2f-b1989beb6c91"))
	AudioEffect : public ff::ComBase, public ff::IAudioEffect
{
public:
	DECLARE_HEADER(AudioEffect);

	virtual HRESULT _Construct(IUnknown *unkOuter) override;
	void OnBufferEnd(AudioEffectPlaying *pPlaying);

	// IAudioEffect functions
	virtual bool SetBuffer(
		ff::IAudioBuffer *buffer,
		size_t start,
		size_t length,
		size_t loopStart,
		size_t loopLength,
		size_t loopCount,
		float volume,
		float freqRatio) override;

	virtual bool Play(
		bool startPlaying,
		float volume,
		float freqRatio,
		ff::IAudioPlaying **playing) override;

	virtual bool IsPlaying() const override;
	virtual void StopAll() override;

	// IAudioDeviceChild
	virtual ff::IAudioDevice *GetDevice() const override;
	virtual void Reset() override;

private:
	typedef ff::Vector<ff::ComPtr<AudioEffectPlaying, ff::IAudioPlaying>, 4> CPlayingVector;

	CPlayingVector _playing;
	ff::ComPtr<ff::IAudioDevice> _device;
	ff::ComPtr<ff::IAudioBuffer> _buffer;
	size_t _start;
	size_t _length;
	size_t _loopStart;
	size_t _loopLength;
	size_t _loopCount;
	float _volume;
	float _freqRatio;
};

BEGIN_INTERFACES(AudioEffect)
	HAS_INTERFACE(ff::IAudioEffect)
	HAS_INTERFACE(ff::IAudioDeviceChild)
END_INTERFACES()

static ff::ModuleStartup RegisterAudioEffect([](ff::Module &module)
{
	static ff::StaticString name(L"Audio Effect");
	module.RegisterClassT<AudioEffect>(name, __uuidof(ff::IAudioEffect), ff::GetCategoryAudioObject());
});

bool ff::CreateAudioEffect(IAudioDevice *pDevice, IAudioEffect **ppEffect)
{
	return SUCCEEDED(ComAllocator<AudioEffect>::CreateInstance(
		pDevice, GUID_NULL, __uuidof(IAudioEffect), (void**)ppEffect));
}

class __declspec(uuid("30002677-efc1-4446-8ab3-d66ed2be3800"))
	AudioEffectPlaying
		: public ff::ComBase
		, public ff::IAudioPlaying
		, public IXAudio2VoiceCallback
{
public:
	DECLARE_HEADER(AudioEffectPlaying);

	bool Init(ff::IAudioBuffer *pBuffer, IXAudio2SourceVoice *source, bool bStartPlaying);
	void SetEffect(AudioEffect *pEffect);

	virtual HRESULT _Construct(IUnknown *unkOuter) override;
	virtual void _DeleteThis() override;

	// IAudioPlaying functions
	virtual bool IsPlaying() const override;
	virtual bool IsPaused() const override;
	virtual bool IsStopped() const override;

	virtual void Advance() override;
	virtual void Stop() override;
	virtual void Pause() override;
	virtual void Resume() override;

	virtual double GetDuration() const override;
	virtual double GetPosition() const override;
	virtual bool SetPosition(double value) override;
	virtual double GetVolume() const override;
	virtual bool SetVolume(double value) override;
	virtual bool FadeIn(double value) override;
	virtual bool FadeOut(double value) override;

	// IAudioDeviceChild
	virtual ff::IAudioDevice *GetDevice() const override;
	virtual void Reset() override;

	// IXAudio2VoiceCallback functions
	COM_FUNC_VOID OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
	COM_FUNC_VOID OnVoiceProcessingPassEnd() override;
	COM_FUNC_VOID OnStreamEnd() override;
	COM_FUNC_VOID OnBufferStart(void* pBufferContext) override;
	COM_FUNC_VOID OnBufferEnd(void* pBufferContext) override;
	COM_FUNC_VOID OnLoopEnd(void* pBufferContext) override;
	COM_FUNC_VOID OnVoiceError(void* pBufferContext, HRESULT error) override;

private:
	void OnBufferEnd();

	ff::ComPtr<ff::IAudioDevice> _device;
	ff::ComPtr<ff::IAudioBuffer> _buffer;
	AudioEffect *_effect;
	IXAudio2SourceVoice *_source;
	bool _paused;
	bool _done;
};

BEGIN_INTERFACES(AudioEffectPlaying)
	HAS_INTERFACE(IAudioPlaying)
	HAS_INTERFACE(IAudioDeviceChild)
END_INTERFACES()

// STATIC_DATA (object)
static ff::PoolAllocator<ff::ComObject<AudioEffectPlaying>> s_audioEffectPlayingAllocator;

static HRESULT CreateAudioEffectPlaying(IUnknown *unkOuter, REFGUID clsid, REFGUID iid, void **ppObj)
{
	assertRetVal(clsid == GUID_NULL || clsid == __uuidof(AudioEffectPlaying), E_INVALIDARG);
	ff::ComPtr<ff::ComObject<AudioEffectPlaying>> pObj = s_audioEffectPlayingAllocator.New();
	assertRetVal(SUCCEEDED(pObj->_Construct(unkOuter)), E_FAIL);

	return pObj->QueryInterface(iid, ppObj);
}

static ff::ModuleStartup RegisterAudioEffectPlaying([](ff::Module &module)
{
	static ff::StaticString name(L"Audio Effect Playing");

	module.RegisterClass(
		name,
		__uuidof(AudioEffectPlaying),
		CreateAudioEffectPlaying,
		__uuidof(ff::IAudioPlaying),
		ff::GetCategoryAudioObject());
});

AudioEffect::AudioEffect()
	: _start(0)
	, _length(0)
	, _loopStart(0)
	, _loopLength(0)
	, _loopCount(0)
	, _volume(1)
	, _freqRatio(1)
{
}

AudioEffect::~AudioEffect()
{
	Reset();

	for (size_t i = 0; i < _playing.Size(); i++)
	{
		_playing[i]->SetEffect(nullptr);
	}
}

HRESULT AudioEffect::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);

	return __super::_Construct(unkOuter);
}

void AudioEffect::OnBufferEnd(AudioEffectPlaying *pPlaying)
{
	verify(_playing.DeleteItem(pPlaying));
}

bool AudioEffect::SetBuffer(
		ff::IAudioBuffer *buffer,
		size_t start,
		size_t length,
		size_t loopStart,
		size_t loopLength,
		size_t loopCount,
		float volume,
		float freqRatio)
{
	_buffer     = buffer;
	_start      = start;
	_length     = length;
	_loopStart  = loopStart;
	_loopLength = loopLength;
	_loopCount  = loopCount;
	_volume      = volume;
	_freqRatio   = freqRatio;

	assertRetVal(_buffer, false);

	// can't validate the positions since the buffer might not be loaded yet

	return true;
}

bool AudioEffect::Play(
		bool startPlaying,
		float volume,
		float freqRatio,
		ff::IAudioPlaying **playing)
{
	assertRetVal(_buffer && _device->GetAudio(), false);

	ff::ComPtr<AudioEffectPlaying, ff::IAudioPlaying> pPlaying;
	IXAudio2SourceVoice* source = nullptr;

	assertHrRetVal(CreateAudioEffectPlaying(_device, GUID_NULL, __uuidof(AudioEffectPlaying), (void**)&pPlaying), false);

	XAUDIO2_SEND_DESCRIPTOR send;
	send.Flags = 0;
	send.pOutputVoice = _device->GetVoice(ff::AudioVoiceType::EFFECTS);

	XAUDIO2_VOICE_SENDS sends;
	sends.SendCount = 1;
	sends.pSends = &send;

	assertHrRetVal(_device->GetAudio()->CreateSourceVoice(
		&source,
		_buffer->GetFormat(),
		0, // flags
		XAUDIO2_DEFAULT_FREQ_RATIO,
		pPlaying, // callback
		&sends, // send list
		nullptr), false); // effect chain

	XAUDIO2_BUFFER buffer;
	buffer.Flags      = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = (DWORD)_buffer->GetDataSize();
	buffer.pAudioData = _buffer->GetData();
	buffer.PlayBegin  = (DWORD)_start;
	buffer.PlayLength = (DWORD)_length;
	buffer.LoopBegin  = (DWORD)_loopStart;
	buffer.LoopLength = (DWORD)_loopLength;
	buffer.LoopCount  = (_loopCount == ff::INVALID_SIZE) ? XAUDIO2_LOOP_INFINITE : (DWORD)_loopCount;
	buffer.pContext   = nullptr;

	assertHrRetVal(source->SubmitSourceBuffer(&buffer), false);
	source->SetVolume(_volume * volume);
	source->SetFrequencyRatio(_freqRatio * freqRatio);

	if (!pPlaying->Init(_buffer, source, startPlaying))
	{
		source->DestroyVoice();
		assertRetVal(false, false);
	}

	pPlaying->SetEffect(this);
	_playing.Push(pPlaying);

	if (playing)
	{
		*playing = pPlaying.Detach();
	}

	return true;
}

bool AudioEffect::IsPlaying() const
{
	for (size_t i = 0; i < _playing.Size(); i++)
	{
		if (_playing[i]->IsPlaying())
		{
			return true;
		}
	}

	return false;
}

void AudioEffect::StopAll()
{
	CPlayingVector playing = _playing;

	for (size_t i = 0; i < playing.Size(); i++)
	{
		playing[i]->Stop();
	}
}

ff::IAudioDevice *AudioEffect::GetDevice() const
{
	return _device;
}

void AudioEffect::Reset()
{
}

AudioEffectPlaying::AudioEffectPlaying()
	: _effect(nullptr)
	, _source(nullptr)
	, _paused(true)
	, _done(false)
{
}

AudioEffectPlaying::~AudioEffectPlaying()
{
	Reset();
}

HRESULT AudioEffectPlaying::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	return __super::_Construct(unkOuter);
}

void AudioEffectPlaying::_DeleteThis()
{
	s_audioEffectPlayingAllocator.Delete(static_cast<ff::ComObject<AudioEffectPlaying>*>(this));
}

bool AudioEffectPlaying::Init(
		ff::IAudioBuffer *pBuffer,
		IXAudio2SourceVoice *source,
		bool bStartPlaying)
{
	assertRetVal(pBuffer && source, false);

	_buffer = pBuffer;
	_source = source;

	if (bStartPlaying)
	{
		Resume();
	}

	return true;
}

void AudioEffectPlaying::SetEffect(AudioEffect *pEffect)
{
	_effect = pEffect;
}

bool AudioEffectPlaying::IsPlaying() const
{
	return _source && !_paused && !_done;
}

bool AudioEffectPlaying::IsPaused() const
{
	return _source && _paused && !_done;
}

bool AudioEffectPlaying::IsStopped() const
{
	return !_source || _done;
}

void AudioEffectPlaying::Advance()
{
	if (_done && _source)
	{
		OnBufferEnd();
	}
}

void AudioEffectPlaying::Stop()
{
	if (_source && !_done)
	{
		_source->Stop();
		_source->FlushSourceBuffers();
	}
}

void AudioEffectPlaying::Pause()
{
	if (_source && !_done)
	{
		_source->Stop();
		_paused = true;
	}
}

void AudioEffectPlaying::Resume()
{
	if (IsPaused())
	{
		_source->Start();
		_paused = false;
	}
}

double AudioEffectPlaying::GetDuration() const
{
	return 0;
}

double AudioEffectPlaying::GetPosition() const
{
	return 0;
}

bool AudioEffectPlaying::SetPosition(double value)
{
	return false;
}

double AudioEffectPlaying::GetVolume() const
{
	return 1.0;
}

bool AudioEffectPlaying::SetVolume(double value)
{
	return false;
}

bool AudioEffectPlaying::FadeIn(double value)
{
	return false;
}

bool AudioEffectPlaying::FadeOut(double value)
{
	return false;
}

ff::IAudioDevice *AudioEffectPlaying::GetDevice() const
{
	return _device;
}

void AudioEffectPlaying::Reset()
{
	if (_source)
	{
		IXAudio2SourceVoice *source = _source;
		_source = nullptr;
		source->DestroyVoice();
	}
}

void AudioEffectPlaying::OnVoiceProcessingPassStart(UINT32 BytesRequired)
{
}

void AudioEffectPlaying::OnVoiceProcessingPassEnd()
{
}

void AudioEffectPlaying::OnStreamEnd()
{
}

void AudioEffectPlaying::OnBufferStart(void *pBufferContext)
{
}

void AudioEffectPlaying::OnBufferEnd(void *pBufferContext)
{
	_done = true;
}

void AudioEffectPlaying::OnBufferEnd()
{
	assert(_done && ff::IsRunningOnMainThread());

	ff::ComPtr<ff::IAudioPlaying> pKeepAlive = this;

	if (_effect)
	{
		_effect->OnBufferEnd(this);
		_effect = nullptr;
	}

	if (_source)
	{
		ff::ComPtr<ff::DestroyVoiceWorkItem, ff::IWorkItem> pWorkItem;

		if (!ff::CreateDestroyVoiceWorkItem(_device, _source, &pWorkItem))
		{
			_source->DestroyVoice();
		}
		else
		{
			ff::ProcessGlobals::Get()->GetThreadPool()->Add(pWorkItem);
		}

		_source = nullptr;
	}
}

void AudioEffectPlaying::OnLoopEnd(void *pBufferContext)
{
}

void AudioEffectPlaying::OnVoiceError(void* pBufferContext, HRESULT error)
{
	assertSz(false, L"XAudio2 voice error");
}
