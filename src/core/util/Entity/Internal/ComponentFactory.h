#pragma once
#include "Entity/Entity.h"

namespace ff
{
	namespace details
	{
		struct Component;

		/// Creates and deletes components of a certain type for any entity
		class ComponentFactory
		{
		public:
			template<class T>
			static std::shared_ptr<ComponentFactory> Create();

			UTIL_API ~ComponentFactory();

			ff::details::Component &New(Entity entity, bool *usedExisting = nullptr);
			ff::details::Component *Clone(Entity entity, Entity sourceEntity);
			ff::details::Component *Lookup(Entity entity) const;
			bool Delete(Entity entity);
			void Advance();

			void *CastToVoid(details::Component *component) const;

			template<class T> T &New(Entity entity, bool *usedExisting = nullptr);
			template<class T> T *Clone(Entity entity, Entity sourceEntity);
			template<class T> T *Lookup(Entity entity) const;

		private:
			UTIL_API ComponentFactory(
				size_t dataSize,
				std::function<void(details::Component &)> &&constructor,
				std::function<void(details::Component &, const details::Component &)> &&copyConstructor,
				std::function<void(details::Component &)> &&destructor,
				std::function<details::Component *(void *)> &&castToBase,
				std::function<void *(details::Component *)> &&castFromBase);

			UTIL_API ComponentFactory(
				size_t dataSize,
				std::function<details::Component *(void *)> &&castToBase,
				std::function<void *(details::Component *)> &&castFromBase);

			size_t LookupIndex(Entity entity) const;
			details::Component &LookupByIndex(size_t index) const;
			size_t AllocateIndex();

			size_t _dataSize;
			std::function<void(details::Component &)> _constructor;
			std::function<void(details::Component &, const details::Component &)> _copyConstructor;
			std::function<void(details::Component &)> _destructor;
			std::function<details::Component *(void *)> _castToBase;
			std::function<void *(details::Component *)> _castFromBase;

			static const size_t COMPONENT_BUCKET_SIZE = 256;
			typedef Vector<BYTE, 0, AlignedByteAllocator<16>> ComponentBucket;

			Vector<std::unique_ptr<ComponentBucket>, 8> _components;
			Vector<size_t> _freeComponents;
			Map<Entity, size_t> _usedComponents;
		};
	}
}

#include "Entity/Internal/ComponentFactoryImpl.h"
