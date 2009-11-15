
// Can't use #pragma once in the precompiled header, as it won't work correctly with GCC PCH.

#ifndef UTILITIES_PRECOMPILED_HEADER
#define UTILITIES_PRECOMPILED_HEADER

#include "Dependencies.h"

using std::string;
using std::min;
using std::max;

#include "Assertions.h"
#include "MemcpyFast.h"
#include "Console.h"
#include "Exceptions.h"
#include "SafeArray.h"
#include "General.h"

#endif

