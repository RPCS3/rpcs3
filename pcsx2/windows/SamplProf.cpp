
#include "PrecompiledHeader.h"
#include "Utilities/RedtapeWindows.h"

#include "SamplProf.h"
#include <map>
#include <algorithm>

using namespace std;

DWORD GetModuleFromPtr(IN void* ptr,OUT LPWSTR lpFilename,IN DWORD nSize)
{
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(ptr,&mbi,sizeof(mbi));
	return GetModuleFileName((HMODULE)mbi.AllocationBase,lpFilename,nSize);
}

struct Module
{
	uptr base;
	uptr end;
	uptr len;
	wxString name;
	u32 ticks;

	Module(const wxChar* name, const void* ptr)
	{
		if (name!=0)
			this->name=name;
		FromAddress(ptr,name==0);
		ticks=0;
	}
	Module(const wxChar* name, const void* b, u32 s)
	{
		this->name=name;
		FromValues(b,s);
		ticks=0;
	}
	bool operator<(const Module &other) const
	{
		return ticks>other.ticks;
	}
	wxString ToString(u32 total_ticks)
	{
		return wxsFormat( L"| %s: %2.2f%% |", name.c_str(), (float)(((double)ticks*100.0) / (double)total_ticks) );
	}
	bool Inside(uptr val) { return val>=base && val<=end; }
	void FromAddress(const void* ptr,bool getname)
	{
		wxChar filename[512];
		wxChar filename2[512];
		static const void* ptr_old=0;

		if (ptr_old==ptr)
			return;
		ptr_old=ptr;

		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(ptr,&mbi,sizeof(mbi));
		base=(u32)mbi.AllocationBase;
		GetModuleFileName((HMODULE)mbi.AllocationBase,filename,512);
		len=(u8*)mbi.BaseAddress-(u8*)mbi.AllocationBase+mbi.RegionSize;

		if (getname)
		{
			name=filename;
			size_t last=name.find_last_of('\\');
			last=last==name.npos?0:last+1;
			name=name.substr(last);
		}

		for(;;)
		{
			VirtualQuery(((u8*)base)+len,&mbi,sizeof(mbi));
			if (!(mbi.Type&MEM_IMAGE))
				break;

			if (!GetModuleFileName((HMODULE)mbi.AllocationBase,filename2,512))
				break;

			if (wxStrcmp(filename,filename2)!=0)
				break;
			len+=mbi.RegionSize;
		}


		end=base+len-1;
	}
	void FromValues(const void* b,u32 s)
	{
		base= (uptr)b;
		len=s;
		end=base+len-1;
	}
};

typedef map<wxString,Module> MapType;

static vector<Module> ProfModules;
static MapType ProfUnknownHash;

static HANDLE hEmuThread = NULL;
static HANDLE hMtgsThread = NULL;
static HANDLE hProfThread = NULL;

static CRITICAL_SECTION ProfModulesLock;

static volatile bool ProfRunning=false;

static bool _registeredName( const wxString& name )
{
	for( vector<Module>::const_iterator
		iter = ProfModules.begin(),
		end = ProfModules.end(); iter<end; ++iter )
	{
		if( iter->name.compare( name ) == 0 )
			return true;
	}
	return false;
}

void ProfilerRegisterSource(const char* Name, const void* buff, u32 sz)
{
	if( ProfRunning )
		EnterCriticalSection( &ProfModulesLock );

	wxString strName( fromUTF8(Name) );
	if( !_registeredName( strName ) )
		ProfModules.push_back( Module( strName, buff, sz ) );

	if( ProfRunning )
		LeaveCriticalSection( &ProfModulesLock );
}

void ProfilerRegisterSource(const char* Name, const void* function)
{
	if( ProfRunning )
		EnterCriticalSection( &ProfModulesLock );

	wxString strName( fromUTF8(Name) );
	if( !_registeredName( strName ) )
		ProfModules.push_back( Module(strName,function) );

	if( ProfRunning )
		LeaveCriticalSection( &ProfModulesLock );
}

void ProfilerTerminateSource( const char* Name )
{
	wxString strName( fromUTF8(Name) );
	for( vector<Module>::const_iterator
		iter = ProfModules.begin(),
		end = ProfModules.end(); iter<end; ++iter )
	{
		if( iter->name.compare( strName ) == 0 )
		{
			ProfModules.erase( iter );
			break;
		}
	}
}

static bool DispatchKnownModules( uint Eip )
{
	bool retval = false;
	EnterCriticalSection( &ProfModulesLock );

	size_t i;
	for(i=0;i<ProfModules.size();i++)
		if (ProfModules[i].Inside(Eip)) break;

	if( i < ProfModules.size() )
	{
		ProfModules[i].ticks++;
		retval = true;
	}

	LeaveCriticalSection( &ProfModulesLock );
	return retval;
}

static void MapUnknownSource( uint Eip )
{
	wxChar modulename[512];
	DWORD sz=GetModuleFromPtr((void*)Eip,modulename,512);
	wxString modulenam( (sz==0) ? L"[Unknown]" : modulename );

	map<wxString,Module>::iterator iter = ProfUnknownHash.find(modulenam);
	if (iter!=ProfUnknownHash.end())
	{
		iter->second.ticks++;
		return;
	}

	Module tmp((sz==0) ? modulenam.c_str() : NULL, (void*)Eip);
	tmp.ticks++;

	ProfUnknownHash.insert(MapType::value_type(modulenam, tmp));
}

int __stdcall ProfilerThread(void* nada)
{
	ProfUnknownHash.clear();
	u32 tick_count=0;

	while(ProfRunning)
	{
		Sleep(5);

		if (tick_count>500)
		{
			wxString rT = L"";
			wxString rv = L"";
			u32 subtotal=0;
			for (size_t i=0;i<ProfModules.size();i++)
			{
				wxString t  = ProfModules[i].ToString(tick_count);
				bool	 b0 = EmuConfig.Cpu.Recompiler.UseMicroVU0;
				bool	 b1 = EmuConfig.Cpu.Recompiler.UseMicroVU1;
				if		( b0 &&  b1) { if (t.Find(L"sVU")  == -1) rT+=t;}
				else if (!b0 && !b1) { if (t.Find(L"mVU")  == -1) rT+=t;}
				else if (!b0)		 { if (t.Find(L"mVU0") == -1) rT+=t;}
				else if (!b1)		 { if (t.Find(L"mVU1") == -1) rT+=t;}
				else rT+=t;
				
				subtotal+=ProfModules[i].ticks;
				ProfModules[i].ticks=0;
			}

			rT += wxsFormat( L"| Recs Total: %2.2f%% |", (float)(((double)subtotal*100.0) / (double)tick_count));
			vector<MapType::mapped_type> lst;
			for (MapType::iterator i=ProfUnknownHash.begin();i!=ProfUnknownHash.end();i++)
			{
				lst.push_back(i->second);
			}

			sort(lst.begin(),lst.end());
			for (size_t i=0;i<lst.size();i++)
			{
				rv += lst[i].ToString(tick_count);
			}

			Console.WriteLn( L"Sampling Profiler Results:\n%s\n%s\n", rT.c_str(), rv.c_str() );
			Console.SetTitle(rT);

			tick_count=0;

			ProfUnknownHash.clear();
		}

		tick_count++;

		CONTEXT ctx;
		ctx.ContextFlags = CONTEXT_FULL;
		GetThreadContext(hEmuThread,&ctx);

		if( !DispatchKnownModules( ctx.Eip ) )
		{
			MapUnknownSource( ctx.Eip );
		}

		if( hMtgsThread != NULL )
		{
			GetThreadContext(hMtgsThread,&ctx);
			if( DispatchKnownModules( ctx.Eip ) )
				continue;
		}
	}

	return -1;
}

void ProfilerInit()
{
	if (ProfRunning)
		return;

	Console.WriteLn( "Profiler Thread Initializing..." );
	ProfRunning=true;
	DuplicateHandle(GetCurrentProcess(),
		GetCurrentThread(),
		GetCurrentProcess(),
		&(HANDLE)hEmuThread,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS);

	InitializeCriticalSection( &ProfModulesLock );

	hProfThread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)ProfilerThread,0,0,0);
	SetThreadPriority(hProfThread,THREAD_PRIORITY_HIGHEST);
	Console.WriteLn( "Profiler Thread Started!" );
}

void ProfilerTerm()
{
	Console.WriteLn( "Profiler Terminating..." );
	if (!ProfRunning)
		return;

	ProfRunning=false;

	if( hProfThread != NULL )
	{
		ResumeThread(hProfThread);
		WaitForSingleObject(hProfThread,INFINITE);
		CloseHandle(hProfThread);
	}

	if( hEmuThread != NULL )
		CloseHandle( hEmuThread );

	if( hMtgsThread != NULL )
		CloseHandle( hMtgsThread );

	DeleteCriticalSection( &ProfModulesLock );
	Console.WriteLn( "Profiler Termination Done!" );
}

void ProfilerSetEnabled(bool Enabled)
{
	if (!ProfRunning)
	{
		if( !Enabled ) return;
		ProfilerInit();
	}

	if (Enabled)
		ResumeThread(hProfThread);
	else
		SuspendThread(hProfThread);
}
