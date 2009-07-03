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

#include "PrecompiledHeader.h"

#include <time.h>
#include <cmath>
#include "Common.h"
#include "Counters.h"

#include "R3000A.h"
#include "IopCounters.h"

#include "GS.h"
#include "VUmicro.h"

using namespace Threading;

extern u8 psxhblankgate;

static const uint EECNT_FUTURE_TARGET = 0x10000000;

u64 profile_starttick = 0;
u64 profile_totalticks = 0;

int gates = 0;

// Counter 4 takes care of scanlines - hSync/hBlanks
// Counter 5 takes care of vSync/vBlanks
Counter counters[4];
SyncCounter hsyncCounter;
SyncCounter vsyncCounter;

u32 nextsCounter;	// records the cpuRegs.cycle value of the last call to rcntUpdate()
s32 nextCounter;	// delta from nextsCounter, in cycles, until the next rcntUpdate() 

void rcntReset(int index) {
	counters[index].count = 0;
	counters[index].sCycleT = cpuRegs.cycle;
}

// Updates the state of the nextCounter value (if needed) to serve
// any pending events for the given counter.
// Call this method after any modifications to the state of a counter.
static __forceinline void _rcntSet( int cntidx )
{
	s32 c;
	jASSUME( cntidx <= 4 );		// rcntSet isn't valid for h/vsync counters.

	const Counter& counter = counters[cntidx];

	// Stopped or special hsync gate?
	if (!counter.mode.IsCounting || (counter.mode.ClockSource == 0x3) ) return;
	
	// check for special cases where the overflow or target has just passed
	// (we probably missed it because we're doing/checking other things)
	if( counter.count > 0x10000 || counter.count > counter.target )
	{
		nextCounter = 4;
		return;
	}

	// nextCounter is relative to the cpuRegs.cycle when rcntUpdate() was last called.
	// However, the current _rcntSet could be called at any cycle count, so we need to take
	// that into account.  Adding the difference from that cycle count to the current one
	// will do the trick!

	c = ((0x10000 - counter.count) * counter.rate) - (cpuRegs.cycle - counter.sCycleT);
	c += cpuRegs.cycle - nextsCounter;		// adjust for time passed since last rcntUpdate();
	if (c < nextCounter) 
	{
		nextCounter = c;
		cpuSetNextBranch( nextsCounter, nextCounter );	//Need to update on counter resets/target changes
	}
	
	// Ignore target diff if target is currently disabled.
	// (the overflow is all we care about since it goes first, and then the 
	// target will be turned on afterward, and handled in the next event test).

	if( counter.target & EECNT_FUTURE_TARGET ) 
	{
		return;
	}
	else
	{
		c = ((counter.target - counter.count) * counter.rate) - (cpuRegs.cycle - counter.sCycleT);
		c += cpuRegs.cycle - nextsCounter;		// adjust for time passed since last rcntUpdate();
		if (c < nextCounter) 
		{
			nextCounter = c;
			cpuSetNextBranch( nextsCounter, nextCounter );	//Need to update on counter resets/target changes
		}	
	}
}


static __forceinline void cpuRcntSet()
{
	int i;

	nextsCounter = cpuRegs.cycle;
	nextCounter = vsyncCounter.CycleT - (cpuRegs.cycle - vsyncCounter.sCycle);

	for (i = 0; i < 4; i++)
		_rcntSet( i );

	// sanity check!
	if( nextCounter < 0 ) nextCounter = 0;
}

void rcntInit() {
	int i;

	memzero_obj(counters);

	for (i=0; i<4; i++) {
		counters[i].rate = 2;
		counters[i].target = 0xffff;
	}
	counters[0].interrupt =  9;
	counters[1].interrupt = 10;
	counters[2].interrupt = 11;
	counters[3].interrupt = 12;

	hsyncCounter.Mode = MODE_HRENDER;
	hsyncCounter.sCycle = cpuRegs.cycle;
	vsyncCounter.Mode = MODE_VRENDER; 
	vsyncCounter.sCycle = cpuRegs.cycle;

	UpdateVSyncRate();

	for (i=0; i<4; i++) rcntReset(i);
	cpuRcntSet();
}

// debug code, used for stats
int g_nhsyncCounter;
static uint iFrame = 0;	

#ifndef _WIN32
#include <sys/time.h>
#endif

static s64 m_iTicks=0;
static u64 m_iStart=0;

struct vSyncTimingInfo
{
	u32 Framerate;			// frames per second * 100 (so 2500 for PAL and 2997 for NTSC)
	u32 Render;				// time from vblank end to vblank start (cycles)
	u32 Blank;				// time from vblank start to vblank end (cycles)

	u32 hSyncError;			// rounding error after the duration of a rendered frame (cycles)
	u32 hRender;			// time from hblank end to hblank start (cycles)
	u32 hBlank;				// time from hblank start to hblank end (cycles)
	u32 hScanlinesPerFrame;	// number of scanlines per frame (525/625 for NTSC/PAL)
};


static vSyncTimingInfo vSyncInfo;


static void vSyncInfoCalc( vSyncTimingInfo* info, u32 framesPerSecond, u32 scansPerFrame )
{
	// Important: Cannot use floats or doubles here.  The emulator changes rounding modes
	// depending on user-set speedhack options, and it can break float/double code
	// (as in returning infinities and junk)

	// NOTE: mgs3 likes a /4 vsync, but many games prefer /2.  This seems to indicate a
	// problem in the counters vsync gates somewhere.

	u64 Frame = ((u64)PS2CLK * 1000000ULL) / framesPerSecond;
	u64 HalfFrame = Frame / 2;
	u64 Blank = HalfFrame / 2;		// two blanks and renders per frame
	u64 Render = HalfFrame - Blank;	// so use the half-frame value for these...

	// Important!  The hRender/hBlank timers should be 50/50 for best results.
	// In theory a 70%/30% ratio would be more correct but in practice it runs
	// like crap and totally screws audio synchronization and other things.
	
	u64 Scanline = Frame / scansPerFrame;
	u64 hBlank = Scanline / 2;
	u64 hRender = Scanline - hBlank;
	
	info->Framerate = framesPerSecond;
	info->Render = (u32)(Render/10000);
	info->Blank  = (u32)(Blank/10000);

	info->hRender = (u32)(hRender/10000);
	info->hBlank  = (u32)(hBlank/10000);
	info->hScanlinesPerFrame = scansPerFrame;
	
	// Apply rounding:
	if( ( Render - info->Render ) >= 5000 ) info->Render++;
	else if( ( Blank - info->Blank ) >= 5000 ) info->Blank++;

	if( ( hRender - info->hRender ) >= 5000 ) info->hRender++;
	else if( ( hBlank - info->hBlank ) >= 5000 ) info->hBlank++;
	
	// Calculate accumulative hSync rounding error per half-frame:
	{
	u32 hSyncCycles = ((info->hRender + info->hBlank) * scansPerFrame) / 2;
	u32 vSyncCycles = (info->Render + info->Blank);
	info->hSyncError = vSyncCycles - hSyncCycles;
	}

	// Note: In NTSC modes there is some small rounding error in the vsync too,
	// however it would take thousands of frames for it to amount to anything and
	// is thus not worth the effort at this time.
}


u32 UpdateVSyncRate()
{
	const char *limiterMsg = "Framelimiter rate updated (UpdateVSyncRate): %d.%d fps";

	// fixme - According to some docs, progressive-scan modes actually refresh slower than
	// interlaced modes.  But I can't fathom how, since the refresh rate is a function of
	// the television and all the docs I found on TVs made no indication that they ever
	// run anything except their native refresh rate.

	//#define VBLANK_NTSC			((Config.PsxType & 2) ? 59.94 : 59.82) //59.94 is more precise
	//#define VBLANK_PAL			((Config.PsxType & 2) ? 50.00 : 49.76)

	if(Config.PsxType & 1)
	{
		if( vSyncInfo.Framerate != FRAMERATE_PAL )
			vSyncInfoCalc( &vSyncInfo, FRAMERATE_PAL, SCANLINES_TOTAL_PAL );
	}
	else
	{
		if( vSyncInfo.Framerate != FRAMERATE_NTSC )
			vSyncInfoCalc( &vSyncInfo, FRAMERATE_NTSC, SCANLINES_TOTAL_NTSC );
	}

	hsyncCounter.CycleT = vSyncInfo.hRender; // Amount of cycles before the counter will be updated
	vsyncCounter.CycleT = vSyncInfo.Render; // Amount of cycles before the counter will be updated

	if (Config.CustomFps > 0)
	{
		s64 ticks = GetTickFrequency() / Config.CustomFps;
		if( m_iTicks != ticks )
		{
			m_iTicks = ticks;
			gsOnModeChanged( vSyncInfo.Framerate, m_iTicks );
			Console::Status( limiterMsg, params Config.CustomFps, 0 );
		}
	}
	else
	{
		s64 ticks = (GetTickFrequency() * 50) / vSyncInfo.Framerate;
		if( m_iTicks != ticks )
		{
			m_iTicks = ticks;
			gsOnModeChanged( vSyncInfo.Framerate, m_iTicks );
			Console::Status( limiterMsg, params vSyncInfo.Framerate/50, (vSyncInfo.Framerate*2)%100 );
		}
	}

	m_iStart = GetCPUTicks();
	cpuRcntSet();

	return (u32)m_iTicks;
}

void frameLimitReset()
{
	m_iStart = GetCPUTicks();
}

// Framelimiter - Measures the delta time between calls and stalls until a
// certain amount of time passes if such time hasn't passed yet.
// See the GS FrameSkip function for details on why this is here and not in the GS.
static __forceinline void frameLimit()
{
	if( CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_NORMAL ) return;
	if( Config.CustomFps >= 999 ) return;	// means the user would rather just have framelimiting turned off...
	
	s64 sDeltaTime;
	u64 uExpectedEnd;
	u64 iEnd;

	uExpectedEnd = m_iStart + m_iTicks;
	iEnd = GetCPUTicks();

	sDeltaTime = iEnd - uExpectedEnd;

	// If the framerate drops too low, reset the expected value.  This avoids
	// excessive amounts of "fast forward" syndrome which would occur if we
	// tried to catch up too much.
	
	if( sDeltaTime > m_iTicks*8 )
	{
		m_iStart = iEnd - m_iTicks;

		// Let the GS Skipper know we lost time.
		// Keeps the GS skipper from trying to catch up to a framerate
		// that the limiter already gave up on.

		gsSyncLimiterLostTime( (s32)(m_iStart - uExpectedEnd) );
		return;
	}

	// use the expected frame completion time as our starting point.
	// improves smoothness by making the framelimiter more adaptive to the
	// imperfect TIMESLICE() wait, and allows it to speed up a wee bit after
	// slow frames to "catch up."

	m_iStart = uExpectedEnd;

	while( sDeltaTime < 0 )
	{
		Timeslice();
		iEnd = GetCPUTicks();
		sDeltaTime = iEnd - uExpectedEnd;
	}
}

static __forceinline void VSyncStart(u32 sCycle)
{
	EECNT_LOG( "/////////  EE COUNTER VSYNC START  \\\\\\\\\\\\\\\\\\\\  (frame: %d)", iFrame );
	vSyncDebugStuff( iFrame ); // EE Profiling and Debug code

	if ((CSRw & 0x8)) 
	{
		
		
		if (!(GSIMR&0x800)) 
		{
			gsIrq();
		}
		GSCSRr|= 0x8;
	}

	hwIntcIrq(INTC_VBLANK_S);
	psxVBlankStart();

	if (gates) rcntStartGate(true, sCycle); // Counters Start Gate code
	if (Config.Patch) applypatch(1); // Apply patches (ToDo: clean up patch code)

	// INTC - VB Blank Start Hack --
	// Hack fix!  This corrects a freezeup in Granda 2 where it decides to spin
	// on the INTC_STAT register after the exception handler has already cleared
	// it.  But be warned!  Set the value to larger than 4 and it breaks Dark
	// Cloud and other games. -_-

	// How it works: Normally the INTC raises exceptions immediately at the end of the
	// current branch test.  But in the case of Grandia 2, the game's code is spinning
	// on the INTC status, and the exception handler (for some reason?) clears the INTC
	// before returning *and* returns to a location other than EPC.  So the game never
	// gets to the point where it sees the INTC Irq set true.

	// (I haven't investigated why Dark Cloud freezes on larger values)
	// (all testing done using the recompiler -- dunno how the ints respond yet)

	//cpuRegs.eCycle[30] = 2;

	// Should no longer be required (Refraction)
}

static __forceinline void VSyncEnd(u32 sCycle)
{
	EECNT_LOG( "/////////  EE COUNTER VSYNC END  \\\\\\\\\\\\\\\\\\\\  (frame: %d)", iFrame );

	iFrame++;

	gsPostVsyncEnd( true );

	hwIntcIrq(INTC_VBLANK_E);  // HW Irq
	psxVBlankEnd(); // psxCounters vBlank End
	if (gates) rcntEndGate(true, sCycle); // Counters End Gate Code
	frameLimit(); // limit FPS

	// This doesn't seem to be needed here.  Games only seem to break with regard to the
	// vsyncstart irq. 
	//cpuRegs.eCycle[30] = 2;
}

//#define VSYNC_DEBUG		// Uncomment this to enable some vSync Timer debugging features.
#ifdef VSYNC_DEBUG
static u32 hsc=0;
static int vblankinc = 0;
#endif

__forceinline void rcntUpdate_hScanline()
{
	if( !cpuTestCycle( hsyncCounter.sCycle, hsyncCounter.CycleT ) ) return;

	//iopBranchAction = 1;
	if (hsyncCounter.Mode & MODE_HBLANK) { //HBLANK Start
		rcntStartGate(false, hsyncCounter.sCycle);
		psxCheckStartGate16(0);
		
		// Setup the hRender's start and end cycle information:
		hsyncCounter.sCycle += vSyncInfo.hBlank;		// start  (absolute cycle value)
		hsyncCounter.CycleT = vSyncInfo.hRender;		// endpoint (delta from start value)
		hsyncCounter.Mode = MODE_HRENDER;
	}
	else { //HBLANK END / HRENDER Begin
		if (CSRw & 0x4) 
		{
			
			
			if (!(GSIMR&0x400)) 
			{
				gsIrq();
			}
			GSCSRr |= 4; // signal
		}
		if (gates) rcntEndGate(false, hsyncCounter.sCycle);
		if (psxhblankgate) psxCheckEndGate16(0);

		// set up the hblank's start and end cycle information:
		hsyncCounter.sCycle += vSyncInfo.hRender;	// start (absolute cycle value)
		hsyncCounter.CycleT = vSyncInfo.hBlank;		// endpoint (delta from start value)
		hsyncCounter.Mode = MODE_HBLANK;

#		ifdef VSYNC_DEBUG
		hsc++;
#		endif
	}
}

__forceinline bool rcntUpdate_vSync()
{
	s32 diff = (cpuRegs.cycle - vsyncCounter.sCycle);
	if( diff < vsyncCounter.CycleT ) return false;

	//iopBranchAction = 1;
	if (vsyncCounter.Mode == MODE_VSYNC)
	{
		VSyncEnd(vsyncCounter.sCycle);

		vsyncCounter.sCycle += vSyncInfo.Blank;
		vsyncCounter.CycleT = vSyncInfo.Render;
		vsyncCounter.Mode = MODE_VRENDER;

		return true;
	}
	else	// VSYNC end / VRENDER begin
	{
		VSyncStart(vsyncCounter.sCycle);

		vsyncCounter.sCycle += vSyncInfo.Render;
		vsyncCounter.CycleT = vSyncInfo.Blank;
		vsyncCounter.Mode = MODE_VSYNC;

		// Accumulate hsync rounding errors:
		hsyncCounter.sCycle += vSyncInfo.hSyncError;
	
		if (CHECK_MICROVU0) vsyncVUrec(0);
		if (CHECK_MICROVU1) vsyncVUrec(1);

#		ifdef VSYNC_DEBUG
		vblankinc++;
		if( vblankinc > 1 )
		{
			if( hsc != vSyncInfo.hScanlinesPerFrame )
				Console::WriteLn( " ** vSync > Abnormal Scanline Count: %d", params hsc );
			hsc = 0;
			vblankinc = 0;
		}
#		endif
	}
	return false;
}

static __forceinline void _cpuTestTarget( int i )
{
	if (counters[i].count < counters[i].target) return;

	if(counters[i].mode.TargetInterrupt) {

		EECNT_LOG("EE Counter[%d] TARGET reached - mode=%x, count=%x, target=%x", i, counters[i].mode, counters[i].count, counters[i].target);
		counters[i].mode.TargetReached = 1;
		hwIntcIrq(counters[i].interrupt);

		// The PS2 only resets if the interrupt is enabled - Tested on PS2
		if (counters[i].mode.ZeroReturn)
			counters[i].count -= counters[i].target; // Reset on target
		else
			counters[i].target |= EECNT_FUTURE_TARGET;
	} 
	else counters[i].target |= EECNT_FUTURE_TARGET;
}

static __forceinline void _cpuTestOverflow( int i )
{
	if (counters[i].count <= 0xffff) return;
	
	if (counters[i].mode.OverflowInterrupt) {
		EECNT_LOG("EE Counter[%d] OVERFLOW - mode=%x, count=%x", i, counters[i].mode, counters[i].count);
		counters[i].mode.OverflowReached = 1;
		hwIntcIrq(counters[i].interrupt);
	}
	
	// wrap counter back around zero, and enable the future target:
	counters[i].count -= 0x10000;
	counters[i].target &= 0xffff;
}


// forceinline note: this method is called from two locations, but one
// of them is the interpreter, which doesn't count. ;)  So might as
// well forceinline it!
__forceinline bool rcntUpdate()
{
	bool retval = rcntUpdate_vSync();

	// Update counters so that we can perform overflow and target tests.
	
	for (int i=0; i<=3; i++) {
		
		// We want to count gated counters (except the hblank which exclude below, and are
		// counted by the hblank timer instead)

		//if ( gates & (1<<i) ) continue;
		
		if (!counters[i].mode.IsCounting ) continue;

		if(counters[i].mode.ClockSource != 0x3)	// don't count hblank sources
		{
			s32 change = cpuRegs.cycle - counters[i].sCycleT;
			if( change < 0 ) change = 0;	// sanity check!

			counters[i].count += change / counters[i].rate;
			change -= (change / counters[i].rate) * counters[i].rate;
			counters[i].sCycleT = cpuRegs.cycle - change;

			// Check Counter Targets and Overflows:
			_cpuTestTarget( i );
			_cpuTestOverflow( i );
		} 
		else counters[i].sCycleT = cpuRegs.cycle;
	}

	cpuRcntSet();
	return retval;
}

static __forceinline void _rcntSetGate( int index )
{
	if (counters[index].mode.EnableGate)
	{
		// If the Gate Source is hblank and the clock selection is also hblank
		// then the gate is disabled and the counter acts as a normal hblank source.

		if( !(counters[index].mode.GateSource == 0 && counters[index].mode.ClockSource == 3) )
		{
			EECNT_LOG( "EE Counter[%d] Using Gate!  Source=%s, Mode=%d.",
				index, counters[index].mode.GateSource ? "vblank" : "hblank", counters[index].mode.GateMode );

			gates |= (1<<index);
			counters[index].mode.IsCounting = 0;
			rcntReset(index);
			return;
		}
		else
			EECNT_LOG( "EE Counter[%d] GATE DISABLED because of hblank source.", index );
	}

	gates &= ~(1<<index);
}

// mode - 0 means hblank source, 8 means vblank source.
__forceinline void rcntStartGate(bool isVblank, u32 sCycle)
{
	int i;

	for (i=0; i <=3; i++) {

		//if ((mode == 0) && ((counters[i].mode & 0x83) == 0x83))
		if (!isVblank && counters[i].mode.IsCounting && (counters[i].mode.ClockSource == 3) )
		{
			// Update counters using the hblank as the clock.  This keeps the hblank source
			// nicely in sync with the counters and serves as an optimization also, since these
			// counter won't recieve special rcntUpdate scheduling.

			// Note: Target and overflow tests must be done here since they won't be done
			// currectly by rcntUpdate (since it's not being scheduled for these counters)

			counters[i].count += HBLANK_COUNTER_SPEED;
			_cpuTestTarget( i );
			_cpuTestOverflow( i );
		}

		if (!(gates & (1<<i))) continue;
		if ((!!counters[i].mode.GateSource) != isVblank) continue;

		switch (counters[i].mode.GateMode) {
			case 0x0: //Count When Signal is low (off)
			
				// Just set the start cycle (sCycleT) -- counting will be done as needed
				// for events (overflows, targets, mode changes, and the gate off below)
			
				counters[i].mode.IsCounting = 1;
				counters[i].sCycleT = sCycle;
				EECNT_LOG("EE Counter[%d] %s StartGate Type0, count = %x",
					isVblank ? "vblank" : "hblank", i, counters[i].count );
				break;
				
			case 0x2:	// reset and start counting on vsync end
				// this is the vsync start so do nothing.
				break;
				
			case 0x1: //Reset and start counting on Vsync start
			case 0x3: //Reset and start counting on Vsync start and end
				counters[i].mode.IsCounting = 1;
				counters[i].count = 0;
				counters[i].target &= 0xffff;
				counters[i].sCycleT = sCycle;
				EECNT_LOG("EE Counter[%d] %s StartGate Type%d, count = %x",
					isVblank ? "vblank" : "hblank", i, counters[i].mode.GateMode, counters[i].count );
				break;
		}
	}

	// No need to update actual counts here.  Counts are calculated as needed by reads to
	// rcntRcount().  And so long as sCycleT is set properly, any targets or overflows
	// will be scheduled and handled.

	// Note: No need to set counters here.  They'll get set when control returns to
	// rcntUpdate, since we're being called from there anyway.
}

// mode - 0 means hblank signal, 8 means vblank signal.
__forceinline void rcntEndGate(bool isVblank , u32 sCycle)
{
	int i;

	for(i=0; i <=3; i++) { //Gates for counters
		if (!(gates & (1<<i))) continue;
		if ((!!counters[i].mode.GateSource) != isVblank) continue;

		switch (counters[i].mode.GateMode) {
			case 0x0: //Count When Signal is low (off)

				// Set the count here.  Since the timer is being turned off it's
				// important to record its count at this point (it won't be counted by
				// calls to rcntUpdate).

				counters[i].count = rcntRcount(i);
				counters[i].mode.IsCounting = 0;
				counters[i].sCycleT = sCycle;
				EECNT_LOG("EE Counter[%d] %s EndGate Type0, count = %x",
					isVblank ? "vblank" : "hblank", i, counters[i].count );
			break;

			case 0x1:	// Reset and start counting on Vsync start
				// this is the vsync end so do nothing
			break;

			case 0x2: //Reset and start counting on Vsync end
			case 0x3: //Reset and start counting on Vsync start and end
				counters[i].mode.IsCounting = 1;
				counters[i].count = 0;
				counters[i].target &= 0xffff;
				counters[i].sCycleT = sCycle;
				EECNT_LOG("EE Counter[%d] %s EndGate Type%d, count = %x",
					isVblank ? "vblank" : "hblank", i, counters[i].mode.GateMode, counters[i].count );
			break;
		}
	}
	// Note: No need to set counters here.  They'll get set when control returns to
	// rcntUpdate, since we're being called from there anyway.
}

__forceinline void rcntWmode(int index, u32 value)  
{
	if(counters[index].mode.IsCounting) {
		if(counters[index].mode.ClockSource != 0x3) {

			u32 change = cpuRegs.cycle - counters[index].sCycleT;
			if( change > 0 )
			{
				counters[index].count += change / counters[index].rate;
				change -= (change / counters[index].rate) * counters[index].rate;
				counters[index].sCycleT = cpuRegs.cycle - change;
			}
		}
	}
	else counters[index].sCycleT = cpuRegs.cycle;

	counters[index].modeval &= ~(value & 0xc00); //Clear status flags, the ps2 only clears what is given in the value
	counters[index].modeval = (counters[index].modeval & 0xc00) | (value & 0x3ff);
	EECNT_LOG("EE Counter[%d] writeMode = %x   passed value=%x", index, counters[index].modeval, value );

	switch (counters[index].mode.ClockSource) { //Clock rate divisers *2, they use BUSCLK speed not PS2CLK
		case 0: counters[index].rate = 2; break;
		case 1: counters[index].rate = 32; break;
		case 2: counters[index].rate = 512; break;
		case 3: counters[index].rate = vSyncInfo.hBlank+vSyncInfo.hRender; break;
	}
	
	_rcntSetGate( index );
	_rcntSet( index );
}

__forceinline void rcntWcount(int index, u32 value) 
{
	EECNT_LOG("EE Counter[%d] writeCount = %x,   oldcount=%x, target=%x", index, value, counters[index].count, counters[index].target );

	counters[index].count = value & 0xffff;
	
	// reset the target, and make sure we don't get a premature target.
	counters[index].target &= 0xffff;
	if( counters[index].count > counters[index].target )
		counters[index].target |= EECNT_FUTURE_TARGET;

	// re-calculate the start cycle of the counter based on elapsed time since the last counter update:
	if(counters[index].mode.IsCounting) {
		if(counters[index].mode.ClockSource != 0x3) {
			s32 change = cpuRegs.cycle - counters[index].sCycleT;
			if( change > 0 ) {
				change -= (change / counters[index].rate) * counters[index].rate;
				counters[index].sCycleT = cpuRegs.cycle - change;
			}
		}
	} 
	else counters[index].sCycleT = cpuRegs.cycle;

	_rcntSet( index );
}

__forceinline void rcntWtarget(int index, u32 value)
{
	EECNT_LOG("EE Counter[%d] writeTarget = %x", index, value);

	counters[index].target = value & 0xffff;

	// guard against premature (instant) targeting.
	// If the target is behind the current count, set it up so that the counter must
	// overflow first before the target fires:

	if(counters[index].mode.IsCounting) {
		if(counters[index].mode.ClockSource != 0x3) {

			u32 change = cpuRegs.cycle - counters[index].sCycleT;
			if( change > 0 )
			{
				counters[index].count += change / counters[index].rate;
				change -= (change / counters[index].rate) * counters[index].rate;
				counters[index].sCycleT = cpuRegs.cycle - change;
			}
		}
	}

	if( counters[index].target <= rcntCycle(index) )
		counters[index].target |= EECNT_FUTURE_TARGET;

	_rcntSet( index );
}

__forceinline void rcntWhold(int index, u32 value)
{
	EECNT_LOG("EE Counter[%d] Hold Write = %x", index, value);
	counters[index].hold = value;
}

__forceinline u32 rcntRcount(int index)
{
	u32 ret;

	// only count if the counter is turned on (0x80) and is not an hsync gate (!0x03)
	if (counters[index].mode.IsCounting && (counters[index].mode.ClockSource != 0x3)) 
		ret = counters[index].count + ((cpuRegs.cycle - counters[index].sCycleT) / counters[index].rate);
	else 
		ret = counters[index].count;

	// Spams the Console.
	EECNT_LOG("EE Counter[%d] readCount32 = %x", index, ret);
	return ret;
}

__forceinline u32 rcntCycle(int index)
{
	if (counters[index].mode.IsCounting && (counters[index].mode.ClockSource != 0x3)) 
		return counters[index].count + ((cpuRegs.cycle - counters[index].sCycleT) / counters[index].rate);
	else 
		return counters[index].count;
}

void SaveState::rcntFreeze()
{
	Freeze( counters );
	Freeze( hsyncCounter );
	Freeze( vsyncCounter );
	Freeze( nextCounter );
	Freeze( nextsCounter );
	Freeze( Config.PsxType );

	if( IsLoading() )
	{
		UpdateVSyncRate();

		// make sure the gate flags are set based on the counter modes...
		for( int i=0; i<4; i++ )
			_rcntSetGate( i );

		iopBranchAction = 1;	// probably not needed but won't hurt anything either.
	}
}
