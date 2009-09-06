/////////////////////////////////////////////////////////////////////////////
// Name:        datstrm.h
// Purpose:     Data stream classes
// Author:      Guilhem Lavaux
// Modified by: Mickael Gilabert
// Created:     28/06/1998
// RCS-ID:      $Id: datstrm.h 38576 2006-04-05 16:10:08Z VZ $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DATSTREAM_H_
#define _WX_DATSTREAM_H_

#include "wx/stream.h"
#include "wx/longlong.h"
#include "wx/convauto.h"

#if wxUSE_STREAMS

class WXDLLIMPEXP_BASE wxDataInputStream
{
public:
#if wxUSE_UNICODE
    wxDataInputStream(wxInputStream& s, const wxMBConv& conv = wxConvAuto());
#else
    wxDataInputStream(wxInputStream& s);
#endif
    ~wxDataInputStream();

    bool IsOk() { return m_input->IsOk(); }

#if wxHAS_INT64
    wxUint64 Read64();
#endif
#if wxUSE_LONGLONG
    wxLongLong ReadLL();
#endif
    wxUint32 Read32();
    wxUint16 Read16();
    wxUint8 Read8();
    double ReadDouble();
    wxString ReadString();

#if wxHAS_INT64
    void Read64(wxUint64 *buffer, size_t size);
    void Read64(wxInt64 *buffer, size_t size);
#endif
#if defined(wxLongLong_t) && wxUSE_LONGLONG
    void Read64(wxULongLong *buffer, size_t size);
    void Read64(wxLongLong *buffer, size_t size);
#endif
#if wxUSE_LONGLONG
    void ReadLL(wxULongLong *buffer, size_t size);
    void ReadLL(wxLongLong *buffer, size_t size);
#endif
    void Read32(wxUint32 *buffer, size_t size);
    void Read16(wxUint16 *buffer, size_t size);
    void Read8(wxUint8 *buffer, size_t size);
    void ReadDouble(double *buffer, size_t size);

    wxDataInputStream& operator>>(wxString& s);
    wxDataInputStream& operator>>(wxInt8& c);
    wxDataInputStream& operator>>(wxInt16& i);
    wxDataInputStream& operator>>(wxInt32& i);
    wxDataInputStream& operator>>(wxUint8& c);
    wxDataInputStream& operator>>(wxUint16& i);
    wxDataInputStream& operator>>(wxUint32& i);
#if wxHAS_INT64
    wxDataInputStream& operator>>(wxUint64& i);
    wxDataInputStream& operator>>(wxInt64& i);
#endif
#if defined(wxLongLong_t) && wxUSE_LONGLONG
    wxDataInputStream& operator>>(wxULongLong& i);
    wxDataInputStream& operator>>(wxLongLong& i);
#endif
    wxDataInputStream& operator>>(double& i);
    wxDataInputStream& operator>>(float& f);

    void BigEndianOrdered(bool be_order) { m_be_order = be_order; }

protected:
    wxInputStream *m_input;
    bool m_be_order;
#if wxUSE_UNICODE
    wxMBConv *m_conv;
#endif

    DECLARE_NO_COPY_CLASS(wxDataInputStream)
};

class WXDLLIMPEXP_BASE wxDataOutputStream
{
public:
#if wxUSE_UNICODE
    wxDataOutputStream(wxOutputStream& s, const wxMBConv& conv = wxConvAuto());
#else
    wxDataOutputStream(wxOutputStream& s);
#endif
    ~wxDataOutputStream();

    bool IsOk() { return m_output->IsOk(); }

#if wxHAS_INT64
    void Write64(wxUint64 i);
    void Write64(wxInt64 i);
#endif
#if wxUSE_LONGLONG
    void WriteLL(const wxLongLong &ll);
    void WriteLL(const wxULongLong &ll);
#endif
    void Write32(wxUint32 i);
    void Write16(wxUint16 i);
    void Write8(wxUint8 i);
    void WriteDouble(double d);
    void WriteString(const wxString& string);

#if wxHAS_INT64
    void Write64(const wxUint64 *buffer, size_t size);
    void Write64(const wxInt64 *buffer, size_t size);
#endif
#if defined(wxLongLong_t) && wxUSE_LONGLONG
    void Write64(const wxULongLong *buffer, size_t size);
    void Write64(const wxLongLong *buffer, size_t size);
#endif
#if wxUSE_LONGLONG
    void WriteLL(const wxULongLong *buffer, size_t size);
    void WriteLL(const wxLongLong *buffer, size_t size);
#endif
    void Write32(const wxUint32 *buffer, size_t size);
    void Write16(const wxUint16 *buffer, size_t size);
    void Write8(const wxUint8 *buffer, size_t size);
    void WriteDouble(const double *buffer, size_t size);

    wxDataOutputStream& operator<<(const wxChar *string);
    wxDataOutputStream& operator<<(const wxString& string);
    wxDataOutputStream& operator<<(wxInt8 c);
    wxDataOutputStream& operator<<(wxInt16 i);
    wxDataOutputStream& operator<<(wxInt32 i);
    wxDataOutputStream& operator<<(wxUint8 c);
    wxDataOutputStream& operator<<(wxUint16 i);
    wxDataOutputStream& operator<<(wxUint32 i);
#if wxHAS_INT64
    wxDataOutputStream& operator<<(wxUint64 i);
    wxDataOutputStream& operator<<(wxInt64 i);
#endif
#if defined(wxLongLong_t) && wxUSE_LONGLONG
    wxDataOutputStream& operator<<(const wxULongLong &i);
    wxDataOutputStream& operator<<(const wxLongLong &i);
#endif
    wxDataOutputStream& operator<<(double f);
    wxDataOutputStream& operator<<(float f);

    void BigEndianOrdered(bool be_order) { m_be_order = be_order; }

protected:
    wxOutputStream *m_output;
    bool m_be_order;
#if wxUSE_UNICODE
    wxMBConv *m_conv;
#endif

    DECLARE_NO_COPY_CLASS(wxDataOutputStream)
};

#endif
  // wxUSE_STREAMS

#endif
    // _WX_DATSTREAM_H_
