#include "pch.h"
#include "COM/ComAlloc.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteList.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Module/ModuleFactory.h"

static const ff::SpriteData &GetEmptySpriteData()
{
	static const ff::SpriteData data =
	{
		ff::RectFloat(0, 0, 0, 0),
		ff::RectFloat(0, 0, 0, 0),
		ff::ComPtr<ff::IGraphTexture>(nullptr),
		ff::String()
	};

	return data;
}

ff::RectInt ff::SpriteData::GetTextureRect() const
{
	return GetTextureRectF().ToInt();
}

ff::RectFloat ff::SpriteData::GetTextureRectF() const
{
	assertRetVal(_texture, RectFloat(0, 0, 0, 0));

	PointFloat texSize = _texture->GetSize().ToFloat();
	RectFloat texRect = _textureUV * texSize;
	texRect.Normalize();

	return texRect;
}

class __declspec(uuid("7f942967-3605-4b36-a880-a6252d2518b9"))
	Sprite : public ff::ComBase, public ff::ISprite
{
public:
	DECLARE_HEADER(Sprite);

	// ISprite functions
	virtual const ff::SpriteData &GetData() const override;
	virtual void SetData(const ff::SpriteData &data) override;

	virtual bool CopyTo(ff::IGraphTexture *pTexture, ff::PointInt pos) override;

private:
	ff::SpriteData _data;
};

BEGIN_INTERFACES(Sprite)
	HAS_INTERFACE(ff::ISprite)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"CSprite");
	module.RegisterClassT<Sprite>(name, __uuidof(ff::ISprite));
});

bool ff::CreateSprite(const SpriteData &data, ISprite **ppSprite)
{
	assertRetVal(ppSprite && data._texture, false);

	ComPtr<Sprite, ISprite> myObj;
	assertHrRetVal(ff::ComAllocator<Sprite>::CreateInstance(&myObj), false);
	myObj->SetData(data);

	*ppSprite = myObj.Detach();
	return true;
}

bool ff::CreateSprite(IGraphTexture *pTexture, StringRef name, RectInt rect, PointFloat handle, PointFloat scale, ISprite **ppSprite)
{
	assertRetVal(ppSprite && pTexture, false);

	PointInt sizeTex = pTexture->GetSize();
	assertRetVal(sizeTex.x && sizeTex.y, false);

	SpriteData data;
	data._name = name;
	data._texture = pTexture;

	data._textureUV.SetRect(
		rect.left   / (float)sizeTex.x,
		rect.top    / (float)sizeTex.y,
		rect.right  / (float)sizeTex.x,
		rect.bottom / (float)sizeTex.y);

	data._worldRect.SetRect(
		-handle.x * scale.x,
		-handle.y * scale.y,
		(-handle.x + rect.Width()) * scale.x,
		(-handle.y + rect.Height()) * scale.y);

	return CreateSprite(data, ppSprite);
}

bool ff::CreateSprite(ISprite *pParentSprite, RectInt rect, PointFloat handle, PointFloat scale, ISprite **ppSprite)
{
	assertRetVal(pParentSprite && ppSprite, false);

	const SpriteData& parentData = pParentSprite->GetData();
	RectInt             parentRect = parentData.GetTextureRect();

	rect.Offset(parentRect.TopLeft());
	assertRetVal(rect.IsInside(parentRect), false);

	return CreateSprite(parentData._texture, GetEmptyString(), rect, handle, scale, ppSprite);
}

bool ff::CreateSprite(IGraphTexture *pTexture, ISprite **ppSprite)
{
	assertRetVal(ppSprite && pTexture, false);

	return CreateSprite(pTexture, GetEmptyString(),
		RectInt(PointInt(0, 0), pTexture->GetSize()),
		PointFloat(0, 0), // handle
		PointFloat(1, 1), // scale
		ppSprite);
}

bool ff::CreateSprite(ISpriteList *pList, StringRef name, ISprite **ppSprite)
{
	assertRetVal(pList && name.size() && ppSprite, false);

	ComPtr<ISprite> pSprite = pList->Get(name);
	*ppSprite = pSprite.Detach();
	return true;
}

bool ff::CreateSprite(ISpriteList *pList, size_t index, ISprite **ppSprite)
{
	String name = String::format_new(L"%lu", index);
	return CreateSprite(pList, name, ppSprite);
}

Sprite::Sprite()
{
}

Sprite::~Sprite()
{
}

const ff::SpriteData &Sprite::GetData() const
{
	return _data;
}

void Sprite::SetData(const ff::SpriteData &data)
{
	_data = data;
}

bool Sprite::CopyTo(ff::IGraphTexture *pTexture, ff::PointInt pos)
{
	assertRetVal(pTexture && _data._texture, false);

	ff::RectInt rect = _data.GetTextureRect();

	D3D11_BOX box;
	box.left   = rect.left;
	box.top    = rect.top;
	box.right  = rect.right;
	box.bottom = rect.bottom;
	box.front  = 0;
	box.back   = 1;

	pTexture->GetDevice()->GetContext()->CopySubresourceRegion(
		pTexture->GetTexture(),          0, pos.x, pos.y, 0,
		_data._texture->GetTexture(), 0, &box);

	return true;
}
