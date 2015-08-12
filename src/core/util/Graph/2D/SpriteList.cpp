#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/DataPersist.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteList.h"
#include "Graph/2D/SpriteOptimizer.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Module/ModuleFactory.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

namespace ff
{
	class __declspec(uuid("7ddb9bd1-c9e0-4788-b0b2-3bb252515013"))
		SpriteList : public ComBase, public ISpriteList
	{
	public:
		DECLARE_HEADER(SpriteList);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;

		// IGraphDeviceChild functions
		virtual IGraphDevice *GetDevice() const override;
		virtual void Reset() override;

		// ISpriteList functions
		virtual ISprite* Add(IGraphTexture *pTexture, StringRef name, RectInt rect, PointFloat handle, PointFloat scale) override;
		virtual ISprite* Add(ISprite *pSprite) override;
		virtual bool     Add(ISpriteList *pList) override;

		virtual size_t   GetCount() override;
		virtual ISprite* Get(size_t nSprite) override;
		virtual ISprite* Get(StringRef name) override;
		virtual size_t   GetIndex(StringRef name) override;
		virtual bool     Remove(ISprite *pSprite) override;
		virtual bool     Remove(size_t nSprite) override;

	private:
		ComPtr<IGraphDevice> _device;
		Vector<ComPtr<ISprite>> _sprites;
	};
}

BEGIN_INTERFACES(ff::SpriteList)
	HAS_INTERFACE(ff::ISpriteList)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"Sprite List");
	module.RegisterClassT<ff::SpriteList>(name, __uuidof(ff::ISpriteList), ff::GetCategoryGraphicsObject());
});

bool ff::CreateSpriteList(IGraphDevice *pDevice, ISpriteList **ppList)
{
	return SUCCEEDED(ComAllocator<SpriteList>::CreateInstance(
		pDevice, GUID_NULL, __uuidof(ISpriteList), (void**)ppList));
}

ff::SpriteList::SpriteList()
{
}

ff::SpriteList::~SpriteList()
{
}

HRESULT ff::SpriteList::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);

	return __super::_Construct(unkOuter);
}

ff::IGraphDevice *ff::SpriteList::GetDevice() const
{
	return _device;
}

void ff::SpriteList::Reset()
{
	// nothing to reset
}

ff::ISprite *ff::SpriteList::Add(IGraphTexture *pTexture, StringRef name, RectInt rect, PointFloat handle, PointFloat scale)
{
	ComPtr<ISprite> pSprite;
	assertRetVal(CreateSprite(pTexture, name, rect, handle, scale, &pSprite), false);

	_sprites.Push(pSprite);

	return pSprite;
}

ff::ISprite *ff::SpriteList::Add(ISprite *pSprite)
{
	assertRetVal(pSprite, nullptr);

	ComPtr<ISprite> pNewSprite;
	assertRetVal(CreateSprite(pSprite->GetData(), &pNewSprite), false);

	_sprites.Push(pNewSprite);

	return pNewSprite;
}

bool ff::SpriteList::Add(ISpriteList *pList)
{
	assertRetVal(pList, false);

	for (size_t i = 0; i < pList->GetCount(); i++)
	{
		Add(pList->Get(i));
	}

	return true;
}

size_t ff::SpriteList::GetCount()
{
	return _sprites.Size();
}

ff::ISprite *ff::SpriteList::Get(size_t nSprite)
{
	noAssertRetVal(nSprite < _sprites.Size(), nullptr);

	return _sprites[nSprite];
}

ff::ISprite *ff::SpriteList::Get(StringRef name)
{
	size_t nIndex = GetIndex(name);

	if (nIndex == INVALID_SIZE && name.size() && isdigit(name[0]))
	{
		int nValue = 0;
		int nChars = 0;

		// convert the string to an integer
		if (_snwscanf_s(name.c_str(), name.size(), L"%d%n", &nValue, &nChars) == 1 &&
			(size_t)nChars == name.size())
		{
			return Get((size_t)nValue);
		}
	}

	return (nIndex != INVALID_SIZE) ? _sprites[nIndex] : nullptr;
}

size_t ff::SpriteList::GetIndex(StringRef name)
{
	assertRetVal(name.size(), INVALID_SIZE);
	size_t nLen = name.size();

	for (size_t i = 0; i < _sprites.Size(); i++)
	{
		const SpriteData &data = _sprites[i]->GetData();

		if (data._name.size() == nLen && data._name == name)
		{
			return i;
		}
	}

	return INVALID_SIZE;
}

bool ff::SpriteList::Remove(ISprite *pSprite)
{
	for (size_t i = 0; i < _sprites.Size(); i++)
	{
		if (_sprites[i] == pSprite)
		{
			_sprites.Delete(i);
			return true;
		}
	}

	assertRetVal(false, false);
}

bool ff::SpriteList::Remove(size_t nSprite)
{
	assertRetVal(nSprite >= 0 && nSprite < _sprites.Size(), false);
	_sprites.Delete(nSprite);

	return true;
}
