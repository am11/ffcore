#include "pch.h"
#include <Audio/AudioDevice.h>
#include <Audio/AudioMusic.h>
#include <Audio/AudioPlaying.h>
#include <Audio/AudioStream.h>
#include <String/StringUtil.h>

#include "MusicModel.h"

using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

FerretPlayer::MusicModel::MusicModel(ff::IAudioDevice *device)
	: _position(0)
	, _duration(0)
	, _playVolume(1)
	, _fadeIn(false)
	, _fadeOut(false)
	, _ignoreChanges(0)
{
	_device = device;
}

FerretPlayer::MusicModel::~MusicModel()
{
}

String ^FerretPlayer::MusicModel::FileName::get()
{
	ff::String tail = ff::GetPathTail(_path);
	return tail.empty() ? ref new String(L"<-- Click to choose a file") : tail.pstring();
}

double FerretPlayer::MusicModel::Position::get()
{
	return _position;
}

void FerretPlayer::MusicModel::Position::set(double value)
{
	if (!_ignoreChanges && _playing && _duration > 0)
	{
		value = ff::Clamp(value, 0.0, _duration);
		_playing->SetPosition(value);
	}
}

String ^FerretPlayer::MusicModel::PositionText::get()
{
	return ff::String::format_new(L"%.2f/%.2f", _position, _duration).pstring();
}

double FerretPlayer::MusicModel::Duration::get()
{
	return _duration;
}

double FerretPlayer::MusicModel::PlayVolume::get()
{
	return _playVolume * 100.0;
}

void FerretPlayer::MusicModel::PlayVolume::set(double value)
{
	_playVolume = ff::Clamp(value / 100, 0.0, 1.0);

	if (_playing)
	{
		_playing->SetVolume(_playVolume);
	}

	NotifyPropertyChanged("PlayVolume");
}

bool FerretPlayer::MusicModel::IsPositionEnabled::get()
{
	return _duration > 0;
}

bool FerretPlayer::MusicModel::IsPlaying::get()
{
	return _playing && _playing->IsPlaying();
}

bool FerretPlayer::MusicModel::IsFadeIn::get()
{
	return _fadeIn;
}

void FerretPlayer::MusicModel::IsFadeIn::set(bool value)
{
	_fadeIn = value;
	NotifyPropertyChanged("IsFadeIn");
}

bool FerretPlayer::MusicModel::IsFadeOut::get()
{
	return _fadeOut;
}

void FerretPlayer::MusicModel::IsFadeOut::set(bool value)
{
	_fadeOut = value;
	NotifyPropertyChanged("IsFadeOut");
}

bool FerretPlayer::MusicModel::CanPlay::get()
{
	return _music != nullptr && !IsPlaying;
}

bool FerretPlayer::MusicModel::CanPause::get()
{
	return _playing != nullptr && _playing->IsPlaying();
}

bool FerretPlayer::MusicModel::CanStop::get()
{
	return _playing != nullptr;
}

bool FerretPlayer::MusicModel::CanRewind::get()
{
	return CanPause;
}

bool FerretPlayer::MusicModel::CanForward::get()
{
	return CanPause;
}

double FerretPlayer::MusicModel::PlayOpacity::get()
{
	return CanPlay ? 1.0 : 0.25;
}

double FerretPlayer::MusicModel::PauseOpacity::get()
{
	return CanPause ? 1.0 : 0.25;
}

double FerretPlayer::MusicModel::StopOpacity::get()
{
	return CanStop ? 1.0 : 0.25;
}

double FerretPlayer::MusicModel::RewindOpacity::get()
{
	return CanRewind ? 1.0 : 0.25;
}

double FerretPlayer::MusicModel::ForwardOpacity::get()
{
	return CanForward ? 1.0 : 0.25;
}

void FerretPlayer::MusicModel::Reset()
{
	_path.clear();
	_music = nullptr;
	_playing = nullptr;
	_position = 0;

	NotifyPropertyChanged();
}

bool FerretPlayer::MusicModel::Open(String ^path, IRandomAccessStream ^stream)
{
	assertRetVal(_device && stream != nullptr, false);

	ff::ComPtr<ff::IAudioStream> audioStream;
	assertRetVal(ff::CreateAudioStream(stream, &audioStream), false);

	ff::ComPtr<ff::IAudioMusic> music;
	assertRetVal(ff::CreateAudioMusic(_device, audioStream, &music), false);

	_path = path ? path->Data() : L"";
	_music = music;
	_playing = nullptr;
	_position = 0;

	NotifyPropertyChanged();

	return true;
}

void FerretPlayer::MusicModel::Advance()
{
	if (_playing && _playing->IsStopped())
	{
		_playing = nullptr;
		NotifyPropertyChanged();
	}

	double duration = _playing ? _duration : 0;
	double position = _playing ? _position : 0;
	double playVolume = _playing ? _playVolume : 1;

	if (_playing && _playing->IsPlaying())
	{
		duration = _playing->GetDuration();
		position = _playing->GetPosition();
		playVolume = _playing->GetVolume();
	}

	if (_duration != duration)
	{
		_duration = duration;
		NotifyPropertyChanged();
	}

	if (_position != position)
	{
		_position = position;
		NotifyPropertyChanged("Position");
		NotifyPropertyChanged("PositionText");
	}

	if (_playVolume != playVolume)
	{
		_playVolume = playVolume;
		NotifyPropertyChanged("PlayVolume");
	}
}

void FerretPlayer::MusicModel::Play()
{
	if (_playing && _playing->IsStopped())
	{
		_playing = nullptr;
		NotifyPropertyChanged();
	}

	if (!_playing && _music)
	{
		verify(_music->Play(&_playing, false, 1, 1));
	}

	if (_playing)
	{
		if (_fadeIn)
		{
			_playing->FadeIn(5);
		}

		_playing->SetVolume(_playVolume);
		_playing->Resume();
		NotifyPropertyChanged();
	}
}

void FerretPlayer::MusicModel::Pause()
{
	if (_playing)
	{
		_playing->Pause();
		NotifyPropertyChanged();
	}
}

void FerretPlayer::MusicModel::Stop()
{
	if (_playing)
	{
		if (_fadeOut)
		{
			_playing->FadeOut(5);
		}
		else
		{
			_playing->Stop();
		}

		_playing = nullptr;

		NotifyPropertyChanged();
	}
}

void FerretPlayer::MusicModel::Rewind()
{
	if (_playing)
	{
		_playing->SetPosition(_playing->GetPosition() - _playing->GetDuration() / 16);
		NotifyPropertyChanged();
	}
}

void FerretPlayer::MusicModel::Forward()
{
	if (_playing)
	{
		_playing->SetPosition(_playing->GetPosition() + _playing->GetDuration() / 16);
		NotifyPropertyChanged();
	}
}

void FerretPlayer::MusicModel::NotifyPropertyChanged(String ^name)
{
	_ignoreChanges++;
	PropertyChanged(this, ref new PropertyChangedEventArgs(name ? name : ""));
	_ignoreChanges--;
}
