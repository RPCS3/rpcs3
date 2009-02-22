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
	void Combine( std::string& dest, const std::string& srcPath, const std::string& srcFile );
	bool isRooted( const std::string& path );
	bool isDirectory( const std::string& path );
	bool isFile( const std::string& path );
	bool Exists( const std::string& path );
	int getFileSize( const std::string& path );

	void ReplaceExtension( std::string& dest, const std::string& src, const std::string& ext );
	void ReplaceFilename( std::string& dest, const std::string& src, const std::string& newfilename );
	void GetFilename( const std::string& src, std::string& dest );
	void GetDirectory( const std::string& src, std::string& dest );
	void GetRootDirectory( const std::string& src, std::string& dest );
	void Split( const std::string& src, std::string& destpath, std::string& destfile );

}

#endif
