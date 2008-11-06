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

#include "StdAfx.h"
#include "GSPerfMon.h"

#if _MSC_VER >= 1400
extern "C" unsigned __int64 __rdtsc();
#else
__declspec(naked) unsigned __int64 __rdtsc() {__asm rdtsc __asm ret}
#endif

GSPerfMon::GSPerfMon()
	: m_total(0), m_begin(0)
	, m_frame(0), m_lastframe(0)
{
}

void GSPerfMon::IncCounter(counter_t c, double val)
{
	if(c == c_frame)
	{
		clock_t now = clock();
		if(m_lastframe != 0) 
			m_counters[c].AddTail(now - m_lastframe);
		m_lastframe = now;
		m_frame++;
	}
	else
	{
		m_counters[c].AddTail(val);
	}
}

void GSPerfMon::StartTimer()
{
	m_start = __rdtsc();
	if(m_begin == 0) m_begin = m_start;
}

void GSPerfMon::StopTimer()
{
	if(m_start > 0)
	{
		m_total += __rdtsc() - m_start;
		m_start = 0;
	}
}

CString GSPerfMon::ToString(double expected_fps)
{
	if(m_counters[c_frame].IsEmpty())
		return _T("");

	double stats[c_last];

	for(int i = 0; i < countof(m_counters); i++)
	{
		double sum = 0;
		POSITION pos = m_counters[i].GetHeadPosition();
		while(pos) sum += m_counters[i].GetNext(pos);
		stats[i] = sum / m_counters[c_frame].GetCount();
	}

	UINT64 start = m_start;

	if(start > 0) StopTimer();

	double fps = 1000.0 / stats[c_frame];
	double cpu = 100.0 * m_total / (__rdtsc() - m_begin);

	CString str;

	str.Format(_T("frame: %I64d | cpu: %d%% | %.2f fps (%d%%) | %d ppf | %.2f kbpf | %.2f kbpf | %.2f kbpf"), 
		m_frame,
		(int)(cpu),
		(float)(fps),
		(int)(100.0 * fps / expected_fps),
		(int)(stats[c_prim]),
		(float)(stats[c_swizzle] / 1024),
		(float)(stats[c_unswizzle] / 1024),
		(float)(stats[c_texture] / 1024)); 

	for(int i = 0; i < countof(m_counters); i++)
		m_counters[i].RemoveAll();
	m_total = m_begin = 0;

	if(start > 0) StartTimer();

	return str;
}
