#include "pch.h"
#include "COM/ComAlloc.h"
#include "COM/ComListener.h"
#include "Graph/BufferCache.h"
#include "Graph/Data/GraphCategory.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphDeviceChild.h"
#include "Graph/GraphFactory.h"
#include "Graph/GraphStateCache.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Module/Module.h"

namespace ff
{
	class __declspec(uuid("dce9ac0d-79da-4456-b441-7bde392df877"))
		GraphDevice
			: public ComBase
			, public IGraphDevice
			, public IComListener
	{
	public:
		DECLARE_HEADER(GraphDevice);

		bool Init(IDXGIAdapterX *pCard, bool bSoftware);

		// IComListener functions
		virtual void OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj) override;
		virtual void OnDestruct (REFGUID catid, REFGUID clsid, IUnknown *pObj) override;

		// IGraphDevice functions
		virtual void Reset() override;
		virtual bool IsSoftware() const override;

		virtual ID3D11DeviceX *GetDX() override;
		virtual IDXGIDeviceX *GetDXGI() override;
		virtual ID3D11DeviceContextX *GetContext() override;
		virtual IDXGIAdapterX *GetAdapter() override;
		virtual D3D_FEATURE_LEVEL GetFeatureLevel() const override;

		virtual BufferCache &GetVertexBuffers() override;
		virtual BufferCache &GetIndexBuffers() override;
		virtual GraphStateCache &GetStateCache() override;

	private:
		ComPtr<ID3D11DeviceX> _device;
		ComPtr<ID3D11DeviceContextX> _context;
		ComPtr<IDXGIAdapterX> _adapter;
		ComPtr<IDXGIDeviceX> _dxgiDevice;
		ComPtr<IProxyComListener> _listener;
		Vector<IGraphDeviceChild *> _children;
		BufferCache _vertexCache;
		BufferCache _indexCache;
		GraphStateCache _stateCache;
		D3D_FEATURE_LEVEL _featureLevel;
		bool _softwareDevice;
	};
}

BEGIN_INTERFACES(ff::GraphDevice)
	HAS_INTERFACE(ff::IGraphDevice)
	HAS_INTERFACE(ff::IComListener)
END_INTERFACES()

bool ff::CreateHardwareGraphDevice(IGraphDevice **device)
{
	assertRetVal(device, false);

	ComPtr<GraphDevice, IGraphDevice> pDevice;
	assertRetVal(SUCCEEDED(ComAllocator<GraphDevice>::CreateInstance(&pDevice)), false);

	assertRetVal(pDevice->Init(nullptr, false), false);
	*device = pDevice.Detach();

	return true;
}

bool ff::CreateSoftwareGraphDevice(IGraphDevice **device)
{
	assertRetVal(device, false);

	ComPtr<GraphDevice, IGraphDevice> pDevice;
	assertRetVal(SUCCEEDED(ComAllocator<GraphDevice>::CreateInstance(&pDevice)), false);

	assertRetVal(pDevice->Init(nullptr, true), false);
	*device = pDevice.Detach();

	return true;
}

bool ff::CreateGraphDevice(IDXGIAdapterX *pCard, IGraphDevice **device)
{
	assertRetVal(pCard && device, false);

	ComPtr<GraphDevice, IGraphDevice> pDevice;
	assertRetVal(SUCCEEDED(ComAllocator<GraphDevice>::CreateInstance(&pDevice)), false);

	assertRetVal(pDevice->Init(pCard, false), false);
	*device = pDevice.Detach();

	return true;
}

ff::GraphDevice::GraphDevice()
	: _vertexCache(D3D11_BIND_VERTEX_BUFFER)
	, _indexCache(D3D11_BIND_INDEX_BUFFER)
	, _featureLevel((D3D_FEATURE_LEVEL)0)
	, _softwareDevice(false)
{
	_vertexCache.SetDevice(this);
	_indexCache.SetDevice(this);
	_stateCache.SetDevice(this);

	verify(CreateProxyComListener(this, &_listener));
	verify(AddComListener(GetCategoryGraphicsObject(), _listener));
}

ff::GraphDevice::~GraphDevice()
{
	assert(!_children.Size());

	if (_context)
	{
		_context->ClearState();
	}

	_vertexCache.SetDevice(nullptr);
	_indexCache.SetDevice(nullptr);
	_stateCache.SetDevice(nullptr);

	verify(RemoveComListener(GetCategoryGraphicsObject(), _listener));
	_listener->SetOwner(nullptr);
}

static bool InternalCreateDevice(
	IDXGIAdapterX *pCard,
	bool bSoftware,
	ID3D11DeviceX **device,
	ID3D11DeviceContextX **ppContext,
	D3D_FEATURE_LEVEL *pFeatureLevel)
{
	assertRetVal(!bSoftware || !pCard, false);
	assertRetVal(device && ppContext, false);

	D3D_DRIVER_TYPE driverType = pCard
		? D3D_DRIVER_TYPE_UNKNOWN
		: (bSoftware ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE);

	UINT nFlags = ff::GetThisModule().IsDebugBuild() ? D3D11_CREATE_DEVICE_DEBUG : 0;

	const D3D_FEATURE_LEVEL featureLevels[] =
	{
#if METRO_APP
		D3D_FEATURE_LEVEL_11_1,
#endif
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

#if defined(_DEBUG) && !METRO_APP
	// The VS graphics helper will crash if I give it an adapter to use, so clear it
	if (pCard && ::GetModuleHandle(L"dxcapturereplay.dll"))
	{
		pCard = nullptr;
		driverType = D3D_DRIVER_TYPE_HARDWARE;
	}
#endif

	ff::ComPtr<ID3D11Device>         pDevice;
	ff::ComPtr<ID3D11DeviceX>        pDeviceX;
	ff::ComPtr<ID3D11DeviceContext>  pContext;
	ff::ComPtr<ID3D11DeviceContextX> pContextX;

	assertRetVal(SUCCEEDED(D3D11CreateDevice(
		pCard, driverType, nullptr, nFlags,
		featureLevels, _countof(featureLevels),
		D3D11_SDK_VERSION, &pDevice, pFeatureLevel, &pContext)), false);

	assertRetVal(pDeviceX.QueryFrom(pDevice), false);
	assertRetVal(pContextX.QueryFrom(pContext), false);

	*device = pDeviceX.Detach();
	*ppContext = pContextX.Detach();

	return true;
}

bool ff::GraphDevice::Init(IDXGIAdapterX *pCard, bool bSoftware)
{
	assertRetVal(!bSoftware || !pCard, false);
	assertRetVal(!_device && !_context, false);

	_featureLevel = (D3D_FEATURE_LEVEL)0;
	_softwareDevice = bSoftware;

	assertRetVal(InternalCreateDevice(pCard, bSoftware, &_device, &_context, &_featureLevel), false);
	assertRetVal(GetParentDXGI(_device, __uuidof(IDXGIAdapterX), (void**)&_adapter), false);
	assertRetVal(_dxgiDevice.QueryFrom(_device), false);

	return true;
}

ff::BufferCache &ff::GraphDevice::GetVertexBuffers()
{
	return _vertexCache;
}

ff::BufferCache &ff::GraphDevice::GetIndexBuffers()
{
	return _indexCache;
}

ff::GraphStateCache &ff::GraphDevice::GetStateCache()
{
	return _stateCache;
}

void ff::GraphDevice::OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	ComPtr<IGraphDeviceChild> pChild;
	ComPtr<IRenderTarget>     pRenderer;

	if (pChild.QueryFrom(pObj) && pChild->GetDevice() == this)
	{
		_children.Push(pChild);
	}
}

void ff::GraphDevice::OnDestruct(REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	ComPtr<IGraphDeviceChild> pChild;
	ComPtr<IRenderTarget>  pRenderer;

	if (pChild.QueryFrom(pObj) && pChild->GetDevice() == this)
	{
		size_t i = _children.Find(pChild);
		assertRet(i != INVALID_SIZE);
		_children.Delete(i);
	}
}

void ff::GraphDevice::Reset()
{
	ComPtr<IDXGIAdapterX> pAdapter = GetAdapter();

	if (_context)
	{
		_context->ClearState();
	}

	_vertexCache.Reset();
	_indexCache.Reset();
	_stateCache.Reset();

	_context  = nullptr;
	_device = nullptr;
	_adapter  = nullptr;
	_dxgiDevice   = nullptr;

	if (Init(pAdapter, _softwareDevice))
	{
		for (size_t i = 0; i < _children.Size(); i++)
		{
			_children[i]->Reset();
		}
	}
}

bool ff::GraphDevice::IsSoftware() const
{
	return _softwareDevice;
}

ID3D11DeviceX *ff::GraphDevice::GetDX()
{
	return _device;
}

IDXGIDeviceX *ff::GraphDevice::GetDXGI()
{
	return _dxgiDevice;
}

ID3D11DeviceContextX *ff::GraphDevice::GetContext()
{
	return _context;
}

IDXGIAdapterX *ff::GraphDevice::GetAdapter()
{
	return _adapter;
}

D3D_FEATURE_LEVEL ff::GraphDevice::GetFeatureLevel() const
{
	assert(_featureLevel != 0);
	return _featureLevel;
}
