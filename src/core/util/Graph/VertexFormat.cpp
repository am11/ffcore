#include "pch.h"
#include "Graph/GraphDevice.h"
#include "Graph/VertexFormat.h"

// Must match the VertexPN structure
static D3D11_INPUT_ELEMENT_DESC s_layoutVertexPN[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the VertexPNA structure
static D3D11_INPUT_ELEMENT_DESC s_layoutVertexPNA[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "WEIGHT",   0, DXGI_FORMAT_R32_FLOAT,          0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the VertexPNC structure
static D3D11_INPUT_ELEMENT_DESC s_layoutVertexPNC[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the VertexPNCT structure
static D3D11_INPUT_ELEMENT_DESC s_layoutVertexPNCT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the VertexPNT structure
static D3D11_INPUT_ELEMENT_DESC s_layoutVertexPNT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the VertexPNTB structure
static D3D11_INPUT_ELEMENT_DESC s_layoutVertexPNTB[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "INDEX",    0, DXGI_FORMAT_R32_UINT,           0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "WEIGHT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the LineArtVertex structure
static D3D11_INPUT_ELEMENT_DESC s_layoutLineArtVertex[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the SpriteVertex structure
static D3D11_INPUT_ELEMENT_DESC s_layoutSpriteVertex[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXINDEX", 0, DXGI_FORMAT_R32_FLOAT,          0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the MultiSpriteVertex structure
static D3D11_INPUT_ELEMENT_DESC s_layoutMultiSpriteVertex[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,   D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 28,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",    3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 76,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,       0, 84,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT,       0, 92,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 3, DXGI_FORMAT_R32G32_FLOAT,       0, 100, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXINDEX", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 108, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static const D3D11_INPUT_ELEMENT_DESC *GetVertexDescription(ff::VertexType type)
{
	switch (type)
	{
		default: assert(false);            return nullptr;
		case ff::VERTEX_TYPE_PN:           return s_layoutVertexPN;
		case ff::VERTEX_TYPE_PNA:          return s_layoutVertexPNA;
		case ff::VERTEX_TYPE_PNC:          return s_layoutVertexPNC;
		case ff::VERTEX_TYPE_PNCT:         return s_layoutVertexPNCT;
		case ff::VERTEX_TYPE_PNT:          return s_layoutVertexPNT;
		case ff::VERTEX_TYPE_PNTB:         return s_layoutVertexPNTB;
		case ff::VERTEX_TYPE_LINE_ART:     return s_layoutLineArtVertex;
		case ff::VERTEX_TYPE_SPRITE:       return s_layoutSpriteVertex;
		case ff::VERTEX_TYPE_MULTI_SPRITE: return s_layoutMultiSpriteVertex;
	}
}

static size_t GetVertexDescriptionCount(ff::VertexType type)
{
	switch (type)
	{
		default: assert(false);            return 0;
		case ff::VERTEX_TYPE_PN:           return _countof(s_layoutVertexPN);
		case ff::VERTEX_TYPE_PNA:          return _countof(s_layoutVertexPNA);
		case ff::VERTEX_TYPE_PNC:          return _countof(s_layoutVertexPNC);
		case ff::VERTEX_TYPE_PNCT:         return _countof(s_layoutVertexPNCT);
		case ff::VERTEX_TYPE_PNT:          return _countof(s_layoutVertexPNT);
		case ff::VERTEX_TYPE_PNTB:         return _countof(s_layoutVertexPNTB);
		case ff::VERTEX_TYPE_LINE_ART:     return _countof(s_layoutLineArtVertex);
		case ff::VERTEX_TYPE_SPRITE:       return _countof(s_layoutSpriteVertex);
		case ff::VERTEX_TYPE_MULTI_SPRITE: return _countof(s_layoutMultiSpriteVertex);
	}
}

size_t ff::GetVertexStride(ff::VertexType type)
{
	switch (type)
	{
		default: assert(false);        return 0;
		case VERTEX_TYPE_PN:           return sizeof(VertexPN);
		case VERTEX_TYPE_PNA:          return sizeof(VertexPNA);
		case VERTEX_TYPE_PNC:          return sizeof(VertexPNC);
		case VERTEX_TYPE_PNCT:         return sizeof(VertexPNCT);
		case VERTEX_TYPE_PNT:          return sizeof(VertexPNT);
		case VERTEX_TYPE_PNTB:         return sizeof(VertexPNTB);
		case VERTEX_TYPE_LINE_ART:     return sizeof(LineArtVertex);
		case VERTEX_TYPE_SPRITE:       return sizeof(SpriteVertex);
		case VERTEX_TYPE_MULTI_SPRITE: return sizeof(MultiSpriteVertex);
	}
}

ff::VertexComponents ff::GetVertexComponents(VertexType type)
{
	switch (type)
	{
		default:
			assertRetVal(false, VERTEX_COMP_NONE);

		case VERTEX_TYPE_PN:
			return (VertexComponents)(VERTEX_COMP_POSITION | VERTEX_COMP_NORMAL);

		case VERTEX_TYPE_PNA:
			return (VertexComponents)(VERTEX_COMP_POSITION | VERTEX_COMP_NORMAL | VERTEX_COMP_ANIMWEIGHT);

		case VERTEX_TYPE_PNC:
			return (VertexComponents)(VERTEX_COMP_POSITION | VERTEX_COMP_NORMAL | VERTEX_COMP_COLOR);

		case VERTEX_TYPE_PNCT:
			return (VertexComponents)(VERTEX_COMP_POSITION | VERTEX_COMP_NORMAL | VERTEX_COMP_COLOR | VERTEX_COMP_TEXTURE);

		case VERTEX_TYPE_PNT:
			return (VertexComponents)(VERTEX_COMP_POSITION | VERTEX_COMP_NORMAL | VERTEX_COMP_TEXTURE);

		case VERTEX_TYPE_PNTB:
			return (VertexComponents)(VERTEX_COMP_POSITION | VERTEX_COMP_NORMAL | VERTEX_COMP_TEXTURE | VERTEX_COMP_BONEWEIGHT);

		case VERTEX_TYPE_LINE_ART:
			return (VertexComponents)(VERTEX_COMP_POSITION | VERTEX_COMP_COLOR);

		case VERTEX_TYPE_SPRITE:
			return (VertexComponents)(VERTEX_COMP_POSITION | VERTEX_COMP_COLOR | VERTEX_COMP_TEXTURE | VERTEX_COMP_TEXINDEX);

		case VERTEX_TYPE_MULTI_SPRITE:
			// not really accurate since the colors and textures are repeated four times
			return (VertexComponents)(VERTEX_COMP_POSITION | VERTEX_COMP_COLOR | VERTEX_COMP_TEXTURE | VERTEX_COMP_TEXINDEX);
	}
}

bool ff::Is3dVertexType(VertexType type)
{
	return type != VERTEX_TYPE_UNKNOWN && !Is2dVertexType(type);
}

bool ff::Is2dVertexType(VertexType type)
{
	switch (type)
	{
	case VERTEX_TYPE_LINE_ART:
	case VERTEX_TYPE_SPRITE:
	case VERTEX_TYPE_MULTI_SPRITE:
		return true;

	default:
		return false;
	}
}

bool ff::CreateVertexLayout(
	IGraphDevice *pDevice,
	const void *pShaderBytes,
	size_t nShaderSize,
	VertexType type,
	ID3D11InputLayout **ppLayout)
{
	assertRetVal(pDevice && pDevice->GetDX() && ppLayout, false);
	assertRetVal(pShaderBytes && nShaderSize, false);

	ComPtr<ID3D11InputLayout> pLayout;
	assertRetVal(SUCCEEDED(pDevice->GetDX()->CreateInputLayout(
		GetVertexDescription(type),
		(UINT)GetVertexDescriptionCount(type),
		pShaderBytes,
		nShaderSize,
		&pLayout)), false);

	*ppLayout = pLayout.Detach();
	return true;
}
