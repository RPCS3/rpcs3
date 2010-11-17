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

#include "CpuUsageProvider.h"
#include "System.h"

#ifndef __LINUX__
#include "SysThreads.h"
#endif

#include "GS.h"

void AllThreeThreads::LoadWithCurrentTimes()
{
	ee		= GetCoreThread().GetCpuTime();
	gs		= GetMTGS().GetCpuTime();
	ui		= GetThreadCpuTime();
	update	= GetCPUTicks();
}

AllThreeThreads AllThreeThreads::operator-( const AllThreeThreads& right ) const
{
	AllThreeThreads retval;

	retval.ee		= ee - right.ee;
	retval.gs		= gs - right.gs;
	retval.ui		= ui - right.ui;
	retval.update	= update - right.update;

	return retval;
}

DefaultCpuUsageProvider::DefaultCpuUsageProvider()
{
	m_pct_ee = 0;
	m_pct_gs = 0;
	m_pct_ui = 0;
	m_writepos = 0;

	Reset();
}

void DefaultCpuUsageProvider::Reset()
{
	for( uint i=0; i<QueueDepth; ++i )
		m_queue[i].LoadWithCurrentTimes();
}

bool DefaultCpuUsageProvider::IsImplemented() const
{
	return GetThreadTicksPerSecond() != 0;
}

void DefaultCpuUsageProvider::UpdateStats()
{
	// Measure deltas between the first and last positions in the ring buffer:

	AllThreeThreads& newone( m_queue[m_writepos] );
	newone.LoadWithCurrentTimes();
	m_writepos = (m_writepos+1) % QueueDepth;
	const AllThreeThreads deltas( newone - m_queue[m_writepos] );

	// get the real time passed, scaled to the Thread's tick frequency.
	u64 timepass	= (deltas.update * GetThreadTicksPerSecond()) / GetTickFrequency();

	m_pct_ee = (deltas.ee * 100) / timepass;
	m_pct_gs = (deltas.gs * 100) / timepass;
	m_pct_ui = (deltas.ui * 100) / timepass;
}

int DefaultCpuUsageProvider::GetEEcorePct() const
{
	return m_pct_ee;
}

int DefaultCpuUsageProvider::GetGsPct() const
{
	return m_pct_gs;
}

int DefaultCpuUsageProvider::GetGuiPct() const
{
	return m_pct_ui;
}
