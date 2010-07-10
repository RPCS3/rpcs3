/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


////////////////////
// Small profiler //
////////////////////
#include <list>
#include <string>
#include <map>
#include "Profile.h"

using namespace std;

#define GET_PROFILE_TIME() GetCPUTicks()

#if !defined(ZEROGS_DEVBUILD)
#define g_bWriteProfile 0
#else
bool g_bWriteProfile = 0;
#endif

u64 luPerfFreq;

struct DVPROFSTRUCT;

struct DVPROFSTRUCT
{

	struct DATA
	{
		DATA(u64 time, u32 user = 0) : dwTime(time), dwUserData(user) {}

		DATA() : dwTime(0), dwUserData(0) {}

		u64 dwTime;
		u32 dwUserData;
	};

	~DVPROFSTRUCT()
	{
		list<DVPROFSTRUCT*>::iterator it = listpChild.begin();

		while (it != listpChild.end())
		{
			SAFE_DELETE(*it);
			++it;
		}
	}

	list<DATA> listTimes;		 // before DVProfEnd is called, contains the global time it started
	// after DVProfEnd is called, contains the time it lasted
	// the list contains all the tracked times
	char pname[256];

	list<DVPROFSTRUCT*> listpChild;	 // other profilers called during this profiler period
};

struct DVPROFTRACK
{
	u32 dwUserData;
	DVPROFSTRUCT::DATA* pdwTime;
	DVPROFSTRUCT* pprof;
};

list<DVPROFTRACK> g_listCurTracking;	// the current profiling functions, the back element is the
// one that will first get popped off the list when DVProfEnd is called
// the pointer is an element in DVPROFSTRUCT::listTimes
list<DVPROFSTRUCT> g_listProfilers;		 // the current profilers, note that these are the parents
// any profiler started during the time of another is held in
// DVPROFSTRUCT::listpChild
list<DVPROFSTRUCT*> g_listAllProfilers;	 // ignores the hierarchy, pointer to elements in g_listProfilers

void DVProfRegister(char* pname)
{
	if (!g_bWriteProfile) return;

	list<DVPROFSTRUCT*>::iterator it = g_listAllProfilers.begin();

//	while(it != g_listAllProfilers.end() ) {
//
//		if( _tcscmp(pname, (*it)->pname) == 0 ) {
//			(*it)->listTimes.push_back(timeGetTime());
//			DVPROFTRACK dvtrack;
//			dvtrack.pdwTime = &(*it)->listTimes.back();
//			dvtrack.pprof = *it;
//			g_listCurTracking.push_back(dvtrack);
//			return;
//		}
//
//		++it;
//	}

	// else add in a new profiler to the appropriate parent profiler
	DVPROFSTRUCT* pprof = NULL;

	if (g_listCurTracking.size() > 0)
	{
		assert(g_listCurTracking.back().pprof != NULL);
		g_listCurTracking.back().pprof->listpChild.push_back(new DVPROFSTRUCT());
		pprof = g_listCurTracking.back().pprof->listpChild.back();
	}
	else
	{
		g_listProfilers.push_back(DVPROFSTRUCT());
		pprof = &g_listProfilers.back();
	}

	strncpy(pprof->pname, pname, 256);

	// setup the profiler for tracking
	pprof->listTimes.push_back(DVPROFSTRUCT::DATA(GET_PROFILE_TIME()));

	DVPROFTRACK dvtrack;
	dvtrack.pdwTime = &pprof->listTimes.back();
	dvtrack.pprof = pprof;
	dvtrack.dwUserData = 0;

	g_listCurTracking.push_back(dvtrack);

	// add to all profiler list
	g_listAllProfilers.push_back(pprof);
}

void DVProfEnd(u32 dwUserData)
{
	if (!g_bWriteProfile)
		return;

	B_RETURN(g_listCurTracking.size() > 0);

	DVPROFTRACK dvtrack = g_listCurTracking.back();

	assert(dvtrack.pdwTime != NULL && dvtrack.pprof != NULL);

	dvtrack.pdwTime->dwTime = 1000000 * (GET_PROFILE_TIME() - dvtrack.pdwTime->dwTime) / luPerfFreq;

	dvtrack.pdwTime->dwUserData = dwUserData;

	g_listCurTracking.pop_back();
}

struct DVTIMEINFO
{
	DVTIMEINFO() : uInclusive(0), uExclusive(0) {}

	u64 uInclusive, uExclusive;
};

map<string, DVTIMEINFO> mapAggregateTimes;

u64 DVProfWriteStruct(FILE* f, DVPROFSTRUCT* p, int ident)
{
	fprintf(f, "%*s%s - ", ident, "", p->pname);

	list<DVPROFSTRUCT::DATA>::iterator ittime = p->listTimes.begin();

	u64 utime = 0;

	while (ittime != p->listTimes.end())
	{
		utime += (u32)ittime->dwTime;

		if (ittime->dwUserData)
			fprintf(f, "time: %d, user: 0x%8.8x", (u32)ittime->dwTime, ittime->dwUserData);
		else
			fprintf(f, "time: %d", (u32)ittime->dwTime);

		++ittime;
	}

	mapAggregateTimes[p->pname].uInclusive += utime;

	fprintf(f, "\n");

	list<DVPROFSTRUCT*>::iterator itprof = p->listpChild.begin();

	u32 uex = utime;

	while (itprof != p->listpChild.end())
	{

		uex -= (u32)DVProfWriteStruct(f, *itprof, ident + 4);
		++itprof;
	}

	mapAggregateTimes[p->pname].uExclusive += uex;

	return utime;
}

void DVProfWrite(char* pfilename, u32 frames)
{
	assert(pfilename != NULL);
	FILE* f = fopen(pfilename, "wb");

	mapAggregateTimes.clear();
	list<DVPROFSTRUCT>::iterator it = g_listProfilers.begin();

	while (it != g_listProfilers.end())
	{
		DVProfWriteStruct(f, &(*it), 0);
		++it;
	}

	{
		map<string, DVTIMEINFO>::iterator it;
		fprintf(f, "\n\n-------------------------------------------------------------------\n\n");

		u64 uTotal[2] = {0};
		double fiTotalTime[2];

		for (it = mapAggregateTimes.begin(); it != mapAggregateTimes.end(); ++it)
		{
			uTotal[0] += it->second.uExclusive;
			uTotal[1] += it->second.uInclusive;
		}

		fprintf(f, "total times (%d): ex: %Lu ", frames, uTotal[0] / frames);

		fprintf(f, "inc: %Lu\n", uTotal[1] / frames);

		fiTotalTime[0] = 1.0 / (double)uTotal[0];
		fiTotalTime[1] = 1.0 / (double)uTotal[1];

		// output the combined times

		for (it = mapAggregateTimes.begin(); it != mapAggregateTimes.end(); ++it)
		{
			fprintf(f, "%s - ex: %f inc: %f\n", it->first.c_str(), (double)it->second.uExclusive * fiTotalTime[0],
					(double)it->second.uInclusive * fiTotalTime[1]);
		}
	}


	fclose(f);
}

void DVProfClear()
{
	g_listCurTracking.clear();
	g_listProfilers.clear();
	g_listAllProfilers.clear();
}

void InitProfile()
{
	luPerfFreq = GetCPUTicks();
}
