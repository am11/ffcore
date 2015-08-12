#include "pch.h"
#include "Entity/ComponentBase.h"
#include "Entity/EntityDomain.h"
#include "Entity/EntityEvents.h"
#include "Entity/EntitySystemBase.h"

struct TestComponent1 : public ff::ComponentBase<TestComponent1>
{
	TestComponent1();
	DECLARE_COMPONENT_FACTORY();

	// Data
	ff::PointInt _pos;
	float _speed;
};

DEFINE_COMPONENT_FACTORY(TestComponent1);

TestComponent1::TestComponent1()
	: _pos(0, 0)
	, _speed(1)
{
}

struct TestComponent2 : public ff::ComponentBase<TestComponent2>
{
	TestComponent2();
	DECLARE_COMPONENT_FACTORY();

	// Data
	ff::RectInt _bounds;
	ff::RectInt _prevBounds;
};

DEFINE_COMPONENT_FACTORY(TestComponent2);

TestComponent2::TestComponent2()
	: _bounds(0, 0, 1, 1)
	, _prevBounds(0, 0, 0, 0)
{
}

struct TestSystemEntry : ff::EntityEntryBase
{
	TestSystemEntry();

	DECLARE_SYSTEM_COMPONENTS();
	TestComponent1 *_comp1;
	TestComponent2 *_comp2;

	// Extra data can go after the components (not before)
	bool _test;
};

BEGIN_SYSTEM_COMPONENTS(TestSystemEntry)
	HAS_COMPONENT(TestComponent1)
	HAS_COMPONENT(TestComponent2)
END_SYSTEM_COMPONENTS()

TestSystemEntry::TestSystemEntry()
{
	ZeroObject(*this);
}

class TestSystem : public ff::EntitySystemBase<TestSystemEntry>
{
	typedef ff::EntitySystemBase<TestSystemEntry> MyBase;

public:
	TestSystem(ff::EntityDomain *domain);
	virtual ~TestSystem();

	size_t TestGetEntryCount() const;
	const TestSystemEntry *TestGetFirstEntry() const;
};

TestSystem::TestSystem(ff::EntityDomain *domain)
	: MyBase::EntitySystemBase(domain)
{
}

TestSystem::~TestSystem()
{
}

size_t TestSystem::TestGetEntryCount() const
{
	return GetEntryCount();
}

const TestSystemEntry *TestSystem::TestGetFirstEntry() const
{
	return GetFirstEntry();
}

struct TestEventArgs
{
	TestEventArgs();

	int _arg1;
	int _arg2;
};

TestEventArgs::TestEventArgs()
	: _arg1(1)
	, _arg2(2)
{
}

class TestEventHandler : public ff::IEntityEventHandler
{
public:
	TestEventHandler();

	virtual void OnEntityEvent(
		ff::EntityDomain *domain,
		ff::Entity entity,
		size_t eventId,
		void *eventArgs) override;

	int _count;
};

TestEventHandler::TestEventHandler()
	: _count(0)
{
}

void TestEventHandler::OnEntityEvent(
	ff::EntityDomain *domain,
	ff::Entity entity,
	size_t eventId,
	void *eventArgs)
{
	if (entity == ff::INVALID_ENTITY)
	{
		_count += 1000;
	}

	if (eventArgs)
	{
		TestEventArgs *realArgs = reinterpret_cast<TestEventArgs *>(eventArgs);
		realArgs->_arg1++;
		_count += 100;
	}

	_count++;
}

bool EntityTest()
{
	ff::EntityDomain domain;

	domain.AddComponentFactory<TestComponent1>();
	domain.AddComponentFactory<TestComponent2>();
	TestSystem *testSystem = domain.AddSystem<TestSystem>();

	ff::Entity entity1 = domain.CreateEntity(ff::String(L"TestEntity"));
	assertRetVal(domain.GetEntityName(entity1) == L"TestEntity", false);
	assertRetVal(domain.GetEntity(ff::String(L"TestEntity")) == ff::INVALID_ENTITY, false); // not activated yet

	TestComponent1 *t1 = domain.AddComponent<TestComponent1>(entity1);
	TestComponent2 *t2 = domain.AddComponent<TestComponent2>(entity1);
	TestComponent2 *t3 = domain.AddComponent<TestComponent2>(entity1);
	assertRetVal(domain.LookupComponent<TestComponent1>(entity1) == t1, false);
	assertRetVal(domain.LookupComponent<TestComponent2>(entity1) == t2, false);
	assertRetVal(t2 == t3, false);

	ff::RectInt testBounds(2, 3, 4, 5);
	t2->_bounds = testBounds;

	assertRetVal(testSystem->TestGetEntryCount() == 0, false);

	domain.ActivateEntity(entity1);
	assertRetVal(domain.GetEntity(ff::String(L"TestEntity")) == entity1, false);

	ff::Entity entity2 = domain.CloneEntity(entity1);
	assertRetVal(testSystem->TestGetEntryCount() == 1, false);

	domain.ActivateEntity(entity2);
	assertRetVal(testSystem->TestGetEntryCount() == 2, false);

	const TestSystemEntry *firstEntry = testSystem->TestGetFirstEntry();
	assertRetVal(firstEntry, false);
	assertRetVal(firstEntry->_entity == entity1, false);
	assertRetVal(firstEntry->_comp1 == t1, false);
	assertRetVal(firstEntry->_comp2 == t2, false);
	assertRetVal(firstEntry->_comp2->_bounds == testBounds, false);

	domain.DeactivateEntity(entity1);
	assertRetVal(testSystem->TestGetEntryCount() == 1, false);

	const size_t testEventId = 100;
	ff::String testEventName(L"TestEvent");
	domain.AddEvent(testEventName, testEventId);

	TestEventHandler handler;
	assertRetVal(domain.AddEventHandler(testEventName, entity1, &handler), false);
	assertRetVal(domain.AddEventHandler(testEventId, entity1, &handler), false);
	assertRetVal(domain.AddEventHandler(testEventId, entity2, &handler), false);
	{
		ff::EntityEventConnection eventConnection;
		assertRetVal(eventConnection.Connect(&domain, testEventId, &handler), false);

		TestEventArgs args;
		assertRetVal(domain.TriggerEvent(testEventId), false);
		assertRetVal(domain.TriggerEvent(testEventName), false);
		assertRetVal(!domain.TriggerEvent(ff::String(L"TestEvent2")), false);
		assertRetVal(domain.TriggerEvent(testEventId, &args), false);
		assertRetVal(handler._count == 3103 && args._arg1 == 2 && args._arg2 == 2, false);
	}
	assertRetVal(domain.RemoveEventHandler(testEventName, entity1, &handler), false);
	assertRetVal(domain.RemoveEventHandler(testEventId, entity1, &handler), false);
	assertRetVal(domain.RemoveEventHandler(testEventId, entity2, &handler), false);
	assertRetVal(!domain.RemoveEventHandler(ff::String(L"TestEvent2"), &handler), false);

	domain.DeleteEntity(entity1);
	assertRetVal(testSystem->TestGetEntryCount() == 1, false);

	return true;
}
