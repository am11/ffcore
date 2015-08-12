#include "pch.h"
#include "App/AppGlobals.h"
#include "App/MostRecentlyUsed.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

bool ff::RecentlyUsedItem::operator==(const RecentlyUsedItem &rhs) const
{
	return ff::PathsEqual(rhs._path, _path);
}

ff::MostRecentlyUsed::MostRecentlyUsed()
	: _listener(nullptr)
	, _limit(16)
{
}

ff::MostRecentlyUsed::~MostRecentlyUsed()
{
}

void ff::MostRecentlyUsed::SetListener(IMostRecentlyUsedListener *listener)
{
	_listener = listener;
}

const ff::RecentlyUsedItem &ff::MostRecentlyUsed::GetItem(size_t index) const
{
	return _items[index];
}

size_t ff::MostRecentlyUsed::GetCount() const
{
	return _items.Size();
}

void ff::MostRecentlyUsed::SetLimit(size_t nLimit)
{
	_limit = std::min<size_t>(128, nLimit);
}

void ff::MostRecentlyUsed::Add(StringRef path, StringRef name, bool pinned)
{
	assertRet(path.size());

	// Look for dupes
	for (size_t i = PreviousSize(_items.Size()); i != INVALID_SIZE; i = PreviousSize(i))
	{
		if (ff::PathsEqual(_items[i]._path, path))
		{
			pinned = pinned || _items[i]._pinned;
			_items.Delete(i);
			break;
		}
	}

	// Always add to the top of the list
	RecentlyUsedItem item;
	item._path = path;
	item._name = name;
	item._pinned = pinned;

	_items.Insert(0, item);

	// Limit the amount of items
	while (_items.Size() > _limit)
	{
		size_t nDelete = _items.Size() - 1;
		for (size_t i = nDelete; i != INVALID_SIZE; i = PreviousSize(i))
		{
			if (!_items[i]._pinned)
			{
				nDelete = i;
				break;
			}
		}

		_items.Delete(nDelete);
	}

	if (_listener != nullptr)
	{
		_listener->OnMruChanged(*this);
	}
}

bool ff::MostRecentlyUsed::Pin(StringRef path, bool pinned)
{
	assertRetVal(path.size(), false);

	for (RecentlyUsedItem &item: _items)
	{
		if (ff::PathsEqual(item._path, path) && item._pinned != pinned)
		{
			item._pinned = pinned;

			if (_listener != nullptr)
			{
				_listener->OnMruChanged(*this);
			}

			return true;
		}
	}

	return false;
}

bool ff::MostRecentlyUsed::Remove(StringRef path)
{
	assertRetVal(path.size(), false);

	for (const RecentlyUsedItem &item: _items)
	{
		if (ff::PathsEqual(item._path, path))
		{
			_items.DeleteItem(item);

			if (_listener != nullptr)
			{
				_listener->OnMruChanged(*this);
			}

			return true;
		}
	}

	return false;
}

void ff::MostRecentlyUsed::Clear()
{
	if (!_items.IsEmpty())
	{
		_items.Clear();

		if (_listener != nullptr)
		{
			_listener->OnMruChanged(*this);
		}
	}
}
