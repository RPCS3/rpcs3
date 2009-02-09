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

#include "GS.h"

struct GSRasterizerStats
{
	__int64 ticks;
	int prims, pixels;

	GSRasterizerStats() 
	{
		Reset();
	}

	void Reset() 
	{
		ticks = 0;
		pixels = prims = 0;
	}
};

template<class T> class GSFunctionMap
{
protected:
	struct ActivePtr
	{
		UINT64 frame, frames;
		__int64 ticks, pixels;
		T f;
	};

	CRBMap<DWORD, T> m_map;
	CRBMap<DWORD, ActivePtr*> m_map_active;
	ActivePtr* m_active;

	virtual T GetDefaultFunction(DWORD sel) = 0;

public:
	GSFunctionMap() 
		: m_active(NULL) 
	{
	}

	virtual ~GSFunctionMap()
	{
		POSITION pos = m_map_active.GetHeadPosition();

		while(pos)
		{
			delete m_map_active.GetNextValue(pos);
		}

		m_map_active.RemoveAll();
	}

	void SetAt(DWORD sel, T f)
	{
		m_map.SetAt(sel, f);
	}

	T Lookup(DWORD sel)
	{
		m_active = NULL;

		if(!m_map_active.Lookup(sel, m_active))
		{
			CRBMap<DWORD, T>::CPair* pair = m_map.Lookup(sel);

			ActivePtr* p = new ActivePtr();

			memset(p, 0, sizeof(*p));

			p->frame = (UINT64)-1;

			p->f = pair ? pair->m_value : GetDefaultFunction(sel);

			m_map_active.SetAt(sel, p);

			m_active = p;
		}

		return m_active->f;
	}

	void UpdateStats(const GSRasterizerStats& stats, UINT64 frame)
	{
		if(m_active)
		{
			if(m_active->frame != frame)
			{
				m_active->frame = frame;
				m_active->frames++;
			}

			m_active->pixels += stats.pixels;
			m_active->ticks += stats.ticks;
		}
	}

	virtual void PrintStats()
	{
		__int64 ttpf = 0;

		POSITION pos = m_map_active.GetHeadPosition();

		while(pos)
		{
			ActivePtr* p = m_map_active.GetNextValue(pos);
			
			if(p->frames)
			{
				ttpf += p->ticks / p->frames;
			}
		}

		pos = m_map_active.GetHeadPosition();

		while(pos)
		{
			DWORD sel;
			ActivePtr* p;

			m_map_active.GetNextAssoc(pos, sel, p);

			if(p->frames > 0)
			{
				__int64 tpp = p->pixels > 0 ? p->ticks / p->pixels : 0;
				__int64 tpf = p->frames > 0 ? p->ticks / p->frames : 0;
				__int64 ppf = p->frames > 0 ? p->pixels / p->frames : 0;

				printf("[%08x]%c %6.2f%% | %5.2f%% | f %4I64d | p %10I64d | tpp %4I64d | tpf %9I64d | ppf %7I64d\n", 
					sel, !m_map.Lookup(sel) ? '*' : ' ',
					(float)(tpf * 10000 / 50000000) / 100, 
					(float)(tpf * 10000 / ttpf) / 100, 
					p->frames, p->pixels, 
					tpp, tpf, ppf);
			}
		}
	}
};
