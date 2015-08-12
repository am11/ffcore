#pragma once

namespace ff
{
	class IWorkItem;
	class IWorkItemListener;
	class IProxyWorkItemListener;
	class IThreadPool;

	UTIL_API bool CreateThreadPool(IThreadPool **ppThreadPool, int nMaxThreads = 0);
	UTIL_API void FlushAllThreadPoolWork();
	UTIL_API void SuspendAllWorkerThreads();
	UTIL_API void ResumeAllWorkerThreads();
	UTIL_API bool CreateProxyWorkItemListener(IWorkItemListener *owner, IProxyWorkItemListener **obj);

	class CScopeBlockAllWorkerThreads
	{
	public:
		UTIL_API CScopeBlockAllWorkerThreads();
		UTIL_API ~CScopeBlockAllWorkerThreads();
	};

	class __declspec(uuid("27a26564-ec49-41c4-b39c-0c27681dbdcc")) __declspec(novtable)
		IWorkItemListener : public IUnknown
	{
	public:
		virtual void OnComplete(IWorkItem *pWork) = 0;
		virtual void OnCancel(IWorkItem *pWork) = 0;
	};

	class __declspec(uuid("c6a03c11-2b7d-4e70-9536-f0dac27798c6")) __declspec(novtable)
		IProxyWorkItemListener : public IWorkItemListener
	{
	public:
		// Must call SetOwner(nullptr) when the owner is destroyed
		virtual void SetOwner(IWorkItemListener *pOwner) = 0;
	};

	// IWorkItem does not have pure virtual functions, and it's
	// already a COM base object, so it's simple to use. The minimal work
	// you need to do is derive from IWorkItem and override Run().

	class __declspec(uuid("20dde21e-5f26-4a1b-837a-ac519d94e33b")) __declspec(novtable)
		IWorkItem : public ComBase, public IUnknown
	{
	public:
		DECLARE_API_HEADER(IWorkItem);

		UTIL_API virtual void Run();               // called from a worker thread
		UTIL_API virtual void OnCancel();          // called from the main thread if the thread pool is destroyed
		UTIL_API virtual void OnComplete();        // called from the main thread after the work is done
		UTIL_API virtual int  GetPriority() const; // larger numbers are processed first

		UTIL_API void AddListener(IWorkItemListener *pListener);
		UTIL_API bool AddProxyListener(IWorkItemListener *pListener, IProxyWorkItemListener **ppProxy);
		UTIL_API void RemoveListener(IWorkItemListener *pListener);

		// These are called by the owner of this work item
		UTIL_API void InternalOnCancel();
		UTIL_API void InternalOnComplete();

		// STATIC_DATA (pod)
		static const int s_nDefaultPriority = 5;

	private:
		std::unique_ptr<Vector<ComPtr<IWorkItemListener>>> _listeners;
	};


	class __declspec(uuid("691c1238-aaf7-439a-af66-d9bfdb897543")) __declspec(novtable)
		IThreadPool : public IUnknown
	{
	public:
		virtual void Add(IWorkItem *pWork) = 0;
		virtual bool Wait(IWorkItem *pWork) = 0;
		virtual bool Cancel(IWorkItem *pWork) = 0; // cancel a PENDING work item (prevent it from starting to run)
		virtual void Flush() = 0; // wait for everything to complete

		virtual void Suspend() = 0;
		virtual void Resume() = 0;
	};
}
