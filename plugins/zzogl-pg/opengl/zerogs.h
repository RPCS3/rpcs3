/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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

#ifndef __ZEROGS__H
#define __ZEROGS__H

#ifdef _MSC_VER
#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union
#endif

// ----------------------------- Includes
//#include <list>
//#include <vector>
//#include <map>
//#include <string>
//#include <math.h>

//#include "ZZGl.h"
//#include "CRC.h"
//#include "targets.h"
#include "PS2Edefs.h"
// ------------------------ Variables -------------------------

//////////////////////////
// State parameters

#ifdef ZEROGS_DEVBUILD
extern char* EFFECT_NAME;
extern char* EFFECT_DIR;
extern u32 g_nGenVars, g_nTexVars, g_nAlphaVars, g_nResolve;
extern bool g_bSaveTrans, g_bUpdateEffect, g_bSaveTex, g_bSaveResolved;
#endif

extern bool s_bWriteDepth;

extern int nBackbufferWidth, nBackbufferHeight;

extern float fiTexWidth[2], fiTexHeight[2];	// current tex width and height

// Methods //

void ZZGSStateReset();


// flush current vertices, call before setting new registers (the main render method)
void Flush(int context);
void FlushBoth();

// called on a primitive switch
void Prim();

#endif
