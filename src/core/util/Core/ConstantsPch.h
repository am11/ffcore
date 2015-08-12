#pragma once

#if defined(UTIL_LIB)
#  define UTIL_API
#  define UTIL_IMPORT
#  define UTIL_EXPORT
#elif defined(UTIL_DLL)
#  define UTIL_API    __declspec(dllexport)
#  define UTIL_IMPORT __declspec(dllimport)
#  define UTIL_EXPORT __declspec(dllexport)
#else
#  define UTIL_API    __declspec(dllimport)
#  define UTIL_IMPORT __declspec(dllimport)
#  define UTIL_EXPORT __declspec(dllexport)
#endif

#define WIDEN2(str) L ## str
#define WIDEN(str) WIDEN2(str)
#define __TFILE__ WIDEN(__FILE__)

// For use in multi-argument macros when you need a real comma
#define COMMA ,

namespace ff
{
	static const double PI_D = 3.1415926535897932384626433832795;
	static const float  PI_F = 3.1415926535897932384626433832795f;

	// Help iteration with size_t
	const size_t INVALID_SIZE = (size_t)-1;
	const size_t INVALID_DWORD = (DWORD)-1;

	typedef size_t Atom;
}
