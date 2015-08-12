#pragma once

#include "Audio/AudioDeviceChild.h"
#include "Thread/ThreadPool.h"

namespace ff
{
	class __declspec(uuid("3638c79d-784e-4ded-830e-26e400fa5af5"))
		DestroyVoiceWorkItem
			: public IWorkItem
			, public IAudioDeviceChild
	{
	public:
		DECLARE_HEADER(DestroyVoiceWorkItem);

		bool Init(IXAudio2SourceVoice *source);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		virtual void _DeleteThis() override;

		// IWorkItem
		virtual void Run() override;
		virtual void OnCancel() override;
		virtual void OnComplete() override;

		// IAudioDeviceChild
		virtual IAudioDevice *GetDevice() const override;
		virtual void Reset() override;

	private:
		ComPtr<IAudioDevice> _device;
		IXAudio2SourceVoice *_source;
	};

	bool CreateDestroyVoiceWorkItem(IAudioDevice *device, IXAudio2SourceVoice *source, DestroyVoiceWorkItem **obj);
}
