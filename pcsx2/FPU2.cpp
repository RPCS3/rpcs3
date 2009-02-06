/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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
#include <math.h>

// sqrtf only defined in C++
extern "C" {

float (*fpusqrtf)(float fval) = 0;
float (*fpufabsf)(float fval) = 0;
float (*fpusinf)(float fval) = 0;
float (*fpucosf)(float fval) = 0;
float (*fpuexpf)(float fval) = 0;
float (*fpuatanf)(float fval) = 0;
float (*fpuatan2f)(float fvalx, float fvaly) = 0;

void InitFPUOps()
{
	fpusqrtf = sqrtf;
	fpufabsf = fabsf;
	fpusinf = sinf;
	fpucosf = cosf;
	fpuexpf = expf;
	fpuatanf = atanf;
	fpuatan2f = atan2f;
}

}
