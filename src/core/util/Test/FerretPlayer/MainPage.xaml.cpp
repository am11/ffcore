#include "pch.h"

#include <Audio/AudioDevice.h>
#include <Audio/AudioFactory.h>
#include <Globals/ProcessGlobals.h>

#include "App.xaml.h"
#include "MainPage.xaml.h"

using namespace Platform;
using namespace Windows::UI::Xaml;

FerretPlayer::MainPage::MainPage()
{
	InitializeComponent();

	_controls.Push(MusicControl0);
	_controls.Push(MusicControl1);
	//_controls.Push(MusicControl2);
	//_controls.Push(MusicControl3);
}

FerretPlayer::MainPage::~MainPage()
{
}

void FerretPlayer::MainPage::OnLoaded(Object ^sender, RoutedEventArgs ^args)
{
	if (!_device)
	{
		ff::ProcessGlobals::Get()->GetAudioFactory()->CreateDefaultDevice(&_device);
	}

	for (MusicControl ^control: _controls)
	{
		control->Reset(_device);
	}
}

void FerretPlayer::MainPage::OnUnloaded(Object ^sender, RoutedEventArgs ^args)
{
	_device = nullptr;
}
