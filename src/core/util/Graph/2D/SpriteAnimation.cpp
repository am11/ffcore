#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/DataPersist.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteAnimation.h"
#include "Graph/Anim/KeyFrames.h"
#include "Graph/Anim/AnimKeys.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Module/ModuleFactory.h"

namespace ff
{
	class __declspec(uuid("07643649-dd11-42d9-9e56-614844600ca5"))
		SpriteAnimation : public ComBase, public ISpriteAnimation
	{
	public:
		DECLARE_HEADER(SpriteAnimation);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;

		// IGraphDeviceChild functions
		virtual IGraphDevice *GetDevice() const override;
		virtual void Reset() override;

		// ISpriteAnimation

		virtual void  Clear() override;
		virtual void  Render(
			I2dRenderer *pRender,
			AnimTweenType type,
			float frame,
			PointFloat pos,
			const PointFloat *pScale,
			float rotate,
			const XMFLOAT4 *pColor) override;

		virtual void SetSprite(float frame, size_t nPart, size_t nSprite, ISprite* pSprite) override;
		virtual void SetColor (float frame, size_t nPart, size_t nSprite, const XMFLOAT4& color) override;
		virtual void SetOffset(float frame, size_t nPart, PointFloat offset) override;
		virtual void SetScale (float frame, size_t nPart, PointFloat scale) override;
		virtual void SetRotate(float frame, size_t nPart, float rotate) override;
		virtual void SetHitBox(float frame, size_t nPart, RectFloat  hitBox) override;
		virtual RectFloat GetHitBox(float frame, size_t nPart, AnimTweenType type) override;

		virtual void  SetLastFrame(float frame) override;
		virtual float GetLastFrame() const override;

		virtual void  SetFPS(float fps) override;
		virtual float GetFPS() const override;

	private:
		struct PartKeys
		{
			PartKeys()
			{
				_scale.SetIdentityValue(VectorKey::IdentityScale()._value);
			}

			KeyFrames<SpriteKey> _sprites[4];
			KeyFrames<VectorKey> _colors[4];
			KeyFrames<VectorKey> _offset;
			KeyFrames<VectorKey> _scale;
			KeyFrames<FloatKey>  _rotate;
			KeyFrames<VectorKey> _hitBox;
		};

		void UpdateKeys();
		PartKeys& EnsurePartKeys(size_t nPart, float frame);

		ComPtr<IGraphDevice> _device;
		Vector<PartKeys *> _parts;
		float _lastFrame;
		float _fps;
		bool _keysChanged;
	};
}

BEGIN_INTERFACES(ff::SpriteAnimation)
	HAS_INTERFACE(ff::ISpriteAnimation)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"Sprite Animation");
	module.RegisterClassT<ff::SpriteAnimation>(name, __uuidof(ff::ISpriteAnimation), ff::GetCategoryGraphicsObject());
});

bool ff::CreateSpriteAnimation(IGraphDevice *pDevice, ISpriteAnimation **ppAnim)
{
	return SUCCEEDED(ComAllocator<SpriteAnimation>::CreateInstance(
		pDevice, GUID_NULL, __uuidof(ISpriteAnimation), (void**)ppAnim));
}

ff::SpriteAnimation::SpriteAnimation()
	: _lastFrame(0)
	, _fps(1)
	, _keysChanged(false)
{
}

ff::SpriteAnimation::~SpriteAnimation()
{
	Clear();
}

HRESULT ff::SpriteAnimation::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);

	return __super::_Construct(unkOuter);
}

ff::IGraphDevice *ff::SpriteAnimation::GetDevice() const
{
	return _device;
}

void ff::SpriteAnimation::Reset()
{
	// nothing to reset
}

void ff::SpriteAnimation::Clear()
{
	for (size_t i = 0; i < _parts.Size(); i++)
	{
		delete _parts[i];
	}

	_parts.Clear();
}

void ff::SpriteAnimation::Render(
		I2dRenderer*    pRender,
		AnimTweenType   type,
		float           frame,
		PointFloat          pos,
		const PointFloat*   pScale,
		float           rotate,
		const XMFLOAT4* pColor)
{
	UpdateKeys();

	for (size_t i = 0; i < _parts.Size(); i++)
	{
		if (!_parts[i])
		{
			continue;
		}

		PartKeys& part        = *_parts[i];
		ISprite*  pSprites[4] = { nullptr, nullptr, nullptr, nullptr };
		XMFLOAT4  realColors[4];
		size_t    nSprites = 0;

		// Sneaky way to avoid constructor/destructor
		// (don't want them to show up in profiles anymore, this function is called A LOT)
		ComPtr<ISprite>* pSpritePtrs = reinterpret_cast<ComPtr<ISprite>*>(pSprites);

		for (; nSprites < _countof(part._sprites); nSprites++)
		{
			part._sprites[nSprites].Get(frame, type, pSpritePtrs[nSprites]);

			if (!pSprites[nSprites])
			{
				break;
			}
		}

		for (size_t h = 0; h < nSprites; h++)
		{
			part._colors[h].Get(frame, type, realColors[h]);
		}

		if (nSprites)
		{
			XMFLOAT4 vectorPos;
			XMFLOAT4 vectorScale;
			float    realRotate;

			part._offset.Get(frame, type, vectorPos);
			part._scale.Get (frame, type, vectorScale);
			part._rotate.Get(frame, type, realRotate);

			realRotate += rotate;

			XMStoreFloat4(&vectorPos,
				XMVectorAdd(
					XMLoadFloat4(&vectorPos),
					XMVectorSet(pos.x, pos.y, 0, 0)));

			if (pScale)
			{
				XMStoreFloat4(&vectorScale,
					XMVectorMultiply(
						XMLoadFloat4(&vectorScale),
						XMVectorSet(pScale->x, pScale->y, 1, 1)));
			}

			if (pColor)
			{
				for (size_t h = 0; h < nSprites; h++)
				{
					XMStoreFloat4(&realColors[h],
						XMColorModulate(
							XMLoadFloat4(&realColors[h]),
							XMLoadFloat4(pColor)));
				}
			}

			PointFloat realPos(vectorPos.x, vectorPos.y);
			PointFloat realScale(vectorScale.x, vectorScale.y);

			if (nSprites == 1)
			{
				pRender->DrawSprite(pSprites[0], &realPos, &realScale, realRotate, &realColors[0]);
				pSprites[0]->Release();
			}
			else
			{
				pRender->DrawMultiSprite(pSprites, nSprites, &realPos, &realScale, realRotate, &realColors[0], nSprites);

				for (size_t h = 0; h < _countof(pSprites); h++)
				{
					pSprites[h]->Release();
				}
			}
		}
	}
}

void ff::SpriteAnimation::SetSprite(float frame, size_t nPart, size_t nSprite, ISprite* pSprite)
{
	assertRet(nSprite >= 0 && nSprite < 4);

	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._sprites[nSprite].Set(frame, pSprite);
}

void ff::SpriteAnimation::SetColor(float frame, size_t nPart, size_t nSprite, const XMFLOAT4& color)
{
	assertRet(nSprite >= 0 && nSprite < 4);

	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._colors[nSprite].Set(frame, color);
}

void ff::SpriteAnimation::SetOffset(float frame, size_t nPart, PointFloat offset)
{
	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._offset.Set(frame, XMFLOAT4(offset.x, offset.y, 0, 0));
}

void ff::SpriteAnimation::SetScale(float frame, size_t nPart, PointFloat scale)
{
	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._scale.Set(frame, XMFLOAT4(scale.x, scale.y, 0, 0));
}

void ff::SpriteAnimation::SetRotate(float frame, size_t nPart, float rotate)
{
	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._rotate.Set(frame, rotate);
}

void ff::SpriteAnimation::SetHitBox(float frame, size_t nPart, RectFloat hitBox)
{
	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._hitBox.Set(frame, *(const XMFLOAT4*)&hitBox);
}

ff::RectFloat ff::SpriteAnimation::GetHitBox(float frame, size_t nPart, AnimTweenType type)
{
	UpdateKeys();

	RectFloat hitBox(0, 0, 0, 0);
	assertRetVal(nPart >= 0 && nPart < _parts.Size(), hitBox);

	_parts[nPart]->_hitBox.Get(frame, type, *(XMFLOAT4*)&hitBox);

	return hitBox;
}

void ff::SpriteAnimation::SetLastFrame(float frame)
{
	_lastFrame = std::max<float>(0, frame);
	_keysChanged = true;
}

float ff::SpriteAnimation::GetLastFrame() const
{
	return _lastFrame;
}

void ff::SpriteAnimation::SetFPS(float fps)
{
	_fps = std::abs(fps);
}

float ff::SpriteAnimation::GetFPS() const
{
	return _fps;
}

void ff::SpriteAnimation::UpdateKeys()
{
	if (_keysChanged)
	{
		for (size_t i = 0; i < _parts.Size(); i++)
		{
			if (_parts[i])
			{
				PartKeys &part = *_parts[i];

				part._sprites[0].SetLastFrame(_lastFrame);
				part._sprites[1].SetLastFrame(_lastFrame);
				part._sprites[2].SetLastFrame(_lastFrame);
				part._sprites[3].SetLastFrame(_lastFrame);

				part._colors[0].SetLastFrame(_lastFrame);
				part._colors[1].SetLastFrame(_lastFrame);
				part._colors[2].SetLastFrame(_lastFrame);
				part._colors[3].SetLastFrame(_lastFrame);

				part._offset.SetLastFrame(_lastFrame);
				part._scale.SetLastFrame (_lastFrame);
				part._rotate.SetLastFrame(_lastFrame);
				part._hitBox.SetLastFrame(_lastFrame);
			}
		}
	}
}

ff::SpriteAnimation::PartKeys &ff::SpriteAnimation::EnsurePartKeys(size_t nPart, float frame)
{
	while (nPart >= _parts.Size())
	{
		_parts.Push(nullptr);
	}

	if (!_parts[nPart])
	{
		_parts[nPart] = new PartKeys();

		_parts[nPart]->_scale.SetIdentityValue(VectorKey::IdentityScale()._value);

		_parts[nPart]->_colors[0].SetIdentityValue(VectorKey::IdentityWhite()._value);
		_parts[nPart]->_colors[1].SetIdentityValue(VectorKey::IdentityWhite()._value);
		_parts[nPart]->_colors[2].SetIdentityValue(VectorKey::IdentityWhite()._value);
		_parts[nPart]->_colors[3].SetIdentityValue(VectorKey::IdentityWhite()._value);
	}

	_lastFrame = std::max(_lastFrame, frame);
	_keysChanged = true;

	return *_parts[nPart];
}
