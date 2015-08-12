#pragma once

namespace ff
{
	class IAudioDevice;

	enum class AudioVoiceType
	{
		MASTER,
		EFFECTS,
		MUSIC,
	};

	class __declspec(uuid("bd29d94d-5c4d-4f97-8856-9789d5c4e02b")) __declspec(novtable)
		IAudioDevice : public IUnknown
	{
	public:
		virtual bool IsValid() const = 0;
		virtual void Destroy() = 0;
		virtual void Reset() = 0;

		virtual void Stop() = 0;
		virtual void Start() = 0;

		virtual float GetVolume(AudioVoiceType type) const = 0;
		virtual void  SetVolume(AudioVoiceType type, float volume) = 0;

		virtual void AdvanceEffects() = 0;
		virtual void StopEffects() = 0;
		virtual void PauseEffects() = 0;
		virtual void ResumeEffects() = 0;

		virtual IXAudio2 *GetAudio() const = 0;
		virtual IXAudio2Voice *GetVoice(AudioVoiceType type) const = 0;
	};

	// You should use GetAudioFactory()->CreateDevice(...) instead
	bool CreateAudioDevice(
		IXAudio2 *audio,
		StringRef name,
		size_t channels,
		size_t sampleRate,
		IAudioDevice **device);
}
