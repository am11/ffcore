#pragma once

namespace ff
{
	class IAudioDevice;
	class IAudioMusic;
	class IAudioPlaying;
}

namespace FerretPlayer
{
	using namespace Platform;
	using namespace Windows::Foundation::Metadata;
	using namespace Windows::Storage::Streams;
	using namespace Windows::UI::Xaml::Data;

	[Bindable]
	[WebHostHidden]
	public ref class MusicModel sealed : INotifyPropertyChanged
	{
	internal:
		MusicModel(ff::IAudioDevice *device);

	public:
		virtual ~MusicModel();

		virtual event PropertyChangedEventHandler ^PropertyChanged;

		property String ^FileName { String ^get(); }
		property String ^PositionText { String ^get(); }
		property double Position { double get(); void set(double value); }
		property double Duration { double get(); }
		property double PlayVolume { double get(); void set(double value); }

		property bool IsPositionEnabled { bool get(); }
		property bool IsPlaying { bool get(); }
		property bool IsFadeIn { bool get(); void set(bool value); }
		property bool IsFadeOut { bool get(); void set(bool value); }

		property bool CanPlay { bool get(); }
		property bool CanPause { bool get(); }
		property bool CanStop { bool get(); }
		property bool CanRewind { bool get(); }
		property bool CanForward { bool get(); }

		property double PlayOpacity { double get(); }
		property double PauseOpacity { double get(); }
		property double StopOpacity { double get(); }
		property double RewindOpacity { double get(); }
		property double ForwardOpacity { double get(); }

		void Reset();
		bool Open(String ^path, IRandomAccessStream ^file);
		void Advance();

		void Play();
		void Pause();
		void Stop();
		void Rewind();
		void Forward();

	private:
		void NotifyPropertyChanged(String ^name = nullptr);

		ff::String _path;
		ff::ComPtr<ff::IAudioDevice> _device;
		ff::ComPtr<ff::IAudioMusic> _music;
		ff::ComPtr<ff::IAudioPlaying> _playing;
		double _position;
		double _duration;
		double _playVolume;
		bool _fadeIn;
		bool _fadeOut;
		int _ignoreChanges;
	};
}
