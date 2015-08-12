#include "pch.h"
#include <Audio/AudioDevice.h>
#include <Audio/AudioMusic.h>
#include <Audio/AudioPlaying.h>
#include "MusicControl.xaml.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml;

FerretPlayer::MusicControl::MusicControl()
{
	DataContext = this;
	InitializeComponent();
}

FerretPlayer::MusicControl::~MusicControl()
{
	if (_model)
	{
		_model->Reset();
	}
}

void FerretPlayer::MusicControl::Reset(ff::IAudioDevice *device)
{
	if (_model)
	{
		_model->Reset();
	}

	_model = ref new MusicModel(device);
	NotifyPropertyChanged();
}

FerretPlayer::MusicModel ^FerretPlayer::MusicControl::Model::get()
{
	return _model;
}

void FerretPlayer::MusicControl::NotifyPropertyChanged(String ^name)
{
	PropertyChanged(this, ref new PropertyChangedEventArgs(name ? name : ""));
}

void FerretPlayer::MusicControl::OnLoaded(Object ^sender, RoutedEventArgs ^args)
{
	DataContext = this;
	NotifyPropertyChanged();

	if (_timer != nullptr)
	{
		_timer->Stop();
		_timer = nullptr;
	}

	_timer = ref new DispatcherTimer();

	TimeSpan timeSpan;
	timeSpan.Duration = (LONGLONG)(0.0625 * 10000000.0);
	_timer->Interval = timeSpan;
	_timerToken = _timer->Tick += ref new EventHandler<Object ^>(this, &FerretPlayer::MusicControl::OnTick);
	_timer->Start();
}

void FerretPlayer::MusicControl::OnUnloaded(Object ^sender, RoutedEventArgs ^args)
{
	if (_timer != nullptr)
	{
		_timer->Tick -= _timerToken;
		_timer->Stop();
		_timer = nullptr;
	}

	DataContext = nullptr;
	NotifyPropertyChanged();
}

void FerretPlayer::MusicControl::OnTick(Object ^sender, Object ^args)
{
	if (_model)
	{
		_model->Advance();
	}
}

void FerretPlayer::MusicControl::OnClickOpenButton(Object ^sender, RoutedEventArgs ^args)
{
	FileOpenPicker ^picker = ref new FileOpenPicker();
	picker->SuggestedStartLocation = PickerLocationId::MusicLibrary;
	picker->FileTypeFilter->Append("*");

	concurrency::create_task(picker->PickSingleFileAsync()).then([this](StorageFile ^file)
	{
		if (file)
		{
			concurrency::create_task(file->OpenReadAsync()).then([this, file](IRandomAccessStream ^stream)
			{
				if (_model && stream)
				{
					_model->Open(file->Path, stream);
				}
			});
		}
	});
}

void FerretPlayer::MusicControl::OnClickRewindButton(Object ^sender, RoutedEventArgs ^args)
{
	if (_model)
	{
		_model->Rewind();
	}
}

void FerretPlayer::MusicControl::OnClickPauseButton(Object ^sender, RoutedEventArgs ^args)
{
	if (_model)
	{
		_model->Pause();
	}
}

void FerretPlayer::MusicControl::OnClickPlayButton(Object ^sender, RoutedEventArgs ^args)
{
	if (_model)
	{
		_model->Play();
	}
}

void FerretPlayer::MusicControl::OnClickForwardButton(Object ^sender, RoutedEventArgs ^args)
{
	if (_model)
	{
		_model->Forward();
	}
}

void FerretPlayer::MusicControl::OnClickStopButton(Object ^sender, RoutedEventArgs ^args)
{
	if (_model)
	{
		_model->Stop();
	}
}

void FerretPlayer::MusicControl::OnClickFadeInButton(Object ^sender, RoutedEventArgs ^args)
{
	if (_model)
	{
		_model->IsFadeIn = !_model->IsFadeIn;
	}
}

void FerretPlayer::MusicControl::OnClickFadeOutButton(Object ^sender, RoutedEventArgs ^args)
{
	if (_model)
	{
		_model->IsFadeOut = !_model->IsFadeOut;
	}
}
