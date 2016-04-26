#pragma once

#ifdef MSVC_CRT_MEMLEAK_DETECTION
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define NOMINMAX

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#if defined(MSVC_CRT_MEMLEAK_DETECTION) && defined(_DEBUG) && !defined(DBG_NEW)
	#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	#include "define_new_memleakdetect.h"
#endif

#pragma warning( disable : 4351 )

#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cstring>
#include <string>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <chrono>

using namespace std::string_literals;
using namespace std::chrono_literals;

// Obsolete, throw fmt::exception directly. Use 'HERE' macro, if necessary.
#define EXCEPTION(format_str, ...) fmt::exception("%s(): " format_str HERE, __FUNCTION__, ##__VA_ARGS__)

#define _PRGNAME_ "RPCS3"
#define _PRGVER_ "0.0.0.9"

#include "Utilities/types.h"
#include "Utilities/Macro.h"
#include "Utilities/Platform.h"
#include "Utilities/BEType.h"
#include "Utilities/Atomic.h"
#include "Utilities/StrFmt.h"
#include "Utilities/File.h"
#include "Utilities/Log.h"
