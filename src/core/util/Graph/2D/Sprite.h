#pragma once

namespace ff
{
	class IGraphTexture;
	class IPackageNodePersist;
	class ISpriteList;

	struct SpriteData
	{
		UTIL_API RectInt GetTextureRect() const;
		UTIL_API RectFloat GetTextureRectF() const;

		RectFloat _textureUV;
		RectFloat _worldRect;
		ComPtr<IGraphTexture> _texture;
		String _name;
	};

	class __declspec(uuid("960fc0cc-a692-4a3e-9e37-59e7ca330aac")) __declspec(novtable)
		ISprite : public IUnknown
	{
	public:
		virtual const SpriteData &GetData() const = 0;
		virtual void SetData(const SpriteData &data) = 0;
		virtual bool CopyTo(IGraphTexture *pTexture, PointInt pos) = 0;
	};

	UTIL_API bool CreateSprite(const SpriteData &data, ISprite **ppSprite);
	UTIL_API bool CreateSprite(IGraphTexture *pTexture, StringRef name, RectInt rect, PointFloat handle, PointFloat scale, ISprite **ppSprite);

	UTIL_API bool CreateSprite(ISprite *pParentSprite, RectInt rect, PointFloat handle, PointFloat scale, ISprite **ppSprite);
	UTIL_API bool CreateSprite(IGraphTexture *pTexture, ISprite **ppSprite);
	UTIL_API bool CreateSprite(ISpriteList *pList, StringRef name, ISprite **ppSprite);
	UTIL_API bool CreateSprite(ISpriteList *pList, size_t index, ISprite **ppSprite);
}
