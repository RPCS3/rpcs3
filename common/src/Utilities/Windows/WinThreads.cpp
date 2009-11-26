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
 
#include "PrecompiledHeader.h"
#include "RedtapeWindows.h"
#include "x86emitter/tools.h"
#include "Threading.h"

#ifndef __WXMSW__

#	pragma message( "WinThreads.cpp should only be compiled by projects or makefiles targeted at Microsoft Windows.")

#else

__forceinline void Threading::Sleep( int ms )
{
	::Sleep( ms );
}

// For use in spin/wait loops,  Acts as a hint to Intel CPUs and should, in theory
// improve performance and reduce cpu power consumption.
__forceinline void Threading::SpinWait()
{
	__asm pause;
}

__forceinline void Threading::EnableHiresScheduler()
{
	// This improves accuracy of Sleep() by some amount, and only adds a negligible amount of
	// overhead on modern CPUs.  Typically desktops are already set pretty low, but laptops in
	// particular may have a scheduler Period of 15 or 20ms to extend battery life.
	
	// (note: this same trick is used by most multimedia software and games)

	timeBeginPeriod( 1 );
}

__forceinline void Threading::DisableHiresScheduler()
{
	timeEndPeriod( 1 );
}

void pxYieldToMain()
{
	// Windows Implementation Note:
	//  wxWidgets has a wxEventLoop::Pending() function, however it uses PeekMessage internally
	//  which is a multithreaded NO-NO.  So instead we must use GetQueueStatus.  This is ok
	//  anyway since I get extra fancy and scale the sleep duration based on the type of messages
	//  in the queue.  User input (key and mouse button) cue longer worker thread yields since
	//  maintaining a responsive gui is typically a high priority.
	
	DWORD result = GetQueueStatus( QS_ALLEVENTS );
	uint hiword = HIWORD( result );

	if( hiword == 0 ) return;
	
	int sleepdur = 1;
	if( (hiword & (QS_MOUSEBUTTON | QS_KEY)) != 0 )
		sleepdur += 3;

	Sleep( sleepdur );
}

#endif

