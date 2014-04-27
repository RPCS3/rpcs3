/*  PCSX2 - PS2 Emulator for PCs
*  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include "PrecompiledHeader.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <math.h>
#include "Gte.h"
//#include "R3000A.h"
#include "IopCommon.h"
#ifdef GTE_DUMP
#define G_OP(name,delay) fprintf(gteLog, "* : %08X : %02d : %s\n", psxRegs.code, delay, name);
#define G_SD(reg)  fprintf(gteLog, "+D%02d : %08X\n", reg, psxRegs.CP2D.r[reg]);
#define G_SC(reg)  fprintf(gteLog, "+C%02d : %08X\n", reg, psxRegs.CP2C.r[reg]);
#define G_GD(reg)  fprintf(gteLog, "-D%02d : %08X\n", reg, psxRegs.CP2D.r[reg]);
#define G_GC(reg)  fprintf(gteLog, "-C%02d : %08X\n", reg, psxRegs.CP2C.r[reg]);
#else
#define G_OP(name,delay)
#define G_SD(reg)
#define G_SC(reg)
#define G_GD(reg)
#define G_GC(reg)
#endif

#define SUM_FLAG if(gteFLAG & 0x7F87E000) gteFLAG |= 0x80000000;

#ifdef _MSC_VER_
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

#define gteVX0     ((s16*)psxRegs.CP2D.r)[0]
#define gteVY0     ((s16*)psxRegs.CP2D.r)[1]
#define gteVZ0     ((s16*)psxRegs.CP2D.r)[2]
#define gteVX1     ((s16*)psxRegs.CP2D.r)[4]
#define gteVY1     ((s16*)psxRegs.CP2D.r)[5]
#define gteVZ1     ((s16*)psxRegs.CP2D.r)[6]
#define gteVX2     ((s16*)psxRegs.CP2D.r)[8]
#define gteVY2     ((s16*)psxRegs.CP2D.r)[9]
#define gteVZ2     ((s16*)psxRegs.CP2D.r)[10]
#define gteRGB     psxRegs.CP2D.r[6]
#define gteOTZ     ((s16*)psxRegs.CP2D.r)[7*2]
#define gteIR0     ((s32*)psxRegs.CP2D.r)[8]
#define gteIR1     ((s32*)psxRegs.CP2D.r)[9]
#define gteIR2     ((s32*)psxRegs.CP2D.r)[10]
#define gteIR3     ((s32*)psxRegs.CP2D.r)[11]
#define gteSXY0    ((s32*)psxRegs.CP2D.r)[12]
#define gteSXY1    ((s32*)psxRegs.CP2D.r)[13]
#define gteSXY2    ((s32*)psxRegs.CP2D.r)[14]
#define gteSXYP    ((s32*)psxRegs.CP2D.r)[15]
#define gteSX0     ((s16*)psxRegs.CP2D.r)[12*2]
#define gteSY0     ((s16*)psxRegs.CP2D.r)[12*2+1]
#define gteSX1     ((s16*)psxRegs.CP2D.r)[13*2]
#define gteSY1     ((s16*)psxRegs.CP2D.r)[13*2+1]
#define gteSX2     ((s16*)psxRegs.CP2D.r)[14*2]
#define gteSY2     ((s16*)psxRegs.CP2D.r)[14*2+1]
#define gteSXP     ((s16*)psxRegs.CP2D.r)[15*2]
#define gteSYP     ((s16*)psxRegs.CP2D.r)[15*2+1]
#define gteSZx     ((u16*)psxRegs.CP2D.r)[16*2]
#define gteSZ0     ((u16*)psxRegs.CP2D.r)[17*2]
#define gteSZ1     ((u16*)psxRegs.CP2D.r)[18*2]
#define gteSZ2     ((u16*)psxRegs.CP2D.r)[19*2]
#define gteRGB0    psxRegs.CP2D.r[20]
#define gteRGB1    psxRegs.CP2D.r[21]
#define gteRGB2    psxRegs.CP2D.r[22]
#define gteMAC0    psxRegs.CP2D.r[24]
#define gteMAC1    ((s32*)psxRegs.CP2D.r)[25]
#define gteMAC2    ((s32*)psxRegs.CP2D.r)[26]
#define gteMAC3    ((s32*)psxRegs.CP2D.r)[27]
#define gteIRGB    psxRegs.CP2D.r[28]
#define gteORGB    psxRegs.CP2D.r[29]
#define gteLZCS    psxRegs.CP2D.r[30]
#define gteLZCR    psxRegs.CP2D.r[31]

#define gteR       ((u8 *)psxRegs.CP2D.r)[6*4]
#define gteG       ((u8 *)psxRegs.CP2D.r)[6*4+1]
#define gteB       ((u8 *)psxRegs.CP2D.r)[6*4+2]
#define gteCODE    ((u8 *)psxRegs.CP2D.r)[6*4+3]
#define gteC       gteCODE

#define gteR0      ((u8 *)psxRegs.CP2D.r)[20*4]
#define gteG0      ((u8 *)psxRegs.CP2D.r)[20*4+1]
#define gteB0      ((u8 *)psxRegs.CP2D.r)[20*4+2]
#define gteCODE0   ((u8 *)psxRegs.CP2D.r)[20*4+3]
#define gteC0      gteCODE0

#define gteR1      ((u8 *)psxRegs.CP2D.r)[21*4]
#define gteG1      ((u8 *)psxRegs.CP2D.r)[21*4+1]
#define gteB1      ((u8 *)psxRegs.CP2D.r)[21*4+2]
#define gteCODE1   ((u8 *)psxRegs.CP2D.r)[21*4+3]
#define gteC1      gteCODE1

#define gteR2      ((u8 *)psxRegs.CP2D.r)[22*4]
#define gteG2      ((u8 *)psxRegs.CP2D.r)[22*4+1]
#define gteB2      ((u8 *)psxRegs.CP2D.r)[22*4+2]
#define gteCODE2   ((u8 *)psxRegs.CP2D.r)[22*4+3]
#define gteC2      gteCODE2



#define gteR11  ((s16*)psxRegs.CP2C.r)[0]
#define gteR12  ((s16*)psxRegs.CP2C.r)[1]
#define gteR13  ((s16*)psxRegs.CP2C.r)[2]
#define gteR21  ((s16*)psxRegs.CP2C.r)[3]
#define gteR22  ((s16*)psxRegs.CP2C.r)[4]
#define gteR23  ((s16*)psxRegs.CP2C.r)[5]
#define gteR31  ((s16*)psxRegs.CP2C.r)[6]
#define gteR32  ((s16*)psxRegs.CP2C.r)[7]
#define gteR33  ((s16*)psxRegs.CP2C.r)[8]
#define gteTRX  ((s32*)psxRegs.CP2C.r)[5]
#define gteTRY  ((s32*)psxRegs.CP2C.r)[6]
#define gteTRZ  ((s32*)psxRegs.CP2C.r)[7]
#define gteL11  ((s16*)psxRegs.CP2C.r)[16]
#define gteL12  ((s16*)psxRegs.CP2C.r)[17]
#define gteL13  ((s16*)psxRegs.CP2C.r)[18]
#define gteL21  ((s16*)psxRegs.CP2C.r)[19]
#define gteL22  ((s16*)psxRegs.CP2C.r)[20]
#define gteL23  ((s16*)psxRegs.CP2C.r)[21]
#define gteL31  ((s16*)psxRegs.CP2C.r)[22]
#define gteL32  ((s16*)psxRegs.CP2C.r)[23]
#define gteL33  ((s16*)psxRegs.CP2C.r)[24]
#define gteRBK  ((s32*)psxRegs.CP2C.r)[13]
#define gteGBK  ((s32*)psxRegs.CP2C.r)[14]
#define gteBBK  ((s32*)psxRegs.CP2C.r)[15]
#define gteLR1  ((s16*)psxRegs.CP2C.r)[32]
#define gteLR2  ((s16*)psxRegs.CP2C.r)[33]
#define gteLR3  ((s16*)psxRegs.CP2C.r)[34]
#define gteLG1  ((s16*)psxRegs.CP2C.r)[35]
#define gteLG2  ((s16*)psxRegs.CP2C.r)[36]
#define gteLG3  ((s16*)psxRegs.CP2C.r)[37]
#define gteLB1  ((s16*)psxRegs.CP2C.r)[38]
#define gteLB2  ((s16*)psxRegs.CP2C.r)[39]
#define gteLB3  ((s16*)psxRegs.CP2C.r)[40]
#define gteRFC  ((s32*)psxRegs.CP2C.r)[21]
#define gteGFC  ((s32*)psxRegs.CP2C.r)[22]
#define gteBFC  ((s32*)psxRegs.CP2C.r)[23]
#define gteOFX  ((s32*)psxRegs.CP2C.r)[24]
#define gteOFY  ((s32*)psxRegs.CP2C.r)[25]
#define gteH    ((u16*)psxRegs.CP2C.r)[52]
#define gteDQA  ((s16*)psxRegs.CP2C.r)[54]
#define gteDQB  ((s32*)psxRegs.CP2C.r)[28]
#define gteZSF3 ((s16*)psxRegs.CP2C.r)[58]
#define gteZSF4 ((s16*)psxRegs.CP2C.r)[60]
#define gteFLAG psxRegs.CP2C.r[31]

__inline unsigned long MFC2(int reg) {
	switch (reg) {
	case 29:
		gteORGB = (((gteIR1 >> 7) & 0x1f)) |
			(((gteIR2 >> 7) & 0x1f) << 5) |
			(((gteIR3 >> 7) & 0x1f) << 10);
		//			gteORGB = (gteIR1      ) | 
		//					  (gteIR2 <<  5) | 
		//					  (gteIR3 << 10);
		//			gteORGB = ((gteIR1 & 0xf80)>>7) | 
		//					  ((gteIR2 & 0xf80)>>2) | 
		//					  ((gteIR3 & 0xf80)<<3);
		return gteORGB;

	default:
		return psxRegs.CP2D.r[reg];
	}
}

__inline void MTC2(unsigned long value, int reg) {
	int a;

	switch (reg) {
	case 8: case 9: case 10: case 11:
		psxRegs.CP2D.r[reg] = (short)value;
		break;

	case 15:
		gteSXY0 = gteSXY1;
		gteSXY1 = gteSXY2;
		gteSXY2 = value;
		gteSXYP = value;
		break;

	case 16: case 17: case 18: case 19:
		psxRegs.CP2D.r[reg] = (value & 0xffff);
		break;

	case 28:
		psxRegs.CP2D.r[28] = value;
		gteIR1 = ((value)& 0x1f) << 7;
		gteIR2 = ((value >> 5) & 0x1f) << 7;
		gteIR3 = ((value >> 10) & 0x1f) << 7;
		//			gteIR1 = (value      ) & 0x1f;
		//			gteIR2 = (value >>  5) & 0x1f;
		//			gteIR3 = (value >> 10) & 0x1f;
		//			gteIR1 = ((value      ) & 0x1f) << 4;
		//			gteIR2 = ((value >>  5) & 0x1f) << 4;
		//			gteIR3 = ((value >> 10) & 0x1f) << 4;
		break;

	case 30:
		psxRegs.CP2D.r[30] = value;

		a = psxRegs.CP2D.r[30];
#if defined(_MSC_VER_)
		if (a > 0) {
			__asm {
				mov eax, a;
				bsr eax, eax;
				mov a, eax;
			}
			psxRegs.CP2D.r[31] = 31 - a;
		} else if (a < 0) {
			__asm {
				mov eax, a;
				xor eax, 0xffffffff;
				bsr eax, eax;
				mov a, eax;
			}
			psxRegs.CP2D.r[31] = 31 - a;
		} else {
			psxRegs.CP2D.r[31] = 32;
		}
#elif defined(__LINUX__) || defined(__MINGW32__)
		if (a > 0) {
			__asm__ ("bsrl %1, %0\n" : "=r"(a) : "r"(a) );
			psxRegs.CP2D.r[31] = 31 - a;
		} else if (a < 0) {
			a^= 0xffffffff;
			__asm__ ("bsrl %1, %0\n" : "=r"(a) : "r"(a) );
			psxRegs.CP2D.r[31] = 31 - a;
		} else {
			psxRegs.CP2D.r[31] = 32;
		}
#else
		if (a > 0) {
			int i;
			for (i = 31; (a & (1 << i)) == 0 && i >= 0; i--);
			psxRegs.CP2D.r[31] = 31 - i;
		}
		else if (a < 0) {
			int i;
			a ^= 0xffffffff;
			for (i = 31; (a & (1 << i)) == 0 && i >= 0; i--);
			psxRegs.CP2D.r[31] = 31 - i;
		}
		else {
			psxRegs.CP2D.r[31] = 32;
		}
#endif
		break;

	default:
		psxRegs.CP2D.r[reg] = value;
	}
}

void gteMFC2() {
	if (!_Rt_) return;
	psxRegs.GPR.r[_Rt_] = MFC2(_Rd_);
}

void gteCFC2() {
	if (!_Rt_) return;
	psxRegs.GPR.r[_Rt_] = psxRegs.CP2C.r[_Rd_];
}

void gteMTC2() {
	MTC2(psxRegs.GPR.r[_Rt_], _Rd_);
}

void gteCTC2() {
	psxRegs.CP2C.r[_Rd_] = psxRegs.GPR.r[_Rt_];
}

#define _oB_ (psxRegs.GPR.r[_Rs_] + _Imm_)

void gteLWC2() {
	MTC2(iopMemRead32(_oB_), _Rt_);
}

void gteSWC2() {
	iopMemWrite32(_oB_, MFC2(_Rt_));
}

/////LIMITATIONS AND OTHER STUFF************************************


/*
#define MAGIC  (((65536. * 65536. * 16) + (65536.*.5)) * 65536.)

static __inline long float2int(double d)
{
double dtemp = MAGIC + d;
return (*(long *)&dtemp)-0x80000000;
}*/
/*
__inline double EDETEC1(double data)
{
if (data<(double)-2147483647) {gteFLAG|=1<<30; return (double)-2147483647;}
else
if (data>(double) 2147483647) {gteFLAG|=1<<27; return (double) 2147483647;}

else return data;
}

__inline double EDETEC2(double data)
{
if (data<(double)-2147483647) {gteFLAG|=1<<29; return (double)-2147483647;}
else
if (data>(double) 2147483647) {gteFLAG|=1<<26; return (double) 2147483647;}

else return data;
}

__inline double EDETEC3(double data)
{
if (data<(double)-2147483647) {gteFLAG|=1<<28; return (double)-2147483647;}
else
if (data>(double) 2147483647) {gteFLAG|=1<<25; return (double) 2147483647;}

else return data;
}

__inline double EDETEC4(double data)
{
if (data<(double)-2147483647) {gteFLAG|=1<<16; return (double)-2147483647;}
else
if (data>(double) 2147483647) {gteFLAG|=1<<15; return (double) 2147483647;}

else return data;
}*/
/*
double LimitAU(double fraction,unsigned long bitIndex) {
if (fraction <     0.0) { fraction =     0.0; gteFLAG |= (1<<bitIndex); }
else
if (fraction > 32767.0) { fraction = 32767.0; gteFLAG |= (1<<bitIndex); }

return (fraction);
}

double LimitAS(double fraction,unsigned long bitIndex) {
if (fraction <-32768.0) { fraction =-32768.0; gteFLAG |= (1<<bitIndex); }
else
if (fraction > 32767.0) { fraction = 32767.0; gteFLAG |= (1<<bitIndex); }

return (fraction);
}

double LimitB (double fraction,unsigned long bitIndex) {
if (fraction <     0.0) { fraction =     0.0; gteFLAG |= (1<<bitIndex); }
else
if (fraction >   255.0) { fraction =   255.0; gteFLAG |= (1<<bitIndex); }

return (fraction);
}

double LimitC (double fraction,unsigned long bitIndex) {
if (fraction <     0.0) { fraction =     0.0; gteFLAG |= (1<<bitIndex); }
else
if (fraction > 65535.0) { fraction = 65535.0; gteFLAG |= (1<<bitIndex); }

return (fraction);
}

double LimitD (double fraction,unsigned long bitIndex) {
if (fraction < -1024.0) { fraction = -1024.0; gteFLAG |= (1<<bitIndex); }
else
if (fraction >  1023.0) { fraction =  1023.0; gteFLAG |= (1<<bitIndex); }

return (fraction);
}

double LimitE (double fraction,unsigned long bitIndex) {
if (fraction <     0.0) { fraction =     0.0; gteFLAG |= (1<<bitIndex); }
else
if (fraction >  1023.0) { fraction =  1023.0; gteFLAG |= (1<<bitIndex); }

return (fraction);
}

double LIMIT(double data,double MIN,double MAX,int FLAG)
{
if (data<MIN) {gteFLAG|=1<<FLAG; return MIN;}
else
if (data>MAX) {gteFLAG|=1<<FLAG; return MAX;}

else return data;
}

double ALIMIT(double data,double MIN,double MAX)
{
if (data<MIN) return MIN;
else
if (data>MAX) return MAX;

else return data;
}

double OLIMIT(double data)
{
data=(data);

if (data<(double)-2147483647) {return (double)-2147483647;}
else
if (data>(double) 2147483647) {return (double) 2147483647;}

else return data;
}*/

__inline double NC_OVERFLOW1(double x) {
	if (x<-2147483648.0) { gteFLAG |= 1 << 29; }
	else if (x> 2147483647.0) { gteFLAG |= 1 << 26; }

	return x;
}

__inline double NC_OVERFLOW2(double x) {
	if (x<-2147483648.0) { gteFLAG |= 1 << 28; }
	else if (x> 2147483647.0) { gteFLAG |= 1 << 25; }

	return x;
}

__inline double NC_OVERFLOW3(double x) {
	if (x<-2147483648.0) { gteFLAG |= 1 << 27; }
	else if (x> 2147483647.0) { gteFLAG |= 1 << 24; }

	return x;
}

__inline double NC_OVERFLOW4(double x) {
	if (x<-2147483648.0) { gteFLAG |= 1 << 16; }
	else if (x> 2147483647.0) { gteFLAG |= 1 << 15; }

	return x;
}

__inline s32 FNC_OVERFLOW1(s64 x) {
	if (x< (s64)0xffffffff80000000) { gteFLAG |= 1 << 29; }
	else if (x> 2147483647) { gteFLAG |= 1 << 26; }

	return (s32)x;
}

__inline s32 FNC_OVERFLOW2(s64 x) {
	if (x< (s64)0xffffffff80000000) { gteFLAG |= 1 << 28; }
	else if (x> 2147483647) { gteFLAG |= 1 << 25; }

	return (s32)x;
}

__inline s32 FNC_OVERFLOW3(s64 x) {
	if (x< (s64)0xffffffff80000000) { gteFLAG |= 1 << 27; }
	else if (x> 2147483647) { gteFLAG |= 1 << 24; }

	return (s32)x;
}

__inline s32 FNC_OVERFLOW4(s64 x) {
	if (x< (s64)0xffffffff80000000) { gteFLAG |= 1 << 16; }
	else if (x> 2147483647) { gteFLAG |= 1 << 15; }

	return (s32)x;
}

#define _LIMX(negv, posv, flagb) { \
	if (x < (negv)) { x = (negv); gteFLAG |= (1<<flagb); } else \
	if (x > (posv)) { x = (posv); gteFLAG |= (1<<flagb); } return (x); \
}

__inline double limA1S(double x) { _LIMX(-32768.0, 32767.0, 24); }
__inline double limA2S(double x) { _LIMX(-32768.0, 32767.0, 23); }
__inline double limA3S(double x) { _LIMX(-32768.0, 32767.0, 22); }
__inline double limA1U(double x) { _LIMX(0.0, 32767.0, 24); }
__inline double limA2U(double x) { _LIMX(0.0, 32767.0, 23); }
__inline double limA3U(double x) { _LIMX(0.0, 32767.0, 22); }
__inline double limB1(double x) { _LIMX(0.0, 255.0, 21); }
__inline double limB2(double x) { _LIMX(0.0, 255.0, 20); }
__inline double limB3(double x) { _LIMX(0.0, 255.0, 19); }
__inline double limC(double x) { _LIMX(0.0, 65535.0, 18); }
__inline double limD1(double x) { _LIMX(-1024.0, 1023.0, 14); }
__inline double limD2(double x) { _LIMX(-1024.0, 1023.0, 13); }
__inline double limE(double x) { _LIMX(0.0, 4095.0, 12); }

__inline double limG1(double x) {
	if (x > 2147483647.0) { gteFLAG |= (1 << 16); }
	else
		if (x <-2147483648.0) { gteFLAG |= (1 << 15); }

	if (x >       1023.0) { x = 1023.0; gteFLAG |= (1 << 14); }
	else
		if (x <      -1024.0) { x = -1024.0; gteFLAG |= (1 << 14); } return (x);
}

__inline double limG2(double x) {
	if (x > 2147483647.0) { gteFLAG |= (1 << 16); }
	else
		if (x <-2147483648.0) { gteFLAG |= (1 << 15); }

	if (x >       1023.0) { x = 1023.0; gteFLAG |= (1 << 13); }
	else
		if (x < -1024.0) { x = -1024.0; gteFLAG |= (1 << 13); } return (x);
}

__inline s32 F12limA1S(s64 x) { _LIMX(-32768 << 12, 32767 << 12, 24); }
__inline s32 F12limA2S(s64 x) { _LIMX(-32768 << 12, 32767 << 12, 23); }
__inline s32 F12limA3S(s64 x) { _LIMX(-32768 << 12, 32767 << 12, 22); }
__inline s32 F12limA1U(s64 x) { _LIMX(0, 32767 << 12, 24); }
__inline s32 F12limA2U(s64 x) { _LIMX(0, 32767 << 12, 23); }
__inline s32 F12limA3U(s64 x) { _LIMX(0, 32767 << 12, 22); }

__inline s16 FlimA1S(s32 x) { _LIMX(-32768, 32767, 24); }
__inline s16 FlimA2S(s32 x) { _LIMX(-32768, 32767, 23); }
__inline s16 FlimA3S(s32 x) { _LIMX(-32768, 32767, 22); }
__inline s16 FlimA1U(s32 x) { _LIMX(0, 32767, 24); }
__inline s16 FlimA2U(s32 x) { _LIMX(0, 32767, 23); }
__inline s16 FlimA3U(s32 x) { _LIMX(0, 32767, 22); }
__inline u8  FlimB1(s32 x) { _LIMX(0, 255, 21); }
__inline u8  FlimB2(s32 x) { _LIMX(0, 255, 20); }
__inline u8  FlimB3(s32 x) { _LIMX(0, 255, 19); }
__inline u16 FlimC(s32 x) { _LIMX(0, 65535, 18); }
__inline s32 FlimD1(s32 x) { _LIMX(-1024, 1023, 14); }
__inline s32 FlimD2(s32 x) { _LIMX(-1024, 1023, 13); }
__inline s32 FlimE(s32 x) { _LIMX(0, 65535, 12); }
//__inline s32 FlimE  (s32 x) { _LIMX(0, 4095, 12); }

__inline s32 FlimG1(s64 x) {
	if (x > 2147483647) { gteFLAG |= (1 << 16); }
	else
		if (x < (s64)0xffffffff80000000) { gteFLAG |= (1 << 15); }

	if (x >       1023) { x = 1023; gteFLAG |= (1 << 14); }
	else
		if (x <      -1024) { x = -1024; gteFLAG |= (1 << 14); } return (x);
}

__inline s32 FlimG2(s64 x) {
	if (x > 2147483647) { gteFLAG |= (1 << 16); }
	else
		if (x < (s64)0xffffffff80000000) { gteFLAG |= (1 << 15); }

	if (x >       1023) { x = 1023; gteFLAG |= (1 << 13); }
	else
		if (x < -1024) { x = -1024; gteFLAG |= (1 << 13); } return (x);
}

#define MAC2IR() { \
	if (gteMAC1 < (long)(-32768)) { gteIR1=(long)(-32768); gteFLAG|=1<<24;} \
else \
	if (gteMAC1 > (long)( 32767)) { gteIR1=(long)( 32767); gteFLAG|=1<<24;} \
else gteIR1=(long)gteMAC1; \
	if (gteMAC2 < (long)(-32768)) { gteIR2=(long)(-32768); gteFLAG|=1<<23;} \
else \
	if (gteMAC2 > (long)( 32767)) { gteIR2=(long)( 32767); gteFLAG|=1<<23;} \
else gteIR2=(long)gteMAC2; \
	if (gteMAC3 < (long)(-32768)) { gteIR3=(long)(-32768); gteFLAG|=1<<22;} \
else \
	if (gteMAC3 > (long)( 32767)) { gteIR3=(long)( 32767); gteFLAG|=1<<22;} \
else gteIR3=(long)gteMAC3; \
}


#define MAC2IR1() {           \
	if (gteMAC1 < (long)0) { gteIR1=(long)0; gteFLAG|=1<<24;}  \
else if (gteMAC1 > (long)(32767)) { gteIR1=(long)(32767); gteFLAG|=1<<24;} \
else gteIR1=(long)gteMAC1;                                                         \
	if (gteMAC2 < (long)0) { gteIR2=(long)0; gteFLAG|=1<<23;}      \
else if (gteMAC2 > (long)(32767)) { gteIR2=(long)(32767); gteFLAG|=1<<23;}    \
else gteIR2=(long)gteMAC2;                                                            \
	if (gteMAC3 < (long)0) { gteIR3=(long)0; gteFLAG|=1<<22;}         \
else if (gteMAC3 > (long)(32767)) { gteIR3=(long)(32767); gteFLAG|=1<<22;}       \
else gteIR3=(long)gteMAC3; \
}

//********END OF LIMITATIONS**********************************/

#define GTE_RTPS1(vn) { \
	gteMAC1 = FNC_OVERFLOW1(((signed long)(gteR11*gteVX##vn + gteR12*gteVY##vn + gteR13*gteVZ##vn)>>12) + gteTRX); \
	gteMAC2 = FNC_OVERFLOW2(((signed long)(gteR21*gteVX##vn + gteR22*gteVY##vn + gteR23*gteVZ##vn)>>12) + gteTRY); \
	gteMAC3 = FNC_OVERFLOW3(((signed long)(gteR31*gteVX##vn + gteR32*gteVY##vn + gteR33*gteVZ##vn)>>12) + gteTRZ); \
}

/*	gteMAC1 = NC_OVERFLOW1(((signed long)(gteR11*gteVX0 + gteR12*gteVY0 + gteR13*gteVZ0)>>12) + gteTRX);
	gteMAC2 = NC_OVERFLOW2(((signed long)(gteR21*gteVX0 + gteR22*gteVY0 + gteR23*gteVZ0)>>12) + gteTRY);
	gteMAC3 = NC_OVERFLOW3(((signed long)(gteR31*gteVX0 + gteR32*gteVY0 + gteR33*gteVZ0)>>12) + gteTRZ);*/

#if 0

#define GTE_RTPS2(vn) { \
	if (gteSZ##vn == 0) { \
	DSZ = 2.0f; gteFLAG |= 1<<17; \
	} else { \
	DSZ = (double)gteH / gteSZ##vn; \
	if (DSZ > 2.0) { DSZ = 2.0f; gteFLAG |= 1<<17; } \
	/*		if (DSZ > 2147483647.0) { DSZ = 2.0f; gteFLAG |= 1<<17; }*/ \
	} \
	\
	/*	gteSX##vn = limG1(gteOFX/65536.0 + (limA1S(gteMAC1) * DSZ));*/ \
	/*	gteSY##vn = limG2(gteOFY/65536.0 + (limA2S(gteMAC2) * DSZ));*/ \
	gteSX##vn = FlimG1(gteOFX/65536.0 + (gteIR1 * DSZ)); \
	gteSY##vn = FlimG2(gteOFY/65536.0 + (gteIR2 * DSZ)); \
}

#define GTE_RTPS3() { \
	DSZ = gteDQB/16777216.0 + (gteDQA/256.0) * DSZ; \
	gteMAC0 =      DSZ * 16777216.0; \
	gteIR0  = limE(DSZ * 4096.0f); \
	printf("zero %x, %x\n", gteMAC0, gteIR0); \
}
#endif
//#if 0
#define GTE_RTPS2(vn) { \
	if (gteSZ##vn == 0) { \
	FDSZ = 2 << 16; gteFLAG |= 1<<17; \
	} else { \
	FDSZ = ((u64)gteH << 32) / ((u64)gteSZ##vn << 16); \
	if ((u64)FDSZ > (2 << 16)) { FDSZ = 2 << 16; gteFLAG |= 1<<17; } \
	} \
	\
	gteSX##vn = FlimG1((gteOFX + (((s64)((s64)gteIR1 << 16) * FDSZ) >> 16)) >> 16); \
	gteSY##vn = FlimG2((gteOFY + (((s64)((s64)gteIR2 << 16) * FDSZ) >> 16)) >> 16); \
}

#define GTE_RTPS3() { \
	FDSZ = (s64)((s64)gteDQB + (((s64)((s64)gteDQA << 8) * FDSZ) >> 8)); \
	gteMAC0 = FDSZ; \
	gteIR0  = FlimE(FDSZ >> 12); \
}
//#endif
//	gteMAC0 =      (gteDQB/16777216.0 + (gteDQA/256.0) * DSZ) * 16777216.0;
//	gteIR0  = limE((gteDQB/16777216.0 + (gteDQA/256.0) * DSZ) * 4096.0);
//	gteMAC0 =       ((gteDQB >> 24) + (gteDQA >> 8) * DSZ) * 16777216.0;
//	gteIR0  = FlimE(((gteDQB >> 24) + (gteDQA >> 8) * DSZ) * 4096.0);


void gteRTPS() {
	//	double SSX0,SSY0,SSZ0;
	//	double SZ;
	//	double DSZ;
	s64 FDSZ;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_RTPS\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("RTPS", 14);
		G_SD(0);
		G_SD(1);

		G_SD(16); // Store original fifo
		G_SD(17);
		G_SD(18);
		G_SD(19);

		G_SC(0);
		G_SC(1);
		G_SC(2);
		G_SC(3);
		G_SC(4);
		G_SC(5);
		G_SC(6);
		G_SC(7);

		G_SC(24);
		G_SC(25);
		G_SC(26);
		G_SC(27);
		G_SC(28);
	}
#endif
	/*	gteFLAG = 0;

		SSX0 = NC_OVERFLOW1((double)gteTRX + ((double)(gteVX0*gteR11) + (double)(gteVY0*gteR12) + (double)(gteVZ0*gteR13))/4096.0);
		SSY0 = NC_OVERFLOW2((double)gteTRY + ((double)(gteVX0*gteR21) + (double)(gteVY0*gteR22) + (double)(gteVZ0*gteR23))/4096.0);
		SSZ0 = NC_OVERFLOW3((double)gteTRZ + ((double)(gteVX0*gteR31) + (double)(gteVY0*gteR32) + (double)(gteVZ0*gteR33))/4096.0);

		SZ   = LIMIT(SSZ0,(double)0,(double)65535,18);
		DSZ  = ((double)gteH/SZ);

		if ((DSZ>(double)2147483647)) {DSZ=(double)2; gteFLAG|=1<<17;}

		gteSZ0  = gteSZ1;
		gteSZ1  = gteSZ2;
		gteSZ2  = gteSZx;
		gteSZx  = (unsigned short)float2int(SZ);

		psxRegs.CP2D.r[12]= psxRegs.CP2D.r[13];
		psxRegs.CP2D.r[13]= psxRegs.CP2D.r[14];

		gteSX2  = (signed short)float2int(LIMIT((double)(gteOFX)/65536.0f + (LimitAS(SSX0,24)*DSZ),(double)-1024,(double)1024,14));
		gteSY2  = (signed short)float2int(LIMIT((double)(gteOFY)/65536.0f + (LimitAS(SSY0,23)*DSZ),(double)-1024,(double)1024,13));

		gteMAC1 = (signed long)(SSX0);
		gteMAC2 = (signed long)(SSY0);
		gteMAC3 = (signed long)(SSZ0);

		MAC2IR();

		gteMAC0 = (signed long)float2int(OLIMIT((((double)gteDQB/(double)16777216) + (((double)gteDQA/(double)256)*DSZ))*16777216));
		gteIR0  = (signed long)float2int(LIMIT(((((double)gteDQB/(double)16777216) + (((double)gteDQA/(double)256)*DSZ))*4096),(double)0,(double)4095,12));

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/

	gteFLAG = 0;

	GTE_RTPS1(0);

	MAC2IR();

	gteSZx = gteSZ0;
	gteSZ0 = gteSZ1;
	gteSZ1 = gteSZ2;
	//	gteSZ2 = limC(gteMAC3);
	gteSZ2 = FlimC(gteMAC3);

	gteSXY0 = gteSXY1;
	gteSXY1 = gteSXY2;

	GTE_RTPS2(2);
	gteSXYP = gteSXY2;

	GTE_RTPS3();

	SUM_FLAG;

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_GD(8);
		G_GD(9);
		G_GD(10);
		G_GD(11);

		//G_GD(12);
		//G_GD(13);
		G_GD(14);

		G_GD(16);
		G_GD(17);
		G_GD(18);
		G_GD(19);

		G_GD(24);
		G_GD(25);
		G_GD(26);
		G_GD(27);

		G_GC(31);
	}
#endif
}

void gteRTPT() {
	//	double SSX0,SSY0,SSZ0;
	//	double SZ;
	//	double DSZ;
	s64 FDSZ;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_RTPT\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("RTPT", 22);
		G_SD(0);
		G_SD(1);
		G_SD(2);
		G_SD(3);
		G_SD(4);
		G_SD(5);

		G_SD(16); // Store original fifo
		G_SD(17);
		G_SD(18);
		G_SD(19);

		G_SC(0);
		G_SC(1);
		G_SC(2);
		G_SC(3);
		G_SC(4);
		G_SC(5);
		G_SC(6);
		G_SC(7);

		G_SC(24);
		G_SC(25);
		G_SC(26);
		G_SC(27);
		G_SC(28);
	}
#endif
	/*	gteFLAG = 0;

		gteSZ0  = gteSZx;

		SSX0 = NC_OVERFLOW1((double)gteTRX + ((double)(gteVX0 * gteR11) + (double)(gteVY0 * gteR12) + (double)(gteVZ0 * gteR13)) / 4096.0);
		SSY0 = NC_OVERFLOW2((double)gteTRY + ((double)(gteVX0 * gteR21) + (double)(gteVY0 * gteR22) + (double)(gteVZ0 * gteR23)) / 4096.0);
		SSZ0 = NC_OVERFLOW3((double)gteTRZ + ((double)(gteVX0 * gteR31) + (double)(gteVY0 * gteR32) + (double)(gteVZ0 * gteR33)) / 4096.0);

		SZ   = LIMIT(SSZ0, (double)0, (double)65535, 18);
		DSZ  = ((double)gteH / SZ);

		if ((DSZ>(double)2147483647)) {DSZ=(double)2; gteFLAG|=1<<17;}

		gteSZ1 = (unsigned short)float2int(SZ);
		gteSX0 = (signed short)float2int(LIMIT((double)(gteOFX)/65536.0f + (LimitAS(SSX0,24)*DSZ),(double)-1024,(double)1023,14));
		gteSY0 = (signed short)float2int(LIMIT((double)(gteOFY)/65536.0f + (LimitAS(SSY0,23)*DSZ),(double)-1024,(double)1023,13));

		SSX0 = NC_OVERFLOW1((double)gteTRX + ((double)(gteVX1*gteR11) + (double)(gteVY1*gteR12) + (double)(gteVZ1*gteR13))/4096.0);
		SSY0 = NC_OVERFLOW2((double)gteTRY + ((double)(gteVX1*gteR21) + (double)(gteVY1*gteR22) + (double)(gteVZ1*gteR23))/4096.0);
		SSZ0 = NC_OVERFLOW3((double)gteTRZ + ((double)(gteVX1*gteR31) + (double)(gteVY1*gteR32) + (double)(gteVZ1*gteR33))/4096.0);

		SZ   = LIMIT(SSZ0,(double)0,(double)65535,18);
		DSZ  = ((double)gteH/SZ);

		if ((DSZ>(double)2147483647)) {DSZ=(double)2; gteFLAG|=1<<17;}

		gteSZ2 = (unsigned short)float2int(SZ);
		gteSX1 = (signed short)float2int(LIMIT((double)(gteOFX)/65536.0f + (LimitAS(SSX0,24)*DSZ),(double)-1024,(double)1023,14));
		gteSY1 = (signed short)float2int(LIMIT((double)(gteOFY)/65536.0f + (LimitAS(SSY0,23)*DSZ),(double)-1024,(double)1023,13));

		SSX0 = NC_OVERFLOW1((double)gteTRX + ((double)(gteVX2*gteR11) + (double)(gteVY2*gteR12) + (double)(gteVZ2*gteR13))/4096.0);
		SSY0 = NC_OVERFLOW2((double)gteTRY + ((double)(gteVX2*gteR21) + (double)(gteVY2*gteR22) + (double)(gteVZ2*gteR23))/4096.0);
		SSZ0 = NC_OVERFLOW3((double)gteTRZ + ((double)(gteVX2*gteR31) + (double)(gteVY2*gteR32) + (double)(gteVZ2*gteR33))/4096.0);

		SZ   = LIMIT(SSZ0,(double)0,(double)65535,18);
		DSZ  = ((double)gteH/SZ);

		if ((DSZ>(double)2147483647)) {DSZ=(double)2; gteFLAG|=1<<17;}

		gteSZx = (unsigned short)float2int(SZ);
		gteSX2 = (signed short)float2int(LIMIT((double)(gteOFX)/65536.0f + (LimitAS(SSX0,24)*DSZ),(double)-1024,(double)1023,14));
		gteSY2 = (signed short)float2int(LIMIT((double)(gteOFY)/65536.0f + (LimitAS(SSY0,23)*DSZ),(double)-1024,(double)1023,13));

		gteMAC1 = (signed long)float2int(SSX0);
		gteMAC2 = (signed long)float2int(SSY0);
		gteMAC3 = (signed long)float2int(SSZ0);

		MAC2IR();

		gteMAC0 = (signed long)float2int(OLIMIT((((double)gteDQB/(double)16777216) + (((double)gteDQA/(double)256)*DSZ))*16777216));
		gteIR0  = (signed long)float2int(LIMIT(((((double)gteDQB/(double)16777216) + (((double)gteDQA/(double)256)*DSZ))*4096),(double)0,(double)4095,12));

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/

	/* NC: old
	gteFLAG = 0;

	gteSZ0 = gteSZx;

	gteMAC1 = NC_OVERFLOW1(((signed long)(gteR11*gteVX0 + gteR12*gteVY0 + gteR13*gteVZ0)>>12) + gteTRX);
	gteMAC2 = NC_OVERFLOW2(((signed long)(gteR21*gteVX0 + gteR22*gteVY0 + gteR23*gteVZ0)>>12) + gteTRY);
	gteMAC3 = NC_OVERFLOW3(((signed long)(gteR31*gteVX0 + gteR32*gteVY0 + gteR33*gteVZ0)>>12) + gteTRZ);

	DSZ = gteH / limC(gteMAC3);
	if (DSZ > 2147483647.0) { DSZ = 2.0f; gteFLAG |= 1<<17; }

	gteSZ1 = limC(gteMAC3);

	gteSX0 = limG1(gteOFX/65536.0 + (limA1S(gteMAC1) * DSZ));
	gteSY0 = limG2(gteOFY/65536.0 + (limA2S(gteMAC2) * DSZ));

	gteMAC1 = NC_OVERFLOW1(((signed long)(gteR11*gteVX1 + gteR12*gteVY1 + gteR13*gteVZ1)>>12) + gteTRX);
	gteMAC2 = NC_OVERFLOW2(((signed long)(gteR21*gteVX1 + gteR22*gteVY1 + gteR23*gteVZ1)>>12) + gteTRY);
	gteMAC3 = NC_OVERFLOW3(((signed long)(gteR31*gteVX1 + gteR32*gteVY1 + gteR33*gteVZ1)>>12) + gteTRZ);

	DSZ = gteH / limC(gteMAC3);
	if (DSZ > 2147483647.0) { DSZ = 2.0f; gteFLAG |= 1<<17; }

	gteSZ2 = limC(gteMAC3);

	gteSX1 = limG1(gteOFX/65536.0 + (limA1S(gteMAC1) * DSZ ));
	gteSY1 = limG2(gteOFY/65536.0 + (limA2S(gteMAC2) * DSZ ));

	gteMAC1 = NC_OVERFLOW1(((signed long)(gteR11*gteVX2 + gteR12*gteVY2 + gteR13*gteVZ2)>>12) + gteTRX);
	gteMAC2 = NC_OVERFLOW2(((signed long)(gteR21*gteVX2 + gteR22*gteVY2 + gteR23*gteVZ2)>>12) + gteTRY);
	gteMAC3 = NC_OVERFLOW3(((signed long)(gteR31*gteVX2 + gteR32*gteVY2 + gteR33*gteVZ2)>>12) + gteTRZ);

	DSZ = gteH / limC(gteMAC3); if (DSZ  > 2147483647.0f) { DSZ  = 2.0f; gteFLAG |= 1<<17; }

	gteSZx = gteSZ2;

	gteSX2 = limG1(gteOFX/65536.0 + (limA1S(gteMAC1) * DSZ ));
	gteSY2 = limG2(gteOFY/65536.0 + (limA2S(gteMAC2) * DSZ ));

	MAC2IR();

	gteMAC0 =      (gteDQB/16777216.0 + (gteDQA/256.0) * DSZ ) * 16777216.0;
	gteIR0  = limE((gteDQB/16777216.0 + (gteDQA/256.0) * DSZ ) * 4096.0f);
	*/

	gteFLAG = 0;

	gteSZx = gteSZ2;

	GTE_RTPS1(0);

	//	gteSZ0 = limC(gteMAC3);
	gteSZ0 = FlimC(gteMAC3);

	gteIR1 = FlimA1S(gteMAC1);
	gteIR2 = FlimA2S(gteMAC2);
	GTE_RTPS2(0);

	GTE_RTPS1(1);

	//	gteSZ1 = limC(gteMAC3);
	gteSZ1 = FlimC(gteMAC3);

	gteIR1 = FlimA1S(gteMAC1);
	gteIR2 = FlimA2S(gteMAC2);
	GTE_RTPS2(1);

	GTE_RTPS1(2);

	MAC2IR();

	//	gteSZ2 = limC(gteMAC3);
	gteSZ2 = FlimC(gteMAC3);

	GTE_RTPS2(2);
	gteSXYP = gteSXY2;

	GTE_RTPS3();

	SUM_FLAG;

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_GD(8);
		G_GD(9);
		G_GD(10);
		G_GD(11);

		G_GD(12);
		G_GD(13);
		G_GD(14);

		G_GD(16);
		G_GD(17);
		G_GD(18);
		G_GD(19);

		G_GD(24);
		G_GD(25);
		G_GD(26);
		G_GD(27);

		G_GC(31);
	}
#endif
}

#define gte_C11 gteLR1
#define gte_C12 gteLR2
#define gte_C13 gteLR3
#define gte_C21 gteLG1
#define gte_C22 gteLG2
#define gte_C23 gteLG3
#define gte_C31 gteLB1
#define gte_C32 gteLB2
#define gte_C33 gteLB3

#define _MVMVA_FUNC(_v0, _v1, _v2, mx) { \
	SSX = (_v0) * mx##11 + (_v1) * mx##12 + (_v2) * mx##13; \
	SSY = (_v0) * mx##21 + (_v1) * mx##22 + (_v2) * mx##23; \
	SSZ = (_v0) * mx##31 + (_v1) * mx##32 + (_v2) * mx##33; \
}

void gteMVMVA() {
	//	double SSX, SSY, SSZ;
	s64 SSX, SSY, SSZ;

#ifdef GTE_LOG
	GTE_LOG("GTE_MVMVA %lx\n", psxRegs.code & 0x1ffffff);
#endif

	switch (psxRegs.code & 0x78000) {
	case 0x00000: // V0 * R
		_MVMVA_FUNC(gteVX0, gteVY0, gteVZ0, gteR); break;
	case 0x08000: // V1 * R
		_MVMVA_FUNC(gteVX1, gteVY1, gteVZ1, gteR); break;
	case 0x10000: // V2 * R
		_MVMVA_FUNC(gteVX2, gteVY2, gteVZ2, gteR); break;
	case 0x18000: // IR * R
		_MVMVA_FUNC((short)gteIR1, (short)gteIR2, (short)gteIR3, gteR);
		break;
	case 0x20000: // V0 * L
		_MVMVA_FUNC(gteVX0, gteVY0, gteVZ0, gteL); break;
	case 0x28000: // V1 * L
		_MVMVA_FUNC(gteVX1, gteVY1, gteVZ1, gteL); break;
	case 0x30000: // V2 * L
		_MVMVA_FUNC(gteVX2, gteVY2, gteVZ2, gteL); break;
	case 0x38000: // IR * L
		_MVMVA_FUNC((short)gteIR1, (short)gteIR2, (short)gteIR3, gteL); break;
	case 0x40000: // V0 * C
		_MVMVA_FUNC(gteVX0, gteVY0, gteVZ0, gte_C); break;
	case 0x48000: // V1 * C
		_MVMVA_FUNC(gteVX1, gteVY1, gteVZ1, gte_C); break;
	case 0x50000: // V2 * C
		_MVMVA_FUNC(gteVX2, gteVY2, gteVZ2, gte_C); break;
	case 0x58000: // IR * C
		_MVMVA_FUNC((short)gteIR1, (short)gteIR2, (short)gteIR3, gte_C); break;
	default:
		SSX = SSY = SSZ = 0;
	}

	if (psxRegs.code & 0x80000) {
		//		SSX /= 4096.0; SSY /= 4096.0; SSZ /= 4096.0;
		SSX >>= 12; SSY >>= 12; SSZ >>= 12;
	}

	switch (psxRegs.code & 0x6000) {
	case 0x0000: // Add TR
		SSX += gteTRX;
		SSY += gteTRY;
		SSZ += gteTRZ;
		break;
	case 0x2000: // Add BK
		SSX += gteRBK;
		SSY += gteGBK;
		SSZ += gteBBK;
		break;
	case 0x4000: // Add FC
		SSX += gteRFC;
		SSY += gteGFC;
		SSZ += gteBFC;
		break;
	}

	gteFLAG = 0;
	//gteMAC1 = (long)SSX;
	//gteMAC2 = (long)SSY;
	//gteMAC3 = (long)SSZ;//okay the follow lines are correct??
	/*	gteMAC1 = NC_OVERFLOW1(SSX);
		gteMAC2 = NC_OVERFLOW2(SSY);
		gteMAC3 = NC_OVERFLOW3(SSZ);*/
	gteMAC1 = FNC_OVERFLOW1(SSX);
	gteMAC2 = FNC_OVERFLOW2(SSY);
	gteMAC3 = FNC_OVERFLOW3(SSZ);
	if (psxRegs.code & 0x400)
		MAC2IR1()
	else MAC2IR()

	SUM_FLAG;
}

void gteNCLIP() {
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_NCLIP\n");
#endif

	//gteLog
#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("NCLIP", 8);
		G_SD(12);
		G_SD(13);
		G_SD(14);
	}
#endif

	/*	gteFLAG = 0;

		gteMAC0 = (signed long)float2int(EDETEC4(
		((double)gteSX0*((double)gteSY1-(double)gteSY2))+
		((double)gteSX1*((double)gteSY2-(double)gteSY0))+
		((double)gteSX2*((double)gteSY0-(double)gteSY1))));

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/
	gteFLAG = 0;


	gteMAC0 = gteSX0 * (gteSY1 - gteSY2) +
		gteSX1 * (gteSY2 - gteSY0) +
		gteSX2 * (gteSY0 - gteSY1);

	//gteMAC0 = (gteSX0 - gteSX1) * (gteSY0 - gteSY2) - (gteSX0 - gteSX2) * (gteSY0 - gteSY1);

	SUM_FLAG;

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_GD(24);
		G_GC(31);
	}
#endif
}

void gteAVSZ3() {
	//	unsigned long SS;
	//	double SZ1,SZ2,SZ3;
	//	double ZSF3;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_AVSZ3\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("AVSZ3", 5);
		G_SD(16);
		G_SD(17);
		G_SD(18);
		G_SD(19);
		G_SC(29);
		G_SC(30);
	}
#endif

	/*	gteFLAG = 0;

		SS = psxRegs.CP2D.r[17] & 0xffff; SZ1  = (double)SS;
		SS = psxRegs.CP2D.r[18] & 0xffff; SZ2  = (double)SS;
		SS = psxRegs.CP2D.r[19] & 0xffff; SZ3  = (double)SS;
		SS = psxRegs.CP2C.r[29] & 0xffff; ZSF3 = (double)SS/(double)4096;

		psxRegs.CP2D.r[24] = (signed long)float2int(EDETEC4(((SZ1+SZ2+SZ3)*ZSF3)));
		psxRegs.CP2D.r[7]  = (unsigned short)float2int(LimitC(((SZ1+SZ2+SZ3)*ZSF3),18));

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/


	gteFLAG = 0;

	/* NC: OLD
	gteMAC0 = ((gteSZ1 + gteSZ2 + gteSZx) * (gteZSF3/4096.0f));

	gteOTZ = limC((double)gteMAC0);
	*/
	/*	gteMAC0 = ((gteSZ1 + gteSZ2 + gteSZx) * (gteZSF3));

		gteOTZ = limC((double)(gteMAC0 >> 12));*/
	gteMAC0 = ((gteSZ0 + gteSZ1 + gteSZ2) * (gteZSF3)) >> 12;

	gteOTZ = FlimC(gteMAC0);
	//	gteOTZ = limC((double)gteMAC0);

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(7);
			G_GD(24);
			G_GC(31);
		}
#endif
}

void gteAVSZ4() {
	//	unsigned long SS;
	//	double SZ0,SZ1,SZ2,SZ3;
	//	double ZSF4;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_AVSZ4\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("AVSZ4", 6);
		G_SD(16);
		G_SD(17);
		G_SD(18);
		G_SD(19);
		G_SC(29);
		G_SC(30);
	}
#endif

	/*	gteFLAG = 0;

		SS = psxRegs.CP2D.r[16] & 0xffff; SZ0  = (double)SS;
		SS = psxRegs.CP2D.r[17] & 0xffff; SZ1  = (double)SS;
		SS = psxRegs.CP2D.r[18] & 0xffff; SZ2  = (double)SS;
		SS = psxRegs.CP2D.r[19] & 0xffff; SZ3  = (double)SS;
		SS = psxRegs.CP2C.r[30] & 0xffff; ZSF4 = (double)SS/(double)4096;

		psxRegs.CP2D.r[24] = (signed long)float2int(EDETEC4(((SZ0+SZ1+SZ2+SZ3)*ZSF4)));
		psxRegs.CP2D.r[7]  = (unsigned short)float2int(LimitC(((SZ0+SZ1+SZ2+SZ3)*ZSF4),18));

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/
	gteFLAG = 0;

	/* NC: OLD
	gteMAC0 = ((gteSZ0 + gteSZ1 + gteSZ2 + gteSZx) * (gteZSF4/4096.0f));

	gteOTZ = limC((double)gteMAC0);
	*/
	/*	gteMAC0 = ((gteSZ0 + gteSZ1 + gteSZ2 + gteSZx) * (gteZSF4));

		gteOTZ = limC((double)(gteMAC0 >> 12));
		*/
	gteMAC0 = ((gteSZx + gteSZ0 + gteSZ1 + gteSZ2) * (gteZSF4)) >> 12;

	gteOTZ = FlimC(gteMAC0);
	//	gteOTZ = limC((double)gteMAC0);

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(7);
			G_GD(24);
			G_GC(31);
		}
#endif
}

void gteSQR() {
	//double SSX0,SSY0,SSZ0;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_SQR %lx\n", psxRegs.code & 0x1ffffff);
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("SQR", 5);
		G_SD(9);
		G_SD(10);
		G_SD(11);
	}
#endif

	/*	gteFLAG = 0;

		SSX0 = (double)gteIR1 * gteIR1;
		SSY0 = (double)gteIR2 * gteIR2;
		SSZ0 = (double)gteIR3 * gteIR3;

		if (psxRegs.code & 0x80000) {
		SSX0 /= 4096.0; SSY0 /= 4096.0; SSZ0 /= 4096.0;
		}

		gteMAC1 = (long)SSX0;
		gteMAC2 = (long)SSY0;
		gteMAC3 = (long)SSZ0;

		MAC2IR1();

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/
	gteFLAG = 0;

	/*	if (psxRegs.code & 0x80000) {
			gteMAC1 = NC_OVERFLOW1((gteIR1 * gteIR1) / 4096.0f);
			gteMAC2 = NC_OVERFLOW2((gteIR2 * gteIR2) / 4096.0f);
			gteMAC3 = NC_OVERFLOW3((gteIR3 * gteIR3) / 4096.0f);
			} else {
			gteMAC1 = NC_OVERFLOW1(gteIR1 * gteIR1);
			gteMAC2 = NC_OVERFLOW2(gteIR2 * gteIR2);
			gteMAC3 = NC_OVERFLOW3(gteIR3 * gteIR3);
			}*/
	if (psxRegs.code & 0x80000) {
		gteMAC1 = FNC_OVERFLOW1((gteIR1 * gteIR1) >> 12);
		gteMAC2 = FNC_OVERFLOW2((gteIR2 * gteIR2) >> 12);
		gteMAC3 = FNC_OVERFLOW3((gteIR3 * gteIR3) >> 12);
	}
	else {
		gteMAC1 = FNC_OVERFLOW1(gteIR1 * gteIR1);
		gteMAC2 = FNC_OVERFLOW2(gteIR2 * gteIR2);
		gteMAC3 = FNC_OVERFLOW3(gteIR3 * gteIR3);
	}
	MAC2IR1();

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);
			G_GD(25);
			G_GD(26);
			G_GD(27);
			G_GC(31);
		}
#endif
}
/*
#define GTE_NCCS(vn) { \
	RR0 = ((double)gteL11 * gteVX##vn + (double)gteL12 * gteVY##vn + (double)gteL13 * gteVZ##vn)/4096.0; \
	GG0 = ((double)gteL21 * gteVX##vn + (double)gteL22 * gteVY##vn + (double)gteL23 * gteVZ##vn)/4096.0; \
	BB0 = ((double)gteL31 * gteVX##vn + (double)gteL32 * gteVY##vn + (double)gteL33 * gteVZ##vn)/4096.0; \
	t1 = LimitAU(RR0,24); \
	t2 = LimitAU(GG0,23); \
	t3 = LimitAU(BB0,22); \
	\
	RR0 = (double)gteRBK + ((double)gteLR1 * t1 + (double)gteLR2 * t2 + (double)gteLR3 * t3)/4096.0; \
	GG0 = (double)gteGBK + ((double)gteLG1 * t1 + (double)gteLG2 * t2 + (double)gteLG3 * t3)/4096.0; \
	BB0 = (double)gteBBK + ((double)gteLB1 * t1 + (double)gteLB2 * t2 + (double)gteLB3 * t3)/4096.0; \
	t1 = LimitAU(RR0,24); \
	t2 = LimitAU(GG0,23); \
	t3 = LimitAU(BB0,22); \
	\
	RR0 = ((double)gteR * t1)/256.0; \
	GG0 = ((double)gteG * t2)/256.0; \
	BB0 = ((double)gteB * t3)/256.0; \
	\
	gteIR1 = (long)LimitAU(RR0,24); \
	gteIR2 = (long)LimitAU(GG0,23); \
	gteIR3 = (long)LimitAU(BB0,22); \
	\
	gteCODE0 = gteCODE1; gteCODE1 = gteCODE2; gteCODE2 = gteCODE; \
	gteR0 = gteR1; gteR1 = gteR2; gteR2 = (unsigned char)LimitB(RR0/16.0,21); \
	gteG0 = gteG1; gteG1 = gteG2; gteG2 = (unsigned char)LimitB(GG0/16.0,20); \
	gteB0 = gteB1; gteB1 = gteB2; gteB2 = (unsigned char)LimitB(BB0/16.0,19); \
	\
	gteMAC1 = (long)RR0; \
	gteMAC2 = (long)GG0; \
	gteMAC3 = (long)BB0; \
	}
	*/
/*
__forceinline double ncLIM1(double x)
{
if(x > 8796093022207.0)
{
return 8796093022207.0;
}
}
*/



/* NC: OLD
#define GTE_NCCS(vn)\
gte_LL1 = limA1U((gteL11*gteVX##vn + gteL12*gteVY##vn + gteL13*gteVZ##vn)/16777216.0f);\
gte_LL2 = limA2U((gteL21*gteVX##vn + gteL22*gteVY##vn + gteL23*gteVZ##vn)/16777216.0f);\
gte_LL3 = limA3U((gteL31*gteVX##vn + gteL32*gteVY##vn + gteL33*gteVZ##vn)/16777216.0f);\
gte_RRLT= limA1U(gteRBK/4096.0f + (gteLR1/4096.0f*gte_LL1 + gteLR2/4096.0f*gte_LL2 + gteLR3/4096.0f*gte_LL3));\
gte_GGLT= limA2U(gteGBK/4096.0f + (gteLG1/4096.0f*gte_LL1 + gteLG2/4096.0f*gte_LL2 + gteLG3/4096.0f*gte_LL3));\
gte_BBLT= limA3U(gteBBK/4096.0f + (gteLB1/4096.0f*gte_LL1 + gteLB2/4096.0f*gte_LL2 + gteLB3/4096.0f*gte_LL3));\
gte_RR0 = gteR*gte_RRLT;\
gte_GG0 = gteG*gte_GGLT;\
gte_BB0 = gteB*gte_BBLT;\
gteIR1 = (long)limA1U(gte_RR0);\
gteIR2 = (long)limA2U(gte_GG0);\
gteIR3 = (long)limA3U(gte_BB0);\
gteCODE0 = gteCODE1; gteCODE1 = gteCODE2; gteCODE2 = gteCODE;\
gteR0 = gteR1; gteR1 = gteR2; gteR2 = (unsigned char)limB1(gte_RR0);\
gteG0 = gteG1; gteG1 = gteG2; gteG2 = (unsigned char)limB2(gte_GG0);\
gteB0 = gteB1; gteB1 = gteB2; gteB2 = (unsigned char)limB3(gte_BB0);\
gteMAC1 = (long)gte_RR0;\
gteMAC2 = (long)gte_GG0;\
gteMAC3 = (long)gte_BB0;\
*/
/*
#define GTE_NCCS(vn)\
gte_LL1 = limA1U((gteL11*gteVX##vn + gteL12*gteVY##vn + gteL13*gteVZ##vn)/16777216.0f);\
gte_LL2 = limA2U((gteL21*gteVX##vn + gteL22*gteVY##vn + gteL23*gteVZ##vn)/16777216.0f);\
gte_LL3 = limA3U((gteL31*gteVX##vn + gteL32*gteVY##vn + gteL33*gteVZ##vn)/16777216.0f);\
gte_RRLT= limA1U(gteRBK/4096.0f + (gteLR1/4096.0f*gte_LL1 + gteLR2/4096.0f*gte_LL2 + gteLR3/4096.0f*gte_LL3));\
gte_GGLT= limA2U(gteGBK/4096.0f + (gteLG1/4096.0f*gte_LL1 + gteLG2/4096.0f*gte_LL2 + gteLG3/4096.0f*gte_LL3));\
gte_BBLT= limA3U(gteBBK/4096.0f + (gteLB1/4096.0f*gte_LL1 + gteLB2/4096.0f*gte_LL2 + gteLB3/4096.0f*gte_LL3));\
gteMAC1 = (long)(gteR*gte_RRLT*16);\
gteMAC2 = (long)(gteG*gte_GGLT*16);\
gteMAC3 = (long)(gteB*gte_BBLT*16);\
gteIR1 = (long)limA1U(gteMAC1);\
gteIR2 = (long)limA2U(gteMAC2);\
gteIR3 = (long)limA3U(gteMAC3);\
gte_RR0 = gteMAC1>>4;\
gte_GG0 = gteMAC2>>4;\
gte_BB0 = gteMAC3>>4;\
gteCODE0 = gteCODE1; gteCODE1 = gteCODE2; gteCODE2 = gteCODE;\
gteR0 = gteR1; gteR1 = gteR2; gteR2 = (unsigned char)limB1(gte_RR0);\
gteG0 = gteG1; gteG1 = gteG2; gteG2 = (unsigned char)limB2(gte_GG0);\
gteB0 = gteB1; gteB1 = gteB2; gteB2 = (unsigned char)limB3(gte_BB0);*/


/*
	gte_LL1 = limA1U((gteL11*gteVX##vn + gteL12*gteVY##vn + gteL13*gteVZ##vn)/16777216.0f); \
	gte_LL2 = limA2U((gteL21*gteVX##vn + gteL22*gteVY##vn + gteL23*gteVZ##vn)/16777216.0f); \
	gte_LL3 = limA3U((gteL31*gteVX##vn + gteL32*gteVY##vn + gteL33*gteVZ##vn)/16777216.0f); \
	gte_RRLT= limA1U(gteRBK/4096.0f + (gteLR1/4096.0f*gte_LL1 + gteLR2/4096.0f*gte_LL2 + gteLR3/4096.0f*gte_LL3)); \
	gte_GGLT= limA2U(gteGBK/4096.0f + (gteLG1/4096.0f*gte_LL1 + gteLG2/4096.0f*gte_LL2 + gteLG3/4096.0f*gte_LL3)); \
	gte_BBLT= limA3U(gteBBK/4096.0f + (gteLB1/4096.0f*gte_LL1 + gteLB2/4096.0f*gte_LL2 + gteLB3/4096.0f*gte_LL3)); \
	\
	gteMAC1 = (long)(gteR*gte_RRLT*16); \
	gteMAC2 = (long)(gteG*gte_GGLT*16); \
	gteMAC3 = (long)(gteB*gte_BBLT*16); \
	*/
#define GTE_NCCS(vn) \
	gte_LL1 = F12limA1U((gteL11*gteVX##vn + gteL12*gteVY##vn + gteL13*gteVZ##vn) >> 12); \
	gte_LL2 = F12limA2U((gteL21*gteVX##vn + gteL22*gteVY##vn + gteL23*gteVZ##vn) >> 12); \
	gte_LL3 = F12limA3U((gteL31*gteVX##vn + gteL32*gteVY##vn + gteL33*gteVZ##vn) >> 12); \
	gte_RRLT= F12limA1U(gteRBK + ((gteLR1*gte_LL1 + gteLR2*gte_LL2 + gteLR3*gte_LL3) >> 12)); \
	gte_GGLT= F12limA2U(gteGBK + ((gteLG1*gte_LL1 + gteLG2*gte_LL2 + gteLG3*gte_LL3) >> 12)); \
	gte_BBLT= F12limA3U(gteBBK + ((gteLB1*gte_LL1 + gteLB2*gte_LL2 + gteLB3*gte_LL3) >> 12)); \
	\
	gteMAC1 = (long)(((s64)((u32)gteR<<12)*gte_RRLT) >> 20);\
	gteMAC2 = (long)(((s64)((u32)gteG<<12)*gte_GGLT) >> 20);\
	gteMAC3 = (long)(((s64)((u32)gteB<<12)*gte_BBLT) >> 20);


void gteNCCS()  {
	//	double RR0,GG0,BB0;
	//	double t1, t2, t3;
	//	double gte_LL1, gte_LL2, gte_LL3;
	//	double gte_RRLT, gte_GGLT, gte_BBLT;
	s32 gte_LL1, gte_LL2, gte_LL3;
	s32 gte_RRLT, gte_GGLT, gte_BBLT;

#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_NCCS\n");
#endif

	/*
		gteFLAG = 0;

		GTE_NCCS(0);

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("NCCS", 17);
		G_SD(0);
		G_SD(1);
		G_SD(6);
		G_SC(8);
		G_SC(9);
		G_SC(10);
		G_SC(11);
		G_SC(12);
		G_SC(13);
		G_SC(14);
		G_SC(15);
		G_SC(16);
		G_SC(17);
		G_SC(18);
		G_SC(19);
		G_SC(20);
	}
#endif

	gteFLAG = 0;

	GTE_NCCS(0);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	MAC2IR1();

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			//G_GD(20);
			//G_GD(21);
			G_GD(22);

			//G_GD(24); Doc must be wrong.  PSX does not touch it.
			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

void gteNCCT() {
	//	double RR0,GG0,BB0;
	//	double t1, t2, t3;
	//	double gte_LL1, gte_LL2, gte_LL3;
	//	double gte_RRLT, gte_GGLT, gte_BBLT;
	s32 gte_LL1, gte_LL2, gte_LL3;
	s32 gte_RRLT, gte_GGLT, gte_BBLT;

#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_NCCT\n");
#endif


	/*gteFLAG = 0;

	GTE_NCCS(0);
	GTE_NCCS(1);
	GTE_NCCS(2);

	if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("NCCT", 39);
		G_SD(0);
		G_SD(1);
		G_SD(2);
		G_SD(3);
		G_SD(4);
		G_SD(5);
		G_SD(6);

		G_SC(8);
		G_SC(9);
		G_SC(10);
		G_SC(11);
		G_SC(12);
		G_SC(13);
		G_SC(14);
		G_SC(15);
		G_SC(16);
		G_SC(17);
		G_SC(18);
		G_SC(19);
		G_SC(20);
	}
#endif

	gteFLAG = 0;

	GTE_NCCS(0);

	gteR0 = FlimB1(gteMAC1 >> 4);
	gteG0 = FlimB2(gteMAC2 >> 4);
	gteB0 = FlimB3(gteMAC3 >> 4); gteCODE0 = gteCODE;

	GTE_NCCS(1);

	gteR1 = FlimB1(gteMAC1 >> 4);
	gteG1 = FlimB2(gteMAC2 >> 4);
	gteB1 = FlimB3(gteMAC3 >> 4); gteCODE1 = gteCODE;

	GTE_NCCS(2);

	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	MAC2IR1();

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			G_GD(20);
			G_GD(21);
			G_GD(22);

			//G_GD(24); Doc must be wrong.  PSX does not touch it.
			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}
/*
#define GTE_NCDS(vn) \
gte_LL1 = limA1U((gteL11*gteVX##vn + gteL12*gteVY##vn + gteL13*gteVZ##vn)/16777216.0f);\
gte_LL2 = limA2U((gteL21*gteVX##vn + gteL22*gteVY##vn + gteL23*gteVZ##vn)/16777216.0f);\
gte_LL3 = limA3U((gteL31*gteVX##vn + gteL32*gteVY##vn + gteL33*gteVZ##vn)/16777216.0f);\
gte_RRLT= limA1U(gteRBK/4096.0f + (gteLR1/4096.0f*gte_LL1 + gteLR2/4096.0f*gte_LL2 + gteLR3/4096.0f*gte_LL3));\
gte_GGLT= limA2U(gteGBK/4096.0f + (gteLG1/4096.0f*gte_LL1 + gteLG2/4096.0f*gte_LL2 + gteLG3/4096.0f*gte_LL3));\
gte_BBLT= limA3U(gteBBK/4096.0f + (gteLB1/4096.0f*gte_LL1 + gteLB2/4096.0f*gte_LL2 + gteLB3/4096.0f*gte_LL3));\
gte_RR0 = (gteR*gte_RRLT) + (gteIR0/4096.0f * limA1S(gteRFC/16.0f - (gteR*gte_RRLT)));\
gte_GG0 = (gteG*gte_GGLT) + (gteIR0/4096.0f * limA2S(gteGFC/16.0f - (gteG*gte_GGLT)));\
gte_BB0 = (gteB*gte_BBLT) + (gteIR0/4096.0f * limA3S(gteBFC/16.0f - (gteB*gte_BBLT)));\
gteMAC1= (long)(gte_RR0 * 16.0f); gteIR1 = (long)limA1U(gte_RR0*16.0f);\
gteMAC2= (long)(gte_GG0 * 16.0f); gteIR2 = (long)limA2U(gte_GG0*16.0f);\
gteMAC3= (long)(gte_BB0 * 16.0f); gteIR3 = (long)limA3U(gte_BB0*16.0f);\
gteRGB0 = gteRGB1; \
gteRGB1 = gteRGB2; \
gteR2 = limB1(gte_RR0); \
gteG2 = limB2(gte_GG0); \
gteB2 = limB3(gte_BB0); gteCODE2 = gteCODE;
*/
/*
#define GTE_NCDS(vn) \
gte_LL1 = limA1U((gteL11*gteVX##vn + gteL12*gteVY##vn + gteL13*gteVZ##vn)/16777216.0f);\
gte_LL2 = limA2U((gteL21*gteVX##vn + gteL22*gteVY##vn + gteL23*gteVZ##vn)/16777216.0f);\
gte_LL3 = limA3U((gteL31*gteVX##vn + gteL32*gteVY##vn + gteL33*gteVZ##vn)/16777216.0f);\
gte_RRLT= limA1U(gteRBK/4096.0f + (gteLR1/4096.0f*gte_LL1 + gteLR2/4096.0f*gte_LL2 + gteLR3/4096.0f*gte_LL3));\
gte_GGLT= limA2U(gteGBK/4096.0f + (gteLG1/4096.0f*gte_LL1 + gteLG2/4096.0f*gte_LL2 + gteLG3/4096.0f*gte_LL3));\
gte_BBLT= limA3U(gteBBK/4096.0f + (gteLB1/4096.0f*gte_LL1 + gteLB2/4096.0f*gte_LL2 + gteLB3/4096.0f*gte_LL3));\
\
gte_RR0 = (gteR*gte_RRLT) + (gteIR0/4096.0f * limA1S(gteRFC/16.0f - (gteR*gte_RRLT)));\
gte_GG0 = (gteG*gte_GGLT) + (gteIR0/4096.0f * limA2S(gteGFC/16.0f - (gteG*gte_GGLT)));\
gte_BB0 = (gteB*gte_BBLT) + (gteIR0/4096.0f * limA3S(gteBFC/16.0f - (gteB*gte_BBLT)));\
gteMAC1 = (long)(gte_RR0 << 4); \
gteMAC2 = (long)(gte_GG0 << 4); \
gteMAC3 = (long)(gte_BB0 << 4);
*/
#define GTE_NCDS(vn) \
	gte_LL1 = F12limA1U((gteL11*gteVX##vn + gteL12*gteVY##vn + gteL13*gteVZ##vn) >> 12); \
	gte_LL2 = F12limA2U((gteL21*gteVX##vn + gteL22*gteVY##vn + gteL23*gteVZ##vn) >> 12); \
	gte_LL3 = F12limA3U((gteL31*gteVX##vn + gteL32*gteVY##vn + gteL33*gteVZ##vn) >> 12); \
	gte_RRLT= F12limA1U(gteRBK + ((gteLR1*gte_LL1 + gteLR2*gte_LL2 + gteLR3*gte_LL3) >> 12)); \
	gte_GGLT= F12limA2U(gteGBK + ((gteLG1*gte_LL1 + gteLG2*gte_LL2 + gteLG3*gte_LL3) >> 12)); \
	gte_BBLT= F12limA3U(gteBBK + ((gteLB1*gte_LL1 + gteLB2*gte_LL2 + gteLB3*gte_LL3) >> 12)); \
	\
	gte_RR0 = (long)(((s64)((u32)gteR<<12)*gte_RRLT) >> 12);\
	gte_GG0 = (long)(((s64)((u32)gteG<<12)*gte_GGLT) >> 12);\
	gte_BB0 = (long)(((s64)((u32)gteB<<12)*gte_BBLT) >> 12);\
	gteMAC1 = (long)((gte_RR0 + (((s64)gteIR0 * F12limA1S((s64)(gteRFC << 8) - gte_RR0)) >> 12)) >> 8);\
	gteMAC2 = (long)((gte_GG0 + (((s64)gteIR0 * F12limA2S((s64)(gteGFC << 8) - gte_GG0)) >> 12)) >> 8);\
	gteMAC3 = (long)((gte_BB0 + (((s64)gteIR0 * F12limA3S((s64)(gteBFC << 8) - gte_BB0)) >> 12)) >> 8);

void gteNCDS() {
	/*	double tRLT,tRRLT;
		double tGLT,tGGLT;
		double tBLT,tBBLT;
		double tRR0,tL1,tLL1;
		double tGG0,tL2,tLL2;
		double tBB0,tL3,tLL3;
		unsigned long C,R,G,B;	*/
	//	double gte_LL1, gte_LL2, gte_LL3;
	//	double gte_RRLT, gte_GGLT, gte_BBLT;
	s32 gte_LL1, gte_LL2, gte_LL3;
	s32 gte_RRLT, gte_GGLT, gte_BBLT;
	s32 gte_RR0, gte_GG0, gte_BB0;

#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_NCDS\n");
#endif

	/*	gteFLAG = 0;

		R = ((gteRGB)&0xff);
		G = ((gteRGB>> 8)&0xff);
		B = ((gteRGB>>16)&0xff);
		C = ((gteRGB>>24)&0xff);

		tLL1 = (gteL11/4096.0 * gteVX0/4096.0) + (gteL12/4096.0 * gteVY0/4096.0) + (gteL13/4096.0 * gteVZ0/4096.0);
		tLL2 = (gteL21/4096.0 * gteVX0/4096.0) + (gteL22/4096.0 * gteVY0/4096.0) + (gteL23/4096.0 * gteVZ0/4096.0);
		tLL3 = (gteL31/4096.0 * gteVX0/4096.0) + (gteL32/4096.0 * gteVY0/4096.0) + (gteL33/4096.0 * gteVZ0/4096.0);

		tL1 = LimitAU(tLL1,24);
		tL2 = LimitAU(tLL2,23);
		tL3 = LimitAU(tLL3,22);

		tRRLT = gteRBK/4096.0 + (gteLR1/4096.0 * tL1) + (gteLR2/4096.0 * tL2) + (gteLR3/4096.0 * tL3);
		tGGLT = gteGBK/4096.0 + (gteLG1/4096.0 * tL1) + (gteLG2/4096.0 * tL2) + (gteLG3/4096.0 * tL3);
		tBBLT = gteBBK/4096.0 + (gteLB1/4096.0 * tL1) + (gteLB2/4096.0 * tL2) + (gteLB3/4096.0 * tL3);

		tRLT = LimitAU(tRRLT,24);
		tGLT = LimitAU(tGGLT,23);
		tBLT = LimitAU(tBBLT,22);

		tRR0 = (R * tRLT) + (gteIR0/4096.0 * LimitAS(gteRFC/16.0 - (R * tRLT),24));
		tGG0 = (G * tGLT) + (gteIR0/4096.0 * LimitAS(gteGFC/16.0 - (G * tGLT),23));
		tBB0 = (B * tBLT) + (gteIR0/4096.0 * LimitAS(gteBFC/16.0 - (B * tBLT),22));

		gteMAC1 = (long)(tRR0 * 16.0); gteIR1 = (long)LimitAU((tRR0*16.0),24);
		gteMAC2 = (long)(tGG0 * 16.0); gteIR2 = (long)LimitAU((tGG0*16.0),23);
		gteMAC3 = (long)(tBB0 * 16.0); gteIR3 = (long)LimitAU((tBB0*16.0),22);

		R = (unsigned long)LimitB(tRR0,21); if (R>255) R=255; else if (R<0) R=0;
		G = (unsigned long)LimitB(tGG0,20); if (G>255) G=255; else if (G<0) G=0;
		B = (unsigned long)LimitB(tBB0,19); if (B>255) B=255; else if (B<0) B=0;

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteRGB2 = R|(G<<8)|(B<<16)|(C<<24);

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("NCDS", 19);
		G_SD(0);
		G_SD(1);
		G_SD(6);
		G_SD(8);

		G_SC(8);
		G_SC(9);
		G_SC(10);
		G_SC(11);
		G_SC(12);
		G_SC(13);
		G_SC(14);
		G_SC(15);
		G_SC(16);
		G_SC(17);
		G_SC(18);
		G_SC(19);
		G_SC(20);
		G_SC(21);
		G_SC(22);
		G_SC(23);
	}
#endif

	gteFLAG = 0;
	GTE_NCDS(0);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	MAC2IR1();

	SUM_FLAG;

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_GD(9);
		G_GD(10);
		G_GD(11);

		//G_GD(20);
		//G_GD(21);
		G_GD(22);

		G_GD(25);
		G_GD(26);
		G_GD(27);

		G_GC(31);
	}
#endif
}

void gteNCDT() {
	/*double tRLT,tRRLT;
	double tGLT,tGGLT;
	double tBLT,tBBLT;
	double tRR0,tL1,tLL1;
	double tGG0,tL2,tLL2;
	double tBB0,tL3,tLL3;
	unsigned long C,R,G,B;*/
	//	double gte_LL1, gte_LL2, gte_LL3;
	//	double gte_RRLT, gte_GGLT, gte_BBLT;
	s32 gte_LL1, gte_LL2, gte_LL3;
	s32 gte_RRLT, gte_GGLT, gte_BBLT;
	s32 gte_RR0, gte_GG0, gte_BB0;

#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_NCDT\n");
#endif

	/*	gteFLAG = 0;

		R = ((gteRGB)&0xff);
		G = ((gteRGB>> 8)&0xff);
		B = ((gteRGB>>16)&0xff);
		C = ((gteRGB>>24)&0xff);

		tLL1 = (gteL11/4096.0 * gteVX0/4096.0) + (gteL12/4096.0 * gteVY0/4096.0) + (gteL13/4096.0 * gteVZ0/4096.0);
		tLL2 = (gteL21/4096.0 * gteVX0/4096.0) + (gteL22/4096.0 * gteVY0/4096.0) + (gteL23/4096.0 * gteVZ0/4096.0);
		tLL3 = (gteL31/4096.0 * gteVX0/4096.0) + (gteL32/4096.0 * gteVY0/4096.0) + (gteL33/4096.0 * gteVZ0/4096.0);

		tL1 = LimitAU(tLL1,24);
		tL2 = LimitAU(tLL2,23);
		tL3 = LimitAU(tLL3,22);

		tRRLT = gteRBK/4096.0 + (gteLR1/4096.0 * tL1) + (gteLR2/4096.0 * tL2) + (gteLR3/4096.0 * tL3);
		tGGLT = gteGBK/4096.0 + (gteLG1/4096.0 * tL1) + (gteLG2/4096.0 * tL2) + (gteLG3/4096.0 * tL3);
		tBBLT = gteBBK/4096.0 + (gteLB1/4096.0 * tL1) + (gteLB2/4096.0 * tL2) + (gteLB3/4096.0 * tL3);

		tRLT = LimitAU(tRRLT,24);
		tGLT = LimitAU(tGGLT,23);
		tBLT = LimitAU(tBBLT,22);

		tRR0 = (R * tRLT) + (gteIR0/4096.0 * LimitAS(gteRFC/16.0 - (R * tRLT),24));
		tGG0 = (G * tGLT) + (gteIR0/4096.0 * LimitAS(gteGFC/16.0 - (G * tGLT),23));
		tBB0 = (B * tBLT) + (gteIR0/4096.0 * LimitAS(gteBFC/16.0 - (B * tBLT),22));

		gteMAC1 = (long)(tRR0 * 16.0); gteIR1 = (long)LimitAU((tRR0*16.0),24);
		gteMAC2 = (long)(tGG0 * 16.0); gteIR2 = (long)LimitAU((tGG0*16.0),23);
		gteMAC3 = (long)(tBB0 * 16.0); gteIR3 = (long)LimitAU((tBB0*16.0),22);

		R = (unsigned long)LimitB(tRR0,21); if (R>255) R=255; else if (R<0) R=0;
		G = (unsigned long)LimitB(tGG0,20); if (G>255) G=255; else if (G<0) G=0;
		B = (unsigned long)LimitB(tBB0,19); if (B>255) B=255; else if (B<0) B=0;

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteRGB2 = R|(G<<8)|(B<<16)|(C<<24);

		R = ((gteRGB)&0xff);
		G = ((gteRGB>> 8)&0xff);
		B = ((gteRGB>>16)&0xff);
		C = ((gteRGB>>24)&0xff);

		tLL1 = (gteL11/4096.0 * gteVX1/4096.0) + (gteL12/4096.0 * gteVY1/4096.0) + (gteL13/4096.0 * gteVZ1/4096.0);
		tLL2 = (gteL21/4096.0 * gteVX1/4096.0) + (gteL22/4096.0 * gteVY1/4096.0) + (gteL23/4096.0 * gteVZ1/4096.0);
		tLL3 = (gteL31/4096.0 * gteVX1/4096.0) + (gteL32/4096.0 * gteVY1/4096.0) + (gteL33/4096.0 * gteVZ1/4096.0);

		tL1 = LimitAU(tLL1,24);
		tL2 = LimitAU(tLL2,23);
		tL3 = LimitAU(tLL3,22);

		tRRLT = gteRBK/4096.0 + (gteLR1/4096.0 * tL1) + (gteLR2/4096.0 * tL2) + (gteLR3/4096.0 * tL3);
		tGGLT = gteGBK/4096.0 + (gteLG1/4096.0 * tL1) + (gteLG2/4096.0 * tL2) + (gteLG3/4096.0 * tL3);
		tBBLT = gteBBK/4096.0 + (gteLB1/4096.0 * tL1) + (gteLB2/4096.0 * tL2) + (gteLB3/4096.0 * tL3);

		tRLT = LimitAU(tRRLT,24);
		tGLT = LimitAU(tGGLT,23);
		tBLT = LimitAU(tBBLT,22);

		tRR0 = (R * tRLT) + (gteIR0/4096.0 * LimitAS(gteRFC/16.0 - (R * tRLT),24));
		tGG0 = (G * tGLT) + (gteIR0/4096.0 * LimitAS(gteGFC/16.0 - (G * tGLT),23));
		tBB0 = (B * tBLT) + (gteIR0/4096.0 * LimitAS(gteBFC/16.0 - (B * tBLT),22));

		gteMAC1 = (long)(tRR0 * 16.0); gteIR1 = (long)LimitAU((tRR0*16.0),24);
		gteMAC2 = (long)(tGG0 * 16.0); gteIR2 = (long)LimitAU((tGG0*16.0),23);
		gteMAC3 = (long)(tBB0 * 16.0); gteIR3 = (long)LimitAU((tBB0*16.0),22);

		R = (unsigned long)LimitB(tRR0,21); if (R>255) R=255; else if (R<0) R=0;
		G = (unsigned long)LimitB(tGG0,20); if (G>255) G=255; else if (G<0) G=0;
		B = (unsigned long)LimitB(tBB0,19); if (B>255) B=255; else if (B<0) B=0;

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteRGB2 = R|(G<<8)|(B<<16)|(C<<24);

		R = ((gteRGB)&0xff);
		G = ((gteRGB>> 8)&0xff);
		B = ((gteRGB>>16)&0xff);
		C = ((gteRGB>>24)&0xff);

		tLL1 = (gteL11/4096.0 * gteVX2/4096.0) + (gteL12/4096.0 * gteVY2/4096.0) + (gteL13/4096.0 * gteVZ2/4096.0);
		tLL2 = (gteL21/4096.0 * gteVX2/4096.0) + (gteL22/4096.0 * gteVY2/4096.0) + (gteL23/4096.0 * gteVZ2/4096.0);
		tLL3 = (gteL31/4096.0 * gteVX2/4096.0) + (gteL32/4096.0 * gteVY2/4096.0) + (gteL33/4096.0 * gteVZ2/4096.0);

		tL1 = LimitAU(tLL1,24);
		tL2 = LimitAU(tLL2,23);
		tL3 = LimitAU(tLL3,22);

		tRRLT = gteRBK/4096.0 + (gteLR1/4096.0 * tL1) + (gteLR2/4096.0 * tL2) + (gteLR3/4096.0 * tL3);
		tGGLT = gteGBK/4096.0 + (gteLG1/4096.0 * tL1) + (gteLG2/4096.0 * tL2) + (gteLG3/4096.0 * tL3);
		tBBLT = gteBBK/4096.0 + (gteLB1/4096.0 * tL1) + (gteLB2/4096.0 * tL2) + (gteLB3/4096.0 * tL3);

		tRLT = LimitAU(tRRLT,24);
		tGLT = LimitAU(tGGLT,23);
		tBLT = LimitAU(tBBLT,22);

		tRR0 = (R * tRLT) + (gteIR0/4096.0 * LimitAS(gteRFC/16.0 - (R * tRLT),24));
		tGG0 = (G * tGLT) + (gteIR0/4096.0 * LimitAS(gteGFC/16.0 - (G * tGLT),23));
		tBB0 = (B * tBLT) + (gteIR0/4096.0 * LimitAS(gteBFC/16.0 - (B * tBLT),22));

		gteMAC1 = (long)(tRR0 * 16.0); gteIR1 = (long)LimitAU((tRR0*16.0),24);
		gteMAC2 = (long)(tGG0 * 16.0); gteIR2 = (long)LimitAU((tGG0*16.0),23);
		gteMAC3 = (long)(tBB0 * 16.0); gteIR3 = (long)LimitAU((tBB0*16.0),22);

		R = (unsigned long)LimitB(tRR0,21); if (R>255) R=255; else if (R<0) R=0;
		G = (unsigned long)LimitB(tGG0,20); if (G>255) G=255; else if (G<0) G=0;
		B = (unsigned long)LimitB(tBB0,19); if (B>255) B=255; else if (B<0) B=0;

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteRGB2 = R|(G<<8)|(B<<16)|(C<<24);

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("NCDT", 44);
		G_SD(0);
		G_SD(1);
		G_SD(2);
		G_SD(3);
		G_SD(4);
		G_SD(5);
		G_SD(6);
		G_SD(8);

		G_SC(8);
		G_SC(9);
		G_SC(10);
		G_SC(11);
		G_SC(12);
		G_SC(13);
		G_SC(14);
		G_SC(15);
		G_SC(16);
		G_SC(17);
		G_SC(18);
		G_SC(19);
		G_SC(20);
		G_SC(21);
		G_SC(22);
		G_SC(23);
	}
#endif

	gteFLAG = 0;
	GTE_NCDS(0);

	gteR0 = FlimB1(gteMAC1 >> 4);
	gteG0 = FlimB2(gteMAC2 >> 4);
	gteB0 = FlimB3(gteMAC3 >> 4); gteCODE0 = gteCODE;

	GTE_NCDS(1);

	gteR1 = FlimB1(gteMAC1 >> 4);
	gteG1 = FlimB2(gteMAC2 >> 4);
	gteB1 = FlimB3(gteMAC3 >> 4); gteCODE1 = gteCODE;

	GTE_NCDS(2);

	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	MAC2IR1();

	SUM_FLAG;

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_GD(9);
		G_GD(10);
		G_GD(11);

		G_GD(20);
		G_GD(21);
		G_GD(22);

		G_GD(25);
		G_GD(26);
		G_GD(27);

		G_GC(31);
	}
#endif
}

#define	gteD1	(*(short *)&gteR11)
#define	gteD2	(*(short *)&gteR22)
#define	gteD3	(*(short *)&gteR33)

void gteOP() {
	//	double SSX0=0,SSY0=0,SSZ0=0;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_OP %lx\n", psxRegs.code & 0x1ffffff);
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("OP", 6);
		G_SD(9);
		G_SD(10);
		G_SD(11);

		G_SC(0);
		G_SC(2);
		G_SC(4);
	}
#endif
	/*	gteFLAG=0;

		switch (psxRegs.code & 0x1ffffff) {
		case 0x178000C://op12
		SSX0 = EDETEC1((gteR22*(short)gteIR3 - gteR33*(short)gteIR2)/(double)4096);
		SSY0 = EDETEC2((gteR33*(short)gteIR1 - gteR11*(short)gteIR3)/(double)4096);
		SSZ0 = EDETEC3((gteR11*(short)gteIR2 - gteR22*(short)gteIR1)/(double)4096);
		break;
		case 0x170000C:
		SSX0 = EDETEC1((gteR22*(short)gteIR3 - gteR33*(short)gteIR2));
		SSY0 = EDETEC2((gteR33*(short)gteIR1 - gteR11*(short)gteIR3));
		SSZ0 = EDETEC3((gteR11*(short)gteIR2 - gteR22*(short)gteIR1));
		break;
		}

		gteMAC1 = (long)float2int(SSX0);
		gteMAC2 = (long)float2int(SSY0);
		gteMAC3 = (long)float2int(SSZ0);

		MAC2IR();

		if (gteIR1<0) gteIR1=0;
		if (gteIR2<0) gteIR2=0;
		if (gteIR3<0) gteIR3=0;

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/
	gteFLAG = 0;

	/*	if (psxRegs.code  & 0x80000) {

			gteMAC1 = NC_OVERFLOW1((gteD2 * gteIR3 - gteD3 * gteIR2) / 4096.0f);
			gteMAC2 = NC_OVERFLOW2((gteD3 * gteIR1 - gteD1 * gteIR3) / 4096.0f);
			gteMAC3 = NC_OVERFLOW3((gteD1 * gteIR2 - gteD2 * gteIR1) / 4096.0f);
			} else {

			gteMAC1 = NC_OVERFLOW1(gteD2 * gteIR3 - gteD3 * gteIR2);
			gteMAC2 = NC_OVERFLOW2(gteD3 * gteIR1 - gteD1 * gteIR3);
			gteMAC3 = NC_OVERFLOW3(gteD1 * gteIR2 - gteD2 * gteIR1);
			}*/
	if (psxRegs.code & 0x80000) {
		gteMAC1 = FNC_OVERFLOW1((gteD2 * gteIR3 - gteD3 * gteIR2) >> 12);
		gteMAC2 = FNC_OVERFLOW2((gteD3 * gteIR1 - gteD1 * gteIR3) >> 12);
		gteMAC3 = FNC_OVERFLOW3((gteD1 * gteIR2 - gteD2 * gteIR1) >> 12);
	}
	else {
		gteMAC1 = FNC_OVERFLOW1(gteD2 * gteIR3 - gteD3 * gteIR2);
		gteMAC2 = FNC_OVERFLOW2(gteD3 * gteIR1 - gteD1 * gteIR3);
		gteMAC3 = FNC_OVERFLOW3(gteD1 * gteIR2 - gteD2 * gteIR1);
	}

	/* NC: old
	MAC2IR1();
	*/
	MAC2IR();

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

void gteDCPL() {
	//	unsigned long C,R,G,B;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif
#ifdef GTE_LOG
	GTE_LOG("GTE_DCPL\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("DCPL", 8);
		G_SD(6);
		G_SD(8);
		G_SD(9);
		G_SD(10);
		G_SD(11);

		G_SC(21);
		G_SC(22);
		G_SC(23);
	}
#endif
	/*	R = ((gteRGB)&0xff);
		G = ((gteRGB>> 8)&0xff);
		B = ((gteRGB>>16)&0xff);
		C = ((gteRGB>>24)&0xff);

		gteMAC1 = (signed long)((double)(R*gteIR1) + (double)(gteIR0*LimitAS(gteRFC-(double)(R*gteIR1),24))/4096.0);
		gteMAC2 = (signed long)((double)(G*gteIR2) + (double)(gteIR0*LimitAS(gteGFC-(double)(G*gteIR2),23))/4096.0);
		gteMAC3 = (signed long)((double)(B*gteIR3) + (double)(gteIR0*LimitAS(gteBFC-(double)(B*gteIR3),22))/4096.0);

		MAC2IR()

		R = (unsigned long)LimitB(gteMAC1,21); if (R>255) R=255; else if (R<0) R=0;
		G = (unsigned long)LimitB(gteMAC2,20); if (G>255) G=255; else if (G<0) G=0;
		B = (unsigned long)LimitB(gteMAC3,19); if (B>255) B=255; else if (B<0) B=0;

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteRGB2 = R|(G<<8)|(B<<16)|(C<<24);

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/

	/*	gteFLAG = 0;

		gteMAC1 = NC_OVERFLOW1((gteR * gteIR1) / 256.0f + (gteIR0 * limA1S(gteRFC - ((gteR * gteIR1) / 256.0f))) / 4096.0f);
		gteMAC2 = NC_OVERFLOW2((gteG * gteIR1) / 256.0f + (gteIR0 * limA2S(gteGFC - ((gteG * gteIR1) / 256.0f))) / 4096.0f);
		gteMAC3 = NC_OVERFLOW3((gteB * gteIR1) / 256.0f + (gteIR0 * limA3S(gteBFC - ((gteB * gteIR1) / 256.0f))) / 4096.0f);
		*/
	/*	gteMAC1 = ( (signed long)(gteR)*gteIR1 + (gteIR0*(signed short)limA1S(gteRFC - ((gteR*gteIR1)>>12) )) ) >>6;
		gteMAC2 = ( (signed long)(gteG)*gteIR2 + (gteIR0*(signed short)limA2S(gteGFC - ((gteG*gteIR2)>>12) )) ) >>6;
		gteMAC3 = ( (signed long)(gteB)*gteIR3 + (gteIR0*(signed short)limA3S(gteBFC - ((gteB*gteIR3)>>12) )) ) >>6;*/

	/*	gteMAC1 = ( (signed long)(gteR)*gteIR1 + (gteIR0*(signed short)limA1S(gteRFC - ((gteR*gteIR1)>>12) )) ) >>8;
		gteMAC2 = ( (signed long)(gteG)*gteIR2 + (gteIR0*(signed short)limA2S(gteGFC - ((gteG*gteIR2)>>12) )) ) >>8;
		gteMAC3 = ( (signed long)(gteB)*gteIR3 + (gteIR0*(signed short)limA3S(gteBFC - ((gteB*gteIR3)>>12) )) ) >>8;*/
	gteMAC1 = ((signed long)(gteR)*gteIR1 + (gteIR0*(signed short)FlimA1S(gteRFC - ((gteR*gteIR1) >> 12)))) >> 8;
	gteMAC2 = ((signed long)(gteG)*gteIR2 + (gteIR0*(signed short)FlimA2S(gteGFC - ((gteG*gteIR2) >> 12)))) >> 8;
	gteMAC3 = ((signed long)(gteB)*gteIR3 + (gteIR0*(signed short)FlimA3S(gteBFC - ((gteB*gteIR3) >> 12)))) >> 8;

	gteFLAG = 0;
	MAC2IR();

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			//G_GD(20);
			//G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

void gteGPF() {
	//	double ipx, ipy, ipz;
	//	s32 ipx, ipy, ipz;

#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif
#ifdef GTE_LOG
	GTE_LOG("GTE_GPF %lx\n", psxRegs.code & 0x1ffffff);
#endif
#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("GPF", 5);
		G_SD(6);
		G_SD(8);
		G_SD(9);
		G_SD(10);
		G_SD(11);
	}
#endif
	/*	gteFLAG = 0;

		ipx = (double)((short)gteIR0) * ((short)gteIR1);
		ipy = (double)((short)gteIR0) * ((short)gteIR2);
		ipz = (double)((short)gteIR0) * ((short)gteIR3);

		// same as mvmva
		if (psxRegs.code & 0x80000) {
		ipx /= 4096.0; ipy /= 4096.0; ipz /= 4096.0;
		}

		gteMAC1 = (long)ipx;
		gteMAC2 = (long)ipy;
		gteMAC3 = (long)ipz;

		gteIR1 = (long)LimitAS(ipx,24);
		gteIR2 = (long)LimitAS(ipy,23);
		gteIR3 = (long)LimitAS(ipz,22);

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteC2 = gteCODE;
		gteR2 = (unsigned char)LimitB(ipx,21);
		gteG2 = (unsigned char)LimitB(ipy,20);
		gteB2 = (unsigned char)LimitB(ipz,19);*/

	gteFLAG = 0;

	/*	if (psxRegs.code & 0x80000) {
			gteMAC1 = NC_OVERFLOW1((gteIR0 * gteIR1) / 4096.0f);
			gteMAC2 = NC_OVERFLOW2((gteIR0 * gteIR2) / 4096.0f);
			gteMAC3 = NC_OVERFLOW3((gteIR0 * gteIR3) / 4096.0f);
			} else {
			gteMAC1 = NC_OVERFLOW1(gteIR0 * gteIR1);
			gteMAC2 = NC_OVERFLOW2(gteIR0 * gteIR2);
			gteMAC3 = NC_OVERFLOW3(gteIR0 * gteIR3);
			}*/
	if (psxRegs.code & 0x80000) {
		gteMAC1 = FNC_OVERFLOW1((gteIR0 * gteIR1) >> 12);
		gteMAC2 = FNC_OVERFLOW2((gteIR0 * gteIR2) >> 12);
		gteMAC3 = FNC_OVERFLOW3((gteIR0 * gteIR3) >> 12);
	}
	else {
		gteMAC1 = FNC_OVERFLOW1(gteIR0 * gteIR1);
		gteMAC2 = FNC_OVERFLOW2(gteIR0 * gteIR2);
		gteMAC3 = FNC_OVERFLOW3(gteIR0 * gteIR3);
	}
	MAC2IR();

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			//G_GD(20);
			//G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

void gteGPL() {
	//   double IPX=0,IPY=0,IPZ=0;
	//    unsigned long C,R,G,B;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif
#ifdef GTE_LOG
	GTE_LOG("GTE_GPL %lx\n", psxRegs.code & 0x1ffffff);
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("GPL", 5);
		G_SD(6);
		G_SD(8);
		G_SD(9);
		G_SD(10);
		G_SD(11);

		G_SD(25);
		G_SD(26);
		G_SD(27);
	}
#endif

	/*	gteFLAG=0;
		switch(psxRegs.code & 0x1ffffff) {
		case 0x1A8003E:
		IPX = EDETEC1((double)gteMAC1 + ((double)gteIR0*(double)gteIR1)/4096.0f);
		IPY = EDETEC2((double)gteMAC2 + ((double)gteIR0*(double)gteIR2)/4096.0f);
		IPZ = EDETEC3((double)gteMAC3 + ((double)gteIR0*(double)gteIR3)/4096.0f);
		break;

		case 0x1A0003E:
		IPX = EDETEC1((double)gteMAC1 + ((double)gteIR0*(double)gteIR1));
		IPY = EDETEC2((double)gteMAC2 + ((double)gteIR0*(double)gteIR2));
		IPZ = EDETEC3((double)gteMAC3 + ((double)gteIR0*(double)gteIR3));
		break;
		}
		gteIR1  = (short)float2int(LimitAS(IPX,24));
		gteIR2  = (short)float2int(LimitAS(IPY,23));
		gteIR3  = (short)float2int(LimitAS(IPZ,22));

		gteMAC1 = (int)float2int(IPX);
		gteMAC2 = (int)float2int(IPY);
		gteMAC3 = (int)float2int(IPZ);

		C = gteRGB & 0xff000000;
		R = float2int(ALIMIT(IPX,0,255));
		G = float2int(ALIMIT(IPY,0,255));
		B = float2int(ALIMIT(IPZ,0,255));

		gteRGB0 = gteRGB1;
		gteRGB1 = gteRGB2;
		gteRGB2 = C|R|(G<<8)|(B<<16);*/
	gteFLAG = 0;

	/*	if (psxRegs.code & 0x80000) {
			gteMAC1 = NC_OVERFLOW1(gteMAC1 + (gteIR0 * gteIR1) / 4096.0f);
			gteMAC2 = NC_OVERFLOW2(gteMAC2 + (gteIR0 * gteIR2) / 4096.0f);
			gteMAC3 = NC_OVERFLOW3(gteMAC3 + (gteIR0 * gteIR3) / 4096.0f);
			} else {
			gteMAC1 = NC_OVERFLOW1(gteMAC1 + (gteIR0 * gteIR1));
			gteMAC2 = NC_OVERFLOW2(gteMAC2 + (gteIR0 * gteIR2));
			gteMAC3 = NC_OVERFLOW3(gteMAC3 + (gteIR0 * gteIR3));
			}*/
	if (psxRegs.code & 0x80000) {
		gteMAC1 = FNC_OVERFLOW1(gteMAC1 + ((gteIR0 * gteIR1) >> 12));
		gteMAC2 = FNC_OVERFLOW2(gteMAC2 + ((gteIR0 * gteIR2) >> 12));
		gteMAC3 = FNC_OVERFLOW3(gteMAC3 + ((gteIR0 * gteIR3) >> 12));
	}
	else {
		gteMAC1 = FNC_OVERFLOW1(gteMAC1 + (gteIR0 * gteIR1));
		gteMAC2 = FNC_OVERFLOW2(gteMAC2 + (gteIR0 * gteIR2));
		gteMAC3 = FNC_OVERFLOW3(gteMAC3 + (gteIR0 * gteIR3));
	}
	MAC2IR();

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			//G_GD(20);
			//G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

/*
#define GTE_DPCS() { \
	RR0 = (double)R + (gteIR0*LimitAS((double)(gteRFC - R),24))/4096.0; \
	GG0 = (double)G + (gteIR0*LimitAS((double)(gteGFC - G),23))/4096.0; \
	BB0 = (double)B + (gteIR0*LimitAS((double)(gteBFC - B),22))/4096.0; \
	\
	gteIR1 = (long)LimitAS(RR0,24); \
	gteIR2 = (long)LimitAS(GG0,23); \
	gteIR3 = (long)LimitAS(BB0,22); \
	\
	gteRGB0 = gteRGB1; \
	gteRGB1 = gteRGB2; \
	gteC2 = C; \
	gteR2 = (unsigned char)LimitB(RR0/16.0,21); \
	gteG2 = (unsigned char)LimitB(GG0/16.0,20); \
	gteB2 = (unsigned char)LimitB(BB0/16.0,19); \
	\
	gteMAC1 = (long)RR0; \
	gteMAC2 = (long)GG0; \
	gteMAC3 = (long)BB0; \
	}
	*/
void gteDPCS() {
	//	unsigned long C,R,G,B;
	//	double RR0,GG0,BB0;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif
#ifdef GTE_LOG
	GTE_LOG("GTE_DPCS\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("DPCS", 8);
		G_SD(6);
		G_SD(8);

		G_SC(21);
		G_SC(22);
		G_SC(23);
	}
#endif

	/*	gteFLAG = 0;

		C = gteCODE;
		R = gteR * 16.0;
		G = gteG * 16.0;
		B = gteB * 16.0;

		GTE_DPCS();

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/
	/*	gteFLAG = 0;

		gteMAC1 = NC_OVERFLOW1((gteR * 16.0f) + (gteIR0 * limA1S(gteRFC - (gteR * 16.0f))) / 4096.0f);
		gteMAC2 = NC_OVERFLOW2((gteG * 16.0f) + (gteIR0 * limA2S(gteGFC - (gteG * 16.0f))) / 4096.0f);
		gteMAC3 = NC_OVERFLOW3((gteB * 16.0f) + (gteIR0 * limA3S(gteBFC - (gteB * 16.0f))) / 4096.0f);
		*/
	/*	gteMAC1 = (gteR<<4) + ( (gteIR0*(signed short)limA1S(gteRFC-(gteR<<4)) ) >>12);
		gteMAC2 = (gteG<<4) + ( (gteIR0*(signed short)limA2S(gteGFC-(gteG<<4)) ) >>12);
		gteMAC3 = (gteB<<4) + ( (gteIR0*(signed short)limA3S(gteBFC-(gteB<<4)) ) >>12);*/
	gteMAC1 = (gteR << 4) + ((gteIR0*(signed short)FlimA1S(gteRFC - (gteR << 4))) >> 12);
	gteMAC2 = (gteG << 4) + ((gteIR0*(signed short)FlimA2S(gteGFC - (gteG << 4))) >> 12);
	gteMAC3 = (gteB << 4) + ((gteIR0*(signed short)FlimA3S(gteBFC - (gteB << 4))) >> 12);

	gteFLAG = 0;
	MAC2IR();

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			//G_GD(20);
			//G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

void gteDPCT() {
	//	unsigned long C,R,G,B;	
	//	double RR0,GG0,BB0;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif
#ifdef GTE_LOG
	GTE_LOG("GTE_DPCT\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("DPCT", 17);
		G_SD(8);

		G_SD(20);
		G_SD(21);
		G_SD(22);

		G_SC(21);
		G_SC(22);
		G_SC(23);
	}
#endif
	/*	gteFLAG = 0;

		C = gteCODE0;
		R = gteR0 * 16.0;
		G = gteG0 * 16.0;
		B = gteB0 * 16.0;

		GTE_DPCS();

		C = gteCODE0;
		R = gteR0 * 16.0;
		G = gteG0 * 16.0;
		B = gteB0 * 16.0;

		GTE_DPCS();

		C = gteCODE0;
		R = gteR0 * 16.0;
		G = gteG0 * 16.0;
		B = gteB0 * 16.0;

		GTE_DPCS();

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/
	/*	gteFLAG = 0;

		gteMAC1 = NC_OVERFLOW1((gteR0 * 16.0f) + gteIR0 * limA1S(gteRFC - (gteR0 * 16.0f)));
		gteMAC2 = NC_OVERFLOW2((gteG0 * 16.0f) + gteIR0 * limA2S(gteGFC - (gteG0 * 16.0f)));
		gteMAC3 = NC_OVERFLOW3((gteB0 * 16.0f) + gteIR0 * limA3S(gteBFC - (gteB0 * 16.0f)));
		*/
	/*	gteMAC1 = (gteR0<<4) + ( (gteIR0*(signed short)limA1S(gteRFC-(gteR0<<4)) ) >>12);
		gteMAC2 = (gteG0<<4) + ( (gteIR0*(signed short)limA2S(gteGFC-(gteG0<<4)) ) >>12);
		gteMAC3 = (gteB0<<4) + ( (gteIR0*(signed short)limA3S(gteBFC-(gteB0<<4)) ) >>12);*/
	gteMAC1 = (gteR0 << 4) + ((gteIR0*(signed short)FlimA1S(gteRFC - (gteR0 << 4))) >> 12);
	gteMAC2 = (gteG0 << 4) + ((gteIR0*(signed short)FlimA2S(gteGFC - (gteG0 << 4))) >> 12);
	gteMAC3 = (gteB0 << 4) + ((gteIR0*(signed short)FlimA3S(gteBFC - (gteB0 << 4))) >> 12);
	//	MAC2IR();

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	/*	gteMAC1 = (gteR0<<4) + ( (gteIR0*(signed short)limA1S(gteRFC-(gteR0<<4)) ) >>12);
		gteMAC2 = (gteG0<<4) + ( (gteIR0*(signed short)limA2S(gteGFC-(gteG0<<4)) ) >>12);
		gteMAC3 = (gteB0<<4) + ( (gteIR0*(signed short)limA3S(gteBFC-(gteB0<<4)) ) >>12);*/
	gteMAC1 = (gteR0 << 4) + ((gteIR0*(signed short)FlimA1S(gteRFC - (gteR0 << 4))) >> 12);
	gteMAC2 = (gteG0 << 4) + ((gteIR0*(signed short)FlimA2S(gteGFC - (gteG0 << 4))) >> 12);
	gteMAC3 = (gteB0 << 4) + ((gteIR0*(signed short)FlimA3S(gteBFC - (gteB0 << 4))) >> 12);
	//	MAC2IR();
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	/*	gteMAC1 = (gteR0<<4) + ( (gteIR0*(signed short)limA1S(gteRFC-(gteR0<<4)) ) >>12);
		gteMAC2 = (gteG0<<4) + ( (gteIR0*(signed short)limA2S(gteGFC-(gteG0<<4)) ) >>12);
		gteMAC3 = (gteB0<<4) + ( (gteIR0*(signed short)limA3S(gteBFC-(gteB0<<4)) ) >>12);*/
	gteMAC1 = (gteR0 << 4) + ((gteIR0*(signed short)FlimA1S(gteRFC - (gteR0 << 4))) >> 12);
	gteMAC2 = (gteG0 << 4) + ((gteIR0*(signed short)FlimA2S(gteGFC - (gteG0 << 4))) >> 12);
	gteMAC3 = (gteB0 << 4) + ((gteIR0*(signed short)FlimA3S(gteBFC - (gteB0 << 4))) >> 12);
	gteFLAG = 0;
	MAC2IR();
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			G_GD(20);
			G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

/*
#define GTE_NCS(vn) { \
	RR0 = ((double)gteVX##vn * gteL11 + (double)gteVY##vn * (double)gteL12 + (double)gteVZ##vn * gteL13) / 4096.0; \
	GG0 = ((double)gteVX##vn * gteL21 + (double)gteVY##vn * (double)gteL22 + (double)gteVZ##vn * gteL23) / 4096.0; \
	BB0 = ((double)gteVX##vn * gteL31 + (double)gteVY##vn * (double)gteL32 + (double)gteVZ##vn * gteL33) / 4096.0; \
	t1 = LimitAU(RR0, 24); \
	t2 = LimitAU(GG0, 23); \
	t3 = LimitAU(BB0, 22); \
	\
	RR0 = (double)gteRBK + ((double)gteLR1 * t1 + (double)gteLR2 * t2 + (double)gteLR3 * t3) / 4096.0; \
	GG0 = (double)gteGBK + ((double)gteLG1 * t1 + (double)gteLG2 * t2 + (double)gteLG3 * t3) / 4096.0; \
	BB0 = (double)gteBBK + ((double)gteLB1 * t1 + (double)gteLB2 * t2 + (double)gteLB3 * t3) / 4096.0; \
	t1 = LimitAU(RR0, 24); \
	t2 = LimitAU(GG0, 23); \
	t3 = LimitAU(BB0, 22); \
	\
	gteRGB0 = gteRGB1; gteRGB1 = gteRGB2; \
	gteR2 = (unsigned char)LimitB(RR0/16.0, 21); \
	gteG2 = (unsigned char)LimitB(GG0/16.0, 20); \
	gteB2 = (unsigned char)LimitB(BB0/16.0, 19); \
	gteCODE2=gteCODE0; \
	}*/

#define LOW(a) (((a) < 0) ? 0 : (a))
/*
#define	GTE_NCS(vn)  \
RR0 = LOW((gteL11*gteVX##vn + gteL12*gteVY##vn + gteL13*gteVZ##vn)/4096.0f); \
GG0 = LOW((gteL21*gteVX##vn + gteL22*gteVY##vn + gteL23*gteVZ##vn)/4096.0f); \
BB0 = LOW((gteL31*gteVX##vn + gteL32*gteVY##vn + gteL33*gteVZ##vn)/4096.0f); \
gteMAC1 = gteRBK + (gteLR1*RR0 + gteLR2*GG0 + gteLR3*BB0)/4096.0f; \
gteMAC2 = gteGBK + (gteLG1*RR0 + gteLG2*GG0 + gteLG3*BB0)/4096.0f; \
gteMAC3 = gteBBK + (gteLB1*RR0 + gteLB2*GG0 + gteLB3*BB0)/4096.0f; \
gteRGB0 = gteRGB1; \
gteRGB1 = gteRGB2; \
gteR2 = FlimB1(gteMAC1 >> 4); \
gteG2 = FlimB2(gteMAC2 >> 4); \
gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;*/
/*gteR2 = limB1(gteMAC1 / 16.0f); \
gteG2 = limB2(gteMAC2 / 16.0f); \
gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/

#define	GTE_NCS(vn)  \
	gte_LL1 = F12limA1U((gteL11*gteVX##vn + gteL12*gteVY##vn + gteL13*gteVZ##vn) >> 12); \
	gte_LL2 = F12limA2U((gteL21*gteVX##vn + gteL22*gteVY##vn + gteL23*gteVZ##vn) >> 12); \
	gte_LL3 = F12limA3U((gteL31*gteVX##vn + gteL32*gteVY##vn + gteL33*gteVZ##vn) >> 12); \
	gteMAC1 = F12limA1U(gteRBK + ((gteLR1*gte_LL1 + gteLR2*gte_LL2 + gteLR3*gte_LL3) >> 12)); \
	gteMAC2 = F12limA2U(gteGBK + ((gteLG1*gte_LL1 + gteLG2*gte_LL2 + gteLG3*gte_LL3) >> 12)); \
	gteMAC3 = F12limA3U(gteBBK + ((gteLB1*gte_LL1 + gteLB2*gte_LL2 + gteLB3*gte_LL3) >> 12));

void gteNCS() {
	//	double RR0,GG0,BB0;
	s32 gte_LL1, gte_LL2, gte_LL3;
	//	s32 RR0,GG0,BB0;
	//	double t1, t2, t3;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_NCS\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("NCS", 14);
		G_SD(0);
		G_SD(1);
		G_SD(6);

		G_SC(8);
		G_SC(9);
		G_SC(10);
		G_SC(11);
		G_SC(12);
		G_SC(13);
		G_SC(14);
		G_SC(15);
		G_SC(16);
		G_SC(17);
		G_SC(18);
		G_SC(19);
		G_SC(20);
	}
#endif
	/*	gteFLAG = 0;

		GTE_NCS(0);

		gteMAC1=(long)RR0;
		gteMAC2=(long)GG0;
		gteMAC3=(long)BB0;

		gteIR1=(long)t1;
		gteIR2=(long)t2;
		gteIR3=(long)t3;

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/
	gteFLAG = 0;

	GTE_NCS(0);

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	MAC2IR1();

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			//G_GD(20);
			//G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

void gteNCT() {
	//	double RR0,GG0,BB0;
	s32 gte_LL1, gte_LL2, gte_LL3;
	//	s32 RR0,GG0,BB0;
	//	double t1, t2, t3;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_NCT\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("NCT", 30);
		G_SD(0);
		G_SD(1);
		G_SD(2);
		G_SD(3);
		G_SD(4);
		G_SD(5);
		G_SD(6);

		G_SC(8);
		G_SC(9);
		G_SC(10);
		G_SC(11);
		G_SC(12);
		G_SC(13);
		G_SC(14);
		G_SC(15);
		G_SC(16);
		G_SC(17);
		G_SC(18);
		G_SC(19);
		G_SC(20);
	}
#endif
	/*
		gteFLAG = 0;

		//V0
		GTE_NCS(0);
		//V1
		GTE_NCS(1);
		//V2
		GTE_NCS(2);

		gteMAC1=(long)RR0;
		gteMAC2=(long)GG0;
		gteMAC3=(long)BB0;

		gteIR1=(long)t1;
		gteIR2=(long)t2;
		gteIR3=(long)t3;

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/
	gteFLAG = 0;

	GTE_NCS(0);

	gteR0 = FlimB1(gteMAC1 >> 4);
	gteG0 = FlimB2(gteMAC2 >> 4);
	gteB0 = FlimB3(gteMAC3 >> 4); gteCODE0 = gteCODE;

	GTE_NCS(1);
	gteR1 = FlimB1(gteMAC1 >> 4);
	gteG1 = FlimB2(gteMAC2 >> 4);
	gteB1 = FlimB3(gteMAC3 >> 4); gteCODE1 = gteCODE;

	GTE_NCS(2);
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	MAC2IR1();

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			G_GD(20);
			G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

void gteCC() {
	//	double RR0,GG0,BB0;
	s32 RR0, GG0, BB0;
	//	double t1,t2,t3;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif
#ifdef GTE_LOG
	GTE_LOG("GTE_CC\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("CC", 11);
		G_SD(6);
		G_SD(9);
		G_SD(10);
		G_SD(11);

		G_SC(13);
		G_SC(14);
		G_SC(15);
		G_SC(16);
		G_SC(17);
		G_SC(18);
		G_SC(19);
	}
#endif
	/*	gteFLAG = 0;

		RR0 = (double)gteRBK + ((double)gteLR1 * gteIR1 + (double)gteLR2 * gteIR2 + (double)gteLR3 * gteIR3) / 4096.0;
		GG0 = (double)gteGBK + ((double)gteLG1 * gteIR1 + (double)gteLG2 * gteIR2 + (double)gteLG3 * gteIR3) / 4096.0;
		BB0 = (double)gteBBK + ((double)gteLB1 * gteIR1 + (double)gteLB2 * gteIR2 + (double)gteLB3 * gteIR3) / 4096.0;
		t1 = LimitAU(RR0, 24);
		t2 = LimitAU(GG0, 23);
		t3 = LimitAU(BB0, 22);

		RR0=((double)gteR * t1)/256.0;
		GG0=((double)gteG * t2)/256.0;
		BB0=((double)gteB * t3)/256.0;
		gteIR1 = (long)LimitAU(RR0,24);
		gteIR2 = (long)LimitAU(GG0,23);
		gteIR3 = (long)LimitAU(BB0,22);

		gteCODE0=gteCODE1; gteCODE1=gteCODE2;
		gteC2 = gteCODE0;
		gteR2 = (unsigned char)LimitB(RR0/16.0, 21);
		gteG2 = (unsigned char)LimitB(GG0/16.0, 20);
		gteB2 = (unsigned char)LimitB(BB0/16.0, 19);

		if (gteFLAG & 0x7f87e000) gteFLAG|=0x80000000;*/
	gteFLAG = 0;

	/*	RR0 = NC_OVERFLOW1(gteRBK + (gteLR1*gteIR1 + gteLR2*gteIR2 + gteLR3*gteIR3) / 4096.0f);
		GG0 = NC_OVERFLOW2(gteGBK + (gteLG1*gteIR1 + gteLG2*gteIR2 + gteLG3*gteIR3) / 4096.0f);
		BB0 = NC_OVERFLOW3(gteBBK + (gteLB1*gteIR1 + gteLB2*gteIR2 + gteLB3*gteIR3) / 4096.0f);

		gteMAC1 = gteR * RR0 / 256.0f;
		gteMAC2 = gteG * GG0 / 256.0f;
		gteMAC3 = gteB * BB0 / 256.0f;*/
	RR0 = FNC_OVERFLOW1(gteRBK + ((gteLR1*gteIR1 + gteLR2*gteIR2 + gteLR3*gteIR3) >> 12));
	GG0 = FNC_OVERFLOW2(gteGBK + ((gteLG1*gteIR1 + gteLG2*gteIR2 + gteLG3*gteIR3) >> 12));
	BB0 = FNC_OVERFLOW3(gteBBK + ((gteLB1*gteIR1 + gteLB2*gteIR2 + gteLB3*gteIR3) >> 12));

	gteMAC1 = (gteR * RR0) >> 8;
	gteMAC2 = (gteG * GG0) >> 8;
	gteMAC3 = (gteB * BB0) >> 8;

	MAC2IR1();

	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			//G_GD(20);
			//G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

void gteINTPL() { //test opcode
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif
#ifdef GTE_LOG
	GTE_LOG("GTE_INTP\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("INTPL", 8);
		G_SD(6);
		G_SD(8);
		G_SD(9);
		G_SD(10);
		G_SD(11);

		G_SC(21);
		G_SC(22);
		G_SC(23);
	}
#endif
	/* NC: old
	gteFLAG=0;
	gteMAC1 = gteIR1 + gteIR0*limA1S(gteRFC-gteIR1);
	gteMAC2 = gteIR2 + gteIR0*limA2S(gteGFC-gteIR2);
	gteMAC3 = gteIR3 + gteIR0*limA3S(gteBFC-gteIR3);
	//gteFLAG = 0;
	MAC2IR();
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	gteR2 = limB1(gteMAC1 / 16.0f);
	gteG2 = limB2(gteMAC2 / 16.0f);
	gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;
	*/

	/*	gteFLAG=0;
		gteMAC1 = gteIR1 + gteIR0*(gteRFC-gteIR1)/4096.0;
		gteMAC2 = gteIR2 + gteIR0*(gteGFC-gteIR2)/4096.0;
		gteMAC3 = gteIR3 + gteIR0*(gteBFC-gteIR3)/4096.0;

		//gteMAC3 = (int)((((psxRegs).CP2D).n).ir3+(((psxRegs).CP2D).n).ir0 * ((((psxRegs).CP2C).n).bfc-(((psxRegs).CP2D).n).ir3)/4096.0);

		if(gteMAC3 > gteIR1 && gteMAC3 > gteBFC)
		{
		gteMAC3 = gteMAC3;
		}
		//gteFLAG = 0;*/
	//NEW CODE
	/*	gteMAC1 = gteIR1 + ((gteIR0*(signed short)limA1S(gteRFC-gteIR1))>>12);
		gteMAC2 = gteIR2 + ((gteIR0*(signed short)limA2S(gteGFC-gteIR2))>>12);
		gteMAC3 = gteIR3 + ((gteIR0*(signed short)limA3S(gteBFC-gteIR3))>>12);*/
	gteMAC1 = gteIR1 + ((gteIR0*(signed short)FlimA1S(gteRFC - gteIR1)) >> 12);
	gteMAC2 = gteIR2 + ((gteIR0*(signed short)FlimA2S(gteGFC - gteIR2)) >> 12);
	gteMAC3 = gteIR3 + ((gteIR0*(signed short)FlimA3S(gteBFC - gteIR3)) >> 12);
	gteFLAG = 0;

	MAC2IR();
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			//G_GD(20);
			//G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}

void gteCDP() { //test opcode
	double RR0, GG0, BB0;
	//	s32 RR0,GG0,BB0;
#ifdef GTE_DUMP
	static int sample = 0; sample++;
#endif

#ifdef GTE_LOG
	GTE_LOG("GTE_CDP\n");
#endif

#ifdef GTE_DUMP
	if(sample < 100)
	{
		G_OP("CDP", 13);
		G_SD(6);
		G_SD(8);
		G_SD(9);
		G_SD(10);
		G_SD(11);

		G_SC(13);
		G_SC(14);
		G_SC(15);
		G_SC(16);
		G_SC(17);
		G_SC(18);
		G_SC(19);
		G_SC(20);
		G_SC(21);
		G_SC(22);
		G_SC(23);
	}
#endif

	gteFLAG = 0;

	RR0 = NC_OVERFLOW1(gteRBK + (gteLR1*gteIR1 + gteLR2*gteIR2 + gteLR3*gteIR3));
	GG0 = NC_OVERFLOW2(gteGBK + (gteLG1*gteIR1 + gteLG2*gteIR2 + gteLG3*gteIR3));
	BB0 = NC_OVERFLOW3(gteBBK + (gteLB1*gteIR1 + gteLB2*gteIR2 + gteLB3*gteIR3));
	gteMAC1 = gteR*RR0 + gteIR0*limA1S(gteRFC - gteR*RR0);
	gteMAC2 = gteG*GG0 + gteIR0*limA2S(gteGFC - gteG*GG0);
	gteMAC3 = gteB*BB0 + gteIR0*limA3S(gteBFC - gteB*BB0);

	/*	RR0 = FNC_OVERFLOW1(gteRBK + (gteLR1*gteIR1 +gteLR2*gteIR2 + gteLR3*gteIR3));
		GG0 = FNC_OVERFLOW2(gteGBK + (gteLG1*gteIR1 +gteLG2*gteIR2 + gteLG3*gteIR3));
		BB0 = FNC_OVERFLOW3(gteBBK + (gteLB1*gteIR1 +gteLB2*gteIR2 + gteLB3*gteIR3));
		gteMAC1 = gteR*RR0 + gteIR0*FlimA1S(gteRFC-gteR*RR0);
		gteMAC2 = gteG*GG0 + gteIR0*FlimA2S(gteGFC-gteG*GG0);
		gteMAC3 = gteB*BB0 + gteIR0*FlimA3S(gteBFC-gteB*BB0);*/

	MAC2IR1();
	gteRGB0 = gteRGB1;
	gteRGB1 = gteRGB2;

	/*	gteR2 = limB1(gteMAC1 / 16.0f);
		gteG2 = limB2(gteMAC2 / 16.0f);
		gteB2 = limB3(gteMAC3 / 16.0f); gteCODE2 = gteCODE;*/
	gteR2 = FlimB1(gteMAC1 >> 4);
	gteG2 = FlimB2(gteMAC2 >> 4);
	gteB2 = FlimB3(gteMAC3 >> 4); gteCODE2 = gteCODE;

	SUM_FLAG

#ifdef GTE_DUMP
		if(sample < 100)
		{
			G_GD(9);
			G_GD(10);
			G_GD(11);

			//G_GD(20);
			//G_GD(21);
			G_GD(22);

			G_GD(25);
			G_GD(26);
			G_GD(27);

			G_GC(31);
		}
#endif
}
