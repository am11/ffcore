#include "pch.h"
#include "Entity/EntityDomain.h"
#include "Entity/EntityEvents.h"

ff::EntityEventConnection::EntityEventConnection()
{
	Init();
}

ff::EntityEventConnection::~EntityEventConnection()
{
	Disconnect();
}

bool ff::EntityEventConnection::Connect(EntityDomain *domain, StringRef name, IEntityEventHandler *handler)
{
	return Connect(domain, name, INVALID_ENTITY, handler);
}

bool ff::EntityEventConnection::Connect(EntityDomain *domain, size_t eventId, IEntityEventHandler *handler)
{
	return Connect(domain, eventId, INVALID_ENTITY, handler);
}

bool ff::EntityEventConnection::Connect(EntityDomain *domain, StringRef name, Entity entity, IEntityEventHandler *handler)
{
	if (domain && domain->AddEventHandler(name, entity, handler))
	{
		_domain = domain;
		_eventName = name;
		_entity = entity;
		_handler = handler;

		return true;
	}

	assertRetVal(false, false);
}

bool ff::EntityEventConnection::Connect(EntityDomain *domain, size_t eventId, Entity entity, IEntityEventHandler *handler)
{
	if (domain && domain->AddEventHandler(eventId, entity, handler))
	{
		_domain = domain;
		_eventId = eventId;
		_entity = entity;
		_handler = handler;

		return true;
	}

	assertRetVal(false, false);
}

bool ff::EntityEventConnection::Disconnect()
{
	bool removed = false;

	if (_eventName.size())
	{
		bool removed = _domain->RemoveEventHandler(_eventName, _entity, _handler);
		assert(removed);
		Init();
	}
	else if (_eventId != INVALID_SIZE)
	{
		bool removed = _domain->RemoveEventHandler(_eventId, _entity, _handler);
		assert(removed);
		Init();
	}

	return removed;
}

void ff::EntityEventConnection::Init()
{
	_domain = nullptr;
	_eventName.clear();
	_eventId = INVALID_SIZE;
	_entity = INVALID_ENTITY;
	_handler = nullptr;
}
