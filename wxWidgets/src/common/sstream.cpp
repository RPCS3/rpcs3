///////////////////////////////////////////////////////////////////////////////
// Name:        common/sstream.cpp
// Purpose:     string-based streams implementation
// Author:      Vadim Zeitlin
// Modified by: Ryan Norton (UTF8 UNICODE)
// Created:     2004-09-19
// RCS-ID:      $Id: sstream.cpp 45772 2007-05-03 02:19:16Z VZ $
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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

#if wxUSE_STREAMS

#include "wx/sstream.h"

#if wxUSE_UNICODE
    #include "wx/hashmap.h"
#endif

// ============================================================================
// wxStringInputStream implementation
// ============================================================================

// ----------------------------------------------------------------------------
// construction/destruction
// ----------------------------------------------------------------------------

// TODO:  Do we want to include the null char in the stream?  If so then
// just add +1 to m_len in the ctor
wxStringInputStream::wxStringInputStream(const wxString& s)
#if wxUSE_UNICODE
    : m_str(s), m_buf(wxMBConvUTF8().cWX2MB(s).release()), m_len(strlen(m_buf))
#else
    : m_str(s), m_buf((char*)s.c_str()), m_len(s.length())
#endif
{
#if wxUSE_UNICODE
    wxASSERT_MSG(m_buf != NULL, _T("Could not convert string to UTF8!"));
#endif
    m_pos = 0;
}

wxStringInputStream::~wxStringInputStream()
{
#if wxUSE_UNICODE
    // Note: wx[W]CharBuffer uses malloc()/free()
    free(m_buf);
#endif
}

// ----------------------------------------------------------------------------
// getlength
// ----------------------------------------------------------------------------

wxFileOffset wxStringInputStream::GetLength() const
{
    return m_len;
}

// ----------------------------------------------------------------------------
// seek/tell
// ----------------------------------------------------------------------------

wxFileOffset wxStringInputStream::OnSysSeek(wxFileOffset ofs, wxSeekMode mode)
{
    switch ( mode )
    {
        case wxFromStart:
            // nothing to do, ofs already ok
            break;

        case wxFromEnd:
            ofs += m_len;
            break;

        case wxFromCurrent:
            ofs += m_pos;
            break;

        default:
            wxFAIL_MSG( _T("invalid seek mode") );
            return wxInvalidOffset;
    }

    if ( ofs < 0 || ofs > wx_static_cast(wxFileOffset, m_len) )
        return wxInvalidOffset;

    // FIXME: this can't be right
    m_pos = wx_truncate_cast(size_t, ofs);

    return ofs;
}

wxFileOffset wxStringInputStream::OnSysTell() const
{
    return wx_static_cast(wxFileOffset, m_pos);
}

// ----------------------------------------------------------------------------
// actual IO
// ----------------------------------------------------------------------------

size_t wxStringInputStream::OnSysRead(void *buffer, size_t size)
{
    const size_t sizeMax = m_len - m_pos;

    if ( size >= sizeMax )
    {
        if ( sizeMax == 0 )
        {
            m_lasterror = wxSTREAM_EOF;
            return 0;
        }

        size = sizeMax;
    }

    memcpy(buffer, m_buf + m_pos, size);
    m_pos += size;

    return size;
}

// ============================================================================
// wxStringOutputStream implementation
// ============================================================================

// ----------------------------------------------------------------------------
// seek/tell
// ----------------------------------------------------------------------------

wxFileOffset wxStringOutputStream::OnSysTell() const
{
    return wx_static_cast(wxFileOffset, m_pos);
}

// ----------------------------------------------------------------------------
// actual IO
// ----------------------------------------------------------------------------

#if wxUSE_UNICODE

// we can't add a member to wxStringOutputStream in 2.8 branch without breaking
// backwards binary compatibility, so we emulate it by using a hash indexed by
// wxStringOutputStream pointers

// can't use wxCharBuffer as it has incorrect copying semantics and doesn't
// store the length which we need here
WX_DECLARE_VOIDPTR_HASH_MAP(wxMemoryBuffer, wxStringStreamUnconvBuffers);

static wxStringStreamUnconvBuffers gs_unconverted;

wxStringOutputStream::~wxStringOutputStream()
{
    // TODO: check that nothing remains (i.e. the unconverted buffer is empty)?
    gs_unconverted.erase(this);
}

#endif // wxUSE_UNICODE

size_t wxStringOutputStream::OnSysWrite(const void *buffer, size_t size)
{
    const char *p = wx_static_cast(const char *, buffer);

#if wxUSE_UNICODE
    // the part of the string we have here may be incomplete, i.e. it can stop
    // in the middle of an UTF-8 character and so converting it would fail; if
    // this is the case, accumulate the part which we failed to convert until
    // we get the rest (and also take into account the part which we might have
    // left unconverted before)
    const char *src;
    size_t srcLen;
    wxMemoryBuffer& unconv = gs_unconverted[this];
    if ( unconv.GetDataLen() )
    {
        // append the new data to the data remaining since the last time
        unconv.AppendData(p, size);
        src = unconv;
        srcLen = unconv.GetDataLen();
    }
    else // no unconverted data left, avoid extra copy
    {
        src = p;
        srcLen = size;
    }

    wxWCharBuffer wbuf(m_conv.cMB2WC(src, srcLen, NULL /* out len */));
    if ( wbuf )
    {
        // conversion succeeded, clear the unconverted buffer
        unconv = wxMemoryBuffer(0);

        *m_str += wbuf;
    }
    else // conversion failed
    {
        // remember unconverted data if there had been none before (otherwise
        // we've already got it in the buffer)
        if ( src == p )
            unconv.AppendData(src, srcLen);

        // pretend that we wrote the data anyhow, otherwise the caller would
        // believe there was an error and this might not be the case, but do
        // not update m_pos as m_str hasn't changed
        return size;
    }
#else // !wxUSE_UNICODE
    // append directly, no conversion necessary
    m_str->Append(wxString(p, size));
#endif // wxUSE_UNICODE/!wxUSE_UNICODE

    // update position
    m_pos += size;

    // return number of bytes actually written
    return size;
}

#endif // wxUSE_STREAMS

