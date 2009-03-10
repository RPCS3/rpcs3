///////////////////////////////////////////////////////////////////////////////
// Name:        common/wxexec.cpp
// Purpose:     defines wxStreamTempInputBuffer which is used by Unix and MSW
//              implementations of wxExecute; this file is only used by the
//              library and never by the user code
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.08.02
// RCS-ID:      $Id: execcmn.cpp 35289 2005-08-23 23:12:48Z VZ $
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WXEXEC_CPP_
#define _WX_WXEXEC_CPP_

// this file should never be compiled directly, just included by other code
#ifndef _WX_USED_BY_WXEXECUTE_
    #error "You should never directly build this file!"
#endif

// ----------------------------------------------------------------------------
// wxStreamTempInputBuffer
// ----------------------------------------------------------------------------

/*
   wxStreamTempInputBuffer is a hack which we need to solve the problem of
   executing a child process synchronously with IO redirecting: when we do
   this, the child writes to a pipe we open to it but when the pipe buffer
   (which has finite capacity, e.g. commonly just 4Kb) becomes full we have to
   read data from it because the child blocks in its write() until then and if
   it blocks we are never going to return from wxExecute() so we dead lock.

   So here is the fix: we now read the output as soon as it appears into a temp
   buffer (wxStreamTempInputBuffer object) and later just stuff it back into
   the stream when the process terminates. See supporting code in wxExecute()
   itself as well.

   Note that this is horribly inefficient for large amounts of output (count
   the number of times we copy the data around) and so a better API is badly
   needed! However it's not easy to devise a way to do this keeping backwards
   compatibility with the existing wxExecute(wxEXEC_SYNC)...
*/

class wxStreamTempInputBuffer
{
public:
    wxStreamTempInputBuffer();

    // call to associate a stream with this buffer, otherwise nothing happens
    // at all
    void Init(wxPipeInputStream *stream);

    // check for input on our stream and cache it in our buffer if any
    void Update();

    ~wxStreamTempInputBuffer();

private:
    // the stream we're buffering, if NULL we don't do anything at all
    wxPipeInputStream *m_stream;

    // the buffer of size m_size (NULL if m_size == 0)
    void *m_buffer;

    // the size of the buffer
    size_t m_size;

    DECLARE_NO_COPY_CLASS(wxStreamTempInputBuffer)
};

inline wxStreamTempInputBuffer::wxStreamTempInputBuffer()
{
    m_stream = NULL;
    m_buffer = NULL;
    m_size = 0;
}

inline void wxStreamTempInputBuffer::Init(wxPipeInputStream *stream)
{
    m_stream = stream;
}

inline
void wxStreamTempInputBuffer::Update()
{
    if ( m_stream && m_stream->CanRead() )
    {
        // realloc in blocks of 4Kb: this is the default (and minimal) buffer
        // size of the Unix pipes so it should be the optimal step
        //
        // NB: don't use "static int" in this inline function, some compilers
        //     (e.g. IBM xlC) don't like it
        enum { incSize = 4096 };

        void *buf = realloc(m_buffer, m_size + incSize);
        if ( !buf )
        {
            // don't read any more, we don't have enough memory to do it
            m_stream = NULL;
        }
        else // got memory for the buffer
        {
            m_buffer = buf;
            m_stream->Read((char *)m_buffer + m_size, incSize);
            m_size += m_stream->LastRead();
        }
    }
}

inline
wxStreamTempInputBuffer::~wxStreamTempInputBuffer()
{
    if ( m_buffer )
    {
        m_stream->Ungetch(m_buffer, m_size);
        free(m_buffer);
    }
}

#endif // _WX_WXEXEC_CPP_

