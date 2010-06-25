
// Can't use #pragma once in the precompiled header, as it won't work correctly with GCC PCH.

#ifndef EMITTER_PRECOMPILED_HEADER
#define EMITTER_PRECOMPILED_HEADER

#include <wx/string.h>

#include "Pcsx2Defs.h"

#include <stdexcept>
#include <cstring>		// string.h under c++
#include <cstdio>		// stdio.h under c++
#include <cstdlib>

#include "Utilities/Console.h"
#include "Utilities/Exceptions.h"
#include "Utilities/General.h"
#include "Utilities/MemcpyFast.h"

#endif

