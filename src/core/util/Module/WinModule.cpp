#include "pch.h"
#include "Data/Data.h"
#include "Data/DataWriterReader.h"
#include "Module/WinModule.h"

#if !METRO_APP

ff::WinModule::WinModule(StringRef name, REFGUID id, HINSTANCE instance)
	: Module(name, id, instance)
{
}

bool ff::WinModule::GetAsset(unsigned int id, IDataReader **data) const
{
	assertRetVal(data, false);

	ComPtr<IData> resourceData;

	assertRetVal(CreateDataInResource(GetInstance(), id, &resourceData), false);
	assertRetVal(CreateDataReader(resourceData, 0, data), false);

	return *data != nullptr;
}

ff::String ff::WinModule::GetString(unsigned int id) const
{
	wchar_t *str = nullptr;
	int strLen = ::LoadStringW(GetInstance(), id, (LPWSTR)&str, 0);
	assert(strLen != 0 || GetLastError() == NO_ERROR);

	return String::from_static(strLen > 0 ? str : L"", static_cast<size_t>(strLen));
}

#endif // !METRO_APP
