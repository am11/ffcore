#pragma once

namespace ff
{
	class IServiceProvider;
	struct IInteropServices;

	UTIL_API bool CreateInteropServicesShim(IServiceProvider *services, IInteropServices **shim);
	UTIL_API bool CreateInteropServicesShim(IInteropServices *services, IServiceProvider **shim);

	template<class T>
	bool GetInteropService(IInteropServices *services, T **obj);
	template<class T>
	bool GetInteropService(IInteropServices *services, REFGUID serviceid, T **obj);
	UTIL_API bool GetInteropService(IInteropServices *services, REFGUID serviceid, REFGUID iid, void **obj);
	template<>
	UTIL_API bool GetInteropService<IUnknown>(IInteropServices *services, REFGUID serviceid, IUnknown **obj);
}

template<class T>
bool ff::GetInteropService(IInteropServices *services, T **obj)
{
	return ff::GetInteropService<T>(services, __uuidof(T), obj);
}

template<class T>
bool ff::GetInteropService(IInteropServices *services, REFGUID serviceid, T **obj)
{
	assertRetVal(obj, false);

	ComPtr<IUnknown> unknownObj;
	assertRetVal(ff::GetInteropService(services, serviceid, &unknownObj), false);

	ComPtr<T> knownObj;
	assertRetVal(knownObj.QueryFrom(unknownObj), false);

	*obj = knownObj.Detach();
	return *obj != nullptr;
}
