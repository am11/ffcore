#pragma once

#include "COM/ComAlloc.h"

namespace ff
{
	class __declspec(uuid("5951cbbb-2d45-44e9-895a-52b3a9eb9959"))
		ConnectionPointBase
			: public ComBase
			, public IConnectionPointBase
	{
	public:
		DECLARE_API_HEADER(ConnectionPointBase);

		struct Sink
		{
			bool operator==(const Sink &rhs) const;

			ComPtr<IUnknown> _sink;
			void *_knownSink;
			DWORD _cookie;
		};

		typedef SharedObject<Vector<Sink>> SharedSinks;
		typedef SmartPtr<SharedSinks> SharedSinksPtr;

		UTIL_API SharedSinksPtr GetSinks() const;

		// IConnectionPoint
		UTIL_API COM_FUNC GetConnectionPointContainer(IConnectionPointContainer **value) override;
		UTIL_API COM_FUNC Advise(IUnknown *sink, DWORD *cookie) override;
		UTIL_API COM_FUNC Unadvise(DWORD cookie) override;
		UTIL_API COM_FUNC EnumConnections(IEnumConnections **value) override;

		// IConnectionPointBase
		UTIL_API virtual void DestroySink() override;
		UTIL_API virtual size_t GetSinkCount() const override;
		UTIL_API virtual IUnknown *GetSink(size_t index) const override;
		UTIL_API virtual DWORD GetCookie(size_t index) const override;

	protected:
		UTIL_API bool Init(IConnectionPointContainer *parent);

	private:
		IConnectionPointContainer *_parent;
		SharedSinksPtr _sinks;
	};

	template<class T>
	class ConnectionPointType : public ConnectionPointBase
	{
	public:
		static bool Create(IConnectionPointContainer *parent, ConnectionPointType<T> **value);

		// IConnectionPoint
		COM_FUNC GetConnectionInterface(IID *iid) override;
	};

	template<class T>
	class ConnectionPoint : public IConnectionPointHolder
	{
	public:
		ConnectionPoint();
		~ConnectionPoint();

		// IConnectionSlot
		virtual bool HasConnectionPoint() const override;
		virtual IConnectionPoint *GetConnectionPoint(IConnectionPointContainer *parent) override;
		virtual REFGUID GetConnectionInterface() const override;

	private:
		ComPtr<ConnectionPointType<T>> _sinks;

	// C++ iterators
	public:
		class iterator : public std::iterator<std::input_iterator_tag, T *>
		{
		public:
			iterator(ConnectionPointBase::SharedSinksPtr &&sinks, size_t pos)
				: _sinks(std::move(sinks))
				, _cur((pos == INVALID_SIZE) ? (_sinks != nullptr ? _sinks->Size() : 0) : 0)
			{
			}

			iterator(iterator &&rhs)
				: _sinks(std::move(rhs._sinks))
				, _cur(rhs._cur)
			{
			}

			iterator(const iterator &rhs)
				: _sinks(rhs._sinks)
				, _cur(rhs._cur)
			{
			}

			T *operator*() const
			{
				return Cur();
			}

			T *operator->() const
			{
				return Cur();
			}

			iterator &operator++()
			{
				_cur++;
				return *this;
			}

			void operator++(int)
			{
				_cur++;
			}

			bool operator==(const iterator &rhs) const
			{
				return _sinks == rhs._sinks && _cur == rhs._cur;
			}

			bool operator!=(const iterator &rhs) const
			{
				return _sinks != rhs._sinks || _cur != rhs._cur;
			}

		private:
			T *Cur() const
			{
				void *knownSink = _sinks->GetAt(_cur)._knownSink;
				return static_cast<T *>(knownSink);
			}

			ConnectionPointBase::SharedSinksPtr _sinks;
			size_t _cur;
		};

		iterator begin()  const { return iterator(_sinks ? _sinks->GetSinks() : ConnectionPointBase::SharedSinksPtr(), 0); }
		iterator end()    const { return iterator(_sinks ? _sinks->GetSinks() : ConnectionPointBase::SharedSinksPtr(), INVALID_SIZE); }
		iterator cbegin() const { return iterator(_sinks ? _sinks->GetSinks() : ConnectionPointBase::SharedSinksPtr(), 0); }
		iterator cend()   const { return iterator(_sinks ? _sinks->GetSinks() : ConnectionPointBase::SharedSinksPtr(), INVALID_SIZE); }
	};

	template<class T>
	class Connection
	{
	public:
		Connection();
		Connection(Connection<T> &&rhs);
		~Connection();

		bool Connect(IUnknown *parent, T *listener);
		void Disconnect();

	private:
		Connection(const Connection &rhs);
		Connection &operator=(const Connection &rhs);

		ComPtr<IConnectionPoint> _point;
		DWORD _cookie;
	};
}

// static
template<class T>
bool ff::ConnectionPointType<T>::Create(IConnectionPointContainer *parent, ConnectionPointType<T> **value)
{
	assertRetVal(value, false);
	ComPtr<ConnectionPointType<T>> myValue;
	assertHrRetVal(ComAllocator<ConnectionPointType<T>>::CreateInstance(&myValue), false);
	assertHrRetVal(myValue->Init(parent), false);

	*value = myValue.Detach();
	return true;
}

template<class T>
HRESULT ff::ConnectionPointType<T>::GetConnectionInterface(IID *iid)
{
	assertRetVal(iid, E_INVALIDARG);
	*iid = __uuidof(T);
	return S_OK;
}

template<class T>
ff::ConnectionPoint<T>::ConnectionPoint()
{
}

template<class T>
ff::ConnectionPoint<T>::~ConnectionPoint()
{
	if (_sinks)
	{
		_sinks->DestroySink();
	}
}

template<class T>
bool ff::ConnectionPoint<T>::HasConnectionPoint() const
{
	return _sinks != nullptr;
}

template<class T>
IConnectionPoint *ff::ConnectionPoint<T>::GetConnectionPoint(IConnectionPointContainer *parent)
{
	if (_sinks == nullptr && parent != nullptr)
	{
		assertRetVal(ff::ConnectionPointType<T>::Create(parent, &_sinks), false);
	}

	return _sinks;
}

template<class T>
REFGUID ff::ConnectionPoint<T>::GetConnectionInterface() const
{
	return __uuidof(T);
}

template<class T>
ff::Connection<T>::Connection()
	: _cookie(0)
{
}

template<class T>
ff::Connection<T>::Connection(Connection<T> &&rhs)
	: _cookie(rhs._cookie)
	, _point(std::move(rhs._point))
{
	rhs._cookie = 0;
}

template<class T>
ff::Connection<T>::~Connection()
{
	Disconnect();
}

template<class T>
bool ff::Connection<T>::Connect(IUnknown *parent, T *listener)
{
	Disconnect();
	assertRetVal(listener, false);

	if (parent)
	{
		ComPtr<IConnectionPointContainer> container;
		assertRetVal(container.QueryFrom(parent), false);

		ComPtr<IConnectionPoint> point;
		assertHrRetVal(container->FindConnectionPoint(__uuidof(T), &point), false);

		DWORD cookie = 0;
		assertHrRetVal(point->Advise(listener, &cookie), false);

		_point = point;
		_cookie = cookie;
	}

	return true;
}

template<class T>
void ff::Connection<T>::Disconnect()
{
	if (_point)
	{
		if (_cookie)
		{
			verify(SUCCEEDED(_point->Unadvise(_cookie)));
		}

		_point = nullptr;
		_cookie = 0;
	}
}
