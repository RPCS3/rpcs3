/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

// macro provided for tagging translation strings, without actually running them through the
// translator (which the _() does automatically, and sometimes we don't want that).  This is
// a shorthand replacement for wxTRANSLATE.
#define wxLt(a)		(a)

#ifndef wxASSERT_MSG_A
#	define wxASSERT_MSG_A( cond, msg ) wxASSERT_MSG( cond, wxString::FromAscii( msg ).c_str() )
#endif

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
