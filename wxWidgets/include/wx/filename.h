/////////////////////////////////////////////////////////////////////////////
// Name:        wx/filename.h
// Purpose:     wxFileName - encapsulates a file path
// Author:      Robert Roebling, Vadim Zeitlin
// Modified by:
// Created:     28.12.00
// RCS-ID:      $Id: filename.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 2000 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef   _WX_FILENAME_H_
#define   _WX_FILENAME_H_

/*
    TODO:

    1. support for drives under Windows
    2. more file operations:
        a) chmod()
        b) [acm]time() - get and set
        c) rename()?
    3. SameFileAs() function to compare inodes under Unix
 */

#include "wx/arrstr.h"
#include "wx/filefn.h"
#include "wx/datetime.h"
#include "wx/intl.h"

#if wxUSE_FILE
class WXDLLIMPEXP_FWD_BASE wxFile;
#endif

#if wxUSE_FFILE
class WXDLLIMPEXP_FWD_BASE wxFFile;
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the various values for the path format: this mainly affects the path
// separator but also whether or not the path has the drive part (as under
// Windows)
enum wxPathFormat
{
    wxPATH_NATIVE = 0,      // the path format for the current platform
    wxPATH_UNIX,
    wxPATH_BEOS = wxPATH_UNIX,
    wxPATH_MAC,
    wxPATH_DOS,
    wxPATH_WIN = wxPATH_DOS,
    wxPATH_OS2 = wxPATH_DOS,
    wxPATH_VMS,

    wxPATH_MAX // Not a valid value for specifying path format
};

// the kind of normalization to do with the file name: these values can be
// or'd together to perform several operations at once
enum wxPathNormalize
{
    wxPATH_NORM_ENV_VARS = 0x0001,  // replace env vars with their values
    wxPATH_NORM_DOTS     = 0x0002,  // squeeze all .. and . and prepend cwd
    wxPATH_NORM_TILDE    = 0x0004,  // Unix only: replace ~ and ~user
    wxPATH_NORM_CASE     = 0x0008,  // if case insensitive => tolower
    wxPATH_NORM_ABSOLUTE = 0x0010,  // make the path absolute
    wxPATH_NORM_LONG =     0x0020,  // make the path the long form
    wxPATH_NORM_SHORTCUT = 0x0040,  // resolve the shortcut, if it is a shortcut
    wxPATH_NORM_ALL      = 0x00ff & ~wxPATH_NORM_CASE
};

// what exactly should GetPath() return?
enum
{
    wxPATH_GET_VOLUME    = 0x0001,  // include the volume if applicable
    wxPATH_GET_SEPARATOR = 0x0002   // terminate the path with the separator
};

// MkDir flags
enum
{
    wxPATH_MKDIR_FULL    = 0x0001   // create directories recursively
};

// error code of wxFileName::GetSize()
extern WXDLLIMPEXP_DATA_BASE(wxULongLong) wxInvalidSize;



// ----------------------------------------------------------------------------
// wxFileName: encapsulates a file path
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFileName
{
public:
    // constructors and assignment

        // the usual stuff
    wxFileName() { Clear(); }
    wxFileName(const wxFileName& filepath) { Assign(filepath); }

        // from a full filename: if it terminates with a '/', a directory path
        // is contructed (the name will be empty), otherwise a file name and
        // extension are extracted from it
    wxFileName( const wxString& fullpath, wxPathFormat format = wxPATH_NATIVE )
        { Assign( fullpath, format ); }

        // from a directory name and a file name
    wxFileName(const wxString& path,
               const wxString& name,
               wxPathFormat format = wxPATH_NATIVE)
        { Assign(path, name, format); }

        // from a volume, directory name, file base name and extension
    wxFileName(const wxString& volume,
               const wxString& path,
               const wxString& name,
               const wxString& ext,
               wxPathFormat format = wxPATH_NATIVE)
        { Assign(volume, path, name, ext, format); }

        // from a directory name, file base name and extension
    wxFileName(const wxString& path,
               const wxString& name,
               const wxString& ext,
               wxPathFormat format = wxPATH_NATIVE)
        { Assign(path, name, ext, format); }

        // the same for delayed initialization

    void Assign(const wxFileName& filepath);

    void Assign(const wxString& fullpath,
                wxPathFormat format = wxPATH_NATIVE);

    void Assign(const wxString& volume,
                const wxString& path,
                const wxString& name,
                const wxString& ext,
                bool hasExt,
                wxPathFormat format = wxPATH_NATIVE);

    void Assign(const wxString& volume,
                const wxString& path,
                const wxString& name,
                const wxString& ext,
                wxPathFormat format = wxPATH_NATIVE)
        { Assign(volume, path, name, ext, !ext.empty(), format); }

    void Assign(const wxString& path,
                const wxString& name,
                wxPathFormat format = wxPATH_NATIVE);

    void Assign(const wxString& path,
                const wxString& name,
                const wxString& ext,
                wxPathFormat format = wxPATH_NATIVE);

    void AssignDir(const wxString& dir, wxPathFormat format = wxPATH_NATIVE);

        // assorted assignment operators

    wxFileName& operator=(const wxFileName& filename)
        { Assign(filename); return *this; }

    wxFileName& operator=(const wxString& filename)
        { Assign(filename); return *this; }

        // reset all components to default, uninitialized state
    void Clear();

        // static pseudo constructors
    static wxFileName FileName(const wxString& file,
                               wxPathFormat format = wxPATH_NATIVE);
    static wxFileName DirName(const wxString& dir,
                              wxPathFormat format = wxPATH_NATIVE);

    // file tests

        // is the filename valid at all?
    bool IsOk() const
    {
        // we're fine if we have the path or the name or if we're a root dir
        return m_dirs.size() != 0 || !m_name.empty() || !m_relative ||
                !m_ext.empty() || m_hasExt;
    }

        // does the file with this name exists?
    bool FileExists() const;
    static bool FileExists( const wxString &file );

        // does the directory with this name exists?
    bool DirExists() const;
    static bool DirExists( const wxString &dir );

        // checks on most common flags for files/directories;
        // more platform-specific features (like e.g. Unix permissions) are not
        // available in wxFileName

    bool IsDirWritable() const { return wxIsWritable(GetPath()); }
    static bool IsDirWritable(const wxString &path) { return wxDirExists(path) && wxIsWritable(path); }

    bool IsDirReadable() const { return wxIsReadable(GetPath()); }
    static bool IsDirReadable(const wxString &path) { return wxDirExists(path) && wxIsReadable(path); }

    // NOTE: IsDirExecutable() is not present because the meaning of "executable"
    //       directory is very platform-dependent and also not so useful

    bool IsFileWritable() const { return wxIsWritable(GetFullPath()); }
    static bool IsFileWritable(const wxString &path) { return wxFileExists(path) && wxIsWritable(path); }

    bool IsFileReadable() const { return wxIsReadable(GetFullPath()); }
    static bool IsFileReadable(const wxString &path) { return wxFileExists(path) && wxIsReadable(path); }

    bool IsFileExecutable() const { return wxIsExecutable(GetFullPath()); }
    static bool IsFileExecutable(const wxString &path) { return wxFileExists(path) && wxIsExecutable(path); }


    // time functions
#if wxUSE_DATETIME
        // set the file last access/mod and creation times
        // (any of the pointers may be NULL)
    bool SetTimes(const wxDateTime *dtAccess,
                  const wxDateTime *dtMod,
                  const wxDateTime *dtCreate);

        // set the access and modification times to the current moment
    bool Touch();

        // return the last access, last modification and create times
        // (any of the pointers may be NULL)
    bool GetTimes(wxDateTime *dtAccess,
                  wxDateTime *dtMod,
                  wxDateTime *dtCreate) const;

        // convenience wrapper: get just the last mod time of the file
    wxDateTime GetModificationTime() const
    {
        wxDateTime dtMod;
        (void)GetTimes(NULL, &dtMod, NULL);
        return dtMod;
    }
#endif // wxUSE_DATETIME

#ifdef __WXMAC__
    bool MacSetTypeAndCreator( wxUint32 type , wxUint32 creator ) ;
    bool MacGetTypeAndCreator( wxUint32 *type , wxUint32 *creator ) ;
    // gets the 'common' type and creator for a certain extension
    static bool MacFindDefaultTypeAndCreator( const wxString& ext , wxUint32 *type , wxUint32 *creator ) ;
    // registers application defined extensions and their default type and creator
    static void MacRegisterDefaultTypeAndCreator( const wxString& ext , wxUint32 type , wxUint32 creator ) ;
    // looks up the appropriate type and creator from the registration and then sets
    bool MacSetDefaultTypeAndCreator() ;
#endif

    // various file/dir operations

        // retrieve the value of the current working directory
    void AssignCwd(const wxString& volume = wxEmptyString);
    static wxString GetCwd(const wxString& volume = wxEmptyString);

        // change the current working directory
    bool SetCwd();
    static bool SetCwd( const wxString &cwd );

        // get the value of user home (Unix only mainly)
    void AssignHomeDir();
    static wxString GetHomeDir();

        // get the system temporary directory
    static wxString GetTempDir();

#if wxUSE_FILE || wxUSE_FFILE
        // get a temp file name starting with the specified prefix
    void AssignTempFileName(const wxString& prefix);
    static wxString CreateTempFileName(const wxString& prefix);
#endif // wxUSE_FILE

#if wxUSE_FILE
        // get a temp file name starting with the specified prefix and open the
        // file passed to us using this name for writing (atomically if
        // possible)
    void AssignTempFileName(const wxString& prefix, wxFile *fileTemp);
    static wxString CreateTempFileName(const wxString& prefix,
                                       wxFile *fileTemp);
#endif // wxUSE_FILE

#if wxUSE_FFILE
        // get a temp file name starting with the specified prefix and open the
        // file passed to us using this name for writing (atomically if
        // possible)
    void AssignTempFileName(const wxString& prefix, wxFFile *fileTemp);
    static wxString CreateTempFileName(const wxString& prefix,
                                       wxFFile *fileTemp);
#endif // wxUSE_FFILE

    // directory creation and removal.
    bool Mkdir( int perm = 0777, int flags = 0);
    static bool Mkdir( const wxString &dir, int perm = 0777, int flags = 0 );

    bool Rmdir();
    static bool Rmdir( const wxString &dir );

    // operations on the path

        // normalize the path: with the default flags value, the path will be
        // made absolute, without any ".." and "." and all environment
        // variables will be expanded in it
        //
        // this may be done using another (than current) value of cwd
    bool Normalize(int flags = wxPATH_NORM_ALL,
                   const wxString& cwd = wxEmptyString,
                   wxPathFormat format = wxPATH_NATIVE);

        // get a path path relative to the given base directory, i.e. opposite
        // of Normalize
        //
        // pass an empty string to get a path relative to the working directory
        //
        // returns true if the file name was modified, false if we failed to do
        // anything with it (happens when the file is on a different volume,
        // for example)
    bool MakeRelativeTo(const wxString& pathBase = wxEmptyString,
                        wxPathFormat format = wxPATH_NATIVE);

        // make the path absolute
        //
        // this may be done using another (than current) value of cwd
    bool MakeAbsolute(const wxString& cwd = wxEmptyString,
                      wxPathFormat format = wxPATH_NATIVE)
        { return Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE |
                           wxPATH_NORM_TILDE, cwd, format); }

#if defined(__WIN32__) && !defined(__WXWINCE__) && wxUSE_OLE
        // if the path is a shortcut, return the target and optionally,
        // the arguments
    bool GetShortcutTarget(const wxString& shortcutPath,
                           wxString& targetFilename,
                           wxString* arguments = NULL);
#endif

    // Comparison

        // compares with the rules of the given platforms format
    bool SameAs(const wxFileName& filepath,
                wxPathFormat format = wxPATH_NATIVE) const;

        // compare with another filename object
    bool operator==(const wxFileName& filename) const
        { return SameAs(filename); }
    bool operator!=(const wxFileName& filename) const
        { return !SameAs(filename); }

        // compare with a filename string interpreted as a native file name
    bool operator==(const wxString& filename) const
        { return SameAs(wxFileName(filename)); }
    bool operator!=(const wxString& filename) const
        { return !SameAs(wxFileName(filename)); }

        // are the file names of this type cases sensitive?
    static bool IsCaseSensitive( wxPathFormat format = wxPATH_NATIVE );

        // is this filename absolute?
    bool IsAbsolute(wxPathFormat format = wxPATH_NATIVE) const;

        // is this filename relative?
    bool IsRelative(wxPathFormat format = wxPATH_NATIVE) const
        { return !IsAbsolute(format); }

    // Returns the characters that aren't allowed in filenames
    // on the specified platform.
    static wxString GetForbiddenChars(wxPathFormat format = wxPATH_NATIVE);

    // Information about path format

    // get the string separating the volume from the path for this format,
    // return an empty string if this format doesn't support the notion of
    // volumes at all
    static wxString GetVolumeSeparator(wxPathFormat format = wxPATH_NATIVE);

    // get the string of path separators for this format
    static wxString GetPathSeparators(wxPathFormat format = wxPATH_NATIVE);

    // get the string of path terminators, i.e. characters which terminate the
    // path
    static wxString GetPathTerminators(wxPathFormat format = wxPATH_NATIVE);

    // get the canonical path separator for this format
    static wxChar GetPathSeparator(wxPathFormat format = wxPATH_NATIVE)
        { return GetPathSeparators(format)[0u]; }

    // is the char a path separator for this format?
    static bool IsPathSeparator(wxChar ch, wxPathFormat format = wxPATH_NATIVE);

    // Dir accessors
    size_t GetDirCount() const { return m_dirs.size(); }
    void AppendDir(const wxString& dir);
    void PrependDir(const wxString& dir);
    void InsertDir(size_t before, const wxString& dir);
    void RemoveDir(size_t pos);
    void RemoveLastDir() { RemoveDir(GetDirCount() - 1); }

    // Other accessors
    void SetExt( const wxString &ext )          { m_ext = ext; m_hasExt = !m_ext.empty(); }
    void ClearExt()                             { m_ext = wxEmptyString; m_hasExt = false; }
    void SetEmptyExt()                          { m_ext = wxT(""); m_hasExt = true; }
    wxString GetExt() const                     { return m_ext; }
    bool HasExt() const                         { return m_hasExt; }

    void SetName( const wxString &name )        { m_name = name; }
    wxString GetName() const                    { return m_name; }
    bool HasName() const                        { return !m_name.empty(); }

    void SetVolume( const wxString &volume )    { m_volume = volume; }
    wxString GetVolume() const                  { return m_volume; }
    bool HasVolume() const                      { return !m_volume.empty(); }

    // full name is the file name + extension (but without the path)
    void SetFullName(const wxString& fullname);
    wxString GetFullName() const;

    const wxArrayString& GetDirs() const        { return m_dirs; }

    // flags are combination of wxPATH_GET_XXX flags
    wxString GetPath(int flags = wxPATH_GET_VOLUME,
                     wxPathFormat format = wxPATH_NATIVE) const;

    // Replace current path with this one
    void SetPath( const wxString &path, wxPathFormat format = wxPATH_NATIVE );

    // Construct full path with name and ext
    wxString GetFullPath( wxPathFormat format = wxPATH_NATIVE ) const;

    // Return the short form of the path (returns identity on non-Windows platforms)
    wxString GetShortPath() const;

    // Return the long form of the path (returns identity on non-Windows platforms)
    wxString GetLongPath() const;

    // Is this a file or directory (not necessarily an existing one)
    bool IsDir() const { return m_name.empty() && m_ext.empty(); }

    // various helpers

        // get the canonical path format for this platform
    static wxPathFormat GetFormat( wxPathFormat format = wxPATH_NATIVE );

        // split a fullpath into the volume, path, (base) name and extension
        // (all of the pointers can be NULL)
    static void SplitPath(const wxString& fullpath,
                          wxString *volume,
                          wxString *path,
                          wxString *name,
                          wxString *ext,
                          bool *hasExt = NULL,
                          wxPathFormat format = wxPATH_NATIVE);

    static void SplitPath(const wxString& fullpath,
                          wxString *volume,
                          wxString *path,
                          wxString *name,
                          wxString *ext,
                          wxPathFormat format)
    {
        SplitPath(fullpath, volume, path, name, ext, NULL, format);
    }

        // compatibility version: volume is part of path
    static void SplitPath(const wxString& fullpath,
                          wxString *path,
                          wxString *name,
                          wxString *ext,
                          wxPathFormat format = wxPATH_NATIVE);

        // split a path into volume and pure path part
    static void SplitVolume(const wxString& fullpathWithVolume,
                            wxString *volume,
                            wxString *path,
                            wxPathFormat format = wxPATH_NATIVE);

#if wxABI_VERSION >= 20811
        // strip the file extension
    static wxString StripExtension(const wxString& fullpath);
#endif // wxABI_VERSION >= 20811


    // Filesize

        // returns the size of the given filename
    wxULongLong GetSize() const;
    static wxULongLong GetSize(const wxString &file);

        // returns the size in a human readable form
    wxString GetHumanReadableSize(const wxString &nullsize = wxGetTranslation(wxT("Not available")),
                                  int precision = 1) const;
    static wxString GetHumanReadableSize(const wxULongLong &sz,
                                         const wxString &nullsize = wxGetTranslation(wxT("Not available")),
                                         int precision = 1);


    // deprecated methods, don't use any more
    // --------------------------------------

#ifndef __DIGITALMARS__
    wxString GetPath( bool withSep, wxPathFormat format = wxPATH_NATIVE ) const
        { return GetPath(withSep ? wxPATH_GET_SEPARATOR : 0, format); }
#endif
    wxString GetPathWithSep(wxPathFormat format = wxPATH_NATIVE ) const
        { return GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR, format); }

private:
    // check whether this dir is valid for Append/Prepend/InsertDir()
    static bool IsValidDirComponent(const wxString& dir);

    // the drive/volume/device specification (always empty for Unix)
    wxString        m_volume;

    // the path components of the file
    wxArrayString   m_dirs;

    // the file name and extension (empty for directories)
    wxString        m_name,
                    m_ext;

    // when m_dirs is empty it may mean either that we have no path at all or
    // that our path is '/', i.e. the root directory
    //
    // we use m_relative to distinguish between these two cases, it will be
    // true in the former and false in the latter
    //
    // NB: the path is not absolute just because m_relative is false, it still
    //     needs the drive (i.e. volume) in some formats (Windows)
    bool            m_relative;

    // when m_ext is empty, it may be because we don't have any extension or
    // because we have an empty extension
    //
    // the difference is important as file with name "foo" and without
    // extension has full name "foo" while with empty extension it is "foo."
    bool            m_hasExt;
};

#endif // _WX_FILENAME_H_

