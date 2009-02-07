/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2003  Pcsx2 Team
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PrecompiledHeader.h"
#include "win32.h"

#include "Common.h"
#include "Paths.h"

static const u32 IniVersion = 100;

const char* g_CustomConfigFile;
char g_WorkingFolder[g_MaxPath];		// Working folder at application startup

// Returns TRUE if the user has invoked the -cfg command line option.
static bool hasCustomConfig()
{
	return (g_CustomConfigFile != NULL) && (g_CustomConfigFile[0] != 0);
}

// Returns the FULL (absolute) path and filename of the configuration file.
static void GetConfigFilename( string& dest )
{
	if( hasCustomConfig() )
	{
		// Load a user-specified configuration.
		// If the configuration isn't found, fail outright (see below)

		Path::Combine( dest, g_WorkingFolder, g_CustomConfigFile );
	}
	else
	{
		// use the ini relative to the application's working directory.
		// Our current working directory can change, so we use the one we detected
		// at startup:

		Path::Combine( dest, g_WorkingFolder, CONFIG_DIR "\\pcsx2.ini" );
	}
}

class IniFile
{
protected:
	string m_filename;
	string m_section;

public:
	virtual ~IniFile() {}
	IniFile() : m_filename(), m_section("Misc")
	{
		GetConfigFilename( m_filename );
	}

	void SetCurrentSection( const string& newsection )
	{
		m_section = newsection;
	}

	virtual void Entry( const string& var, string& value, const string& defvalue=string() )=0;
	virtual void Entry( const string& var, char (&value)[g_MaxPath], const string& defvalue=string() )=0;
	virtual void Entry( const string& var, int& value, const int defvalue=0 )=0;
	virtual void Entry( const string& var, uint& value, const uint defvalue=0 )=0;
	virtual void Entry( const string& var, bool& value, const bool defvalue=0 )=0;
	virtual void EnumEntry( const string& var, int& value, const char* const* enumArray, const int defvalue=0 )=0;

	void DoConfig( PcsxConfig& Conf )
	{
		SetCurrentSection( "Misc" );

		Entry( "Patching", Conf.Patch, false );
		Entry( "GameFixes", Conf.GameFixes);
#ifdef PCSX2_DEVBUILD
		Entry( "DevLogFlags", varLog );
#endif

		//interface
		SetCurrentSection( "Interface" );
		Entry( "Bios", Conf.Bios );
		Entry( "Language", Conf.Lang );
		Entry( "PluginsDir", Conf.PluginsDir, DEFAULT_PLUGINS_DIR );
		Entry( "BiosDir", Conf.BiosDir, DEFAULT_BIOS_DIR );
		Entry( "CloseGsOnEscape", Conf.closeGSonEsc, true );

		SetCurrentSection( "Console" );
		Entry( "ConsoleWindow", Conf.PsxOut, true );
		Entry( "Profiler", Conf.Profiler, false );
		Entry( "CdvdVerbose", Conf.cdvdPrint, false );

		Entry( "ThreadPriority", Conf.ThPriority, THREAD_PRIORITY_NORMAL );
		Entry( "Memorycard1", Conf.Mcd1, MEMCARDS_DIR "\\" DEFAULT_MEMCARD1 );
		Entry( "Memorycard2", Conf.Mcd2, MEMCARDS_DIR "\\" DEFAULT_MEMCARD2 );

		SetCurrentSection( "Framelimiter" );
		Entry( "CustomFps", Conf.CustomFps );
		Entry( "FrameskipMode", Conf.CustomFrameSkip );
		Entry( "ConsecutiveFramesToRender", Conf.CustomConsecutiveFrames );
		Entry( "ConsecutiveFramesToSkip", Conf.CustomConsecutiveSkip );

		// Plugins are saved from the winConfig struct.
		// It contains the user config settings and not the
		// runtime cmdline overrides.

		SetCurrentSection( "Plugins" );

		Entry( "GS", Conf.GS );
		Entry( "SPU2", Conf.SPU2 );
		Entry( "CDVD", Conf.CDVD );
		Entry( "PAD1", Conf.PAD1 );
		Entry( "PAD2", Conf.PAD2 );
		Entry( "DEV9", Conf.DEV9 );
		Entry( "USB", Conf.USB );
		Entry( "FW", Conf.FW );

		//cpu
		SetCurrentSection( "Cpu" );
		Entry( "Options", Conf.Options, PCSX2_EEREC|PCSX2_VU0REC|PCSX2_VU1REC|PCSX2_GSMULTITHREAD );
		Entry( "sseMXCSR", Conf.sseMXCSR, DEFAULT_sseMXCSR );
		Entry( "sseVUMXCSR", Conf.sseVUMXCSR, DEFAULT_sseVUMXCSR );
		Entry( "eeOptions", Conf.eeOptions, DEFAULT_eeOptions );
		Entry( "vuOptions", Conf.vuOptions, DEFAULT_vuOptions );
		Entry( "SpeedHacks", Conf.Hacks );
	}
};

class IniFileLoader : public IniFile
{
protected:
	MemoryAlloc<char> m_workspace;

public:
	virtual ~IniFileLoader() {}
	IniFileLoader() : IniFile(),
		m_workspace( 4096, "IniFileLoader Workspace" )
	{
	}

	void Entry( const string& var, string& value, const string& defvalue=string() )
	{
		int retval = GetPrivateProfileString(
			m_section.c_str(), var.c_str(), defvalue.c_str(), m_workspace.GetPtr(), m_workspace.GetLength(), m_filename.c_str()
		);

		if( retval >= m_workspace.GetLength() - 2 )
			Console::Notice( "Loadini Warning > Possible truncated value on key '%hs'", params &var );
		value = m_workspace.GetPtr();
	}

	void Entry( const string& var, char (&value)[g_MaxPath], const string& defvalue=string() )
	{
		int retval = GetPrivateProfileString(
			m_section.c_str(), var.c_str(), defvalue.c_str(), value, sizeof( value ), m_filename.c_str()
		);

		if( retval >= sizeof(value) - 2 )
			Console::Notice( "Loadini Warning > Possible truncated value on key '%hs'", params &var );
	}

	void Entry( const string& var, int& value, const int defvalue=0 )
	{
		string retval;
		Entry( var, retval, to_string( defvalue ) );
		value = atoi( retval.c_str() );
	}

	void Entry( const string& var, uint& value, const uint defvalue=0 )
	{
		string retval;
		Entry( var, retval, to_string( defvalue ) );
		value = atoi( retval.c_str() );
	}

	void Entry( const string& var, bool& value, const bool defvalue=false )
	{
		string retval;
		Entry( var, retval, defvalue ? "enabled" : "disabled" );
		value = (retval == "enabled");
	}

	void EnumEntry( const string& var, int& value, const char* const* enumArray, const int defvalue=0 )
	{
		string retval;
		Entry( var, retval, enumArray[defvalue] );

		int i=0;
		while( enumArray[i] != NULL && ( retval != enumArray[i] ) ) i++;

		if( enumArray[i] == NULL )
		{
			Console::Notice( "Loadini Warning > Unrecognized value '%hs' on key '%hs'\n\tUsing the default setting of '%s'.",
				params &retval, &var, enumArray[defvalue] );
			value = defvalue;
		}
		else
			value = i;
	}

};

class IniFileSaver : public IniFile
{
public:
	virtual ~IniFileSaver() {}
	IniFileSaver() : IniFile()
	{
		char versionStr[20];
		_itoa( IniVersion, versionStr, 10 );
		WritePrivateProfileString( "Misc", "IniVersion", versionStr, m_filename.c_str() );
	}

	void Entry( const string& var, const string& value, const string& defvalue=string() )
	{
		WritePrivateProfileString( m_section.c_str(), var.c_str(), value.c_str(), m_filename.c_str() );
	}

	void Entry( const string& var, string& value, const string& defvalue=string() )
	{
		WritePrivateProfileString( m_section.c_str(), var.c_str(), value.c_str(), m_filename.c_str() );
	}

	void Entry( const string& var, char (&value)[g_MaxPath], const string& defvalue=string() )
	{
		WritePrivateProfileString( m_section.c_str(), var.c_str(), value, m_filename.c_str() );
	}

	void Entry( const string& var, int& value, const int defvalue=0 )
	{
		Entry( var, to_string( value ) );
	}

	void Entry( const string& var, uint& value, const uint defvalue=0 )
	{
		Entry( var, to_string( value ) );
	}

	void Entry( const string& var, bool& value, const bool defvalue=false )
	{
		Entry( var, value ? "enabled" : "disabled" );
	}

	void EnumEntry( const string& var, int& value, const char* const* enumArray, const int defvalue=0 )
	{
		Entry( var, enumArray[value] );
	}
};

bool LoadConfig()
{
	string szIniFile;
	bool status  = true;

	GetConfigFilename( szIniFile );

	if( !Path::Exists( szIniFile ) )
	{
		if( hasCustomConfig() )
		{
			// using custom config, so fail outright:
			throw Exception::FileNotFound(
				"User-specified configuration file not found:\n\t%s\n\nPCSX2 will now exit."
			);
		}

		// standard mode operation.  Create the directory.
		CreateDirectory( "inis", NULL ); 
		status = false;		// inform caller that we're not configured.
	}
	else
	{
		// sanity check to make sure the user doesn't have some kind of
		// crazy ass setup... why not!

		if( Path::isDirectory( szIniFile ) )
			throw Exception::Stream( 
				"Cannot open or create the Pcsx2 ini file because a directory of\n"
				"the same name already exists!  Please delete it or reinstall Pcsx2\n"
				"fresh and try again."
			);

		// Ini Version check! ---->
		// If the user's ini is old, give them a friendly warning that says they should
		// probably delete it.

		char versionStr[20];
		u32 version;

		GetPrivateProfileString( "Misc", "IniVersion", NULL, versionStr, 20, szIniFile.c_str() );
		version = atoi( versionStr );
		if( version < IniVersion )
		{
			// Warn the user of a version mismatch.
			Msgbox::Alert(
				"Configuration versions do not match.  Pcsx2 may be unstable.\n"
				"If you experience problems, delete the pcsx2-pg.ini file from the ini dir."
			);

			// save the new version -- gets rid of the warning on subsequent startups
			_itoa( IniVersion, versionStr, 10 );
			WritePrivateProfileString( "Misc", "IniVersion", versionStr, szIniFile.c_str() );
		}
	}

	IniFileLoader().DoConfig( Config );

#ifdef ENABLE_NLS
	{
		string text;
		extern int _nl_msg_cat_cntr;
		ssprintf(text, "LANGUAGE=%s", Config.Lang);
		gettext_putenv(text.c_str());
	}
#endif

	return status;
}

void SaveConfig()
{
	PcsxConfig tmpConf = Config;

	strcpy( tmpConf.GS, winConfig.GS );
	strcpy( tmpConf.SPU2, winConfig.SPU2 );
	strcpy( tmpConf.CDVD, winConfig.CDVD );
	strcpy( tmpConf.PAD1, winConfig.PAD1 );
	strcpy( tmpConf.PAD2, winConfig.PAD2 );
	strcpy( tmpConf.DEV9, winConfig.DEV9 );
	strcpy( tmpConf.USB, winConfig.USB );
	strcpy( tmpConf.FW, winConfig.FW );

	IniFileSaver().DoConfig( tmpConf );
}

