/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include <wx/fileconf.h>

#include "Utilities/IniInterface.h"
#include "Config.h"
#include "GS.h"

void TraceLogFilters::LoadSave( IniInterface& ini )
{
	TraceLogFilters defaults;
	IniScopedGroup path( ini, L"TraceLog" );

	IniEntry( Enabled );
	IniEntry( SIF );

	// Retaining backwards compat of the trace log enablers isn't really important, and
	// doing each one by hand would be murder.  So let's cheat and just save it as an int:

	IniEntry( EE.bitset );
	IniEntry( IOP.bitset );
}

// all speedhacks are disabled by default
Pcsx2Config::SpeedhackOptions::SpeedhackOptions()
{
	bitset			= 0;
	EECycleRate		= 0;
	VUCycleSteal	= 0;
}

ConsoleLogFilters::ConsoleLogFilters()
{
	ELF			= false;
	StdoutEE	= true;
	StdoutIOP	= true;
	Deci2		= true;
}

void ConsoleLogFilters::LoadSave( IniInterface& ini )
{
	ConsoleLogFilters defaults;
	IniScopedGroup path( ini, L"ConsoleLog" );

	IniBitBool( ELF );
	IniBitBool( StdoutEE );
	IniBitBool( StdoutIOP );
	IniBitBool( Deci2 );
}

void Pcsx2Config::SpeedhackOptions::LoadSave( IniInterface& ini )
{
	SpeedhackOptions defaults;
	IniScopedGroup path( ini, L"Speedhacks" );

	IniBitfield( EECycleRate );
	IniBitfield( VUCycleSteal );
	IniBitBool( IopCycleRate_X2 );
	IniBitBool( IntcStat );
	IniBitBool( WaitLoop );
	IniBitBool( vuFlagHack );
	IniBitBool( vuBlockHack );
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

Pcsx2Config::RecompilerOptions::RecompilerOptions()
{
	bitset		= 0;

	StackFrameChecks = false;

	// All recs are enabled by default.

	EnableEE	= true;
	EnableIOP	= true;
	EnableVU0	= true;
	EnableVU1	= true;

	UseMicroVU0	= true;
	UseMicroVU1	= true;

	// vu and fpu clamping default to standard overflow.
	vuOverflow	= true;
	//vuExtraOverflow = false;
	//vuSignOverflow = false;
	//vuUnderflow = false;

	fpuOverflow	= true;
	//fpuExtraOverflow = false;
	//fpuFullMode = false;
}

void Pcsx2Config::RecompilerOptions::ApplySanityCheck()
{
	bool fpuIsRight = true;

	if( fpuExtraOverflow )
		fpuIsRight = fpuOverflow;

	if( fpuFullMode )
		fpuIsRight = fpuOverflow && fpuExtraOverflow;

	if( !fpuIsRight )
	{
		// Values are wonky; assume the defaults.
		fpuOverflow		= RecompilerOptions().fpuOverflow;
		fpuExtraOverflow= RecompilerOptions().fpuExtraOverflow;
		fpuFullMode		= RecompilerOptions().fpuFullMode;
	}

	bool vuIsOk = true;

	if( vuExtraOverflow ) vuIsOk = vuIsOk && vuOverflow;
	if( vuSignOverflow ) vuIsOk = vuIsOk && vuExtraOverflow;

	if( !vuIsOk )
	{
		// Values are wonky; assume the defaults.
		vuOverflow		= RecompilerOptions().vuOverflow;
		vuExtraOverflow	= RecompilerOptions().vuExtraOverflow;
		vuSignOverflow	= RecompilerOptions().vuSignOverflow;
		vuUnderflow		= RecompilerOptions().vuUnderflow;
	}
}

void Pcsx2Config::RecompilerOptions::LoadSave( IniInterface& ini )
{
	RecompilerOptions defaults;
	IniScopedGroup path( ini, L"Recompiler" );

	IniBitBool( EnableEE );
	IniBitBool( EnableIOP );
	IniBitBool( EnableVU0 );
	IniBitBool( EnableVU1 );

	IniBitBool( UseMicroVU0 );
	IniBitBool( UseMicroVU1 );

	IniBitBool( vuOverflow );
	IniBitBool( vuExtraOverflow );
	IniBitBool( vuSignOverflow );
	IniBitBool( vuUnderflow );

	IniBitBool( fpuOverflow );
	IniBitBool( fpuExtraOverflow );
	IniBitBool( fpuFullMode );

	IniBitBool( StackFrameChecks );
}

Pcsx2Config::CpuOptions::CpuOptions()
{
	sseMXCSR.bitmask	= DEFAULT_sseMXCSR;
	sseVUMXCSR.bitmask	= DEFAULT_sseVUMXCSR;
}

void Pcsx2Config::CpuOptions::ApplySanityCheck()
{
	sseMXCSR.ClearExceptionFlags().DisableExceptions();
	sseVUMXCSR.ClearExceptionFlags().DisableExceptions();

	Recompiler.ApplySanityCheck();
}

void Pcsx2Config::CpuOptions::LoadSave( IniInterface& ini )
{
	CpuOptions defaults;
	IniScopedGroup path( ini, L"CPU" );

	IniBitBoolEx( sseMXCSR.DenormalsAreZero,	"FPU.DenormalsAreZero" );
	IniBitBoolEx( sseMXCSR.FlushToZero,			"FPU.FlushToZero" );
	IniBitfieldEx( sseMXCSR.RoundingControl,	"FPU.Roundmode" );

	IniBitBoolEx( sseVUMXCSR.DenormalsAreZero,	"VU.DenormalsAreZero" );
	IniBitBoolEx( sseVUMXCSR.FlushToZero,		"VU.FlushToZero" );
	IniBitfieldEx( sseVUMXCSR.RoundingControl,	"VU.Roundmode" );

	Recompiler.LoadSave( ini );
}

// Default GSOptions
Pcsx2Config::GSOptions::GSOptions()
{
	FrameLimitEnable		= true;
	FrameSkipEnable			= false;
	VsyncEnable				= false;

	SynchronousMTGS			= false;
	DisableOutput			= false;

	DefaultRegionMode		= Region_NTSC;
	FramesToDraw			= 2;
	FramesToSkip			= 2;

	LimitScalar				= 1.0;
	FramerateNTSC			= 59.94;
	FrameratePAL			= 50.0;
}

void Pcsx2Config::GSOptions::LoadSave( IniInterface& ini )
{
	GSOptions defaults;
	IniScopedGroup path( ini, L"GS" );

	IniEntry( SynchronousMTGS );
	IniEntry( DisableOutput );

	IniEntry( FrameLimitEnable );
	IniEntry( FrameSkipEnable );
	IniEntry( VsyncEnable );

	IniEntry( LimitScalar );
	IniEntry( FramerateNTSC );
	IniEntry( FrameratePAL );

	static const wxChar * const ntsc_pal_str[2] =  { L"ntsc", L"pal" };
	ini.EnumEntry( L"DefaultRegionMode", DefaultRegionMode, ntsc_pal_str, defaults.DefaultRegionMode );

	IniEntry( FramesToDraw );
	IniEntry( FramesToSkip );
}

const wxChar *const tbl_GamefixNames[] =
{
	L"VuAddSub",
	L"VuClipFlag",
	L"FpuCompare",
	L"FpuMul",
	L"FpuNegDiv",
	L"XGKick",
	L"IpuWait",
	L"EETiming",
	L"SkipMpeg"
};

const __forceinline wxChar* EnumToString( GamefixId id )
{
	return tbl_GamefixNames[id];
}

// Enables a full list of gamefixes.  The list can be either comma or pipe-delimited.
//   Example:  "XGKick,IpuWait"  or  "EEtiming,FpuCompare"
// If an unrecognized tag is encountered, a warning is printed to the console, but no error
// is generated.  This allows the system to function in the event that future versions of
// PCSX2 remove old hacks once they become obsolete.
void Pcsx2Config::GamefixOptions::Set( const wxString& list, bool enabled )
{
	wxStringTokenizer izer( list, L",|", wxTOKEN_STRTOK );
	
	while( izer.HasMoreTokens() )
	{
		wxString token( izer.GetNextToken() );

		GamefixId i;
		for (i=GamefixId_FIRST; i < pxEnumEnd; ++i)
		{
			if( token.CmpNoCase( EnumToString(i) ) == 0 ) break;
		}
		if( i < pxEnumEnd ) Set( i );
	}
}

void Pcsx2Config::GamefixOptions::Set( GamefixId id, bool enabled )
{
	EnumAssume( id );
	switch(id)
	{
		case Fix_VuAddSub:		VuAddSubHack		= enabled;	break;
		case Fix_VuClipFlag:	VuClipFlagHack		= enabled;	break;
		case Fix_FpuCompare:	FpuCompareHack		= enabled;	break;
		case Fix_FpuMultiply:	FpuMulHack			= enabled;	break;
		case Fix_FpuNegDiv:		FpuNegDivHack		= enabled;	break;
		case Fix_XGKick:		XgKickHack			= enabled;	break;
		case Fix_IpuWait:		IPUWaitHack			= enabled;	break;
		case Fix_EETiming:		EETimingHack		= enabled;	break;
		case Fix_SkipMpeg:		SkipMPEGHack		= enabled;	break;

		jNO_DEFAULT;
	}
}

bool Pcsx2Config::GamefixOptions::Get( GamefixId id ) const
{
	EnumAssume( id );
	switch(id)
	{
		case Fix_VuAddSub:		return VuAddSubHack;
		case Fix_VuClipFlag:	return VuClipFlagHack;
		case Fix_FpuCompare:	return FpuCompareHack;
		case Fix_FpuMultiply:	return FpuMulHack;
		case Fix_FpuNegDiv:		return FpuNegDivHack;
		case Fix_XGKick:		return XgKickHack;
		case Fix_IpuWait:		return IPUWaitHack;
		case Fix_EETiming:		return EETimingHack;
		case Fix_SkipMpeg:		return SkipMPEGHack;
		
		jNO_DEFAULT
	}
	return false;		// unreachable, but we still need to suppress warnings >_<
}

void Pcsx2Config::GamefixOptions::LoadSave( IniInterface& ini )
{
	GamefixOptions defaults;
	IniScopedGroup path( ini, L"Gamefixes" );

	IniBitBool( VuAddSubHack );
	IniBitBool( VuClipFlagHack );
	IniBitBool( FpuCompareHack );
	IniBitBool( FpuMulHack );
	IniBitBool( FpuNegDivHack );
	IniBitBool( XgKickHack );
	IniBitBool( IPUWaitHack );
	IniBitBool( EETimingHack );
	IniBitBool( SkipMPEGHack );
}

Pcsx2Config::Pcsx2Config()
{
	bitset = 0;
	McdEnableEjection = true;
}

void Pcsx2Config::LoadSave( IniInterface& ini )
{
	Pcsx2Config defaults;
	IniScopedGroup path( ini, L"EmuCore" );

	IniBitBool( CdvdVerboseReads );
	IniBitBool( CdvdDumpBlocks );
	IniBitBool( EnablePatches );
	IniBitBool( EnableCheats );
	IniBitBool( ConsoleToStdio );
	IniBitBool( HostFs );

	IniBitBool( McdEnableEjection );
	IniBitBool( MultitapPort0_Enabled );
	IniBitBool( MultitapPort1_Enabled );

	// Process various sub-components:

	Speedhacks		.LoadSave( ini );
	Cpu				.LoadSave( ini );
	GS				.LoadSave( ini );
	Gamefixes		.LoadSave( ini );
	Profiler		.LoadSave( ini );

	Trace			.LoadSave( ini );
	Log				.LoadSave( ini );

	ini.Flush();
}

bool Pcsx2Config::MultitapEnabled( uint port ) const
{
	pxAssert( port < 2 );
	return (port==0) ? MultitapPort0_Enabled : MultitapPort1_Enabled;
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
