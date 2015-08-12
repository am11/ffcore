#include "pch.h"
#include "CLR/HostControl.h"
#include "COM/ComAlloc.h"
#include "COM/ServiceCollection.h"
#include "Globals/ProcessGlobals.h"
#include "Resource/util-resource.h"

#if METRO_APP

static bool s_unusedFile_HostControl = false;

#else

namespace ff
{
	#include <FerretFace.Interop_h.h>
}

class __declspec(uuid("06f37a4d-bcd1-4147-9738-b1cb8a331e9b"))
	NativeHostControl
		: public ff::ComBaseNoRef
		, public ff::INativeHostControl
		, public ff::IInteropServices
{
public:
	DECLARE_HEADER_AND_TYPELIB(NativeHostControl, ID_TYPELIB_INTEROP);

	// IHostControl
	STDMETHODIMP GetHostManager(REFIID riid, void **ppObject) override;
	STDMETHODIMP SetAppDomainManager(DWORD dwAppDomainID, IUnknown *pUnkAppDomainManager) override;

	// INativeHostControl
	virtual bool Startup() override;
	virtual void Shutdown() override;
	virtual IInteropServices *GetManagedServices() const override;
	virtual ff::IManagedGlobals *GetManagedGlobals() const override;
	virtual ff::IServiceCollection *GetServices() const override;

	// IInteropServices
	STDMETHODIMP GetInteropService(REFGUID serviceID, IUnknown **ppService) override;
	STDMETHODIMP SetInteropService(REFGUID serviceID, IUnknown *ppService) override;

private:
	ff::ComPtr<ff::IInteropServices> _managedServices;
	ff::ComPtr<ff::IManagedGlobals> _globals;
	ff::ComPtr<ff::IServiceCollection> _services;
	bool _started;
};

BEGIN_INTERFACES(NativeHostControl)
	HAS_INTERFACE(ff::INativeHostControl)
	HAS_INTERFACE(ff::IInteropServices)
	HAS_INTERFACE(IHostControl)
END_INTERFACES()

static NativeHostControl *s_hostControl = nullptr;

bool ff::CreateHostControl(INativeHostControl **ppHostControl)
{
	assertRetVal(ppHostControl, false);

	if (!s_hostControl)
	{
		ScopeStaticMemAlloc staticAlloc;
		ComPtr<NativeHostControl, INativeHostControl> pHostControl;

		assertRetVal(SUCCEEDED(ComAllocator<NativeHostControl>::CreateInstance(&pHostControl)), false);

		*ppHostControl = pHostControl.Detach();
	}
	else
	{
		*ppHostControl = GetAddRef<INativeHostControl>(s_hostControl);
	}

	return true;
}

NativeHostControl::NativeHostControl()
	: _started(false)
{
	assert(!s_hostControl);
	s_hostControl = this;
}

NativeHostControl::~NativeHostControl()
{
	// This global object is never deleted (managed code can hold onto it forever)
	assert(false);
	s_hostControl = nullptr;
}

bool NativeHostControl::Startup()
{
	assertRetVal(!_started, false);
	_started = true;

	assertRetVal(CreateServiceCollection(ff::ProcessGlobals::Get()->GetServices(), &_services), false);

	assertRetVal(_globals, false);
	ff::IInteropServices *services = this;
	assertHrRetVal(_globals->Startup(services), false);

	return true;
}

void NativeHostControl::Shutdown()
{
	assert(_started);
	_started = false;

	if (_globals)
	{
		_globals->Shutdown();
		_globals = nullptr;
	}

	_managedServices = nullptr;
	_services = nullptr;

	assert(s_hostControl == this);
	s_hostControl = nullptr;
}

ff::IInteropServices *NativeHostControl::GetManagedServices() const
{
	return _managedServices;
}

ff::IManagedGlobals *NativeHostControl::GetManagedGlobals() const
{
	return _globals;
}

ff::IServiceCollection *NativeHostControl::GetServices() const
{
	return _services;
}

HRESULT NativeHostControl::GetHostManager(REFIID riid, void **ppObject)
{
	return E_NOINTERFACE;
}

HRESULT NativeHostControl::SetAppDomainManager(DWORD dwAppDomainID, IUnknown *pUnkAppDomainManager)
{
	if (pUnkAppDomainManager && !_managedServices)
	{
		if (_managedServices.QueryFrom(pUnkAppDomainManager))
		{
			ff::ComPtr<IUnknown> pUnkGlobals;
			assertHrRetVal(_managedServices->GetInteropService(__uuidof(ff::IManagedGlobals), &pUnkGlobals), E_FAIL);
			assertRetVal(_globals.QueryFrom(pUnkGlobals), false);
		}
	}

	assert(_globals);

	return S_OK;
}

HRESULT NativeHostControl::GetInteropService(REFGUID serviceId, IUnknown **ppService)
{
	return ff::GetService(_services, serviceId, ppService) ? S_OK : E_FAIL;
}

HRESULT NativeHostControl::SetInteropService(REFGUID serviceId, IUnknown *service)
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

template<>
bool ff::INativeHostControl::GetManagedService<IUnknown>(REFGUID serviceid, IUnknown **obj)
{
	return SUCCEEDED(ff::GetInteropService(GetManagedServices(), serviceid, obj));
}

#endif // !METRO_APP
