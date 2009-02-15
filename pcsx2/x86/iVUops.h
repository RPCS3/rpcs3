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
