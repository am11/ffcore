#include "pch.h"
#include "COM/ComAlloc.h"
#include "COM/ComFactory.h"
#include "COM/ComListener.h"
#include "Module/Module.h"

class __declspec(uuid("d79aaf15-483c-4040-9836-fc9cc2ff14ac"))
	CFactory : public ff::ComBase, public IClassFactory
{
public:
	DECLARE_HEADER(CFactory);

	void SetAllocator(REFGUID clsid, const ff::Module *module, ff::ClassFactoryFunc func);

	// IClassFactory
	COM_FUNC CreateInstance(IUnknown *unkOuter, REFIID iid, void **ppv) override;
	COM_FUNC LockServer(BOOL bLock) override;

private:
	const ff::Module *_module;
	ff::ClassFactoryFunc _func;
	GUID _clsid;
};

BEGIN_INTERFACES(CFactory)
	HAS_INTERFACE(IClassFactory)
END_INTERFACES()

class __declspec(uuid("fb7e618b-a9ba-4e66-86aa-7f078abc7489"))
	CSingletonFactory : public CFactory, public ff::IComListener
{
public:
	DECLARE_HEADER(CSingletonFactory);

	// IClassFactory
	COM_FUNC CreateInstance(IUnknown *unkOuter, REFIID iid, void **ppv) override;

	// IComListener
	virtual void OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj) override;
	virtual void OnDestruct (REFGUID catid, REFGUID clsid, IUnknown *pObj) override;

private:
	ff::Mutex _cs;
	IUnknown *_object;
};

BEGIN_INTERFACES(CSingletonFactory)
	PARENT_INTERFACES(CFactory)
END_INTERFACES()

bool ff::CreateClassFactory(REFGUID clsid, const ff::Module *module, ff::ClassFactoryFunc func, IClassFactory **factory)
{
	assertRetVal(factory && module && func, false);
	*factory = nullptr;

	ff::ComPtr<CFactory> myFactory;
	assertRetVal(SUCCEEDED(ff::ComAllocator<CFactory>::CreateInstance(&myFactory)), false);
	myFactory->SetAllocator(clsid, module, func);

	*factory = myFactory.Detach();
	return true;
}

bool ff::CreateSingletonClassFactory(REFGUID clsid, const ff::Module *module, ff::ClassFactoryFunc func, IClassFactory **factory)
{
	assertRetVal(*factory && module && func, false);
	*factory = nullptr;

	ff::ComPtr<CSingletonFactory, IClassFactory> myFactory;
	assertRetVal(SUCCEEDED(ff::ComAllocator<CSingletonFactory>::CreateInstance(&myFactory)), false);
	myFactory->SetAllocator(clsid, module, func);

	*factory = myFactory.Detach();
	return true;
}

CFactory::CFactory()
	: _module(nullptr)
	, _func(nullptr)
{
}

CFactory::~CFactory()
{
}

void CFactory::SetAllocator(REFGUID clsid, const ff::Module *module, ff::ClassFactoryFunc func)
{
	_module = module;
	_func   = func;
	_clsid   = clsid;
}

HRESULT CFactory::CreateInstance(IUnknown *unkOuter, REFIID iid, void **ppv)
{
	assertRetVal(ppv, E_INVALIDARG);
	assertRetVal(_func, E_FAIL);

	return _func(unkOuter, _clsid, iid, ppv);
}

HRESULT CFactory::LockServer(BOOL bLock)
{
	assertRetVal(_module, E_FAIL);

	if (bLock)
	{
		_module->AddRef();
	}
	else
	{
		_module->Release();
	}

	return S_OK;
}

CSingletonFactory::CSingletonFactory()
	: _object(nullptr)
{
}

CSingletonFactory::~CSingletonFactory()
{
}

HRESULT CSingletonFactory::CreateInstance(IUnknown *unkOuter, REFIID iid, void **ppv)
{
	ff::LockMutex crit(_cs);

	ff::ComPtr<IUnknown> obj = _object;

	if (!obj)
	{
		HRESULT hr = __super::CreateInstance(unkOuter, __uuidof(IUnknown), (void**)&obj);
		assertRetVal(SUCCEEDED(hr), hr);

		AddComListener(obj, this);

		_object = obj;
	}

	return obj->QueryInterface(iid, ppv);
}

void CSingletonFactory::OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
}

void CSingletonFactory::OnDestruct(REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	ff::LockMutex crit(_cs);

	_object = nullptr;
}
