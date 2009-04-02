#ifndef _PCSX2_PATHS_H_
#define _PCSX2_PATHS_H_

#define g_MaxPath 255			// 255 is safer with antiquated Win32 ASCII APIs.

#ifdef __LINUX__
extern char MAIN_DIR[g_MaxPath];
#endif

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

// Windows.h namespace pollution!
#undef CreateDirectory

namespace Path
{
	extern bool isRooted( const wxString& path );
	extern bool isDirectory( const wxString& path );
	extern bool isFile( const wxString& path );
	extern bool Exists( const wxString& path );
	extern int getFileSize( const wxString& path );

	extern wxString Combine( const wxString& srcPath, const wxString& srcFile );
	extern wxString ReplaceExtension( const wxString& src, const wxString& ext );
	extern wxString ReplaceFilename( const wxString& src, const wxString& newfilename );
	extern wxString GetFilename( const wxString& src );
	extern wxString GetDirectory( const wxString& src );
	extern wxString GetFilenameWithoutExt( const string& src );
	extern wxString GetRootDirectory( const wxString& src );
	extern void Split( const wxString& src, wxString& destpath, wxString& destfile );

	extern void CreateDirectory( const wxString& src );

}

#endif
