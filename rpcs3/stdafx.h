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

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <climits>
#include <cmath>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <vector>
#include <queue>
#include <set>
#include <array>
#include <string>
#include <functional>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <list>
#include <forward_list>
#include <typeindex>
#include <future>
#include <chrono>

using namespace std::string_literals;
using namespace std::chrono_literals;

#include <common/GNU.h>
#include <common/BEType.h>
#include <common/Atomic.h>
#include <common/StrFmt.h>
#include <common/BitField.h>
#include <common/Log.h>
#include <common/File.h>

#define _PRGNAME_ "RPCS3"
#define _PRGVER_ "0.0.0.6"