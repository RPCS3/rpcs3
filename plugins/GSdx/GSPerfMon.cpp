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

#include "stdafx.h"
#include "GSPerfMon.h"

GSPerfMon::GSPerfMon()
	: m_frame(0)
	, m_lastframe(0)
	, m_count(0)
{
	memset(m_counters, 0, sizeof(m_counters));
	memset(m_stats, 0, sizeof(m_stats));
	memset(m_total, 0, sizeof(m_total));
	memset(m_begin, 0, sizeof(m_begin));
}

void GSPerfMon::Put(counter_t c, double val)
{
	if(c == Frame)
	{
		clock_t now = clock();

		if(m_lastframe != 0)
		{
			m_counters[c] += (now - m_lastframe) * 1000 / CLOCKS_PER_SEC;
		}

		m_lastframe = now;
		m_frame++;
		m_count++;
	}
	else
	{
		m_counters[c] += val;
	}
}

void GSPerfMon::Update()
{
	if(m_count > 0)
	{
		for(int i = 0; i < countof(m_counters); i++)
		{
			m_stats[i] = m_counters[i] / m_count;
		}

		m_count = 0;
	}

	memset(m_counters, 0, sizeof(m_counters));
}

void GSPerfMon::Start(int timer)
{
	m_start[timer] = __rdtsc();

	if(m_begin[timer] == 0)
	{
		m_begin[timer] = m_start[timer];
	}
}

void GSPerfMon::Stop(int timer)
{
	if(m_start[timer] > 0)
	{
		m_total[timer] += __rdtsc() - m_start[timer];
		m_start[timer] = 0;
	}
}

int GSPerfMon::CPU(int timer, bool reset)
{
	int percent = m_total[timer] / 1000; // (int)(100 * m_total[timer] / (__rdtsc() - m_begin[timer]));

	if(reset)
	{
		m_begin[timer] = 0;
		m_start[timer] = 0;
		m_total[timer] = 0;
	}

	return percent;
}
