#include "pch.h"
#include "Resource/ResourceHandle.h"

ff::ResourceHandle::ResourceHandle(StringRef name)
	: _name(name)
	, _refs(0)
{
}

ff::ResourceHandle::ResourceHandle(ResourceHandle &rhs)
	: _refs(0)
{
	*this = rhs;
}

ff::ResourceHandle::ResourceHandle(ResourceHandle &&rhs)
	: _name(std::move(rhs._name))
	, _object(std::move(rhs._object))
	, _refs(0)
{
}

ff::ResourceHandle::~ResourceHandle()
{
	assert(!_refs);
}

ff::ResourceHandle &ff::ResourceHandle::operator=(const ResourceHandle &rhs)
{
	if (this != &rhs)
	{
		_name = rhs._name;
		_object = rhs._object;
	}

	return *this;
}

void ff::ResourceHandle::SetObject(IUnknown *value)
{
	if (_object != value)
	{
		_object = value;
		OnObjectChanged(value);
	}
}

void ff::ResourceHandle::OnObjectChanged(IUnknown *value)
{
}

const ff::String &ff::ResourceHandle::GetName() const
{
	return _name;
}

IUnknown *ff::ResourceHandle::GetObject() const
{
	return _object;
}

void ff::ResourceHandle::AddRef()
{
	InterlockedIncrement(&_refs);
}

void ff::ResourceHandle::Release()
{
	if (InterlockedDecrement(&_refs) == 0)
	{
		_object = nullptr;
	}
}

