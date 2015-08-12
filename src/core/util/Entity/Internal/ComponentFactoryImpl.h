#pragma once

template<class T>
std::shared_ptr<ff::details::ComponentFactory> ff::details::ComponentFactory::Create()
{
	assert(16 % __alignof(T) == 0);

	if (ff::IsPlainOldData<T>::value)
	{
		return std::shared_ptr<ComponentFactory>(
			new ComponentFactory(
				sizeof(T),
				// castToBase
				[](void *component)
				{
					return static_cast<details::Component *>(reinterpret_cast<T *>(component));
				},
				// castFromBase
				[](details::Component *component)
				{
					return static_cast<T *>(component);
				}));
	}

	return std::shared_ptr<ComponentFactory>(
		new ComponentFactory(
			sizeof(T),
			// T::T constructor
			[](details::Component &component)
			{
				T *myComponent = static_cast<T *>(&component);
				::new(myComponent) T();
			},
			// T::T(T) constructor
			[](details::Component &component, const details::Component &sourceComponent)
			{
				T *myComponent = static_cast<T *>(&component);
				const T *mySourceComponent = static_cast<const T *>(&sourceComponent);
				::new(myComponent) T(*mySourceComponent);
			},
			// T::~T destructor
			[](details::Component &component)
			{
				T *myComponent = static_cast<T *>(&component);
				myComponent->~T();
			},
			// castToBase
			[](void *component)
			{
				return static_cast<details::Component *>(reinterpret_cast<T *>(component));
			},
			// castFromBase
			[](details::Component *component)
			{
				return static_cast<T *>(component);
			}));
}

template<class T>
T &ff::details::ComponentFactory::New(Entity entity, bool *usedExisting)
{
	return static_cast<T &>(New(entity, usedExisting));
}

template<class T>
T *ff::details::ComponentFactory::Clone(Entity entity, Entity sourceEntity)
{
	return static_cast<T *>(Clone(entity, sourceEntity));
}

template<class T>
T *ff::details::ComponentFactory::Lookup(Entity entity) const
{
	return static_cast<T *>(Lookup(entity));
}
