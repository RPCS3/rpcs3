/////////////////////////////////////////////////////////////////////////////
// Name:        wx/ffile.h
// Purpose:     wxFFile - encapsulates "FILE *" stream
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.07.99
// RCS-ID:      $Id: ffile.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef   _WX_FFILE_H_
#define   _WX_FFILE_H_

#include "wx/defs.h"        // for wxUSE_FFILE

#if wxUSE_FFILE

#include  "wx/string.h"
#include  "wx/filefn.h"
#include  "wx/convauto.h"

#include <stdio.h>

// ----------------------------------------------------------------------------
// class wxFFile: standard C stream library IO
//
// NB: for space efficiency this class has no virtual functions, including
//     dtor which is _not_ virtual, so it shouldn't be used as a base class.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFFile
{
public:
  // ctors
  // -----
    // def ctor
  wxFFile() { m_fp = NULL; }
    // open specified file (may fail, use IsOpened())
  wxFFile(const wxChar *filename, const wxChar *mode = wxT("r"));
    // attach to (already opened) file
  wxFFile(FILE *lfp) { m_fp = lfp; }

  // open/close
    // open a file (existing or not - the mode controls what happens)
  bool Open(const wxChar *filename, const wxChar *mode = wxT("r"));
    // closes the opened file (this is a NOP if not opened)
  bool Close();

  // assign an existing file descriptor and get it back from wxFFile object
  void Attach(FILE *lfp, const wxString& name = wxEmptyString)
    { Close(); m_fp = lfp; m_name = name; }
  void Detach() { m_fp = NULL; }
  FILE *fp() const { return m_fp; }

  // read/write (unbuffered)
    // read all data from the file into a string (useful for text files)
  bool ReadAll(wxString *str, const wxMBConv& conv = wxConvAuto());
    // returns number of bytes read - use Eof() and Error() to see if an error
    // occurred or not
  size_t Read(void *pBuf, size_t nCount);
    // returns the number of bytes written
  size_t Write(const void *pBuf, size_t nCount);
    // returns true on success
  bool Write(const wxString& s, const wxMBConv& conv = wxConvAuto())
  {
      const wxWX2MBbuf buf = s.mb_str(conv);
      size_t size = strlen(buf);
      return Write((const char *)buf, size) == size;
  }
    // flush data not yet written
  bool Flush();

  // file pointer operations (return ofsInvalid on failure)
    // move ptr ofs bytes related to start/current pos/end of file
  bool Seek(wxFileOffset ofs, wxSeekMode mode = wxFromStart);
    // move ptr to ofs bytes before the end
  bool SeekEnd(wxFileOffset ofs = 0) { return Seek(ofs, wxFromEnd); }
    // get current position in the file
  wxFileOffset Tell() const;
    // get current file length
  wxFileOffset Length() const;

  // simple accessors: note that Eof() and Error() may only be called if
  // IsOpened()!
    // is file opened?
  bool IsOpened() const { return m_fp != NULL; }
    // is end of file reached?
  bool Eof() const { return feof(m_fp) != 0; }
    // has an error occurred?
  bool Error() const { return ferror(m_fp) != 0; }
    // get the file name
  const wxString& GetName() const { return m_name; }
    // type such as disk or pipe
  wxFileKind GetKind() const { return wxGetFileKind(m_fp); }

  // dtor closes the file if opened
  ~wxFFile() { Close(); }

private:
  // copy ctor and assignment operator are private because it doesn't make
  // sense to copy files this way: attempt to do it will provoke a compile-time
  // error.
  wxFFile(const wxFFile&);
  wxFFile& operator=(const wxFFile&);

  FILE *m_fp;       // IO stream or NULL if not opened

  wxString m_name;  // the name of the file (for diagnostic messages)
};

#endif // wxUSE_FFILE

#endif // _WX_FFILE_H_

