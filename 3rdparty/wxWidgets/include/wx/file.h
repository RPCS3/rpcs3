/////////////////////////////////////////////////////////////////////////////
// Name:        file.h
// Purpose:     wxFile - encapsulates low-level "file descriptor"
//              wxTempFile - safely replace the old file
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: file.h 46331 2007-06-05 13:16:11Z JS $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FILEH__
#define _WX_FILEH__

#include  "wx/defs.h"

#if wxUSE_FILE

#include  "wx/string.h"
#include  "wx/filefn.h"
#include  "wx/strconv.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// we redefine these constants here because S_IREAD &c are _not_ standard
// however, we do assume that the values correspond to the Unix umask bits
#define wxS_IRUSR 00400
#define wxS_IWUSR 00200
#define wxS_IXUSR 00100

#define wxS_IRGRP 00040
#define wxS_IWGRP 00020
#define wxS_IXGRP 00010

#define wxS_IROTH 00004
#define wxS_IWOTH 00002
#define wxS_IXOTH 00001

// default mode for the new files: corresponds to umask 022
#define wxS_DEFAULT   (wxS_IRUSR | wxS_IWUSR | wxS_IRGRP | wxS_IWGRP |\
                       wxS_IROTH | wxS_IWOTH)

// ----------------------------------------------------------------------------
// class wxFile: raw file IO
//
// NB: for space efficiency this class has no virtual functions, including
//     dtor which is _not_ virtual, so it shouldn't be used as a base class.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFile
{
public:
  // more file constants
  // -------------------
    // opening mode
  enum OpenMode { read, write, read_write, write_append, write_excl };
    // standard values for file descriptor
  enum { fd_invalid = -1, fd_stdin, fd_stdout, fd_stderr };

  // static functions
  // ----------------
    // check whether a regular file by this name exists
  static bool Exists(const wxChar *name);
    // check whether we can access the given file in given mode
    // (only read and write make sense here)
  static bool Access(const wxChar *name, OpenMode mode);

  // ctors
  // -----
    // def ctor
  wxFile() { m_fd = fd_invalid; m_error = false; }
    // open specified file (may fail, use IsOpened())
  wxFile(const wxChar *szFileName, OpenMode mode = read);
    // attach to (already opened) file
  wxFile(int lfd) { m_fd = lfd; m_error = false; }

  // open/close
    // create a new file (with the default value of bOverwrite, it will fail if
    // the file already exists, otherwise it will overwrite it and succeed)
  bool Create(const wxChar *szFileName, bool bOverwrite = false,
              int access = wxS_DEFAULT);
  bool Open(const wxChar *szFileName, OpenMode mode = read,
            int access = wxS_DEFAULT);
  bool Close();  // Close is a NOP if not opened

  // assign an existing file descriptor and get it back from wxFile object
  void Attach(int lfd) { Close(); m_fd = lfd; m_error = false; }
  void Detach()       { m_fd = fd_invalid;  }
  int  fd() const { return m_fd; }

  // read/write (unbuffered)
    // returns number of bytes read or wxInvalidOffset on error
  ssize_t Read(void *pBuf, size_t nCount);
    // returns the number of bytes written
  size_t Write(const void *pBuf, size_t nCount);
    // returns true on success
  bool Write(const wxString& s, const wxMBConv& conv = wxConvUTF8)
  {
      const wxWX2MBbuf buf = s.mb_str(conv);
      if (!buf)
          return false;
      size_t size = strlen(buf);
      return Write((const char *) buf, size) == size;
  }
    // flush data not yet written
  bool Flush();

  // file pointer operations (return wxInvalidOffset on failure)
    // move ptr ofs bytes related to start/current offset/end of file
  wxFileOffset Seek(wxFileOffset ofs, wxSeekMode mode = wxFromStart);
    // move ptr to ofs bytes before the end
  wxFileOffset SeekEnd(wxFileOffset ofs = 0) { return Seek(ofs, wxFromEnd); }
    // get current offset
  wxFileOffset Tell() const;
    // get current file length
  wxFileOffset Length() const;

  // simple accessors
    // is file opened?
  bool IsOpened() const { return m_fd != fd_invalid; }
    // is end of file reached?
  bool Eof() const;
    // has an error occurred?
  bool Error() const { return m_error; }
    // type such as disk or pipe
  wxFileKind GetKind() const { return wxGetFileKind(m_fd); }

  // dtor closes the file if opened
  ~wxFile() { Close(); }

private:
  // copy ctor and assignment operator are private because
  // it doesn't make sense to copy files this way:
  // attempt to do it will provoke a compile-time error.
  wxFile(const wxFile&);
  wxFile& operator=(const wxFile&);

  int m_fd; // file descriptor or INVALID_FD if not opened
  bool m_error; // error memory
};

// ----------------------------------------------------------------------------
// class wxTempFile: if you want to replace another file, create an instance
// of wxTempFile passing the name of the file to be replaced to the ctor. Then
// you can write to wxTempFile and call Commit() function to replace the old
// file (and close this one) or call Discard() to cancel the modification. If
// you call neither of them, dtor will call Discard().
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxTempFile
{
public:
  // ctors
    // default
  wxTempFile() { }
    // associates the temp file with the file to be replaced and opens it
  wxTempFile(const wxString& strName);

  // open the temp file (strName is the name of file to be replaced)
  bool Open(const wxString& strName);

  // is the file opened?
  bool IsOpened() const { return m_file.IsOpened(); }
    // get current file length
  wxFileOffset Length() const { return m_file.Length(); }
    // move ptr ofs bytes related to start/current offset/end of file
  wxFileOffset Seek(wxFileOffset ofs, wxSeekMode mode = wxFromStart)
    { return m_file.Seek(ofs, mode); }
    // get current offset
  wxFileOffset Tell() const { return m_file.Tell(); }

  // I/O (both functions return true on success, false on failure)
  bool Write(const void *p, size_t n) { return m_file.Write(p, n) == n; }
  bool Write(const wxString& str, const wxMBConv& conv = wxConvUTF8)
    { return m_file.Write(str, conv); }

  // different ways to close the file
    // validate changes and delete the old file of name m_strName
  bool Commit();
    // discard changes
  void Discard();

  // dtor calls Discard() if file is still opened
 ~wxTempFile();

private:
  // no copy ctor/assignment operator
  wxTempFile(const wxTempFile&);
  wxTempFile& operator=(const wxTempFile&);

  wxString  m_strName,  // name of the file to replace in Commit()
            m_strTemp;  // temporary file name
  wxFile    m_file;     // the temporary file
};

#endif // wxUSE_FILE

#endif // _WX_FILEH__
