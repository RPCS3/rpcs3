/* 
 *	Copyright (C) 2003-2005 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "x86.h"

class GSPerfMon
{
public:
	enum counter_t {c_frame, c_prim, c_swizzle, c_unswizzle, c_texture, c_last};

protected:
	CAtlList<double> m_counters[c_last];
	UINT64 m_begin, m_total, m_start, m_frame;
	clock_t m_lastframe;

public:
	GSPerfMon();

	void IncCounter(counter_t c, double val = 0);
	void StartTimer(), StopTimer();
	CString ToString(double expected_fps);
	UINT64 GetFrame() {return m_frame;}
};

class GSPerfMonAutoTimer
{
	GSPerfMon* m_pm;

public:
	GSPerfMonAutoTimer(GSPerfMon& pm) {(m_pm = &pm)->StartTimer();}
	~GSPerfMonAutoTimer() {m_pm->StopTimer();}
};