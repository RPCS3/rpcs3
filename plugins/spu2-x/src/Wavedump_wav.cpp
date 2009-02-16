//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <stdexcept>
#include <new>

#include "spu2.h"

#include "SoundTouch/WavFile.h"

static WavOutFile* _new_WavOutFile( const char* destfile )
{
	return new WavOutFile( destfile, 48000, 16, 2 );
}

namespace WaveDump
{
	static WavOutFile* m_CoreWav[2][CoreSrc_Count] = { NULL };

	static const char* m_tbl_CoreOutputTypeNames[CoreSrc_Count]  =
	{
		"Input",
		"DryVoiceMix",
		"WetVoiceMix",
		"PreReverb",
		"PostReverb",
		"External"
	};

	void Open()
	{
		if( !IsDevBuild ) return;
		if( !WaveLog() ) return;
		
		char wavfilename[256];

		for( uint cidx=0; cidx<2; cidx++ )
		{
			for( int srcidx=0; srcidx<CoreSrc_Count; srcidx++ )
			{
				SAFE_DELETE_OBJ( m_CoreWav[cidx][srcidx] );
				
				sprintf( wavfilename, "logs\\spu2x-Core%d-%s.wav",
					cidx, m_tbl_CoreOutputTypeNames[ srcidx ] );

				try
				{
					m_CoreWav[cidx][srcidx] = _new_WavOutFile( wavfilename );
				}
				catch( std::runtime_error& ex )
				{
					printf( "SPU2-X > %s.\n\tWave Log for this core source disabled.", ex.what() );
					m_CoreWav[cidx][srcidx] = NULL;
				}
			}
		}
	}

	void Close()
	{
		if( !IsDevBuild ) return;
		for( uint cidx=0; cidx<2; cidx++ )
		{
			for( int srcidx=0; srcidx<CoreSrc_Count; srcidx++ )
			{
				SAFE_DELETE_OBJ( m_CoreWav[cidx][srcidx]  );
			}
		}
	}

	void WriteCore( uint coreidx, CoreSourceType src, s16 left, s16 right )
	{
		if( !IsDevBuild ) return;
		if( m_CoreWav[coreidx][src] != NULL )
		{
			s16 buffer[2] = { left, right };
			m_CoreWav[coreidx][src]->write( buffer, 2 );
		}
	}
}

WavOutFile* m_wavrecord = NULL;

void RecordStart()
{
	SAFE_DELETE_OBJ( m_wavrecord );

	try
	{
		m_wavrecord = new WavOutFile( "recording.wav", 48000, 16, 2 );
	}
	catch( std::runtime_error& )
	{
		SysMessage("SPU2-X couldn't open file for recording: %s.\nRecording to wavfile disabled.", "recording.wav");
		m_wavrecord = NULL;	
	}
}

void RecordStop()
{
	SAFE_DELETE_OBJ( m_wavrecord );
}

void RecordWrite(s16 left, s16 right)
{
	if( m_wavrecord == NULL ) return;

	s16 buffer[2] = { left, right };
	m_wavrecord->write( buffer, 2 );
}
