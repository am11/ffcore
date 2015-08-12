#include "pch.h"
#include "COM/ComAlloc.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Module/Module.h"
#include "Module/ModuleFactory.h"

#if METRO_APP

#include "Windows.UI.XAML.Media.DxInterop.h" // $(METRO_APP)

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;

namespace ff
{
	class __declspec(uuid("52e3f024-2b70-4e42-afe2-1070ab22f42d"))
		RenderTargetWindow : public ComBase, public IRenderTargetWindow
	{
	public:
		DECLARE_HEADER(RenderTargetWindow);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;

		bool Init(CoreWindow ^window, DXGI_FORMAT format, size_t nBackBuffers, size_t nMultiSamples);

		// IGraphDeviceChild
		virtual IGraphDevice *GetDevice() const override;
		virtual void Reset() override;

		// IRenderTarget
		virtual PointInt GetSize() const override;
		virtual RectInt GetViewport() const override;
		virtual void Clear(const XMFLOAT4 *pColor = nullptr) override;

		virtual void SetDepth(IRenderDepth *pDepth) override;
		virtual IRenderDepth *GetDepth() override;

		virtual ID3D11Texture2D *GetTexture() override;
		virtual ID3D11RenderTargetView *GetTarget() override;

		// IRenderTargetWindow
		virtual PWND GetWindow() const override;
		virtual PWND GetTopWindow() const override;
		virtual bool IsHidden() override;
		virtual bool SetSize(PointInt size) override;
		virtual void SetAutoSize() override;

		virtual void Present(bool bVsync) override;
		virtual void WaitForVsync() const override;

		virtual bool CanSetFullScreen() const override;
		virtual bool IsFullScreen() const override;
		virtual bool SetFullScreen(bool bFullScreen) override;

	private:
		ref class WindowEvents
		{
		internal:
			WindowEvents(RenderTargetWindow *pParent);

		public:
			virtual ~WindowEvents();

			void Destroy();

		private:
			void OnClosed(CoreWindow ^sender, CoreWindowEventArgs ^args);
			void OnSizeChanged(CoreWindow ^sender, WindowSizeChangedEventArgs ^args);

			RenderTargetWindow *_parent;
			Windows::Foundation::EventRegistrationToken _tokens[2];
		};

		// Window events
		void OnClosed(CoreWindow ^sender, CoreWindowEventArgs ^args);
		void OnSizeChanged(CoreWindow ^sender, WindowSizeChangedEventArgs ^args);

		// Construct/destruct stuff
		void Destroy();
		bool InitSwapChain(size_t nBackBuffers);
		bool ResizeSwapChain(PointInt size);

		ComPtr<IGraphDevice> _device;
		ComPtr<IDXGISwapChainX> _swapChain;
		ComPtr<ID3D11Texture2D> _backBuffer;
		ComPtr<ID3D11RenderTargetView> _target;
		ComPtr<IRenderDepth> _depth;
		PointInt _size;
		DXGI_FORMAT _format;
		size_t _multiSamples;

		Agile<CoreWindow> _window;
		Agile<Window> _windowXaml;
		SwapChainBackgroundPanel ^_panel;
		WindowEvents ^_events;
		bool _autoSize;
		bool _occluded;
	};
}

BEGIN_INTERFACES(ff::RenderTargetWindow)
	HAS_INTERFACE(ff::IRenderTarget)
	HAS_INTERFACE(ff::IRenderTargetWindow)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	ff::StaticString name(L"CRenderTargetWindow");
	module.RegisterClassT<ff::RenderTargetWindow>(name, GUID_NULL, ff::GetCategoryGraphicsObject());
});

ff::RenderTargetWindow::WindowEvents::WindowEvents(RenderTargetWindow *pParent)
	: _parent(pParent)
{
	_tokens[0] = _parent->_window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &WindowEvents::OnClosed);
	_tokens[1] = _parent->_window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &WindowEvents::OnSizeChanged);
}

ff::RenderTargetWindow::WindowEvents::~WindowEvents()
{
	Destroy();
}

void ff::RenderTargetWindow::WindowEvents::Destroy()
{
	_parent->_window->Closed -= _tokens[0];
	_parent->_window->SizeChanged -= _tokens[1];
}

void ff::RenderTargetWindow::WindowEvents::OnClosed(CoreWindow ^sender, CoreWindowEventArgs ^args)
{
	_parent->OnClosed(sender, args);
}

void ff::RenderTargetWindow::WindowEvents::OnSizeChanged(CoreWindow ^sender, WindowSizeChangedEventArgs ^args)
{
	_parent->OnSizeChanged(sender, args);
}

bool ff::CreateRenderTargetWindow(
	IGraphDevice *pDevice,
	PWND hwnd,
	bool bFullScreen, // ignored
	DXGI_FORMAT format,
	size_t nBackBuffers,
	size_t nMultiSamples,
	IRenderTargetWindow **ppRender)
{
	assertRetVal(ppRender, false);
	*ppRender = nullptr;

	ComPtr<RenderTargetWindow> pRender;
	assertRetVal(SUCCEEDED(ComAllocator<RenderTargetWindow>::CreateInstance(pDevice, &pRender)), false);
	assertRetVal(pRender->Init(hwnd, format, nBackBuffers, nMultiSamples), false);

	*ppRender = pRender.Detach();
	return true;
}

// STATIC_DATA (object)
static ff::Vector<ff::RenderTargetWindow*> s_allRenderWindows;

ff::RenderTargetWindow::RenderTargetWindow()
	: _size(0, 0)
	, _format(DXGI_FORMAT_UNKNOWN)
	, _multiSamples(0)
	, _autoSize(true)
	, _occluded(false)
{
	s_allRenderWindows.Push(this);
}

ff::RenderTargetWindow::~RenderTargetWindow()
{
	Destroy();

	s_allRenderWindows.Delete(s_allRenderWindows.Find(this));
}

HRESULT ff::RenderTargetWindow::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	
	return __super::_Construct(unkOuter);
}

static SwapChainBackgroundPanel ^GetSwapChainBackgroundPanel(Window ^window)
{
	SwapChainBackgroundPanel ^panel = nullptr;

	if (window != nullptr)
	{
		UserControl ^control = dynamic_cast<UserControl ^>(window->Content);
		if (control != nullptr)
		{
			panel = dynamic_cast<SwapChainBackgroundPanel ^>(control->Content);
		}
	}

	return panel;
}

bool ff::RenderTargetWindow::Init(
	CoreWindow^ window,
	DXGI_FORMAT format,
	size_t      nBackBuffers,
	size_t      nMultiSamples)
{
	assertRetVal(_device && window != nullptr, false);

	_window        = window;
	_windowXaml    = (Window::Current != nullptr && Window::Current->CoreWindow == window) ? Window::Current : nullptr;
	_panel         = GetSwapChainBackgroundPanel(_windowXaml.Get());
	_events        = ref new WindowEvents(this);
	_format        = format;
	_multiSamples = nMultiSamples;

	assertRetVal(InitSwapChain(nBackBuffers), false);

	return true;
}

void ff::RenderTargetWindow::Destroy()
{
	if (_events)
	{
		_events->Destroy();
		_events = nullptr;
	}
}

bool ff::RenderTargetWindow::InitSwapChain(size_t nBackBuffers)
{
	PointInt size = _size;
	if (size == PointInt(0, 0) && _panel != nullptr)
	{
		size = GetClientRect(_window.Get()).Size();
	}

	_format = (_format != DXGI_FORMAT_UNKNOWN) ? _format : DXGI_FORMAT_B8G8R8A8_UNORM;

	DXGI_SWAP_CHAIN_DESC1 desc;
	ZeroObject(desc);

	desc.Width            = size.x;
	desc.Height           = size.y;
	desc.Format           = _format;
	desc.SampleDesc.Count = (UINT)_multiSamples;
	desc.BufferCount      = 2; // must be 2 for sequential flip
	desc.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Scaling          = (_panel != nullptr) ? DXGI_SCALING_STRETCH : DXGI_SCALING_NONE;
	desc.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	ComPtr<IDXGIFactoryX> pCardDXGI;
	assertRetVal(GetParentDXGI(_device->GetAdapter(), __uuidof(IDXGIFactoryX), (void**)&pCardDXGI), false);

	if (_panel != nullptr)
	{
		assertRetVal(SUCCEEDED(pCardDXGI->CreateSwapChainForComposition(
			_device->GetDX(), &desc, nullptr, &_swapChain)), false);

		ComPtr<ISwapChainBackgroundPanelNative> pNativePanel;
		assertRetVal(pNativePanel.QueryFrom(reinterpret_cast<IUnknown*>(_panel)), false);
		assertRetVal(SUCCEEDED(pNativePanel->SetSwapChain(_swapChain)), false);
	}
	else
	{
		assertRetVal(SUCCEEDED(pCardDXGI->CreateSwapChainForCoreWindow(
			_device->GetDX(), (IUnknown*)_window.Get(), &desc, nullptr, &_swapChain)), false);
	}

	// Ensure that DXGI does not queue more than one frame at a time
	assertRetVal(SUCCEEDED(_device->GetDXGI()->SetMaximumFrameLatency(1)), false);
	
	return true;
}

bool ff::RenderTargetWindow::ResizeSwapChain(PointInt size)
{
	if (_swapChain && (_size != size || size == PointInt(0, 0)))
	{
		_size        = size;
		_target     = nullptr;
		_backBuffer = nullptr;

		_device->GetContext()->ClearState();

		if (size == PointInt(0, 0) && _panel != nullptr)
		{
			// swap chains require the size to be set
			size = GetClientRect(_window.Get()).Size();
		}

		assertRetVal(SUCCEEDED(_swapChain->ResizeBuffers(0, size.x, size.y, DXGI_FORMAT_UNKNOWN, 0)), false);
	}

	return true;
}

ff::IGraphDevice *ff::RenderTargetWindow::GetDevice() const
{
	return _device;
}

void ff::RenderTargetWindow::Reset()
{
	assertRet(_swapChain);

	DXGI_SWAP_CHAIN_DESC desc;
	ZeroObject(desc);
	_swapChain->GetDesc(&desc);

	_target     = nullptr;
	_swapChain  = nullptr;
	_backBuffer = nullptr;
	_occluded   = false;

	verify(InitSwapChain(desc.BufferCount));
}

ff::PointInt ff::RenderTargetWindow::GetSize() const
{
	PointInt size(0, 0);

	DXGI_SWAP_CHAIN_DESC1 desc;
	if (_swapChain && SUCCEEDED(_swapChain->GetDesc1(&desc)))
	{
		size.SetPoint(desc.Width, desc.Height);
	}

	return size;
}

ff::RectInt ff::RenderTargetWindow::GetViewport() const
{
	return RectInt(PointInt(0, 0), GetSize());
}

void ff::RenderTargetWindow::Clear(const XMFLOAT4 *pColor)
{
	if (GetTarget())
	{
		static const XMFLOAT4 defaultColor(0, 0, 0, 1);
		const XMFLOAT4 *pUseColor = pColor ? pColor : &defaultColor;

		_device->GetContext()->ClearRenderTargetView(GetTarget(), &pUseColor->x);
	}
}

void ff::RenderTargetWindow::SetDepth(IRenderDepth *pDepth)
{
	_depth = pDepth;
}

ff::IRenderDepth *ff::RenderTargetWindow::GetDepth()
{
	return _depth;
}

ID3D11Texture2D *ff::RenderTargetWindow::GetTexture()
{
	assertRetVal(_swapChain, nullptr);

	_backBuffer = nullptr;
	assertRetVal(SUCCEEDED(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&_backBuffer)), nullptr);

	return _backBuffer;
}

// from RenderTargetTexture.cpp
bool CreateRenderTarget(
	ff::IGraphDevice *pDevice,
	ID3D11Texture2D *pTexture,
	size_t nArrayStart,
	size_t nArrayCount,
	size_t nMipLevel,
	ID3D11RenderTargetView **ppView);

ID3D11RenderTargetView *ff::RenderTargetWindow::GetTarget()
{
	if (_occluded || _window == nullptr)
	{
		return nullptr;
	}

	if (!_target && GetTexture())
	{
		assertRetVal(CreateRenderTarget(_device, GetTexture(), 0, 1, 0, &_target), false);
	}

	return _target;
}

ff::PWND ff::RenderTargetWindow::GetWindow() const
{
	return _window.Get();
}

ff::PWND ff::RenderTargetWindow::GetTopWindow() const
{
	return GetWindow();
}

bool ff::RenderTargetWindow::IsHidden()
{
	return !_window->Visible;
}

bool ff::RenderTargetWindow::SetSize(PointInt size)
{
	_autoSize = (size == PointInt(0, 0));

	assertRetVal(ResizeSwapChain(size), false);

	return true;
}

void ff::RenderTargetWindow::SetAutoSize()
{
	_autoSize = true;

	OnSizeChanged(_window.Get(), nullptr);
}

void ff::RenderTargetWindow::Present(bool bVsync)
{
	assertRet(_swapChain);

	UINT nSync  = (bVsync && !_occluded && _panel == nullptr) ? 1 : 0;
	UINT nFlags = _occluded ? DXGI_PRESENT_TEST : 0;
	DXGI_PRESENT_PARAMETERS pp = { 0 };

	HRESULT hr = _swapChain->Present1(nSync, nFlags, &pp);

	if (_target)
	{
		_device->GetContext()->DiscardView(_target);
	}

	if (_depth && _depth->GetView())
	{
		_device->GetContext()->DiscardView(_depth->GetView());
	}

	switch (hr)
	{
	case S_OK:
		_occluded = false;
		break;

	case DXGI_STATUS_OCCLUDED:
		_occluded = true;
		break;

	case DXGI_ERROR_DEVICE_RESET:
	case DXGI_ERROR_DEVICE_REMOVED:
		_device->Reset();
		break;

	default:
		assert(false);
		break;
	}
}

void ff::RenderTargetWindow::WaitForVsync() const
{
	assertRet(_swapChain);

	ComPtr<IDXGIOutput> pOutput;
	if (SUCCEEDED(_swapChain->GetContainingOutput(&pOutput)))
	{
		pOutput->WaitForVBlank();
	}
}

bool ff::RenderTargetWindow::CanSetFullScreen() const
{
	return false;
}

bool ff::RenderTargetWindow::IsFullScreen() const
{
	return true;
}

bool ff::RenderTargetWindow::SetFullScreen(bool bFullScreen)
{
	assert(bFullScreen);
	return bFullScreen;
}

void ff::RenderTargetWindow::OnClosed(CoreWindow ^sender, CoreWindowEventArgs ^args)
{
	Destroy();
}

void ff::RenderTargetWindow::OnSizeChanged(CoreWindow ^sender, WindowSizeChangedEventArgs ^args)
{
	if (_autoSize)
	{
		ResizeSwapChain(PointInt(0, 0));
	}
}

#endif // METRO_APP
