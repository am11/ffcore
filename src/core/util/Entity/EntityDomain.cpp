#include "pch.h"
#include "App/AppGlobals.h"
#include "COM/ServiceCollection.h"
#include "Entity/EntityDomain.h"
#include "Entity/EntityEvents.h"
#include "Entity/EntitySystemBase.h"
#include "Entity/Internal/Component.h"
#include "Entity/Internal/EntitySystem.h"
#include "Resource/ResourceContext.h"

bool ff::EntityDomain::EventHandler::operator==(const EventHandler &rhs) const
{
	return _event == rhs._event && _handler == rhs._handler;
}

// static
ff::EntityDomain::EntityEntry *ff::EntityDomain::EntityEntry::FromEntity(Entity entity)
{
	return static_cast<EntityEntry *>(entity);
}

ff::Entity ff::EntityDomain::EntityEntry::ToEntity()
{
	return static_cast<Entity>(this);
}

ff::EntityDomain::EntityDomain()
	: _active(false)
{
	Init(nullptr, nullptr, nullptr);
}

ff::EntityDomain::EntityDomain(const EntityDomain *parent)
	: _active(false)
{
	Init(parent, nullptr, nullptr);
}

ff::EntityDomain::EntityDomain(AppGlobals *parent)
	: _active(false)
{
	Init(nullptr, parent, nullptr);
}

ff::EntityDomain::EntityDomain(IResourceContext *parent)
	: _active(false)
{
	Init(nullptr, nullptr, parent);
}

void ff::EntityDomain::Init(const EntityDomain *parentDomain, AppGlobals *parentApp, IResourceContext *parentResourceContext)
{
	_parentDomain = parentDomain;
	_parentApp = parentApp;
	_parentResourceContext = parentResourceContext;
	_nextEventId = INVALID_SIZE;

	if (_parentApp == nullptr && _parentDomain != nullptr)
	{
		_parentApp = _parentDomain->GetAppGlobals();
	}

	if (_parentApp == nullptr && _parentResourceContext != nullptr)
	{
		_parentApp = &_parentResourceContext->GetAppGlobals();
	}

	verify(CreateServiceCollection(_parentApp != nullptr ? _parentApp->GetServices() : nullptr, &_services));
}

ff::EntityDomain::~EntityDomain()
{
	Vector<EntityEntry *> entities = _entities.ToPointerVector();
	SystemEntries systems = _systems;

	for (EntityEntry *entityEntry: entities)
	{
		if (entityEntry->_valid)
		{
			DeleteEntity(entityEntry->ToEntity());
		}
	}

	for (SystemEntry *systemEntry: systems)
	{
		RemoveSystem(systemEntry->_system.get());
	}

	FlushDeletedEntities();
	FlushDeletedSystems();
}

// static
ff::EntityDomain *ff::EntityDomain::FromEntity(Entity entity)
{
	assertRetVal(entity != INVALID_ENTITY, nullptr);
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	return entityEntry->_domain;
}

ff::AppGlobals *ff::EntityDomain::GetAppGlobals() const
{
	if (_parentApp)
	{
		return _parentApp;
	}

	if (_parentDomain)
	{
		return _parentDomain->GetAppGlobals();
	}

	return nullptr;
}

ff::ProcessGlobals *ff::EntityDomain::GetProcessGlobals() const
{
	AppGlobals *app = GetAppGlobals();
	return app ? &app->GetProcessGlobals() : nullptr;
}

void ff::EntityDomain::Activate()
{
	_active = true;

	for (SystemEntry *system: _systems)
	{
		if (system->_valid)
		{
			system->_system->OnDomainActivated();
		}
	}
}

void ff::EntityDomain::Deactivate()
{
	_active = false;

	for (SystemEntry *system: _systems)
	{
		if (system->_valid)
		{
			system->_system->OnDomainDectivated();
		}
	}
}

bool ff::EntityDomain::IsActive() const
{
	return _active;
}

void ff::EntityDomain::Advance()
{
	for (SystemEntry *system: _systems)
	{
		if (system->_valid)
		{
			system->_system->Ready();
		}
	}

	for (SystemEntry *system: _systems)
	{
		if (system->_valid)
		{
			system->_system->Advance();
		}
	}

	for (SystemEntry *system: _systems)
	{
		if (system->_valid)
		{
			system->_system->Cleanup();
		}
	}

	for (ComponentFactoryEntry *factoryEntry: _componentFactories)
	{
		factoryEntry->_factory->Advance();
	}

	FlushDeletedEntities();
	FlushDeletedSystems();
}

void ff::EntityDomain::Render(IRenderTarget *target)
{
	for (SystemEntry *system: _systems)
	{
		if (system->_valid)
		{
			system->_system->Render(target);
		}
	}
}

bool ff::EntityDomain::AddSystem(std::shared_ptr<details::EntitySystem> system, StringRef nameOverride)
{
	assertRetVal(system != nullptr && system->GetDomain() == this, false);

	String name = nameOverride.size() ? nameOverride : system->GetDefaultName();
	assertRetVal(name.size() && GetSystem(name) == nullptr, false);

	BucketIter iter;
	{
		SystemEntry systemEntry;
		systemEntry._system = system;
		systemEntry._name = name;
		systemEntry._componentBits = 0;
		systemEntry._valid = true;

		iter = _systemsByName.Insert(systemEntry._name, systemEntry);
	}
	SystemEntry &savedSystemEntry = _systemsByName.ValueAt(iter);
	_systems.Push(&savedSystemEntry);

	savedSystemEntry._components = FindComponentEntries(system->GetComponents());

	for (ComponentFactoryEntry *factoryEntry: savedSystemEntry._components)
	{
		savedSystemEntry._componentBits |= factoryEntry->_bit;
		factoryEntry->_systems.Push(&savedSystemEntry);
	}

	for (EntityEntry &entityEntry: _entities)
	{
		TryRegisterEntityWithSystem(&entityEntry, &savedSystemEntry);
	}

	return true;
}

bool ff::EntityDomain::RemoveSystem(details::EntitySystem *system)
{
	for (BucketIter i = _systemsByName.StartIteration(); i != INVALID_ITER; i = _systemsByName.Iterate(i))
	{
		SystemEntry &systemEntry = _systemsByName.ValueAt(i);

		if (systemEntry._valid && systemEntry._system.get() == system)
		{
			for (auto &h: systemEntry._entities)
			{
				systemEntry._system->DeleteEntry(h.GetValue());
			}

			for (auto &h: systemEntry._components)
			{
				verify(h->_systems.DeleteItem(&systemEntry));
			}

			// Removing a system should be rare, so looping over every system in every entity
			// shouldn't be a problem (if it is, then systems need to know exactly which entities
			// hold a pointer to the system)
			for (EntityEntry &entityEntry: _entities)
			{
				entityEntry._systems.DeleteItem(&systemEntry);
			}

			systemEntry._entities.Clear();
			systemEntry._components.Clear();
			systemEntry._componentBits = 0;
			systemEntry._valid = false;

			verify(_systems.DeleteItem(&systemEntry));

			return true;
		}
	}

	assertRetVal(false, false);
}

ff::details::EntitySystem *ff::EntityDomain::GetSystem(StringRef name) const
{
	assertRetVal(name.size(), nullptr);

	for (BucketIter i = _systemsByName.Get(name); i != INVALID_ITER; i = _systemsByName.GetNext(i))
	{
		const SystemEntry &systemEntry = _systemsByName.ValueAt(i);
		if (systemEntry._valid)
		{
			return systemEntry._system.get();
		}
	}
	
	return nullptr;
}

/// Creates a new blank entity and optionally gives it a name
ff::Entity ff::EntityDomain::CreateEntity(StringRef name)
{
	EntityEntry &entityEntry = _entities.Insert();
	entityEntry._domain = this;
	entityEntry._name = name;
	entityEntry._componentBits = 0;
	entityEntry._valid = true;
	entityEntry._active = false;

	return entityEntry.ToEntity();
}

/// Creates a new entity with the exact same components as the source entity
ff::Entity ff::EntityDomain::CloneEntity(Entity sourceEntity, StringRef name)
{
	Entity newEntity = CreateEntity(name);
	EntityEntry *newEntityEntry = EntityEntry::FromEntity(newEntity);
	EntityEntry *sourceEntityEntry = EntityEntry::FromEntity(sourceEntity);

	for (auto factoryEntry: sourceEntityEntry->_components)
	{
		factoryEntry->_factory->Clone(newEntity, sourceEntity);
	}

	newEntityEntry->_componentBits = sourceEntityEntry->_componentBits;
	newEntityEntry->_components = sourceEntityEntry->_components;
	newEntityEntry->_systems = sourceEntityEntry->_systems;

	return newEntity;
}

/// If an entity was created with a name, this will find it
///
/// If multiple entities have the same name, then this can only return one of them.
ff::Entity ff::EntityDomain::GetEntity(StringRef name) const
{
	auto i = _entitiesByName.Get(name);
	return i != INVALID_ITER ? _entitiesByName.ValueAt(i) : INVALID_ENTITY;
}

const ff::String &ff::EntityDomain::GetEntityName(Entity entity) const
{
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	return entityEntry->_name;
}

/// Adds a new or deactivated entity to the systems that need to know about it
void ff::EntityDomain::ActivateEntity(Entity entity)
{
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	assertRet(entityEntry->_valid);

	if (!entityEntry->_active)
	{
		entityEntry->_active = true;
		RegisterActivatedEntity(entityEntry);
	}
}

/// Keeps the entity in memory, but it will be removed from any systems that know about it
void ff::EntityDomain::DeactivateEntity(Entity entity)
{
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	assertRet(entityEntry->_valid);

	if (entityEntry->_active)
	{
		entityEntry->_active = false;
		UnregisterDeactivatedEntity(entityEntry);
	}
}

/// Deletes an entity from memory and all systems that know about it
void ff::EntityDomain::DeleteEntity(Entity entity)
{
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	assertRet(entityEntry->_valid);

	DeactivateEntity(entity);

	entityEntry->_valid = false;
	_deletedEntities.Push(entityEntry);

	// Wait until frame cleanup to reclaim the entity memory
}

bool ff::EntityDomain::AddComponentFactory(
	StringRef name,
	size_t index,
	std::shared_ptr<details::ComponentFactory> factory)
{
	assertRetVal(name.size() && GetComponentFactory(name) == nullptr, false);
	assertRetVal(index == INVALID_SIZE ||
		index >= _componentFactoriesByIndex.Size() ||
		_componentFactoriesByIndex[index] == nullptr, false);

	BucketIter iter;
	{
		ComponentFactoryEntry factoryEntry;
		factoryEntry._factory = factory;
		factoryEntry._name = name;
		factoryEntry._index = index;
		factoryEntry._bitIndex = _componentFactoriesByName.Size();
		factoryEntry._bit = (uint64_t)1 << (factoryEntry._bitIndex % 64);

		iter = _componentFactoriesByName.SetKey(factoryEntry._name, factoryEntry);
	}
	ComponentFactoryEntry &savedFactoryEntry = _componentFactoriesByName.ValueAt(iter);
	_componentFactories.Push(&savedFactoryEntry);

	if (index != INVALID_SIZE)
	{
		_componentFactoriesByIndex.Reserve(index + 1);
		while (_componentFactoriesByIndex.Size() <= index)
		{
			_componentFactoriesByIndex.Push(nullptr);
		}

		_componentFactoriesByIndex[index] = &savedFactoryEntry;
	}

	return true;
}

ff::EntityDomain::ComponentFactoryEntry *ff::EntityDomain::GetComponentFactory(StringRef name)
{
	auto i = _componentFactoriesByName.Get(name);
	return i != INVALID_ITER ? &_componentFactoriesByName.ValueAt(i) : nullptr;
}

ff::EntityDomain::ComponentFactoryEntry *ff::EntityDomain::GetComponentFactory(size_t index)
{
	assertRetVal(index < _componentFactoriesByIndex.Size(), nullptr);
	return _componentFactoriesByIndex[index];
}

ff::details::Component *ff::EntityDomain::AddComponent(Entity entity, ComponentFactoryEntry *factoryEntry)
{
	assertRetVal(factoryEntry, nullptr);

	bool usedExisting = false;
	details::Component *component = &factoryEntry->_factory->New(entity, &usedExisting);

	if (!usedExisting)
	{
		EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
		assert(entityEntry->_components.Find(factoryEntry) == INVALID_SIZE);

		entityEntry->_componentBits |= factoryEntry->_bit;
		entityEntry->_components.Push(factoryEntry);

		RegisterEntityWithSystems(entityEntry, factoryEntry);
	}

	return component;
}

ff::details::Component *ff::EntityDomain::CloneComponent(Entity entity, Entity sourceEntity, ComponentFactoryEntry *factoryEntry)
{
	assertRetVal(factoryEntry, nullptr);
	details::Component *component = LookupComponent(entity, factoryEntry);
	assertRetVal(component == nullptr, component);

	component = factoryEntry->_factory->Clone(entity, sourceEntity);
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	assert(entityEntry->_components.Find(factoryEntry) == INVALID_SIZE);

	entityEntry->_componentBits |= factoryEntry->_bit;
	entityEntry->_components.Push(factoryEntry);

	RegisterEntityWithSystems(entityEntry, factoryEntry);

	return component;
}

ff::details::Component *ff::EntityDomain::LookupComponent(Entity entity, ComponentFactoryEntry *factoryEntry)
{
	assertRetVal(factoryEntry, nullptr);
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);

	return ((entityEntry->_componentBits & factoryEntry->_bit) != 0)
		? factoryEntry->_factory->Lookup(entity)
		: nullptr;
}

bool ff::EntityDomain::DeleteComponent(Entity entity, ComponentFactoryEntry *factoryEntry)
{
	assertRetVal(factoryEntry, false);
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);

	size_t i = (entityEntry->_componentBits & factoryEntry->_bit) != 0
		? entityEntry->_components.Find(factoryEntry)
		: INVALID_SIZE;

	if (i != INVALID_SIZE)
	{
		UnregisterEntityWithSystems(entityEntry, factoryEntry);
		verify(factoryEntry->_factory->Delete(entity));

		entityEntry->_components.Delete(i);
		entityEntry->_componentBits = 0;

		for (ComponentFactoryEntry *factoryEntry: entityEntry->_components)
		{
			entityEntry->_componentBits |= factoryEntry->_bit;
		}

		return true;
	}

	return false;
}

ff::EntityDomain::ComponentFactoryEntries ff::EntityDomain::FindComponentEntries(const String **components)
{
	ComponentFactoryEntries entries;

	for (const String **cur = components; cur && *cur && (*cur)->size(); cur++)
	{
		ComponentFactoryEntry *factoryEntry = GetComponentFactory(**cur);
		assertRetVal(factoryEntry != nullptr, ComponentFactoryEntries());
		entries.Push(factoryEntry);
	}

	return entries;
}

void ff::EntityDomain::FlushDeletedSystems()
{
	for (BucketIter i = _systemsByName.StartIteration(); i != INVALID_ITER; )
	{
		SystemEntry &systemEntry = _systemsByName.ValueAt(i);
		if (!systemEntry._valid)
		{
			i = _systemsByName.DeletePos(i);
		}
		else
		{
			i = _systemsByName.Iterate(i);
		}
	}
}

void ff::EntityDomain::FlushDeletedEntities()
{
	for (EntityEntry *entityEntry: _deletedEntities)
	{
		assert(!entityEntry->_valid);

		for (auto factoryEntry: entityEntry->_components)
		{
			factoryEntry->_factory->Delete(entityEntry->ToEntity());
		}

		_entities.Delete(*entityEntry);
	}

	_deletedEntities.Clear();
}

void ff::EntityDomain::RegisterActivatedEntity(EntityEntry *entityEntry)
{
	assert(entityEntry->_valid && entityEntry->_active);

	if (!entityEntry->_name.empty())
	{
		_entitiesByName.Insert(entityEntry->_name, entityEntry);
	}

	for (SystemEntry *systemEntry: entityEntry->_systems)
	{
		RegisterEntityWithSystem(entityEntry, systemEntry);
	}
}

void ff::EntityDomain::UnregisterDeactivatedEntity(EntityEntry *entityEntry)
{
	assert(entityEntry->_valid && !entityEntry->_active);

	if (!entityEntry->_name.empty())
	{
		bool deleted = false;

		for (BucketIter i = _entitiesByName.Get(entityEntry->_name); i != INVALID_ITER; i = _entitiesByName.GetNext(i))
		{
			if (_entitiesByName.ValueAt(i) == entityEntry)
			{
				_entitiesByName.DeletePos(i);
				deleted = true;
				break;
			}
		}

		assert(deleted);
	}

	for (SystemEntry *systemEntry: entityEntry->_systems)
	{
		UnregisterEntityWithSystem(entityEntry, systemEntry);
	}
}

bool ff::EntityDomain::TryRegisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry)
{
	bool allFound = true;

	if ((systemEntry->_componentBits & entityEntry->_componentBits) == systemEntry->_componentBits &&
		!systemEntry->_components.IsEmpty())
	{
		for (ComponentFactoryEntry *factoryEntry: systemEntry->_components)
		{
			if ((entityEntry->_componentBits & factoryEntry->_bit) == 0 ||
				entityEntry->_components.Find(factoryEntry) == INVALID_SIZE)
			{
				allFound = false;
				break;
			}
		}

		if (allFound)
		{
			entityEntry->_systems.Push(systemEntry);

			if (entityEntry->_active)
			{
				RegisterEntityWithSystem(entityEntry, systemEntry);
			}
		}
	}

	return allFound;
}

void ff::EntityDomain::RegisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry)
{
	assert(entityEntry->_valid && entityEntry->_active);

	struct FakeEntry : public EntityEntryBase
	{
		void *_components[1];
	};

	EntityEntryBase *systemEntityEntry = systemEntry->_system->NewEntry();
	systemEntityEntry->_entity = entityEntry->ToEntity();

	void **derivedComponents = static_cast<FakeEntry *>(systemEntityEntry)->_components;
	for (ComponentFactoryEntry *factoryEntry: systemEntry->_components)
	{
		details::Component *component = LookupComponent(systemEntityEntry->_entity, factoryEntry);
		assert(component != nullptr);

		*derivedComponents = factoryEntry->_factory->CastToVoid(component);
		derivedComponents++;
	}

	systemEntry->_entities.SetKey(systemEntityEntry->_entity, systemEntityEntry);
}

void ff::EntityDomain::UnregisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry)
{
	BucketIter iter = systemEntry->_entities.Get(entityEntry->ToEntity());
	assertRet(iter != INVALID_ITER && entityEntry->_valid && !entityEntry->_active);

	systemEntry->_system->DeleteEntry(systemEntry->_entities.ValueAt(iter));
	systemEntry->_entities.DeletePos(iter);
}

void ff::EntityDomain::RegisterEntityWithSystems(EntityEntry *entityEntry, ComponentFactoryEntry *factoryEntry)
{
	for (SystemEntry *systemEntry: factoryEntry->_systems)
	{
		TryRegisterEntityWithSystem(entityEntry, systemEntry);
	}
}

void ff::EntityDomain::UnregisterEntityWithSystems(EntityEntry *entityEntry, ComponentFactoryEntry *factoryEntry)
{
	for (SystemEntry *systemEntry: factoryEntry->_systems)
	{
		if ((systemEntry->_componentBits & entityEntry->_componentBits) == systemEntry->_componentBits)
		{
			size_t i = entityEntry->_systems.Find(systemEntry);
			assert(i != INVALID_SIZE);

			if (i != INVALID_SIZE)
			{
				UnregisterEntityWithSystem(entityEntry, systemEntry);
				entityEntry->_systems.Delete(i);
			}
		}
	}
}

bool ff::EntityDomain::AddEvent(StringRef name, size_t eventId)
{
	assertRetVal(name.size(), false);
	EventEntry *eventEntry = GetEventEntry(name, eventId, true);
	assertRetVal(eventEntry && eventEntry->_eventId != INVALID_SIZE, false);
	assertRetVal(eventId == INVALID_SIZE || eventEntry->_eventId == eventId, false);
	_eventsById.SetKey(eventEntry->_eventId, eventEntry);

	return true;
}

const ff::String &ff::EntityDomain::GetEventName(size_t eventId) const
{
	EventEntry *eventEntry = GetEventEntry(eventId);
	assertRetVal(eventEntry, GetEmptyString());

	return eventEntry->_name;
}

bool ff::EntityDomain::AddEventHandler(StringRef name, IEntityEventHandler *handler)
{
	return AddEventHandler(GetEventEntry(name, INVALID_SIZE, true), INVALID_ENTITY, handler);
}

bool ff::EntityDomain::AddEventHandler(size_t eventId, IEntityEventHandler *handler)
{
	return AddEventHandler(GetEventEntry(eventId), INVALID_ENTITY, handler);
}

bool ff::EntityDomain::RemoveEventHandler(StringRef name, IEntityEventHandler *handler)
{
	return RemoveEventHandler(GetEventEntry(name, INVALID_SIZE, false), INVALID_ENTITY, handler);
}

bool ff::EntityDomain::RemoveEventHandler(size_t eventId, IEntityEventHandler *handler)
{
	return RemoveEventHandler(GetEventEntry(eventId), INVALID_ENTITY, handler);
}

bool ff::EntityDomain::AddEventHandler(StringRef name, Entity entity, IEntityEventHandler *handler)
{
	return AddEventHandler(GetEventEntry(name, INVALID_SIZE, true), entity, handler);
}

bool ff::EntityDomain::AddEventHandler(size_t eventId, Entity entity, IEntityEventHandler *handler)
{
	return AddEventHandler(GetEventEntry(eventId), entity, handler);
}

bool ff::EntityDomain::RemoveEventHandler(StringRef name, Entity entity, IEntityEventHandler *handler)
{
	return RemoveEventHandler(GetEventEntry(name, INVALID_SIZE, false), entity, handler);
}

bool ff::EntityDomain::RemoveEventHandler(size_t eventId, Entity entity, IEntityEventHandler *handler)
{
	return RemoveEventHandler(GetEventEntry(eventId), entity, handler);
}

bool ff::EntityDomain::AddEventHandler(EventEntry *eventEntry, Entity entity, IEntityEventHandler *handler)
{
	assertRetVal(eventEntry && handler, false);

	SmartEventHandlers &eventHandlers = (entity != INVALID_ENTITY)
		? EntityEntry::FromEntity(entity)->_eventHandlers
		: eventEntry->_eventHandlers;
	SharedEventHandlers::GetUnshared(eventHandlers.Address());

	EventHandler eventHandler;
	eventHandler._event = eventEntry;
	eventHandler._handler = handler;

	eventHandlers->Push(eventHandler);

	return true;
}

bool ff::EntityDomain::RemoveEventHandler(EventEntry *eventEntry, Entity entity, IEntityEventHandler *handler)
{
	assertRetVal(handler, false);

	if (eventEntry == nullptr)
	{
		// never heard of the event, whatever it was
		return false;
	}

	SmartEventHandlers &eventHandlers = (entity != INVALID_ENTITY)
		? EntityEntry::FromEntity(entity)->_eventHandlers
		: eventEntry->_eventHandlers;

	if (eventHandlers != nullptr && !eventHandlers->IsEmpty())
	{
		SharedEventHandlers::GetUnshared(eventHandlers.Address());

		EventHandler eventHandler;
		eventHandler._event = eventEntry;
		eventHandler._handler = handler;

		assertRetVal(eventHandlers->DeleteItem(eventHandler), false);

		if (eventHandlers->IsEmpty())
		{
			eventHandlers = nullptr;
		}

		return true;
	}

	assertRetVal(false, false);
}

ff::EntityDomain::EventEntry *ff::EntityDomain::GetEventEntry(StringRef name, size_t eventId, bool allowCreate)
{
	assertRetVal(name.size(), nullptr);

	BucketIter iter = _eventsByName.Get(name);
	if (iter == INVALID_ITER && allowCreate)
	{
		EventEntry eventEntry;
		eventEntry._name = name;
		eventEntry._eventId = (eventId != INVALID_SIZE) ? eventId : --_nextEventId;

		iter = _eventsByName.SetKey(name, eventEntry);
		assert(!_eventsById.Exists(eventEntry._eventId));
		_eventsById.SetKey(eventEntry._eventId, &_eventsByName.ValueAt(iter));
	}

	return (iter != INVALID_ITER) ? &_eventsByName.ValueAt(iter) : nullptr;
}

ff::EntityDomain::EventEntry *ff::EntityDomain::GetEventEntry(size_t eventId) const
{
	BucketIter iter = _eventsById.Get(eventId);
	assertRetVal(iter != INVALID_ITER, nullptr);
	return _eventsById.ValueAt(iter);
}

bool ff::EntityDomain::TriggerEvent(StringRef name, Entity entity)
{
	return TriggerEvent(GetEventEntry(name, INVALID_SIZE, false), entity, nullptr);
}

bool ff::EntityDomain::TriggerEvent(size_t eventId, Entity entity)
{
	return TriggerEvent(GetEventEntry(eventId), entity, nullptr);
}

bool ff::EntityDomain::TriggerEvent(StringRef name)
{
	return TriggerEvent(GetEventEntry(name, INVALID_SIZE, false), INVALID_ENTITY, nullptr);
}

bool ff::EntityDomain::TriggerEvent(size_t eventId)
{
	return TriggerEvent(GetEventEntry(eventId), INVALID_ENTITY, nullptr);
}

bool ff::EntityDomain::TriggerEvent(EventEntry *eventEntry, Entity entity, void *args)
{
	if (eventEntry == nullptr)
	{
		// nobody registered for the event (whatever it was)
		return false;
	}

	// Call listeners for the specific entity first
	if (entity != INVALID_ENTITY)
	{
		EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
		SmartEventHandlers eventHandlers = entityEntry->_eventHandlers;
		if (eventHandlers != nullptr && !eventHandlers->IsEmpty())
		{
			for (const EventHandler &eventHandler: *eventHandlers)
			{
				if (eventHandler._event == eventEntry)
				{
					eventHandler._handler->OnEntityEvent(this, entity, eventEntry->_eventId, args);
				}
			}
		}
	}

	// Call global listeners too
	{
		SmartEventHandlers eventHandlers = eventEntry->_eventHandlers;
		if (eventHandlers != nullptr && !eventHandlers->IsEmpty())
		{
			for (const EventHandler &eventHandler: *eventHandlers)
			{
				assert(eventHandler._event == eventEntry);
				eventHandler._handler->OnEntityEvent(this, entity, eventEntry->_eventId, args);
			}
		}
	}

	return true;
}

ff::IServiceCollection *ff::EntityDomain::GetServices()
{
	return _services;
}

void ff::EntityDomain::Dump(Log &log)
{
}
