#pragma once

#ifdef MSVC_CRT_MEMLEAK_DETECTION
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define NOMINMAX

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
#include "util/atomic.hpp"
#include "Utilities/StrFmt.h"
#include "Utilities/File.h"
#include "util/logs.hpp"

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
#include <string_view>

using namespace std::literals;
