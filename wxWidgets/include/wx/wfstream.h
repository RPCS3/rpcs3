/////////////////////////////////////////////////////////////////////////////
// Name:        wx/wfstream.h
// Purpose:     File stream classes
// Author:      Guilhem Lavaux
// Modified by:
// Created:     11/07/98
// RCS-ID:      $Id: wfstream.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WXFSTREAM_H__
#define _WX_WXFSTREAM_H__

#include "wx/defs.h"

#if wxUSE_STREAMS

#include "wx/object.h"
#include "wx/string.h"
#include "wx/stream.h"
#include "wx/file.h"
#include "wx/ffile.h"

#if wxUSE_FILE

// ----------------------------------------------------------------------------
// wxFileStream using wxFile
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFileInputStream : public wxInputStream
{
public:
    wxFileInputStream(const wxString& ifileName);
    wxFileInputStream(wxFile& file);
    wxFileInputStream(int fd);
    virtual ~wxFileInputStream();

    wxFileOffset GetLength() const;

    bool Ok() const { return IsOk(); }
    virtual bool IsOk() const;
    bool IsSeekable() const { return m_file->GetKind() == wxFILE_KIND_DISK; }

protected:
    wxFileInputStream();

    size_t OnSysRead(void *buffer, size_t size);
    wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode);
    wxFileOffset OnSysTell() const;

protected:
    wxFile *m_file;
    bool m_file_destroy;

    DECLARE_NO_COPY_CLASS(wxFileInputStream)
};

class WXDLLIMPEXP_BASE wxFileOutputStream : public wxOutputStream
{
public:
    wxFileOutputStream(const wxString& fileName);
    wxFileOutputStream(wxFile& file);
    wxFileOutputStream(int fd);
    virtual ~wxFileOutputStream();

    void Sync();
    bool Close() { return m_file_destroy ? m_file->Close() : true; }
    wxFileOffset GetLength() const;

    bool Ok() const { return IsOk(); }
    virtual bool IsOk() const;
    bool IsSeekable() const { return m_file->GetKind() == wxFILE_KIND_DISK; }

protected:
    wxFileOutputStream();

    size_t OnSysWrite(const void *buffer, size_t size);
    wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode);
    wxFileOffset OnSysTell() const;

protected:
    wxFile *m_file;
    bool m_file_destroy;

    DECLARE_NO_COPY_CLASS(wxFileOutputStream)
};

class WXDLLIMPEXP_BASE wxTempFileOutputStream : public wxOutputStream
{
public:
    wxTempFileOutputStream(const wxString& fileName);
    virtual ~wxTempFileOutputStream();

    bool Close() { return Commit(); }
    virtual bool Commit() { return m_file->Commit(); }
    virtual void Discard() { m_file->Discard(); }

    wxFileOffset GetLength() const { return m_file->Length(); }
    bool IsSeekable() const { return true; }

protected:
    size_t OnSysWrite(const void *buffer, size_t size);
    wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode)
        { return m_file->Seek(pos, mode); }
    wxFileOffset OnSysTell() const { return m_file->Tell(); }

private:
    wxTempFile *m_file;

    DECLARE_NO_COPY_CLASS(wxTempFileOutputStream)
};

class WXDLLIMPEXP_BASE wxFileStream : public wxFileInputStream,
                                      public wxFileOutputStream
{
public:
    wxFileStream(const wxString& fileName);

private:
    DECLARE_NO_COPY_CLASS(wxFileStream)
};

#endif //wxUSE_FILE

#if wxUSE_FFILE

// ----------------------------------------------------------------------------
// wxFFileStream using wxFFile
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFFileInputStream : public wxInputStream
{
public:
    wxFFileInputStream(const wxString& fileName, const wxChar *mode = wxT("rb"));
    wxFFileInputStream(wxFFile& file);
    wxFFileInputStream(FILE *file);
    virtual ~wxFFileInputStream();

    wxFileOffset GetLength() const;

    bool Ok() const { return IsOk(); }
    virtual bool IsOk() const;
    bool IsSeekable() const { return m_file->GetKind() == wxFILE_KIND_DISK; }

protected:
    wxFFileInputStream();

    size_t OnSysRead(void *buffer, size_t size);
    wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode);
    wxFileOffset OnSysTell() const;

protected:
    wxFFile *m_file;
    bool m_file_destroy;

    DECLARE_NO_COPY_CLASS(wxFFileInputStream)
};

class WXDLLIMPEXP_BASE wxFFileOutputStream : public wxOutputStream
{
public:
    wxFFileOutputStream(const wxString& fileName, const wxChar *mode = wxT("w+b"));
    wxFFileOutputStream(wxFFile& file);
    wxFFileOutputStream(FILE *file);
    virtual ~wxFFileOutputStream();

    void Sync();
    bool Close() { return m_file_destroy ? m_file->Close() : true; }
    wxFileOffset GetLength() const;

    bool Ok() const { return IsOk(); }
    virtual bool IsOk() const ;
    bool IsSeekable() const { return m_file->GetKind() == wxFILE_KIND_DISK; }

protected:
    wxFFileOutputStream();

    size_t OnSysWrite(const void *buffer, size_t size);
    wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode);
    wxFileOffset OnSysTell() const;

protected:
    wxFFile *m_file;
    bool m_file_destroy;

    DECLARE_NO_COPY_CLASS(wxFFileOutputStream)
};

class WXDLLIMPEXP_BASE wxFFileStream : public wxFFileInputStream,
                                       public wxFFileOutputStream
{
public:
    wxFFileStream(const wxString& fileName);

private:
    DECLARE_NO_COPY_CLASS(wxFFileStream)
};

#endif //wxUSE_FFILE

#endif // wxUSE_STREAMS

#endif // _WX_WXFSTREAM_H__
