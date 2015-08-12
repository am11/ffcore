#include "pch.h"

#include <Globals/MetroProcessGlobals.h>

#include "App.xaml.h"
#include "AppGlobals.h"
#include "MainPage.xaml.h"

using namespace Platform;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Xaml;

FerretPlayer::App::App()
{
	InitializeComponent();
}

FerretPlayer::App::~App()
{
	_appGlobals.reset();

	if (_processGlobals != nullptr)
	{
		_processGlobals->Shutdown();
		_processGlobals.reset();
	}
}

FerretPlayer::App ^FerretPlayer::App::Current::get()
{
	return safe_cast<App ^>(Application::Current);
}

FerretPlayer::MainPage ^FerretPlayer::App::Page::get()
{
	return App::Current->_mainPage;
}

Windows::ApplicationModel::Activation::SplashScreen ^FerretPlayer::App::OriginalSplashScreen::get()
{
	return _originalSplashScreen;
}

void FerretPlayer::App::OriginalSplashScreen::set(Windows::ApplicationModel::Activation::SplashScreen ^value)
{
	assert(value == nullptr);
	_originalSplashScreen = value;
}

void FerretPlayer::App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args)
{
	_originalSplashScreen = args->SplashScreen;

	InitWindow();
	InitApp();
}

void FerretPlayer::App::InitWindow()
{
	_mainPage = ref new MainPage();
	Window::Current->Content = _mainPage;
}

bool FerretPlayer::App::InitApp()
{
	bool success = true;
	_processGlobals.reset(new ff::MetroProcessGlobals());

	if (_processGlobals->Startup() && _processGlobals->IsValid())
	{
		_appGlobals.reset(new FerretPlayer::AppGlobals(*_processGlobals));
		success = _appGlobals->WinMain();
	}

	if (!success)
	{
		ff::String logFile;
		if (_appGlobals != nullptr)
		{
			logFile = _appGlobals->GetLogFile();
			_appGlobals.reset();
		}

		if (_processGlobals != nullptr)
		{
			_processGlobals->Shutdown();
			_processGlobals.reset();
		}

		InitFailure(logFile);
	}

	return true;
}

void FerretPlayer::App::InitFailure(ff::String logFile)
{
	_mainPage = nullptr;
	// Window::Current->Content = ref new FailurePage(logFile.pstring());
	Window::Current->Activate();
}
