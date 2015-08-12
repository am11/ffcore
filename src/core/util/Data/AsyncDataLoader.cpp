#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/AsyncDataLoader.h"
#include "Data/SavedData.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"

namespace ff
{
	class CAsyncDataLoader;

	class __declspec(uuid("0e85e7ea-89bf-46ac-9db0-ec57e3f83404"))
		CProxyAsyncDataListener : public ComBase, public IProxyAsyncDataListener
	{
	public:
		DECLARE_HEADER(CProxyAsyncDataListener);

		// IProxyAsyncDataListener
		virtual void SetOwner  (IAsyncDataListener* pOwner)   override;
		virtual void OnComplete(ISavedData *pData, EDataWorkType type) override;
		virtual void OnCancel  (ISavedData *pData, EDataWorkType type) override;

	private:
		IAsyncDataListener *_owner;
	};
}

BEGIN_INTERFACES(ff::CProxyAsyncDataListener)
	HAS_INTERFACE(ff::IProxyAsyncDataListener)
	HAS_INTERFACE(ff::IAsyncDataListener)
END_INTERFACES()

bool ff::CreateProxyAsyncDataListener(IAsyncDataListener *pOwner, IProxyAsyncDataListener **ppListener)
{
	assertRetVal(pOwner && ppListener, false);
	*ppListener = nullptr;

	ComPtr<CProxyAsyncDataListener> pListener;
	assertRetVal(SUCCEEDED(ComAllocator<CProxyAsyncDataListener>::CreateInstance(&pListener)), false);
	pListener->SetOwner(pOwner);

	*ppListener = pListener.Detach();
	return true;
}

ff::CProxyAsyncDataListener::CProxyAsyncDataListener()
	: _owner(nullptr)
{
}

ff::CProxyAsyncDataListener::~CProxyAsyncDataListener()
{
	assert(!_owner);
}

void ff::CProxyAsyncDataListener::SetOwner(IAsyncDataListener *pOwner)
{
	_owner = pOwner;
}

void ff::CProxyAsyncDataListener::OnComplete(ISavedData *pData, EDataWorkType type)
{
	assert(IsRunningOnMainThread());

	if (_owner)
	{
		_owner->OnComplete(pData, type);
	}
}

void ff::CProxyAsyncDataListener::OnCancel(ISavedData *pData, EDataWorkType type)
{
	assert(IsRunningOnMainThread());

	if (_owner)
	{
		_owner->OnCancel(pData, type);
	}
}

namespace ff
{
	class __declspec(uuid("474d7cff-7afc-4b94-b5bd-cb5ae4f5a8c6"))
		CLoadDataWorkItem : public IWorkItem
	{
	public:
		CLoadDataWorkItem();
		~CLoadDataWorkItem();

		bool Init(CAsyncDataLoader *pLoader, ISavedData *pData, EDataWorkType type, int nPriority, IAsyncDataListener *pListener);

		virtual void Run() override;
		virtual void OnCancel() override;
		virtual void OnComplete() override;
		virtual int  GetPriority() const override;

		ISavedData *GetData();
		size_t GetListenerCount();
		IAsyncDataListener *GetListener(size_t nIndex);
		void AddListener(IAsyncDataListener *pListener);
		EDataWorkType GetWorkType() const;

	private:
		ComPtr<ISavedData> _data;
		ComPtr<ISavedData> _clone;
		ComPtr<CAsyncDataLoader> _loader;
		Vector<ComPtr<IAsyncDataListener>> _listeners;
		EDataWorkType _type;
		int _priority;
	};
}

static bool CreateLoadDataWorkItem(
		ff::CAsyncDataLoader *pLoader,
		ff::ISavedData *pData,
		ff::EDataWorkType type,
		int nPriority,
		ff::IAsyncDataListener *pListener,
		ff::CLoadDataWorkItem **ppWork)
{
	assertRetVal(ppWork && pData && pLoader, false);
	*ppWork = nullptr;

	ff::ComPtr<ff::CLoadDataWorkItem> pWork = new ff::ComObject<ff::CLoadDataWorkItem>;
	assertRetVal(pWork && pWork->Init(pLoader, pData, type, nPriority, pListener), false);

	*ppWork = pWork.Detach();

	return *ppWork != nullptr;
}

namespace ff
{
	class __declspec(uuid("8ede0d7b-82cd-43c2-a7e0-40e18b5721ed"))
		CAsyncDataLoader : public ComBase, public IAsyncDataLoader
	{
	public:
		DECLARE_HEADER(CAsyncDataLoader);

		void AddLoading (size_t nSize);
		void AddProgress(size_t nSize);

		// IAsyncDataLoader functions

		virtual bool Load      (ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener) override;
		virtual bool Unload    (ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener) override;
		virtual bool SaveToMem (ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener) override;
		virtual bool SaveToFile(ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener) override;

		virtual bool Wait      (ISavedData *pData) override;
		virtual bool Wait      (const IAsyncDataListener *pListener) override;
		virtual bool Cancel    (ISavedData *pData) override;
		virtual bool Cancel    (const IAsyncDataListener *pListener) override;

		virtual bool Flush() override;
		virtual bool CancelAll() override;

		virtual void AddListener   (IAsyncDataListener *pListener) override;
		virtual void RemoveListener(IAsyncDataListener *pListener) override;

		virtual bool IsLoading() const override;
		virtual void ClearTotalLoadedBytes() override;
		virtual bool GetProgress(size_t &nSize, size_t &nTotal) const override;

		// Work item functions (called on the main thread from work items)

		void OnComplete(ISavedData *pData, EDataWorkType type);
		void OnCancel  (ISavedData *pData, EDataWorkType type);

	private:
		bool               InternalLoadOrSave (ISavedData *pData, EDataWorkType type, int nPriority, IAsyncDataListener *pListener);
		bool               CreateWorkItem     (ISavedData *pData, EDataWorkType type, int nPriority, IAsyncDataListener *pListener);
		bool               ReleaseData        (ISavedData *pData);
		CLoadDataWorkItem* FindWorkItemForData(ISavedData *pData);
		CLoadDataWorkItem* FindWorkItemForListener(const IAsyncDataListener *pListener);

		// data
		Vector<ComPtr<CLoadDataWorkItem>>  _workItems;
		Vector<ComPtr<IAsyncDataListener>> _listeners;
		size_t _loading;
		size_t _progress;
	};
}

BEGIN_INTERFACES(ff::CAsyncDataLoader)
	HAS_INTERFACE(ff::IAsyncDataLoader)
END_INTERFACES()

bool ff::CreateAsyncDataLoader(IAsyncDataLoader **ppLoader)
{
	assertRetVal(ppLoader, false);
	*ppLoader = nullptr;

	ComPtr<CAsyncDataLoader> pLoader = new ComObject<CAsyncDataLoader>;
	*ppLoader = pLoader.Detach();

	return *ppLoader != nullptr;
}

ff::CAsyncDataLoader::CAsyncDataLoader()
	: _loading(0)
	, _progress(0)
{
}

ff::CAsyncDataLoader::~CAsyncDataLoader()
{
	Flush();
}

bool ff::CAsyncDataLoader::Load(ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener)
{
	return InternalLoadOrSave(pData, DWT_LOAD,  IWorkItem::s_nDefaultPriority + nPriorityOffset, pListener);
}

bool ff::CAsyncDataLoader::Unload(ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener)
{
	return InternalLoadOrSave(pData, DWT_UNLOAD, IWorkItem::s_nDefaultPriority + nPriorityOffset, pListener);
}

bool ff::CAsyncDataLoader::SaveToMem(ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener)
{
	return InternalLoadOrSave(pData, DWT_SAVE_TO_MEM, IWorkItem::s_nDefaultPriority + nPriorityOffset, pListener);
}

bool ff::CAsyncDataLoader::SaveToFile(ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener)
{
	return InternalLoadOrSave(pData, DWT_SAVE_TO_FILE, IWorkItem::s_nDefaultPriority + nPriorityOffset, pListener);
}

bool ff::CAsyncDataLoader::InternalLoadOrSave(ISavedData *pData, EDataWorkType type, int nPriority, IAsyncDataListener *pListener)
{
	assertRetVal(pData, false);

	for (ComPtr<IAsyncDataLoader> pOtherLoader; pData->GetDataLoader(&pOtherLoader); pOtherLoader = nullptr)
	{
		ComPtr<CLoadDataWorkItem> pWork = (pOtherLoader == this) ? FindWorkItemForData(pData) : nullptr;

		if (pWork && pWork->GetWorkType() == type)
		{
			// I'm already doing the right work for this data

			if (pListener)
			{
				pWork->AddListener(pListener);
			}

			return true;
		}
		else
		{
			// another thread is loading or saving, wait for it to finish
			verify(pOtherLoader->Wait(pData));
		}
	}

	assertRetVal(CreateWorkItem(pData, type, nPriority, pListener), false);

	return true;
}

bool ff::CAsyncDataLoader::CreateWorkItem(ISavedData *pData, EDataWorkType type, int nPriority, IAsyncDataListener *pListener)
{
	assertRetVal(!FindWorkItemForData(pData), false);

	ComPtr<CLoadDataWorkItem> pWork;
	assertRetVal(CreateLoadDataWorkItem(this, pData, type, nPriority, pListener, &pWork), false);

	pData->SetDataLoader(this);
	_workItems.Push(pWork);

	ProcessGlobals::Get()->GetThreadPool()->Add(pWork);

	return true;
}

bool ff::CAsyncDataLoader::Wait(ISavedData *pData)
{
	ComPtr<CLoadDataWorkItem> pWork = FindWorkItemForData(pData);

	return !pWork || ProcessGlobals::Get()->GetThreadPool()->Wait(pWork);
}

bool ff::CAsyncDataLoader::Wait(const IAsyncDataListener *pListener)
{
	for (ComPtr<CLoadDataWorkItem> pWork = FindWorkItemForListener(pListener);
		pWork; pWork = FindWorkItemForListener(pListener))
	{
		assertRetVal(ProcessGlobals::Get()->GetThreadPool()->Wait(pWork), false);
	}

	return true;
}

bool ff::CAsyncDataLoader::Cancel(ISavedData *pData)
{
	ComPtr<CLoadDataWorkItem> pWork = FindWorkItemForData(pData);
	assertRetVal(pWork, false);

	return ProcessGlobals::Get()->GetThreadPool()->Cancel(pWork);
}

bool ff::CAsyncDataLoader::Cancel(const IAsyncDataListener *pListener)
{
	for (ComPtr<CLoadDataWorkItem> pWork = FindWorkItemForListener(pListener);
		pWork; pWork = FindWorkItemForListener(pListener))
	{
		assertRetVal(ProcessGlobals::Get()->GetThreadPool()->Cancel(pWork), false);
	}

	return true;
}

bool ff::CAsyncDataLoader::CancelAll()
{
	Vector<ComPtr<CLoadDataWorkItem>> workItems = _workItems;

	for (size_t i = 0; i < workItems.Size(); i++)
	{
		ProcessGlobals::Get()->GetThreadPool()->Cancel(workItems[i]);
	}

	return true;
}

void ff::CAsyncDataLoader::OnComplete(ISavedData *pData, EDataWorkType type)
{
	assertRet(ReleaseData(pData));

	// notify the listeners

	for (size_t i = PreviousSize(_listeners.Size()); i != INVALID_SIZE; i = PreviousSize(i))
	{
		_listeners[i]->OnComplete(pData, type);
	}
}

void ff::CAsyncDataLoader::OnCancel(ISavedData *pData, EDataWorkType type)
{
	assertRet(ReleaseData(pData));

	// notify the listeners

	for (size_t i = PreviousSize(_listeners.Size()); i != INVALID_SIZE; i = PreviousSize(i))
	{
		_listeners[i]->OnCancel(pData, type);
	}
}

ff::CLoadDataWorkItem *ff::CAsyncDataLoader::FindWorkItemForData(ISavedData *pData)
{
	assertRetVal(pData, nullptr);

	for (size_t i = 0; i < _workItems.Size(); i++)
	{
		if (_workItems[i]->GetData() == pData)
		{
			return _workItems[i];
		}
	}

	return nullptr;
}

ff::CLoadDataWorkItem *ff::CAsyncDataLoader::FindWorkItemForListener(const IAsyncDataListener *pListener)
{
	assertRetVal(pListener, nullptr);

	for (size_t i = 0; i < _workItems.Size(); i++)
	{
		for (size_t h = 0; h < _workItems[i]->GetListenerCount(); h++)
		{
			if (_workItems[i]->GetListener(h) == pListener)
			{
				return _workItems[i];
			}
		}
	}

	return nullptr;
}

bool ff::CAsyncDataLoader::ReleaseData(ISavedData *pData)
{
	// break the link between the data and the loader

	pData->SetDataLoader(nullptr);

	ComPtr<CLoadDataWorkItem> pWork = FindWorkItemForData(pData);
	size_t nFound = _workItems.Find(pWork);
	assertRetVal(nFound != INVALID_SIZE, false);

	_workItems.Delete(nFound);

	return true;
}

bool ff::CAsyncDataLoader::Flush()
{
	// Don't call Flush() on the thread pool because there could be
	// other types of work items running.

	Vector<ComPtr<CLoadDataWorkItem>> workItems = _workItems;

	for (size_t i = 0; i < workItems.Size(); i++)
	{
		ProcessGlobals::Get()->GetThreadPool()->Wait(workItems[i]);
	}

	return true;
}

void ff::CAsyncDataLoader::AddListener(IAsyncDataListener *pListener)
{
	ComPtr<IAsyncDataListener> pMyListener = pListener;
	assertRet(pListener && _listeners.Find(pMyListener) == INVALID_SIZE);

	_listeners.Push(pMyListener);
}

void ff::CAsyncDataLoader::RemoveListener(IAsyncDataListener *pListener)
{
	ComPtr<IAsyncDataListener> pMyListener = pListener;
	size_t nFound = _listeners.Find(pMyListener);
	assertRet(nFound != INVALID_SIZE);

	_listeners.Delete(nFound);
}

void ff::CAsyncDataLoader::AddLoading(size_t nSize)
{
	_loading += nSize;
}

void ff::CAsyncDataLoader::AddProgress(size_t nSize)
{
	_progress += nSize;

	// Log::DebugTraceF(L"*** Progress: %lu / %lu\n", _progress, _loading);

	assert(_progress <= _loading);
}

bool ff::CAsyncDataLoader::IsLoading() const
{
	return _progress < _loading;
}

void ff::CAsyncDataLoader::ClearTotalLoadedBytes()
{
	_loading  = 0;
	_progress = 0;
}

bool ff::CAsyncDataLoader::GetProgress(size_t &nSize, size_t &nTotal) const
{
	nSize  = _progress;
	nTotal = _loading;

	return IsLoading();
}

ff::CLoadDataWorkItem::CLoadDataWorkItem()
	: _type(DWT_LOAD)
	, _priority(s_nDefaultPriority)
{
}

ff::CLoadDataWorkItem::~CLoadDataWorkItem()
{
	assert(!_loader);
}

bool ff::CLoadDataWorkItem::Init(
	CAsyncDataLoader *pLoader,
	ISavedData *pData,
	EDataWorkType type,
	int nPriority,
	IAsyncDataListener *pListener)
{
	assertRetVal(pData && pLoader, false);

	// This creates a circular loop between the loader and this work item.
	// But the chain will be broken once the work item is completed or canceled.
	_loader   = pLoader;
	_data     = pData;
	_type      = type;
	_priority = nPriority;

	if (pListener)
	{
		_listeners.Push(pListener);
	}

	// Make a clone because it's the clone that will actually load the data
	// (don't want to mess with the main thread's saved data)

	assertRetVal(pData->Clone(&_clone), false);

	_loader->AddLoading(_clone->GetFullSize());

	return true;
}

void ff::CLoadDataWorkItem::Run()
{
	if (_clone && _data)
	{
		switch (_type)
		{
		case DWT_LOAD:
			verify(_clone->Load());
			break;

		case DWT_UNLOAD:
			verify(_clone->Unload());
			break;

		case DWT_SAVE_TO_MEM:
			verify(_clone->SaveToMem());
			break;

		case DWT_SAVE_TO_FILE:
			verify(_clone->SaveToFile());
			break;
		}
	}
}

void ff::CLoadDataWorkItem::OnCancel()
{
	if (_data)
	{
		if (_loader)
		{
			_loader->AddProgress(_data->GetFullSize());
			_loader->OnCancel(_data, _type);
		}

		for (size_t i = 0; i < _listeners.Size(); i++)
		{
			_listeners[i]->OnCancel(_data, _type);
		}
	}

	_data   = nullptr;
	_clone  = nullptr;
	_loader = nullptr;
	_listeners.Clear();
}

void ff::CLoadDataWorkItem::OnComplete()
{
	if (_data)
	{
		if (_clone)
		{
			_data->Copy(_clone);
		}

		if (_loader)
		{
			_loader->AddProgress(_data->GetFullSize());
			_loader->OnComplete(_data, _type);
		}

		for (size_t i = 0; i < _listeners.Size(); i++)
		{
			_listeners[i]->OnComplete(_data, _type);
		}
	}

	_data   = nullptr;
	_clone  = nullptr;
	_loader = nullptr;
	_listeners.Clear();
}

int ff::CLoadDataWorkItem::GetPriority() const
{
	return _priority;
}

ff::ISavedData *ff::CLoadDataWorkItem::GetData()
{
	return _data;
}

size_t ff::CLoadDataWorkItem::GetListenerCount()
{
	return _listeners.Size();
}

ff::IAsyncDataListener *ff::CLoadDataWorkItem::GetListener(size_t nIndex)
{
	return _listeners[nIndex];
}

void ff::CLoadDataWorkItem::AddListener(IAsyncDataListener *pListener)
{
	assertRet(pListener);

	ComPtr<IAsyncDataListener> pMyListener = pListener;
	
	if (_listeners.Find(pMyListener) == INVALID_SIZE)
	{
		_listeners.Push(pMyListener);
	}
}

ff::EDataWorkType ff::CLoadDataWorkItem::GetWorkType() const
{
	return _type;
}
