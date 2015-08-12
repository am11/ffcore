#include "pch.h"
#include "Entity/Internal/Component.h"

static size_t s_nextFactoryIndex = 0;

// static
size_t ff::details::Component::GetNextFactoryIndex()
{
	return InterlockedIncrement(&s_nextFactoryIndex) - 1;
}
