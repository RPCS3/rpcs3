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

#ifndef __MISC_H__
#define __MISC_H__

#include <stddef.h>
#include <malloc.h>
#include <assert.h>

// compile-time assert
#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif

#ifdef __x86_64__
#define X86_32CODE(x)
#else
#define X86_32CODE(x) x
#endif

#define PCSX2_GSMULTITHREAD 1 // uses multithreaded gs
#define PCSX2_DUALCORE 2 // speed up for dual cores
#define PCSX2_FRAMELIMIT 4 // limits frames to normal speeds
#define PCSX2_EEREC 0x10
#define PCSX2_VU0REC 0x20
#define PCSX2_VU1REC 0x40
#define PCSX2_COP2REC 0x80
#define PCSX2_FORCEABS 0x100
#define PCSX2_FRAMELIMIT_MASK 0xc00
#define PCSX2_FRAMELIMIT_NORMAL 0x000
#define PCSX2_FRAMELIMIT_LIMIT 0x400
#define PCSX2_FRAMELIMIT_SKIP 0x800
#define PCSX2_FRAMELIMIT_VUSKIP 0xc00

#define CHECK_MULTIGS (Config.Options&PCSX2_GSMULTITHREAD)
#define CHECK_DUALCORE (Config.Options&PCSX2_DUALCORE)
#define CHECK_EEREC (Config.Options&PCSX2_EEREC)
#define CHECK_COP2REC (Config.Options&PCSX2_COP2REC) // goes with ee option
#define CHECK_FORCEABS (~(Config.Hacks >> 1) & 1) // always on, (Config.Options&PCSX2_FORCEABS)

#define CHECK_FRAMELIMIT (Config.Options&PCSX2_FRAMELIMIT_MASK)

//#ifdef PCSX2_DEVBUILD
#define CHECK_VU0REC (Config.Options&PCSX2_VU0REC)
#define CHECK_VU1REC (Config.Options&PCSX2_VU1REC)
//#else
//// force to VU recs all the time
//#define CHECK_VU0REC 1
//#define CHECK_VU1REC 1
//
//#endif

typedef struct {
	char Bios[256];
	char GS[256];
	char PAD1[256];
	char PAD2[256];
	char SPU2[256];
	char CDVD[256];
	char DEV9[256];
	char USB[256];
	char FW[256];
	char Mcd1[256];
	char Mcd2[256];
	char PluginsDir[256];
	char BiosDir[256];
	char Lang[256];
	u32 Options; // PCSX2_X options
	int PsxOut;
	int PsxType;
	int Cdda;
	int Mdec;
	int Patch;
	int ThPriority;
	int CustomFps;
	int Hacks;
} PcsxConfig;

extern PcsxConfig Config;
extern u32 BiosVersion;
extern char CdromId[12];

#define gzfreeze(ptr, size) \
	if (Mode == 1) gzwrite(f, ptr, size); \
	else if (Mode == 0) gzread(f, ptr, size);

#define gzfreezel(ptr) gzfreeze(ptr, sizeof(ptr))

int LoadCdrom();
int CheckCdrom();
int GetPS2ElfName(char*);

extern char *LabelAuthors;
extern char *LabelGreets;
int SaveState(char *file);
int LoadState(char *file);
int CheckState(char *file);

int SaveGSState(char *file);
int LoadGSState(char *file);

char *ParseLang(char *id);
void ProcessFKeys(int fkey, int shift); // processes fkey related commands value 1-12

#ifdef _WIN32

void ListPatches (HWND hW);
int ReadPatch (HWND hW, char fileName[1024]);
char * lTrim (char *s);
BOOL Save_Patch_Proc( char * filename );

#else

// functions that linux lacks
#define Sleep(seconds) usleep(1000*(seconds))

#include <sys/timeb.h>

extern __forceinline u32 timeGetTime()
{
	struct timeb t;
	ftime(&t);
	return (u32)(t.time*1000+t.millitm);
}

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define BOOL int

#undef TRUE
#define TRUE  1
#undef FALSE
#define FALSE 0

#ifndef strnicmp
#define strnicmp strncasecmp
#endif

#ifndef stricmp
#define stricmp strcasecmp
#endif

#endif

#define DIRENTRY_SIZE 16

#if defined(_MSC_VER)
#pragma pack(1)
#endif

struct romdir{
	char fileName[10];
	u16 extInfoSize;
	u32 fileSize;
#if defined(_MSC_VER)
};						//+22
#else
} __attribute__((packed));
#endif

u32 GetBiosVersion();
int IsBIOS(char *filename, char *description);

// check to see if needs freezing
#ifdef PCSX2_NORECBUILD
#define FreezeMMXRegs(save)
#define FreezeXMMRegs(save)
#else
void FreezeXMMRegs_(int save);
extern u32 g_EEFreezeRegs;
#define FreezeXMMRegs(save) if( g_EEFreezeRegs ) { FreezeXMMRegs_(save); }

#ifndef __x86_64__
void FreezeMMXRegs_(int save);
#define FreezeMMXRegs(save) if( g_EEFreezeRegs ) { FreezeMMXRegs_(save); }
#else
#define FreezeMMXRegs(save)
#endif

#endif

// define a PCS2 specific memcpy and make sure it is used all in real-time code
#if _MSC_VER >= 1400 // vs2005+ uses xmm/mmx in memcpy
__forceinline void memcpy_pcsx2(void* dest, const void* src, size_t n)
{
    //FreezeMMXRegs(1); // mmx not used
    FreezeXMMRegs(1);
    memcpy(dest, src, n);
    // have to be unfrozen by parent call!
}
#else
#define memcpy_pcsx2 memcpy
#endif

#ifdef PCSX2_NORECBUILD
#define memcpy_fast memcpy
#else

#if defined(_WIN32) && !defined(__x86_64__)
// faster memcpy
void * memcpy_amd_(void *dest, const void *src, size_t n);
#define memcpy_fast memcpy_amd_
//#define memcpy_fast memcpy //Dont use normal memcpy, it has sse in 2k5!
#else
// for now disable linux fast memcpy
#define memcpy_fast memcpy_pcsx2
#endif

#endif

u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize);
void memxor_mmx(void* dst, const void* src1, int cmpsize);

#ifdef	_MSC_VER
#pragma pack()
#endif

void __Log(char *fmt, ...);
void injectIRX(char *filename);

#if !defined(_MSC_VER) && !defined(HAVE_ALIGNED_MALLOC)

// declare linux equivalents
extern  __forceinline void* pcsx2_aligned_malloc(size_t size, size_t align)
{
    assert( align < 0x10000 );
	char* p = (char*)malloc(size+align);
	int off = 2+align - ((int)(uptr)(p+2) % align);

	p += off;
	*(u16*)(p-2) = off;

	return p;
}

extern __forceinline void pcsx2_aligned_free(void* pmem)
{
    if( pmem != NULL ) {
        char* p = (char*)pmem;
        free(p - (int)*(u16*)(p-2));
    }
}

#define _aligned_malloc pcsx2_aligned_malloc
#define _aligned_free pcsx2_aligned_free

#endif

// cross-platform atomic operations
#if defined (_WIN32)
/*
#ifndef __x86_64__ // for some reason x64 doesn't like this

LONG  __cdecl _InterlockedIncrement(LONG volatile *Addend);
LONG  __cdecl _InterlockedDecrement(LONG volatile *Addend);
LONG  __cdecl _InterlockedCompareExchange(LPLONG volatile Dest, LONG Exchange, LONG Comp);
LONG  __cdecl _InterlockedExchange(LPLONG volatile Target, LONG Value);
PVOID __cdecl _InterlockedExchangePointer(PVOID volatile* Target, PVOID Value);

LONG  __cdecl _InterlockedExchangeAdd(LPLONG volatile Addend, LONG Value);
LONG  __cdecl _InterlockedAnd(LPLONG volatile Addend, LONG Value);
#endif

#pragma intrinsic (_InterlockedExchange)
#define InterlockedExchange _InterlockedExchange

#pragma intrinsic (_InterlockedExchangeAdd)
#define InterlockedExchangeAdd _InterlockedExchangeAdd
*/
#else

typedef void* PVOID;

/*inline unsigned long _Atomic_swap(unsigned long * __p, unsigned long __q) {
 #       if __mips < 3 || !(defined (_ABIN32) || defined(_ABI64))
             return test_and_set(__p, __q);
 #       else
             return __test_and_set(__p, (unsigned long)__q);
 #       endif
 }*/

extern __forceinline void InterlockedExchangePointer(PVOID volatile* Target, void* Value)
{
#ifdef __x86_64__
	__asm__ __volatile__(".intel_syntax\n"
						 "lock xchg [%0], %%rax\n"
						 ".att_syntax\n" : : "r"(Target), "a"(Value) : "memory" );
#else
	__asm__ __volatile__(".intel_syntax\n"
						 "lock xchg [%0], %%eax\n"
						 ".att_syntax\n" : : "r"(Target), "a"(Value) : "memory" );
#endif
}

extern __forceinline long InterlockedExchange(long volatile* Target, long Value)
{
	__asm__ __volatile__(".intel_syntax\n"
						 "lock xchg [%0], %%eax\n"
						 ".att_syntax\n" : : "r"(Target), "a"(Value) : "memory" );
}

extern __forceinline long InterlockedExchangeAdd(long volatile* Addend, long Value)
{
	__asm__ __volatile__(".intel_syntax\n"
						 "lock xadd [%0], %%eax\n"
						 ".att_syntax\n" : : "r"(Addend), "a"(Value) : "memory" );
}

#endif

//#pragma intrinsic (_InterlockedExchange64)
//#define InterlockedExchange64 _InterlockedExchange64
//
//#pragma intrinsic (_InterlockedExchangeAdd64)
//#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64

//#ifdef __x86_64__
//#define InterlockedExchangePointerAdd InterlockedExchangeAdd64
//#else
//#define InterlockedExchangePointerAdd InterlockedExchangeAdd
//#endif

#endif /* __MISC_H__ */

