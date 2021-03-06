#include "pch.h"
#include "COM/ComAlloc.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Module/ModuleFactory.h"

namespace ff
{
	class __declspec(uuid("f94a37e0-bec9-44f7-b5ff-7c9f37c1a6b6"))
		RenderDepth : public ComBase, public IRenderDepth
	{
	public:
		DECLARE_HEADER(RenderDepth);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		bool Init(PointInt size, DXGI_FORMAT format, size_t nMultiSamples);

		// IGraphDeviceChild
		virtual IGraphDevice *GetDevice() const override;
		virtual void Reset() override;

		// IRenderDepth
		virtual PointInt GetSize() const override;
		virtual bool SetSize(PointInt size) override;
		virtual void Clear(float *pDepth = nullptr, BYTE *pStencil = nullptr) override;
		virtual void ClearDepth(float *pDepth = nullptr) override;
		virtual void ClearStencil(BYTE *pStencil = nullptr) override;

		virtual ID3D11Texture2D *GetTexture() override;
		virtual ID3D11DepthStencilView *GetView() override;

	private:
		bool CreateTextureAndView(PointInt size, DXGI_FORMAT format, size_t nMultiSamples);

		ComPtr<IGraphDevice> _device;
		ComPtr<ID3D11Texture2D> _texture;
		ComPtr<ID3D11DepthStencilView> _vView;
		PointInt _size;
	};
}

BEGIN_INTERFACES(ff::RenderDepth)
	HAS_INTERFACE(ff::IRenderDepth)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"CRenderDepth");
	module.RegisterClassT<ff::RenderDepth>(name, GUID_NULL, ff::GetCategoryGraphicsObject());
});

bool ff::CreateRenderDepth(
	IGraphDevice*  pDevice,
	PointInt         size,
	DXGI_FORMAT    format,
	size_t         nMultiSamples,
	IRenderDepth** ppDepth)
{
	assertRetVal(ppDepth, false);
	*ppDepth = nullptr;

	ComPtr<RenderDepth> pDepth;
	assertRetVal(SUCCEEDED(ComAllocator<RenderDepth>::CreateInstance(pDevice, &pDepth)), false);
	assertRetVal(pDepth->Init(size, format, nMultiSamples), false);

	*ppDepth = pDepth.Detach();
	return true;
}

ff::RenderDepth::RenderDepth()
	: _size(0, 0)
{
}

ff::RenderDepth::~RenderDepth()
{
}

HRESULT ff::RenderDepth::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	
	return __super::_Construct(unkOuter);
}

bool ff::RenderDepth::Init(PointInt size, DXGI_FORMAT format, size_t nMultiSamples)
{
	assertRetVal(_device, false);
	assertRetVal(CreateTextureAndView(size, format, nMultiSamples), false);

	return true;
}

ff::IGraphDevice *ff::RenderDepth::GetDevice() const
{
	return _device;
}

void ff::RenderDepth::Reset()
{
	if (_texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);

		verify(CreateTextureAndView(PointInt(desc.Width, desc.Height), desc.Format, desc.SampleDesc.Count));
	}
}

ff::PointInt ff::RenderDepth::GetSize() const
{
	return _size;
}

bool ff::RenderDepth::SetSize(PointInt size)
{
	if (size != _size)
	{
		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);

		assertRetVal(CreateTextureAndView(size, desc.Format, desc.SampleDesc.Count), false);
	}

	return true;
}

void ff::RenderDepth::Clear(float *pDepth, BYTE *pStencil)
{
	assertRet(_vView);

	_device->GetContext()->ClearDepthStencilView(
		_vView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		pDepth ? *pDepth : 1.0f, pStencil ? *pStencil : 0);
}

void ff::RenderDepth::ClearDepth(float *pDepth)
{
	assertRet(_vView);

	_device->GetContext()->ClearDepthStencilView(
		_vView, D3D11_CLEAR_DEPTH,
		pDepth ? *pDepth : 1.0f, 0);
}

void ff::RenderDepth::ClearStencil(BYTE *pStencil)
{
	assertRet(_vView);

	_device->GetContext()->ClearDepthStencilView(
		_vView, D3D11_CLEAR_STENCIL,
		0, pStencil ? *pStencil : 0);
}

ID3D11Texture2D *ff::RenderDepth::GetTexture()
{
	return _texture;
}

ID3D11DepthStencilView *ff::RenderDepth::GetView()
{
	return _vView;
}

bool ff::RenderDepth::CreateTextureAndView(PointInt size, DXGI_FORMAT format, size_t nMultiSamples)
{
	_vView    = nullptr;
	_texture = nullptr;
	_size.SetPoint(0, 0);

	D3D11_TEXTURE2D_DESC descTex;
	ZeroObject(descTex);

	descTex.ArraySize          = 1;
	descTex.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
	descTex.CPUAccessFlags     = 0;
	descTex.Format             = (format == DXGI_FORMAT_UNKNOWN) ? DXGI_FORMAT_D24_UNORM_S8_UINT : format;
	descTex.Width              = size.x;
	descTex.Height             = size.y;
	descTex.MipLevels          = 1;
	descTex.MiscFlags          = 0;
	descTex.SampleDesc.Count   = nMultiSamples ? (UINT)nMultiSamples : 1;
	descTex.SampleDesc.Quality = 0;
	descTex.Usage              = D3D11_USAGE_DEFAULT;

	assertRetVal(SUCCEEDED(_device->GetDX()->CreateTexture2D(&descTex, nullptr, &_texture)), false);

	D3D11_DEPTH_STENCIL_VIEW_DESC desc;
	ZeroObject(desc);

	desc.Format        = format;
	desc.ViewDimension = (descTex.SampleDesc.Count > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;

	assertRetVal(SUCCEEDED(_device->GetDX()->CreateDepthStencilView(_texture, &desc, &_vView)), false);

	_size = size;

	return true;
}
