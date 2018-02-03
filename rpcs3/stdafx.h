#pragma once

#ifdef MSVC_CRT_MEMLEAK_DETECTION
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define NOMINMAX

//#ifndef __STDC_CONSTANT_MACROS
//#define __STDC_CONSTANT_MACROS
//#endif

#if defined(MSVC_CRT_MEMLEAK_DETECTION) && defined(_DEBUG) && !defined(DBG_NEW)
	#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	#include "define_new_memleakdetect.h"
#endif

#pragma warning( disable : 4351 )

// MSVC bug workaround
#ifdef _MSC_VER
namespace std { inline namespace literals { inline namespace chrono_literals {}}}
#endif

#include "Utilities/types.h"
#include "Utilities/BEType.h"
#include "Utilities/Atomic.h"
#include "Utilities/StrFmt.h"
#include "Utilities/File.h"
#include "Utilities/Log.h"

#include <cstdlib>
#include <cstring>
#include <climits>
#include <exception>
#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <unordered_map>
#include <algorithm>

using namespace std::literals;

// Remove once we move to C++17
namespace std
{
	template<typename T>
	constexpr const T clamp(const T value, const T min, const T max)
	{
		return value < min ? min : value > max ? max : value;
	}
}
