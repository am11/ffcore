#include "pch.h"
#include "COM/ServiceCollection.h"
#include "Entity/EntityDomain.h"
#include "Entity/Internal/EntitySystem.h"

ff::details::EntitySystem::EntitySystem(EntityDomain *domain, const String **components)
	: _domain(domain)
	, _components(components)
{
	assert(_domain != nullptr);
}

ff::details::EntitySystem::~EntitySystem()
{
}

ff::ProcessGlobals *ff::details::EntitySystem::GetGlobals() const
{
	return _domain->GetProcessGlobals();
}

ff::AppGlobals *ff::details::EntitySystem::GetApp() const
{
	return _domain->GetAppGlobals();
}

ff::EntityDomain *ff::details::EntitySystem::GetDomain() const
{
	return _domain;
}

const ff::String **ff::details::EntitySystem::GetComponents() const
{
	return _components;
}

void ff::details::EntitySystem::Ready()
{
}

void ff::details::EntitySystem::Advance()
{
}

void ff::details::EntitySystem::Render(IRenderTarget *target)
{
}

void ff::details::EntitySystem::Cleanup()
{
}

void ff::details::EntitySystem::OnDomainActivated()
{
}

void ff::details::EntitySystem::OnDomainDectivated()
{
}

bool ff::details::EntitySystem::GetService(REFGUID guidService, void **obj) const
{
	return SUCCEEDED(_domain->GetServices()->QueryService(guidService, guidService, obj));
}
