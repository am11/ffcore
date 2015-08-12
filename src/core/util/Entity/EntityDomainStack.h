#pragma once

namespace ff
{
	class AppGlobals;
	class EntityDomain;

	class EntityDomainStack
	{
	public:
		UTIL_API EntityDomainStack(AppGlobals *app);
		UTIL_API ~EntityDomainStack();

		UTIL_API std::shared_ptr<EntityDomain> Push();
		UTIL_API bool Pop();
		UTIL_API bool Remove(EntityDomain *domain);
		UTIL_API void Clear();
		UTIL_API bool IsEmpty() const;

		UTIL_API std::shared_ptr<EntityDomain> Peek();
		UTIL_API std::shared_ptr<EntityDomain> PeekAfter(EntityDomain *domain, size_t index);

	private:
		ff::AppGlobals *_app;
		ff::Vector<std::shared_ptr<EntityDomain>> _domains;
	};
}
