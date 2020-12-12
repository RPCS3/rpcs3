#pragma once // No BOM and only basic ASCII in this header, or a neko will die

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

#include "util/types.hpp"
#include "Utilities/BEType.h"
#include "util/atomic.hpp"
#include "Utilities/StrFmt.h"
#include "Utilities/File.h"
#include "util/logs.hpp"
#include "util/shared_ptr.hpp"

#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <string_view>
