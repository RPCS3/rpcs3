#ifndef EMITTER_PRECOMPILED_HEADER
#define EMITTER_PRECOMPILED_HEADER

// must include wx/setup.h first, otherwise we get warnings/errors regarding __LINUX__
#include <wx/setup.h>

#include "Pcsx2Defs.h"

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/gdicmn.h>		// for wxPoint/wxRect stuff
#include <wx/intl.h>
#include <wx/log.h>

#include <stdexcept>
#include <algorithm>
#include <string>
#include <cstring>		// string.h under c++
#include <cstdio>		// stdio.h under c++
#include <cstdlib>

using std::string;
using std::min;
using std::max;

#include "Utilities/Console.h"
#include "Utilities/Exceptions.h"
#include "Utilities/General.h"
#include "Utilities/MemcpyFast.h"

#endif

