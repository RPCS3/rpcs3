/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/datstrm.cpp
// Purpose:     Data stream classes
// Author:      Guilhem Lavaux
// Modified by: Mickael Gilabert
// Created:     28/06/98
// RCS-ID:      $Id: datstrm.cpp 53028 2008-04-05 17:28:32Z VZ $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_STREAMS

#include "wx/datstrm.h"

#ifndef WX_PRECOMP
    #include "wx/math.h"
#endif //WX_PRECOMP

// ---------------------------------------------------------------------------
// wxDataInputStream
// ---------------------------------------------------------------------------

#if wxUSE_UNICODE
wxDataInputStream::wxDataInputStream(wxInputStream& s, const wxMBConv& conv)
  : m_input(&s), m_be_order(false), m_conv(conv.Clone())
#else
wxDataInputStream::wxDataInputStream(wxInputStream& s)
  : m_input(&s), m_be_order(false)
#endif
{
}

wxDataInputStream::~wxDataInputStream()
{
#if wxUSE_UNICODE
    delete m_conv;
#endif // wxUSE_UNICODE
}

#if wxHAS_INT64
wxUint64 wxDataInputStream::Read64()
{
  wxUint64 tmp;
  Read64(&tmp, 1);
  return tmp;
}
#endif // wxHAS_INT64

wxUint32 wxDataInputStream::Read32()
{
  wxUint32 i32;

  m_input->Read(&i32, 4);

  if (m_be_order)
    return wxUINT32_SWAP_ON_LE(i32);
  else
    return wxUINT32_SWAP_ON_BE(i32);
}

wxUint16 wxDataInputStream::Read16()
{
  wxUint16 i16;

  m_input->Read(&i16, 2);

  if (m_be_order)
    return wxUINT16_SWAP_ON_LE(i16);
  else
    return wxUINT16_SWAP_ON_BE(i16);
}

wxUint8 wxDataInputStream::Read8()
{
  wxUint8 buf;

  m_input->Read(&buf, 1);
  return (wxUint8)buf;
}

double wxDataInputStream::ReadDouble()
{
#if wxUSE_APPLE_IEEE
  char buf[10];

  m_input->Read(buf, 10);
  return ConvertFromIeeeExtended((const wxInt8 *)buf);
#else
  return 0.0;
#endif
}

wxString wxDataInputStream::ReadString()
{
    wxString ret;

    const size_t len = Read32();
    if ( len > 0 )
    {
#if wxUSE_UNICODE
        wxCharBuffer tmp(len + 1);
        if ( tmp )
        {
            m_input->Read(tmp.data(), len);
            tmp.data()[len] = '\0';
            ret = m_conv->cMB2WX(tmp.data());
        }
#else
        wxStringBuffer buf(ret, len);
        if ( buf )
            m_input->Read(buf, len);
#endif
    }

    return ret;
}

#if wxUSE_LONGLONG

template <class T>
static
void DoReadLL(T *buffer, size_t size, wxInputStream *input, bool be_order)
{
    typedef T DataType;
    unsigned char *pchBuffer = new unsigned char[size * 8];
    // TODO: Check for overflow when size is of type uint and is > than 512m
    input->Read(pchBuffer, size * 8);
    size_t idx_base = 0;
    if ( be_order )
    {
        for ( size_t uiIndex = 0; uiIndex != size; ++uiIndex )
        {
            buffer[uiIndex] = 0l;
            for ( unsigned ui = 0; ui != 8; ++ui )
            {
                buffer[uiIndex] = buffer[uiIndex] * 256l +
                            DataType((unsigned long) pchBuffer[idx_base + ui]);
            }

            idx_base += 8;
        }
    }
    else // little endian
    {
        for ( size_t uiIndex=0; uiIndex!=size; ++uiIndex )
        {
            buffer[uiIndex] = 0l;
            for ( unsigned ui=0; ui!=8; ++ui )
                buffer[uiIndex] = buffer[uiIndex] * 256l +
                    DataType((unsigned long) pchBuffer[idx_base + 7 - ui]);
            idx_base += 8;
        }
    }
    delete[] pchBuffer;
}

template <class T>
static void DoWriteLL(const T *buffer, size_t size, wxOutputStream *output, bool be_order)
{
    typedef T DataType;
    unsigned char *pchBuffer = new unsigned char[size * 8];
    size_t idx_base = 0;
    if ( be_order )
    {
        for ( size_t uiIndex = 0; uiIndex != size; ++uiIndex )
        {
            DataType i64 = buffer[uiIndex];
            for ( unsigned ui = 0; ui != 8; ++ui )
            {
                pchBuffer[idx_base + 7 - ui] =
                    (unsigned char) (i64.GetLo() & 255l);
                i64 >>= 8l;
            }

            idx_base += 8;
        }
    }
    else // little endian
    {
        for ( size_t uiIndex=0; uiIndex != size; ++uiIndex )
        {
            DataType i64 = buffer[uiIndex];
            for (unsigned ui=0; ui!=8; ++ui)
            {
                pchBuffer[idx_base + ui] =
                    (unsigned char) (i64.GetLo() & 255l);
                i64 >>= 8l;
            }

            idx_base += 8;
        }
    }

    // TODO: Check for overflow when size is of type uint and is > than 512m
    output->Write(pchBuffer, size * 8);
    delete[] pchBuffer;
}

#endif // wxUSE_LONGLONG

#ifdef wxLongLong_t

template <class T>
static
void DoReadI64(T *buffer, size_t size, wxInputStream *input, bool be_order)
{
    typedef T DataType;
    unsigned char *pchBuffer = (unsigned char*) buffer;
    // TODO: Check for overflow when size is of type uint and is > than 512m
    input->Read(pchBuffer, size * 8);
    if ( be_order )
    {
        for ( wxUint32 i = 0; i < size; i++ )
        {
            DataType v = wxUINT64_SWAP_ON_LE(*buffer);
            *(buffer++) = v;
        }
    }
    else // little endian
    {
        for ( wxUint32 i=0; i<size; i++ )
        {
            DataType v = wxUINT64_SWAP_ON_BE(*buffer);
            *(buffer++) = v;
        }
    }
}

template <class T>
static
void DoWriteI64(const T *buffer, size_t size, wxOutputStream *output, bool be_order)
{
  typedef T DataType;
  if ( be_order )
  {
    for ( size_t i = 0; i < size; i++ )
    {
      DataType i64 = wxUINT64_SWAP_ON_LE(*buffer);
      buffer++;
      output->Write(&i64, 8);
    }
  }
  else // little endian
  {
    for ( size_t i=0; i < size; i++ )
    {
      DataType i64 = wxUINT64_SWAP_ON_BE(*buffer);
      buffer++;
      output->Write(&i64, 8);
    }
  }
}

#endif // wxLongLong_t


#if wxHAS_INT64
void wxDataInputStream::Read64(wxUint64 *buffer, size_t size)
{
#ifndef wxLongLong_t
    DoReadLL(buffer, size, m_input, m_be_order);
#else
    DoReadI64(buffer, size, m_input, m_be_order);
#endif
}

void wxDataInputStream::Read64(wxInt64 *buffer, size_t size)
{
#ifndef wxLongLong_t
    DoReadLL(buffer, size, m_input, m_be_order);
#else
    DoReadI64(buffer, size, m_input, m_be_order);
#endif
}
#endif // wxHAS_INT64

#if defined(wxLongLong_t) && wxUSE_LONGLONG
void wxDataInputStream::Read64(wxULongLong *buffer, size_t size)
{
    DoReadLL(buffer, size, m_input, m_be_order);
}

void wxDataInputStream::Read64(wxLongLong *buffer, size_t size)
{
    DoReadLL(buffer, size, m_input, m_be_order);
}
#endif // wxLongLong_t

#if wxUSE_LONGLONG
void wxDataInputStream::ReadLL(wxULongLong *buffer, size_t size)
{
    DoReadLL(buffer, size, m_input, m_be_order);
}

void wxDataInputStream::ReadLL(wxLongLong *buffer, size_t size)
{
    DoReadLL(buffer, size, m_input, m_be_order);
}

wxLongLong wxDataInputStream::ReadLL(void)
{
    wxLongLong ll;
    DoReadLL(&ll, (size_t)1, m_input, m_be_order);
    return ll;
}
#endif // wxUSE_LONGLONG

void wxDataInputStream::Read32(wxUint32 *buffer, size_t size)
{
    m_input->Read(buffer, size * 4);

    if (m_be_order)
    {
        for (wxUint32 i=0; i<size; i++)
        {
            wxUint32 v = wxUINT32_SWAP_ON_LE(*buffer);
            *(buffer++) = v;
        }
    }
    else
    {
        for (wxUint32 i=0; i<size; i++)
        {
            wxUint32 v = wxUINT32_SWAP_ON_BE(*buffer);
            *(buffer++) = v;
        }
    }
}

void wxDataInputStream::Read16(wxUint16 *buffer, size_t size)
{
  m_input->Read(buffer, size * 2);

  if (m_be_order)
  {
    for (wxUint32 i=0; i<size; i++)
    {
      wxUint16 v = wxUINT16_SWAP_ON_LE(*buffer);
      *(buffer++) = v;
    }
  }
  else
  {
    for (wxUint32 i=0; i<size; i++)
    {
      wxUint16 v = wxUINT16_SWAP_ON_BE(*buffer);
      *(buffer++) = v;
    }
  }
}

void wxDataInputStream::Read8(wxUint8 *buffer, size_t size)
{
  m_input->Read(buffer, size);
}

void wxDataInputStream::ReadDouble(double *buffer, size_t size)
{
  for (wxUint32 i=0; i<size; i++)
  {
    *(buffer++) = ReadDouble();
  }
}

wxDataInputStream& wxDataInputStream::operator>>(wxString& s)
{
  s = ReadString();
  return *this;
}

wxDataInputStream& wxDataInputStream::operator>>(wxInt8& c)
{
  c = (wxInt8)Read8();
  return *this;
}

wxDataInputStream& wxDataInputStream::operator>>(wxInt16& i)
{
  i = (wxInt16)Read16();
  return *this;
}

wxDataInputStream& wxDataInputStream::operator>>(wxInt32& i)
{
  i = (wxInt32)Read32();
  return *this;
}

wxDataInputStream& wxDataInputStream::operator>>(wxUint8& c)
{
  c = Read8();
  return *this;
}

wxDataInputStream& wxDataInputStream::operator>>(wxUint16& i)
{
  i = Read16();
  return *this;
}

wxDataInputStream& wxDataInputStream::operator>>(wxUint32& i)
{
  i = Read32();
  return *this;
}

#if wxHAS_INT64
wxDataInputStream& wxDataInputStream::operator>>(wxUint64& i)
{
  i = Read64();
  return *this;
}

wxDataInputStream& wxDataInputStream::operator>>(wxInt64& i)
{
  i = Read64();
  return *this;
}
#endif // wxHAS_INT64

#if defined(wxLongLong_t) && wxUSE_LONGLONG
wxDataInputStream& wxDataInputStream::operator>>(wxULongLong& i)
{
  i = ReadLL();
  return *this;
}

wxDataInputStream& wxDataInputStream::operator>>(wxLongLong& i)
{
  i = ReadLL();
  return *this;
}
#endif // wxLongLong_t

wxDataInputStream& wxDataInputStream::operator>>(double& i)
{
  i = ReadDouble();
  return *this;
}

wxDataInputStream& wxDataInputStream::operator>>(float& f)
{
  f = (float)ReadDouble();
  return *this;
}

// ---------------------------------------------------------------------------
// wxDataOutputStream
// ---------------------------------------------------------------------------

#if wxUSE_UNICODE
wxDataOutputStream::wxDataOutputStream(wxOutputStream& s, const wxMBConv& conv)
  : m_output(&s), m_be_order(false), m_conv(conv.Clone())
#else
wxDataOutputStream::wxDataOutputStream(wxOutputStream& s)
  : m_output(&s), m_be_order(false)
#endif
{
}

wxDataOutputStream::~wxDataOutputStream()
{
#if wxUSE_UNICODE
    delete m_conv;
#endif // wxUSE_UNICODE
}

#if wxHAS_INT64
void wxDataOutputStream::Write64(wxUint64 i)
{
  Write64(&i, 1);
}

void wxDataOutputStream::Write64(wxInt64 i)
{
  Write64(&i, 1);
}
#endif // wxHAS_INT64

void wxDataOutputStream::Write32(wxUint32 i)
{
  wxUint32 i32;

  if (m_be_order)
    i32 = wxUINT32_SWAP_ON_LE(i);
  else
    i32 = wxUINT32_SWAP_ON_BE(i);
  m_output->Write(&i32, 4);
}

void wxDataOutputStream::Write16(wxUint16 i)
{
  wxUint16 i16;

  if (m_be_order)
    i16 = wxUINT16_SWAP_ON_LE(i);
  else
    i16 = wxUINT16_SWAP_ON_BE(i);

  m_output->Write(&i16, 2);
}

void wxDataOutputStream::Write8(wxUint8 i)
{
  m_output->Write(&i, 1);
}

void wxDataOutputStream::WriteString(const wxString& string)
{
#if wxUSE_UNICODE
  const wxWX2MBbuf buf = string.mb_str(*m_conv);
#else
  const wxWX2MBbuf buf = string.mb_str();
#endif
  size_t len = strlen(buf);
  Write32(len);
  if (len > 0)
      m_output->Write(buf, len);
}

void wxDataOutputStream::WriteDouble(double d)
{
  char buf[10];

#if wxUSE_APPLE_IEEE
  ConvertToIeeeExtended(d, (wxInt8 *)buf);
#else
#if !defined(__VMS__) && !defined(__GNUG__)
# pragma warning "wxDataOutputStream::WriteDouble() not using IeeeExtended - will not work!"
#endif
   buf[0] = '\0';
#endif
  m_output->Write(buf, 10);
}

#if wxHAS_INT64
void wxDataOutputStream::Write64(const wxUint64 *buffer, size_t size)
{
#ifndef wxLongLong_t
    DoWriteLL(buffer, size, m_output, m_be_order);
#else
    DoWriteI64(buffer, size, m_output, m_be_order);
#endif
}

void wxDataOutputStream::Write64(const wxInt64 *buffer, size_t size)
{
#ifndef wxLongLong_t
    DoWriteLL(buffer, size, m_output, m_be_order);
#else
    DoWriteI64(buffer, size, m_output, m_be_order);
#endif
}
#endif // wxHAS_INT64

#if defined(wxLongLong_t) && wxUSE_LONGLONG
void wxDataOutputStream::Write64(const wxULongLong *buffer, size_t size)
{
    DoWriteLL(buffer, size, m_output, m_be_order);
}

void wxDataOutputStream::Write64(const wxLongLong *buffer, size_t size)
{
    DoWriteLL(buffer, size, m_output, m_be_order);
}
#endif // wxLongLong_t

#if wxUSE_LONGLONG
void wxDataOutputStream::WriteLL(const wxULongLong *buffer, size_t size)
{
    DoWriteLL(buffer, size, m_output, m_be_order);
}

void wxDataOutputStream::WriteLL(const wxLongLong *buffer, size_t size)
{
    DoWriteLL(buffer, size, m_output, m_be_order);
}

void wxDataOutputStream::WriteLL(const wxLongLong &ll)
{
    WriteLL(&ll, 1);
}

void wxDataOutputStream::WriteLL(const wxULongLong &ll)
{
    WriteLL(&ll, 1);
}
#endif // wxUSE_LONGLONG

void wxDataOutputStream::Write32(const wxUint32 *buffer, size_t size)
{
  if (m_be_order)
  {
    for (wxUint32 i=0; i<size ;i++)
    {
      wxUint32 i32 = wxUINT32_SWAP_ON_LE(*buffer);
      buffer++;
      m_output->Write(&i32, 4);
    }
  }
  else
  {
    for (wxUint32 i=0; i<size ;i++)
    {
      wxUint32 i32 = wxUINT32_SWAP_ON_BE(*buffer);
      buffer++;
      m_output->Write(&i32, 4);
    }
  }
}

void wxDataOutputStream::Write16(const wxUint16 *buffer, size_t size)
{
  if (m_be_order)
  {
    for (wxUint32 i=0; i<size ;i++)
    {
      wxUint16 i16 = wxUINT16_SWAP_ON_LE(*buffer);
      buffer++;
      m_output->Write(&i16, 2);
    }
  }
  else
  {
    for (wxUint32 i=0; i<size ;i++)
    {
      wxUint16 i16 = wxUINT16_SWAP_ON_BE(*buffer);
      buffer++;
      m_output->Write(&i16, 2);
    }
  }
}

void wxDataOutputStream::Write8(const wxUint8 *buffer, size_t size)
{
  m_output->Write(buffer, size);
}

void wxDataOutputStream::WriteDouble(const double *buffer, size_t size)
{
  for (wxUint32 i=0; i<size; i++)
  {
    WriteDouble(*(buffer++));
  }
}

wxDataOutputStream& wxDataOutputStream::operator<<(const wxChar *string)
{
  Write32(wxStrlen(string));
  m_output->Write((const char *)string, wxStrlen(string)*sizeof(wxChar));
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(const wxString& string)
{
  WriteString(string);
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(wxInt8 c)
{
  Write8((wxUint8)c);
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(wxInt16 i)
{
  Write16((wxUint16)i);
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(wxInt32 i)
{
  Write32((wxUint32)i);
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(wxUint8 c)
{
  Write8(c);
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(wxUint16 i)
{
  Write16(i);
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(wxUint32 i)
{
  Write32(i);
  return *this;
}

#if wxHAS_INT64
wxDataOutputStream& wxDataOutputStream::operator<<(wxUint64 i)
{
  Write64(i);
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(wxInt64 i)
{
  Write64(i);
  return *this;
}
#endif // wxHAS_INT64

#if defined(wxLongLong_t) && wxUSE_LONGLONG
wxDataOutputStream& wxDataOutputStream::operator<<(const wxULongLong &i)
{
  WriteLL(i);
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(const wxLongLong &i)
{
  WriteLL(i);
  return *this;
}
#endif // wxLongLong_t

wxDataOutputStream& wxDataOutputStream::operator<<(double f)
{
  WriteDouble(f);
  return *this;
}

wxDataOutputStream& wxDataOutputStream::operator<<(float f)
{
  WriteDouble((double)f);
  return *this;
}

#endif
  // wxUSE_STREAMS
