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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __MISC_H__
#define __MISC_H__

#include "System.h"

/////////////////////////////////////////////////////////////////////////
// GNU GetText / NLS

#ifdef ENABLE_NLS

#ifdef _WIN32
#include "libintlmsc.h"
#else
#include <locale.h>
#include <libintl.h>
#endif

#undef _
#define _(String) dgettext (PACKAGE, String)
#ifdef gettext_noop
#  define N_(String) gettext_noop (String)
#else
#  define N_(String) (String)
#endif

#else

#define _(msgid) msgid
#define N_(msgid) msgid

#endif		// ENABLE_NLS

// [TODO] : Move the config options mess from Misc.h into "config.h" or someting more sensible.

/////////////////////////////////////////////////////////////////////////
// Session Configuration Override Flags
//
// a handful of flags that can override user configurations for the current application session
// only.  This allows us to do things like force-disable recompilers if the memory allocations
// for them fail.
struct SessionOverrideFlags
{
	bool ForceDisableEErec:1;
	bool ForceDisableVU0rec:1;
	bool ForceDisableVU1rec:1;
};

extern SessionOverrideFlags g_Session;

//////////////////////////////////////////////////////////////////////////
// Pcsx2 User Configuration Options!

//#define PCSX2_MICROVU // Use Micro VU recs instead of Zero VU Recs
#define PCSX2_GSMULTITHREAD 1 // uses multi-threaded gs
#define PCSX2_EEREC 0x10
#define PCSX2_VU0REC 0x20
#define PCSX2_VU1REC 0x40
#define PCSX2_FRAMELIMIT_MASK 0xc00
#define PCSX2_FRAMELIMIT_NORMAL 0x000
#define PCSX2_FRAMELIMIT_LIMIT 0x400
#define PCSX2_FRAMELIMIT_SKIP 0x800
#define PCSX2_FRAMELIMIT_VUSKIP 0xc00

#define CHECK_MULTIGS (Config.Options&PCSX2_GSMULTITHREAD)
#define CHECK_EEREC (!g_Session.ForceDisableEErec && Config.Options&PCSX2_EEREC)
//------------ SPEED/MISC HACKS!!! ---------------
#define CHECK_EE_CYCLERATE (Config.Hacks & 0x03)
#define CHECK_IOP_CYCLERATE (Config.Hacks & (1<<3))
#define CHECK_WAITCYCLE_HACK (Config.Hacks & (1<<4))
#define CHECK_INTC_STAT_HACK (Config.Hacks & (1<<5))
#define CHECK_ESCAPE_HACK	(Config.Hacks & 0x400)
//------------ SPECIAL GAME FIXES!!! ---------------
#define CHECK_VUADDSUBHACK	(Config.GameFixes & 0x1) // Special Fix for Tri-ace games, they use an encryption algorithm that requires VU addi opcode to be bit-accurate.
#define CHECK_FPUCLAMPHACK	(Config.GameFixes & 0x4) // Special Fix for Tekken 5, different clamping for FPU (sets NaN to zero; doesn't clamp infinities)
#define CHECK_VUBRANCHHACK	(Config.GameFixes & 0x8) // Special Fix for Magna Carta (note: Breaks Crash Bandicoot)
//------------ Advanced Options!!! ---------------
#define CHECK_VU_OVERFLOW		(Config.vuOptions & 0x1)
#define CHECK_VU_EXTRA_OVERFLOW	(Config.vuOptions & 0x2) // If enabled, Operands are clamped before being used in the VU recs
#define CHECK_VU_SIGN_OVERFLOW	(Config.vuOptions & 0x4)
#define CHECK_VU_UNDERFLOW		(Config.vuOptions & 0x8)
#define CHECK_VU_EXTRA_FLAGS 0	// Always disabled now // Sets correct flags in the VU recs
#define CHECK_FPU_OVERFLOW			(Config.eeOptions & 0x1)
#define CHECK_FPU_EXTRA_OVERFLOW	(Config.eeOptions & 0x2) // If enabled, Operands are checked for infinities before being used in the FPU recs
#define CHECK_FPU_EXTRA_FLAGS 1	// Always enabled now // Sets D/I flags on FPU instructions
#define DEFAULT_eeOptions	0x01
#define DEFAULT_vuOptions	0x01
//------------ DEFAULT sseMXCSR VALUES!!! ---------------
#define DEFAULT_sseMXCSR	0x7fc0 //FPU rounding, DaZ, "chop" - Note: Dont enable FtZ by default, it breaks games! E.g. Enthusia (Refraction)
#define DEFAULT_sseVUMXCSR	0x7fc0 //VU rounding, DaZ, "chop"

#define CHECK_FRAMELIMIT (Config.Options&PCSX2_FRAMELIMIT_MASK)

#define CHECK_VU0REC (!g_Session.ForceDisableVU0rec && Config.Options&PCSX2_VU0REC)
#define CHECK_VU1REC (!g_Session.ForceDisableVU1rec && (Config.Options&PCSX2_VU1REC))

// Memory Card configuration, per slot.
struct McdConfig
{
	// filename of the memory card for this slot.
	// If the string is empty characters long then the default is used.
	char Filename[g_MaxPath];
	
	// Enables the memory card at the emulation level.  When false, games will treat this
	// slot as if the memory card has been physically removed from the PS2.
	bool Enabled;
};

struct PcsxConfig
{
public:
	char Bios[g_MaxPath];
	char GS[g_MaxPath];
	char PAD1[g_MaxPath];
	char PAD2[g_MaxPath];
	char SPU2[g_MaxPath];
	char CDVD[g_MaxPath];
	char DEV9[g_MaxPath];
	char USB[g_MaxPath];
	char FW[g_MaxPath];
	char PluginsDir[g_MaxPath];
	char BiosDir[g_MaxPath];
	char Lang[g_MaxPath];

	McdConfig Mcd[2];
	
	bool McdEnableNTFS;		// enables NTFS compression on cards and the mcd folder.
	bool McdEnableEject;	// enables auto-ejection of cards after loading savestates.

	u32 Options; // PCSX2_X options

	bool PsxOut;
	bool Profiler; // Displays profiling info to console
	bool cdvdPrint; // Prints cdvd reads to console 
	bool closeGSonEsc; // closes the GS (and saves its state) on escape automatically.

	int PsxType;
	int Cdda;
	int Mdec;
	int Patch;
	int ThPriority;
	int CustomFps;
	int Hacks;
	int GameFixes;
	int CustomFrameSkip;
	int CustomConsecutiveFrames;
	int CustomConsecutiveSkip;
	u32 sseMXCSR;
	u32 sseVUMXCSR;
	u32 eeOptions;
	u32 vuOptions;
};

extern PcsxConfig Config;
extern u32 BiosVersion;
extern char CdromId[12];
extern uptr pDsp;		// what the hell is this unused piece of crap passed to every plugin for? (air)

int LoadCdrom();
int CheckCdrom();
int GetPS2ElfName(char*);

extern const char *LabelAuthors;
extern const char *LabelGreets;

void SaveGSState(const string& file);
void LoadGSState(const string& file);

char *ParseLang(char *id);
void ProcessFKeys(int fkey, int shift); // processes fkey related commands value 1-12

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

extern u32 g_sseVUMXCSR, g_sseMXCSR;

void SetCPUState(u32 sseMXCSR, u32 sseVUMXCSR);

// when using mmx/xmm regs, use; 0 is load
// freezes no matter the state
extern void FreezeXMMRegs_(int save);
extern void FreezeMMXRegs_(int save);
extern bool g_EEFreezeRegs;
extern u8 g_globalMMXSaved;
extern u8 g_globalXMMSaved;

// these macros check to see if needs freezing
#define FreezeXMMRegs(save) if( g_EEFreezeRegs ) { FreezeXMMRegs_(save); }
#define FreezeMMXRegs(save) if( g_EEFreezeRegs ) { FreezeMMXRegs_(save); }

#ifdef	_MSC_VER
#pragma pack()
#endif

void injectIRX(const char *filename);

extern void InitCPUTicks();
extern u64 GetTickFrequency();
extern u64 GetCPUTicks();


#endif /* __MISC_H__ */

