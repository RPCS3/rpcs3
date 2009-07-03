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
#include "GSCodeBuffer.h"

struct GSRasterizerStats
{
	int64 ticks;
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

template<class KEY, class VALUE> class GSFunctionMap
{
protected:
	struct ActivePtr
	{
		uint64 frame, frames;
		int64 ticks, pixels;
		VALUE f;
	};

	hash_map<KEY, VALUE> m_map;
	hash_map<KEY, ActivePtr*> m_map_active;

	ActivePtr* m_active;

	virtual VALUE GetDefaultFunction(KEY key) = 0;

public:
	GSFunctionMap() 
		: m_active(NULL) 
	{
	}

	virtual ~GSFunctionMap()
	{
		for(hash_map<KEY, ActivePtr*>::iterator i = m_map_active.begin(); i != m_map_active.end(); i++)
		{
			delete i->second;
		}
	}

	VALUE operator [] (KEY key)
	{
		m_active = NULL;

		hash_map<KEY, ActivePtr*>::iterator i = m_map_active.find(key);

		if(i != m_map_active.end())
		{
			m_active = i->second;
		}
		else
		{
			hash_map<KEY, VALUE>::iterator i = m_map.find(key);

			ActivePtr* p = new ActivePtr();

			memset(p, 0, sizeof(*p));

			p->frame = (uint64)-1;

			p->f = i != m_map.end() ? i->second : GetDefaultFunction(key);

			m_map_active[key] = p;

			m_active = p;
		}

		return m_active->f;
	}

	void UpdateStats(const GSRasterizerStats& stats, uint64 frame)
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
		int64 ttpf = 0;

		for(hash_map<KEY, ActivePtr*>::iterator i = m_map_active.begin(); i != m_map_active.end(); i++)
		{
			ActivePtr* p = i->second;
			
			if(p->frames)
			{
				ttpf += p->ticks / p->frames;
			}
		}

		for(hash_map<KEY, ActivePtr*>::iterator i = m_map_active.begin(); i != m_map_active.end(); i++)
		{
			KEY key = i->first;
			ActivePtr* p = i->second;

			if(p->frames > 0)
			{
				int64 tpp = p->pixels > 0 ? p->ticks / p->pixels : 0;
				int64 tpf = p->frames > 0 ? p->ticks / p->frames : 0;
				int64 ppf = p->frames > 0 ? p->pixels / p->frames : 0;

				printf("[%012I64x]%c %6.2f%% | %5.2f%% | f %4I64d | p %10I64d | tpp %4I64d | tpf %9I64d | ppf %7I64d\n", 
					(uint64)key, m_map.find(key) == m_map.end() ? '*' : ' ',
					(float)(tpf * 10000 / 50000000) / 100, 
					(float)(tpf * 10000 / ttpf) / 100, 
					p->frames, p->pixels, 
					tpp, tpf, ppf);
			}
		}
	}
};

#include "vtune/JITProfiling.h"

template<class CG, class KEY, class VALUE>
class GSCodeGeneratorFunctionMap : public GSFunctionMap<KEY, VALUE>
{
	uint32 m_id;
	string m_name;
	hash_map<uint64, CG*> m_cgmap;
	GSCodeBuffer m_cb;

	enum {MAX_SIZE = 4096};

protected:
	virtual CG* Create(KEY key, void* ptr, size_t maxsize = MAX_SIZE) = 0;

public:
	GSCodeGeneratorFunctionMap(const char* name)
		: m_id(0x100000)
		, m_name(name)
	{
	}

	virtual ~GSCodeGeneratorFunctionMap()
	{
		for(hash_map<uint64, CG*>::iterator i = m_cgmap.begin(); i != m_cgmap.end(); i++)
		{
			delete i->second;
		}
	}

	VALUE GetDefaultFunction(KEY key)
	{
		CG* cg = NULL;

		hash_map<uint64, CG*>::iterator i = m_cgmap.find(key);

		if(i != m_cgmap.end())
		{
			cg = i->second;
		}
		else
		{
			void* ptr = m_cb.GetBuffer(MAX_SIZE);

			cg = Create(key, ptr, MAX_SIZE);

			ASSERT(cg);

			m_cb.ReleaseBuffer(cg->getSize());

			m_cgmap[key] = cg;

			// vtune method registration

			string name = format("%s<%016I64x>()", m_name.c_str(), (uint64)key);

			iJIT_Method_Load ml;

			memset(&ml, 0, sizeof(ml));

			ml.method_id = m_id++;
			ml.method_name = (char*)name.c_str();
			ml.method_load_address = (void*)cg->getCode();
			ml.method_size = cg->getSize();

			iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, &ml);
		}

		return (VALUE)cg->getCode();
	}
};
