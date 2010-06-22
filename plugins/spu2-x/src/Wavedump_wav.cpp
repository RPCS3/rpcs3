/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Global.h"
#include "soundtouch/WavFile.h"

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
				safe_delete( m_CoreWav[cidx][srcidx] );
#ifdef __LINUX__
				sprintf( wavfilename, "logs/spu2x-Core%d-%s.wav",
					cidx, m_tbl_CoreOutputTypeNames[ srcidx ] );
#else
				sprintf( wavfilename, "logs\\spu2x-Core%d-%s.wav",
					cidx, m_tbl_CoreOutputTypeNames[ srcidx ] );
#endif

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
				safe_delete( m_CoreWav[cidx][srcidx]  );
			}
		}
	}

	void WriteCore( uint coreidx, CoreSourceType src, const StereoOut16& sample )
	{
		if( !IsDevBuild ) return;
		if( m_CoreWav[coreidx][src] != NULL )
			m_CoreWav[coreidx][src]->write( (s16*)&sample, 2 );
	}

	void WriteCore( uint coreidx, CoreSourceType src, s16 left, s16 right )
	{
		WriteCore( coreidx, src, StereoOut16( left, right ) );
	}
}

#include "Utilities/Threading.h"

using namespace Threading;

bool WavRecordEnabled = false;

static WavOutFile*		m_wavrecord = NULL;
static Mutex			WavRecordMutex;

void RecordStart()
{
	WavRecordEnabled = false;

	try
	{
		ScopedLock lock( WavRecordMutex );
		safe_delete( m_wavrecord );
		m_wavrecord = new WavOutFile( "recording.wav", 48000, 16, 2 );
		WavRecordEnabled = true;
	}
	catch( std::runtime_error& )
	{
		m_wavrecord = NULL;		// not needed, but what the heck. :)
		SysMessage("SPU2-X couldn't open file for recording: %s.\nRecording to wavfile disabled.", "recording.wav");
	}
}

void RecordStop()
{
	WavRecordEnabled = false;
	ScopedLock lock( WavRecordMutex );
	safe_delete( m_wavrecord );
}

void RecordWrite( const StereoOut16& sample )
{
	ScopedLock lock( WavRecordMutex );
	if( m_wavrecord == NULL ) return;
	m_wavrecord->write( (s16*)&sample, 2 );
}
