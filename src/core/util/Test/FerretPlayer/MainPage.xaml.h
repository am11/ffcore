#pragma once

#include "MainPage.g.h"
#include "MusicControl.xaml.h"
#include "MusicModel.h"

namespace ff
{
	class IAudioDevice;
}

namespace FerretPlayer
{
	using namespace Platform;
	using namespace Windows::UI::Xaml;

	public ref class MainPage sealed
	{
	public:
		MainPage();
		virtual ~MainPage();

	private:
		void OnLoaded(Object ^sender, RoutedEventArgs ^args);
		void OnUnloaded(Object ^sender, RoutedEventArgs ^args);

		ff::ComPtr<ff::IAudioDevice> _device;
		ff::Vector<MusicControl ^> _controls;
	};
}
