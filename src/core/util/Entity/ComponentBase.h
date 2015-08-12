#pragma once
#include "Entity/Internal/Component.h"

namespace ff
{
	/// All of your component classes must derive from this.
	template<class DerivedT>
	struct ComponentBase : public details::Component
	{
	};
}

// Component factory helper macros

#define DECLARE_COMPONENT_FACTORY() \
	static ff::StringRef GetFactoryName(); \
	static size_t GetFactoryIndex(); \

#define DEFINE_COMPONENT_FACTORY(className) \
	ff::StringRef className::GetFactoryName() \
	{ \
		static ff::StaticString name(WIDEN(#className)); \
		return name; \
	} \
	size_t className::GetFactoryIndex() \
	{ \
		static size_t s_index = ff::details::Component::GetNextFactoryIndex(); \
		return s_index; \
	}

#define DEFINE_DERIVED_COMPONENT_FACTORY(className, derivedClassName) \
	ff::StringRef className::GetFactoryName() \
	{ \
		static ff::StaticString name(WIDEN(#className)); \
		return name; \
	} \
	size_t className::GetFactoryIndex() \
	{ \
		static size_t s_index = ff::details::Component::GetNextFactoryIndex(); \
		return s_index; \
	}
