#pragma once
#include "CLR/InteropServices.h"

#if !METRO_APP

namespace ff
{
	class IServiceCollection;
	struct IManagedGlobals;

	class __declspec(uuid("2b129dfe-2265-4f58-897c-3f0357242645")) __declspec(novtable)
		INativeHostControl : public IHostControl
	{
	public:
		virtual bool Startup() = 0;
		virtual void Shutdown() = 0;
		virtual IInteropServices *GetManagedServices() const = 0;
		virtual IManagedGlobals *GetManagedGlobals() const = 0;
		virtual IServiceCollection *GetServices() const = 0;

		template<class T>
		bool GetManagedService(T **obj);

		template<class T>
		bool GetManagedService(REFGUID serviceid, T **obj);

		template<>
		UTIL_API bool GetManagedService<IUnknown>(REFGUID serviceid, IUnknown **obj);
	};

	bool CreateHostControl(INativeHostControl **ppHostControl);
}

template<class T>
bool ff::INativeHostControl::GetManagedService(T **obj)
{
	return ff::GetInteropService<T>(GetManagedServices(), __uuidof(T), obj);
}

template<class T>
bool ff::INativeHostControl::GetManagedService(REFGUID serviceid, T **obj)
{
	return ff::GetInteropService(GetManagedServices(), serviceid, obj);
}

#endif
