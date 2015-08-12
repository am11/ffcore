#pragma once

template<class T>
bool ff::EntityDomain::AddComponentFactory()
{
	size_t index = INVALID_SIZE;
	StringRef name = T::GetFactoryName();

	__if_exists (T::GetFactoryIndex)
	{
		index = T::GetFactoryIndex();
	}

	return AddComponentFactory<T>(name, index);
}

template<class T>
bool ff::EntityDomain::AddComponentFactory(StringRef name, size_t index)
{
	return AddComponentFactory(name, index, details::ComponentFactory::Create<T>());
}

template<class T>
T *ff::EntityDomain::AddComponent(Entity entity)
{
	__if_exists (T::GetFactoryIndex)
	{
		return AddComponent<T>(entity, T::GetFactoryIndex());
	}

	__if_not_exists (T::GetFactoryIndex)
	{
		return AddComponent<T>(entity, T::GetFactoryName());
	}
}

template<class T>
T *ff::EntityDomain::AddComponent(Entity entity, StringRef componentName)
{
	return static_cast<T *>(AddComponent(entity, GetComponentFactory(componentName)));
}

template<class T>
T *ff::EntityDomain::AddComponent(Entity entity, size_t componentIndex)
{
	return static_cast<T *>(AddComponent(entity, GetComponentFactory(componentIndex)));
}

template<class T>
T *ff::EntityDomain::CloneComponent(Entity entity, Entity sourceEntity)
{
	__if_exists (T::GetFactoryIndex)
	{
		return CloneComponent<T>(entity, sourceEntity, T::GetFactoryIndex());
	}

	__if_not_exists (T::GetFactoryIndex)
	{
		return CloneComponent<T>(entity, sourceEntity, T::GetFactoryName());
	}
}

template<class T>
T *ff::EntityDomain::CloneComponent(Entity entity, Entity sourceEntity, StringRef componentName)
{
	return static_cast<T *>(CloneComponent(entity, sourceEntity, GetComponentFactory(componentName)));
}

template<class T>
T *ff::EntityDomain::CloneComponent(Entity entity, Entity sourceEntity, size_t componentIndex)
{
	return static_cast<T *>(CloneComponent(entity, sourceEntity, GetComponentFactory(componentIndex)));
}

template<class T>
T *ff::EntityDomain::LookupComponent(Entity entity)
{
	__if_exists (T::GetFactoryIndex)
	{
		return LookupComponent<T>(entity, T::GetFactoryIndex());
	}

	__if_not_exists (T::GetFactoryIndex)
	{
		return LookupComponent<T>(entity, T::GetFactoryName());
	}
}

template<class T>
T *ff::EntityDomain::LookupComponent(Entity entity, StringRef componentName)
{
	return static_cast<T *>(LookupComponent(entity, GetComponentFactory(componentName)));
}

template<class T>
T *ff::EntityDomain::LookupComponent(Entity entity, size_t componentIndex)
{
	return static_cast<T *>(LookupComponent(entity, GetComponentFactory(componentIndex)));
}

template<class T>
bool ff::EntityDomain::DeleteComponent(Entity entity)
{
	__if_exists (T::GetFactoryIndex)
	{
		return DeleteComponent<T>(entity, T::GetFactoryIndex());
	}

	__if_not_exists (T::GetFactoryIndex)
	{
		return DeleteComponent<T>(entity, T::GetFactoryName());
	}
}

template<class T>
bool ff::EntityDomain::DeleteComponent(Entity entity, StringRef componentName)
{
	return DeleteComponent(entity, GetComponentFactory(componentName));
}

template<class T>
bool ff::EntityDomain::DeleteComponent(Entity entity, size_t componentIndex)
{
	return DeleteComponent(entity, GetComponentFactory(componentIndex));
}

template<class T>
T *ff::EntityDomain::AddSystem()
{
	auto system = std::make_shared<T>(this);
	return AddSystem(system) ? system.get() : nullptr;
}

template<class ArgsT>
bool ff::EntityDomain::TriggerEvent(StringRef name, Entity entity, ArgsT *args)
{
	return TriggerEvent(GetEventEntry(name, INVALID_SIZE, false), entity, args);
}

template<class ArgsT>
bool ff::EntityDomain::TriggerEvent(size_t eventId, Entity entity, ArgsT *args)
{
	return TriggerEvent(GetEventEntry(eventId), entity, args);
}

template<class ArgsT>
bool ff::EntityDomain::TriggerEvent(StringRef name, ArgsT *args)
{
	return TriggerEvent(GetEventEntry(name, INVALID_SIZE, false), INVALID_ENTITY, args);
}

template<class ArgsT>
bool ff::EntityDomain::TriggerEvent(size_t eventId, ArgsT *args)
{
	return TriggerEvent(GetEventEntry(eventId), INVALID_ENTITY, args);
}
