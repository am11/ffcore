#include "pch.h"
#include "COM/ComAlloc.h"
#include "Graph/Anim/AnimPos.h"
#include "Graph/2D/2dEffect.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/Sprite.h"
#include "Graph/BufferCache.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/VertexFormat.h"
#include "Module/ModuleFactory.h"

namespace ff
{
	// Keeps track of all the textures used when drawing a multi-sprite.
	// There can be dupes in this list since sprites don't have to use unique textures.
	struct MultiSpriteTextures
	{
		static const size_t MAX_TEXTURES = 4;
		typedef Vector<IGraphTexture *, MAX_TEXTURES> TextureVector;

		TextureVector _textures;
	};

	// Keeps track of a set of textures that can be selected into the GPU
	// at the same time. The list of sprites and multi-sprites to draw using
	// those textures are also stored here.
	struct TextureBucket
	{
		static const size_t MAX_TEXTURES = 8;
		typedef Vector<IGraphTexture *, MAX_TEXTURES> TextureVector;

		TextureVector _textures;
		SpriteVertexes *_sprites;
		MultiSpriteVertexes *_multiSprites;
		size_t _maxTextures;

		// Checks if this bucket can be used when drawing a new sprite or multi-sprite
		bool IsSupported(IGraphTexture *pTexture, size_t &nIndex) const;
		bool AddSupport (IGraphTexture *pTexture, size_t &nIndex);

		bool IsSupported(const MultiSpriteTextures &mst, size_t indexes[MultiSpriteTextures::MAX_TEXTURES]) const;
		bool AddSupport (const MultiSpriteTextures &mst, size_t indexes[MultiSpriteTextures::MAX_TEXTURES]);
	};
}

bool ff::TextureBucket::IsSupported(IGraphTexture *pTexture, size_t &nIndex) const
{
	nIndex = _textures.Find(pTexture);
	return nIndex != INVALID_SIZE;
}

bool ff::TextureBucket::AddSupport(IGraphTexture *pTexture, size_t &nIndex)
{
	nIndex = _textures.Find(pTexture);

	if (nIndex == INVALID_SIZE)
	{
		if (_textures.Size() < _maxTextures)
		{
			nIndex = _textures.Size();
			_textures.Push(pTexture);
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool ff::TextureBucket::IsSupported(const MultiSpriteTextures &mst, size_t indexes[MultiSpriteTextures::MAX_TEXTURES]) const
{
	IGraphTexture *pPrev = nullptr;

	for (size_t i = 0; i < mst._textures.Size(); i++)
	{
		IGraphTexture *pFind = mst._textures[i];

		if (pFind != pPrev)
		{
			indexes[i] = _textures.Find(pFind);

			if (indexes[i] == INVALID_SIZE)
			{
				return false;
			}

			pPrev = pFind;
		}
		else
		{
			indexes[i] = indexes[i - 1];
		}
	}

	// Every texture was found in my list
	return true;
}

bool ff::TextureBucket::AddSupport(const MultiSpriteTextures &mst, size_t indexes[MultiSpriteTextures::MAX_TEXTURES])
{
	TextureVector addTextures;

	// See which textures aren't already in my list

	for (size_t i = 0; i < mst._textures.Size(); i++)
	{
		IGraphTexture *pFind = mst._textures[i];
		indexes[i] = _textures.Find(pFind);

		if (indexes[i] == INVALID_SIZE)
		{
			indexes[i] = addTextures.Find(pFind);

			if (indexes[i] == INVALID_SIZE)
			{
				indexes[i] = _textures.Size() + addTextures.Size();
				addTextures.Push(pFind);
			}
			else
			{
				indexes[i] += _textures.Size();
			}
		}
	}

	if (addTextures.Size() > _maxTextures - _textures.Size())
	{
		// Not enough empty slots to add the new textures
		return false;
	}

	// Add each new texture

	for (size_t i = 0; i < addTextures.Size(); i++)
	{
		_textures.Push(addTextures[i]);
	}

	return true;
}

namespace ff
{
	struct RenderState
	{
		ComPtr<IRenderTarget> _target;
		ComPtr<IRenderDepth> _depth;
		RectFloat _viewRect;

		// Cached old state
		size_t _matrixStackSize[MATRIX_COUNT];
		size_t _effectStackSize;
		float  _oldSpriteDepth;
	};


	class __declspec(uuid("6e511227-0dd9-4966-9ced-d244c9eb3c46"))
		Renderer2d : public ComBase, public I2dRenderer
	{
	public:
		DECLARE_HEADER(Renderer2d);

		bool Init();

		// ComBase
		virtual HRESULT _Construct(IUnknown *unkOuter) override;

		// IGraphDeviceChild
		virtual IGraphDevice* GetDevice() const override;
		virtual void          Reset() override;

		// I2dRenderer functions
		virtual bool BeginRender(
			IRenderTarget *pTarget,
			IRenderDepth *pDepth,
			RectFloat viewRect,
			PointFloat worldTopLeft,
			PointFloat worldScale,
			I2dEffect *pEffect) override;

		virtual void Flush() override;
		virtual void EndRender(bool resetDepth) override;
		virtual I2dEffect *GetEffect() override;
		virtual IRenderTarget *GetTarget() override;
		virtual bool PushEffect(I2dEffect *pEffect) override;
		virtual void PopEffect() override;
		virtual void NudgeDepth() override;
		virtual void ResetDepth() override;
		virtual const RectFloat &GetRenderViewRect() const override;
		virtual const RectFloat &GetRenderWorldRect() const override;
		virtual PointFloat GetZBounds() const override;

		virtual void DrawSprite(const ISprite *pSprite, const PointFloat *pPos, const PointFloat *pScale, const float rotate, const XMFLOAT4 *pColor) override;
		virtual void DrawMultiSprite(ISprite **ppSprite, size_t nSpriteCount, const PointFloat *pPos, const PointFloat *pScale, const float rotate, const XMFLOAT4 *pColor, size_t nColorCount) override;
		virtual bool DrawCachedSprites(IUnknown*  pCached) override;
		virtual bool CacheSprites(IUnknown** pCached) override;
		virtual void DrawPoints(const PointFloat *pPoints, const XMFLOAT4 *pColor, size_t nCount) override;
		virtual void DrawPolyLine(const PointFloat *pPoints, size_t nCount, const XMFLOAT4 *pColor) override;
		virtual void DrawClosedPolyLine(const PointFloat *pPoints, size_t nCount, const XMFLOAT4 *pColor) override;
		virtual void DrawLine(const PointFloat *pStart, const PointFloat *pEnd, const XMFLOAT4 *pColor) override;
		virtual void DrawRectangle(const RectFloat *pRect, const XMFLOAT4 *pColor) override;
		virtual void DrawRectangle(const RectFloat *pRect, float thickness, const XMFLOAT4 *pColor) override;
		virtual void DrawFilledRectangle(const RectFloat *pRect, const XMFLOAT4 *pColor, size_t nColorCount) override;
		virtual void DrawFilledTriangle(const PointFloat *pPoints, const XMFLOAT4 *pColor, size_t nColorCount) override;

		virtual XMMATRIX &GetMatrix(MatrixType type) override;
		virtual void SetMatrix(MatrixType type, const XMMATRIX *pMatrix) override;
		virtual void TransformMatrix(MatrixType type, const XMMATRIX *pMatrix) override;
		virtual void PushMatrix(MatrixType type) override;
		virtual void PopMatrix(MatrixType type) override;

	private:
		// Constants
		static const size_t MAX_DEFER_SPRITES       = 16384;
		static const size_t SPRITE_CHUNK_SIZE       = 2048;
		static const size_t MULTI_SPRITE_CHUNK_SIZE = 1024;
		static const float  SPRITE_DEPTH_DELTA;
		static const float  MAX_SPRITE_DEPTH;

		void Destroy();

		bool BeginRender(RenderState *pState);
		void EndRender(RenderState *pState);
		void PopRenderState(bool resetDepth);

		void FlushSprites();
		void FlushRectangles();
		void FlushTriangles();
		void FlushLines();
		void FlushPoints();

		// Render state
		ComPtr<IGraphDevice> _device;
		Vector<XMMATRIX> _matrixStack[MATRIX_COUNT];
		Vector<ComPtr<I2dEffect>> _effectStack;
		Vector<RenderState> _renderStateStack;
		RenderState *_renderState;
		RectFloat _renderWorldRect;
		float _spriteDepth;
		bool _level9;

		// Line art
		Vector<LineArtVertex> _lines;
		Vector<LineArtVertex> _points;
		Vector<LineArtVertex> _rectangles;
		Vector<LineArtVertex> _triangles;

		// Sprites
		void ClearSpriteArrays();
		void DeleteSpriteArrays();
		TextureBucket *GetTextureBucket(const ComPtr<IGraphTexture> &pTexture, size_t &nIndex);
		TextureBucket *GetTextureBucket(const MultiSpriteTextures &mst, size_t indexes[MultiSpriteTextures::MAX_TEXTURES]);
		void MapTexturesToBucket(const MultiSpriteTextures &mst, TextureBucket *pBucket);
		void CacheTextureBucket(TextureBucket *pBucket);
		void CombineTextureBuckets();

		ComPtr<ID3D11Buffer> _spriteIndexes;
		ComPtr<ID3D11Buffer> _triangleIndexes;
		Vector<TextureBucket *> _textureBuckets;
		TextureBucket *_lastBuckets[2];
		Map<ComPtr<IGraphTexture>, TextureBucket*> _textureToBuckets;

		// Sprite allocator stuff
		TextureBucket *NewTextureBucket();
		SpriteVertexes *NewSpriteVertexes();
		MultiSpriteVertexes *NewMultiSpriteVertexes();
		void DeleteTextureBucket(TextureBucket *pObj);
		void DeleteSpriteVertexes(SpriteVertexes *pObj);
		void DeleteMultiSpriteVertexes(MultiSpriteVertexes* pObj);

		PoolAllocator<TextureBucket> _textureBucketPool;
		Vector<SpriteVertexes *> _freeSprites;
		Vector<MultiSpriteVertexes *> _freeMultiSprites;
	};
}

BEGIN_INTERFACES(ff::Renderer2d)
	HAS_INTERFACE(ff::I2dRenderer)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"C2dRenderer");
	module.RegisterClassT<ff::Renderer2d>(name, GUID_NULL, ff::GetCategoryGraphicsObject());
});


const float ff::Renderer2d::SPRITE_DEPTH_DELTA = 1.0f / 2048.0f;
const float ff::Renderer2d::MAX_SPRITE_DEPTH = ff::Renderer2d::MAX_DEFER_SPRITES * ff::Renderer2d::SPRITE_DEPTH_DELTA;

bool ff::Create2dRenderer(IGraphDevice *pDevice, I2dRenderer **ppRender)
{
	assertRetVal(ppRender && pDevice, false);
	*ppRender = nullptr;

	ComPtr<Renderer2d> pRender;
	assertRetVal(SUCCEEDED(ComAllocator<Renderer2d>::CreateInstance(pDevice, &pRender)), false);
	assertRetVal(pRender->Init(), false);

	*ppRender = pRender.Detach();
	return true;
}

namespace ff
{
	class __declspec(uuid("b0d75d3c-af95-493b-a306-ee156752f2d7"))
		CCachedSprites : public ComBase, public IGraphDeviceChild
	{
	public:
		DECLARE_HEADER(CCachedSprites);
	
		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		bool Init();

		// IGraphDeviceChild
		virtual IGraphDevice* GetDevice() const override;
		virtual void          Reset() override;

		struct Bucket
		{
			Vector<ComPtr<IGraphTexture>, 8> _textures;
			ComPtr<ID3D11Buffer>             _spriteBuffer;
			ComPtr<ID3D11Buffer>             _multiSpriteBuffer;
			size_t                           _sprites;
			size_t                           _multiSprites;
		};

		size_t  GetBucketCount();
		Bucket& GetBucket(size_t nIndex);
		bool    AddBucket(
			IGraphTexture**    ppTextures,    size_t nTextures,
			SpriteVertex*      pSprites,      size_t nSprites,
			MultiSpriteVertex* pMultiSprites, size_t nMultiSprites);

		bool  IsValid() const;
		float GetZOffset(); // the furthest back z-coordinate of any sprite

	private:
		void Destroy();

		ComPtr<IGraphDevice> _device;
		Vector<Bucket *> _buckets;
		float _zoffset;
		bool _valid;
	};
}

BEGIN_INTERFACES(ff::CCachedSprites)
END_INTERFACES()

static ff::ModuleStartup RegisterCachedSprites([](ff::Module &module)
{
	static ff::StaticString name(L"CCachedSprites");
	module.RegisterClassT<ff::CCachedSprites>(name, GUID_NULL, ff::GetCategoryGraphicsObject());
});

ff::CCachedSprites::CCachedSprites()
	: _zoffset(0)
	, _valid(true)
{
}

ff::CCachedSprites::~CCachedSprites()
{
	Destroy();
}

void ff::CCachedSprites::Destroy()
{
	for (size_t i = 0; i < _buckets.Size(); i++)
	{
		delete _buckets[i];
	}

	_valid = false;
}

HRESULT ff::CCachedSprites::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	
	return __super::_Construct(unkOuter);
}

bool ff::CCachedSprites::Init()
{
	assertRetVal(_device, false);

	return true;
}

ff::IGraphDevice *ff::CCachedSprites::GetDevice() const
{
	return _device;
}

void ff::CCachedSprites::Reset()
{
	Destroy();
}

size_t ff::CCachedSprites::GetBucketCount()
{
	return _buckets.Size();
}

ff::CCachedSprites::Bucket &ff::CCachedSprites::GetBucket(size_t nIndex)
{
	return *_buckets[nIndex];
}

bool ff::CCachedSprites::AddBucket(
	IGraphTexture**    ppTextures,    size_t nTextures,
	SpriteVertex*      pSprites,      size_t nSprites,
	MultiSpriteVertex* pMultiSprites, size_t nMultiSprites)
{
	assertRetVal(ppTextures && nTextures, false);
	assertRetVal(pSprites || pMultiSprites, false);
	assertRetVal(!nSprites || pSprites, false);
	assertRetVal(!nMultiSprites || pMultiSprites, false);

	Bucket *pBucket = new Bucket();
	pBucket->_sprites      = nSprites;
	pBucket->_multiSprites = nMultiSprites;

	_buckets.Push(pBucket);

	for (size_t i = 0; i < nTextures; i++)
	{
		pBucket->_textures.Push(ppTextures[i]);
	}

	bool bFoundZOffset = false;

	if (nSprites)
	{
		for (size_t i = 0; i < nSprites; i++)
		{
			if (!bFoundZOffset || pSprites[i].pos.z > _zoffset)
			{
				_zoffset     = pSprites[i].pos.z;
				bFoundZOffset = true;
			}
		}

		assertRetVal(_device->GetVertexBuffers().CreateStaticBuffer(
			pSprites, nSprites * sizeof(SpriteVertex), &pBucket->_spriteBuffer), false);
	}

	if (nMultiSprites)
	{
		for (size_t i = 0; i < nMultiSprites; i++)
		{
			if (!bFoundZOffset || pMultiSprites[i].pos.z > _zoffset)
			{
				_zoffset     = pMultiSprites[i].pos.z;
				bFoundZOffset = true;
			}
		}

		assertRetVal(_device->GetVertexBuffers().CreateStaticBuffer(
			pMultiSprites, nMultiSprites * sizeof(MultiSpriteVertex), &pBucket->_multiSpriteBuffer), false);
	}

	return true;
}

bool ff::CCachedSprites::IsValid() const
{
	return _valid;
}

float ff::CCachedSprites::GetZOffset()
{
	return _zoffset;
}

ff::Renderer2d::Renderer2d()
	: _renderState(nullptr)
	, _spriteDepth(0)
	, _level9(false)
	, _textureBucketPool(false)
	, _renderWorldRect(0, 0, 0, 0)
{
	ZeroObject(_lastBuckets);

	_textureToBuckets.SetBucketCount(256, false);

	PushMatrix(MATRIX_PROJECTION);
	PushMatrix(MATRIX_VIEW);
	PushMatrix(MATRIX_WORLD);
}

ff::Renderer2d::~Renderer2d()
{
	Destroy();
}

HRESULT ff::Renderer2d::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	
	return __super::_Construct(unkOuter);
}

bool ff::Renderer2d::Init()
{
	assertRetVal(_device, false);
	_level9 = _device->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0;

	Vector<WORD> indexes;
	indexes.Resize(SPRITE_CHUNK_SIZE * 6);

	WORD wCurVertex = 0;
	for (size_t nCurIndex = 0; nCurIndex < indexes.Size(); nCurIndex += 6, wCurVertex += 4)
	{
		indexes[nCurIndex + 0] = wCurVertex + 0;
		indexes[nCurIndex + 1] = wCurVertex + 1;
		indexes[nCurIndex + 2] = wCurVertex + 2;
		indexes[nCurIndex + 3] = wCurVertex + 0;
		indexes[nCurIndex + 4] = wCurVertex + 2;
		indexes[nCurIndex + 5] = wCurVertex + 3;
	}

	assertRetVal(_device->GetIndexBuffers().CreateStaticBuffer(
		indexes.Data(), indexes.ByteSize(), &_spriteIndexes), false);

	indexes.Reserve(SPRITE_CHUNK_SIZE * 3);

	wCurVertex = 0;
	for (size_t nCurIndex = 0; nCurIndex < indexes.Size(); nCurIndex++, wCurVertex++)
	{
		indexes[nCurIndex] = wCurVertex;
	}

	assertRetVal(_device->GetIndexBuffers().CreateStaticBuffer(
		indexes.Data(), indexes.ByteSize(), &_triangleIndexes), false);

	return true;
}

void ff::Renderer2d::Destroy()
{
	DeleteSpriteArrays();

	_spriteIndexes   = nullptr;
	_triangleIndexes = nullptr;
}

ff::IGraphDevice *ff::Renderer2d::GetDevice() const
{
	return _device;
}

void ff::Renderer2d::Reset()
{
	Destroy();
	verify(Init());
}

bool ff::Renderer2d::BeginRender(RenderState *pState)
{
	// Target and depth views
	{
		ID3D11RenderTargetView* pTargetView = pState->_target->GetTarget();
		ID3D11DepthStencilView* pDepthView  = nullptr;

		if (!pTargetView)
		{
			return false;
		}

		if (pState->_depth)
		{
			assertRetVal(pState->_depth->SetSize(pState->_target->GetSize()), false);
			pDepthView = pState->_depth->GetView();
			assertRetVal(pDepthView, false);
		}

		_device->GetContext()->OMSetRenderTargets(1, &pTargetView, pDepthView);
	}

	// Viewport
	{
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = pState->_viewRect.left;
		viewport.TopLeftY = pState->_viewRect.top;
		viewport.Width    = pState->_viewRect.Width();
		viewport.Height   = pState->_viewRect.Height();
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;

		_device->GetContext()->RSSetViewports(1, &viewport);
	}

	// Effect

	if (!GetEffect()->OnBeginRender(this))
	{
		return false;
	}

	return true;
}

void ff::Renderer2d::EndRender(RenderState *pState)
{
	Flush();

	GetEffect()->OnEndRender(this);
}

void ff::Renderer2d::PopRenderState(bool resetDepth)
{
	assertRet(_renderState);

	assert(_renderState->_effectStackSize < _effectStack.Size());
	_effectStack.Resize(_renderState->_effectStackSize);

	for (size_t i = 0; i < MATRIX_COUNT; i++)
	{
		assert(_renderState->_matrixStackSize[i] <= _matrixStack[i].Size());
		_matrixStack[i].Resize(_renderState->_matrixStackSize[i]);
	}

	if (resetDepth)
	{
		_spriteDepth = _renderState->_oldSpriteDepth;
	}

	_renderStateStack.Pop();

	if (_renderStateStack.Size())
	{
		_renderState = &_renderStateStack.GetLast();
		BeginRender(_renderState);
	}
	else
	{
		_renderState = nullptr;
		_renderWorldRect.SetRect(0, 0, 0, 0);
	}
}

bool ff::Renderer2d::BeginRender(
	IRenderTarget *pTarget,
	IRenderDepth *pDepth,
	RectFloat viewRect,
	PointFloat worldTopLeft,
	PointFloat worldScale,
	I2dEffect *pEffect)
{
	assertRetVal(pTarget && pEffect && pEffect->IsValid(), false);
	assertRetVal(worldScale.x != 0.0f && worldScale.y != 0.0f, false);
	noAssertRetVal(viewRect.Width() > 0 && viewRect.Height() > 0, false);

	if (_renderState)
	{
		EndRender(_renderState);
	}

	// Create new render state
	{
		_renderStateStack.Push(RenderState());
		_renderState = &_renderStateStack.GetLast();

		_renderState->_target = pTarget;
		_renderState->_depth = pDepth;
		_renderState->_oldSpriteDepth = _spriteDepth;
		_renderState->_effectStackSize = _effectStack.Size();
		_renderState->_viewRect = viewRect;

		for (size_t i = 0; i < MATRIX_COUNT; i++)
		{
			_renderState->_matrixStackSize[i] = _matrixStack[i].Size();
		}
	}

	// Push new state
	{
		_renderWorldRect.SetRect(
			worldTopLeft.x,
			worldTopLeft.y,
			worldTopLeft.x + worldScale.x * viewRect.Width(),
			worldTopLeft.y + worldScale.y * viewRect.Height());

		XMMATRIX matProj = XMMatrixOrthographicOffCenterLH(
			_renderWorldRect.left, _renderWorldRect.right,
			-_renderWorldRect.bottom, -_renderWorldRect.top,
			-MAX_SPRITE_DEPTH, SPRITE_DEPTH_DELTA);

		PushMatrix(MATRIX_PROJECTION);
		SetMatrix(MATRIX_PROJECTION, &matProj);

		_effectStack.Push(pEffect);
	}

	if (!BeginRender(_renderState))
	{
		PopRenderState(true);
		return false;
	}

	return true;
}

void ff::Renderer2d::EndRender(bool resetDepth)
{
	assertRet(_renderState);

	EndRender(_renderState);
	PopRenderState(resetDepth);
}

ff::I2dEffect *ff::Renderer2d::GetEffect()
{
	return _effectStack.Size() ? _effectStack.GetLast() : nullptr;
}

ff::IRenderTarget *ff::Renderer2d::GetTarget()
{
	assertRetVal(_renderState, nullptr);
	return _renderState->_target;
}

bool ff::Renderer2d::PushEffect(I2dEffect *pEffect)
{
	assertRetVal(pEffect && pEffect->IsValid(), false);

	bool bNewEffect = (pEffect != GetEffect());

	if (bNewEffect)
	{
		Flush();
		GetEffect()->OnEndRender(this);
	}

	_effectStack.Push(pEffect);

	if (bNewEffect)
	{
		// ignore failure
		GetEffect()->OnBeginRender(this);
	}

	return true;
}

void ff::Renderer2d::PopEffect()
{
	assertRet(_effectStack.Size() > 1);

	bool bNewEffect = (_effectStack.GetLast() != _effectStack[_effectStack.Size() - 2]);

	if (bNewEffect)
	{
		Flush();
		GetEffect()->OnEndRender(this);
	}

	_effectStack.Pop();

	if (bNewEffect)
	{
		// ignore failure
		GetEffect()->OnBeginRender(this);
	}
}

void ff::Renderer2d::NudgeDepth()
{
	_spriteDepth -= SPRITE_DEPTH_DELTA;
}

void ff::Renderer2d::ResetDepth()
{
	_spriteDepth = 0;
}

const ff::RectFloat &ff::Renderer2d::GetRenderViewRect() const
{
	assert(_renderState);
	return _renderState->_viewRect;
}

const ff::RectFloat &ff::Renderer2d::GetRenderWorldRect() const
{
	assert(_renderState);
	return _renderWorldRect;
}

ff::PointFloat ff::Renderer2d::GetZBounds() const
{
	return PointFloat(-MAX_SPRITE_DEPTH, SPRITE_DEPTH_DELTA);
}

void ff::Renderer2d::Flush()
{
	assertRet(_renderState);

	FlushRectangles();
	FlushTriangles();
	FlushLines();
	FlushPoints();
	FlushSprites();
}

static void AdjustSpriteRect(const ff::RectFloat &rect, const ff::PointFloat *pPos, const ff::PointFloat *pScale, const float rotate, ff::PointFloat outRectPoints[4])
{
	XMMATRIX matrix = XMMatrixAffineTransformation2D(
		pScale ? XMLoadFloat2((XMFLOAT2*)pScale) : XMVectorSplatOne(),
		XMVectorZero(),
		rotate,
		pPos ? XMLoadFloat2((XMFLOAT2*)pPos) : XMVectorZero());

	outRectPoints[0].SetPoint(rect.left,  rect.top);
	outRectPoints[1].SetPoint(rect.right, rect.top);
	outRectPoints[2].SetPoint(rect.right, rect.bottom);
	outRectPoints[3].SetPoint(rect.left,  rect.bottom);

	XMFLOAT4 output[4];
	XMVector2TransformStream(output, sizeof(XMFLOAT4), (XMFLOAT2*)outRectPoints, sizeof(XMFLOAT2), 4, matrix);

	outRectPoints[0].SetPoint(output[0].x, output[0].y);
	outRectPoints[1].SetPoint(output[1].x, output[1].y);
	outRectPoints[2].SetPoint(output[2].x, output[2].y);
	outRectPoints[3].SetPoint(output[3].x, output[3].y);
}

static void AdjustSpriteRect(const ff::RectFloat &rect, const ff::PointFloat *pPos, const ff::PointFloat *pScale, ff::PointFloat outRectPoints[4])
{
	ff::RectFloat rectCopy;
	const ff::RectFloat *pRectCopy;

	if (pPos || pScale)
	{
		XMStoreFloat4((XMFLOAT4*)&rectCopy.arr[0],
			XMVectorMultiplyAdd(
				XMLoadFloat4((const XMFLOAT4*)&rect.arr[0]),
				pScale ? XMVectorSet(pScale->x, pScale->y, pScale->x, pScale->y) : XMVectorSplatOne(),
				pPos   ? XMVectorSet(pPos->x,   pPos->y,   pPos->x,   pPos->y)   : XMVectorZero()));

		pRectCopy = &rectCopy;
	}
	else
	{
		pRectCopy = &rect;
	}

	outRectPoints[0].SetPoint(pRectCopy->left,  pRectCopy->top);
	outRectPoints[1].SetPoint(pRectCopy->right, pRectCopy->top);
	outRectPoints[2].SetPoint(pRectCopy->right, pRectCopy->bottom);
	outRectPoints[3].SetPoint(pRectCopy->left,  pRectCopy->bottom);
}

void ff::Renderer2d::DrawSprite(
		const ISprite*   pSprite,
		const PointFloat*    pPos,
		const PointFloat*    pScale,
		const float      rotate,
		const XMFLOAT4*  pColor)
{
	assertRet(pSprite);

	const SpriteData &data = pSprite->GetData();
	size_t nTextureIndex = INVALID_SIZE;
	SpriteVertexes *pSpriteArray = GetTextureBucket(data._texture, nTextureIndex)->_sprites;
	size_t nFirstSprite = pSpriteArray->Size();

	PointFloat rectPoints[4];

	if (rotate != 0)
	{
		AdjustSpriteRect(data._worldRect, pPos, pScale, -rotate, rectPoints);
	}
	else
	{
		AdjustSpriteRect(data._worldRect, pPos, pScale, rectPoints);
	}

	SpriteVertex sv;
	sv.color = pColor ? *pColor : GetColorWhite();

	pSpriteArray->InsertDefault(nFirstSprite, 4);
	SpriteVertex *pWriteSprite = pSpriteArray->Data(nFirstSprite, 4);

	// upper left
	sv.pos.x = rectPoints[0].x;
	sv.pos.y = -rectPoints[0].y;
	sv.pos.z = _spriteDepth;
	sv.tex.x = data._textureUV.left;
	sv.tex.y = data._textureUV.top;
	sv.ntex  = (float)nTextureIndex;
	pWriteSprite[0] = sv;

	// upper right
	sv.pos.x = rectPoints[1].x;
	sv.pos.y = -rectPoints[1].y;
	sv.tex.x = data._textureUV.right;
	pWriteSprite[1] = sv;

	// bottom right
	sv.pos.x = rectPoints[2].x;
	sv.pos.y = -rectPoints[2].y;
	sv.tex.y = data._textureUV.bottom;
	pWriteSprite[2] = sv;

	// bottom left
	sv.pos.x = rectPoints[3].x;
	sv.pos.y = -rectPoints[3].y;
	sv.tex.x = data._textureUV.left;
	pWriteSprite[3] = sv;

	NudgeDepth();
}

void ff::Renderer2d::DrawMultiSprite(
	ISprite **ppSprite,
	size_t nSpriteCount,
	const PointFloat *pPos,
	const PointFloat *pScale,
	const float rotate,
	const XMFLOAT4 *pColors,
	size_t nColorCount)
{
	assertRet(ppSprite && nSpriteCount);

	XMFLOAT4 colors[4] = { GetColorWhite(), GetColorWhite(), GetColorWhite(), GetColorWhite() };

	switch (nColorCount)
	{
		case 4: colors[3] = pColors[3]; __fallthrough;
		case 3: colors[2] = pColors[2]; __fallthrough;
		case 2: colors[1] = pColors[1]; __fallthrough;
		case 1: colors[0] = pColors[0]; __fallthrough;
	}

	if (_level9) // DX9 cards can barely handle the multi-sprite pixel shader
	{
		for (size_t i = 0; i < nSpriteCount; i++)
		{
			DrawSprite(ppSprite[i], pPos, pScale, rotate, &colors[i]);
		}

		return;
	}

	// Get the sprite data (up to four)

	const SpriteData *data[4];
	data[0] = &ppSprite[0]->GetData();
	data[1] = data[0];
	data[2] = data[0];
	data[3] = data[0];

	switch (nSpriteCount)
	{
		case 4: data[3] = &ppSprite[3]->GetData(); __fallthrough;
		case 3: data[2] = &ppSprite[2]->GetData(); __fallthrough;
		case 2: data[1] = &ppSprite[1]->GetData(); __fallthrough;
	}

	// Get the texture for each sprite data

	MultiSpriteTextures mst;
	mst._textures.Push(data[0]->_texture);

	if (nSpriteCount > 1) mst._textures.Push(data[1]->_texture);
	if (nSpriteCount > 2) mst._textures.Push(data[2]->_texture);
	if (nSpriteCount > 3) mst._textures.Push(data[3]->_texture);

	// Get the colors

	MultiSpriteVertex msv;
	CopyMemory(&msv.color0, colors, sizeof(colors));

	// Find which vertex array to use

	size_t textureIndexes[4] = { INVALID_SIZE, INVALID_SIZE, INVALID_SIZE, INVALID_SIZE };
	MultiSpriteVertexes* pSpriteArray = GetTextureBucket(mst, textureIndexes)->_multiSprites;
	size_t               nFirstSprite = pSpriteArray->Size();

	// Find out where this sprite is drawn in the world (from the first sprite)

	PointFloat rectPoints[4];

	if (rotate != 0)
	{
		AdjustSpriteRect(data[0]->_worldRect, pPos, pScale, -rotate, rectPoints);
	}
	else
	{
		AdjustSpriteRect(data[0]->_worldRect, pPos, pScale, rectPoints);
	}

	// Create the vertexes

	pSpriteArray->InsertDefault(nFirstSprite, 4);
	MultiSpriteVertex *pWriteSprite = pSpriteArray->Data(nFirstSprite, 4);

	// upper left
	msv.pos.x  = rectPoints[0].x;
	msv.pos.y  = -rectPoints[0].y;
	msv.pos.z  = _spriteDepth;
	msv.tex0.x = data[0]->_textureUV.left;
	msv.tex0.y = data[0]->_textureUV.top;
	msv.tex1.x = data[1]->_textureUV.left;
	msv.tex1.y = data[1]->_textureUV.top;
	msv.tex2.x = data[2]->_textureUV.left;
	msv.tex2.y = data[2]->_textureUV.top;
	msv.tex3.x = data[3]->_textureUV.left;
	msv.tex3.y = data[3]->_textureUV.top;
	msv.ntex.x = (textureIndexes[0] == INVALID_SIZE) ? -1.0f : (float)textureIndexes[0];
	msv.ntex.y = (textureIndexes[1] == INVALID_SIZE) ? -1.0f : (float)textureIndexes[1];
	msv.ntex.z = (textureIndexes[2] == INVALID_SIZE) ? -1.0f : (float)textureIndexes[2];
	msv.ntex.w = (textureIndexes[3] == INVALID_SIZE) ? -1.0f : (float)textureIndexes[3];
	pWriteSprite[0] = msv;

	// upper right
	msv.pos.x  = rectPoints[1].x;
	msv.pos.y  = -rectPoints[1].y;
	msv.tex0.x = data[0]->_textureUV.right;
	msv.tex1.x = data[1]->_textureUV.right;
	msv.tex2.x = data[2]->_textureUV.right;
	msv.tex3.x = data[3]->_textureUV.right;
	pWriteSprite[1] = msv;

	// bottom right
	msv.pos.x  = rectPoints[2].x;
	msv.pos.y  = -rectPoints[2].y;
	msv.tex0.y = data[0]->_textureUV.bottom;
	msv.tex1.y = data[1]->_textureUV.bottom;
	msv.tex2.y = data[2]->_textureUV.bottom;
	msv.tex3.y = data[3]->_textureUV.bottom;
	pWriteSprite[2] = msv;

	// bottom left
	msv.pos.x  = rectPoints[3].x;
	msv.pos.y  = -rectPoints[3].y;
	msv.tex0.x = data[0]->_textureUV.left;
	msv.tex1.x = data[1]->_textureUV.left;
	msv.tex2.x = data[2]->_textureUV.left;
	msv.tex3.x = data[3]->_textureUV.left;
	pWriteSprite[3] = msv;

	NudgeDepth();
}

bool ff::Renderer2d::DrawCachedSprites(IUnknown *pUnknown)
{
	assertRetVal(_renderState, false);

	ComPtr<CCachedSprites> pCached;
	assertRetVal(pCached.QueryFrom(pUnknown), false);

	float zoffset = _spriteDepth - pCached->GetZOffset();
	I2dEffect *pEffect = GetEffect();

	for (size_t nBucket = 0; nBucket < pCached->GetBucketCount(); nBucket++)
	{
		CCachedSprites::Bucket &bucket = pCached->GetBucket(nBucket);
		_spriteDepth -= (bucket._sprites + bucket._multiSprites) / 4 * SPRITE_DEPTH_DELTA;

		pEffect->ApplyTextures(this, (IGraphTexture**)bucket._textures.Data(), bucket._textures.Size());

		// Draw single sprites
		for (size_t i = 0, nCount = 0; i < bucket._sprites; i += nCount)
		{
			// They must be drawn in chunks, figure out the chunk size
			nCount = std::min(bucket._sprites - i, SPRITE_CHUNK_SIZE * 4);

			if (pEffect->Apply(this, DRAW_TYPE_SPRITE, bucket._spriteBuffer, _spriteIndexes, zoffset))
			{
				_device->GetContext()->DrawIndexed((UINT)nCount * 6 / 4, 0, (int)i);
			}
		}

		// Draw multi-sprites
		for (size_t i = 0, nCount = 0; i < bucket._multiSprites; i += nCount)
		{
			// They must be drawn in chunks, figure out the chunk size
			nCount = std::min(bucket._multiSprites - i, MULTI_SPRITE_CHUNK_SIZE * 4);

			if (pEffect->Apply(this, DRAW_TYPE_MULTI_SPRITE, bucket._multiSpriteBuffer, _spriteIndexes, zoffset))
			{
				_device->GetContext()->DrawIndexed((UINT)nCount * 6 / 4, 0, (int)i);
			}
		}
	}

	return pCached->IsValid();
}

bool ff::Renderer2d::CacheSprites(IUnknown** ppCached)
{
	assertRetVal(ppCached, false);
	*ppCached = nullptr;

	ComPtr<CCachedSprites> pCached;
	assertRetVal(SUCCEEDED(ComAllocator<CCachedSprites>::CreateInstance(_device, &pCached)), false);
	assertRetVal(pCached->Init(), false);

	CombineTextureBuckets();

	for (size_t nBucket = 0; nBucket < _textureBuckets.Size(); nBucket++)
	{
		TextureBucket *pMyBucket = _textureBuckets[nBucket];
		
		verify(pCached->AddBucket(
			pMyBucket->_textures.Data(),       pMyBucket->_textures.Size(),
			pMyBucket->_sprites->Data(),      pMyBucket->_sprites->Size(),
			pMyBucket->_multiSprites->Data(), pMyBucket->_multiSprites->Size()));
	}

	ClearSpriteArrays();
	_spriteDepth = pCached->GetZOffset();

	*ppCached = pCached.Detach();

	return true;
}

void ff::Renderer2d::DrawPoints(const PointFloat *pPoints, const XMFLOAT4 *pColor, size_t nCount)
{
	assertRet(pPoints);

	if (nCount)
	{
		LineArtVertex lv;
		lv.pos.z = _spriteDepth;

		for (size_t i = 0; i < nCount; i++, pPoints++)
		{
			lv.color = pColor ? pColor[i] : GetColorWhite();
			lv.pos.x = pPoints->x;
			lv.pos.y = -pPoints->y;

			_points.Push(&lv, 1);
		}
	}
}

void ff::Renderer2d::DrawPolyLine(const PointFloat *pPoints, size_t nCount, const XMFLOAT4 *pColor)
{
	assertRet(pPoints);

	if (nCount == 1)
	{
		DrawPoints(pPoints, pColor, nCount);
	}
	else if (nCount > 1)
	{
		for (size_t i = 1; i < nCount; i++, pPoints++)
		{
			DrawLine(pPoints, pPoints + 1, pColor);
		}
	}
}

void ff::Renderer2d::DrawClosedPolyLine(const PointFloat *pPoints, size_t nCount, const XMFLOAT4 *pColor)
{
	DrawPolyLine(pPoints, nCount, pColor);

	if (nCount >= 3)
	{
		DrawLine(&pPoints[nCount - 1], pPoints, pColor);
	}
}

void ff::Renderer2d::DrawLine(const PointFloat *pStart, const PointFloat *pEnd, const XMFLOAT4 *pColor)
{
	assertRet(pStart && pEnd);

	LineArtVertex lv;
	lv.color = pColor ? *pColor : GetColorWhite();
	lv.pos.z = _spriteDepth;

	lv.pos.x = pStart->x;
	lv.pos.y = -pStart->y;
	_lines.Push(&lv, 1);

	lv.pos.x = pEnd->x;
	lv.pos.y = -pEnd->y;
	_lines.Push(&lv, 1);
}

void ff::Renderer2d::DrawRectangle(const RectFloat *pRect, const XMFLOAT4 *pColor)
{
	assertRet(pRect);

	const PointFloat points[5] =
	{
		pRect->TopLeft(),
		pRect->TopRight(),
		pRect->BottomRight(),
		pRect->BottomLeft(),
		pRect->TopLeft(),
	};

	DrawPolyLine(points, _countof(points), pColor);
}

void ff::Renderer2d::DrawRectangle(const RectFloat *pRect, float thickness, const XMFLOAT4 *pColor)
{
	assertRet(pRect);

	RectFloat rect(*pRect);
	rect.Normalize();

	if (rect.Width() <= thickness || rect.Height() <= thickness)
	{
		rect.Deflate(-thickness, -thickness);
		DrawFilledRectangle(&rect, pColor, pColor ? 1 : 0);
	}
	else
	{
		size_t nColors  = pColor ? 1 : 0;
		float  oldDepth = _spriteDepth;
		float  ht       = thickness / 2;

		XMVECTOR thickVector = XMVectorSet(-ht, -ht, ht, ht);
		XMVECTOR top         = XMVectorAdd(XMVectorSet(rect.left,  rect.top,    rect.right, rect.top),    thickVector);
		XMVECTOR bottom      = XMVectorAdd(XMVectorSet(rect.left,  rect.bottom, rect.right, rect.bottom), thickVector);

		thickVector     = XMVectorSet(-ht, ht, ht, -ht);
		XMVECTOR left   = XMVectorAdd(XMVectorSet(rect.left,  rect.top,    rect.left,  rect.bottom), thickVector);
		XMVECTOR right  = XMVectorAdd(XMVectorSet(rect.right, rect.top,    rect.right, rect.bottom), thickVector);

		XMStoreFloat4((XMFLOAT4*)&rect, top);
		DrawFilledRectangle(&rect, pColor, nColors);

		_spriteDepth = oldDepth;
		XMStoreFloat4((XMFLOAT4*)&rect, bottom);
		DrawFilledRectangle(&rect, pColor, nColors);

		_spriteDepth = oldDepth;
		XMStoreFloat4((XMFLOAT4*)&rect, left);
		DrawFilledRectangle(&rect, pColor, nColors);

		_spriteDepth = oldDepth;
		XMStoreFloat4((XMFLOAT4*)&rect, right);
		DrawFilledRectangle(&rect, pColor, nColors);
	}
}

void ff::Renderer2d::DrawFilledRectangle(const RectFloat *pRect, const XMFLOAT4 *pColor, size_t nColorCount)
{
	assertRet(pRect);

	XMFLOAT4 colors[4];

	switch (nColorCount)
	{
	default:
		assert(false);
		__fallthrough;

	case 0:
		colors[0] = GetColorWhite();
		colors[1] = GetColorWhite();
		colors[2] = GetColorWhite();
		colors[3] = GetColorWhite();
		break;

	case 1:
		colors[0] = *pColor;
		colors[1] = *pColor;
		colors[2] = *pColor;
		colors[3] = *pColor;
		break;

	case 2:
		colors[0] = pColor[0];
		colors[1] = pColor[0];
		colors[2] = pColor[1];
		colors[3] = pColor[1];
		break;

	case 4:
		CopyMemory(colors, pColor, sizeof(colors));
		break;
	}

	LineArtVertex lv;
	lv.pos.z = _spriteDepth;

	lv.pos.x = pRect->left;
	lv.pos.y = -pRect->top;
	lv.color = colors[0];
	_rectangles.Push(&lv, 1);

	lv.pos.x = pRect->right;
	lv.color = colors[1];
	_rectangles.Push(&lv, 1);

	lv.pos.y = -pRect->bottom;
	lv.color = colors[2];
	_rectangles.Push(&lv, 1);

	lv.pos.x = pRect->left;
	lv.color = colors[3];
	_rectangles.Push(&lv, 1);

	NudgeDepth();
}

void ff::Renderer2d::DrawFilledTriangle(const PointFloat *pPoints, const XMFLOAT4 *pColor, size_t nColorCount)
{
	assertRet(pPoints);

	XMFLOAT4 colors[3];

	switch (nColorCount)
	{
	default:
		assert(false);
		__fallthrough;

	case 0:
		colors[0] = GetColorWhite();
		colors[1] = GetColorWhite();
		colors[2] = GetColorWhite();
		break;

	case 1:
		colors[0] = *pColor;
		colors[1] = *pColor;
		colors[2] = *pColor;
		break;

	case 3:
		CopyMemory(colors, pColor, sizeof(colors));
		break;
	}

	LineArtVertex lv;
	lv.pos.z = _spriteDepth;

	lv.pos.x = pPoints[0].x;
	lv.pos.y = -pPoints[0].y;
	lv.color = colors[0];
	_triangles.Push(&lv, 1);

	lv.pos.x = pPoints[1].x;
	lv.pos.y = -pPoints[1].y;
	lv.color = colors[1];
	_triangles.Push(&lv, 1);

	lv.pos.x = pPoints[2].x;
	lv.pos.y = -pPoints[2].y;
	lv.color = colors[2];
	_triangles.Push(&lv, 1);

	NudgeDepth();
}

XMMATRIX &ff::Renderer2d::GetMatrix(MatrixType type)
{
	return _matrixStack[type].GetLast();
}

void ff::Renderer2d::SetMatrix(MatrixType type, const XMMATRIX *pMatrix)
{
	I2dEffect *pEffect = GetEffect();

	if (pEffect)
	{
		Flush();
		pEffect->OnMatrixChanging(this, type);
	}

	XMMATRIX &matrix = _matrixStack[type].GetLast();

	if (pMatrix)
	{
		matrix = *pMatrix;
	}
	else
	{
		matrix = XMMatrixIdentity();
	}

	if (pEffect)
	{
		pEffect->OnMatrixChanged(this, type);
	}
}

void ff::Renderer2d::TransformMatrix(MatrixType type, const XMMATRIX *pMatrix)
{
	if (pMatrix && _matrixStack[type].Size())
	{
		I2dEffect *pEffect = GetEffect();

		if (pEffect)
		{
			Flush();
			pEffect->OnMatrixChanging(this, type);
		}

		XMMATRIX &matrix = _matrixStack[type].GetLast();
		matrix = *pMatrix * matrix;

		if (pEffect)
		{
			pEffect->OnMatrixChanged(this, type);
		}
	}
}

void ff::Renderer2d::PushMatrix(MatrixType type)
{
	XMMATRIX matrix;

	if (_matrixStack[type].Size())
	{
		matrix = _matrixStack[type].GetLast();
	}
	else
	{
		matrix = XMMatrixIdentity();
	}

	_matrixStack[type].Push(matrix);
}

void ff::Renderer2d::PopMatrix(MatrixType type)
{
	assertRet(_matrixStack[type].Size() > 1);

	I2dEffect *pEffect = GetEffect();

	if (pEffect)
	{
		Flush();
		pEffect->OnMatrixChanging(this, type);
	}

	_matrixStack[type].Pop();

	if (pEffect)
	{
		pEffect->OnMatrixChanged(this, type);
	}
}

void ff::Renderer2d::FlushSprites()
{
	if (!_textureBuckets.Size())
	{
		return;
	}

	CombineTextureBuckets();

	I2dEffect *pEffect = GetEffect();

	for (size_t nBucket = 0; nBucket < _textureBuckets.Size(); nBucket++)
	{
		TextureBucket*       pBucket       = _textureBuckets[nBucket];
		SpriteVertexes*      pSprites      = pBucket->_sprites;
		MultiSpriteVertexes* pMultiSprites = pBucket->_multiSprites;

		pEffect->ApplyTextures(this, pBucket->_textures.Data(), pBucket->_textures.Size());

		if (pSprites && pSprites->Size())
		{
			for (size_t i = 0, nCount = 0; i < pSprites->Size(); i += nCount)
			{
				nCount = std::min(
					pSprites->Size() - i,   // the number of vertexes left for this bucket
					SPRITE_CHUNK_SIZE * 4); // the number of vertex slots left for drawing

				AutoBufferMap<SpriteVertex> vertexBuffer(_device->GetVertexBuffers(), nCount);

				if (vertexBuffer.GetMem())
				{
					CopyMemory(vertexBuffer.GetMem(), pSprites->Data(i), vertexBuffer.GetSize());

					if (pEffect->Apply(this, DRAW_TYPE_SPRITE, vertexBuffer.Unmap(), _spriteIndexes, 0))
					{
						_device->GetContext()->DrawIndexed((UINT)nCount * 6 / 4, 0, 0);
					}
				}
			}
		}

		if (pMultiSprites && pMultiSprites->Size())
		{
			for (size_t i = 0, nCount = 0; i < pMultiSprites->Size(); i += nCount)
			{
				nCount = std::min(
					pMultiSprites->Size() - i,    // the number of vertexes left for this bucket
					MULTI_SPRITE_CHUNK_SIZE * 4); // the number of vertex slots left for drawing

				AutoBufferMap<MultiSpriteVertex> vertexBuffer(_device->GetVertexBuffers(), nCount);

				if (vertexBuffer.GetMem())
				{
					CopyMemory(vertexBuffer.GetMem(), pMultiSprites->Data(i), vertexBuffer.GetSize());

					if (pEffect->Apply(this, DRAW_TYPE_MULTI_SPRITE, vertexBuffer.Unmap(), _spriteIndexes, 0))
					{
						_device->GetContext()->DrawIndexed((UINT)nCount * 6 / 4, 0, 0);
					}
				}
			}
		}
	}

	ClearSpriteArrays();
}

void ff::Renderer2d::FlushRectangles()
{
	if (_rectangles.IsEmpty())
	{
		return;
	}

	for (size_t i = 0, nCount = 0; i < _rectangles.Size(); i += nCount)
	{
		nCount = std::min(_rectangles.Size() - i, SPRITE_CHUNK_SIZE * 4);
		assert(nCount % 4 == 0);

		AutoBufferMap<LineArtVertex> vertexBuffer(_device->GetVertexBuffers(), nCount);

		if (vertexBuffer.GetMem())
		{
			CopyMemory(vertexBuffer.GetMem(), &_rectangles[i], sizeof(LineArtVertex) * nCount);

			if (GetEffect()->Apply(this, DRAW_TYPE_RECTS, vertexBuffer.Unmap(), _spriteIndexes, 0))
			{
				_device->GetContext()->DrawIndexed((UINT)nCount * 6 / 4, 0, 0);
			}
		}
	}

	_rectangles.Clear();
}

void ff::Renderer2d::FlushTriangles()
{
	if (_triangles.IsEmpty())
	{
		return;
	}

	for (size_t i = 0, nCount = 0; i < _triangles.Size(); i += nCount)
	{
		nCount = std::min(_triangles.Size() - i, SPRITE_CHUNK_SIZE * 3);
		assert(nCount % 3 == 0);

		AutoBufferMap<LineArtVertex> vertexBuffer(_device->GetVertexBuffers(), nCount);

		if (vertexBuffer.GetMem())
		{
			CopyMemory(vertexBuffer.GetMem(), &_triangles[i], sizeof(LineArtVertex) * nCount);

			if (GetEffect()->Apply(this, DRAW_TYPE_RECTS, vertexBuffer.Unmap(), _triangleIndexes, 0))
			{
				_device->GetContext()->DrawIndexed((UINT)nCount, 0, 0);
			}
		}
	}

	_triangles.Clear();
}

void ff::Renderer2d::FlushLines()
{
	if (_lines.IsEmpty())
	{
		return;
	}

	for (size_t i = 0; i < _lines.Size(); i += SPRITE_CHUNK_SIZE)
	{
		size_t nVertexCount = std::min(_lines.Size() - i, SPRITE_CHUNK_SIZE);
		assert(nVertexCount % 2 == 0);
		
		AutoBufferMap<LineArtVertex> vertexBuffer(_device->GetVertexBuffers(), nVertexCount);

		if (vertexBuffer.GetMem())
		{
			CopyMemory(vertexBuffer.GetMem(), &_lines[i], sizeof(LineArtVertex) * nVertexCount);

			if (GetEffect()->Apply(this, DRAW_TYPE_LINES, vertexBuffer.Unmap(), nullptr, 0))
			{
				_device->GetContext()->Draw((UINT)nVertexCount, 0);
			}
		}
	}

	_lines.Clear();

	NudgeDepth();
}

void ff::Renderer2d::FlushPoints()
{
	if (_points.IsEmpty())
	{
		return;
	}

	for (size_t i = 0; i < _points.Size(); i += SPRITE_CHUNK_SIZE)
	{
		size_t nVertexCount = std::min(_points.Size() - i, SPRITE_CHUNK_SIZE);
		assert(nVertexCount % 4 == 0);

		AutoBufferMap<LineArtVertex> vertexBuffer(_device->GetVertexBuffers(), nVertexCount);

		if (vertexBuffer.GetMem())
		{
			CopyMemory(vertexBuffer.GetMem(), &_points[i], sizeof(LineArtVertex) * nVertexCount);

			if (GetEffect()->Apply(this, DRAW_TYPE_POINTS, vertexBuffer.Unmap(), nullptr, 0))
			{
				_device->GetContext()->Draw((UINT)nVertexCount, 0);
			}
		}
	}

	_points.Clear();

	NudgeDepth();
}

void ff::Renderer2d::ClearSpriteArrays()
{
	for (size_t i = 0; i < _textureBuckets.Size(); i++)
	{
		DeleteTextureBucket(_textureBuckets[i]);
	}

	_textureBuckets.Clear();
	_textureToBuckets.Clear();

	ZeroObject(_lastBuckets);
}

void ff::Renderer2d::DeleteSpriteArrays()
{
	ClearSpriteArrays();

	for (size_t i = 0; i < _freeSprites.Size(); i++)
	{
		delete _freeSprites[i];
	}

	for (size_t i = 0; i < _freeMultiSprites.Size(); i++)
	{
		delete _freeMultiSprites[i];
	}

	_freeSprites.Clear();
	_freeMultiSprites.Clear();
}

void ff::Renderer2d::CacheTextureBucket(TextureBucket *pBucket)
{
	for (size_t i = 1; i < _countof(_lastBuckets); i++)
	{
		_lastBuckets[i] = _lastBuckets[i - 1];
	}

	_lastBuckets[0] = pBucket;
}

ff::TextureBucket *ff::Renderer2d::GetTextureBucket(const ComPtr<IGraphTexture> &pTexture, size_t &nIndex)
{
	// Check the MRU cache first

	for (size_t i = 0; i < _countof(_lastBuckets); i++)
	{
		if (_lastBuckets[i] && _lastBuckets[i]->IsSupported(pTexture, nIndex))
		{
			return _lastBuckets[i];
		}
	}

	// Check the texture-to-bucket map

	BucketIter iter = _textureToBuckets.Get(pTexture);

	if (iter == INVALID_ITER)
	{
		// Can't reuse a bucket, make a new one

		TextureBucket *pBucket = NewTextureBucket();
		pBucket->AddSupport(pTexture, nIndex);

		iter = _textureToBuckets.Insert(pTexture, pBucket);
	}

	TextureBucket *pBucket = _textureToBuckets.ValueAt(iter);
	CacheTextureBucket(pBucket);
	verify(pBucket->IsSupported(pTexture, nIndex));

	return pBucket;
}

ff::TextureBucket *ff::Renderer2d::GetTextureBucket(const MultiSpriteTextures &mst, size_t indexes[MultiSpriteTextures::MAX_TEXTURES])
{
	// Check the MRU cache first

	for (size_t i = 0; i < _countof(_lastBuckets); i++)
	{
		if (_lastBuckets[i] && _lastBuckets[i]->IsSupported(mst, indexes))
		{
			return _lastBuckets[i];
		}
	}

	// Find a supported bucket for the list of textures

	for (size_t i = 0; i < mst._textures.Size(); i++)
	{
		ComPtr<IGraphTexture> pTexture = mst._textures[i];

		for (BucketIter iter = _textureToBuckets.Get(pTexture); iter != INVALID_ITER; iter = _textureToBuckets.GetNext(iter))
		{
			TextureBucket *pBucket = _textureToBuckets.ValueAt(iter);

			if (i == 0 && pBucket->IsSupported(mst, indexes))
			{
				CacheTextureBucket(pBucket);
				return pBucket;
			}

			if (pBucket->AddSupport(mst, indexes))
			{
				MapTexturesToBucket(mst, pBucket);
				CacheTextureBucket(pBucket);
				return pBucket;
			}
		}
	}

	// No buckets can support the textures, make a new one
	{
		TextureBucket *pBucket = NewTextureBucket();
		pBucket->AddSupport(mst, indexes);

		MapTexturesToBucket(mst, pBucket);
		CacheTextureBucket(pBucket);

		return pBucket;
	}
}

void ff::Renderer2d::MapTexturesToBucket(const MultiSpriteTextures &mst, TextureBucket *pBucket)
{
	// Make sure that each texture in "mst" maps to "pBucket"

	ComPtr<IGraphTexture> pPrevTexture;

	for (size_t i = 0; i < mst._textures.Size(); i++)
	{
		ComPtr<IGraphTexture> pTexture = mst._textures[i];

		if (pPrevTexture != pTexture)
		{
			pPrevTexture = pTexture;

			bool bFound = false;

			for (BucketIter iter = _textureToBuckets.Get(pTexture); iter != INVALID_ITER; iter = _textureToBuckets.GetNext(iter))
			{
				if (_textureToBuckets.ValueAt(iter) == pBucket)
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				_textureToBuckets.Insert(pTexture, pBucket);
			}
		}
	}
}

// After calling this function, DO NOT use the _textureToBuckets map anymore
// since the referenced buckets could be deleted.
// It's not worth the work to ensure that _textureToBuckets is still valid.
void ff::Renderer2d::CombineTextureBuckets()
{
	for (size_t i = 0; i + 1 < _textureBuckets.Size(); i++)
	{
		TextureBucket* pBucket = _textureBuckets[i];
		size_t         nCount  = pBucket->_textures.Size();
		size_t         nUnused = pBucket->_maxTextures - nCount;

		// Look for another bucket to fill the unused textures slots

		for (size_t h = i + 1; nUnused && h < _textureBuckets.Size();)
		{
			TextureBucket *pOtherBucket = _textureBuckets[h];

			if (pOtherBucket->_textures.Size() > nUnused)
			{
				h++;
				continue;
			}

			// Fix up the texture index for the vertexes before copying them over
			float                fOffset       = (float)nCount;
			SpriteVertexes*      pSprites      = pOtherBucket->_sprites;
			MultiSpriteVertexes* pMultiSprites = pOtherBucket->_multiSprites;

			if (pSprites->Size())
			{
				for (size_t j = 0; j < pSprites->Size(); j++)
				{
					SpriteVertex &sv = pSprites->GetAt(j);
					if (sv.ntex != -1) sv.ntex += fOffset;
				}

				pBucket->_sprites->Push(pSprites->Data(), pSprites->Size());
			}

			if (pMultiSprites->Size())
			{
				for (size_t j = 0; j < pMultiSprites->Size(); j++)
				{
					MultiSpriteVertex &msv = pMultiSprites->GetAt(j);

					if (msv.ntex.x != -1) msv.ntex.x += fOffset;
					if (msv.ntex.y != -1) msv.ntex.y += fOffset;
					if (msv.ntex.z != -1) msv.ntex.z += fOffset;
					if (msv.ntex.w != -1) msv.ntex.w += fOffset;
				}

				pBucket->_multiSprites->Push(pMultiSprites->Data(), pMultiSprites->Size());
			}

			pBucket->_textures.Push(pOtherBucket->_textures.Data(), pOtherBucket->_textures.Size());
			nCount  = pBucket->_textures.Size();
			nUnused = pBucket->_maxTextures - nCount;

			DeleteTextureBucket(pOtherBucket);
			_textureBuckets.Delete(h);
		}
	}
}

ff::TextureBucket *ff::Renderer2d::NewTextureBucket()
{
	TextureBucket *pObj = _textureBucketPool.New();
	_textureBuckets.Push(pObj);

	pObj->_sprites      = NewSpriteVertexes();
	pObj->_multiSprites = NewMultiSpriteVertexes();
	pObj->_maxTextures  = _level9 ? 1 : 8;

	return pObj;
}

ff::SpriteVertexes *ff::Renderer2d::NewSpriteVertexes()
{
	return _freeSprites.Size()
		? _freeSprites.Pop()
		: new SpriteVertexes;
}

ff::MultiSpriteVertexes *ff::Renderer2d::NewMultiSpriteVertexes()
{
	return _freeMultiSprites.Size()
		? _freeMultiSprites.Pop()
		: new MultiSpriteVertexes;
}

void ff::Renderer2d::DeleteTextureBucket(TextureBucket *pObj)
{
	if (pObj)
	{
		DeleteSpriteVertexes(pObj->_sprites);
		DeleteMultiSpriteVertexes(pObj->_multiSprites);

		_textureBucketPool.Delete(pObj);
	}
}

void ff::Renderer2d::DeleteSpriteVertexes(SpriteVertexes *pObj)
{
	if (pObj)
	{
		pObj->Clear();
		_freeSprites.Push(pObj);
	}
}

void ff::Renderer2d::DeleteMultiSpriteVertexes(MultiSpriteVertexes *pObj)
{
	if (pObj)
	{
		pObj->Clear();
		_freeMultiSprites.Push(pObj);
	}
}
