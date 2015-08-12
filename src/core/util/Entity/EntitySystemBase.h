#pragma once
#include "Entity/Internal/EntitySystem.h"

namespace ff
{
	/// Derived classes must add a list of component pointers of the correct type,
	/// not ComponentBase, but the actual type of the component.
	/// After the components, it's OK to add custom state variables.
	struct EntityEntryBase
	{
		Entity _entity;
	};

	struct EmptyEntityEntry : public EntityEntryBase
	{
		UTIL_API static const String **GetComponentNames();
	};

	/// All of your entity systems must derive from this class.
	/// EntryT must derive from EntityEntryBase
	template<class EntryT = EmptyEntityEntry>
	class EntitySystemBase : public details::EntitySystem
	{
	public:
		EntitySystemBase(EntityDomain *domain);
		virtual ~EntitySystemBase();

		virtual StringRef GetDefaultName() const override;
		virtual EntityEntryBase *NewEntry() final;
		virtual void DeleteEntry(EntityEntryBase *entry) final;

	protected:
		const EntryT *GetFirstEntry() const;
		const EntryT *GetNextEntry(const EntryT &entry) const;
		size_t GetEntryCount() const;
		size_t GetEntryChangeStamp() const;

	private:
		String _defaultName;
		List<EntryT> _entries;
		size_t _changeStamp;
	};
}

#include "Entity/Internal/EntitySystemBaseImpl.h"

// Helper macros

#define DECLARE_SYSTEM_COMPONENTS() \
	static const ff::String **GetComponentNames()

#define DECLARE_SYSTEM_COMPONENTS_EXPORT(apiType) \
	apiType static const ff::String **GetComponentNames()

#define BEGIN_SYSTEM_COMPONENTS(className) \
	const ff::String **className::GetComponentNames() \
	{ \
		static const ff::String *s_names[] = \
		{ \

#define HAS_COMPONENT(componentClass) \
			&componentClass::GetFactoryName(),

#define END_SYSTEM_COMPONENTS() \
			nullptr, \
		}; \
		return s_names; \
	}
