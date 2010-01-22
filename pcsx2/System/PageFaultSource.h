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

// =====================================================================================================
//  Cross-Platform Memory Protection (Used by VTLB, Recompilers and Texture caches)
// =====================================================================================================
// Win32 platforms use the SEH model: __try {}  __except {}
// Linux platforms use the POSIX Signals model: sigaction()

#include "Utilities/EventSource.h"

struct PageFaultInfo
{
	uptr	addr;

	PageFaultInfo( uptr address )
	{
		addr = address;
	}
};

// --------------------------------------------------------------------------------------
//  IEventListener_PageFault
// --------------------------------------------------------------------------------------
class IEventListener_PageFault : public IEventDispatcher<PageFaultInfo>
{
public:
	typedef PageFaultInfo EvtParams;

public:
	IEventListener_PageFault();
	virtual ~IEventListener_PageFault() throw();

	virtual void DispatchEvent( const PageFaultInfo& evtinfo, bool& handled )
	{
		OnPageFaultEvent( evtinfo, handled );
	}

	virtual void DispatchEvent( const PageFaultInfo& evtinfo )
	{
		pxFailRel( "Don't call me, damnit.  Use DispatchException instead." );
	}

protected:	
	virtual void OnPageFaultEvent( const PageFaultInfo& evtinfo, bool& handled ) {}
};

class SrcType_PageFault : public EventSource<IEventListener_PageFault>
{
protected:
	typedef EventSource<IEventListener_PageFault> _parent;

protected:
	bool	m_handled;

public:
	SrcType_PageFault() {}
	virtual ~SrcType_PageFault() throw() { }

	bool WasHandled() const { return m_handled; }
	virtual void Dispatch( const PageFaultInfo& params );

protected:
	virtual void _DispatchRaw( ListenerIterator iter, const ListenerIterator& iend, const PageFaultInfo& evt );
};

#ifdef __LINUX__

#	define PCSX2_PAGEFAULT_PROTECT
#	define PCSX2_PAGEFAULT_EXCEPT

#elif defined( _WIN32 )

struct _EXCEPTION_POINTERS;
extern int SysPageFaultExceptionFilter(struct _EXCEPTION_POINTERS* eps);

#	define PCSX2_PAGEFAULT_PROTECT		__try
#	define PCSX2_PAGEFAULT_EXCEPT		__except(SysPageFaultExceptionFilter(GetExceptionInformation())) {}

#else
#	error PCSX2 - Unsupported operating system platform.
#endif


extern void InstallSignalHandler();

extern SrcType_PageFault Source_PageFault;
