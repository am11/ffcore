#pragma once

// C-Runtime includes
// Precompiled header only

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <functional>
#include <memory>
#include <new>
#include <typeinfo>
#include <utility>

namespace ff
{
	template<class T>
	struct IsPlainOldData
	{
		static const bool value = std::is_pod<T>::value;
		static bool Value() { return value; }
	};
}

#define MAKE_POD(name) \
	template<> struct ff::IsPlainOldData<name> \
	{ static const bool value = true; }
