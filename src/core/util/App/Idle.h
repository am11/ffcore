#pragma once

namespace ff
{
	class IWorkItem;

	class __declspec(uuid("d2c0b9de-33c7-4343-8d02-227e2d75eb27")) __declspec(novtable)
		IIdleMaster : public IUnknown
	{
	public:
		virtual void Add(IWorkItem *work) = 0;
		virtual bool Remove(IWorkItem *work) = 0;

		virtual void ForceIdle() = 0; // Does idle work NOW
		virtual void KickIdle() = 0;  // Does idle work later

		virtual size_t GetIdleTime() = 0;
		virtual void SetIdleTime(size_t nMS) = 0;
	};

	bool CreateIdleMaster(IIdleMaster **obj);
}
