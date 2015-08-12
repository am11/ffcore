#include "pch.h"
#include "Entity/EntityDomain.h"
#include "Entity/EntityDomainStack.h"

ff::EntityDomainStack::EntityDomainStack(AppGlobals *app)
	: _app(app)
{
}

ff::EntityDomainStack::~EntityDomainStack()
{
}

std::shared_ptr<ff::EntityDomain> ff::EntityDomainStack::Push()
{
	std::shared_ptr<ff::EntityDomain> domain = std::make_shared<ff::EntityDomain>(_app);
	_domains.Push(domain);
	return domain;
}

bool ff::EntityDomainStack::Pop()
{
	assertRetVal(!_domains.IsEmpty(), false);
	_domains.Pop();
	return true;
}

bool ff::EntityDomainStack::Remove(EntityDomain *domain)
{
	for (size_t i = 0; i < _domains.Size(); i++)
	{
		if (_domains[i].get() == domain)
		{
			_domains.Delete(i);
			return true;
		}
	}

	assertRetVal(false, false);
}

void ff::EntityDomainStack::Clear()
{
	_domains.Clear();
}

bool ff::EntityDomainStack::IsEmpty() const
{
	return _domains.IsEmpty();
}

std::shared_ptr<ff::EntityDomain> ff::EntityDomainStack::Peek()
{
	return !_domains.IsEmpty()
		? _domains[0]
		: std::shared_ptr<EntityDomain>();
}

std::shared_ptr<ff::EntityDomain> ff::EntityDomainStack::PeekAfter(EntityDomain *domain, size_t index)
{
	for (size_t i = 0; i < _domains.Size(); i++)
	{
		if (_domains[i].get() == domain)
		{
			for (i++; index > 0; index--)
			{
				i++;
			}

			if (i < _domains.Size())
			{
				return _domains[i];
			}

			break;
		}
	}

	return std::shared_ptr<EntityDomain>();
}
