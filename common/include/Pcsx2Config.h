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
 
#ifndef __PCSX2CONFIG_H__
#define __PCSX2CONFIG_H__

// Hack so that you can still use this file from C (not C++), or from a plugin without access to Paths.h.
// .. and removed in favor of a less hackish approach (air)

#ifndef g_MaxPath
#define g_MaxPath 255
#endif
 
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

#define PCSX2_MICROVU	// Use Micro VU recs instead of Zero VU Recs
#define PCSX2_MICROVU_	// Fully enable Micro VU recs (temporary option for now)
#define PCSX2_GSMULTITHREAD 1 // uses multi-threaded gs
#define PCSX2_EEREC 0x10
#define PCSX2_VU0REC 0x20
#define PCSX2_VU1REC 0x40
#define PCSX2_FRAMELIMIT_MASK 0xc00
#define PCSX2_FRAMELIMIT_NORMAL 0x000
#define PCSX2_FRAMELIMIT_LIMIT 0x400
#define PCSX2_FRAMELIMIT_SKIP 0x800

#define CHECK_FRAMELIMIT (Config.Options&PCSX2_FRAMELIMIT_MASK)

//------------ CPU Options!!! ---------------
#define CHECK_MULTIGS (Config.Options&PCSX2_GSMULTITHREAD)
#define CHECK_EEREC (!g_Session.ForceDisableEErec && Config.Options&PCSX2_EEREC)
#define CHECK_VU0REC (!g_Session.ForceDisableVU0rec && Config.Options&PCSX2_VU0REC)
#define CHECK_VU1REC (!g_Session.ForceDisableVU1rec && (Config.Options&PCSX2_VU1REC))

//------------ SPECIAL GAME FIXES!!! ---------------
#define CHECK_VUADDSUBHACK	 (Config.GameFixes & 0x1) // Special Fix for Tri-ace games, they use an encryption algorithm that requires VU addi opcode to be bit-accurate.
#define CHECK_FPUCOMPAREHACK (Config.GameFixes & 0x4) // Special Fix for Digimon Rumble Arena 2, fixes spinning/hanging on intro-menu.
#define CHECK_VUCLIPFLAGHACK (Config.GameFixes & 0x2) // Special Fix for Persona games, maybe others. It's to do with the VU clip flag (again).
#define CHECK_FPUMULHACK	 (Config.GameFixes & 0x8) // Special Fix for Tales of Destiny hangs.

//------------ Advanced Options!!! ---------------
#define CHECK_VU_OVERFLOW		 (Config.vuOptions & 0x1)
#define CHECK_VU_EXTRA_OVERFLOW	 (Config.vuOptions & 0x2) // If enabled, Operands are clamped before being used in the VU recs
#define CHECK_VU_SIGN_OVERFLOW	 (Config.vuOptions & 0x4)
#define CHECK_VU_UNDERFLOW		 (Config.vuOptions & 0x8)
#define CHECK_VU_EXTRA_FLAGS 0	// Always disabled now // Sets correct flags in the VU recs
#define CHECK_FPU_OVERFLOW		 (Config.eeOptions & 0x1)
#define CHECK_FPU_EXTRA_OVERFLOW (Config.eeOptions & 0x2) // If enabled, Operands are checked for infinities before being used in the FPU recs
#define CHECK_FPU_EXTRA_FLAGS 1	// Always enabled now // Sets D/I flags on FPU instructions
#define CHECK_FPU_FULL			 (Config.eeOptions & 0x4)
#define DEFAULT_eeOptions	0x01
#define DEFAULT_vuOptions	0x01

//------------ DEFAULT sseMXCSR VALUES!!! ---------------
#define DEFAULT_sseMXCSR	0xffc0 //FPU rounding > DaZ, FtZ, "chop"
#define DEFAULT_sseVUMXCSR	0xffc0 //VU  rounding > DaZ, FtZ, "chop"

//------------ Recompiler defines - Comment to disable a recompiler ---------------
// Yay!  These work now! (air) ... almost (air)

#define SHIFT_RECOMPILE // Speed majorly reduced if disabled
#define BRANCH_RECOMPILE // Speed extremely reduced if disabled - more then shift

// Disabling all the recompilers in this block is interesting, as it still runs at a reasonable rate.
// It also adds a few glitches. Really reminds me of the old Linux 64-bit version. --arcum42
#define ARITHMETICIMM_RECOMPILE
#define ARITHMETIC_RECOMPILE
#define MULTDIV_RECOMPILE
#define JUMP_RECOMPILE
#define LOADSTORE_RECOMPILE
#define MOVE_RECOMPILE
#define MMI_RECOMPILE
#define MMI0_RECOMPILE
#define MMI1_RECOMPILE
#define MMI2_RECOMPILE
#define MMI3_RECOMPILE
#define FPU_RECOMPILE
#define CP0_RECOMPILE
#define CP2_RECOMPILE

// You can't recompile ARITHMETICIMM without ARITHMETIC.
#ifndef ARITHMETIC_RECOMPILE
#undef ARITHMETICIMM_RECOMPILE
#endif

#define EE_CONST_PROP // rec2 - enables constant propagation (faster)

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
	char InisDir[g_MaxPath]; // This is intended for the program to populate, and the plugins to read. Obviously can't be saved in the config file. :)
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
	int CustomFps;
	struct Hacks_t {
		int EECycleRate;
		bool IOPCycleDouble;
		bool WaitCycleExt;
		bool INTCSTATSlow;
		int VUCycleSteal;
		bool IdleLoopFF;
		bool ESCExits; // this is a hack!?
	} Hacks;
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

#endif // __PCSX2CONFIG_H__
