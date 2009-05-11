/* 
 *	Copyright (C) 2007-2009 Gabest
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

class GSPerfMon
{
public:
	enum counter_t {Frame, Prim, Draw, Swizzle, Unswizzle, Fillrate, Quad, CounterLast};

protected:
	double m_counters[CounterLast];
	double m_stats[CounterLast];
	UINT64 m_begin, m_total, m_start, m_frame;
	clock_t m_lastframe;
	int m_count;

	void Start();
	void Stop();

	friend class GSPerfMonAutoTimer;

public:
	GSPerfMon();

	void SetFrame(UINT64 frame) {m_frame = frame;}
	UINT64 GetFrame() {return m_frame;}
	void Put(counter_t c, double val = 0);
	double Get(counter_t c) {return m_stats[c];}
	void Update();
	int CPU();
};

class GSPerfMonAutoTimer
{
	GSPerfMon* m_pm;

public:
	GSPerfMonAutoTimer(GSPerfMon& pm) {(m_pm = &pm)->Start();}
	~GSPerfMonAutoTimer() {m_pm->Stop();}
};
