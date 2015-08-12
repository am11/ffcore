#include "pch.h"
#include "COM/ComAlloc.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Module/ModuleFactory.h"

namespace ff
{
	class __declspec(uuid("42a1ad55-ca79-4c93-99c2-b8d4b1961de2"))
		RenderTargetTexture : public ComBase, public IRenderTarget
	{
	public:
		DECLARE_HEADER(RenderTargetTexture);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		bool Init(IGraphTexture *pTexture, size_t nArrayStart, size_t nArrayCount, size_t nMipLevel);

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

	private:
		ComPtr<IGraphDevice> _device;
		ComPtr<IGraphTexture> _texture;
		ComPtr<ID3D11RenderTargetView> _target;
		ComPtr<IRenderDepth> _depth;
		size_t _arrayStart;
		size_t _arrayCount;
		size_t _mipLevel;
	};
}

BEGIN_INTERFACES(ff::RenderTargetTexture)
	HAS_INTERFACE(ff::IRenderTarget)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"CRenderTargetTexture");
	module.RegisterClassT<ff::RenderTargetTexture>(name, GUID_NULL, ff::GetCategoryGraphicsObject());
});

bool ff::CreateRenderTargetTexture(
	IGraphDevice*   pDevice,
	IGraphTexture*  pTexture,
	size_t          nArrayStart,
	size_t          nArrayCount,
	size_t          nMipLevel,
	IRenderTarget** ppRender)
{
	assertRetVal(ppRender, false);
	*ppRender = nullptr;

	ComPtr<RenderTargetTexture> pRender;
	assertRetVal(SUCCEEDED(ComAllocator<RenderTargetTexture>::CreateInstance(pDevice, &pRender)), false);
	assertRetVal(pRender->Init(pTexture, nArrayStart, nArrayCount, nMipLevel), false);

	*ppRender = pRender.Detach();
	return true;
}

// helper function
static D3D11_RTV_DIMENSION GetViewDimension(const D3D11_TEXTURE2D_DESC &desc)
{
	if (desc.ArraySize > 1)
	{
		if (desc.SampleDesc.Count > 1)
		{
			return D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
		}
		else
		{
			return D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		}
	}
	else
	{
		if (desc.SampleDesc.Count > 1)
		{
			return D3D11_RTV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			return D3D11_RTV_DIMENSION_TEXTURE2D;
		}
	}
}

// helper function
bool CreateRenderTarget(
	ff::IGraphDevice *pDevice,
	ID3D11Texture2D *pTexture,
	size_t nArrayStart,
	size_t nArrayCount,
	size_t nMipLevel,
	ID3D11RenderTargetView **ppView)
{
	assertRetVal(pDevice && pTexture && ppView, false);
	*ppView = nullptr;

	D3D11_TEXTURE2D_DESC descTex;
	pTexture->GetDesc(&descTex);

	D3D11_RENDER_TARGET_VIEW_DESC desc;
	ff::ZeroObject(desc);

	nArrayCount = nArrayCount ? nArrayCount : descTex.ArraySize - nArrayStart;

	desc.Format = descTex.Format;
	desc.ViewDimension = GetViewDimension(descTex);

	switch (desc.ViewDimension)
	{
	case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
		desc.Texture2DMSArray.FirstArraySlice = (UINT)nArrayStart;
		desc.Texture2DMSArray.ArraySize = (UINT)nArrayCount;
		break;

	case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
		desc.Texture2DArray.FirstArraySlice = (UINT)nArrayStart;
		desc.Texture2DArray.ArraySize = (UINT)nArrayCount;
		desc.Texture2DArray.MipSlice = (UINT)nMipLevel;
		break;

	case D3D_SRV_DIMENSION_TEXTURE2DMS:
		// nothing to define
		break;

	case D3D_SRV_DIMENSION_TEXTURE2D:
		desc.Texture2D.MipSlice = (UINT)nMipLevel;
		break;
	}

	assertRetVal(SUCCEEDED(pDevice->GetDX()->CreateRenderTargetView(pTexture, &desc, ppView)), false);

	return true;
}

ff::RenderTargetTexture::RenderTargetTexture()
	: _arrayStart(0)
	, _mipLevel(0)
{
}

ff::RenderTargetTexture::~RenderTargetTexture()
{
}

HRESULT ff::RenderTargetTexture::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	
	return __super::_Construct(unkOuter);
}

bool ff::RenderTargetTexture::Init(IGraphTexture *pTexture, size_t nArrayStart, size_t nArrayCount, size_t nMipLevel)
{
	assertRetVal(_device && pTexture, false);

	_texture    = pTexture;
	_arrayStart = nArrayStart;
	_arrayCount = nArrayCount;
	_mipLevel   = nMipLevel;

	return true;
}

ff::IGraphDevice *ff::RenderTargetTexture::GetDevice() const
{
	return _device;
}

void ff::RenderTargetTexture::Reset()
{
	_target = nullptr;
}

ff::PointInt ff::RenderTargetTexture::GetSize() const
{
	return _texture->GetSize();
}

ff::RectInt ff::RenderTargetTexture::GetViewport() const
{
	PointInt size = GetSize();
	return RectInt(0, 0, size.x, size.y);
}

void ff::RenderTargetTexture::Clear(const XMFLOAT4 *pColor)
{
	if (GetTarget())
	{
		static const XMFLOAT4 defaultColor(0, 0, 0, 1);
		const XMFLOAT4 *pUseColor = pColor ? pColor : &defaultColor;

		_device->GetContext()->ClearRenderTargetView(GetTarget(), &pUseColor->x);
	}
}

void ff::RenderTargetTexture::SetDepth(IRenderDepth *pDepth)
{
	_depth = pDepth;
}

ff::IRenderDepth *ff::RenderTargetTexture::GetDepth()
{
	return _depth;
}

ID3D11Texture2D *ff::RenderTargetTexture::GetTexture()
{
	return _texture ? _texture->GetTexture() : nullptr;
}

ID3D11RenderTargetView *ff::RenderTargetTexture::GetTarget()
{
	if (!_target && GetTexture())
	{
		assertRetVal(CreateRenderTarget(_device, GetTexture(), _arrayStart, _arrayCount, _mipLevel, &_target), false);
	}

	return _target;
}
