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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GS.h"
#include "GSCodeBuffer.h"
#include "xbyak/xbyak.h"
#include "xbyak/xbyak_util.h"

template<class KEY, class VALUE> class GSFunctionMap
{
protected:
	struct ActivePtr
	{
		uint64 frame, frames;
		uint64 ticks, actual, total;
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
		for_each(m_map_active.begin(), m_map_active.end(), delete_second());
	}

	VALUE operator [] (KEY key)
	{
		m_active = NULL;

		typename hash_map<KEY, ActivePtr*>::iterator i = m_map_active.find(key);

		if(i != m_map_active.end())
		{
			m_active = i->second;
		}
		else
		{
			typename hash_map<KEY, VALUE>::iterator i = m_map.find(key);

			ActivePtr* p = new ActivePtr();

			memset(p, 0, sizeof(*p));

			p->frame = (uint64)-1;

			p->f = i != m_map.end() ? i->second : GetDefaultFunction(key);

			m_map_active[key] = p;

			m_active = p;
		}

		return m_active->f;
	}

	void UpdateStats(uint64 frame, uint64 ticks, int actual, int total)
	{
		if(m_active)
		{
			if(m_active->frame != frame)
			{
				m_active->frame = frame;
				m_active->frames++;
			}

			m_active->ticks += ticks;
			m_active->actual += actual;
			m_active->total += total;

			ASSERT(m_active->total >= m_active->actual);
		}
	}

	virtual void PrintStats()
	{
		uint64 ttpf = 0;

		typename hash_map<KEY, ActivePtr*>::iterator i;

		for(i = m_map_active.begin(); i != m_map_active.end(); i++)
		{
			ActivePtr* p = i->second;

			if(p->frames)
			{
				ttpf += p->ticks / p->frames;
			}
		}

		printf("GS stats\n");

		for(i = m_map_active.begin(); i != m_map_active.end(); i++)
		{
			KEY key = i->first;
			ActivePtr* p = i->second;

			if(p->frames > 0)
			{
				uint64 tpp = p->actual > 0 ? p->ticks / p->actual : 0;
				uint64 tpf = p->frames > 0 ? p->ticks / p->frames : 0;
				uint64 ppf = p->frames > 0 ? p->actual / p->frames : 0;

				printf("[%014llx]%c %6.2f%% %5.2f%% f %4lld t %12lld p %12lld w %12lld tpp %4lld tpf %9lld ppf %9lld\n",
					(uint64)key, m_map.find(key) == m_map.end() ? '*' : ' ',
					(float)(tpf * 10000 / 34000000) / 100,
					(float)(tpf * 10000 / ttpf) / 100,
					p->frames, p->ticks, p->actual, p->total - p->actual,
					tpp, tpf, ppf);
			}
		}
	}
};

class GSCodeGenerator : public Xbyak::CodeGenerator
{
protected:
	Xbyak::util::Cpu m_cpu;

public:
	GSCodeGenerator(void* code, size_t maxsize)
		: Xbyak::CodeGenerator(maxsize, code)
	{
	}
};

template<class CG, class KEY, class VALUE>
class GSCodeGeneratorFunctionMap : public GSFunctionMap<KEY, VALUE>
{
	string m_name;
	void* m_param;
	hash_map<uint64, VALUE> m_cgmap;
	GSCodeBuffer m_cb;

	enum {MAX_SIZE = 8192};

public:
	GSCodeGeneratorFunctionMap(const char* name, void* param)
		: m_name(name)
		, m_param(param)
	{
	}

	VALUE GetDefaultFunction(KEY key)
	{
		VALUE ret = NULL;

		typename hash_map<uint64, VALUE>::iterator i = m_cgmap.find(key);

		if(i != m_cgmap.end())
		{
			ret = i->second;
		}
		else
		{
			CG* cg = new CG(m_param, key, m_cb.GetBuffer(MAX_SIZE), MAX_SIZE);

			ASSERT(cg->getSize() < MAX_SIZE);

			m_cb.ReleaseBuffer(cg->getSize());

			ret = (VALUE)cg->getCode();

			m_cgmap[key] = ret;

			#ifdef ENABLE_VTUNE

			// vtune method registration

			// if(iJIT_IsProfilingActive()) // always > 0
			{
				string name = format("%s<%016llx>()", m_name.c_str(), (uint64)key);

				iJIT_Method_Load ml;

				memset(&ml, 0, sizeof(ml));

				ml.method_id = iJIT_GetNewMethodID();
				ml.method_name = (char*)name.c_str();
				ml.method_load_address = (void*)cg->getCode();
				ml.method_size = (unsigned int)cg->getSize();

				iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED, &ml);
/*
				name = format("c:/temp1/%s_%016llx.bin", m_name.c_str(), (uint64)key);

				if(FILE* fp = fopen(name.c_str(), "wb"))
				{
					fputc(0x0F, fp); fputc(0x0B, fp);
					fputc(0xBB, fp); fputc(0x6F, fp); fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp);
					fputc(0x64, fp); fputc(0x67, fp); fputc(0x90, fp);

					fwrite(cg->getCode(), cg->getSize(), 1, fp);

					fputc(0xBB, fp); fputc(0xDE, fp); fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp);
					fputc(0x64, fp); fputc(0x67, fp); fputc(0x90, fp);
					fputc(0x0F, fp); fputc(0x0B, fp);

					fclose(fp);
				}
*/
			}

			#endif

			delete cg;
		}

		return ret;
	}
};
