#include "pch.h"
#include "App/Timer.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioMusic.h"
#include "Audio/AudioPlaying.h"
#include "Audio/AudioStream.h"
#include "Audio/DestroyVoice.h"
#include "COM/ComAlloc.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/Stream.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/Data/GraphCategory.h"
#include "Module/ModuleFactory.h"
#include "Thread/ThreadUtil.h"
#include "Windows/Handles.h"

#define DEBUG_THIS_FILE 0 // DEBUG

namespace ff
{
	class AudioMusicPlaying;
	class AudioSourceReaderCallback;

	class __declspec(uuid("7bd9c9be-b944-4c4c-95ee-218c516d71f8"))
		AudioMusic : public ComBase, public IAudioMusic
	{
	public:
		DECLARE_HEADER(AudioMusic);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		void OnMusicDone(AudioMusicPlaying *playing);

		// IAudioMusic
		virtual bool SetStream(IAudioStream *stream, float volume, float freqRatio) override;
		virtual bool Play(IAudioPlaying **obj, bool startPlaying, float volume, float freqRatio) override;
		virtual bool IsPlaying() const override;
		virtual void StopAll() override;

		// IAudioDeviceChild
		virtual IAudioDevice *GetDevice() const override;
		virtual void Reset() override;

	private:
		typedef Vector<ComPtr<AudioMusicPlaying, IAudioPlaying>, 4> PlayingVector;

		PlayingVector _playing;
		ComPtr<IAudioDevice> _device;
		ComPtr<IAudioStream> _stream;
		float _volume;
		float _freqRatio;
	};

	class __declspec(uuid("4900158e-fcdd-4179-b85e-63aa12f6ee1f"))
		AudioMusicPlaying
			: public IWorkItem
			, public IAudioPlaying
			, public IXAudio2VoiceCallback
			, public IMFSourceReaderCallback
	{
	public:
		DECLARE_HEADER(AudioMusicPlaying);

		bool Init(AudioMusic *parent, IAudioStream *stream, bool startPlaying, float volume, float freqRatio);
		void SetParent(AudioMusic *parent);
		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		virtual void _Destruct() override;

		enum class State
		{
			MUSIC_INVALID,
			MUSIC_INIT,
			MUSIC_PLAYING,
			MUSIC_PAUSED,
			MUSIC_DONE
		};

		State GetState() const;

		// IWorkItem
		virtual void Run() override;

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
		virtual IAudioDevice *GetDevice() const override;
		virtual void Reset() override;

		// IXAudio2VoiceCallback functions
		COM_FUNC_VOID OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
		COM_FUNC_VOID OnVoiceProcessingPassEnd() override;
		COM_FUNC_VOID OnStreamEnd() override;
		COM_FUNC_VOID OnBufferStart(void* pBufferContext) override;
		COM_FUNC_VOID OnBufferEnd(void* pBufferContext) override;
		COM_FUNC_VOID OnLoopEnd(void* pBufferContext) override;
		COM_FUNC_VOID OnVoiceError(void* pBufferContext, HRESULT error) override;

		// IMFSourceReaderCallback
		COM_FUNC OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample) override;
		COM_FUNC OnFlush(DWORD dwStreamIndex) override;
		COM_FUNC OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent) override;

	private:
		bool AsyncInit();
		void StartAsyncWork();
		void CancelAsync();
		void StartReadSample();
		void OnMusicDone();
		void UpdateSourceVolume(IXAudio2SourceVoice *source);

		static const size_t MAX_BUFFERS = 2;

		struct BufferInfo
		{
			Vector<BYTE> _buffer;
			LONGLONG _startTime;
			LONGLONG _duration;
			UINT64 _startSamples;
			DWORD _streamIndex;
			DWORD _streamFlags;
		};

		enum class MediaState
		{
			None,
			Reading,
			Flushing,
			Done,
		};

		Mutex _cs;
		State _state;
		LONGLONG _duration; // in 100-nanosecond units
		LONGLONG _desiredPosition;
		WinHandle _asyncEvent; // set when there is no async action running
		WinHandle _stopEvent; // set when everything should stop
		List<BufferInfo> _bufferInfos;

		ComPtr<IAudioDevice> _device;
		ComPtr<IAudioStream> _stream;
		ComPtr<IMFSourceReader> _mediaReader;
		ComPtr<AudioSourceReaderCallback, IMFSourceReaderCallback> _mediaCallback;
		IXAudio2SourceVoice *_source;
		AudioMusic *_parent;

		float _freqRatio;
		float _volume;
		float _playVolume;
		float _fadeVolume;
		float _fadeScale;
		Timer _fadeTimer;
		bool _startPlaying;
		MediaState _mediaState;
	};

	class __declspec(uuid("4900158e-fcdd-4179-b85e-63aa12f6ee1f"))
		AudioSourceReaderCallback : public ComBase, public IMFSourceReaderCallback
	{
	public:
		DECLARE_HEADER(AudioSourceReaderCallback);

		void SetParent(AudioMusicPlaying *parent);

		// IMFSourceReaderCallback
		COM_FUNC OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample) override;
		COM_FUNC OnFlush(DWORD dwStreamIndex) override;
		COM_FUNC OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent) override;

	private:
		Mutex _cs;
		AudioMusicPlaying *_parent;
	};
}

BEGIN_INTERFACES(ff::AudioMusic)
	HAS_INTERFACE(ff::IAudioMusic)
	HAS_INTERFACE(ff::IAudioDeviceChild)
END_INTERFACES()

BEGIN_INTERFACES(ff::AudioMusicPlaying)
	HAS_INTERFACE(ff::IAudioPlaying)
	HAS_INTERFACE(ff::IAudioDeviceChild)
	HAS_INTERFACE(IMFSourceReaderCallback)
	PARENT_INTERFACES(ff::IWorkItem)
END_INTERFACES()

BEGIN_INTERFACES(ff::AudioSourceReaderCallback)
	HAS_INTERFACE(IMFSourceReaderCallback)
END_INTERFACES()

static ff::ModuleStartup RegisterAudioMusic([](ff::Module &module)
{
	static ff::StaticString name0(L"Audio Music");
	static ff::StaticString name1(L"Audio Music Playing");

	module.RegisterClassT<ff::AudioMusic>(name0, __uuidof(ff::IAudioMusic), ff::GetCategoryAudioObject());
	module.RegisterClassT<ff::AudioMusicPlaying>(name1, __uuidof(ff::IAudioPlaying), ff::GetCategoryAudioObject());
});

bool ff::CreateAudioMusic(IAudioDevice *device, IAudioMusic **obj)
{
	return SUCCEEDED(ComAllocator<AudioMusic>::CreateInstance(device, GUID_NULL, __uuidof(IAudioMusic), (void**)obj));
}

bool ff::CreateAudioMusic(IAudioDevice *device, IAudioStream *stream, IAudioMusic **obj)
{
	return ff::CreateAudioMusic(device, stream, 1, 1, obj);
}

bool ff::CreateAudioMusic(IAudioDevice *device, IAudioStream *stream, float volume, float freqRatio, IAudioMusic **obj)
{
	assertRetVal(CreateAudioMusic(device, obj), false);
	assertRetVal((*obj)->SetStream(stream, volume, freqRatio), false);
	return true;
}

ff::AudioMusic::AudioMusic()
	: _volume(1)
	, _freqRatio(1)
{
}

ff::AudioMusic::~AudioMusic()
{
	Reset();

	for (AudioMusicPlaying *play: _playing)
	{
		play->SetParent(nullptr);
	}
}

HRESULT ff::AudioMusic::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);

	return __super::_Construct(unkOuter);
}

void ff::AudioMusic::OnMusicDone(AudioMusicPlaying *playing)
{
	verify(_playing.DeleteItem(playing));
}

bool ff::AudioMusic::SetStream(IAudioStream *stream, float volume, float freqRatio)
{
	assertRetVal(!_stream && stream, false);
	_stream = stream;
	_volume = volume;
	_freqRatio = freqRatio;

	return true;
}

bool ff::AudioMusic::Play(IAudioPlaying **obj, bool startPlaying, float volume, float freqRatio)
{
	ComPtr<AudioMusicPlaying, IAudioPlaying> playing;
	assertHrRetVal(ff::ComAllocator<AudioMusicPlaying>::CreateInstance(_device, &playing), false);
	assertRetVal(playing->Init(this, _stream, startPlaying, _volume * volume, _freqRatio * freqRatio), false);
	_playing.Push(playing);

	if (obj)
	{
		*obj = playing.Detach();
	}

	return true;
}

bool ff::AudioMusic::IsPlaying() const
{
	for (AudioMusicPlaying *play: _playing)
	{
		if (play->IsPlaying())
		{
			return true;
		}
	}

	return false;
}

void ff::AudioMusic::StopAll()
{
	PlayingVector playing = _playing;

	for (AudioMusicPlaying *play: playing)
	{
		play->Stop();
	}
}

ff::IAudioDevice *ff::AudioMusic::GetDevice() const
{
	return _device;
}

void ff::AudioMusic::Reset()
{
}

ff::AudioMusicPlaying::AudioMusicPlaying()
	: _state(State::MUSIC_INVALID)
	, _duration(0)
	, _desiredPosition(0)
	, _parent(nullptr)
	, _source(nullptr)
	, _freqRatio(1)
	, _volume(1)
	, _playVolume(1)
	, _fadeVolume(1)
	, _fadeScale(0)
	, _startPlaying(false)
	, _mediaState(MediaState::None)
{
	_asyncEvent = ::CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET | CREATE_EVENT_INITIAL_SET, EVENT_ALL_ACCESS);
	_stopEvent = ::CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
}

ff::AudioMusicPlaying::~AudioMusicPlaying()
{
}

HRESULT ff::AudioMusicPlaying::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	return __super::_Construct(unkOuter);
}

void ff::AudioMusicPlaying::_Destruct()
{
	Reset();

	if (_mediaCallback)
	{
		_mediaCallback->SetParent(nullptr);
	}

	__super::_Destruct();
}

ff::AudioMusicPlaying::State ff::AudioMusicPlaying::GetState() const
{
	return _state;
}

bool ff::AudioMusicPlaying::Init(AudioMusic *parent, IAudioStream *stream, bool startPlaying, float volume, float freqRatio)
{
	assert(IsRunningOnMainThread());
	assertRetVal(_state == State::MUSIC_INVALID, false);
	assertRetVal(stream && _device && _device->GetAudio(), false);

	_state = State::MUSIC_INIT;
	_parent = parent;
	_stream = stream;
	_startPlaying = startPlaying;
	_volume = volume;
	_freqRatio = freqRatio;

	StartAsyncWork();

	return true;
}

// background thread init
void ff::AudioMusicPlaying::Run()
{
	assert(!IsRunningOnMainThread() && !ff::IsEventSet(_asyncEvent));

	if (!_source && !IsEventSet(_stopEvent))
	{
		verify(AsyncInit());
	}

	StartReadSample();

	::SetEvent(_asyncEvent);
}

bool ff::AudioMusicPlaying::AsyncInit()
{
	assertRetVal(_stream && !_mediaCallback && !_source && !_mediaReader, false);

	ComPtr<IDataReader> reader;
	assertRetVal(_stream->CreateReader(&reader), false);

	ComPtr<IMFByteStream> mediaStream;
	assertRetVal(ff::CreateReadStream(reader, &mediaStream), false);

	ComPtr<IMFAttributes> mediaAttributes;
	assertHrRetVal(MFCreateAttributes(&mediaAttributes, 1), false);

	_mediaCallback = new ComObject<AudioSourceReaderCallback>();
	_mediaCallback->SetParent(this);
	assertHrRetVal(mediaAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, _mediaCallback.Interface()), false);

	ComPtr<IMFMediaType> mediaType;
	assertHrRetVal(MFCreateMediaType(&mediaType), false);
	assertHrRetVal(mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio), false);
	assertHrRetVal(mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float), false);

	ComPtr<IMFSourceReader> mediaReader;
	assertHrRetVal(MFCreateSourceReaderFromByteStream(mediaStream, mediaAttributes, &mediaReader), false);
	assertHrRetVal(mediaReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaType), false);

	mediaType = nullptr;
	assertHrRetVal(mediaReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &mediaType), false);

	PROPVARIANT durationValue;
	assertHrRetVal(mediaReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &durationValue), false);

	WAVEFORMATEX *waveFormat = nullptr;
	UINT waveFormatLength = 0;
	assertHrRetVal(MFCreateWaveFormatExFromMFMediaType(mediaType, &waveFormat, &waveFormatLength), false);

	XAUDIO2_SEND_DESCRIPTOR sendDesc;
	sendDesc.Flags = 0;
	sendDesc.pOutputVoice = _device->GetVoice(AudioVoiceType::MUSIC);

	XAUDIO2_VOICE_SENDS sends;
	sends.SendCount = 1;
	sends.pSends = &sendDesc;

	IXAudio2SourceVoice *source = nullptr;
	HRESULT hr = _device->GetAudio()->CreateSourceVoice(
		&source,
		waveFormat,
		0, // flags
		XAUDIO2_DEFAULT_FREQ_RATIO,
		this, // callback,
		&sends);

	CoTaskMemFree(waveFormat);
	waveFormat = nullptr;
	assertHrRetVal(hr, false);

	LockMutex crit(_cs);

	_duration = (LONGLONG)durationValue.uhVal.QuadPart;

	if (_desiredPosition >= 0 && _desiredPosition <= _duration)
	{
		PROPVARIANT value;
		::PropVariantInit(&value);

		value.vt = VT_I8;
		value.hVal.QuadPart = _desiredPosition;
		verifyHr(mediaReader->SetCurrentPosition(GUID_NULL, value));

		::PropVariantClear(&value);
	}

	if (_state == State::MUSIC_INIT)
	{
		UpdateSourceVolume(source);
		source->SetFrequencyRatio(_freqRatio);
		_source = source;
		_mediaReader = mediaReader;
	}
	else
	{
		source->DestroyVoice();
	}

	assertRetVal(_source, false);
	return true;
}

void ff::AudioMusicPlaying::SetParent(AudioMusic *parent)
{
	assert(IsRunningOnMainThread());

	LockMutex crit(_cs);
	_parent = parent;
}

bool ff::AudioMusicPlaying::IsPlaying() const
{
	assert(IsRunningOnMainThread());

	LockMutex crit(_cs);
	return _state == State::MUSIC_PLAYING || (_state == State::MUSIC_INIT && _startPlaying);
}

bool ff::AudioMusicPlaying::IsPaused() const
{
	assert(IsRunningOnMainThread());

	LockMutex crit(_cs);
	return _state == AudioMusicPlaying::State::MUSIC_PAUSED || (_state == State::MUSIC_INIT && !_startPlaying);
}

bool ff::AudioMusicPlaying::IsStopped() const
{
	assert(IsRunningOnMainThread());

	LockMutex crit(_cs);
	return _state == State::MUSIC_DONE;
}

void ff::AudioMusicPlaying::Advance()
{
	assert(IsRunningOnMainThread());

	if (_state == State::MUSIC_PLAYING && _source && _fadeScale != 0)
	{
		// Fade the volume in or out
		_fadeTimer.Tick();

		float absFadeScale = std::abs(_fadeScale);
		_fadeVolume = (float)ff::Clamp(_fadeTimer.GetSeconds() * absFadeScale, 0.0, 1.0);

		bool fadeDone = (_fadeVolume >= 1);
		bool fadeOut = (_fadeScale < 0);

		if (_fadeScale < 0)
		{
			_fadeVolume = 1.0f - _fadeVolume;
		}

		UpdateSourceVolume(_source);

		if (fadeDone)
		{
			_fadeScale = 0;

			if (fadeOut)
			{
				Stop();
			}
		}
	}

	if (_state == State::MUSIC_DONE)
	{
		ComPtr<IAudioPlaying> keepAlive = this;
		LockMutex crit(_cs);

		if (_state == State::MUSIC_DONE)
		{
			OnMusicDone();
		}
	}
}

void ff::AudioMusicPlaying::Stop()
{
	assert(IsRunningOnMainThread());

	LockMutex crit(_cs);

	if (_state != State::MUSIC_DONE)
	{
		if (_source)
		{
			_source->Stop();
		}

		_state = State::MUSIC_DONE;
	}
}

void ff::AudioMusicPlaying::Pause()
{
	assert(IsRunningOnMainThread());

	LockMutex crit(_cs);

	if (_state == State::MUSIC_INIT)
	{
		_startPlaying = false;
	}
	else if (_state == State::MUSIC_PLAYING && _source)
	{
		_desiredPosition = (LONGLONG)(GetPosition() * 10000000.0);
		_source->Stop();
		_state = State::MUSIC_PAUSED;
	}
}

void ff::AudioMusicPlaying::Resume()
{
	assert(IsRunningOnMainThread());

	LockMutex crit(_cs);

	if (_state == State::MUSIC_INIT)
	{
		_startPlaying = true;
	}
	else if (_state == State::MUSIC_PAUSED && _source)
	{
		_source->Start();
		_state = State::MUSIC_PLAYING;
	}
}

double ff::AudioMusicPlaying::GetDuration() const
{
	LockMutex crit(_cs);

	return _duration / 10000000.0;
}

double ff::AudioMusicPlaying::GetPosition() const
{
	LockMutex crit(_cs);
	double pos = 0;
	bool useDesiredPosition = false;

	switch (_state)
	{
	case State::MUSIC_INIT:
	case State::MUSIC_PAUSED:
		useDesiredPosition = true;
		break;

	case State::MUSIC_PLAYING:
		{
			XAUDIO2_VOICE_STATE state;
			_source->GetState(&state);

			XAUDIO2_VOICE_DETAILS details;
			_source->GetVoiceDetails(&details);

			BufferInfo *info = reinterpret_cast<BufferInfo *>(state.pCurrentBufferContext);
			if (info && info->_startSamples != (UINT64)-1)
			{
				double sampleSeconds = (state.SamplesPlayed > info->_startSamples)
					? (state.SamplesPlayed - info->_startSamples) / (double) details.InputSampleRate
					: 0.0;
				pos = sampleSeconds + (info->_startTime / 10000000.0);
			}
			else
			{
				useDesiredPosition = true;
			}
		}
		break;
	}

	if (useDesiredPosition)
	{
		pos = _desiredPosition / 10000000.0;
	}

	return pos;
}

bool ff::AudioMusicPlaying::SetPosition(double value)
{
	LockMutex crit(_cs);

	assertRetVal(_state != State::MUSIC_DONE, false);

	_desiredPosition = (LONGLONG)(value * 10000000.0);

	if (_mediaReader && _mediaState != MediaState::Flushing)
	{
		_mediaState = MediaState::Flushing;

		if (FAILED(_mediaReader->Flush(MF_SOURCE_READER_FIRST_AUDIO_STREAM)))
		{
			_mediaState = MediaState::None;
			assertRetVal(false, false);
		}
	}

	return true;
}

double ff::AudioMusicPlaying::GetVolume() const
{
	return _playVolume;
}

bool ff::AudioMusicPlaying::SetVolume(double value)
{
	_playVolume = ff::Clamp((float)value, 0.0f, 1.0f);
	UpdateSourceVolume(_source);
	return true;
}

bool ff::AudioMusicPlaying::FadeIn(double value)
{
	assertRetVal(!IsPlaying() && value > 0, false);

	_fadeTimer.Reset();
	_fadeScale = (float)(1.0 / ff::Clamp(value, 0.0, 10.0));
	_fadeVolume = 0;

	UpdateSourceVolume(_source);

	return true;
}

bool ff::AudioMusicPlaying::FadeOut(double value)
{
	assertRetVal(IsPlaying() && value > 0, false);

	_fadeTimer.Reset();
	_fadeScale = (float)(-1.0 / ff::Clamp(value, 0.0, 10.0));
	_fadeVolume = 1;

	UpdateSourceVolume(_source);

	return true;
}

ff::IAudioDevice *ff::AudioMusicPlaying::GetDevice() const
{
	return _device;
}

void ff::AudioMusicPlaying::Reset()
{
	assert(IsRunningOnMainThread());

	CancelAsync();

	LockMutex crit(_cs);

	if (_source)
	{
		IXAudio2SourceVoice *source = _source;
		_source = nullptr;
		source->DestroyVoice();
	}

	_state = State::MUSIC_DONE;
}

void ff::AudioMusicPlaying::OnVoiceProcessingPassStart(UINT32 BytesRequired)
{
	assert(!IsRunningOnMainThread());
}

void ff::AudioMusicPlaying::OnVoiceProcessingPassEnd()
{
	assert(!IsRunningOnMainThread());
}

void ff::AudioMusicPlaying::OnStreamEnd()
{
	assert(!IsRunningOnMainThread());

	LockMutex crit(_cs);
	_state = State::MUSIC_DONE;

	// Loop?
}

void ff::AudioMusicPlaying::OnBufferStart(void *pBufferContext)
{
	assert(!IsRunningOnMainThread());

	LockMutex crit(_cs);

	if (_bufferInfos.Size() == MAX_BUFFERS)
	{
		// Reuse buffers
		assert(_bufferInfos.GetLast() == pBufferContext);
		_bufferInfos.MoveToBack(*_bufferInfos.GetFirst());
	}

	if (!_bufferInfos.IsEmpty())
	{
		XAUDIO2_VOICE_STATE state;
		_source->GetState(&state);

		BufferInfo &info = *_bufferInfos.GetFirst();
		assert(state.pCurrentBufferContext == &info);
		info._startSamples = state.SamplesPlayed;
	}

	StartReadSample();
}

void ff::AudioMusicPlaying::OnBufferEnd(void *pBufferContext)
{
	assert(!IsRunningOnMainThread());
}

void ff::AudioMusicPlaying::OnLoopEnd(void *pBufferContext)
{
	assert(!IsRunningOnMainThread());
}

void ff::AudioMusicPlaying::OnVoiceError(void *pBufferContext, HRESULT error)
{
	assert(!IsRunningOnMainThread());

	assertSz(false, L"XAudio2 voice error");
}

HRESULT ff::AudioMusicPlaying::OnReadSample(
	HRESULT hrStatus,
	DWORD dwStreamIndex,
	DWORD dwStreamFlags,
	LONGLONG llTimestamp,
	IMFSample *pSample)
{
	assert(!IsRunningOnMainThread());

	LockMutex crit(_cs);

	noAssertRetVal(_mediaState == MediaState::Reading, S_OK);
	bool endOfStream = (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0;
	_mediaState = endOfStream ? MediaState::Done : MediaState::None;

	if (_source && _state != State::MUSIC_DONE)
	{
		ComPtr<IMFMediaBuffer> mediaBuffer;
		BufferInfo *bufferInfo = nullptr;
		BYTE *data = nullptr;
		DWORD dataSize = 0;

		if (pSample && SUCCEEDED(hrStatus) &&
			SUCCEEDED(pSample->ConvertToContiguousBuffer(&mediaBuffer)) &&
			SUCCEEDED(mediaBuffer->Lock(&data, nullptr, &dataSize)))
		{
			bufferInfo = (_bufferInfos.Size() < MAX_BUFFERS) ? &_bufferInfos.Insert() : _bufferInfos.GetLast();
			bufferInfo->_buffer.Resize(dataSize);
			bufferInfo->_startTime = llTimestamp;
			bufferInfo->_startSamples = (UINT64)-1;
			bufferInfo->_streamIndex = dwStreamIndex;
			bufferInfo->_streamFlags = dwStreamFlags;

			if (FAILED(pSample->GetSampleDuration(&bufferInfo->_duration)))
			{
				bufferInfo->_duration = 0;
			}

			std::memcpy(bufferInfo->_buffer.Data(), data, dataSize);

			mediaBuffer->Unlock();
		}

		if (bufferInfo)
		{
			XAUDIO2_BUFFER buffer;
			ZeroObject(buffer);
			buffer.AudioBytes = (UINT)bufferInfo->_buffer.Size();
			buffer.pAudioData = bufferInfo->_buffer.Data();
			buffer.pContext = bufferInfo;
			buffer.Flags = endOfStream ? XAUDIO2_END_OF_STREAM : 0;
			verifyHr(_source->SubmitSourceBuffer(&buffer));
#if DEBUG_THIS_FILE
			ff::Log::DebugTraceF(L"AudioMusicPlaying::OnReadSample, Size:%lu, Time:%lu-%lu\n",
				bufferInfo->_buffer.Size(),
				llTimestamp,
				llTimestamp + bufferInfo->_duration);
#endif
			if (_state == State::MUSIC_INIT)
			{
				_state = State::MUSIC_PAUSED;

				if (_startPlaying)
				{
					_startPlaying = false;
					_source->Start();
					_state = State::MUSIC_PLAYING;
				}
			}
		}
		else if (_state == State::MUSIC_INIT)
		{
			_state = State::MUSIC_DONE;
		}
		else
		{
			_source->Discontinuity();
		}
	}

	return S_OK;
}

HRESULT ff::AudioMusicPlaying::OnFlush(DWORD dwStreamIndex)
{
	assert(!IsRunningOnMainThread());

	LockMutex crit(_cs);

	assert(_mediaState == MediaState::Flushing);
	_mediaState = MediaState::None;

#if DEBUG_THIS_FILE
	ff::Log::DebugTraceF(L"AudioMusicPlaying::OnFlush\n");
#endif

	if (_desiredPosition >= 0 && _desiredPosition <= _duration)
	{
		if (_source)
		{
			_startPlaying = _startPlaying || (_state == State::MUSIC_PLAYING);
			_source->Stop();
			_source->FlushSourceBuffers();
			_state = State::MUSIC_INIT;
		}

		PROPVARIANT value;
		::PropVariantInit(&value);
		value.vt = VT_I8;
		value.hVal.QuadPart = _desiredPosition;

		verifyHr(_mediaReader->SetCurrentPosition(GUID_NULL, value));

		::PropVariantClear(&value);
	}

	StartReadSample();

	return S_OK;
}

HRESULT ff::AudioMusicPlaying::OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent)
{
	assert(!IsRunningOnMainThread());

	return S_OK;
}

void ff::AudioMusicPlaying::StartAsyncWork()
{
	assert(IsRunningOnMainThread());

	::ResetEvent(_asyncEvent);
	ff::ProcessGlobals::Get()->GetThreadPool()->Add(this);
}

void ff::AudioMusicPlaying::CancelAsync()
{
	assert(IsRunningOnMainThread());

	::SetEvent(_stopEvent);
	WaitForHandle(_asyncEvent);
	::ResetEvent(_stopEvent);
}

void ff::AudioMusicPlaying::StartReadSample()
{
	assert(!IsRunningOnMainThread());

	LockMutex crit(_cs);
	HRESULT hr = E_FAIL;

	noAssertRet(_mediaState == MediaState::None);
	_mediaState = MediaState::Reading;

	if (_mediaReader)
	{
		hr = _mediaReader->ReadSample(
			MF_SOURCE_READER_FIRST_AUDIO_STREAM,
			0, nullptr, nullptr, nullptr, nullptr);
	}

	if (FAILED(hr) && _source)
	{
		_source->Discontinuity();
	}
}

void ff::AudioMusicPlaying::OnMusicDone()
{
	assert(IsRunningOnMainThread());

	ComPtr<IAudioPlaying> keepAlive = this;
	LockMutex crit(_cs);

	if (_parent)
	{
		_parent->OnMusicDone(this);
		_parent = nullptr;
	}

	if (_source)
	{
		ComPtr<DestroyVoiceWorkItem, IWorkItem> workItem;

		if (!ff::CreateDestroyVoiceWorkItem(_device, _source, &workItem))
		{
			_source->DestroyVoice();
		}
		else
		{
			ProcessGlobals::Get()->GetThreadPool()->Add(workItem);
		}

		_source = nullptr;
	}
}

void ff::AudioMusicPlaying::UpdateSourceVolume(IXAudio2SourceVoice *source)
{
	assert(source == _source || !IsRunningOnMainThread());

	if (source)
	{
		source->SetVolume(_volume * _playVolume * _fadeVolume);
	}
}

ff::AudioSourceReaderCallback::AudioSourceReaderCallback()
	: _parent(nullptr)
{
}

ff::AudioSourceReaderCallback::~AudioSourceReaderCallback()
{
	assert(!_parent);
}

void ff::AudioSourceReaderCallback::SetParent(AudioMusicPlaying *parent)
{
	LockMutex crit(_cs);
	_parent = parent;
}

HRESULT ff::AudioSourceReaderCallback::OnReadSample(
	HRESULT hrStatus,
	DWORD dwStreamIndex,
	DWORD dwStreamFlags,
	LONGLONG llTimestamp,
	IMFSample *pSample)
{
	LockMutex crit(_cs);

	if (_parent)
	{
		ComPtr<IUnknown> keepAlive = _parent->_GetUnknown();
		return _parent->OnReadSample(hrStatus, dwStreamIndex, dwStreamFlags, llTimestamp, pSample);
	}

	return S_OK;
}

HRESULT ff::AudioSourceReaderCallback::OnFlush(DWORD dwStreamIndex)
{
	LockMutex crit(_cs);

	if (_parent)
	{
		ComPtr<IUnknown> keepAlive = _parent->_GetUnknown();
		return _parent->OnFlush(dwStreamIndex);
	}

	return S_OK;
}

HRESULT ff::AudioSourceReaderCallback::OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent)
{
	LockMutex crit(_cs);

	if (_parent)
	{
		ComPtr<IUnknown> keepAlive = _parent->_GetUnknown();
		return _parent->OnEvent(dwStreamIndex, pEvent);
	}

	return S_OK;
}
