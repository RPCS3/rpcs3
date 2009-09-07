/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#include "PrecompiledHeader.h"
#include "IniInterface.h"
#include "Config.h"
#include "GS.h"

#include <wx/fileconf.h>

// all speedhacks are disabled by default
Pcsx2Config::SpeedhackOptions::SpeedhackOptions() :
	bitset( 0 )
,	EECycleRate( 0 )
,	VUCycleSteal( 0 )
{
}

void Pcsx2Config::SpeedhackOptions::LoadSave( IniInterface& ini )
{
	SpeedhackOptions defaults;
	IniScopedGroup path( ini, L"Speedhacks" );

	IniBitfield( EECycleRate );
	IniBitfield( VUCycleSteal );
	IniBitBool( IopCycleRate_X2 );
	IniBitBool( IntcStat );
	IniBitBool( BIFC0 );
	IniBitBool( vuFlagHack );
	IniBitBool( vuMinMax );
}

void Pcsx2Config::ProfilerOptions::LoadSave( IniInterface& ini )
{
	ProfilerOptions defaults;
	IniScopedGroup path( ini, L"Profiler" );

	IniBitBool( Enabled );
	IniBitBool( RecBlocks_EE );
	IniBitBool( RecBlocks_IOP );
	IniBitBool( RecBlocks_VU0 );
	IniBitBool( RecBlocks_VU1 );
}

void Pcsx2Config::RecompilerOptions::LoadSave( IniInterface& ini )
{
	RecompilerOptions defaults;
	IniScopedGroup path( ini, L"Recompiler" );

	IniBitBool( EnableEE );
	IniBitBool( EnableIOP );
	IniBitBool( EnableVU0 );
	IniBitBool( EnableVU1 );
}

Pcsx2Config::CpuOptions::CpuOptions() :
	bitset( 0 )
,	sseMXCSR( DEFAULT_sseMXCSR )
,	sseVUMXCSR( DEFAULT_sseVUMXCSR )
{
	vuOverflow	= true;
	fpuOverflow	= true;
}

void Pcsx2Config::CpuOptions::LoadSave( IniInterface& ini )
{
	CpuOptions defaults;
	IniScopedGroup path( ini, L"CPU" );

	IniEntry( sseMXCSR );
	IniEntry( sseVUMXCSR );

	IniBitBool( vuOverflow );
	IniBitBool( vuExtraOverflow );
	IniBitBool( vuSignOverflow );
	IniBitBool( vuUnderflow );

	IniBitBool( fpuOverflow );
	IniBitBool( fpuExtraOverflow );
	IniBitBool( fpuFullMode );

	Recompiler.LoadSave( ini );
}

Pcsx2Config::VideoOptions::VideoOptions() :
	EnableFrameLimiting( false )
,	EnableFrameSkipping( false )
,	DefaultRegionMode( Region_NTSC )
,	FpsTurbo( 60*4 )
,	FpsLimit( 60 )
,	FpsSkip( 55 )
,	ConsecutiveFrames( 2 )
,	ConsecutiveSkip( 1 )
{
}

void Pcsx2Config::VideoOptions::LoadSave( IniInterface& ini )
{
	VideoOptions defaults;
	IniScopedGroup path( ini, L"Video" );

	IniEntry( EnableFrameLimiting );
	IniEntry( EnableFrameSkipping );

	static const wxChar * const ntsc_pal_str[2] =  { L"ntsc", L"pal" };
	ini.EnumEntry( L"DefaultRegionMode", DefaultRegionMode, ntsc_pal_str, defaults.DefaultRegionMode );

	IniEntry( FpsTurbo );
	IniEntry( FpsLimit );
	IniEntry( FpsSkip );
	IniEntry( ConsecutiveFrames );
	IniEntry( ConsecutiveSkip );
}

void Pcsx2Config::GamefixOptions::LoadSave( IniInterface& ini )
{
	GamefixOptions defaults;
	IniScopedGroup path( ini, L"Gamefixes" );

	IniBitBool( VuAddSubHack );
	IniBitBool( VuClipFlagHack );
	IniBitBool( FpuCompareHack );
	IniBitBool( FpuMulHack );
	IniBitBool( DMAExeHack );
	IniBitBool( XgKickHack );
	IniBitBool( MpegHack );
}

Pcsx2Config::Pcsx2Config() :
	bitset( 0 )
{
}

void Pcsx2Config::LoadSave( IniInterface& ini )
{
	Pcsx2Config defaults;
	IniScopedGroup path( ini, L"EmuCore" );

	IniBitBool( CdvdVerboseReads );
	IniBitBool( CdvdDumpBlocks );
	IniBitBool( EnablePatches );

	IniBitBool( closeGSonEsc );
	IniBitBool( McdEnableEjection );

	// Process various sub-components:

	Speedhacks.LoadSave( ini );
	Cpu.LoadSave( ini );
	Video.LoadSave( ini );
	Gamefixes.LoadSave( ini );
	Profiler.LoadSave( ini );

	ini.Flush();
}

void Pcsx2Config::Load( const wxString& srcfile )
{
	//m_IsLoaded = true;

	// Note: Extra parenthesis resolves "I think this is a function" issues with C++.
	wxFileConfig cfg( srcfile );
	IniLoader loader( (IniLoader( cfg )) );
	LoadSave( loader );
}

void Pcsx2Config::Save( const wxString& dstfile )
{
	//if( !m_IsLoaded ) return;

	wxFileConfig cfg( dstfile );
	IniSaver saver( (IniSaver( cfg )) );
	LoadSave( saver );
}
