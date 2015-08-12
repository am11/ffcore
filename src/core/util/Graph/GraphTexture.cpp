#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Graph/2D/Sprite.h"
#include "Module/ModuleFactory.h"
#include "String/StringUtil.h"

#include <DirectXTex.h>

namespace ff
{
	class __declspec(uuid("94ba864d-9773-4ddd-8ede-bc39335e6852"))
		CGraphTexture : public ComBase, public IGraphTexture
	{
	public:
		DECLARE_HEADER(CGraphTexture);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		bool Init(ID3D11Texture2D *pTexture, size_t nMipMapStart, size_t nMipMapCount, size_t nArrayStart, size_t nArrayCount);

		// IGraphDeviceChild
		virtual IGraphDevice *GetDevice() const override;
		virtual void Reset() override;

		// IGraphTexture functions
		virtual PointInt GetSize() override;
		virtual ID3D11Texture2D *GetTexture() override;
		virtual ID3D11ShaderResourceView *GetShaderResource() override;
		virtual bool Convert(DXGI_FORMAT format, size_t nMipMapLevels, IGraphTexture **ppNewTexture) override;

	private:
		ComPtr<IGraphDevice> _device;
		ComPtr<ID3D11Texture2D> _texture;
		ComPtr<ID3D11ShaderResourceView> _view;
		WORD _viewArrayStart;
		WORD _viewArrayCount;
		WORD _viewMipStart;
		WORD _viewMipCount;
	};
}

BEGIN_INTERFACES(ff::CGraphTexture)
	HAS_INTERFACE(ff::IGraphTexture)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup RegisterTexture([](ff::Module &module)
{
	static ff::StaticString name(L"texture");
	module.RegisterClassT<ff::CGraphTexture>(name, __uuidof(ff::IGraphTexture), ff::GetCategoryGraphicsObject());
});

DXGI_FORMAT ParseTextureFormat(const ff::String &szFormat)
{
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	if (szFormat == L"rgba32")
	{
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else if (szFormat == L"bgra32")
	{
		format = DXGI_FORMAT_B8G8R8A8_UNORM;
	}
	else if (szFormat == L"bc1")
	{
		format = DXGI_FORMAT_BC1_UNORM;
	}
	else if (szFormat == L"bc2")
	{
		format = DXGI_FORMAT_BC2_UNORM;
	}
	else if (szFormat == L"bc3")
	{
		format = DXGI_FORMAT_BC3_UNORM;
	}

	return format;
}

bool ff::CreateGraphTexture(
		IGraphDevice *pDevice,
		StringRef path,
		DXGI_FORMAT format,
		size_t nMipMapLevels,
		IGraphTexture **ppTexture)
{
	assertRetVal(pDevice && ppTexture, false);

	DirectX::ScratchImage  scratchOrig;
	DirectX::ScratchImage  scratchMips;
	DirectX::ScratchImage  scratchNew;
	DirectX::ScratchImage* pScratchFinal = &scratchOrig;

	if (!_wcsicmp(GetPathExtension(path).c_str(), L"dds"))
	{
		assertRetVal(SUCCEEDED(DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, scratchOrig)), false);
	}
	else
	{
		assertRetVal(SUCCEEDED(DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, scratchOrig)), false);
	}

	if (DirectX::IsCompressed(format))
	{
		// Compressed images have size restrictions. Upon failure, just use RGB
		size_t width  = scratchOrig.GetMetadata().width;
		size_t height = scratchOrig.GetMetadata().height;

		if (width % 4 || height % 4)
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (nMipMapLevels != 1 && (NearestPowerOfTwo(width) != width || NearestPowerOfTwo(height) != height))
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	if (nMipMapLevels != 1)
	{
		assertRetVal(SUCCEEDED(DirectX::GenerateMipMaps(
			pScratchFinal->GetImages(),
			pScratchFinal->GetImageCount(),
			pScratchFinal->GetMetadata(),
			DirectX::TEX_FILTER_DEFAULT,
			nMipMapLevels,
			scratchMips)), false);

		pScratchFinal->Release();
		pScratchFinal = &scratchMips;
	}

	if (DirectX::IsCompressed(format))
	{
		assertRetVal(SUCCEEDED(DirectX::Compress(
			pScratchFinal->GetImages(),
			pScratchFinal->GetImageCount(),
			pScratchFinal->GetMetadata(),
			format,
			DirectX::TEX_COMPRESS_DEFAULT,
			0, // alpharef
			scratchNew)), false);

		pScratchFinal->Release();
		pScratchFinal = &scratchNew;
	}
	else if (format != pScratchFinal->GetMetadata().format)
	{
		assertRetVal(SUCCEEDED(DirectX::Convert(
			pScratchFinal->GetImages(),
			pScratchFinal->GetImageCount(),
			pScratchFinal->GetMetadata(),
			format,
			DirectX::TEX_FILTER_DEFAULT,
			0, // threshold
			scratchNew)), false);

		pScratchFinal->Release();
		pScratchFinal = &scratchNew;
	}

	ComPtr<ID3D11Resource> pResource;
	assertRetVal(SUCCEEDED(DirectX::CreateTexture(
		pDevice->GetDX(),
		pScratchFinal->GetImages(),
		pScratchFinal->GetImageCount(),
		pScratchFinal->GetMetadata(),
		&pResource)), false);

	pScratchFinal->Release();
	pScratchFinal = nullptr;

	ComPtr<ID3D11Texture2D> pTexture2D;
	assertRetVal(pTexture2D.QueryFrom(pResource), false);

	ComPtr<IGraphTexture> pTexture;
	assertRetVal(CreateGraphTexture(pDevice, pTexture2D, &pTexture), false);

	*ppTexture = pTexture.Detach();
	return true;
}

#if !METRO_APP
std::shared_ptr<DirectX::ScratchImage> ff::CreateScratchImage(HBITMAP bitmap)
{
	std::shared_ptr<DirectX::ScratchImage> badScratch;
	assertRetVal(bitmap, badScratch);

	std::shared_ptr<DirectX::ScratchImage> scratch = std::make_shared<DirectX::ScratchImage>();

	// Get the size of the bitmap
	BITMAP bitmapInfo;
	assertRetVal(::GetObject(bitmap, sizeof(bitmapInfo), &bitmapInfo) >= sizeof(bitmapInfo), badScratch);
	ff::PointInt size(bitmapInfo.bmWidth, bitmapInfo.bmHeight);

	assertHrRetVal(scratch->Initialize2D(DXGI_FORMAT_B8G8R8A8_UNORM, size.x, size.y, 1, 1), badScratch);
	const DirectX::Image &image = *scratch->GetImages();
	assertRetVal(image.rowPitch == size.x * 4, badScratch);

	HDC hdc = ::GetDC(nullptr);
	if (hdc)
	{
		BITMAPINFO bi;
		ZeroObject(bi);

		bi.bmiHeader.biSize = sizeof(bi);
		bi.bmiHeader.biWidth = size.x;
		bi.bmiHeader.biHeight = -size.y;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biCompression = BI_RGB;

		int copied = ::GetDIBits(hdc, bitmap, 0, -size.y, image.pixels, &bi, DIB_RGB_COLORS);
		::ReleaseDC(nullptr, hdc);
		hdc = nullptr;

		assertRetVal(copied == size.y, badScratch);
	}

	// Set alpha
	for (DWORD *cur = reinterpret_cast<DWORD *>(image.pixels),
		*end = reinterpret_cast<DWORD *>(image.pixels + image.height * image.rowPitch);
		cur != end; cur++)
	{
		*cur |= 0xFF000000;
	}

	return scratch;
}
#endif

#if !METRO_APP
bool ff::CreateGraphTexture(
	IGraphDevice *pDevice,
	HBITMAP bitmap,
	DXGI_FORMAT format,
	size_t nMipMapLevels,
	IGraphTexture **ppTexture)
{
	assertRetVal(pDevice && ppTexture, false);

	std::shared_ptr<DirectX::ScratchImage> scratch = ff::CreateScratchImage(bitmap);
	assertRetVal(scratch != nullptr, false);

	ff::ComPtr<ID3D11Resource> destResource;
	assertHrRetVal(DirectX::CreateTexture(pDevice->GetDX(), scratch->GetImages(), 1, scratch->GetMetadata(), &destResource), false);
	scratch.reset();

	ComPtr<ID3D11Texture2D> destTexture;
	assertRetVal(destTexture.QueryFrom(destResource), false);

	ComPtr<IGraphTexture> tempTexture;
	assertRetVal(CreateGraphTexture(pDevice, destTexture, 0, 0, 0, 0, &tempTexture), false);

	if ((format == DXGI_FORMAT_B8G8R8A8_UNORM || format == DXGI_FORMAT_B8G8R8X8_UNORM) &&
		nMipMapLevels == 1)
	{
		*ppTexture = tempTexture.Detach();
		return true;
	}
	else
	{
		return tempTexture->Convert(format, nMipMapLevels, ppTexture);
	}
}
#endif

bool ff::CreateGraphTexture(
	IGraphDevice *pDevice,
	PointInt size,
	DXGI_FORMAT format,
	size_t nMipMapLevels,
	size_t nArraySize,
	size_t nMultiSamples,
	IGraphTexture **ppTexture)
{
	assertRetVal(pDevice && ppTexture && size.x > 0 && size.y > 0, false);

	nMultiSamples = std::max<size_t>(1, nMultiSamples);
	nMultiSamples = std::min<size_t>(8, nMultiSamples);
	nArraySize    = std::max<size_t>(1, nArraySize);

	// compressed textures must have sizes that are multiples of four
	if (DirectX::IsCompressed(format) && (size.x % 4 || size.y % 4))
	{
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	D3D11_TEXTURE2D_DESC desc;
	ZeroObject(desc);

	desc.Width              = size.x;
	desc.Height             = size.y;
	desc.Usage              = D3D11_USAGE_DEFAULT;
	desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | (!DirectX::IsCompressed(format) ? D3D11_BIND_RENDER_TARGET : 0);
	desc.Format             = format;
	desc.CPUAccessFlags     = 0;
	desc.MiscFlags          = 0;
	desc.MipLevels          = (UINT)nMipMapLevels;
	desc.ArraySize          = (UINT)nArraySize;
	desc.SampleDesc.Count   = (UINT)nMultiSamples;
	desc.SampleDesc.Quality = 0;

	ComPtr<ID3D11Texture2D> pTexture2D;
	assertRetVal(SUCCEEDED(pDevice->GetDX()->CreateTexture2D(&desc, nullptr, &pTexture2D)), false);

	return CreateGraphTexture(pDevice, pTexture2D, ppTexture);
}

bool ff::CreateStagingTexture(
	IGraphDevice *pDevice,
	PointInt size,
	DXGI_FORMAT format,
	bool bReadable,
	bool bWritable,
	IGraphTexture **ppTexture)
{
	assertRetVal(pDevice && ppTexture && size.x > 0 && size.y > 0, false);
	assertRetVal(bReadable || bWritable, false);
	assertRetVal(!DirectX::IsCompressed(format), false);

	D3D11_TEXTURE2D_DESC desc;
	ZeroObject(desc);

	desc.Width              = size.x;
	desc.Height             = size.y;
	desc.Usage              = bReadable ? D3D11_USAGE_STAGING : D3D11_USAGE_DYNAMIC;
	desc.BindFlags          = bReadable ? 0 : D3D11_BIND_SHADER_RESOURCE;
	desc.Format             = format;
	desc.CPUAccessFlags     = (bWritable ? D3D11_CPU_ACCESS_WRITE : 0) | (bReadable ? D3D11_CPU_ACCESS_READ : 0);
	desc.MiscFlags          = 0;
	desc.MipLevels          = 1;
	desc.ArraySize          = 1;
	desc.SampleDesc.Count   = 1;
	desc.SampleDesc.Quality = 0;

	ComPtr<ID3D11Texture2D> pTexture2D;
	assertRetVal(SUCCEEDED(pDevice->GetDX()->CreateTexture2D(&desc, nullptr, &pTexture2D)), false);

	return CreateGraphTexture(pDevice, pTexture2D, ppTexture);
}

bool ff::CreateGraphTexture(
	IGraphDevice *pDevice,
	ID3D11Texture2D *pData,
	IGraphTexture ** ppTexture)
{
	return CreateGraphTexture(pDevice, pData, 0, 0, 0, 0, ppTexture);
}

bool ff::CreateGraphTexture(
	IGraphDevice *pDevice,
	ID3D11Texture2D *pData,
	size_t nMipMapStart,
	size_t nMipMapCount,
	size_t nArrayStart,
	size_t nArrayCount,
	IGraphTexture **ppTexture)
{
	assertRetVal(pDevice && pData && ppTexture, false);

	ComPtr<CGraphTexture, IGraphTexture> pTexture;
	assertRetVal(SUCCEEDED(ComAllocator<CGraphTexture>::CreateInstance(pDevice, &pTexture)), false);
	assertRetVal(pTexture->Init(pData, nMipMapStart, nMipMapCount, nArrayStart, nArrayCount), false);

	*ppTexture = pTexture.Detach();
	return true;
}

bool ff::CreateGraphTexture(
	ISprite *sprite,
	IGraphTexture **texture)
{
	assertRetVal(sprite && texture, false);

	const SpriteData &data = sprite->GetData();
	if (data.GetTextureRect().Size() == data._texture->GetSize())
	{
		// The sprite covers the whole texture, so just return it
		*texture = GetAddRef(data._texture.Interface());
		return true;
	}

	D3D11_TEXTURE2D_DESC desc;
	data._texture->GetTexture()->GetDesc(&desc);

	ComPtr<IGraphTexture> newTexture;
	assertRetVal(CreateGraphTexture(data._texture->GetDevice(), data._texture->GetSize(), desc.Format, 1, 1, 0, &newTexture), false);
	assertRetVal(sprite->CopyTo(newTexture, PointInt(0, 0)), false);

	*texture = newTexture.Detach();
	return true;
}

ff::CGraphTexture::CGraphTexture()
	: _viewArrayStart(0)
	, _viewArrayCount(0)
	, _viewMipStart(0)
	, _viewMipCount(0)
{
}

ff::CGraphTexture::~CGraphTexture()
{
}

HRESULT ff::CGraphTexture::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);

	return __super::_Construct(unkOuter);
}

bool ff::CGraphTexture::Init(
		ID3D11Texture2D* pTexture,
		size_t           nMipMapStart,
		size_t           nMipMapCount,
		size_t           nArrayStart,
		size_t           nArrayCount)
{
	assertRetVal(pTexture, false);

	D3D11_TEXTURE2D_DESC descTex;
	pTexture->GetDesc(&descTex);

	size_t nMipMax   = descTex.MipLevels;
	size_t nArrayMax = descTex.ArraySize;

	nMipMapCount = nMipMapCount ? nMipMapCount : nMipMax   - nMipMapStart;
	nArrayCount  = nArrayCount  ? nArrayCount  : nArrayMax - nArrayStart;

	assertRetVal(nMipMax <= 0xFFFF && nArrayMax <= 0xFFFF, false);
	assertRetVal(nMipMapStart < nMipMax && nArrayStart < nArrayMax, false);
	assertRetVal(nMipMapStart + nMipMapCount <= nMipMax && nArrayStart + nArrayCount <= nArrayMax, false);

	_texture        = pTexture;
	_viewMipStart   = (WORD)nMipMapStart;
	_viewMipCount   = (WORD)nMipMapCount;
	_viewArrayStart = (WORD)nArrayStart;
	_viewArrayCount = (WORD)nArrayCount;

	return true;
}

ff::IGraphDevice *ff::CGraphTexture::GetDevice() const
{
	return _device;
}

void ff::CGraphTexture::Reset()
{
	if (_texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);

		_texture = nullptr;
		_view    = nullptr;

		assertRet(SUCCEEDED(_device->GetDX()->CreateTexture2D(&desc, nullptr, &_texture)));
	}
}

ff::PointInt ff::CGraphTexture::GetSize()
{
	if (_texture)
	{
		D3D11_TEXTURE2D_DESC desc;
		_texture->GetDesc(&desc);

		return PointInt(desc.Width, desc.Height);
	}

	// Not ready yet
	return PointInt(0, 0);
}

ID3D11Texture2D *ff::CGraphTexture::GetTexture()
{
	return _texture;
}

static D3D_SRV_DIMENSION GetDefaultDimension(const D3D11_TEXTURE2D_DESC &desc)
{
	if (desc.ArraySize > 1)
	{
		if (desc.SampleDesc.Count > 1)
		{
			return D3D_SRV_DIMENSION_TEXTURE2DMSARRAY;
		}
		else
		{
			return D3D_SRV_DIMENSION_TEXTURE2DARRAY;
		}
	}
	else
	{
		if (desc.SampleDesc.Count > 1)
		{
			return D3D_SRV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			return D3D_SRV_DIMENSION_TEXTURE2D;
		}
	}
}

ID3D11ShaderResourceView *ff::CGraphTexture::GetShaderResource()
{
	if (!_view && _texture)
	{
		D3D11_TEXTURE2D_DESC descTex;
		_texture->GetDesc(&descTex);

		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroObject(desc);

		desc.Format        = descTex.Format;
		desc.ViewDimension = GetDefaultDimension(descTex);

		switch (desc.ViewDimension)
		{
		case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
			desc.Texture2DMSArray.FirstArraySlice = _viewArrayStart;
			desc.Texture2DMSArray.ArraySize       = _viewArrayCount;
			break;

		case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
			desc.Texture2DArray.FirstArraySlice = _viewArrayStart;
			desc.Texture2DArray.ArraySize       = _viewArrayCount;
			desc.Texture2DArray.MostDetailedMip = _viewMipStart;
			desc.Texture2DArray.MipLevels       = _viewMipCount;
			break;

		case D3D_SRV_DIMENSION_TEXTURE2DMS:
			// nothing to define
			break;

		case D3D_SRV_DIMENSION_TEXTURE2D:
			desc.Texture2D.MostDetailedMip = _viewMipStart;
			desc.Texture2D.MipLevels       = _viewMipCount;
			break;
		}

		assertRetVal(SUCCEEDED(_device->GetDX()->CreateShaderResourceView(_texture, &desc, &_view)), nullptr);
	}

	return _view;
}

bool ff::CGraphTexture::Convert(DXGI_FORMAT format, size_t nMipMapLevels, IGraphTexture **ppNewTexture)
{
	assertRetVal(ppNewTexture, false);
	assertRetVal(_texture, false);

	D3D11_TEXTURE2D_DESC desc;
	_texture->GetDesc(&desc);

	DirectX::ScratchImage  scratchOrig;
	DirectX::ScratchImage  scratchRGB;
	DirectX::ScratchImage  scratchMips;
	DirectX::ScratchImage  scratchNew;
	DirectX::ScratchImage* pScratchFinal = &scratchOrig;

	assertRetVal(SUCCEEDED(DirectX::CaptureTexture(
		_device->GetDX(),
		_device->GetContext(),
		_texture,
		scratchOrig)), false);

	if (DirectX::IsCompressed(format))
	{
		// Compressed images have size restrictions. Upon failure, just use RGB
		size_t width  = scratchOrig.GetMetadata().width;
		size_t height = scratchOrig.GetMetadata().height;

		if (width % 4 || height % 4)
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (nMipMapLevels != 1 && (NearestPowerOfTwo(width) != width || NearestPowerOfTwo(height) != height))
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	if (desc.Format != format || desc.MipLevels != nMipMapLevels)
	{
		if (DirectX::IsCompressed(desc.Format))
		{
			assertRetVal(SUCCEEDED(DirectX::Decompress(
				pScratchFinal->GetImages(),
				pScratchFinal->GetImageCount(),
				pScratchFinal->GetMetadata(),
				DXGI_FORMAT_R8G8B8A8_UNORM,
				scratchRGB)), false);

			pScratchFinal->Release();
			pScratchFinal = &scratchRGB;
		}
		else if (desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			assertRetVal(SUCCEEDED(DirectX::Convert(
				pScratchFinal->GetImages(),
				pScratchFinal->GetImageCount(),
				pScratchFinal->GetMetadata(),
				DXGI_FORMAT_R8G8B8A8_UNORM,
				DirectX::TEX_FILTER_DEFAULT,
				0, // threshold
				scratchRGB)), false);

			pScratchFinal->Release();
			pScratchFinal = &scratchRGB;
		}

		if (nMipMapLevels != 1)
		{
			assertRetVal(SUCCEEDED(DirectX::GenerateMipMaps(
				pScratchFinal->GetImages(),
				pScratchFinal->GetImageCount(),
				pScratchFinal->GetMetadata(),
				DirectX::TEX_FILTER_DEFAULT,
				nMipMapLevels,
				scratchMips)), false);

			pScratchFinal->Release();
			pScratchFinal = &scratchMips;
		}

		if (DirectX::IsCompressed(format))
		{
			assertRetVal(SUCCEEDED(DirectX::Compress(
				pScratchFinal->GetImages(),
				pScratchFinal->GetImageCount(),
				pScratchFinal->GetMetadata(),
				format,
				DirectX::TEX_COMPRESS_DEFAULT,
				0, // alpharef
				scratchNew)), false);

			pScratchFinal->Release();
			pScratchFinal = &scratchNew;
		}
		else if (format != pScratchFinal->GetMetadata().format)
		{
			assertRetVal(SUCCEEDED(DirectX::Convert(
				pScratchFinal->GetImages(),
				pScratchFinal->GetImageCount(),
				pScratchFinal->GetMetadata(),
				format,
				DirectX::TEX_FILTER_DEFAULT,
				0, // threshold
				scratchNew)), false);

			pScratchFinal->Release();
			pScratchFinal = &scratchNew;
		}
	}

	ComPtr<ID3D11Resource> pResource;
	assertRetVal(SUCCEEDED(DirectX::CreateTexture(
		_device->GetDX(),
		pScratchFinal->GetImages(),
		pScratchFinal->GetImageCount(),
		pScratchFinal->GetMetadata(),
		&pResource)), false);

	pScratchFinal->Release();
	pScratchFinal = nullptr;

	ComPtr<ID3D11Texture2D> pTexture2D;
	assertRetVal(pTexture2D.QueryFrom(pResource), false);

	return CreateGraphTexture(_device, pTexture2D, 0, 0, 0, 0, ppNewTexture);
}
