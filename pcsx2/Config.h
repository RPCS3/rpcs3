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

class IniInterface;

enum PluginsEnum_t
{
	PluginId_GS = 0,
	PluginId_PAD,
	PluginId_SPU2,
	PluginId_CDVD,
	PluginId_USB,
	PluginId_FW,
	PluginId_DEV9,
	PluginId_Count,
	
	// Memorycard plugin support is preliminary, and is only hacked/hardcoded in at this
	// time.  So it's placed afer PluginId_Count so that it doesn't show up in the conf
	// screens or other plugin tables.

	PluginId_Mcd
};

// This macro is actually useful for about any and every possible application of C++
// equality operators.
#define OpEqu( field )		(field == right.field)

// Macro used for removing some of the redtape involved in defining bitfield/union helpers.
//
#define BITFIELD32()	\
	union {				\
		u32 bitset;		\
		struct {

#define BITFIELD_END	}; };

//------------ DEFAULT sseMXCSR VALUES ---------------
#define DEFAULT_sseMXCSR	0xffc0 //FPU rounding > DaZ, FtZ, "chop"
#define DEFAULT_sseVUMXCSR	0xffc0 //VU  rounding > DaZ, FtZ, "chop"

// --------------------------------------------------------------------------------------
//  Pcsx2Config class
// --------------------------------------------------------------------------------------
// This is intended to be a public class library between the core emulator and GUI only.
// It is *not* meant to be shared data between core emulation and plugins, due to issues
// with version incompatibilities if the structure formats are changed.
//
// When GUI code performs modifications of this class, it must be done with strict thread
// safety, since the emu runs on a separate thread.  Additionally many components of the
// class require special emu-side resets or state save/recovery to be applied.  Please
// use the provided functions to lock the emulation into a safe state and then apply
// chances on the necessary scope (see Core_Pause, Core_ApplySettings, and Core_Resume).
//
class Pcsx2Config
{
public:
	struct ProfilerOptions
	{
		BITFIELD32()
			bool
				Enabled:1,			// universal toggle for the profiler.
				RecBlocks_EE:1,		// Enables per-block profiling for the EE recompiler [unimplemented]
				RecBlocks_IOP:1,	// Enables per-block profiling for the IOP recompiler [unimplemented]
				RecBlocks_VU0:1,	// Enables per-block profiling for the VU0 recompiler [unimplemented]
				RecBlocks_VU1:1;	// Enables per-block profiling for the VU1 recompiler [unimplemented]
		BITFIELD_END

		// Default is Disabled, with all recs enabled underneath.
		ProfilerOptions() : bitset( 0xfffffffe ) {}
		void LoadSave( IniInterface& conf );

		bool operator ==( const ProfilerOptions& right ) const
		{
			return OpEqu( bitset );
		}

		bool operator !=( const ProfilerOptions& right ) const
		{
			return !OpEqu( bitset );
		}
	};

	// ------------------------------------------------------------------------
	struct RecompilerOptions
	{
		BITFIELD32()
			bool
				EnableEE:1,
				EnableIOP:1,
				EnableVU0:1,
				EnableVU1:1;

			bool
				UseMicroVU0:1,
				UseMicroVU1:1;
		BITFIELD_END

		// All recs are enabled by default.
		RecompilerOptions() : bitset( 0xffffffff ) { }
		void LoadSave( IniInterface& conf );

		bool operator ==( const RecompilerOptions& right ) const
		{
			return OpEqu( bitset );
		}

		bool operator !=( const RecompilerOptions& right ) const
		{
			return !OpEqu( bitset );
		}

	} Recompiler;

	// ------------------------------------------------------------------------
	struct CpuOptions
	{
		RecompilerOptions Recompiler;

		u32 sseMXCSR;
		u32 sseVUMXCSR;

		BITFIELD32()
			bool
				vuOverflow:1,
				vuExtraOverflow:1,
				vuSignOverflow:1,
				vuUnderflow:1;

			bool
				fpuOverflow:1,
				fpuExtraOverflow:1,
				fpuFullMode:1;
		BITFIELD_END

		CpuOptions();
		void LoadSave( IniInterface& conf );

		bool operator ==( const CpuOptions& right ) const
		{
			return
				OpEqu( sseMXCSR )	&& OpEqu( sseVUMXCSR )	&&
				OpEqu( bitset )		&& OpEqu( Recompiler );
		}

		bool operator !=( const CpuOptions& right ) const
		{
			return !this->operator ==( right );
		}
	};

	// ------------------------------------------------------------------------
	struct VideoOptions
	{
		bool EnableFrameLimiting;
		bool EnableFrameSkipping;

		// The region mode controls the default Maximum/Minimum FPS settings and also
		// regulates the vsync rates (which in turn control the IOP's SPU2 tick sync and ensure
		// proper audio playback speed).
		int DefaultRegionMode;	// 0=NTSC and 1=PAL

		int FpsTurbo;			// Limiting kicks in if fps goes beyond this (turbo enabled)
		int FpsLimit;			// Limiting kicks in if fps goes beyond this line
		int FpsSkip;			// Skipping kicks in if fps drops below this line
		int ConsecutiveFrames;	// number of consecutive frames (fields) to render
		int ConsecutiveSkip;	// number of consecutive frames (fields) to skip

		VideoOptions();
		void LoadSave( IniInterface& conf );
		
		bool operator ==( const VideoOptions& right ) const
		{
			return
				OpEqu( EnableFrameSkipping )	&&
				OpEqu( EnableFrameLimiting )	&&
				OpEqu( DefaultRegionMode )		&&
				OpEqu( FpsTurbo )				&&
				OpEqu( FpsLimit )				&&
				OpEqu( FpsSkip )				&&
				OpEqu( ConsecutiveFrames )		&&
				OpEqu( ConsecutiveSkip );
		}

		bool operator !=( const VideoOptions& right ) const
		{
			return !this->operator ==( right );
		}

	};

	// ------------------------------------------------------------------------
	// NOTE: The GUI's GameFixes panel is dependent on the order of bits in this structure.
	struct GamefixOptions
	{
		BITFIELD32()
			bool
				VuAddSubHack:1,		// Fix for Tri-ace games, they use an encryption algorithm that requires VU ADDI opcode to be bit-accurate.
				VuClipFlagHack:1,	// Fix for Persona games, maybe others. It's to do with the VU clip flag (again).
				FpuCompareHack:1,	// Fix for Digimon Rumble Arena 2, fixes spinning/hanging on intro-menu.
				FpuMulHack:1,		// Fix for Tales of Destiny hangs.
				DMAExeHack:1,       // Fix for Fatal Frame; breaks Gust and Tri-Ace games.
				XgKickHack:1,		// Fix for Erementar Gerad, adds more delay to VU XGkick instructions. Corrects the color of some graphics, but breaks Tri-ace games and others.
                MpegHack:1;         // Fix for Mana Khemia 1, breaks Digital Devil Saga.
		BITFIELD_END

		// all gamefixes are disabled by default.
		GamefixOptions() : bitset( 0 ) {}
		void LoadSave( IniInterface& conf );

		bool operator ==( const GamefixOptions& right ) const
		{
			return OpEqu( bitset );
		}

		bool operator !=( const GamefixOptions& right ) const
		{
			return !OpEqu( bitset );
		}
	};

	// ------------------------------------------------------------------------
	struct SpeedhackOptions
	{
		BITFIELD32()
			bool
				IopCycleRate_X2:1,	// enables the x2 multiplier of the IOP cyclerate
				IntcStat:1,			// tells Pcsx2 to fast-forward through intc_stat waits.
				BIFC0:1,			// enables BIFC0 detection and fast-forwarding
				vuFlagHack:1,		// microVU specific flag hack; Can cause Infinite loops, SPS, etc...
				vuMinMax:1;			// microVU specific MinMax hack; Can cause SPS, Black Screens,  etc...
		BITFIELD_END

		u8	EECycleRate;		// EE cycle rate selector (1.0, 1.5, 2.0)
		u8	VUCycleSteal;		// VU Cycle Stealer factor (0, 1, 2, or 3)

		SpeedhackOptions();
		void LoadSave( IniInterface& conf );

		bool operator ==( const SpeedhackOptions& right ) const
		{
			return OpEqu( bitset ) && OpEqu( EECycleRate ) && OpEqu( VUCycleSteal );
		}

		bool operator !=( const SpeedhackOptions& right ) const
		{
			return !this->operator ==( right );
		}
	};

public:

	BITFIELD32()
		bool
			CdvdVerboseReads:1,		// enables cdvd read activity verbosely dumped to the console
			CdvdDumpBlocks:1,		// enables cdvd block dumping
			EnablePatches:1,		// enables patch detection and application

		// when enabled performs bios stub execution, skipping full sony bios + splash screens
			SkipBiosSplash:1,

		// enables simulated ejection of memory cards when loading savestates
			McdEnableEjection:1,

			MultitapPort0_Enabled:1,
			MultitapPort1_Enabled:1;
	BITFIELD_END

	CpuOptions			Cpu;
	VideoOptions		Video;
	SpeedhackOptions	Speedhacks;
	GamefixOptions		Gamefixes;
	ProfilerOptions		Profiler;

	wxFileName			BiosFilename;

	Pcsx2Config();
	void LoadSave( IniInterface& ini );

	void Load( const wxString& srcfile );
	void Load( const wxInputStream& srcstream );
	void Save( const wxString& dstfile );
	void Save( const wxOutputStream& deststream );

	bool MultitapEnabled( uint port ) const
	{
		wxASSERT( port < 2 );
		return (port==0) ? MultitapPort0_Enabled : MultitapPort1_Enabled;
	}

	bool operator ==( const Pcsx2Config& right ) const
	{
		return
			OpEqu( bitset )		&&
			OpEqu( Cpu )		&&
			OpEqu( Video )		&&
			OpEqu( Speedhacks )	&&
			OpEqu( Gamefixes )	&&
			OpEqu( Profiler )	&&
			OpEqu( BiosFilename );
	}

	bool operator !=( const Pcsx2Config& right ) const
	{
		return !this->operator ==( right );
	}
};

//////////////////////////////////////////////////////////////////////////
// Session Configuration Override Flags
//
// a handful of flags that can override user configurations for the current application session
// only.  This allows us to do things like force-disable recompilers if the memory allocations
// for them fail.
struct SessionOverrideFlags
{
	bool
		ForceDisableEErec:1,
		ForceDisableIOPrec:1,
		ForceDisableVU0rec:1,
		ForceDisableVU1rec:1;
};

extern const Pcsx2Config EmuConfig;
extern SessionOverrideFlags g_Session;

/////////////////////////////////////////////////////////////////////////////////////////
// Helper Macros for Reading Emu Configurations.
//

// ------------ CPU / Recompiler Options ---------------

#define CHECK_MACROVU0				// If defined uses mVU for VU Macro (COP2), else uses sVU
#define CHECK_MICROVU0				(EmuConfig.Cpu.Recompiler.UseMicroVU0)
#define CHECK_MICROVU1				(EmuConfig.Cpu.Recompiler.UseMicroVU1)
#define CHECK_EEREC					(!g_Session.ForceDisableEErec && EmuConfig.Cpu.Recompiler.EnableEE)
#define CHECK_IOPREC				(!g_Session.ForceDisableIOPrec && EmuConfig.Cpu.Recompiler.EnableIOP)
#define CHECK_VU0REC				(!g_Session.ForceDisableVU0rec && EmuConfig.Cpu.Recompiler.EnableVU0)
#define CHECK_VU1REC				(!g_Session.ForceDisableVU1rec && EmuConfig.Cpu.Recompiler.EnableVU1)

//------------ SPECIAL GAME FIXES!!! ---------------
#define NUM_OF_GAME_FIXES 7

#define CHECK_VUADDSUBHACK			(EmuConfig.Gamefixes.VuAddSubHack) // Special Fix for Tri-ace games, they use an encryption algorithm that requires VU addi opcode to be bit-accurate.
#define CHECK_FPUCOMPAREHACK		(EmuConfig.Gamefixes.FpuCompareHack) // Special Fix for Digimon Rumble Arena 2, fixes spinning/hanging on intro-menu.
#define CHECK_VUCLIPFLAGHACK		(EmuConfig.Gamefixes.VuClipFlagHack) // Special Fix for Persona games, maybe others. It's to do with the VU clip flag (again).
#define CHECK_FPUMULHACK			(EmuConfig.Gamefixes.FpuMulHack) // Special Fix for Tales of Destiny hangs.
#define CHECK_DMAEXECHACK			(EmuConfig.Gamefixes.DMAExeHack)  // Special Fix for Fatal Frame; breaks Gust and Tri-Ace games.
#define CHECK_XGKICKHACK			(EmuConfig.Gamefixes.XgKickHack) // Special Fix for Erementar Gerad, adds more delay to VU XGkick instructions. Corrects the color of some graphics.
#define CHECK_MPEGHACK			    (EmuConfig.Gamefixes.MpegHack) // Special Fix for Mana Khemia 1; breaks Digital Devil Saga.

//------------ Advanced Options!!! ---------------
#define CHECK_VU_OVERFLOW			(EmuConfig.Cpu.vuOverflow)
#define CHECK_VU_EXTRA_OVERFLOW		(EmuConfig.Cpu.vuExtraOverflow) // If enabled, Operands are clamped before being used in the VU recs
#define CHECK_VU_SIGN_OVERFLOW		(EmuConfig.Cpu.vuSignOverflow)
#define CHECK_VU_UNDERFLOW			(EmuConfig.Cpu.vuUnderflow)
#define CHECK_VU_EXTRA_FLAGS		0	// Always disabled now // Sets correct flags in the sVU recs

#define CHECK_FPU_OVERFLOW			(EmuConfig.Cpu.fpuOverflow)
#define CHECK_FPU_EXTRA_OVERFLOW	(EmuConfig.Cpu.fpuExtraOverflow) // If enabled, Operands are checked for infinities before being used in the FPU recs
#define CHECK_FPU_EXTRA_FLAGS		1	// Always enabled now // Sets D/I flags on FPU instructions
#define CHECK_FPU_FULL				(EmuConfig.Cpu.fpuFullMode)

//------------ EE Recompiler defines - Comment to disable a recompiler ---------------

#define SHIFT_RECOMPILE		// Speed majorly reduced if disabled
#define BRANCH_RECOMPILE	// Speed extremely reduced if disabled - more then shift

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
