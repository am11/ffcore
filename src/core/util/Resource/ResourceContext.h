#pragma once

namespace ff
{
	class AppGlobals;
	class I2dEffect;
	class I2dRenderer;
	class IAudioDevice;
	class IGraphDevice;
	class IJoystickInput;
	class IKeyboardDevice;
	class IMouseDevice;
	class IRenderDepth;
	class IRenderTargetWindow;
	class ITouchDevice;

	class IResourceContext
	{
	public:
		virtual AppGlobals &GetAppGlobals() const = 0;
		virtual IRenderTargetWindow *GetTarget() const = 0;
		virtual IRenderDepth *GetDepth() const = 0;
		virtual IGraphDevice *GetGraph() const = 0;
		virtual IAudioDevice *GetAudio() const = 0;
		virtual IMouseDevice *GetMouse() const = 0;
		virtual ITouchDevice *GetTouch() const = 0;
		virtual IKeyboardDevice *GetKeys() const = 0;
		virtual IJoystickInput *GetJoysticks() const = 0;
		virtual I2dRenderer *Get2dRender() const = 0;
		virtual I2dEffect *Get2dEffect() const = 0;
	};
}
