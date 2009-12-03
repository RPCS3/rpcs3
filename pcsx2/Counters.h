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

#ifndef __COUNTERS_H__
#define __COUNTERS_H__

struct EECNT_MODE
{
	// 0 - BUSCLK
	// 1 - 1/16th of BUSCLK
	// 2 - 1/256th of BUSCLK
	// 3 - External Clock (hblank!)
	u32 ClockSource:2;

	// Enables the counter gate (turns counter on/off as according to the
	// h/v blank type set in GateType).
	u32 EnableGate:1;

	// 0 - hblank!  1 - vblank!
	// Note: the hblank source type is disabled when ClockSel = 3
	u32 GateSource:1;

	// 0 - count when the gate signal is low
	// 1 - reset and start counting at the signal's rising edge (h/v blank end)
	// 2 - reset and start counting at the signal's falling edge (h/v blank start)
	// 3 - reset and start counting at both signal edges
	u32 GateMode:2;

	// Counter cleared to zero when target reached.
	// The PS2 only resets if the TargetInterrupt is enabled - Tested on PS2
	u32 ZeroReturn:1;

	// General count enable/status.  If 0, no counting happens.
	// This flag is set/unset by the gates.
	u32 IsCounting:1;

	// Enables target interrupts.
	u32 TargetInterrupt:1;

	// Enables overflow interrupts.
	u32 OverflowInterrupt:1;

	// Set to true by the counter when the target is reached.
	// Flag is set only when TargetInterrupt is enabled.
	u32 TargetReached:1;

	// Set to true by the counter when the target has overflowed.
	// Flag is set only when OverflowInterrupt is enabled.
	u32 OverflowReached:1;
};

// fixme: Cycle and sCycleT members are unused.
//	      But they can't be removed without making a new savestate version.
struct Counter
{
	u32 count;
	union
	{
		u32 modeval;		// the mode as a 32 bit int (for optimized combination masks)
		EECNT_MODE mode;
	};
	u32 target, hold;
	u32 rate, interrupt;
	u32 sCycleT;		// delta values should be signed.
};

struct SyncCounter
{
	u32 Mode;
	u32 sCycle;					// start cycle of timer
	s32 CycleT;
};

//------------------------------------------------------------------
// SPEED HACKS!!! (1 is normal) (They have inverse affects, only set 1 at a time)
//------------------------------------------------------------------
#define HBLANK_COUNTER_SPEED	1 //Set to '3' to double the speed of games like KHII
//#define HBLANK_TIMER_SLOWDOWN	1 //Set to '2' to increase the speed of games like God of War (FPS will be less, but game will be faster)

//------------------------------------------------------------------
// NTSC Timing Information!!! (some scanline info is guessed)
//------------------------------------------------------------------
#define FRAMERATE_NTSC			29.97 // frames per second

#define SCANLINES_TOTAL_NTSC	525 // total number of scanlines
#define SCANLINES_VSYNC_NTSC	3   // scanlines that are used for syncing every half-frame
#define SCANLINES_VRENDER_NTSC	240 // scanlines in a half-frame (because of interlacing)
#define SCANLINES_VBLANK1_NTSC	19  // scanlines used for vblank1 (even interlace)
#define SCANLINES_VBLANK2_NTSC	20  // scanlines used for vblank2 (odd interlace)

//------------------------------------------------------------------
// PAL Timing Information!!! (some scanline info is guessed)
//------------------------------------------------------------------
#define FRAMERATE_PAL			25.0// frames per second * 100 (25)

#define SCANLINES_TOTAL_PAL		625 // total number of scanlines per frame
#define SCANLINES_VSYNC_PAL		5   // scanlines that are used for syncing every half-frame
#define SCANLINES_VRENDER_PAL	288 // scanlines in a half-frame (because of interlacing)
#define SCANLINES_VBLANK1_PAL	19  // scanlines used for vblank1 (even interlace)
#define SCANLINES_VBLANK2_PAL	20  // scanlines used for vblank2 (odd interlace)

//------------------------------------------------------------------
// vSync and hBlank Timing Modes
//------------------------------------------------------------------
#define MODE_VRENDER	0x0		//Set during the Render/Frame Scanlines
#define MODE_VBLANK		0x1		//Set during the Blanking Scanlines
#define MODE_VSYNC		0x3		//Set during the Syncing Scanlines
#define MODE_VBLANK1	0x0		//Set during the Blanking Scanlines (half-frame 1)
#define MODE_VBLANK2	0x1		//Set during the Blanking Scanlines (half-frame 2)

#define MODE_HRENDER	0x0		//Set for ~5/6 of 1 Scanline
#define MODE_HBLANK		0x1		//Set for the remaining ~1/6 of 1 Scanline


extern Counter counters[4];
extern SyncCounter hsyncCounter;
extern SyncCounter vsyncCounter;

extern s32 nextCounter;		// delta until the next counter event (must be signed)
extern u32 nextsCounter;

extern void rcntUpdate_hScanline();
extern void rcntUpdate_vSync();
extern void rcntUpdate();

extern void rcntInit();
extern void rcntStartGate(bool mode, u32 sCycle);
extern void rcntEndGate(bool mode, u32 sCycle);
extern void rcntWcount(int index, u32 value);
extern void rcntWmode(int index, u32 value);
extern void rcntWtarget(int index, u32 value);
extern void rcntWhold(int index, u32 value);
extern u32	rcntRcount(int index);
extern u32	rcntCycle(int index);

extern u32 UpdateVSyncRate();
extern void frameLimitReset();

#endif /* __COUNTERS_H__ */
