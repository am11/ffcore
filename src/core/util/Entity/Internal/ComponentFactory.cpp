#include "pch.h"
#include "Entity/Internal/ComponentFactory.h"

ff::details::ComponentFactory::ComponentFactory(
	size_t dataSize,
	std::function<void(details::Component &)> &&constructor,
	std::function<void(details::Component &, const details::Component &)> &&copyConstructor,
	std::function<void(details::Component &)> &&destructor,
	std::function<details::Component *(void *)> &&castToBase,
	std::function<void *(details::Component *)> &&castFromBase)
	: _dataSize(dataSize)
	, _constructor(std::move(constructor))
	, _copyConstructor(std::move(copyConstructor))
	, _destructor(std::move(destructor))
	, _castToBase(std::move(castToBase))
	, _castFromBase(std::move(castFromBase))
{
}

ff::details::ComponentFactory::ComponentFactory(
	size_t dataSize,
	std::function<details::Component *(void *)> &&castToBase,
	std::function<void *(details::Component *)> &&castFromBase)
	: _dataSize(dataSize)
	, _constructor(nullptr)
	, _copyConstructor(nullptr)
	, _destructor(nullptr)
	, _castToBase(std::move(castToBase))
	, _castFromBase(std::move(castFromBase))
{
}

ff::details::ComponentFactory::~ComponentFactory()
{
	assert(_usedComponents.IsEmpty());

	if (_destructor != nullptr)
	{
		// just in case the component list isn't empty, clean them up
		for (auto &entry: _usedComponents)
		{
			details::Component &component = LookupByIndex(entry.GetValue());
			_destructor(component);
		}
	}
}

ff::details::Component &ff::details::ComponentFactory::New(Entity entity, bool *usedExisting)
{
	details::Component *component = Lookup(entity);

	if (usedExisting != nullptr)
	{
		*usedExisting = (component != nullptr);
	}

	if (component == nullptr)
	{
		size_t index = AllocateIndex();
		component = &LookupByIndex(index);
		_usedComponents.SetKey(entity, index);

		if (_constructor != nullptr)
		{
			_constructor(*component);
		}
	}

	assert(component != nullptr);
	return *component;
}

ff::details::Component *ff::details::ComponentFactory::Clone(Entity entity, Entity sourceEntity)
{
	details::Component *component = Lookup(entity);
	details::Component *sourceComponent = Lookup(sourceEntity);
	assert(component == nullptr);

	if (component == nullptr && sourceComponent != nullptr)
	{
		size_t index = AllocateIndex();
		component = &LookupByIndex(index);
		_usedComponents.SetKey(entity, index);

		if (_copyConstructor != nullptr)
		{
			_copyConstructor(*component, *sourceComponent);
		}
		else
		{
			std::memcpy(component, sourceComponent, _dataSize);
		}
	}

	return component;
}

ff::details::Component *ff::details::ComponentFactory::Lookup(Entity entity) const
{
	size_t index = LookupIndex(entity);
	return index != INVALID_SIZE ? &LookupByIndex(index) : nullptr;
}

bool ff::details::ComponentFactory::Delete(Entity entity)
{
	BucketIter iter = _usedComponents.Get(entity);
	if (iter != INVALID_ITER)
	{
		size_t index = _usedComponents.ValueAt(iter);
		if (_destructor != nullptr)
		{
			details::Component &component = LookupByIndex(index);
			_destructor(component);
		}

		_freeComponents.Push(index);
		_usedComponents.DeletePos(iter);

		return true;
	}

	return false;
}

void ff::details::ComponentFactory::Advance()
{
}

void *ff::details::ComponentFactory::CastToVoid(details::Component *component) const
{
	return _castFromBase(component);
}

size_t ff::details::ComponentFactory::LookupIndex(Entity entity) const
{
	auto iter = _usedComponents.Get(entity);
	return iter != INVALID_ITER ? _usedComponents.ValueAt(iter) : INVALID_SIZE;
}

ff::details::Component &ff::details::ComponentFactory::LookupByIndex(size_t index) const
{
	size_t bucket = index / COMPONENT_BUCKET_SIZE;
	size_t offset = index % COMPONENT_BUCKET_SIZE;
	BYTE *bytes = &_components[bucket]->GetAt(offset * _dataSize);

	return *reinterpret_cast<details::Component *>(bytes);
}

size_t ff::details::ComponentFactory::AllocateIndex()
{
	size_t index;
	if (_freeComponents.IsEmpty())
	{
		ComponentBucket *lastBucket = !_components.IsEmpty() ? _components.GetLast().get() : nullptr;
		if (lastBucket == nullptr || lastBucket->Size() == lastBucket->Allocated())
		{
			_components.Push(std::unique_ptr<ComponentBucket>(new ComponentBucket()));
			lastBucket = _components.GetLast().get();
			lastBucket->Reserve(COMPONENT_BUCKET_SIZE * _dataSize);
		}

		index = (_components.Size() - 1) * COMPONENT_BUCKET_SIZE + lastBucket->Size() / _dataSize;
		lastBucket->InsertDefault(lastBucket->Size(), _dataSize);
	}
	else
	{
		index = _freeComponents.Pop();
	}

	return index;
}
