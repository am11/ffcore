#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"

namespace ff
{

// STATIC_DATA (object)
static Vector<IThreadPool *, 8> s_threadPools;

Vector<IThreadPool *, 8> &GetAllThreadPools()
{
	return s_threadPools;
}

class __declspec(uuid("b95c5be0-181f-4561-9160-3cfc3b4695c6"))
	CProxyWorkItemListener : public ComBase, public IProxyWorkItemListener
{
public:
	DECLARE_HEADER(CProxyWorkItemListener);

	// IProxyWorkItemListener
	virtual void SetOwner  (IWorkItemListener *pOwner) override;
	virtual void OnComplete(IWorkItem *pWork)          override;
	virtual void OnCancel  (IWorkItem *pWork)          override;

private:
	IWorkItemListener *m_pOwner;
};


static bool CreateProxyWorkItemListener(IWorkItemListener *owner, CProxyWorkItemListener **obj)
{
	assertHrRetVal(ComAllocator<CProxyWorkItemListener>::CreateInstance(obj), false);
	(*obj)->SetOwner(owner);
	return true;
}


bool CreateProxyWorkItemListener(IWorkItemListener *owner, IProxyWorkItemListener **obj)
{
	assertRetVal(obj, false);

	ComPtr<CProxyWorkItemListener> pProxy;
	assertRetVal(CreateProxyWorkItemListener(owner, &pProxy), false);

	*obj = pProxy.Detach();
	return true;
}


BEGIN_INTERFACES(CProxyWorkItemListener)
	HAS_INTERFACE(IProxyWorkItemListener)
	HAS_INTERFACE(IWorkItemListener)
END_INTERFACES()


CProxyWorkItemListener::CProxyWorkItemListener()
	: m_pOwner(nullptr)
{
}


CProxyWorkItemListener::~CProxyWorkItemListener()
{
	assert(!m_pOwner);
}


void CProxyWorkItemListener::SetOwner(IWorkItemListener *pOwner)
{
	m_pOwner = pOwner;
}


void CProxyWorkItemListener::OnComplete(IWorkItem *pWork)
{
	assert(IsRunningOnMainThread());

	if (m_pOwner)
	{
		m_pOwner->OnComplete(pWork);
	}
}


void CProxyWorkItemListener::OnCancel(IWorkItem *pWork)
{
	assert(IsRunningOnMainThread());

	if (m_pOwner)
	{
		m_pOwner->OnCancel(pWork);
	}
}



BEGIN_INTERFACES(IWorkItem)
	HAS_INTERFACE(IWorkItem)
END_INTERFACES()


IWorkItem::IWorkItem()
{
}


IWorkItem::~IWorkItem()
{
	assert(_listeners == nullptr);
}


// multi-threaded
void IWorkItem::Run()
{
	// this should be overridden
}


// main thread only
void IWorkItem::OnCancel()
{
	assert(IsRunningOnMainThread());
}


// main thread only
void IWorkItem::OnComplete()
{
	assert(IsRunningOnMainThread());
}


// main thread only
void IWorkItem::InternalOnCancel()
{
	assert(IsRunningOnMainThread());

	OnCancel();

	for (size_t i = 0; _listeners && i < _listeners->Size(); i++)
	{
		if (_listeners->GetAt(i))
		{
			_listeners->GetAt(i)->OnCancel(this);
			_listeners->SetAt(i, nullptr);
		}
	}

	_listeners.reset();
}


// main thread only
void IWorkItem::InternalOnComplete()
{
	assert(IsRunningOnMainThread());

	OnComplete();

	for (size_t i = 0; _listeners && i < _listeners->Size(); i++)
	{
		if (_listeners->GetAt(i))
		{
			_listeners->GetAt(i)->OnComplete(this);
			_listeners->SetAt(i, nullptr);
		}
	}

	_listeners.reset();
}


int IWorkItem::GetPriority() const
{
	return s_nDefaultPriority;
}


void IWorkItem::AddListener(IWorkItemListener *pListener)
{
	assert(IsRunningOnMainThread());

	if (_listeners == nullptr)
	{
		_listeners.reset(new Vector<ComPtr<IWorkItemListener>>);
	}

	ComPtr<IWorkItemListener> pMyListener = pListener;
	assertRet(_listeners->Find(pMyListener) == INVALID_SIZE);

	_listeners->Push(pMyListener);
}


bool IWorkItem::AddProxyListener(IWorkItemListener *pListener, IProxyWorkItemListener **ppProxy)
{
	assert(IsRunningOnMainThread());

	assertRetVal(pListener && ppProxy, false);

	ComPtr<CProxyWorkItemListener> pProxy;
	assertRetVal(CreateProxyWorkItemListener(pListener, &pProxy), false);

	AddListener(pProxy);
	*ppProxy = pProxy.Detach();

	return true;
}

void IWorkItem::RemoveListener(IWorkItemListener *pListener)
{
	assert(IsRunningOnMainThread());

	assertRet(pListener && _listeners);

	ComPtr<IWorkItemListener> pMyListener = pListener;
	size_t i = _listeners->Find(pMyListener);
	assertRet(i != INVALID_SIZE);

	pMyListener = nullptr;
	_listeners->SetAt(i, pMyListener);
}


void FlushAllThreadPoolWork()
{
	Vector<ComPtr<IThreadPool>> pools;

	// save all the current pools just in case they change while iterating
	{
		LockMutex crit(GCS_THREAD_POOL);

		for (size_t i = 0; i < GetAllThreadPools().Size(); i++)
		{
			pools.Push(GetAllThreadPools().GetAt(i));
		}
	}

	for (size_t i = 0; i < pools.Size(); i++)
	{
		pools[i]->Flush();
	}

	FlushMainThreadFunctions();
}


// STATIC_DATA (pod)
static long s_nSuspendThreads = 0;

void SuspendAllWorkerThreads()
{
	assert(IsRunningOnMainThread());

	if (InterlockedIncrement(&s_nSuspendThreads) == 1)
	{
		LockMutex crit(GCS_THREAD_POOL);

		for (size_t i = 0; i < GetAllThreadPools().Size(); i++)
		{
			GetAllThreadPools().GetAt(i)->Suspend();
		}
	}
}


void ResumeAllWorkerThreads()
{
	assert(IsRunningOnMainThread());

	if (!InterlockedDecrement(&s_nSuspendThreads))
	{
		LockMutex crit(GCS_THREAD_POOL);

		for (size_t i = 0; i < GetAllThreadPools().Size(); i++)
		{
			GetAllThreadPools().GetAt(i)->Resume();
		}
	}
}


CScopeBlockAllWorkerThreads::CScopeBlockAllWorkerThreads()
{
	SuspendAllWorkerThreads();
}


CScopeBlockAllWorkerThreads::~CScopeBlockAllWorkerThreads()
{
	ResumeAllWorkerThreads();
}


} // end namespace ff
