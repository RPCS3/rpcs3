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

#include "GS.h"
#include <list>

#ifdef PCSX2_DEVBUILD
void SaveGSState(const wxString& file)
{
	if( g_SaveGSStream ) return;

	Console::WriteLn( "Saving GS State..." );
	Console::WriteLn( wxsFormat( L"\t%s", file.c_str() ) );

	g_fGSSave = new gzSavingState( file );

	g_SaveGSStream = 1;
	g_nLeftGSFrames = 2;

	g_fGSSave->Freeze( g_nLeftGSFrames );
}

void LoadGSState(const wxString& file)
{
	int ret;
	gzLoadingState* f;

	Console::Status( "Loading GS State..." );

	try
	{
		f = new gzLoadingState( file );
	}
	catch( Exception::FileNotFound& )
	{
		// file not found? try prefixing with sstates folder:
		if( !Path::IsRelative( file ) )
		{
			//f = new gzLoadingState( Path::Combine( g_Conf->Folders.Savestates, file ) );

			// If this load attempt fails, then let the exception bubble up to
			// the caller to deal with...
		}
	}

	// Always set gsIrq callback -- GS States are always exclusionary of MTGS mode
	GSirqCallback( gsIrq );

	ret = GSopen(&pDsp, "PCSX2", 0);
	if (ret != 0)
	{
		delete f;
		throw Exception::PluginOpenError( PluginId_GS );
	}

	ret = PADopen((void *)&pDsp);

	f->Freeze(g_nLeftGSFrames);
	f->gsFreeze();

	f->FreezePlugin( "GS", gsSafeFreeze );

	RunGSState( *f );

	delete( f );

	g_plugins->Close( PluginId_GS );
	g_plugins->Close( PluginId_PAD );
}

struct GSStatePacket
{
	u32 type;
	std::vector<u8> mem;
};

// runs the GS
// (this should really be part of the AppGui)
void RunGSState( gzLoadingState& f )
{
	u32 newfield;
	std::list< GSStatePacket > packets;

	while( !f.Finished() )
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
}
#endif
