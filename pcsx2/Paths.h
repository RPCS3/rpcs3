#ifndef _PCSX2_PATHS_H_
#define _PCSX2_PATHS_H_

#define g_MaxPath 255			// 255 is safer with antiquated Win32 ASCII APIs.

#ifdef __LINUX__
extern char MAIN_DIR[g_MaxPath];
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Obsolete Values in wxWidgets Branch!
// The following set of macros has been superceeded by the PathDefs and FilenameDefs namespaces,
// and furethermore by a set of user-configurable paths in g_Conf.
//
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

//////////////////////////////////////////////////////////////////////////////////////////
//
class wxDirName : protected wxFileName
{
public:
	explicit wxDirName( const wxFileName& src )
	{
		wxASSERT_MSG( src.IsDir(), L"Warning: Creating a directory from what looks to be a filename..." );
		Assign( src.GetPath() );
	}
	
	wxDirName() : wxFileName() {}
	wxDirName( const wxDirName& src ) : wxFileName( src ) { }
	explicit wxDirName( const char* src ) { Assign( wxString::FromAscii(src) ); }
	explicit wxDirName( const wxString& src ) { Assign( src ); }

	// ------------------------------------------------------------------------
	void Assign( const wxString& volume, const wxString& path )
	{
		wxFileName::Assign( volume, path, wxEmptyString );
	}

	void Assign( const wxString& path )
	{
		wxFileName::Assign( path, wxEmptyString );
	}

	void Assign( const wxDirName& path )
	{
		wxFileName::Assign( path );
	}

	void Clear() { wxFileName::Clear(); }
	
	wxCharBuffer ToAscii() const { return GetPath().ToAscii(); }
	wxString ToString() const { return GetPath(); }

	// ------------------------------------------------------------------------
	bool IsWritable() const { return IsDirWritable(); }
	bool IsReadable() const { return IsDirReadable(); }
	bool Exists() const { return DirExists(); }
	bool IsOk() const { return wxFileName::IsOk(); }

	bool SameAs( const wxDirName& filepath ) const
	{
		return wxFileName::SameAs( filepath );
	}

	// Returns the number of sub folders in this directory path
	size_t GetCount() const { return GetDirCount(); }

	// ------------------------------------------------------------------------
	wxFileName Combine( const wxFileName& right ) const;
	wxDirName Combine( const wxDirName& right ) const;

	// removes the lastmost directory from the path
	void RemoveLast() { wxFileName::RemoveDir(GetCount() - 1); }

	wxDirName& Normalize( int flags = wxPATH_NORM_ALL, const wxString& cwd = wxEmptyString );
	wxDirName& MakeRelativeTo( const wxString& pathBase = wxEmptyString );
	wxDirName& MakeAbsolute( const wxString& cwd = wxEmptyString );

	// ------------------------------------------------------------------------

	void AssignCwd( const wxString& volume = wxEmptyString ) { wxFileName::AssignCwd( volume ); }
	bool SetCwd() { wxFileName::SetCwd(); }

	// wxWidgets is missing the const qualifier for this one!  Shame!
	void Rmdir();
	bool Mkdir();

	// ------------------------------------------------------------------------
	
	wxDirName& operator=(const wxDirName& dirname)	{ Assign( dirname ); return *this; }
	wxDirName& operator=(const wxString& dirname)	{ Assign( dirname ); return *this; }
	wxDirName& operator=(const char* dirname)		{ Assign( wxString::FromAscii(dirname) ); return *this; }

	wxFileName operator+( const wxFileName& right ) const	{ return Combine( right ); }
	wxDirName operator+( const wxDirName& right )  const	{ return Combine( right ); }
	
	bool operator==(const wxDirName& filename) const { return SameAs(filename); }
	bool operator!=(const wxDirName& filename) const { return !SameAs(filename); }

	bool operator==(const wxFileName& filename) const { return SameAs(wxDirName(filename)); }
	bool operator!=(const wxFileName& filename) const { return !SameAs(wxDirName(filename)); }

	// compare with a filename string interpreted as a native file name
	bool operator==(const wxString& filename) const { return SameAs(wxDirName(filename)); }
	bool operator!=(const wxString& filename) const { return !SameAs(wxDirName(filename)); }
	
	const wxFileName& GetFilename() const { return *this; }
	wxFileName& GetFilename() { return *this; }
};

// remove windows.h namespace pollution:
#undef GetFileSize
#undef CreateDirectory

//////////////////////////////////////////////////////////////////////////////////////////
// Path Namespace
// Cross-platform utilities for manipulation of paths and filenames.
//
namespace Path
{
	extern bool IsRooted( const wxString& path );
	extern bool IsDirectory( const wxString& path );
	extern bool IsFile( const wxString& path );
	extern bool Exists( const wxString& path );
	extern int GetFileSize( const wxString& path );

	extern wxString Combine( const wxString& srcPath, const wxString& srcFile );
	extern wxString Combine( const wxDirName& srcPath, const wxFileName& srcFile );
	extern wxString Combine( const wxString& srcPath, const wxDirName& srcFile );
	extern wxString ReplaceExtension( const wxString& src, const wxString& ext );
	extern wxString ReplaceFilename( const wxString& src, const wxString& newfilename );
	extern wxString GetFilename( const wxString& src );
	extern wxString GetDirectory( const wxString& src );
	extern wxString GetFilenameWithoutExt( const string& src );
	extern wxString GetRootDirectory( const wxString& src );

	extern void CreateDirectory( const wxString& src );
	extern void RemoveDirectory( const wxString& src );
}

//////////////////////////////////////////////////////////////////////////////////////////
// PathDefs Namespace -- contains default values for various pcsx2 path names and locations.
//
// Note: The members of this namespace are intended for default value initialization only.
// Most of the time you should use the path folder assignments in g_Conf instead, since those
// are user-configurable.
//
namespace PathDefs
{
	extern const wxDirName Snapshots;
	extern const wxDirName Savestates;
	extern const wxDirName MemoryCards;
	extern const wxDirName Settings;
	extern const wxDirName Plugins;
	extern const wxDirName Themes;
	
	extern wxDirName GetDocuments();
	extern wxDirName GetSnapshots();
	extern wxDirName GetBios();
	extern wxDirName GetThemes();
	extern wxDirName GetPlugins();
	extern wxDirName GetSavestates();
	extern wxDirName GetMemoryCards();
	extern wxDirName GetSettings();
}

namespace FilenameDefs
{
	extern wxFileName GetConfig();
	extern const wxFileName Memcard[2];
};

#endif
