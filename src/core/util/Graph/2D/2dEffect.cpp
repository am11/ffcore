#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataWriterReader.h"
#include "Graph/2D/2dEffect.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphShader.h"
#include "Graph/GraphState.h"
#include "Graph/GraphStateCache.h"
#include "Graph/GraphTexture.h"
#include "Graph/VertexFormat.h"
#include "Module/Module.h"
#include "Module/ModuleFactory.h"
#include "Resource/util-resource.h"

namespace ff
{
	struct FrameConstantsCore
	{
		XMFLOAT4X4 world;
		XMFLOAT4X4 proj;
		float      zoffset;
		float      padding[3];
	};

	struct FrameConstants : FrameConstantsCore
	{
		FrameConstants()
		{
			ZeroObject(*this);
			XMStoreFloat4x4(&world, XMMatrixIdentity());
			XMStoreFloat4x4(&proj, XMMatrixIdentity());
		}

		bool changed;
	};

	class __declspec(uuid("10005dea-f4cf-4cae-801d-5b058b6d4c3a"))
		Default2dEffect : public ComBase, public I2dEffect
	{
	public:
		DECLARE_HEADER(Default2dEffect);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		bool Init();

		// IGraphDeviceChild
		virtual IGraphDevice *GetDevice() const override;
		virtual void Reset() override;

		// I2dEffect
		virtual bool IsValid() const override;
		virtual bool OnBeginRender(I2dRenderer *pRender) override;
		virtual void OnEndRender(I2dRenderer *pRender) override;
		virtual void OnMatrixChanging(I2dRenderer *pRender, MatrixType type) override;
		virtual void OnMatrixChanged(I2dRenderer *pRender, MatrixType type) override;
		virtual void ApplyTextures(I2dRenderer *pRender, IGraphTexture **ppTextures, size_t nTextures) override;
		virtual bool Apply(I2dRenderer *pRender, DrawType2d type, ID3D11Buffer *pVertexes, ID3D11Buffer *pIndexes, float zOffset) override;
		virtual void PushDrawType(DrawType2d typeFlags) override;
		virtual void PopDrawType() override;

	private:
		void Destroy();

		const GraphState *GetInfo(DrawType2d type);

		bool _level9;
		ComPtr<IGraphDevice> _device;
		ComPtr<IGraphTexture> _emptyTexture;
		Map<DrawType2d, GraphState> _typeInfo;
		Vector<DrawType2d> _typeFlags;

		ComPtr<ID3D11VertexShader> _vsLine;
		ComPtr<ID3D11VertexShader> _vsSprite;
		ComPtr<ID3D11VertexShader> _vsMultiSprite;

		ComPtr<ID3D11PixelShader> _psLine;
		ComPtr<ID3D11PixelShader> _psSprite;
		ComPtr<ID3D11PixelShader> _psSpriteAlpha;
		ComPtr<ID3D11PixelShader> _psMultiSprite;

		ComPtr<ID3D11InputLayout> _layoutLine;
		ComPtr<ID3D11InputLayout> _layoutSprite;
		ComPtr<ID3D11InputLayout> _layoutMultiSprite;

		FrameConstants _frameConstants;
		ComPtr<ID3D11Buffer> _frameConstantsBuffer;
	};
}

BEGIN_INTERFACES(ff::Default2dEffect)
	HAS_INTERFACE(ff::I2dEffect)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"CDefault2dEffect");
	module.RegisterClassT<ff::Default2dEffect>(name, GUID_NULL, ff::GetCategoryGraphicsObject());
});

bool ff::CreateDefault2dEffect(IGraphDevice *pDevice, I2dEffect **ppEffect)
{
	assertRetVal(pDevice && ppEffect, false);
	*ppEffect = nullptr;

	ComPtr<Default2dEffect> pEffect;
	assertRetVal(SUCCEEDED(ComAllocator<Default2dEffect>::CreateInstance(pDevice, &pEffect)), false);
	assertRetVal(pEffect->Init(), false);

	*ppEffect = pEffect.Detach();
	return true;
}

ff::Default2dEffect::Default2dEffect()
	: _level9(false)
{
}

ff::Default2dEffect::~Default2dEffect()
{
	Destroy();
}

HRESULT ff::Default2dEffect::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	
	return __super::_Construct(unkOuter);
}

bool ff::Default2dEffect::Init()
{
	assertRetVal(_device && _device->GetDX() && _device->GetContext(), false);

	_level9 = _device->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0;

	const wchar_t *szSuffix = _level9 ? L"_9_1" : L"_4_0";

	// TODO:
#if 0
	ComPtr<IDataReader> pShaderReader;
	assertRetVal(GetThisModule().GetAsset(ID_SHADER_PACKAGE, &pShaderReader), false);

	ComPtr<IPackage> pShaderPackage;
	assertRetVal(LoadPackage(pShaderReader, &pShaderPackage), false);

	PackageNode parentNode = pShaderPackage->Find(L"Shader", GUID_NULL, GUID_NULL, nullptr, false);
	assertRetVal(parentNode, false);

	assertRetVal(LoadGraphShaders(m_pDevice, parentNode,
		(String(L"LineArtVS") + szSuffix).c_str(), &m_pVsLine,
		(String(L"LineArtPS") + szSuffix).c_str(), &m_pPsLine,
		VERTEX_TYPE_LINE_ART, &m_pLayoutLine), false);

	assertRetVal(LoadGraphShaders(m_pDevice, parentNode,
		(String(L"SpriteVS") + szSuffix).c_str(), &m_pVsSprite,
		(String(L"SpritePS") + szSuffix).c_str(), &m_pPsSprite,
		VERTEX_TYPE_SPRITE, &m_pLayoutSprite), false);

	assertRetVal(LoadGraphShaders(m_pDevice, parentNode,
		(String(L"MultiSpriteVS") + szSuffix).c_str(), &m_pVsMultiSprite,
		(String(L"MultiSpritePS") + szSuffix).c_str(), &m_pPsMultiSprite,
		VERTEX_TYPE_MULTI_SPRITE, &m_pLayoutMultiSprite), false);

	assertRetVal(LoadGraphShaders(m_pDevice, parentNode,
		0, nullptr,
		(String(L"SpriteAlphaPS") + szSuffix).c_str(), &m_pPsSpriteAlpha,
		VERTEX_TYPE_UNKNOWN, nullptr), false);
#endif

	D3D11_SUBRESOURCE_DATA frameConstantsData;
	frameConstantsData.pSysMem = &_frameConstants;
	frameConstantsData.SysMemPitch = sizeof(FrameConstantsCore);
	frameConstantsData.SysMemSlicePitch = 0;

	CD3D11_BUFFER_DESC frameConstantsDesc(sizeof(FrameConstantsCore), D3D11_BIND_CONSTANT_BUFFER);
	assertRetVal(SUCCEEDED(_device->GetDX()->CreateBuffer(&frameConstantsDesc, &frameConstantsData, &_frameConstantsBuffer)), false);

	return true;
}

void ff::Default2dEffect::Destroy()
{
	_frameConstants = FrameConstants();
	_typeInfo.Clear();

	_emptyTexture         = nullptr;
	_vsLine               = nullptr;
	_vsSprite             = nullptr;
	_vsMultiSprite        = nullptr;
	_psLine               = nullptr;
	_psSprite             = nullptr;
	_psMultiSprite        = nullptr;
	_psSpriteAlpha        = nullptr;
	_layoutLine           = nullptr;
	_layoutSprite         = nullptr;
	_layoutMultiSprite    = nullptr;
	_frameConstantsBuffer = nullptr;
}

ff::IGraphDevice *ff::Default2dEffect::GetDevice() const
{
	return _device;
}

void ff::Default2dEffect::Reset()
{
	Destroy();
	verify(Init());
}

bool ff::Default2dEffect::IsValid() const
{
	return _vsLine != nullptr;
}

bool ff::Default2dEffect::OnBeginRender(I2dRenderer *pRender)
{
	assertRetVal(IsValid() && pRender, false);

	OnMatrixChanged(pRender, MATRIX_WORLD);
	OnMatrixChanged(pRender, MATRIX_PROJECTION);

	return true;
}

void ff::Default2dEffect::OnEndRender(I2dRenderer *pRender)
{
	// Gets rid of a harmless(?) warning when a render target was used as pixel shader input
	ID3D11ShaderResourceView *pRes = nullptr;
	pRender->GetDevice()->GetContext()->PSSetShaderResources(0, 1, &pRes);
}

void ff::Default2dEffect::OnMatrixChanging(I2dRenderer *pRender, MatrixType type)
{
}

void ff::Default2dEffect::OnMatrixChanged(I2dRenderer *pRender, MatrixType type)
{
	switch (type)
	{
	case MATRIX_PROJECTION:
		{
			XMFLOAT4X4 oldMatrix = _frameConstants.proj;
			XMStoreFloat4x4(&_frameConstants.proj, XMMatrixTranspose(pRender->GetMatrix(MATRIX_PROJECTION)));

			if (CompareObjects(oldMatrix, _frameConstants.proj) != 0)
			{
				_frameConstants.changed = true;
			}
		}
		break;

	case MATRIX_WORLD:
		{
			XMFLOAT4X4 oldMatrix = _frameConstants.world;
			XMStoreFloat4x4(&_frameConstants.world, XMMatrixTranspose(pRender->GetMatrix(MATRIX_WORLD)));

			if (CompareObjects(oldMatrix, _frameConstants.world) != 0)
			{
				_frameConstants.changed = true;
			}
		}
		break;
	}
}

void ff::Default2dEffect::ApplyTextures(I2dRenderer *pRender, IGraphTexture **ppTextures, size_t nTextures)
{
	if (!_emptyTexture)
	{
		// Use this texture in place of textures that aren't loaded yet
		CreateGraphTexture(_device, PointInt(1, 1), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 0, &_emptyTexture);
	}

	ID3D11ShaderResourceView* resources[8] = { 0 };

	for (size_t i = 0; i < nTextures; i++)
	{
		resources[i] = ppTextures[i] ? ppTextures[i]->GetShaderResource() : nullptr;

		if (!resources[i] && _emptyTexture)
		{
			resources[i] = _emptyTexture->GetShaderResource();
		}
	}

#ifdef _DEBUG
	for (size_t i = nTextures; i < _countof(resources); i++)
	{
		// Avoid harmless debug warnings from Direct3D by selecting an empty
		// texture into the unused slots.

		if (!resources[i] && _emptyTexture)
		{
			resources[i] = _emptyTexture->GetShaderResource();
		}
	}
#endif

	UINT nResources = _level9 ? 1 : _countof(resources);
	pRender->GetDevice()->GetContext()->PSSetShaderResources(0, nResources, resources);
}

bool ff::Default2dEffect::Apply(
	I2dRenderer*  pRender,
	DrawType2d    type,
	ID3D11Buffer* pVertexes,
	ID3D11Buffer* pIndexes,
	float         zOffset)
{
	if (_typeFlags.Size())
	{
		type = (DrawType2d)((type & DRAW_TYPE_BITS) | _typeFlags.GetLast());
	}

	ID3D11DeviceContextX *context = _device->GetContext();

	// Update constant buffers

	if (zOffset != _frameConstants.zoffset)
	{
		_frameConstants.zoffset = zOffset;
		_frameConstants.changed = true;
	}

	if (_frameConstants.changed)
	{
		_frameConstants.changed = false;
		context->UpdateSubresource(_frameConstantsBuffer, 0, nullptr, &_frameConstants, 0, 0);
	}

	const GraphState *state = GetInfo(type);
	assertRetVal(state && state->Apply(_device, pVertexes, pIndexes), false);

	return true;
}

const ff::GraphState *ff::Default2dEffect::GetInfo(DrawType2d type)
{
	BucketIter iter = _typeInfo.Get(type);

	if (iter == INVALID_ITER)
	{
		GraphState info;
		info._vertexConstants.Push(_frameConstantsBuffer);

		switch (type & DRAW_TYPE_BITS)
		{
		case DRAW_TYPE_SPRITE:
			info._vertexType = VERTEX_TYPE_SPRITE;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			info._layout = _layoutSprite;
			info._vertex = _vsSprite;
			info._pixel = (type & DRAW_TEXTURE_ALPHA) ? _psSpriteAlpha : _psSprite;
			break;

		case DRAW_TYPE_MULTI_SPRITE:
			info._vertexType = VERTEX_TYPE_MULTI_SPRITE;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			info._layout = _layoutMultiSprite;
			info._vertex = _vsMultiSprite;
			info._pixel = _psMultiSprite;
			break;

		case DRAW_TYPE_RECTS:
			info._vertexType = VERTEX_TYPE_LINE_ART;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			info._layout = _layoutLine;
			info._vertex = _vsLine;
			info._pixel = _psLine;
			break;

		case DRAW_TYPE_LINES:
			info._vertexType = VERTEX_TYPE_LINE_ART;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			info._layout = _layoutLine;
			info._vertex = _vsLine;
			info._pixel = _psLine;
			break;

		case DRAW_TYPE_POINTS:
			info._vertexType = VERTEX_TYPE_LINE_ART;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			info._layout = _layoutLine;
			info._vertex = _vsLine;
			info._pixel = _psLine;
			break;

		default:
			assertRetVal(false, nullptr);
		}

		D3D11_BLEND_DESC blendDesc;
		D3D11_DEPTH_STENCIL_DESC depthDesc;
		D3D11_RASTERIZER_DESC rasterDesc;
		D3D11_SAMPLER_DESC samplerDesc;

		_device->GetStateCache().GetDefault(blendDesc);
		_device->GetStateCache().GetDefault(depthDesc);
		_device->GetStateCache().GetDefault(rasterDesc);
		_device->GetStateCache().GetDefault(samplerDesc);

		// Blend
		{
			info._sampleMask = 0xFFFFFFFF;

			// newColor = (srcColor * SrcBlend) BlendOp (destColor * DestBlend)
			// newAlpha = (srcAlpha * SrcBlendAlpha) BlendOpAlpha (destAlpha * DestBlendAlpha)

			switch (type & DRAW_BLEND_BITS)
			{
			case 0:
			case DRAW_BLEND_ALPHA:
				blendDesc.RenderTarget[0].BlendEnable    = TRUE;
				blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
				break;

			case DRAW_BLEND_ALPHA_MAX:
				blendDesc.RenderTarget[0].BlendEnable    = TRUE;
				blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_MAX;
				break;

			case DRAW_BLEND_ADD:
				blendDesc.RenderTarget[0].BlendEnable    = TRUE;
				blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
				break;

			case DRAW_BLEND_SUB:
				blendDesc.RenderTarget[0].BlendEnable    = TRUE;
				blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_REV_SUBTRACT;
				blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_REV_SUBTRACT;
				break;

			case DRAW_BLEND_MULTIPLY:
				blendDesc.RenderTarget[0].BlendEnable    = TRUE;
				blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_SRC_COLOR;
				blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
				break;

			case DRAW_BLEND_INV_MUL:
				blendDesc.RenderTarget[0].BlendEnable    = TRUE;
				blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_COLOR;
				blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
				break;

			case DRAW_BLEND_COPY_ALPHA:
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;
				break;

			case DRAW_BLEND_COPY_ALL:
				// default is good
				break;

			case DRAW_BLEND_COPY_PMA:
				blendDesc.RenderTarget[0].BlendEnable    = TRUE;
				blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
				break;

			default:
				assertRetVal(false, false);
			}
		}

		// Depth/Stencil
		{
			info._stencil = 1;

			switch (type & DRAW_DEPTH_BITS)
			{
			case 0:
			case DRAW_DEPTH_ENABLE:
				depthDesc.DepthEnable    = TRUE;
				depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
				break;

			case DRAW_DEPTH_DISABLE:
				depthDesc.DepthEnable    = FALSE;
				depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				break;

			default:
				assertRetVal(false, false);
			}

			switch (type & DRAW_STENCIL_BITS)
			{
			case 0:
			case DRAW_STENCIL_NONE:
				break;

			case DRAW_STENCIL_WRITE:
				depthDesc.StencilEnable                = TRUE;
				depthDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_REPLACE;
				depthDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
				depthDesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_REPLACE;
				depthDesc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;
				break;

			case DRAW_STENCIL_IS_SET:
				depthDesc.StencilEnable                = TRUE;
				depthDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_EQUAL;
				depthDesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilFunc         = D3D11_COMPARISON_EQUAL;
				break;

			case DRAW_STENCIL_NOT_SET:
				depthDesc.StencilEnable                = TRUE;
				depthDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_NOT_EQUAL;
				depthDesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilFunc         = D3D11_COMPARISON_NOT_EQUAL;
				break;

			default:
				assertRetVal(false, nullptr);
			}
		}

		// Raster
		{
			rasterDesc.CullMode              = D3D11_CULL_NONE;
			rasterDesc.MultisampleEnable     = FALSE;
			rasterDesc.AntialiasedLineEnable = FALSE;
		}

		// Sampler
		{
			if ((type & DRAW_SAMPLE_MIN_POINT) && (type & DRAW_SAMPLE_MAG_LINEAR))
			{
				samplerDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			}
			else if (type & DRAW_SAMPLE_MAG_LINEAR)
			{
				samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			}
			else if (type & DRAW_SAMPLE_MIN_POINT)
			{
				samplerDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			}
			else
			{
				samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			}

			if (type & DRAW_SAMPLE_WRAP)
			{
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			}
		}

		info._blend = _device->GetStateCache().GetBlendState(blendDesc);
		info._depth = _device->GetStateCache().GetDepthStencilState(depthDesc);
		info._raster = _device->GetStateCache().GetRasterizerState(rasterDesc);
		info._samplers.Push(_device->GetStateCache().GetSamplerState(samplerDesc));

		assertRetVal(
			info._blend &&
			info._depth &&
			info._raster &&
			info._samplers[0], nullptr);

		iter = _typeInfo.Insert(type, info);
	}

	return &_typeInfo.ValueAt(iter);
}

void ff::Default2dEffect::PushDrawType(DrawType2d typeFlags)
{
	assert(!(typeFlags & DRAW_TYPE_BITS));

	DWORD newType  = typeFlags & ~DRAW_TYPE_BITS;
	DWORD prevType = _typeFlags.Size() ? _typeFlags.GetLast() : 0;

	if (typeFlags & DRAW_STENCIL_BITS)
	{
		prevType &= ~DRAW_STENCIL_BITS;
	}

	if (typeFlags & DRAW_SAMPLE_BITS)
	{
		prevType &= ~DRAW_SAMPLE_BITS;
	}

	if (typeFlags & DRAW_DEPTH_BITS)
	{
		prevType &= ~DRAW_DEPTH_BITS;
	}

	if (typeFlags & DRAW_BLEND_BITS)
	{
		prevType &= ~DRAW_BLEND_BITS;
	}

	if (typeFlags & DRAW_TEXTURE_BITS)
	{
		prevType &= ~DRAW_TEXTURE_BITS;
	}

	newType |= prevType;

	_typeFlags.Push((DrawType2d)newType);
}

void ff::Default2dEffect::PopDrawType()
{
	assertRet(_typeFlags.Size());
	_typeFlags.Pop();
}
