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


#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <zlib.h>

#include "PS2Edefs.h"
#include "Misc.h"

extern FILE *emuLog;

char* disR5900F(u32 code, u32 pc);
char* disR5900Fasm(u32 code, u32 pc);
char* disR3000Fasm(u32 code, u32 pc);

void  disR5900AddSym(u32 addr, char *name);
char* disR5900GetSym(u32 addr);
char* disR5900GetUpperSym(u32 addr);
void  disR5900FreeSyms();

char* disVU0MicroUF(u32 code, u32 pc);
char* disVU0MicroLF(u32 code, u32 pc);
char* disVU1MicroUF(u32 code, u32 pc);
char* disVU1MicroLF(u32 code, u32 pc);

char* disR3000AF(u32 code, u32 pc);

extern char *CP2VFnames[];
extern char *disRNameCP2f[];
extern char *disRNameCP2i[];

//that way is slower but you now not need to compile every time ;P
#ifdef PCSX2_DEVBUILD

extern int Log;
extern u32 varLog;
extern u16 logProtocol;
extern u8  logSource;

void __Log(char *fmt, ...);

//memcars has the same number as PAD_LOG
#define MEMCARDS_LOG if (varLog & 0x02000000) {logProtocol=7; logSource='I';} if (varLog & 0x02000000) __Log

#define CPU_LOG  if (varLog & 0x00000001) {logProtocol=1; logSource='E';} if (varLog & 0x00000001) __Log
#define MEM_LOG  if (varLog & 0x00000002) {logProtocol=6; logSource='E';} if (varLog & 0x00000002) __Log("%8.8lx: ", cpuRegs.pc); if (varLog & 0x00000002) __Log
#define HW_LOG   if (varLog & 0x00000004) {logProtocol=6; logSource='E';} if (varLog & 0x00000004) __Log("%8.8lx: ", cpuRegs.pc); if (varLog & 0x00000004) __Log
#define DMA_LOG  if (varLog & 0x00000008) {logProtocol=5; logSource='E';} if (varLog & 0x00000008) __Log
#define BIOS_LOG if (varLog & 0x00000010) {logProtocol=0; logSource='E';} if (varLog & 0x00000010) __Log("%8.8lx: ", cpuRegs.pc); if (varLog & 0x00000010) __Log
#define ELF_LOG  if (varLog & 0x00000020) {logProtocol=7; logSource='E';} if (varLog & 0x00000020) __Log
#define FPU_LOG  if (varLog & 0x00000040) {logProtocol=1; logSource='E';} if (varLog & 0x00000040) __Log
#define MMI_LOG  if (varLog & 0x00000080) {logProtocol=1; logSource='E';} if (varLog & 0x00000080) __Log
#define VU0_LOG  if (varLog & 0x00000100) {logProtocol=2; logSource='E';} if (varLog & 0x00000100) __Log
#define COP0_LOG if (varLog & 0x00000200) {logProtocol=1; logSource='E';} if (varLog & 0x00000200) __Log
#define VIF_LOG  if (varLog & 0x00000400) {logProtocol=3; logSource='E';} if (varLog & 0x00000400) __Log
#define SPR_LOG  if (varLog & 0x00000800) {logProtocol=7; logSource='E';} if (varLog & 0x00000800) __Log
#define GIF_LOG  if (varLog & 0x00001000) {logProtocol=4; logSource='E';} if (varLog & 0x00001000) __Log
#define SIF_LOG  if (varLog & 0x00002000) {logProtocol=9; logSource='E';} if (varLog & 0x00002000) __Log
#define IPU_LOG  if (varLog & 0x00004000) {logProtocol=8; logSource='E';} if (varLog & 0x00004000) __Log
#define VUM_LOG  if (varLog & 0x00008000) {logProtocol=2; logSource='E';} if (varLog & 0x00008000) __Log
#define RPC_LOG  if (varLog & 0x00010000) {logProtocol=9; logSource='E';} if (varLog & 0x00010000) __Log

#define PSXCPU_LOG  if (varLog & 0x00100000) {logProtocol=1; logSource='I';} if (varLog & 0x00100000) __Log
#define PSXMEM_LOG  if (varLog & 0x00200000) {logProtocol=6; logSource='I';} if (varLog & 0x00200000) __Log("%8.8lx : ", psxRegs.pc); if (varLog & 0x00200000) __Log
#define PSXHW_LOG   if (varLog & 0x00400000) {logProtocol=2; logSource='I';} if (varLog & 0x00400000) __Log("%8.8lx : ", psxRegs.pc); if (varLog & 0x00400000) __Log
#define PSXBIOS_LOG if (varLog & 0x00800000) {logProtocol=0; logSource='I';} if (varLog & 0x00800000) __Log("%8.8lx : ", psxRegs.pc); if (varLog & 0x00800000) __Log
#define PSXDMA_LOG  if (varLog & 0x01000000) {logProtocol=5; logSource='I';} if (varLog & 0x01000000) __Log

#define PAD_LOG  if (varLog & 0x02000000) {logProtocol=7; logSource='I';} if (varLog & 0x02000000) __Log
#define GTE_LOG  if (varLog & 0x04000000) {logProtocol=3; logSource='I';} if (varLog & 0x04000000) __Log
#define CDR_LOG  if (varLog & 0x08000000) {logProtocol=8; logSource='I';} if (varLog & 0x08000000) __Log("%8.8lx %8.8lx: ", psxRegs.pc, psxRegs.cycle); if (varLog & 0x08000000) __Log
#define GPU_LOG  if (varLog & 0x10000000) {logProtocol=4; logSource='I';} if (varLog & 0x10000000) __Log
#define PSXCNT_LOG  if (varLog & 0x20000000) {logProtocol=0; logSource='I';} if (varLog & 0x20000000) __Log("%8.8lx %8.8lx: ", psxRegs.pc, psxRegs.cycle); if (varLog & 0x20000000) __Log
#define EECNT_LOG  if (varLog & 0x40000000) {logProtocol=0; logSource='I';} if (varLog & 0x40000000) __Log("%8.8lx %8.8lx: ", cpuRegs.pc, cpuRegs.cycle); if (varLog & 0x40000000) __Log

#if defined (CPU_LOG)   || defined(MEM_LOG)   || defined(HW_LOG)     || defined(DMA_LOG)  || \
	defined(BIOS_LOG)   || defined(ELF_LOG)   || defined(FPU_LOG)    || defined(MMI_LOG)  || \
	defined(VU0_LOG)    || defined(COP0_LOG)  || defined(VIF_LOG)    || defined(SPR_LOG)  || \
	defined(GIF_LOG)    || defined(SIF_LOG)   || defined(IPU_LOG)    || defined(VUM_log)  || \
	defined(PSXCPU_LOG) || defined(PSXMEM_LOG)|| defined(IOPBIOS_LOG)|| defined(IOPHW_LOG)|| \
	defined(PAD_LOG)    || defined(GTE_LOG)   || defined(CDR_LOG)    || defined(GPU_LOG)  || \
	defined(MEMCARDS_LOG)|| defined(PSXCNT_LOG) || defined(EECNT_LOG)
#define EMU_LOG __Log
#endif

#else // PCSX2_DEVBUILD

#define varLog 0
#define Log 0

#define CPU_LOG  0&&
#define MEM_LOG  0&&
#define HW_LOG   0&&
#define DMA_LOG  0&&
#define BIOS_LOG 0&&
#define ELF_LOG  0&&
#define FPU_LOG  0&&
#define MMI_LOG  0&&
#define VU0_LOG  0&&
#define COP0_LOG 0&&
#define VIF_LOG  0&&
#define SPR_LOG  0&&
#define GIF_LOG  0&&
#define SIF_LOG  0&&
#define IPU_LOG  0&&
#define VUM_LOG  0&&
#define RPC_LOG  0&&

#define PSXCPU_LOG  0&&
#define PSXMEM_LOG  0&&
#define PSXHW_LOG   0&&
#define PSXBIOS_LOG 0&&
#define PSXDMA_LOG  0&&

#define PAD_LOG  0&&
#define GTE_LOG  0&&
#define CDR_LOG  0&&
#define GPU_LOG  0&&
#define PSXCNT_LOG 0&&
#define EECNT_LOG 0&&

#define EMU_LOG 0&&

#endif

#endif /* __DEBUG_H__ */
