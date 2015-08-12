#include "pch.h"
#include "COM/ComConnectionHelpers.h"
#include "COM/ComConnectionPoint.h"

BEGIN_INTERFACES(ff::ConnectionPointEnum)
	HAS_INTERFACE(IEnumConnectionPoints)
END_INTERFACES()

ff::ConnectionPointEnum::ConnectionPointEnum()
	: _pos(0)
{
}

ff::ConnectionPointEnum::~ConnectionPointEnum()
{
}

// static
bool ff::ConnectionPointEnum::Create(IComObject *parent, ConnectionPointEnum **value)
{
	assertRetVal(parent && value, false);

	ff::ComPtr<ConnectionPointEnum> myValue;
	assertHrRetVal(ff::ComAllocator<ConnectionPointEnum>::CreateInstance(&myValue), false);
	myValue->_parent = parent;

	*value = myValue.Detach();
	return S_OK;
}

HRESULT ff::ConnectionPointEnum::Next(ULONG cConnections, LPCONNECTIONPOINT *ppCP, ULONG *pcFetched)
{
	assertRetVal(ppCP, E_INVALIDARG);

	if (pcFetched)
	{
		*pcFetched = 0;
	}

	ff::ComPtr<IConnectionPointContainer> container;
	assertRetVal(container.QueryFrom(_parent), S_FALSE);

	for (size_t index = 0; cConnections > 0; cConnections--, _pos++, index++)
	{
		IConnectionPointHolder *holder = _parent->GetComConnectionPointHolder(_pos);
		if (holder != nullptr)
		{
			ppCP[index] = GetAddRef(holder->GetConnectionPoint(container));

			if (pcFetched)
			{
				++*pcFetched;
			}
		}
		else
		{
			return S_FALSE;
		}
	}

	return S_OK;
}

HRESULT ff::ConnectionPointEnum::Skip(ULONG cConnections)
{
	_pos += cConnections;
	return _parent->GetComConnectionPointHolder(_pos) == nullptr ? S_FALSE : S_OK;
}

HRESULT ff::ConnectionPointEnum::Reset()
{
	_pos = 0;
	return S_OK;
}

HRESULT ff::ConnectionPointEnum::Clone(IEnumConnectionPoints **value)
{
	assertRetVal(value, E_INVALIDARG);

	ff::ComPtr<ConnectionPointEnum> clone;
	assertRetVal(Create(_parent, &clone), E_FAIL);
	clone->_pos = _pos;

	*value = clone.Detach();
	return S_OK;
}
