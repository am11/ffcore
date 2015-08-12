#pragma once

#include "Entity/Entity.h"

namespace ff
{
	class AppGlobals;
	class EntityDomain;
	class IRenderTarget;
	class ProcessGlobals;
	struct EntityEntryBase;

	namespace details
	{
		/// Processes all entities that contain the specified types of components
		class EntitySystem
		{
		public:
			UTIL_API EntitySystem(EntityDomain *domain, const String **components);
			UTIL_API virtual ~EntitySystem();

			UTIL_API ProcessGlobals *GetGlobals() const;
			UTIL_API AppGlobals *GetApp() const;
			UTIL_API EntityDomain *GetDomain() const;
			const String **GetComponents() const;

			template<class T>
			bool GetService(T **obj) const;

			UTIL_API virtual void Ready();
			UTIL_API virtual void Advance();
			UTIL_API virtual void Render(IRenderTarget *target);
			UTIL_API virtual void Cleanup();

			UTIL_API virtual void OnDomainActivated();
			UTIL_API virtual void OnDomainDectivated();

			virtual StringRef GetDefaultName() const = 0;
			virtual EntityEntryBase *NewEntry() = 0;
			virtual void DeleteEntry(EntityEntryBase *entry) = 0;

		private:
			bool GetService(REFGUID guidService, void **obj) const;

			EntityDomain *_domain;
			const String **_components;
		};

		template<class T>
		bool EntitySystem::GetService(T **obj) const
		{
			return GetService(__uuidof(T), (void **)obj);
		}
	}
}
