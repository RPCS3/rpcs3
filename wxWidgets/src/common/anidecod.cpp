/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/anidecod.cpp
// Purpose:     wxANIDecoder, ANI reader for wxImage and wxAnimation
// Author:      Francesco Montorsi
// RCS-ID:      $Id: anidecod.cpp 43898 2006-12-10 14:18:37Z VZ $
// Copyright:   (c) Francesco Montorsi
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_STREAMS && wxUSE_ICO_CUR

#include "wx/anidecod.h"

#ifndef WX_PRECOMP
    #include "wx/palette.h"
#endif

#include <stdlib.h>
#include <string.h>

// static
wxCURHandler wxANIDecoder::sm_handler;

//---------------------------------------------------------------------------
// wxANIFrameInfo
//---------------------------------------------------------------------------

class wxANIFrameInfo
{
public:
    wxANIFrameInfo(unsigned int delay = 0, int idx = -1)
        { m_delay=delay; m_imageIndex=idx; }

    unsigned int m_delay;
    int m_imageIndex;
};

#include "wx/arrimpl.cpp" // this is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(wxImageArray)

#include "wx/arrimpl.cpp" // this is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(wxANIFrameInfoArray)


//---------------------------------------------------------------------------
// wxANIDecoder
//---------------------------------------------------------------------------

wxANIDecoder::wxANIDecoder()
{
}

wxANIDecoder::~wxANIDecoder()
{
}

bool wxANIDecoder::ConvertToImage(unsigned int frame, wxImage *image) const
{
    unsigned int idx = m_info[frame].m_imageIndex;
    *image = m_images[idx];       // copy
    return image->IsOk();
}


//---------------------------------------------------------------------------
// Data accessors
//---------------------------------------------------------------------------

wxSize wxANIDecoder::GetFrameSize(unsigned int WXUNUSED(frame)) const
{
    // all frames are of the same size...
    return m_szAnimation;
}

wxPoint wxANIDecoder::GetFramePosition(unsigned int WXUNUSED(frame)) const
{
    // all frames are of the same size...
    return wxPoint(0,0);
}

wxAnimationDisposal wxANIDecoder::GetDisposalMethod(unsigned int WXUNUSED(frame)) const
{
    // this disposal is implicit for all frames inside an ANI file
    return wxANIM_TOBACKGROUND;
}

long wxANIDecoder::GetDelay(unsigned int frame) const
{
    return m_info[frame].m_delay;
}

wxColour wxANIDecoder::GetTransparentColour(unsigned int frame) const
{
    unsigned int idx = m_info[frame].m_imageIndex;

    if (!m_images[idx].HasMask())
        return wxNullColour;

    return wxColour(m_images[idx].GetMaskRed(),
                    m_images[idx].GetMaskGreen(),
                    m_images[idx].GetMaskBlue());
}


//---------------------------------------------------------------------------
// ANI reading and decoding
//---------------------------------------------------------------------------

bool wxANIDecoder::CanRead(wxInputStream& stream) const
{
    wxInt32 FCC1, FCC2;
    wxUint32 datalen ;

    wxInt32 riff32;
    memcpy( &riff32, "RIFF", 4 );
    wxInt32 list32;
    memcpy( &list32, "LIST", 4 );
    wxInt32 ico32;
    memcpy( &ico32, "icon", 4 );
    wxInt32 anih32;
    memcpy( &anih32, "anih", 4 );

    stream.SeekI(0);
    if ( !stream.Read(&FCC1, 4) )
        return false;

    if ( FCC1 != riff32 )
        return false;

    // we have a riff file:
    while ( stream.IsOk() )
    {
        if ( FCC1 == anih32 )
            return true;        // found the ANIH chunk - this should be an ANI file

        // we always have a data size:
        stream.Read(&datalen, 4);
        datalen = wxINT32_SWAP_ON_BE(datalen) ;

        // data should be padded to make even number of bytes
        if (datalen % 2 == 1) datalen ++ ;

        // now either data or a FCC:
        if ( (FCC1 == riff32) || (FCC1 == list32) )
        {
            stream.Read(&FCC2, 4);
        }
        else
        {
            stream.SeekI(stream.TellI() + datalen);
        }

        // try to read next data chunk:
        if ( !stream.Read(&FCC1, 4) )
        {
            // reading failed -- either EOF or IO error, bail out anyhow
            return false;
        }
    }

    return false;
}

// the "anih" RIFF chunk
struct wxANIHeader
{
    wxInt32 cbSizeOf;     // Num bytes in AniHeader (36 bytes)
    wxInt32 cFrames;      // Number of unique Icons in this cursor
    wxInt32 cSteps;       // Number of Blits before the animation cycles
    wxInt32 cx;           // width of the frames
    wxInt32 cy;           // height of the frames
    wxInt32 cBitCount;    // bit depth
    wxInt32 cPlanes;      // 1
    wxInt32 JifRate;      // Default Jiffies (1/60th of a second) if rate chunk not present.
    wxInt32 flags;        // Animation Flag (see AF_ constants)

    // ANI files are always little endian so we need to swap bytes on big
    // endian architectures
#ifdef WORDS_BIGENDIAN
    void AdjustEndianness()
    {
        // this works because all our fields are wxInt32 and they must be
        // packed without holes between them (if they're not, they wouldn't map
        // to the file header!)
        wxInt32 * const start = (wxInt32 *)this;
        wxInt32 * const end = start + sizeof(wxANIHeader)/sizeof(wxInt32);
        for ( wxInt32 *p = start; p != end; p++ )
        {
            *p = wxINT32_SWAP_ALWAYS(*p);
        }
    }
#else
    void AdjustEndianness() { }
#endif
};

bool wxANIDecoder::Load( wxInputStream& stream )
{
    wxInt32 FCC1, FCC2;
    wxUint32 datalen;
    unsigned int globaldelay=0;

    wxInt32 riff32;
    memcpy( &riff32, "RIFF", 4 );
    wxInt32 list32;
    memcpy( &list32, "LIST", 4 );
    wxInt32 ico32;
    memcpy( &ico32, "icon", 4 );
    wxInt32 anih32;
    memcpy( &anih32, "anih", 4 );
    wxInt32 rate32;
    memcpy( &rate32, "rate", 4 );
    wxInt32 seq32;
    memcpy( &seq32, "seq ", 4 );

    stream.SeekI(0);
    stream.Read(&FCC1, 4);
    if ( FCC1 != riff32 )
        return false;

    m_nFrames = 0;
    m_szAnimation = wxDefaultSize;

    m_images.Clear();
    m_info.Clear();

    // we have a riff file:
    while ( stream.IsOk() )
    {
        // we always have a data size:
        stream.Read(&datalen, 4);
        datalen = wxINT32_SWAP_ON_BE(datalen);

        //data should be padded to make even number of bytes
        if (datalen % 2 == 1) datalen++;

        // now either data or a FCC:
        if ( (FCC1 == riff32) || (FCC1 == list32) )
        {
            stream.Read(&FCC2, 4);
        }
        else if ( FCC1 == anih32 )
        {
            if ( datalen != sizeof(wxANIHeader) )
                return false;

            if (m_nFrames > 0)
                return false;       // already parsed an ani header?

            struct wxANIHeader header;
            stream.Read(&header, sizeof(wxANIHeader));
            header.AdjustEndianness();

            // we should have a global frame size
            m_szAnimation = wxSize(header.cx, header.cy);

            // save interesting info from the header
            m_nFrames = header.cSteps;   // NB: not cFrames!!
            if ( m_nFrames == 0 )
                return false;

            globaldelay = header.JifRate * 1000 / 60;

            m_images.Alloc(header.cFrames);
            m_info.Add(wxANIFrameInfo(), m_nFrames);
        }
        else if ( FCC1 == rate32 )
        {
            // did we already process the anih32 chunk?
            if (m_nFrames == 0)
                return false;       // rate chunks should always be placed after anih chunk

            wxASSERT(m_info.GetCount() == m_nFrames);
            for (unsigned int i=0; i<m_nFrames; i++)
            {
                stream.Read(&FCC2, 4);
                m_info[i].m_delay = wxINT32_SWAP_ON_BE(FCC2) * 1000 / 60;
            }
        }
        else if ( FCC1 == seq32 )
        {
            // did we already process the anih32 chunk?
            if (m_nFrames == 0)
                return false;       // seq chunks should always be placed after anih chunk

            wxASSERT(m_info.GetCount() == m_nFrames);
            for (unsigned int i=0; i<m_nFrames; i++)
            {
                stream.Read(&FCC2, 4);
                m_info[i].m_imageIndex = wxINT32_SWAP_ON_BE(FCC2);
            }
        }
        else if ( FCC1 == ico32 )
        {
            // use DoLoadFile() and not LoadFile()!
            wxImage image;
            if (!sm_handler.DoLoadFile(&image, stream, false /* verbose */, -1))
                return false;

            m_images.Add(image);
        }
        else
        {
            stream.SeekI(stream.TellI() + datalen);
        }

        // try to read next data chunk:
        stream.Read(&FCC1, 4);
    }

    if (m_nFrames==0)
        return false;

    if (m_nFrames==m_images.GetCount())
    {
        // if no SEQ chunk is available, display the frames in the order
        // they were loaded
        for (unsigned int i=0; i<m_nFrames; i++)
            if (m_info[i].m_imageIndex == -1)
                m_info[i].m_imageIndex = i;
    }

    // if some frame has an invalid delay, use the global delay given in the
    // ANI header
    for (unsigned int i=0; i<m_nFrames; i++)
        if (m_info[i].m_delay == 0)
            m_info[i].m_delay = globaldelay;

    // if the header did not contain a valid frame size, try to grab
    // it from the size of the first frame (all frames are of the same size)
    if (m_szAnimation.GetWidth() == 0 ||
        m_szAnimation.GetHeight() == 0)
        m_szAnimation = wxSize(m_images[0].GetWidth(), m_images[0].GetHeight());

    return m_szAnimation != wxDefaultSize;
}

#endif // wxUSE_STREAMS && wxUSE_ICO_CUR
