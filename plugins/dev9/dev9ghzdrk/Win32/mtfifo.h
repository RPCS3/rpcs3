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