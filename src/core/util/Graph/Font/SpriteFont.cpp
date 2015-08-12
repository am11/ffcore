#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/DataPersist.h"
#include "Graph/2D/2dEffect.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteList.h"
#include "Graph/2D/SpriteOptimizer.h"
#include "Graph/Anim/AnimPos.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/Font/FontData.h"
#include "Graph/Font/SpriteFont.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Module/ModuleFactory.h"
#include "Windows/Handles.h"

#if METRO_APP
typedef struct tagKERNINGPAIR
{
   WORD wFirst;
   WORD wSecond;
   int iKernAmount;
} KERNINGPAIR, *LPKERNINGPAIR;
#endif

template<>
struct std::less<KERNINGPAIR>
{
	bool operator()(const KERNINGPAIR &lhs, const KERNINGPAIR &rhs)
	{
		return (lhs.wFirst < rhs.wFirst) || (lhs.wFirst == rhs.wFirst && lhs.wSecond < rhs.wSecond);
	}
};

namespace ff
{
	class ISpriteList;

	class __declspec(uuid("c87f399b-5a75-4e0a-8b80-cebc58e2eaa2"))
		SpriteFont : public ComBase, public ISpriteFont
	{
	public:
		DECLARE_HEADER(SpriteFont);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		bool Init(IFontData *pData, const LOGFONT &logFont);
		ISpriteList *GetGlyphSpriteList() const;

		// IGraphDeviceChild functions
		virtual IGraphDevice *GetDevice() const override;
		virtual void Reset() override;

		// ISpriteFont functions
		virtual PointFloat DrawText(
			I2dRenderer *pRender,
			StringRef text,
			PointFloat pos,
			PointFloat scale,
			PointFloat spacing,
			const XMFLOAT4 *pColor) override;

		virtual PointFloat MeasureText(StringRef text, PointFloat scale, PointFloat spacing) override;

	private:
	#if !METRO_APP
		bool InitGlyphs();
	#endif
		bool InitSprites();

		PointFloat InternalDrawText(
			I2dRenderer *pRender,
			StringRef text,
			PointFloat pos,
			PointFloat scale,
			PointFloat spacing,
			const XMFLOAT4 *pColor,
			RectFloat *pBox);

		struct SGlyphData
		{
			ISprite *_sprite;
			PointFloat _offset;
			bool _kerning;
		};

		struct SSavedGlyphData
		{
			PointFloat _offset;
			bool _kerning;
			bool _hasSprite;
		};

		ComPtr<IGraphDevice> _device;

		LOGFONT _logFont;
		TEXTMETRIC _textMetric;
		float _lineGap;
		bool _loadedSprites;
		bool _spritesValid;

		ComPtr<ISpriteList> _glyphSprites;
		Vector<SGlyphData> _glyphData;
		Vector<KERNINGPAIR> _kerningPairs;
		WORD _charToGlyphData[0x10000]; // 0x0000 - 0xFFFF

		static const WORD s_invalidGlyph = 0xFFFF;
		static ISprite *s_pendingSprite;
	};
}

ff::ISprite *ff::SpriteFont::s_pendingSprite = (ff::ISprite *)1;

BEGIN_INTERFACES(ff::SpriteFont)
	HAS_INTERFACE(ff::ISpriteFont)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"Sprite Font");
	module.RegisterClassT<ff::SpriteFont>(name, __uuidof(ff::ISpriteFont), ff::GetCategoryGraphicsObject());
});

bool ff::CreateSpriteFont(IGraphDevice *pDevice, const LOGFONT &logFont, ISpriteFont **ppFont)
{
	return CreateSpriteFont(nullptr, nullptr, logFont, ppFont);
}

bool ff::CreateSpriteFont(IGraphDevice *pDevice, IFontData *pData, const LOGFONT &logFont, ISpriteFont **ppFont)
{
	assertRetVal(ppFont, false);
	*ppFont = nullptr;

	ComPtr<SpriteFont, ISpriteFont> pFont;
	assertRetVal(SUCCEEDED(ComAllocator<SpriteFont>::CreateInstance(pDevice, &pFont)), false);
	assertRetVal(pFont->Init(pData, logFont), false);

	*ppFont = pFont.Detach();
	return true;
}

bool ff::CreateSpriteFromText(
	I2dRenderer *pRender,
	I2dEffect *pEffect,
	ISpriteFont *pFont,
	StringRef text,
	ISprite **ppSprite)
{
	assertRetVal(pRender && pEffect && pFont && ppSprite, false);

	PointFloat textSize = pFont->MeasureText(text, PointFloat(1, 1), PointFloat(0, 0));
	PointInt size((int)ceil(textSize.x), (int)ceil(textSize.y));

	return CreateSpriteFromText(pRender, pEffect, pFont, text, size, PointFloat(1, 1), PointFloat(0, 0), PointFloat(0, 0), false, ppSprite);
}

bool ff::CreateSpriteFromText(
	I2dRenderer *pRender,
	I2dEffect *pEffect,
	ISpriteFont *pFont,
	StringRef text,
	PointInt size,
	PointFloat scale,
	PointFloat spacing,
	PointFloat offset,
	bool bCenter,
	ISprite **ppSprite)
{
	assertRetVal(pRender && pEffect && pFont && size.x && size.y && ppSprite, false);

	IGraphDevice *pDevice = pRender->GetDevice();
	ComPtr<IGraphTexture> pTexture;
	ComPtr<ISprite> pSprite;
	ComPtr<IRenderTarget> pTarget;
	PointFloat pos = offset;

	if (bCenter)
	{
		PointFloat textSize = pFont->MeasureText(text, scale, spacing);
		pos += PointFloat(size.x - textSize.x, size.y - textSize.y) / 2;
	}

	assertRetVal(CreateGraphTexture(pDevice, size, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 0, &pTexture), false);
	assertRetVal(CreateSprite(pTexture, &pSprite), false);
	assertRetVal(CreateRenderTargetTexture(pDevice, pTexture, 0, 1, 0, &pTarget), false);

	assertRetVal(pRender->BeginRender(
		pTarget,
		nullptr,
		RectFloat(0, 0, (float)size.x, (float)size.y),
		PointFloat(0, 0),
		PointFloat(1, 1),
		pEffect), false);

	pEffect->PushDrawType((DrawType2d)(DRAW_BLEND_COPY_ALL | DRAW_DEPTH_DISABLE));

	pTarget->Clear(&GetColorNone());
	pFont->DrawText(pRender, text, pos, scale, spacing, nullptr);

	pRender->Flush();
	pEffect->PopDrawType();
	pRender->EndRender();

	*ppSprite = pSprite.Detach();
	return *ppSprite != nullptr;
}

ff::SpriteFont::SpriteFont()
	: _lineGap(0)
	, _loadedSprites(false)
	, _spritesValid(false)
{
	ZeroObject(_logFont);
	ZeroObject(_textMetric);

	wmemset((LPWSTR)_charToGlyphData, s_invalidGlyph, _countof(_charToGlyphData));
}

ff::SpriteFont::~SpriteFont()
{
}

HRESULT ff::SpriteFont::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);

	return __super::_Construct(unkOuter);
}

bool ff::SpriteFont::Init(IFontData *pData, const LOGFONT &logFont)
{
	_logFont = logFont;

#if METRO_APP
	assertRetVal(false, false);
#else
	assertRetVal(InitGlyphs(), false);
	return true;
#endif
}

ff::ISpriteList *ff::SpriteFont::GetGlyphSpriteList() const
{
	return _glyphSprites;
}

ff::IGraphDevice *ff::SpriteFont::GetDevice() const
{
	return _device;
}

void ff::SpriteFont::Reset()
{
	// nothing to reset
}

static int CompareKerningPairs(const KERNINGPAIR &pair1, const KERNINGPAIR &pair2)
{
	if (pair1.wFirst < pair2.wFirst)
	{
		return -1;
	}
	else if (pair1.wFirst > pair2.wFirst)
	{
		return 1;
	}
	else if (pair1.wSecond < pair2.wSecond)
	{
		return -1;
	}
	else if (pair1.wSecond > pair2.wSecond)
	{
		return 1;
	}

	return 0;
}

static int CompareKerningPairs(const void *p1, const void *p2)
{
	const KERNINGPAIR &pair1 = *(const KERNINGPAIR *)p1;
	const KERNINGPAIR &pair2 = *(const KERNINGPAIR *)p2;

	return CompareKerningPairs(pair1, pair2);
}

#if !METRO_APP
bool ff::SpriteFont::InitGlyphs()
{
	// Initialize GDI's HFONT

	FontHandle	hFont = CreateFontIndirect(&_logFont);
	assertRetVal(hFont, false);

	HDC hdcDesktop = GetDC(nullptr);
	assertRetVal(hdcDesktop, false);

	CreateDcHandle hdc     = CreateCompatibleDC(hdcDesktop);
	BitmapHandle   hBitmap = CreateCompatibleBitmap(hdcDesktop, 1, 1);
	SelectGdiObject<HBITMAP> selectBitmap(hdc, hBitmap);
	SelectGdiObject<HFONT>   selectFont(hdc, hFont);

	ReleaseDC(nullptr, hdcDesktop);
	hdcDesktop = nullptr;

	// Get HFONT metrics

	assertRetVal(GetTextMetrics(hdc, &_textMetric), false);

	DWORD nOutlineMetricSize = GetOutlineTextMetrics(hdc, 0, nullptr);
	Vector<BYTE> outlineTextMetricData;
	OUTLINETEXTMETRIC *pOutlineTextMetric = nullptr;

	if (nOutlineMetricSize)
	{
		// TrueType font
		outlineTextMetricData.Resize(nOutlineMetricSize);
		pOutlineTextMetric = (OUTLINETEXTMETRIC*)outlineTextMetricData.Data();
		assertRetVal(GetOutlineTextMetrics(hdc, nOutlineMetricSize, pOutlineTextMetric), false);
		_lineGap = (float)pOutlineTextMetric->otmLineGap;
	}

	// Get kerning pairs (sorted)

	DWORD nKerningPairSize = GetKerningPairs(hdc, 0, nullptr);

	if (nKerningPairSize)
	{
		_kerningPairs.Resize(nKerningPairSize);
		assertRetVal(GetKerningPairs(hdc, nKerningPairSize, _kerningPairs.Data()) == nKerningPairSize, false);
		qsort(_kerningPairs.Data(), _kerningPairs.Size(), sizeof(KERNINGPAIR), CompareKerningPairs);
	}

	// Get glyph set (Unicode ranges)

	DWORD nGlyphSetSize = GetFontUnicodeRanges(hdc, nullptr);
	Vector<BYTE> glyphSetData;
	Set<size_t>  glyphSet;
	GLYPHSET*    pGlyphSet = nullptr;
	size_t       nGlyphsSupported = 0;

	if (nGlyphSetSize)
	{
		glyphSetData.Resize(nGlyphSetSize);
		pGlyphSet = (GLYPHSET*)glyphSetData.Data();

		assertRetVal(GetFontUnicodeRanges(hdc, pGlyphSet) == nGlyphSetSize, false);

		nGlyphsSupported = pGlyphSet->cGlyphsSupported;
		_glyphData.Reserve(nGlyphsSupported);
	}

	// Create the glyph-to-data map
	Vector<WORD> glyphToData;
	glyphToData.Resize(0x10000);
	wmemset((LPWSTR)glyphToData.Data(), s_invalidGlyph, glyphToData.Size());

	// Get character-to-glyph map
	Vector<WORD> charToGlyph;
	charToGlyph.Resize(0x10000);
	wmemset((LPWSTR)charToGlyph.Data(), s_invalidGlyph, charToGlyph.Size());
	{
		Vector<wchar_t> allChars;

		for (DWORD i = 0; i < pGlyphSet->cRanges; i++)
		{
			allChars.Resize(pGlyphSet->ranges[i].cGlyphs);

			for (size_t h = 0; h < allChars.Size(); h++)
			{
				allChars[h] = pGlyphSet->ranges[i].wcLow + (wchar_t)h;
			}

			assertRetVal(GetGlyphIndices(hdc, allChars.Data(),
				(int)allChars.Size(), &charToGlyph[pGlyphSet->ranges[i].wcLow],
				GGI_MARK_NONEXISTING_GLYPHS) == allChars.Size(), false);
		}
	}

	for (size_t i = 0; i < charToGlyph.Size(); i++)
	{
		if (charToGlyph[i] == s_invalidGlyph)
		{
			charToGlyph[i] = (charToGlyph[_textMetric.tmDefaultChar] != s_invalidGlyph)
				? charToGlyph[_textMetric.tmDefaultChar]
				: charToGlyph['?'];
		}

		assertRetVal(charToGlyph[i] != s_invalidGlyph, false);

		glyphSet.SetKey(charToGlyph[i]);
	}

	// not sure why these aren't ever equal
	assertRetVal(glyphSet.Size() <= nGlyphsSupported, false);

	// Init transform matrix

	MAT2 mat;
	ZeroObject(mat);
	mat.eM11.value = 1;
	mat.eM22.value = 1;

	// Create temp sprite list (unoptimized)

	ComPtr<ISpriteList> pGlyphSprites;
	assertRetVal(CreateSpriteList(_device, &pGlyphSprites), false);

	// Create an initial large staging texture for placing glyphs

	PointInt stagingTextureSize(1024, 1024);
	PointInt stagingPos(0, 0);
	int    stagingRowHeight = 0;
	ComPtr<IGraphTexture> pStagingTexture;
	assertRetVal(CreateStagingTexture(_device, stagingTextureSize, DXGI_FORMAT_R8G8B8A8_UNORM, false, true, &pStagingTexture), false);

	// Map the staging texture to memory, DO NOT RETURN before it gets unmapped

	D3D11_MAPPED_SUBRESOURCE mapStaging;
	assertRetVal(SUCCEEDED(_device->GetContext()->Map(pStagingTexture->GetTexture(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapStaging)), false);
	ZeroMemory(mapStaging.pData, mapStaging.RowPitch * stagingTextureSize.y);

	// Create a sprite for each glyph

	Vector<BYTE> glyphBytes;

	for (BucketIter iter = glyphSet.StartIteration(); iter != INVALID_ITER; iter = glyphSet.Iterate(iter))
	{
		size_t nGlyph  = glyphSet.KeyAt(iter);
		wchar_t  chGlyph = (wchar_t)(nGlyph & 0xFFFF);
		wchar_t  name[2] = { chGlyph, 0 };

		SGlyphData glyphData;
		ZeroObject(glyphData);

		GLYPHMETRICS gm;
		ZeroObject(gm);

		DWORD nGlyphBytes = GetGlyphOutline(hdc, chGlyph, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &gm, 0, nullptr, &mat);

		// ABC abc;
		// assertRetVal(GetCharABCWidthsI(hdc, chGlyph, 1, nullptr, &abc), false);

		if (nGlyphBytes && nGlyphBytes != GDI_ERROR)
		{
			glyphBytes.Resize(nGlyphBytes);
			GetGlyphOutline(hdc, chGlyph, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &gm, nGlyphBytes, glyphBytes.Data(), &mat);
			PointInt blackBox((int)gm.gmBlackBoxX, (int)gm.gmBlackBoxY);

			if (stagingPos.x + blackBox.x > stagingTextureSize.x)
			{
				// Move down to the next row

				stagingPos.x = 0;
				stagingPos.y += stagingRowHeight + 1;
				stagingRowHeight = 0;
			}

			if (stagingPos.y + blackBox.y > stagingTextureSize.y)
			{
				// Filled up this texture, make a new one

				stagingPos.SetPoint(0, 0);
				stagingRowHeight = 0;

				_device->GetContext()->Unmap(pStagingTexture->GetTexture(), 0);
				pStagingTexture = nullptr;

				assertRetVal(CreateStagingTexture(_device, stagingTextureSize, DXGI_FORMAT_R8G8B8A8_UNORM, false, true, &pStagingTexture), false);
				assertRetVal(SUCCEEDED(_device->GetContext()->Map(pStagingTexture->GetTexture(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapStaging)), false);
				ZeroMemory(mapStaging.pData, mapStaging.RowPitch * stagingTextureSize.y);
			}

			// Copy bits to the texture
			size_t nByteWidth = (blackBox.x + 3) / 4 * 4;

			for (int y = 0; y < blackBox.y; y++)
			{
				LPBYTE pAlpha = &glyphBytes[y * nByteWidth];
				LPBYTE pData  = (LPBYTE)mapStaging.pData + (stagingPos.y + y) * mapStaging.RowPitch + stagingPos.x * 4;

				for (int x = 0; x < blackBox.x; x++)
				{
					pData[x * 4 + 0] = 255;
					pData[x * 4 + 1] = 255;
					pData[x * 4 + 2] = 255;
					pData[x * 4 + 3] = (BYTE)((((DWORD)pAlpha[x] * 255) / 64) & 0xFF);
				}
			}

			// Add sprite to staging texture

			PointFloat spriteHandle((float)-gm.gmptGlyphOrigin.x, (float)(gm.gmptGlyphOrigin.y - _textMetric.tmAscent));
			RectInt  spriteRect(stagingPos.x, stagingPos.y, stagingPos.x + blackBox.x, stagingPos.y + blackBox.y);

			glyphData._sprite = pGlyphSprites->Add(pStagingTexture, String(name), spriteRect, spriteHandle, PointFloat(1, 1));

			stagingPos.x += blackBox.x + 1;
			stagingRowHeight = std::max(stagingRowHeight, blackBox.y);
		}

		glyphData._offset.SetPoint((float)gm.gmCellIncX, (float)gm.gmCellIncY);
		glyphToData[nGlyph] = (WORD)(_glyphData.Size() & 0xFFFF);
		_glyphData.Push(glyphData);
	}

	_device->GetContext()->Unmap(pStagingTexture->GetTexture(), 0);
	pStagingTexture = nullptr;

	// Create optimized sprite list
	assertRetVal(OptimizeSprites(pGlyphSprites, DXGI_FORMAT_BC2_UNORM, 3, &_glyphSprites), false);

	// Get the final sprites for each glyph

	for (size_t i = 0, nSprite = 0; i < _glyphData.Size(); i++)
	{
		if (_glyphData[i]._sprite)
		{
			_glyphData[i]._sprite = _glyphSprites->Get(nSprite++);
			assertRetVal(_glyphData[i]._sprite, false);
		}
	}

	// Map chars to glyph data

	for (size_t i = 0; i < _countof(_charToGlyphData); i++)
	{
		_charToGlyphData[i] = glyphToData[charToGlyph[i]];
		assertRetVal(_charToGlyphData[i] < _glyphData.Size(), false);

		SGlyphData &glyphData = _glyphData[_charToGlyphData[i]];

		if (!glyphData._kerning && _kerningPairs.Size())
		{
			// Find the first kerning pair for this character/glyph

			KERNINGPAIR kp;
			ZeroObject(kp);
			kp.wFirst = (wchar_t)(i & 0xFF);

			size_t nFirst = 0;
			_kerningPairs.SortFind(kp, &nFirst);

			if (nFirst < _kerningPairs.Size() && _kerningPairs[nFirst].wFirst == kp.wFirst)
			{
				glyphData._kerning = true;
			}
		}
	}

	_loadedSprites = true;
	_spritesValid  = true;

	return true;
}
#endif

bool ff::SpriteFont::InitSprites()
{
	if (!_loadedSprites && _glyphSprites)
	{
		size_t nSprite = 0;

		for (size_t i = 0; i < _glyphData.Size(); i++)
		{
			if (_glyphData[i]._sprite)
			{
				if (_glyphData[i]._sprite == s_pendingSprite)
				{
					_glyphData[i]._sprite = _glyphSprites->Get(nSprite++);
				}
				else
				{
					break;
				}
			}
		}

		_loadedSprites = true;
		_spritesValid  = (nSprite == _glyphSprites->GetCount());

		assert(_spritesValid);
	}

	return _spritesValid;
}

ff::PointFloat ff::SpriteFont::InternalDrawText(
	I2dRenderer *pRender,
	StringRef text,
	PointFloat pos,
	PointFloat scale,
	PointFloat spacing,
	const XMFLOAT4 *pColor,
	RectFloat *pBox)
{
	PointFloat curPos = pos;

	if (pBox)
	{
		pBox->SetRect(pos, pos);
	}

	if (text.size() && InitSprites())
	{
		size_t nLen = text.size();
		float lineSpacing = (_lineGap + _textMetric.tmHeight + spacing.y) * scale.y;
		PointFloat letterSpacing(spacing.x, 0);

		for (size_t i = 0; i < nLen; i++)
		{
			wchar_t ch = text[i];

			if (ch == '\r' || ch == '\n')
			{
				if (ch == '\r' && i + 1 < nLen && text[i + 1] == '\n')
				{
					i++;
				}

				curPos.x = pos.x;
				curPos.y += lineSpacing;
			}
			else
			{
				WORD nGlyph = _charToGlyphData[ch];
				const SGlyphData &data = _glyphData[nGlyph];

				if (data._sprite)
				{
					if (pRender)
					{
						pRender->DrawSprite(data._sprite, &curPos, &scale, 0, pColor);
					}

					if (pBox)
					{
						const RectFloat &spriteBox = data._sprite->GetData()._worldRect;

						pBox->right  = std::max(pBox->right,  curPos.x + spriteBox.right  * scale.x);
						pBox->bottom = std::max(pBox->bottom, curPos.y + spriteBox.bottom * scale.y);
					}
				}

				curPos += (data._offset + letterSpacing) * scale;

				// Extra character kerning

				if (data._kerning && i + 1 < nLen)
				{
					wchar_t ch2 = text[i + 1];

					KERNINGPAIR kp;
					kp.wFirst  = ch;
					kp.wSecond = ch2;

					size_t nKern = 0;
					if (_kerningPairs.SortFind(kp, &nKern))
					{
						curPos.x += scale.x * (_kerningPairs[nKern].iKernAmount + spacing.x);
					}
				}
			}
		}
	}

	return curPos;
}

ff::PointFloat ff::SpriteFont::DrawText(
	I2dRenderer *pRender,
	StringRef text,
	PointFloat pos,
	PointFloat scale,
	PointFloat spacing,
	const XMFLOAT4 *pColor)
{
	return InternalDrawText(pRender, text, pos, scale, spacing, pColor, nullptr);
}

ff::PointFloat ff::SpriteFont::MeasureText(StringRef text, PointFloat scale, PointFloat spacing)
{
	RectFloat box;
	InternalDrawText(nullptr, text, PointFloat(0, 0), scale, spacing, nullptr, &box);

	return box.Size();
}
