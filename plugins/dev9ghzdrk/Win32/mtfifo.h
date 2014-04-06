/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014 David Quintana [gigaherz]
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

#pragma once
#include <windows.h>
//a simple, mt-safe fifo template class
template<typename T>
class mtfifo
{
	struct container
	{ 
		container(container* n,T d)
		{
			next=n;
			data=d;
		}
		container* next;T data;
	};
	container* start;
	container* end;
	
	CRITICAL_SECTION cs;
public:
	mtfifo()
	{
		InitializeCriticalSection(&cs);
	}
	~mtfifo()
	{
		//no need to destroy the CS? i cant remember realy .. ;p
	}
	void put(T data)
	{
		EnterCriticalSection(&cs);
		if (end==0)
		{
			end=start=new container(0,data);
		}
		else
		{
			end=end->next=new container(0,data);			
		}
		LeaveCriticalSection(&cs);
	}
	//Note, this is partialy mt-safe, the get may fail even if that returned false
	bool empty(){ return start==0;}
	bool get(T& rvi)
	{
		container* rv;
		EnterCriticalSection(&cs);
		if (start==0)
		{
			rv=0; //error
			
			
		}
		else
		{
			rv=start;
			start=rv->next;
			if (!start)
				end=0; //last item
		}
		LeaveCriticalSection(&cs);

		if(!rv)
			return false;
		rvi=rv->data;
		delete rv;

		return true;
	}
};