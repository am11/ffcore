#pragma once

#include "App.g.h"

namespace ff
{
	class AppGlobals;
	class MetroProcessGlobals;
}

namespace FerretPlayer
{
	using namespace Platform;
	using namespace Windows::ApplicationModel::Activation;
	using namespace Windows::UI::Xaml;

	ref class MainPage;

	ref class App sealed
	{
	public:
		App();
		virtual ~App();

		virtual void OnLaunched(LaunchActivatedEventArgs ^args) override;

	internal:
		static property App ^Current { App ^get(); }
		static property MainPage ^Page { MainPage ^get(); }

		property Windows::ApplicationModel::Activation::SplashScreen ^OriginalSplashScreen
		{
			Windows::ApplicationModel::Activation::SplashScreen ^get();
			void set(Windows::ApplicationModel::Activation::SplashScreen ^value);
		}

	private:
		void InitWindow();
		bool InitApp();
		void InitFailure(ff::String logFile);

		Windows::ApplicationModel::Activation::SplashScreen ^_originalSplashScreen;
		MainPage ^_mainPage;
		std::unique_ptr<ff::MetroProcessGlobals> _processGlobals;
		std::unique_ptr<ff::AppGlobals> _appGlobals;
	};
}
