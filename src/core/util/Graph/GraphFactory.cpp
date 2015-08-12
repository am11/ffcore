#include "pch.h"
#include "COM/ComAlloc.h"
#include "COM/ComListener.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"

namespace ff
{
	class __declspec(uuid("b38739e9-19e5-4cec-8065-6bb784f7ad39"))
		GraphicFactory
			: public ComBase
			, public IGraphicFactory
			, public IComListener
	{
	public:
		DECLARE_HEADER(GraphicFactory);

		// IComListener
		virtual void OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj) override;
		virtual void OnDestruct (REFGUID catid, REFGUID clsid, IUnknown *pObj) override;

		// IGraphicFactory
		virtual IDXGIFactoryX *GetDXGI() override;

		virtual bool CreateDevice(IDXGIAdapterX *pCard, IGraphDevice **device) override;
		virtual bool CreateSoftwareDevice(IGraphDevice **device) override;

		virtual size_t GetDeviceCount() const override;
		virtual IGraphDevice *GetDevice(size_t nIndex) const override;

		virtual bool GetAdapters(Vector<ComPtr<IDXGIAdapterX>> &cards) override;
		virtual bool GetOutputs(IDXGIAdapterX *pCard, Vector<ComPtr<IDXGIOutputX>> &outputs) override;

	#if !METRO_APP
		virtual bool GetAdapterForWindow(HWND hwnd, IDXGIAdapterX **ppCard, IDXGIOutputX **ppOutput) override;
		virtual bool GetAdapterForMonitor(HMONITOR hMonitor, IDXGIAdapterX **ppCard, IDXGIOutputX **ppOutput) override;
	#endif

	private:
		Mutex _mutex;
		ComPtr<IProxyComListener> _listener;
		ComPtr<IDXGIFactoryX> _dxgi;
		Vector<IGraphDevice *> _devices;
	};
}

BEGIN_INTERFACES(ff::GraphicFactory)
	HAS_INTERFACE(ff::IGraphicFactory)
	HAS_INTERFACE(ff::IComListener)
END_INTERFACES()

bool ff::CreateGraphicFactory(IGraphicFactory **ppObj)
{
	assertRetVal(ppObj, false);
	*ppObj = nullptr;

	ComPtr<GraphicFactory, IGraphicFactory> pObj;
	assertRetVal(SUCCEEDED(ComAllocator<GraphicFactory>::CreateInstance(&pObj)), false);
	*ppObj = pObj.Detach();
	return *ppObj != nullptr;
}

bool ff::GetParentDXGI(IUnknown *pObject, REFGUID iid, void **ppParent)
{
	assertRetVal(pObject && ppParent, false);

	ComPtr<IDXGIObject> pObjDXGI;
	assertRetVal(pObjDXGI.QueryFrom(pObject), false);

	ComPtr<IDXGIObject> pParentDXGI;
	assertRetVal(SUCCEEDED(pObjDXGI->GetParent(__uuidof(IDXGIObject), (void**)&pParentDXGI)) && pParentDXGI, false);

	assertRetVal(SUCCEEDED(pParentDXGI->QueryInterface(iid, ppParent)), false);
	return true;
}

ff::GraphicFactory::GraphicFactory()
{
	verify(CreateProxyComListener(this, &_listener));
}

ff::GraphicFactory::~GraphicFactory()
{
	_listener->SetOwner(nullptr);
}

size_t ff::GraphicFactory::GetDeviceCount() const
{
	return _devices.Size();
}

ff::IGraphDevice *ff::GraphicFactory::GetDevice(size_t nIndex) const
{
	assertRetVal(nIndex >= 0 && nIndex < _devices.Size(), nullptr);
	return _devices[nIndex];
}

IDXGIFactoryX *ff::GraphicFactory::GetDXGI()
{
	if (!_dxgi)
	{
		LockMutex crit(_mutex);

		if (!_dxgi)
		{
			assertRetVal(SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactoryX), (void**)&_dxgi)), nullptr);
		}
	}

	return _dxgi;
}

bool ff::GraphicFactory::CreateDevice(IDXGIAdapterX *pCard, IGraphDevice **device)
{
	assertRetVal(device && GetDXGI(), false);
	*device = nullptr;

	ComPtr<IGraphDevice> pDevice;

	if (pCard)
	{
		assertRetVal(CreateGraphDevice(pCard, &pDevice), false);
	}
	else
	{
		assertRetVal(CreateHardwareGraphDevice(&pDevice), false);
	}

	_devices.Push(pDevice);
	AddComListener(pDevice.Interface(), _listener);
	*device = pDevice.Detach();

	return true;
}

bool ff::GraphicFactory::CreateSoftwareDevice(IGraphDevice **device)
{
	assertRetVal(device && GetDXGI(), false);
	*device = nullptr;

	ComPtr<IGraphDevice> pDevice;
	assertRetVal(CreateGraphDevice(nullptr, &pDevice), false);

	_devices.Push(pDevice);
	AddComListener(pDevice.Interface(), _listener);
	*device = pDevice.Detach();

	return true;
}

bool ff::GraphicFactory::GetAdapters(Vector<ComPtr<IDXGIAdapterX>> &cards)
{
	ComPtr<IDXGIAdapter1> pCard;

	for (UINT i = 0; GetDXGI() && SUCCEEDED(GetDXGI()->EnumAdapters1(i++, &pCard)); pCard = nullptr)
	{
		ComPtr<IDXGIAdapterX> pCardX;
		if (pCardX.QueryFrom(pCard))
		{
			cards.Push(pCardX);
		}
	}

	return cards.Size() > 0;
}

bool ff::GraphicFactory::GetOutputs(IDXGIAdapterX *pCard, Vector<ComPtr<IDXGIOutputX>> &outputs)
{
	ComPtr<IDXGIOutput>   pOutput;
	ComPtr<IDXGIAdapter1> pDefaultCard;
	ComPtr<IDXGIAdapterX> pDefaultCardX;

	if (!pCard && SUCCEEDED(GetDXGI()->EnumAdapters1(0, &pDefaultCard)))
	{
		if (pDefaultCardX.QueryFrom(pDefaultCard))
		{
			pCard = pDefaultCardX;
		}
	}

	assertRetVal(pCard, false);

	for (UINT i = 0; pCard && SUCCEEDED(pCard->EnumOutputs(i++, &pOutput)); pOutput = nullptr)
	{
		ComPtr<IDXGIOutputX> pOutputX;
		if (pOutputX.QueryFrom(pOutput))
		{
			outputs.Push(pOutputX);
		}
	}

	return outputs.Size() > 0;
}

#if !METRO_APP
bool ff::GraphicFactory::GetAdapterForWindow(HWND hwnd, IDXGIAdapterX **ppCard, IDXGIOutput **ppOutput)
{
	assertRetVal(hwnd, false);

	HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
	return GetAdapterForMonitor(hMonitor, ppCard, ppOutput);
}
#endif

#if !METRO_APP
static bool DoesAdapterUseMonitor(IDXGIAdapterX *pCard, HMONITOR hMonitor, IDXGIOutput **ppOutput)
{
	assertRetVal(pCard, false);

	ff::ComPtr<IDXGIOutputX> pOutput;
	for (UINT i = 0; pCard && SUCCEEDED(pCard->EnumOutputs(i, &pOutput)); i++, pOutput = nullptr)
	{
		DXGI_OUTPUT_DESC desc;
		ff::ZeroObject(desc);

		if (SUCCEEDED(pOutput->GetDesc(&desc)) && desc.Monitor == hMonitor)
		{
			if (ppOutput)
			{
				*ppOutput = pOutput.Detach();
			}

			return true;
		}
	}

	return false;
}
#endif

#if !METRO_APP
bool ff::GraphicFactory::GetAdapterForMonitor(HMONITOR hMonitor, IDXGIAdapterX **ppCard, IDXGIOutputX **ppOutput)
{
	assertRetVal(hMonitor, false);

	// Try the desktop first
	{
		ComPtr<IGraphDevice> pDevice;
		if (CreateHardwareGraphDevice(&pDevice))
		{
			if (DoesAdapterUseMonitor(pDevice->GetAdapter(), hMonitor, ppOutput))
			{
				if (ppCard)
				{
					*ppCard = GetAddRef(pDevice->GetAdapter());
				}

				return true;
			}
		}
	}

	Vector<ComPtr<IDXGIAdapterX>> cards;
	if (GetAdapters(cards))
	{
		for (size_t i = 0; i < cards.Size(); i++)
		{
			if (DoesAdapterUseMonitor(cards[i], hMonitor, ppOutput))
			{
				if (ppCard)
				{
					*ppCard = cards[i].AddRef();
				}

				return true;
			}
		}
	}

	assertRetVal(false, false);
}
#endif

void ff::GraphicFactory::OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	assert(false);
}

void ff::GraphicFactory::OnDestruct(REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	ComPtr<IGraphDevice> pDevice;
	assertRet(pDevice.QueryFrom(pObj));

	for (size_t i = 0; i < _devices.Size(); i++)
	{
		if (_devices[i] == pDevice)
		{
			_devices.Delete(i);
			return;
		}
	}

	assertRet(false);
}
