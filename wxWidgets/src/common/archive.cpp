/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/archive.cpp
// Purpose:     Streams for archive formats
// Author:      Mike Wetherell
// RCS-ID:      $Id: archive.cpp 42508 2006-10-27 09:53:38Z MW $
// Copyright:   (c) Mike Wetherell
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STREAMS && wxUSE_ARCHIVE_STREAMS

#include "wx/archive.h"

IMPLEMENT_ABSTRACT_CLASS(wxArchiveEntry, wxObject)
IMPLEMENT_ABSTRACT_CLASS(wxArchiveClassFactory, wxFilterClassFactoryBase)


/////////////////////////////////////////////////////////////////////////////
// wxArchiveInputStream

wxArchiveInputStream::wxArchiveInputStream(wxInputStream& stream,
                                           wxMBConv& conv)
  : wxFilterInputStream(stream),
    m_conv(conv)
{
}

wxArchiveInputStream::wxArchiveInputStream(wxInputStream *stream,
                                           wxMBConv& conv)
  : wxFilterInputStream(stream),
    m_conv(conv)
{
}


/////////////////////////////////////////////////////////////////////////////
// wxArchiveOutputStream

wxArchiveOutputStream::wxArchiveOutputStream(wxOutputStream& stream,
                                             wxMBConv& conv)
  : wxFilterOutputStream(stream),
    m_conv(conv)
{
}

wxArchiveOutputStream::wxArchiveOutputStream(wxOutputStream *stream,
                                             wxMBConv& conv)
  : wxFilterOutputStream(stream),
    m_conv(conv)
{
}


/////////////////////////////////////////////////////////////////////////////
// wxArchiveEntry

void wxArchiveEntry::SetNotifier(wxArchiveNotifier& notifier)
{
    UnsetNotifier();
    m_notifier = &notifier;
    m_notifier->OnEntryUpdated(*this);
}

wxArchiveEntry& wxArchiveEntry::operator=(const wxArchiveEntry& WXUNUSED(e))
{
    m_notifier = NULL;
    return *this;
}


/////////////////////////////////////////////////////////////////////////////
// wxArchiveClassFactory

wxArchiveClassFactory *wxArchiveClassFactory::sm_first = NULL;

void wxArchiveClassFactory::Remove()
{
    if (m_next != this)
    {
        wxArchiveClassFactory **pp = &sm_first;

        while (*pp != this)
            pp = &(*pp)->m_next;

        *pp = m_next;

        m_next = this;
    }
}

#endif // wxUSE_STREAMS && wxUSE_ARCHIVE_STREAMS
