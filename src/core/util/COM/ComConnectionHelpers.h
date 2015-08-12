#pragma once

namespace ff
{
	class IComObject;

	class __declspec(uuid("9e0262f6-7b38-4c2a-bba6-5220cc03b993")) __declspec(novtable)
		IConnectionPointBase : public IConnectionPoint
	{
	public:
		virtual void DestroySink() = 0;
		virtual size_t GetSinkCount() const = 0;
		virtual IUnknown *GetSink(size_t index) const = 0;
		virtual DWORD GetCookie(size_t index) const = 0;
	};

	class IConnectionPointHolder
	{
	public:
		virtual bool HasConnectionPoint() const = 0;
		virtual IConnectionPoint *GetConnectionPoint(IConnectionPointContainer *parent) = 0;
		virtual REFGUID GetConnectionInterface() const = 0;
	};

	class __declspec(uuid("303fba88-b172-4e91-8d88-acd82fa76833"))
		ConnectionPointEnum
			: public ff::ComBase
			, public IEnumConnectionPoints
	{
	public:
		DECLARE_HEADER(ConnectionPointEnum);

		UTIL_API static bool Create(IComObject *parent, ConnectionPointEnum **value);

		// IEnumConnectionPoints
		COM_FUNC Next(ULONG cConnections, LPCONNECTIONPOINT *ppCP, ULONG *pcFetched) override;
		COM_FUNC Skip(ULONG cConnections) override;
		COM_FUNC Reset() override;
		COM_FUNC Clone(IEnumConnectionPoints **value) override;

	private:
		ComPtr<IComObject> _parent;
		size_t _pos;
	};
}
