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

#include "CpuUsageProvider.h"
#include "System.h"
#include "SysThreads.h"
#include "GS.h"

DefaultCpuUsageProvider::DefaultCpuUsageProvider()
{
	m_lasttime_ee	= GetCoreThread().GetCpuTime();
	m_lasttime_gs	= GetMTGS().GetCpuTime();
	m_lasttime_ui	= GetThreadCpuTime();

	m_lasttime_update	= GetCPUTicks();
	
	m_pct_ee = 0;
	m_pct_gs = 0;
	m_pct_ui = 0;
}

bool DefaultCpuUsageProvider::IsImplemented() const
{
	return GetThreadTicksPerSecond() != 0;
}

void DefaultCpuUsageProvider::UpdateStats()
{
	u64 curtime		= GetCPUTicks();
	u64 delta		= curtime - m_lasttime_update;
	if( delta < (GetTickFrequency() / 16) ) return;

	u64 curtime_ee	= GetCoreThread().GetCpuTime();
	u64 curtime_gs	= GetMTGS().GetCpuTime();
	u64 curtime_ui	= GetThreadCpuTime();

	// get the real time passed, scaled to the Thread's tick frequency.
	u64 timepass	= (delta * GetThreadTicksPerSecond()) / GetTickFrequency();

	m_pct_ee = ((curtime_ee - m_lasttime_ee) * 100) / timepass;
	m_pct_gs = ((curtime_gs - m_lasttime_gs) * 100) / timepass;
	m_pct_ui = ((curtime_ui - m_lasttime_ui) * 100) / timepass;

	m_lasttime_update	= curtime;
	m_lasttime_ee		= curtime_ee;
	m_lasttime_gs		= curtime_gs;
	m_lasttime_ui		= curtime_ui;
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
