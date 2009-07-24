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

#ifndef _PCSX2_PATHS_H_
#define _PCSX2_PATHS_H_

#define g_MaxPath 255			// 255 is safer with antiquated Win32 ASCII APIs.

//#ifdef __LINUX__
//extern char MAIN_DIR[g_MaxPath];
//#endif

#define DEFAULT_INIS_DIR "inis"
#define DEFAULT_BIOS_DIR "bios"
#define DEFAULT_PLUGINS_DIR "plugins"

#define MEMCARDS_DIR "memcards"
#define PATCHES_DIR  "patches"

#define SSTATES_DIR "sstates"
#define LANGS_DIR "Langs"
#define LOGS_DIR "logs"
#define SNAPSHOTS_DIR "snaps"

#define DEFAULT_MEMCARD1 "Mcd001.ps2"
#define DEFAULT_MEMCARD2 "Mcd002.ps2"

namespace Path
{
	extern bool isRooted( const std::string& path );
	extern bool isDirectory( const std::string& path );
	extern bool isFile( const std::string& path );
	extern bool Exists( const std::string& path );
	extern int getFileSize( const std::string& path );

	extern std::string Combine( const std::string& srcPath, const std::string& srcFile );
	extern std::string ReplaceExtension( const std::string& src, const std::string& ext );
	extern std::string ReplaceFilename( const std::string& src, const std::string& newfilename );
	extern std::string GetFilename( const std::string& src );
	extern std::string GetDirectory( const std::string& src );
	extern std::string GetFilenameWithoutExt( const string& src );
	extern std::string GetRootDirectory( const std::string& src );
	extern void Split( const std::string& src, std::string& destpath, std::string& destfile );

	extern void CreateDirectory( const std::string& src );
	extern std::string GetWorkingDirectory(void);
	extern void ChangeDirectory(const std::string& src);
}

#endif
