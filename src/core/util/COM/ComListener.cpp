#include "pch.h"
#include "COM/ComListener.h"
#include "COM/ComObject.h"

// STATIC_DATA (pod)
static bool s_bRemovedListener = false;
static bool s_bRemovedObjectListener = false;
static int  s_nListenerNestCount = 0;

typedef ff::Map<GUID, ff::ComPtr<ff::IComListener>> GlobalComListenerMap;
typedef ff::Map<IUnknown*, ff::ComPtr<ff::IComListener>> GlobalComObjectListenerMap;

// STATIC_DATA (object)
static GlobalComListenerMap s_comListenerMap;
static GlobalComListenerMap &GetGlobalComListeners()
{
	return s_comListenerMap;
}

// STATIC_DATA (object)
static GlobalComObjectListenerMap s_objectListenerMap;
static GlobalComObjectListenerMap &GetGlobalComObjectListeners()
{
	return s_objectListenerMap;
}

bool ff::AddComListener(REFGUID catid, IComListener *pListener)
{
	LockMutex crit(GCS_COM_LISTENER);

	assertRetVal(pListener, false);

	GlobalComListenerMap &map = GetGlobalComListeners();

	// Check for a dupe

	for (BucketIter i = map.Get(catid); i != INVALID_ITER; i = map.GetNext(i))
	{
		assertRetVal(map.ValueAt(i) != pListener, false);
	}

	if (catid != GUID_NULL)
	{
		// Check global listeners for a dupe too

		for (BucketIter i = map.Get(GUID_NULL); i != INVALID_ITER; i = map.GetNext(i))
		{
			assertRetVal(map.ValueAt(i) != pListener, false);
		}
	}

	map.Insert(catid, ComPtr<IComListener>(pListener));

	return true;
}

bool ff::RemoveComListener(REFGUID catid, IComListener *pListener)
{
	LockMutex crit(GCS_COM_LISTENER);

	assertRetVal(pListener, false);

	GlobalComListenerMap &map = GetGlobalComListeners();

	if (catid == GUID_NULL)
	{
		// Loop through all categories

		for (BucketIter i = map.StartIteration(); i != INVALID_ITER; i = map.Iterate(i))
		{
			if (map.ValueAt(i) == pListener)
			{
				map.ValueAt(i).Release();
				s_bRemovedListener = true;
				return true;
			}
		}
	}
	else
	{
		for (BucketIter i = map.Get(catid); i != INVALID_ITER; i = map.GetNext(i))
		{
			if (map.ValueAt(i) == pListener)
			{
				map.ValueAt(i).Release();
				s_bRemovedListener = true;
				return true;
			}
		}
	}

	assertRetVal(false, false);
}

bool ff::AddComListener(IUnknown *pObj, IComListener *pListener)
{
	LockMutex crit(GCS_COM_LISTENER);

	ComPtr<IUnknown> pObjCanon;
	assertRetVal(pObjCanon.QueryFrom(pObj), false);

	GlobalComObjectListenerMap &map = GetGlobalComObjectListeners();

	// Check for a dupe

	for (BucketIter i = map.Get(pObjCanon); i != INVALID_ITER; i = map.GetNext(i))
	{
		assertRetVal(map.ValueAt(i) != pListener, false);
	}

	map.Insert(pObjCanon, ComPtr<IComListener>(pListener));

	return true;
}

bool ff::RemoveComListener(IUnknown *pObj, IComListener *pListener)
{
	LockMutex crit(GCS_COM_LISTENER);

	ComPtr<IUnknown> pObjCanon;
	assertRetVal(pObjCanon.QueryFrom(pObj), false);

	GlobalComObjectListenerMap &map = GetGlobalComObjectListeners();

	for (BucketIter i = map.Get(pObjCanon); i != INVALID_ITER; i = map.GetNext(i))
	{
		if (map.ValueAt(i) == pListener)
		{
			map.ValueAt(i).Release();
			s_bRemovedObjectListener = true;
			return true;
		}
	}

	assertRetVal(false, false);
}

static void CleanListeners()
{
	assert(!s_nListenerNestCount);

	GlobalComListenerMap &map = GetGlobalComListeners();
	GlobalComObjectListenerMap &objectMap = GetGlobalComObjectListeners();

	if (s_bRemovedListener)
	{
		for (ff::BucketIter i = map.StartIteration(); i != ff::INVALID_ITER; )
		{
			i = !map.ValueAt(i) ? map.DeletePos(i) : map.Iterate(i);
		}

		s_bRemovedListener = false;
	}

	if (s_bRemovedObjectListener)
	{
		for (ff::BucketIter i = objectMap.StartIteration(); i != ff::INVALID_ITER; )
		{
			i = !objectMap.ValueAt(i) ? objectMap.DeletePos(i) : objectMap.Iterate(i);
		}

		s_bRemovedObjectListener = false;
	}
}

// static
void ff::IComListener::CallOnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	LockMutex crit(GCS_COM_LISTENER);
	s_nListenerNestCount++;

	GlobalComListenerMap &map = GetGlobalComListeners();

	// Notify listeners of a specific category

	for (BucketIter i = map.Get(catid); i != INVALID_ITER; i = map.GetNext(i))
	{
		IComListener *pListener = map.ValueAt(i);

		if (pListener)
		{
			pListener->OnConstruct(unkOuter, catid, clsid, pObj);
		}
	}

	// Notify global listeners (unless they were already notified above)

	if (catid != GUID_NULL)
	{
		for (BucketIter i = map.Get(GUID_NULL); i != INVALID_ITER; i = map.GetNext(i))
		{
			IComListener *pListener = map.ValueAt(i);

			if (pListener)
			{
				pListener->OnConstruct(unkOuter, catid, clsid, pObj);
			}
		}
	}

	if (!--s_nListenerNestCount)
	{
		CleanListeners();
	}
}

// static
void ff::IComListener::CallOnDestruct(REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	LockMutex crit(GCS_COM_LISTENER);
	assertRet(pObj);
	s_nListenerNestCount++;

	GlobalComListenerMap&     map       = GetGlobalComListeners();
	GlobalComObjectListenerMap&  objectMap = GetGlobalComObjectListeners();

	// Notify listeners of a specific category

	for (BucketIter i = map.Get(catid); i != INVALID_ITER; i = map.GetNext(i))
	{
		IComListener *pListener = map.ValueAt(i);

		if (pListener)
		{
			pListener->OnDestruct(catid, clsid, pObj);
		}
	}

	// Notify global listeners (unless they were already notified above)

	if (catid != GUID_NULL)
	{
		for (BucketIter i = map.Get(GUID_NULL); i != INVALID_ITER; i = map.GetNext(i))
		{
			IComListener *pListener = map.ValueAt(i);

			if (pListener)
			{
				pListener->OnDestruct(catid, clsid, pObj);
			}
		}
	}

	// Notify single object listeners

	for (BucketIter i = objectMap.Get(pObj); i != INVALID_ITER; i = objectMap.GetNext(i))
	{
		IComListener *pListener = objectMap.ValueAt(i);

		if (pListener)
		{
			pListener->OnDestruct(catid, clsid, pObj);
			objectMap.ValueAt(i).Release();
			s_bRemovedObjectListener = true;
		}
	}

	if (!--s_nListenerNestCount)
	{
		CleanListeners();
	}
}

class __declspec(uuid("c1ee4dd7-7a0a-4758-aa9b-5c64498a21f1"))
	CProxyComListener : public ff::ComBase, public ff::IProxyComListener
{
public:
	DECLARE_HEADER(CProxyComListener);

	// Must call SetOwner(nullptr) when the owner is destroyed
	virtual void SetOwner(ff::IComListener *pOwner) override;

	virtual void OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj) override;
	virtual void OnDestruct (REFGUID catid, REFGUID clsid, IUnknown *pObj) override;

private:
	ff::IComListener *_owner;
};

BEGIN_INTERFACES(CProxyComListener)
	HAS_INTERFACE(ff::IComListener)
	HAS_INTERFACE(ff::IProxyComListener)
END_INTERFACES()

bool ff::CreateProxyComListener(IComListener *pOwner, IProxyComListener **ppProxy)
{
	assertRetVal(ppProxy, false);
	*ppProxy = nullptr;

	ComPtr<CProxyComListener> pProxy = new ComObject<CProxyComListener>();
	pProxy->SetOwner(pOwner);

	*ppProxy = pProxy.Detach();
	return *ppProxy != nullptr;
}

CProxyComListener::CProxyComListener()
	: _owner(nullptr)
{
}

CProxyComListener::~CProxyComListener()
{
	assert(!_owner);
}

void CProxyComListener::SetOwner(ff::IComListener *pOwner)
{
	_owner = pOwner;
}

void CProxyComListener::OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	if (_owner)
	{
		_owner->OnConstruct(unkOuter, catid, clsid, pObj);
	}
}

void CProxyComListener::OnDestruct(REFGUID catid, REFGUID clsid, IUnknown *pObj)
{
	if (_owner)
	{
		_owner->OnDestruct(catid, clsid, pObj);
	}
}
