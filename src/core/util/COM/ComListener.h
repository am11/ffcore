#pragma once

namespace ff
{
	class __declspec(uuid("f52d379b-861c-4b03-9562-33ed922416a6")) __declspec(novtable)
		IComListener : public IUnknown
	{
	public:
		virtual void OnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj) = 0;
		virtual void OnDestruct (REFGUID catid, REFGUID clsid, IUnknown *pObj) = 0;

		// Triggers all listeners to be called:
		static void CallOnConstruct(IUnknown *unkOuter, REFGUID catid, REFGUID clsid, IUnknown *pObj);
		static void CallOnDestruct (REFGUID catid, REFGUID clsid, IUnknown *pObj);
	};

	class __declspec(uuid("547edb98-0447-410a-93aa-6955755a4b03")) __declspec(novtable)
		IProxyComListener : public IComListener
	{
	public:
		// Must call SetOwner(nullptr) when the owner is destroyed
		virtual void SetOwner(IComListener *pOwner) = 0;
	};

	UTIL_API bool CreateProxyComListener(IComListener *pOwner, IProxyComListener **ppProxy);

	UTIL_API bool AddComListener(REFGUID catid, IComListener *pListener);
	UTIL_API bool RemoveComListener(REFGUID catid, IComListener *pListener);

	UTIL_API bool AddComListener(IUnknown *pObject, IComListener *pListener);
	UTIL_API bool RemoveComListener(IUnknown *pObject, IComListener *pListener);
}
