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
#include <wx/fileconf.h>

void Pcsx2Config::SpeedhackOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"Speedhacks" );

	IniBitfield( EECycleRate,		0 );
	IniBitfield( VUCycleSteal,		0 );
	IniBitBool( IopCycleRate_X2,	false );
	IniBitBool( IntcStat,			false );
	IniBitBool( BIFC0,				false );

	ini.SetPath( L".." );
}

void Pcsx2Config::ProfilerOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"Profiler" );
	
	IniBitBool( Enabled,		false );
	IniBitBool( RecBlocks_EE,	true );
	IniBitBool( RecBlocks_IOP,	true );
	IniBitBool( RecBlocks_VU0,	true );
	IniBitBool( RecBlocks_VU1,	true );
	
	ini.SetPath( L".." );
}

void Pcsx2Config::RecompilerOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"Recompiler" );
	
	IniBitBool( EnableEE,	true );
	IniBitBool( EnableIOP,	true );
	IniBitBool( EnableVU0,	true );
	IniBitBool( EnableVU1,	true );
	
	ini.SetPath( L".." );
}

void Pcsx2Config::CpuOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"CPU" );

	IniEntry( sseMXCSR,				DEFAULT_sseMXCSR );
	IniEntry( sseVUMXCSR,			DEFAULT_sseVUMXCSR );

	IniBitBool( vuOverflow,			true );
	IniBitBool( vuExtraOverflow,	false );
	IniBitBool( vuSignOverflow,		false );
	IniBitBool( vuUnderflow,		false );

	IniBitBool( fpuOverflow,		true );
	IniBitBool( fpuExtraOverflow,	false );
	IniBitBool( fpuFullMode,		false );

	Recompiler.LoadSave( ini );
	
	ini.SetPath( L".." );
}

void Pcsx2Config::VideoOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"Video" );
	
	ini.SetPath( L".." );
}

void Pcsx2Config::GamefixOptions::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"Video" );

	ini.SetPath( L".." );
}

void Pcsx2Config::LoadSave( IniInterface& ini )
{
	ini.SetPath( L"EmuCore" );
	
	IniBitBool( CdvdVerboseReads,		false );
	IniBitBool( CdvdDumpBlocks,			false );
	IniBitBool( EnablePatches,			false );

	IniBitBool( closeGSonEsc,			false );
	IniBitBool( McdEnableEjection,		false );

	// Process various sub-components:

	Speedhacks.LoadSave( ini );
	Cpu.LoadSave( ini );
	Video.LoadSave( ini );
	Gamefixes.LoadSave( ini );
	Profiler.LoadSave( ini );
	
	ini.SetPath( L".." );
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
