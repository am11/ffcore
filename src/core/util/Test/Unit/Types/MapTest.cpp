#include "pch.h"

bool MapTest()
{
	ff::Map<ff::String, int> table;
	std::array<wchar_t, 256> buf;

	for (int i = 0; i < 1000; i++)
	{
		_snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%d", i * 17);
		ff::String str(buf.data());
		
		table.SetKey(str, i);
		table.Insert(ff::String(str), -i);
	}

	for (int i = 0; i < 1000; i++)
	{
		_snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%d", i * 17);
		ff::String str(buf.data());
		
		ff::BucketIter pos = table.Get(str);
		assertRetVal(pos != ff::INVALID_ITER, false);
		assertRetVal(std::abs(table.ValueAt(pos)) == i, false);

		pos = table.Get(str);
		assertRetVal(pos != ff::INVALID_ITER, false);
		assertRetVal(std::abs(table.ValueAt(pos)) == i, false);
		
		pos = table.GetNext(pos);
		assertRetVal(pos != ff::INVALID_ITER, false);
		assertRetVal(std::abs(table.ValueAt(pos)) == i, false);

		pos = table.GetNext(pos);
		assertRetVal(pos == ff::INVALID_ITER, false);
	}

	assertRetVal(table.Size() == 2000, false);
	int count = 0;
	ff::BucketIter pos = table.StartIteration();
	assertRetVal(pos != ff::INVALID_ITER, false);

	while (pos != ff::INVALID_ITER)
	{
		int i = std::abs(table.ValueAt(pos));
		_snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%d", i * 17);

		assertRetVal(table.KeyAt(pos) == buf.data(), false);

		pos = table.Iterate(pos);
		count++;
	}

	assertRetVal(count == 2000, false);

	for (const auto &iter: table)
	{
		int i = std::abs(iter.GetValue());
		_snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%d", i * 17);

		assertRetVal(iter.GetKey() == buf.data(), false);
	}

	return true;
}
