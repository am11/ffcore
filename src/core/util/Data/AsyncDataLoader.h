#pragma once

namespace ff
{
	class IAsyncDataLoader;
	class IAsyncDataListener;
	class ISavedData;

	UTIL_API bool CreateAsyncDataLoader(IAsyncDataLoader **ppLoader);

	enum EDataWorkType
	{
		DWT_LOAD,
		DWT_UNLOAD,
		DWT_SAVE_TO_MEM,
		DWT_SAVE_TO_FILE,
	};

	class __declspec(uuid("c4bdd93e-0480-4781-b186-9e899d0930c6")) __declspec(novtable)
		IAsyncDataListener : public IUnknown
	{
	public:
		virtual void OnComplete(ISavedData *pData, EDataWorkType type) = 0;
		virtual void OnCancel  (ISavedData *pData, EDataWorkType type) = 0;
	};

	class __declspec(uuid("f8f43d8e-e0e0-4373-960f-cd844f276925")) __declspec(novtable)
		IProxyAsyncDataListener : public IAsyncDataListener
	{
	public:
		// Must call SetOwner(nullptr) when the owner is destroyed
		virtual void SetOwner(IAsyncDataListener *pOwner) = 0;
	};

	UTIL_API bool CreateProxyAsyncDataListener(IAsyncDataListener *pOwner, IProxyAsyncDataListener **ppListener);

	class __declspec(uuid("3a21c86e-de9d-4e85-b376-1906f1cb9a9e")) __declspec(novtable)
		IAsyncDataLoader : public IUnknown
	{
	public:
		virtual bool Load(ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener) = 0;
		virtual bool Unload(ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener) = 0;
		virtual bool SaveToMem(ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener) = 0;
		virtual bool SaveToFile(ISavedData *pData, int nPriorityOffset, IAsyncDataListener *pListener) = 0;

		virtual bool Wait(ISavedData *pData) = 0;
		virtual bool Wait(const IAsyncDataListener *pListener) = 0;
		virtual bool Cancel(ISavedData *pData) = 0;
		virtual bool Cancel(const IAsyncDataListener *pListener) = 0;

		virtual bool Flush() = 0;
		virtual bool CancelAll() = 0;

		virtual void AddListener(IAsyncDataListener *pListener) = 0;
		virtual void RemoveListener(IAsyncDataListener *pListener) = 0;

		virtual bool IsLoading() const = 0;
		virtual void ClearTotalLoadedBytes() = 0;
		virtual bool GetProgress(size_t &nSize, size_t &nTotal) const = 0;
	};
}
