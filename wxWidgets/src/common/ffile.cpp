/////////////////////////////////////////////////////////////////////////////
// Name:        ffile.cpp
// Purpose:     wxFFile encapsulates "FILE *" IO stream
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.07.99
// RCS-ID:      $Id: ffile.cpp 63300 2010-01-28 21:36:09Z MW $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_FFILE

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
#endif

#ifdef __WINDOWS__
#include "wx/msw/mslu.h"
#endif

#include "wx/ffile.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// opening the file
// ----------------------------------------------------------------------------

wxFFile::wxFFile(const wxChar *filename, const wxChar *mode)
{
    Detach();

    (void)Open(filename, mode);
}

bool wxFFile::Open(const wxChar *filename, const wxChar *mode)
{
    wxASSERT_MSG( !m_fp, wxT("should close or detach the old file first") );

    m_fp = wxFopen(filename, mode);

    if ( !m_fp )
    {
        wxLogSysError(_("can't open file '%s'"), filename);

        return false;
    }

    m_name = filename;

    return true;
}

bool wxFFile::Close()
{
    if ( IsOpened() )
    {
        if ( fclose(m_fp) != 0 )
        {
            wxLogSysError(_("can't close file '%s'"), m_name.c_str());

            return false;
        }

        Detach();
    }

    return true;
}

// ----------------------------------------------------------------------------
// read/write
// ----------------------------------------------------------------------------

bool wxFFile::ReadAll(wxString *str, const wxMBConv& conv)
{
    wxCHECK_MSG( str, false, wxT("invalid parameter") );
    wxCHECK_MSG( IsOpened(), false, wxT("can't read from closed file") );
    wxCHECK_MSG( Length() >= 0, false, wxT("invalid length") );
    size_t length = wx_truncate_cast(size_t, Length());
    wxCHECK_MSG( (wxFileOffset)length == Length(), false, wxT("huge file not supported") );

    clearerr(m_fp);

    wxCharBuffer buf(length + 1);

    // note that real length may be less than file length for text files with DOS EOLs
    // ('\r's get dropped by CRT when reading which means that we have
    // realLen = fileLen - numOfLinesInTheFile)
    length = fread(buf.data(), sizeof(char), length, m_fp);

    if ( Error() ) 
    {
        wxLogSysError(_("Read error on file '%s'"), m_name.c_str());

        return false;
    }

    buf.data()[length] = 0;
    *str = wxString(buf, conv);

    return true;
}

size_t wxFFile::Read(void *pBuf, size_t nCount)
{
    wxCHECK_MSG( pBuf, 0, wxT("invalid parameter") );
    wxCHECK_MSG( IsOpened(), 0, wxT("can't read from closed file") );

    size_t nRead = fread(pBuf, 1, nCount, m_fp);
    if ( (nRead < nCount) && Error() )
    {
        wxLogSysError(_("Read error on file '%s'"), m_name.c_str());
    }

    return nRead;
}

size_t wxFFile::Write(const void *pBuf, size_t nCount)
{
    wxCHECK_MSG( pBuf, 0, wxT("invalid parameter") );
    wxCHECK_MSG( IsOpened(), 0, wxT("can't write to closed file") );

    size_t nWritten = fwrite(pBuf, 1, nCount, m_fp);
    if ( nWritten < nCount )
    {
        wxLogSysError(_("Write error on file '%s'"), m_name.c_str());
    }

    return nWritten;
}

bool wxFFile::Flush()
{
    if ( IsOpened() )
    {
        // fflush returns non-zero on error
        //
        if ( fflush(m_fp) )
        {
            wxLogSysError(_("failed to flush the file '%s'"), m_name.c_str());

            return false;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
// seeking
// ----------------------------------------------------------------------------

bool wxFFile::Seek(wxFileOffset ofs, wxSeekMode mode)
{
    wxCHECK_MSG( IsOpened(), false, wxT("can't seek on closed file") );

    int origin;
    switch ( mode )
    {
        default:
            wxFAIL_MSG(wxT("unknown seek mode"));
            // still fall through

        case wxFromStart:
            origin = SEEK_SET;
            break;

        case wxFromCurrent:
            origin = SEEK_CUR;
            break;

        case wxFromEnd:
            origin = SEEK_END;
            break;
    }

#ifndef wxHAS_LARGE_FFILES
    if ((long)ofs != ofs)
    {
        wxLogError(_("Seek error on file '%s' (large files not supported by stdio)"), m_name.c_str());

        return false;
    }

    if ( wxFseek(m_fp, (long)ofs, origin) != 0 )
#else
    if ( wxFseek(m_fp, ofs, origin) != 0 )
#endif
    {
        wxLogSysError(_("Seek error on file '%s'"), m_name.c_str());

        return false;
    }

    return true;
}

wxFileOffset wxFFile::Tell() const
{
    wxCHECK_MSG( IsOpened(), wxInvalidOffset,
                 _T("wxFFile::Tell(): file is closed!") );

    wxFileOffset rc = wxFtell(m_fp);
    if ( rc == wxInvalidOffset )
    {
        wxLogSysError(_("Can't find current position in file '%s'"),
                      m_name.c_str());
    }

    return rc;
}

wxFileOffset wxFFile::Length() const
{
    wxCHECK_MSG( IsOpened(), wxInvalidOffset,
                 _T("wxFFile::Length(): file is closed!") );

    wxFFile& self = *(wxFFile *)this;   // const_cast

    wxFileOffset posOld = Tell();
    if ( posOld != wxInvalidOffset )
    {
        if ( self.SeekEnd() )
        {
            wxFileOffset len = Tell();

            (void)self.Seek(posOld);

            return len;
        }
    }

    return wxInvalidOffset;
}

#endif // wxUSE_FFILE
