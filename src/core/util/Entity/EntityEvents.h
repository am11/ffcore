#pragma once
#include "Entity/Entity.h"

namespace ff
{
	class EntityDomain;

	class IEntityEventHandler
	{
	public:
		virtual ~IEntityEventHandler() { }

		virtual void OnEntityEvent(EntityDomain *domain, Entity entity, size_t eventId, void *eventArgs) = 0;
	};

	class EntityEventConnection
	{
	public:
		UTIL_API EntityEventConnection();
		UTIL_API ~EntityEventConnection();

		UTIL_API bool Connect(EntityDomain *domain, StringRef name, IEntityEventHandler *handler);
		UTIL_API bool Connect(EntityDomain *domain, size_t eventId, IEntityEventHandler *handler);
		UTIL_API bool Connect(EntityDomain *domain, StringRef name, Entity entity, IEntityEventHandler *handler);
		UTIL_API bool Connect(EntityDomain *domain, size_t eventId, Entity entity, IEntityEventHandler *handler);
		UTIL_API bool Disconnect();

	private:
		void Init();

		EntityDomain *_domain;
		String _eventName;
		size_t _eventId;
		Entity _entity;
		IEntityEventHandler *_handler;
	};
}
