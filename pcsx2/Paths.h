#ifndef _PCSX2_PATHS_H_
#define _PCSX2_PATHS_H_

#define CONFIG_DIR "inis"

#define DEFAULT_BIOS_DIR "bios"
#define DEFAULT_PLUGINS_DIR "plugins"

#define MEMCARDS_DIR "memcards"
#define PATCHES_DIR  "patches"

#define SSTATES_DIR "sstates"
#define LANGS_DIR "Langs"
#define LOGS_DIR "logs"

#define DEFAULT_MEMCARD1 "Mcd001.ps2"
#define DEFAULT_MEMCARD2 "Mcd002.ps2"

#define g_MaxPath 255			// 255 is safer with antiquated Win32 ASCII APIs.

namespace Path
{
	void Combine( string& dest, const string& srcPath, const string& srcFile );
	bool isRooted( const string& path );
	bool isDirectory( const string& path );
	bool isFile( const string& path );
	bool Exists( const string& path );
	int getFileSize( const string& path );

	void ReplaceExtension( string& dest, const string& src, const string& ext );
}

#endif
