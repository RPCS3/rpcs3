#ifndef _PCSX2_PATHS_H_
#define _PCSX2_PATHS_H_


#define g_MaxPath 255			// 255 is safer with antiquated Win32 ASCII APIs.

#ifdef __LINUX__
extern char MAIN_DIR[g_MaxPath];
#endif
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

namespace Path
{
	extern void Combine( std::string& dest, const std::string& srcPath, const std::string& srcFile );
	extern bool isRooted( const std::string& path );
	extern bool isDirectory( const std::string& path );
	extern bool isFile( const std::string& path );
	extern bool Exists( const std::string& path );
	extern int getFileSize( const std::string& path );

	extern void ReplaceExtension( std::string& dest, const std::string& src, const std::string& ext );
	extern void ReplaceFilename( std::string& dest, const std::string& src, const std::string& newfilename );
	extern void GetFilename( const std::string& src, std::string& dest );
	extern void GetDirectory( const std::string& src, std::string& dest );
	extern void GetRootDirectory( const std::string& src, std::string& dest );
	extern void Split( const std::string& src, std::string& destpath, std::string& destfile );

	extern void CreateDirectory( const std::string& src );

}

#endif
