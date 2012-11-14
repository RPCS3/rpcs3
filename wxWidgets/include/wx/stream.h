/////////////////////////////////////////////////////////////////////////////
// Name:        wx/stream.h
// Purpose:     stream classes
// Author:      Guilhem Lavaux, Guillermo Rodriguez Garcia, Vadim Zeitlin
// Modified by:
// Created:     11/07/98
// RCS-ID:      $Id: stream.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WXSTREAM_H__
#define _WX_WXSTREAM_H__

#include "wx/defs.h"

#if wxUSE_STREAMS

#include <stdio.h>
#include "wx/object.h"
#include "wx/string.h"
#include "wx/filefn.h"  // for wxFileOffset, wxInvalidOffset and wxSeekMode

class WXDLLIMPEXP_FWD_BASE wxStreamBase;
class WXDLLIMPEXP_FWD_BASE wxInputStream;
class WXDLLIMPEXP_FWD_BASE wxOutputStream;

typedef wxInputStream& (*__wxInputManip)(wxInputStream&);
typedef wxOutputStream& (*__wxOutputManip)(wxOutputStream&);

WXDLLIMPEXP_BASE wxOutputStream& wxEndL(wxOutputStream& o_stream);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

enum wxStreamError
{
    wxSTREAM_NO_ERROR = 0,      // stream is in good state
    wxSTREAM_EOF,               // EOF reached in Read() or similar
    wxSTREAM_WRITE_ERROR,       // generic write error
    wxSTREAM_READ_ERROR         // generic read error
};

const int wxEOF = -1;

// ============================================================================
// base stream classes: wxInputStream and wxOutputStream
// ============================================================================

// ---------------------------------------------------------------------------
// wxStreamBase: common (but non virtual!) base for all stream classes
// ---------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxStreamBase
{
public:
    wxStreamBase();
    virtual ~wxStreamBase();

    // error testing
    wxStreamError GetLastError() const { return m_lasterror; }
    virtual bool IsOk() const { return GetLastError() == wxSTREAM_NO_ERROR; }
    bool operator!() const { return !IsOk(); }

    // reset the stream state
    void Reset() { m_lasterror = wxSTREAM_NO_ERROR; }

    // this doesn't make sense for all streams, always test its return value
    virtual size_t GetSize() const;
    virtual wxFileOffset GetLength() const { return wxInvalidOffset; }

    // returns true if the streams supports seeking to arbitrary offsets
    virtual bool IsSeekable() const { return false; }

protected:
    virtual wxFileOffset OnSysSeek(wxFileOffset seek, wxSeekMode mode);
    virtual wxFileOffset OnSysTell() const;

    size_t m_lastcount;
    wxStreamError m_lasterror;

    friend class wxStreamBuffer;

    DECLARE_NO_COPY_CLASS(wxStreamBase)
};

// ----------------------------------------------------------------------------
// wxInputStream: base class for the input streams
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxInputStream : public wxStreamBase
{
public:
    // ctor and dtor, nothing exciting
    wxInputStream();
    virtual ~wxInputStream();


    // IO functions
    // ------------

    // return a character from the stream without removing it, i.e. it will
    // still be returned by the next call to GetC()
    //
    // blocks until something appears in the stream if necessary, if nothing
    // ever does (i.e. EOF) LastRead() will return 0 (and the return value is
    // undefined), otherwise 1
    virtual char Peek();

    // return one byte from the stream, blocking until it appears if
    // necessary
    //
    // on success returns a value between 0 - 255, or wxEOF on EOF or error.
    int GetC();

    // read at most the given number of bytes from the stream
    //
    // there are 2 possible situations here: either there is nothing at all in
    // the stream right now in which case Read() blocks until something appears
    // (use CanRead() to avoid this) or there is already some data available in
    // the stream and then Read() doesn't block but returns just the data it
    // can read without waiting for more
    //
    // in any case, if there are not enough bytes in the stream right now,
    // LastRead() value will be less than size but greater than 0. If it is 0,
    // it means that EOF has been reached.
    virtual wxInputStream& Read(void *buffer, size_t size);

    // copy the entire contents of this stream into streamOut, stopping only
    // when EOF is reached or an error occurs
    wxInputStream& Read(wxOutputStream& streamOut);


    // status functions
    // ----------------

    // returns the number of bytes read by the last call to Read(), GetC() or
    // Peek()
    //
    // this should be used to discover whether that call succeeded in reading
    // all the requested data or not
    virtual size_t LastRead() const { return wxStreamBase::m_lastcount; }

    // returns true if some data is available in the stream right now, so that
    // calling Read() wouldn't block
    virtual bool CanRead() const;

    // is the stream at EOF?
    //
    // note that this cannot be really implemented for all streams and
    // CanRead() is more reliable than Eof()
    virtual bool Eof() const;


    // write back buffer
    // -----------------

    // put back the specified number of bytes into the stream, they will be
    // fetched by the next call to the read functions
    //
    // returns the number of bytes really stuffed back
    size_t Ungetch(const void *buffer, size_t size);

    // put back the specified character in the stream
    //
    // returns true if ok, false on error
    bool Ungetch(char c);


    // position functions
    // ------------------

    // move the stream pointer to the given position (if the stream supports
    // it)
    //
    // returns wxInvalidOffset on error
    virtual wxFileOffset SeekI(wxFileOffset pos, wxSeekMode mode = wxFromStart);

    // return the current position of the stream pointer or wxInvalidOffset
    virtual wxFileOffset TellI() const;


    // stream-like operators
    // ---------------------

    wxInputStream& operator>>(wxOutputStream& out) { return Read(out); }
    wxInputStream& operator>>(__wxInputManip func) { return func(*this); }

protected:
    // do read up to size bytes of data into the provided buffer
    //
    // this method should return 0 if EOF has been reached or an error occurred
    // (m_lasterror should be set accordingly as well) or the number of bytes
    // read
    virtual size_t OnSysRead(void *buffer, size_t size) = 0;

    // write-back buffer support
    // -------------------------

    // return the pointer to a buffer big enough to hold sizeNeeded bytes
    char *AllocSpaceWBack(size_t sizeNeeded);

    // read up to size data from the write back buffer, return the number of
    // bytes read
    size_t GetWBack(void *buf, size_t size);

    // write back buffer or NULL if none
    char *m_wback;

    // the size of the buffer
    size_t m_wbacksize;

    // the current position in the buffer
    size_t m_wbackcur;

    friend class wxStreamBuffer;

    DECLARE_NO_COPY_CLASS(wxInputStream)
};

// ----------------------------------------------------------------------------
// wxOutputStream: base for the output streams
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxOutputStream : public wxStreamBase
{
public:
    wxOutputStream();
    virtual ~wxOutputStream();

    void PutC(char c);
    virtual wxOutputStream& Write(const void *buffer, size_t size);
    wxOutputStream& Write(wxInputStream& stream_in);

    virtual wxFileOffset SeekO(wxFileOffset pos, wxSeekMode mode = wxFromStart);
    virtual wxFileOffset TellO() const;

    virtual size_t LastWrite() const { return wxStreamBase::m_lastcount; }

    virtual void Sync();
    virtual bool Close() { return true; }

    wxOutputStream& operator<<(wxInputStream& out) { return Write(out); }
    wxOutputStream& operator<<( __wxOutputManip func) { return func(*this); }

protected:
    // to be implemented in the derived classes (it should have been pure
    // virtual)
    virtual size_t OnSysWrite(const void *buffer, size_t bufsize);

    friend class wxStreamBuffer;

    DECLARE_NO_COPY_CLASS(wxOutputStream)
};

// ============================================================================
// helper stream classes
// ============================================================================

// ---------------------------------------------------------------------------
// A stream for measuring streamed output
// ---------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxCountingOutputStream : public wxOutputStream
{
public:
    wxCountingOutputStream();

    wxFileOffset GetLength() const;
    bool Ok() const { return IsOk(); }
    bool IsOk() const { return true; }

protected:
    virtual size_t OnSysWrite(const void *buffer, size_t size);
    virtual wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode);
    virtual wxFileOffset OnSysTell() const;

    size_t m_currentPos;

    DECLARE_NO_COPY_CLASS(wxCountingOutputStream)
};

// ---------------------------------------------------------------------------
// "Filter" streams
// ---------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFilterInputStream : public wxInputStream
{
public:
    wxFilterInputStream();
    wxFilterInputStream(wxInputStream& stream);
    wxFilterInputStream(wxInputStream *stream);
    virtual ~wxFilterInputStream();

    char Peek() { return m_parent_i_stream->Peek(); }

    wxFileOffset GetLength() const { return m_parent_i_stream->GetLength(); }

    wxInputStream *GetFilterInputStream() const { return m_parent_i_stream; }

protected:
    wxInputStream *m_parent_i_stream;
    bool m_owns;

    DECLARE_NO_COPY_CLASS(wxFilterInputStream)
};

class WXDLLIMPEXP_BASE wxFilterOutputStream : public wxOutputStream
{
public:
    wxFilterOutputStream();
    wxFilterOutputStream(wxOutputStream& stream);
    wxFilterOutputStream(wxOutputStream *stream);
    virtual ~wxFilterOutputStream();

    wxFileOffset GetLength() const { return m_parent_o_stream->GetLength(); }

    wxOutputStream *GetFilterOutputStream() const { return m_parent_o_stream; }

    bool Close();

protected:
    wxOutputStream *m_parent_o_stream;
    bool m_owns;

    DECLARE_NO_COPY_CLASS(wxFilterOutputStream)
};

enum wxStreamProtocolType
{
    wxSTREAM_PROTOCOL,  // wxFileSystem protocol (should be only one)
    wxSTREAM_MIMETYPE,  // MIME types the stream handles
    wxSTREAM_ENCODING,  // The HTTP Content-Encodings the stream handles
    wxSTREAM_FILEEXT    // File extensions the stream handles
};

void WXDLLIMPEXP_BASE wxUseFilterClasses();

class WXDLLIMPEXP_BASE wxFilterClassFactoryBase : public wxObject
{
public:
    virtual ~wxFilterClassFactoryBase() { }

    wxString GetProtocol() const { return wxString(*GetProtocols()); }
    wxString PopExtension(const wxString& location) const;

    virtual const wxChar * const *GetProtocols(wxStreamProtocolType type
                                               = wxSTREAM_PROTOCOL) const = 0;

    bool CanHandle(const wxChar *protocol,
                   wxStreamProtocolType type
                   = wxSTREAM_PROTOCOL) const;

protected:
    wxString::size_type FindExtension(const wxChar *location) const;

    DECLARE_ABSTRACT_CLASS(wxFilterClassFactoryBase)
};

class WXDLLIMPEXP_BASE wxFilterClassFactory : public wxFilterClassFactoryBase
{
public:
    virtual ~wxFilterClassFactory() { }

    virtual wxFilterInputStream  *NewStream(wxInputStream& stream)  const = 0;
    virtual wxFilterOutputStream *NewStream(wxOutputStream& stream) const = 0;
    virtual wxFilterInputStream  *NewStream(wxInputStream *stream)  const = 0;
    virtual wxFilterOutputStream *NewStream(wxOutputStream *stream) const = 0;

    static const wxFilterClassFactory *Find(const wxChar *protocol,
                                            wxStreamProtocolType type
                                            = wxSTREAM_PROTOCOL);

    static const wxFilterClassFactory *GetFirst();
    const wxFilterClassFactory *GetNext() const { return m_next; }

    void PushFront() { Remove(); m_next = sm_first; sm_first = this; }
    void Remove();

protected:
    wxFilterClassFactory() : m_next(this) { }

    wxFilterClassFactory& operator=(const wxFilterClassFactory&)
        { return *this; }

private:
    static wxFilterClassFactory *sm_first;
    wxFilterClassFactory *m_next;

    DECLARE_ABSTRACT_CLASS(wxFilterClassFactory)
};

// ============================================================================
// buffered streams
// ============================================================================

// ---------------------------------------------------------------------------
// Stream buffer: this class can be derived from and passed to
// wxBufferedStreams to implement custom buffering
// ---------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxStreamBuffer
{
public:
    enum BufMode
    {
        read,
        write,
        read_write
    };

    wxStreamBuffer(wxStreamBase& stream, BufMode mode);
    wxStreamBuffer(const wxStreamBuffer& buf);
    virtual ~wxStreamBuffer();

    // Filtered IO
    virtual size_t Read(void *buffer, size_t size);
    size_t Read(wxStreamBuffer *buf);
    virtual size_t Write(const void *buffer, size_t size);
    size_t Write(wxStreamBuffer *buf);

    virtual char Peek();
    virtual char GetChar();
    virtual void PutChar(char c);
    virtual wxFileOffset Tell() const;
    virtual wxFileOffset Seek(wxFileOffset pos, wxSeekMode mode);

    // Buffer control
    void ResetBuffer();

    // NB: the buffer must always be allocated with malloc() if takeOwn is
    //     true as it will be deallocated by free()
    void SetBufferIO(void *start, void *end, bool takeOwnership = false);
    void SetBufferIO(void *start, size_t len, bool takeOwnership = false);
    void SetBufferIO(size_t bufsize);
    void *GetBufferStart() const { return m_buffer_start; }
    void *GetBufferEnd() const { return m_buffer_end; }
    void *GetBufferPos() const { return m_buffer_pos; }
    size_t GetBufferSize() const { return m_buffer_size; }
    size_t GetIntPosition() const { return m_buffer_pos - m_buffer_start; }
    void SetIntPosition(size_t pos) { m_buffer_pos = m_buffer_start + pos; }
    size_t GetLastAccess() const { return m_buffer_end - m_buffer_start; }
    size_t GetBytesLeft() const { return m_buffer_end - m_buffer_pos; }

    void Fixed(bool fixed) { m_fixed = fixed; }
    void Flushable(bool f) { m_flushable = f; }

    bool FlushBuffer();
    bool FillBuffer();
    size_t GetDataLeft();

    // misc accessors
    wxStreamBase *GetStream() const { return m_stream; }
    bool HasBuffer() const { return m_buffer_size != 0; }

    bool IsFixed() const { return m_fixed; }
    bool IsFlushable() const { return m_flushable; }

    // only for input/output buffers respectively, returns NULL otherwise
    wxInputStream *GetInputStream() const;
    wxOutputStream *GetOutputStream() const;

#if WXWIN_COMPATIBILITY_2_6
    // deprecated, for compatibility only
    wxDEPRECATED( wxStreamBase *Stream() );
#endif // WXWIN_COMPATIBILITY_2_6

    // this constructs a dummy wxStreamBuffer, used by (and exists for)
    // wxMemoryStreams only, don't use!
    wxStreamBuffer(BufMode mode);

protected:
    void GetFromBuffer(void *buffer, size_t size);
    void PutToBuffer(const void *buffer, size_t size);

    // set the last error to the specified value if we didn't have it before
    void SetError(wxStreamError err);

    // common part of several ctors
    void Init();

    // init buffer variables to be empty
    void InitBuffer();

    // free the buffer (always safe to call)
    void FreeBuffer();

    // the buffer itself: the pointers to its start and end and the current
    // position in the buffer
    char *m_buffer_start,
         *m_buffer_end,
         *m_buffer_pos;

    // the buffer size
    // FIXME: isn't it the same as m_buffer_end - m_buffer_start? (VZ)
    size_t m_buffer_size;

    // the stream we're associated with
    wxStreamBase *m_stream;

    // its mode
    BufMode m_mode;

    // flags
    bool m_destroybuf,      // deallocate buffer?
         m_fixed,
         m_flushable;

private:
// Cannot use
//  DECLARE_NO_COPY_CLASS(wxStreamBuffer)
// because copy constructor is explicitly declared above;
// but no copy assignment operator is defined, so declare
// it private to prevent the compiler from defining it:
    wxStreamBuffer& operator=(const wxStreamBuffer&);
};

// ---------------------------------------------------------------------------
// wxBufferedInputStream
// ---------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxBufferedInputStream : public wxFilterInputStream
{
public:
    // if a non NULL buffer is given to the stream, it will be deleted by it
    wxBufferedInputStream(wxInputStream& stream,
                          wxStreamBuffer *buffer = NULL);
    virtual ~wxBufferedInputStream();

    char Peek();
    wxInputStream& Read(void *buffer, size_t size);

    // Position functions
    wxFileOffset SeekI(wxFileOffset pos, wxSeekMode mode = wxFromStart);
    wxFileOffset TellI() const;
    bool IsSeekable() const { return m_parent_i_stream->IsSeekable(); }

    // the buffer given to the stream will be deleted by it
    void SetInputStreamBuffer(wxStreamBuffer *buffer);
    wxStreamBuffer *GetInputStreamBuffer() const { return m_i_streambuf; }

#if WXWIN_COMPATIBILITY_2_6
    // deprecated, for compatibility only
    wxDEPRECATED( wxStreamBuffer *InputStreamBuffer() const );
#endif // WXWIN_COMPATIBILITY_2_6

protected:
    virtual size_t OnSysRead(void *buffer, size_t bufsize);
    virtual wxFileOffset OnSysSeek(wxFileOffset seek, wxSeekMode mode);
    virtual wxFileOffset OnSysTell() const;

    wxStreamBuffer *m_i_streambuf;

    DECLARE_NO_COPY_CLASS(wxBufferedInputStream)
};

// ----------------------------------------------------------------------------
// wxBufferedOutputStream
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxBufferedOutputStream : public wxFilterOutputStream
{
public:
    // if a non NULL buffer is given to the stream, it will be deleted by it
    wxBufferedOutputStream(wxOutputStream& stream,
                           wxStreamBuffer *buffer = NULL);
    virtual ~wxBufferedOutputStream();

    wxOutputStream& Write(const void *buffer, size_t size);

    // Position functions
    wxFileOffset SeekO(wxFileOffset pos, wxSeekMode mode = wxFromStart);
    wxFileOffset TellO() const;
    bool IsSeekable() const { return m_parent_o_stream->IsSeekable(); }

    void Sync();
    bool Close();

    wxFileOffset GetLength() const;

    // the buffer given to the stream will be deleted by it
    void SetOutputStreamBuffer(wxStreamBuffer *buffer);
    wxStreamBuffer *GetOutputStreamBuffer() const { return m_o_streambuf; }

#if WXWIN_COMPATIBILITY_2_6
    // deprecated, for compatibility only
    wxDEPRECATED( wxStreamBuffer *OutputStreamBuffer() const );
#endif // WXWIN_COMPATIBILITY_2_6

protected:
    virtual size_t OnSysWrite(const void *buffer, size_t bufsize);
    virtual wxFileOffset OnSysSeek(wxFileOffset seek, wxSeekMode mode);
    virtual wxFileOffset OnSysTell() const;

    wxStreamBuffer *m_o_streambuf;

    DECLARE_NO_COPY_CLASS(wxBufferedOutputStream)
};

#if WXWIN_COMPATIBILITY_2_6
    inline wxStreamBase *wxStreamBuffer::Stream() { return m_stream; }
    inline wxStreamBuffer *wxBufferedInputStream::InputStreamBuffer() const { return m_i_streambuf; }
    inline wxStreamBuffer *wxBufferedOutputStream::OutputStreamBuffer() const { return m_o_streambuf; }
#endif // WXWIN_COMPATIBILITY_2_6

#endif // wxUSE_STREAMS

#endif // _WX_WXSTREAM_H__
