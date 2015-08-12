#pragma once
#include "Entity/Internal/ComponentFactory.h"

namespace ff
{
	class AppGlobals;
	class IEntityEventHandler;
	class IRenderTarget;
	class IResourceContext;
	class IServiceCollection;
	class Log;
	class ProcessGlobals;
	struct EntityEntryBase;

	namespace details
	{
		class EntitySystem;
	}

	/// All entities, components, and systems are owned and managed by a domain.
	///
	/// Domains are self-contained, so create as many as you need and they can
	/// be handled independently.
	///
	/// After you create a domain, you must populate it with:
	///
	/// - Component factories first
	///   - Call AddComponentFactory<T>
	///   - <T> must be a component type that derives from ff::ComponentBase<T>
	///
	/// - Systems second
	///   - Call AddSystem<T>
	///   - <T> must be a system type that derives from ff::EntitySystemBase<EntryT>
	///
	/// - Event names anytime
	///   - Call AddEvent
	///
	/// Then it's OK to create entities afterwards (call CreateEntity)
	class EntityDomain
	{
	public:
		UTIL_API EntityDomain();
		UTIL_API EntityDomain(const EntityDomain *parent);
		UTIL_API EntityDomain(AppGlobals *parent);
		UTIL_API EntityDomain(IResourceContext *parent);
		UTIL_API ~EntityDomain();

		UTIL_API static EntityDomain *FromEntity(Entity entity);
		UTIL_API AppGlobals *GetAppGlobals() const;
		UTIL_API ProcessGlobals *GetProcessGlobals() const;

		void Activate();
		void Deactivate();
		bool IsActive() const;

		// Running the game
		void Advance();
		void Render(IRenderTarget *target);

		// Component methods (T must derive from ComponentBase)
		template<class T> bool AddComponentFactory();
		template<class T> T *AddComponent(Entity entity);
		template<class T> T *CloneComponent(Entity entity, Entity sourceEntity);
		template<class T> T *LookupComponent(Entity entity);
		template<class T> bool DeleteComponent(Entity entity);

		// System methods
		template<class T> T *AddSystem();
		UTIL_API bool AddSystem(std::shared_ptr<details::EntitySystem> system, StringRef nameOverride = String());
		UTIL_API bool RemoveSystem(details::EntitySystem *system);
		UTIL_API details::EntitySystem *GetSystem(StringRef name) const;

		// Entity methods (new entities are not activated by default).
		// Only active entities will be registered with systems.
		// Only active entities will have their names registered for lookup (using GetEntity).
		UTIL_API Entity CreateEntity(StringRef name = String());
		UTIL_API Entity CloneEntity(Entity sourceEntity, StringRef name = String());
		UTIL_API Entity GetEntity(StringRef name) const;
		UTIL_API const String &GetEntityName(Entity entity) const;
		UTIL_API void ActivateEntity(Entity entity);
		UTIL_API void DeactivateEntity(Entity entity);
		UTIL_API void DeleteEntity(Entity entity);

		// Events
		UTIL_API bool AddEvent(StringRef name, size_t eventId = INVALID_SIZE);
		UTIL_API const String &GetEventName(size_t eventId) const;

		// Event triggers can be entity-specific or global. They can have arguments or not.
		template<class ArgsT> bool TriggerEvent(StringRef name, Entity entity, ArgsT *args);
		template<class ArgsT> bool TriggerEvent(size_t eventId, Entity entity, ArgsT *args);
		template<class ArgsT> bool TriggerEvent(StringRef name, ArgsT *args);
		template<class ArgsT> bool TriggerEvent(size_t eventId, ArgsT *args);
		UTIL_API bool TriggerEvent(StringRef name, Entity entity);
		UTIL_API bool TriggerEvent(size_t eventId, Entity entity);
		UTIL_API bool TriggerEvent(StringRef name);
		UTIL_API bool TriggerEvent(size_t eventId);

		// Event registration can be entity-specific or global, by using the event name or ID.
		UTIL_API bool AddEventHandler(StringRef name, IEntityEventHandler *handler);
		UTIL_API bool AddEventHandler(size_t eventId, IEntityEventHandler *handler);
		UTIL_API bool RemoveEventHandler(StringRef name, IEntityEventHandler *handler);
		UTIL_API bool RemoveEventHandler(size_t eventId, IEntityEventHandler *handler);
		UTIL_API bool AddEventHandler(StringRef name, Entity entity, IEntityEventHandler *handler);
		UTIL_API bool AddEventHandler(size_t eventId, Entity entity, IEntityEventHandler *handler);
		UTIL_API bool RemoveEventHandler(StringRef name, Entity entity, IEntityEventHandler *handler);
		UTIL_API bool RemoveEventHandler(size_t eventId, Entity entity, IEntityEventHandler *handler);

		// Domain services
		IServiceCollection *GetServices();

		UTIL_API void Dump(Log &log);

	private:
		void Init(const EntityDomain *parentDomain, AppGlobals *parentApp, IResourceContext *parentResourceContext);

		// Data structures

		struct ComponentFactoryEntry;
		struct SystemEntry;
		struct EntityEntry;
		struct EventEntry;

		struct EventHandler
		{
			bool operator==(const EventHandler &rhs) const;

			EventEntry *_event;
			IEntityEventHandler *_handler;
		};

		typedef Vector<ComponentFactoryEntry *, 8> ComponentFactoryEntries;
		typedef Vector<SystemEntry *, 8> SystemEntries;
		typedef Vector<EntityEntry *, 8> EntityEntries;
		typedef Vector<EventHandler, 8> EventHandlers;
		typedef SharedObject<EventHandlers> SharedEventHandlers;
		typedef SmartPtr<SharedEventHandlers> SmartEventHandlers;

		struct ComponentFactoryEntry
		{
			std::shared_ptr<details::ComponentFactory> _factory;
			String _name;
			size_t _index; // for quick array lookup
			uint64_t _bit; // mostly unique bit (not unique when there are more than 64 factories)
			size_t _bitIndex; // unique index used to generate _bit
			SystemEntries _systems; // all systems that care about this type of component
		};

		struct SystemEntry
		{
			std::shared_ptr<details::EntitySystem> _system;
			String _name;
			uint64_t _componentBits;
			ComponentFactoryEntries _components;
			Map<Entity, EntityEntryBase *> _entities;
			bool _valid;
		};

		struct EntityEntry
		{
			static EntityEntry *FromEntity(Entity entity);
			Entity ToEntity();

			EntityDomain *_domain;
			String _name;
			uint64_t _componentBits;
			ComponentFactoryEntries _components;
			SystemEntries _systems;
			SmartEventHandlers _eventHandlers;
			bool _valid; // false after this entity has been deleted
			bool _active;
		};

		struct EventEntry
		{
			String _name;
			size_t _eventId;
			SmartEventHandlers _eventHandlers; // global handlers
		};

		// Component template methods
		template<class T> bool AddComponentFactory(StringRef name, size_t index);
		template<class T> T *AddComponent(Entity entity, StringRef componentName);
		template<class T> T *AddComponent(Entity entity, size_t componentIndex);
		template<class T> T *CloneComponent(Entity entity, Entity sourceEntity, StringRef componentName);
		template<class T> T *CloneComponent(Entity entity, Entity sourceEntity, size_t componentIndex);
		template<class T> T *LookupComponent(Entity entity, StringRef componentName);
		template<class T> T *LookupComponent(Entity entity, size_t componentIndex);
		template<class T> bool DeleteComponent(Entity entity, StringRef componentName);
		template<class T> bool DeleteComponent(Entity entity, size_t componentIndex);

		// Component methods
		UTIL_API bool AddComponentFactory(StringRef name, size_t index, std::shared_ptr<details::ComponentFactory> factory);
		UTIL_API ComponentFactoryEntry *GetComponentFactory(StringRef name);
		UTIL_API ComponentFactoryEntry *GetComponentFactory(size_t index);
		UTIL_API details::Component *AddComponent(Entity entity, ComponentFactoryEntry *factoryEntry);
		UTIL_API details::Component *CloneComponent(Entity entity, Entity sourceEntity, ComponentFactoryEntry *factoryEntry);
		UTIL_API details::Component *LookupComponent(Entity entity, ComponentFactoryEntry *factoryEntry);
		UTIL_API bool DeleteComponent(Entity entity, ComponentFactoryEntry *factoryEntry);
		ComponentFactoryEntries FindComponentEntries(const String **components);

		// System methods
		void FlushDeletedSystems();

		// Entity methods
		void FlushDeletedEntities();
		void RegisterActivatedEntity(EntityEntry *entityEntry);
		void UnregisterDeactivatedEntity(EntityEntry *entityEntry);
		bool TryRegisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry);
		void RegisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry);
		void UnregisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry);
		void RegisterEntityWithSystems(EntityEntry *entityEntry, ComponentFactoryEntry *newFactory);
		void UnregisterEntityWithSystems(EntityEntry *entityEntry, ComponentFactoryEntry *deletingFactory);

		// Event methods
		UTIL_API bool TriggerEvent(EventEntry *eventEntry, Entity entity, void *args);
		UTIL_API bool AddEventHandler(EventEntry *eventEntry, Entity entity, IEntityEventHandler *handler);
		UTIL_API bool RemoveEventHandler(EventEntry *eventEntry, Entity entity, IEntityEventHandler *handler);
		UTIL_API EventEntry *GetEventEntry(StringRef name, size_t eventId, bool allowCreate);
		UTIL_API EventEntry *GetEventEntry(size_t eventId) const;

		// Parent data
		const EntityDomain *_parentDomain;
		AppGlobals *_parentApp;
		IResourceContext *_parentResourceContext;

		// Component data
		ComponentFactoryEntries _componentFactories;
		ComponentFactoryEntries _componentFactoriesByIndex;
		Map<String, ComponentFactoryEntry> _componentFactoriesByName; // actual storage

		// System data
		SystemEntries _systems;
		Map<String, SystemEntry> _systemsByName; // actual storage

		// Entity data
		List<EntityEntry> _entities; // actual storage
		Map<String, EntityEntry *> _entitiesByName;
		EntityEntries _deletedEntities;

		// Event data
		Map<String, EventEntry> _eventsByName; // actual storage
		Map<size_t, EventEntry *> _eventsById;
		size_t _nextEventId;

		// Services
		ComPtr<IServiceCollection> _services;

		// Other
		bool _active;
	};
}

#include "Entity/Internal/EntityDomainImpl.h"
