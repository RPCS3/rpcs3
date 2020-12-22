#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#define NOMINMAX

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "util/endian.hpp"
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
