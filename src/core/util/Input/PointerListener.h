#pragma once
#if METRO_APP

namespace ff
{
	using namespace Platform;
	using namespace Windows::Foundation;
	using namespace Windows::Graphics::Display;
	using namespace Windows::UI::Core;

	class IPointerEventListener
	{
	public:
		virtual void OnPointerEntered(CoreWindow ^sender, PointerEventArgs ^args) = 0;
		virtual void OnPointerExited(CoreWindow ^sender, PointerEventArgs ^args) = 0;
		virtual void OnPointerMoved(CoreWindow ^sender, PointerEventArgs ^args) = 0;
		virtual void OnPointerPressed(CoreWindow ^sender, PointerEventArgs ^args) = 0;
		virtual void OnPointerReleased(CoreWindow ^sender, PointerEventArgs ^args) = 0;
		virtual void OnPointerCaptureLost(CoreWindow ^sender, PointerEventArgs ^args) = 0;
	};

	ref class PointerEvents
	{
	internal:
		PointerEvents(IPointerEventListener *pParent, CoreWindow ^window);
		float GetDpiScale() const;

	public:
		virtual ~PointerEvents();
		void Destroy();

	private:
		void OnPointerEntered(CoreWindow ^sender, PointerEventArgs ^args);
		void OnPointerExited(CoreWindow ^sender, PointerEventArgs ^args);
		void OnPointerMoved(CoreWindow ^sender, PointerEventArgs ^args);
		void OnPointerPressed(CoreWindow ^sender, PointerEventArgs ^args);
		void OnPointerReleased(CoreWindow ^sender, PointerEventArgs ^args);
		void OnPointerCaptureLost(CoreWindow ^sender, PointerEventArgs ^args);

		void OnLogicalDpiChanged(DisplayInformation ^displayInfo, Object ^sender);

		float _dpiScale;
		Agile<CoreWindow> _window;
		DisplayInformation ^_displayInfo;
		IPointerEventListener *_parent;
		Windows::Foundation::EventRegistrationToken _tokens[7];
	};
}

#endif // METRO_APP
