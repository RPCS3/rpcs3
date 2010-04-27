/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#include "PrecompiledHeader.h"

#include <list>

#include "GS.h"

#ifdef PCSX2_DEVBUILD

// GS Playback
int g_SaveGSStream = 0; // save GS stream; 1 - prepare, 2 - save
int g_nLeftGSFrames = 0; // when saving, number of frames left
static ScopedPtr<memSavingState> g_fGSSave;

// fixme - need to take this concept and make it MTGS friendly.
#ifdef _STGS_GSSTATE_CODE
void GSGIFTRANSFER1(u32 *pMem, u32 addr) {
	if( g_SaveGSStream == 2) {
		u32 type = GSRUN_TRANS1;
		u32 size = (0x4000-(addr))/16;
		g_fGSSave->Freeze( type );
		g_fGSSave->Freeze( size );
		g_fGSSave->FreezeMem( ((u8*)pMem)+(addr), size*16 );
	}
	GSgifTransfer1(pMem, addr);
}

void GSGIFTRANSFER2(u32 *pMem, u32 size) {
	if( g_SaveGSStream == 2) {
		u32 type = GSRUN_TRANS2;
		u32 _size = size;
		g_fGSSave->Freeze( type );
		g_fGSSave->Freeze( size );
		g_fGSSave->FreezeMem( pMem, _size*16 );
	}
	GSgifTransfer2(pMem, size);
}

void GSGIFTRANSFER3(u32 *pMem, u32 size) {
	if( g_SaveGSStream == 2 ) {
		u32 type = GSRUN_TRANS3;
		u32 _size = size;
		g_fGSSave->Freeze( type );
		g_fGSSave->Freeze( size );
		g_fGSSave->FreezeMem( pMem, _size*16 );
	}
	GSgifTransfer3(pMem, size);
}

__forceinline void GSVSYNC(void) {
	if( g_SaveGSStream == 2 ) {
		u32 type = GSRUN_VSYNC;
		g_fGSSave->Freeze( type );
	}
}
#endif

/*void SaveGSState(const wxString& file)
{
	if( g_SaveGSStream ) return;

	Console.WriteLn( "Saving GS State..." );
	Console.WriteLn( L"\t%s", file.c_str() );

	SafeArray<u8> buf;
	g_fGSSave = new memSavingState( buf );

	g_SaveGSStream = 1;
	g_nLeftGSFrames = 2;

	g_fGSSave->Freeze( g_nLeftGSFrames );
}

void LoadGSState(const wxString& file)
{
	int ret;

	Console.WriteLn( "Loading GS State..." );

	wxString src( file );

	//if( !wxFileName::FileExists( src ) )
	//	src = Path::Combine( g_Conf->Folders.Savestates, src );

	if( !wxFileName::FileExists( src ) )
		return;

	SafeArray<u8> buf;
	memLoadingState f( buf );

	// Always set gsIrq callback -- GS States are always exclusionary of MTGS mode
	GSirqCallback( gsIrq );

	ret = GSopen(&pDsp, "PCSX2", 0);
	if (ret != 0)
		throw Exception::PluginOpenError( PluginId_GS );

	ret = PADopen((void *)&pDsp);

	f.Freeze(g_nLeftGSFrames);
	f.gsFreeze();

	GetPluginManager().Freeze( PluginId_GS, f );

	RunGSState( f );

	GetCorePlugins().Close( PluginId_GS );
	GetCorePlugins().Close( PluginId_PAD );
}

struct GSStatePacket
{
	u32 type;
	std::vector<u8> mem;
};

// runs the GS
// (this should really be part of the AppGui)
void RunGSState( memLoadingState& f )
{
	u32 newfield;
	std::list< GSStatePacket > packets;

	while( !f.IsFinished() )
	{
		int type, size;
		f.Freeze( type );

		if( type != GSRUN_VSYNC ) f.Freeze( size );

		packets.push_back(GSStatePacket());
		GSStatePacket& p = packets.back();

		p.type = type;

		if( type != GSRUN_VSYNC ) {
			p.mem.resize(size*16);
			f.FreezeMem( &p.mem[0], size*16 );
		}
	}

	std::list<GSStatePacket>::iterator it = packets.begin();
	g_SaveGSStream = 3;

	// first extract the data
	while(1) {

		switch(it->type) {
			case GSRUN_TRANS1:
				GSgifTransfer1((u32*)&it->mem[0], 0);
				break;
			case GSRUN_TRANS2:
				GSgifTransfer2((u32*)&it->mem[0], it->mem.size()/16);
				break;
			case GSRUN_TRANS3:
				GSgifTransfer3((u32*)&it->mem[0], it->mem.size()/16);
				break;
			case GSRUN_VSYNC:
				// flip
				newfield = (*(u32*)(PS2MEM_GS+0x1000)&0x2000) ? 0 : 0x2000;
				*(u32*)(PS2MEM_GS+0x1000) = (*(u32*)(PS2MEM_GS+0x1000) & ~(1<<13)) | newfield;

				GSvsync(newfield);

				// fixme : Process pending app messages here.
				//SysUpdate();

				if( g_SaveGSStream != 3 )
					return;
				break;

			jNO_DEFAULT
		}

		++it;
		if( it == packets.end() )
			it = packets.begin();
	}
}*/
#endif

//////////////////////////////////////////////////////////////////////////////////////////
//
void vSyncDebugStuff( uint frame )
{
#ifdef OLD_TESTBUILD_STUFF
	if( g_TestRun.enabled && g_TestRun.frame > 0 ) {
		if( frame > g_TestRun.frame ) {
			// take a snapshot
			if( g_TestRun.pimagename != NULL && GSmakeSnapshot2 != NULL ) {
				if( g_TestRun.snapdone ) {
					g_TestRun.curimage++;
					g_TestRun.snapdone = 0;
					g_TestRun.frame += 20;
					if( g_TestRun.curimage >= g_TestRun.numimages ) {
						// exit
						g_EmuThread->Cancel();
					}
				}
				else {
					// query for the image
					GSmakeSnapshot2(g_TestRun.pimagename, &g_TestRun.snapdone, g_TestRun.jpgcapture);
				}
			}
			else {
				// exit
				g_EmuThread->Cancel();
			}
		}
	}

	GSVSYNC();

	if( g_SaveGSStream == 1 ) {
		freezeData fP;

		g_SaveGSStream = 2;
		g_fGSSave->gsFreeze();

		if (GSfreeze(FREEZE_SIZE, &fP) == -1) {
			safe_delete( g_fGSSave );
			g_SaveGSStream = 0;
		}
		else {
			fP.data = (s8*)malloc(fP.size);
			if (fP.data == NULL) {
				safe_delete( g_fGSSave );
				g_SaveGSStream = 0;
			}
			else {
				if (GSfreeze(FREEZE_SAVE, &fP) == -1) {
					safe_delete( g_fGSSave );
					g_SaveGSStream = 0;
				}
				else {
					g_fGSSave->Freeze( fP.size );
					if (fP.size) {
						g_fGSSave->FreezeMem( fP.data, fP.size );
						free(fP.data);
					}
				}
			}
		}
	}
	else if( g_SaveGSStream == 2 ) {

		if( --g_nLeftGSFrames <= 0 ) {
			safe_delete( g_fGSSave );
			g_SaveGSStream = 0;
			Console.WriteLn("Done saving GS stream");
		}
	}
#endif
}
