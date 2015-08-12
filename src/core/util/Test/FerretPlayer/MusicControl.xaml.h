#pragma once

#include "MusicControl.g.h"
#include "MusicModel.h"

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
	using namespace Windows::UI::Xaml;
	using namespace Windows::UI::Xaml::Data;

	[Bindable]
	[WebHostHidden]
	public ref class MusicControl sealed : INotifyPropertyChanged
	{
	public:
		MusicControl();
		virtual ~MusicControl();

		virtual event PropertyChangedEventHandler ^PropertyChanged;

		property MusicModel ^Model { MusicModel ^get(); }

	internal:
		void Reset(ff::IAudioDevice *device);

	private:
		void NotifyPropertyChanged(String ^name = nullptr);

		void OnLoaded(Object ^sender, RoutedEventArgs ^args);
		void OnUnloaded(Object ^sender, RoutedEventArgs ^args);
		void OnTick(Object ^sender, Object ^args);

		void OnClickOpenButton(Object ^sender, RoutedEventArgs ^args);
		void OnClickRewindButton(Object ^sender, RoutedEventArgs ^args);
		void OnClickPauseButton(Object ^sender, RoutedEventArgs ^args);
		void OnClickPlayButton(Object ^sender, RoutedEventArgs ^args);
		void OnClickForwardButton(Object ^sender, RoutedEventArgs ^args);
		void OnClickStopButton(Object ^sender, RoutedEventArgs ^args);
		void OnClickFadeInButton(Object ^sender, RoutedEventArgs ^args);
		void OnClickFadeOutButton(Object ^sender, RoutedEventArgs ^args);

		MusicModel ^_model;
		ff::ComPtr<ff::IAudioMusic> _music;
		ff::ComPtr<ff::IAudioPlaying> _playing;
		DispatcherTimer ^_timer;
		Windows::Foundation::EventRegistrationToken _timerToken;
	};
}
