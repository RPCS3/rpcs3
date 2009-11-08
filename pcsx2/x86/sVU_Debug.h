/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#define REC_VUOP(VU, f) { \
	_freeXMMregs(/*&VU*/); \
	_freeMMXregs(); \
	SetFPUstate();) \
	MOV32ItoM((uptr)&VU.code, (u32)VU.code); \
	CALLFunc((uptr)VU##MI_##f); \
}

#define REC_VUOPs(VU, f) { \
	_freeXMMregs(); \
	_freeMMXregs(); \
	SetFPUstate();) \
	if (VU==&VU1) {  \
		MOV32ItoM((uptr)&VU1.code, (u32)VU1.code); \
		CALLFunc((uptr)VU1MI_##f); \
	}  \
	else {  \
		MOV32ItoM((uptr)&VU0.code, (u32)VU0.code); \
		CALLFunc((uptr)VU0MI_##f); \
	}  \
}

#define REC_VUOPFLAGS(VU, f) { \
	_freeXMMregs(/*&VU*/); \
	_freeMMXregs(); \
	SetFPUstate(); \
	MOV32ItoM((uptr)&VU.code, (u32)VU.code); \
	CALLFunc((uptr)VU##MI_##f); \
}

#define REC_VUBRANCH(VU, f) { \
	_freeXMMregs(/*&VU*/); \
	_freeMMXregs(); \
	SetFPUstate(); \
	MOV32ItoM((uptr)&VU.code, (u32)VU.code); \
	MOV32ItoM((uptr)&VU.VI[REG_TPC].UL, (u32)pc); \
	CALLFunc((uptr)VU##MI_##f); \
	branch = 1; \
}
