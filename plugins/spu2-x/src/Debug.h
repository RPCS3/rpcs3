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
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

extern FILE *spu2Log;

extern void FileLog(const char *fmt, ...);
extern void ConLog(const char *fmt, ...);

extern void DoFullDump();

extern FILE* OpenBinaryLog( const wxString& logfile );
extern FILE* OpenLog( const wxString& logfile );
extern FILE* OpenDump( const wxString& logfile );

namespace WaveDump
{
	enum CoreSourceType
	{
		// Core's input stream, usually pulled from ADMA streams.
		CoreSrc_Input = 0

		// Output of the actual 24 input voices which have dry output enabled.
	,	CoreSrc_DryVoiceMix

		// Output of the actual 24 input voices that have wet output enabled.
	,	CoreSrc_WetVoiceMix

		// Wet mix including inputs and externals, prior to the application of reverb.
	,	CoreSrc_PreReverb

		// Wet mix after reverb has turned it into a pile of garbly gook.
	,	CoreSrc_PostReverb

		// Final output of the core.  For core 0, it's the feed into Core1.
		// For Core1, it's the feed into SndOut.
	,	CoreSrc_External

	,	CoreSrc_Count
	};

	extern void Open();
	extern void Close();
	extern void WriteCore( uint coreidx, CoreSourceType src, s16 left, s16 right );
	extern void WriteCore( uint coreidx, CoreSourceType src, const StereoOut16& sample );
}

using WaveDump::CoreSrc_Input;
using WaveDump::CoreSrc_DryVoiceMix;
using WaveDump::CoreSrc_WetVoiceMix;
using WaveDump::CoreSrc_PreReverb;
using WaveDump::CoreSrc_PostReverb;
using WaveDump::CoreSrc_External;

#endif // DEBUG_H_INCLUDED //
