#pragma once

namespace ff
{
	class IGraphDevice;

	// Type and order of 3D vertex components:
	// (P) Position
	// (N) Normal
	// (G) Tangent
	// (C) Color
	// (T) Texture
	// (A) Animation weight
	// (B) Bone weights/indexes
	// (S) Specular
	// (M) Ambient
	// (I) Texture index

	enum VertexComponents
	{
		VERTEX_COMP_NONE       = 0x0000,
		VERTEX_COMP_POSITION   = 0x0001,
		VERTEX_COMP_NORMAL     = 0x0002,
		VERTEX_COMP_TANGENT    = 0x0004,
		VERTEX_COMP_COLOR      = 0x0008,
		VERTEX_COMP_TEXTURE    = 0x0010,
		VERTEX_COMP_ANIMWEIGHT = 0x0020,
		VERTEX_COMP_BONEWEIGHT = 0x0040,
		VERTEX_COMP_SPECULAR   = 0x0080,
		VERTEX_COMP_AMBIENT    = 0x0100,
		VERTEX_COMP_TEXINDEX   = 0x0200,

		VERTEX_COMP_FORCE_DWORD = 0x7fffffff,
	};

	enum VertexType
	{
		VERTEX_TYPE_UNKNOWN,
		VERTEX_TYPE_PN,       // Post-transformed vertex for animations
		VERTEX_TYPE_PNA,      // Pre-transformed vertex for animation
		VERTEX_TYPE_PNC,      // Colored vertex
		VERTEX_TYPE_PNCT,     // Colored and textured
		VERTEX_TYPE_PNT,      // Only textured
		VERTEX_TYPE_PNTB,     // Textured skin vertex
		VERTEX_TYPE_3D_COUNT, // Placeholder
		VERTEX_TYPE_LINE_ART,
		VERTEX_TYPE_SPRITE,
		VERTEX_TYPE_MULTI_SPRITE,
	};

	struct BoneWeights
	{
		static const BYTE INVALID_BONE = 0xFF;

		BYTE  bone[4];
		float weight[4];
	};

	struct VertexPN
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
	};

	struct VertexPNA
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		float    weight;
	};

	struct VertexPNC
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT4 color;
	};

	struct VertexPNCT
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT4 color;
		XMFLOAT2 tex;
	};

	struct VertexPNT
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 tex;
	};

	struct VertexPNTB
	{
		XMFLOAT3    pos;
		XMFLOAT3    normal;
		XMFLOAT2    tex;
		BoneWeights bones;
	};

	struct LineArtVertex
	{
		XMFLOAT3 pos;
		XMFLOAT4 color;
	};

	struct SpriteVertex
	{
		XMFLOAT3 pos;
		XMFLOAT4 color;
		XMFLOAT2 tex;
		float    ntex;
		// UINT ntex; // not 9_1 compatible
	};

	struct MultiSpriteVertex
	{
		XMFLOAT3 pos;
		XMFLOAT4 color0;
		XMFLOAT4 color1;
		XMFLOAT4 color2;
		XMFLOAT4 color3;
		XMFLOAT2 tex0;
		XMFLOAT2 tex1;
		XMFLOAT2 tex2;
		XMFLOAT2 tex3;
		XMFLOAT4 ntex;
		// UINT ntex; // not 9_1 compatible
	};

	typedef Vector<VertexPN> VertexesPN;
	typedef Vector<VertexPNCT> VertexesPNCT;
	typedef Vector<LineArtVertex> LineArtVertexs;
	typedef Vector<SpriteVertex> SpriteVertexes;
	typedef Vector<MultiSpriteVertex> MultiSpriteVertexes;

	bool Is3dVertexType(VertexType type);
	bool Is2dVertexType(VertexType type);

	UTIL_API size_t GetVertexStride(VertexType type);
	VertexComponents GetVertexComponents(VertexType type);

	bool CreateVertexLayout(
		IGraphDevice *pDevice,
		const void *pShaderBytes,
		size_t nShaderSize,
		VertexType type,
		ID3D11InputLayout **ppLayout);
}

MAKE_POD(ff::BoneWeights);
MAKE_POD(ff::VertexPN);
MAKE_POD(ff::VertexPNA);
MAKE_POD(ff::VertexPNC);
MAKE_POD(ff::VertexPNCT);
MAKE_POD(ff::VertexPNT);
MAKE_POD(ff::VertexPNTB);
MAKE_POD(ff::LineArtVertex);
MAKE_POD(ff::SpriteVertex);
MAKE_POD(ff::MultiSpriteVertex);
