#include "pch.h"
#include "COM/ComAlloc.h"
#include "COM/ServiceCollection.h"
#include "CLR/InteropServices.h"
#include "Resource/util-resource.h"

namespace ff
{
	#include <FerretFace.Interop_h.h>
}

class __declspec(uuid("afdbf938-f94a-49c8-8467-5a1a70d0e825"))
	CInteropServiceShim
		: public ff::ComBase
		, public ff::IInteropServices
{
public:
	DECLARE_HEADER_AND_TYPELIB(CInteropServiceShim, ID_TYPELIB_INTEROP);

	bool Init(ff::IServiceProvider *services);

	// IInteropServices
	COM_FUNC GetInteropService(REFGUID serviceID, IUnknown **ppService) override;
	COM_FUNC SetInteropService(REFGUID serviceID, IUnknown *ppService) override;

private:
	ff::ComPtr<ff::IServiceCollection> _services;
};

BEGIN_INTERFACES(CInteropServiceShim)
	HAS_INTERFACE(ff::IInteropServices)
END_INTERFACES()

class __declspec(uuid("a0d0e614-3b91-4f8f-873f-1f3cce2b7737"))
	CServiceProviderShim
		: public ff::ComBase
		, public ff::IServiceProvider
{
public:
	DECLARE_HEADER(CServiceProviderShim);

	bool Init(ff::IInteropServices *services);

	// IServiceProvider
	COM_FUNC QueryService(REFGUID guidService, REFIID riid, void** ppvObject) override;

private:
	ff::ComPtr<ff::IInteropServices> _services;
};

BEGIN_INTERFACES(CServiceProviderShim)
	HAS_INTERFACE(ff::IServiceProvider)
END_INTERFACES()

bool ff::CreateInteropServicesShim(IServiceProvider *services, IInteropServices **obj)
{
	assertRetVal(obj, false);

	ComPtr<CInteropServiceShim, IInteropServices> shim;
	assertRetVal(SUCCEEDED(ComAllocator<CInteropServiceShim>::CreateInstance(&shim)), false);
	assertRetVal(shim->Init(services), false);

	*obj = shim.Detach();
	return true;
}

bool ff::CreateInteropServicesShim(IInteropServices *services, IServiceProvider **obj)
{
	assertRetVal(obj, false);

	ComPtr<CServiceProviderShim, IServiceProvider> shim;
	assertRetVal(SUCCEEDED(ComAllocator<CServiceProviderShim>::CreateInstance(&shim)), false);
	assertRetVal(shim->Init(services), false);

	*obj = shim.Detach();
	return true;
}

CInteropServiceShim::CInteropServiceShim()
{
}

CInteropServiceShim::~CInteropServiceShim()
{
}

bool CInteropServiceShim::Init(ff::IServiceProvider *services)
{
	assertRetVal(services && !_services, false);
	assertRetVal(ff::CreateServiceCollection(services, &_services), false);

	return true;
}

HRESULT CInteropServiceShim::GetInteropService(REFGUID serviceId, IUnknown **ppService)
{
	return ff::GetService(_services, serviceId, ppService) ? S_OK : S_FALSE;
}

HRESULT CInteropServiceShim::SetInteropService(REFGUID serviceId, IUnknown *service)
{
	if (service == nullptr)
	{
		_services->RemoveService(serviceId);
	}
	else
	{
		assertRetVal(_services->AddService(serviceId, service), E_FAIL);
	}

	return S_OK;
}

CServiceProviderShim::CServiceProviderShim()
{
}

CServiceProviderShim::~CServiceProviderShim()
{
}

bool CServiceProviderShim::Init(ff::IInteropServices *services)
{
	assertRetVal(services && !_services, false);
	_services = services;

	return true;
}

HRESULT CServiceProviderShim::QueryService(REFGUID guidService, REFIID riid, void **obj)
{
	return ff::GetInteropService(_services, guidService, riid, obj) ? S_OK : E_FAIL;
}

bool ff::GetInteropService(IInteropServices *services, REFGUID serviceid, REFGUID iid, void **obj)
{
	assertRetVal(obj, false);

	ComPtr<IUnknown> unknownObj;
	if (ff::GetInteropService(services, serviceid, &unknownObj))
	{
		return SUCCEEDED(unknownObj->QueryInterface(iid, obj));
	}

	return false;
}

template<>
bool ff::GetInteropService<IUnknown>(IInteropServices *services, REFGUID serviceid, IUnknown **obj)
{
	assertRetVal(services && obj, false);
	return SUCCEEDED(services->GetInteropService(serviceid, obj)) && *obj;
}
