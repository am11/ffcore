#pragma once

namespace ff
{
	class AppGlobals;
	class MostRecentlyUsed;

	struct RecentlyUsedItem
	{
		bool operator==(const RecentlyUsedItem &rhs) const;

		String _path;
		String _name;
		bool _pinned;
	};

	class IMostRecentlyUsedListener
	{
	public:
		virtual void OnMruChanged(const MostRecentlyUsed &mru) = 0;
	};

	class MostRecentlyUsed
	{
	public:
		UTIL_API MostRecentlyUsed();
		UTIL_API ~MostRecentlyUsed();

		UTIL_API const RecentlyUsedItem &GetItem(size_t index) const;
		UTIL_API size_t GetCount() const;
		UTIL_API void SetLimit(size_t nLimit);

		UTIL_API void Add(StringRef path, StringRef name, bool pinned);
		UTIL_API bool Pin(StringRef path, bool pinned);
		UTIL_API bool Remove(StringRef path);
		UTIL_API void Clear();

		UTIL_API void SetListener(IMostRecentlyUsedListener *listener);

	private:
		size_t _limit;
		IMostRecentlyUsedListener *_listener;
		Vector<RecentlyUsedItem> _items;
	};
}
