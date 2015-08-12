#include "pch.h"
#include "Module/MetroModule.h"

#if METRO_APP

ff::MetroModule::MetroModule(StringRef name, REFGUID id, HINSTANCE instance)
	: Module(name, id, instance)
{
}

bool ff::MetroModule::GetAsset(unsigned int id, IDataReader **data) const
{
	assertRetVal(data, false);
	*data = nullptr;
	return false;
}

ff::String ff::MetroModule::GetString(unsigned int id) const
{
	return String();
}

#endif // METRO_APP
