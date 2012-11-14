/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/imagfill.cpp
// Purpose:     FloodFill for wxImage
// Author:      Julian Smart
// RCS-ID:      $Id: imagfill.cpp 63770 2010-03-28 22:34:12Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_IMAGE && !defined(__WXMSW__)
// we have no use for this code in wxMSW...

#ifndef WX_PRECOMP
    #include "wx/brush.h"
    #include "wx/dc.h"
    #include "wx/dcmemory.h"
    #include "wx/image.h"
#endif

// DoFloodFill
// Fills with the colour extracted from fillBrush, starting at x,y until either
// a color different from the start pixel is reached (wxFLOOD_SURFACE)
// or fill color is reached (wxFLOOD_BORDER)

static bool LINKAGEMODE MatchPixel(wxImage *img, int x, int y, int w, int h, const wxColour& c)
{
    if ((x<0)||(x>=w)||(y<0)||(y>=h)) return false;

    unsigned char r = img->GetRed(x,y);
    unsigned char g = img->GetGreen(x,y);
    unsigned char b = img->GetBlue(x,y);
    return c.Red() == r && c.Green() == g && c.Blue() == b ;
}

static bool LINKAGEMODE MatchBoundaryPixel(wxImage *img, int x, int y, int w, int h, const wxColour & fill, const wxColour& bound)
{
    if ((x<0)||(x>=w)||(y<0)||(y>=h)) return true;

    unsigned char r = img->GetRed(x,y);
    unsigned char g = img->GetGreen(x,y);
    unsigned char b = img->GetBlue(x,y);
    if ( fill.Red() == r && fill.Green() == g && fill.Blue() == b )
        return true;
    if ( bound.Red() == r && bound.Green() == g && bound.Blue() == b )
        return true;
    return false;
}


static void LINKAGEMODE
wxImageFloodFill(wxImage *image,
                 wxCoord x, wxCoord y, const wxBrush & fillBrush,
                 const wxColour& testColour, int style,
                 int WXUNUSED(LogicalFunction))
{
    /* A diamond flood-fill using a circular queue system.
    Each pixel surrounding the current pixel is added to
    the queue if it meets the criteria, then is retrieved in
    its turn.  Code originally based on http://www.drawit.co.nz/Developers.htm,
    with explicit permission to use this for wxWidgets granted by Andrew Empson
    (no copyright claimed)
     */

    int width = image->GetWidth();
    int height = image->GetHeight();

    //Draw using a pen made from the current brush colour
    //Potentially allows us to use patterned flood fills in future code
    wxColour fillColour = fillBrush.GetColour();
    unsigned char r = fillColour.Red();
    unsigned char g = fillColour.Green();
    unsigned char b = fillColour.Blue();

    //initial test :
    if (style == wxFLOOD_SURFACE)
    {
       //if wxFLOOD_SURFACE, if fill colour is same as required, we don't do anything
       if (     image->GetRed(x,y)   != r
             || image->GetGreen(x,y) != g
             || image->GetBlue (x,y) != b   )
        {
        //prepare memory for queue
        //queue save, start, read
        size_t *qs, *qst, *qr;

        //queue size (physical)
        long qSz= height * width * 2;
        qst = new size_t [qSz];

        //temporary x and y locations
        int xt, yt;

        for (int i=0; i < qSz; i++)
            qst[i] = 0;

        // start queue
        qs=qr=qst;
        *qs=xt=x;
        qs++;
        *qs=yt=y;
        qs++;

        image->SetRGB(xt,yt,r,g,b);

        //Main queue loop
        while(qr!=qs)
        {
            //Add new members to queue
            //Above current pixel
            if(MatchPixel(image,xt,yt-1,width,height,testColour))
            {
                *qs=xt;
                qs++;
                *qs=yt-1;
                qs++;
                image->SetRGB(xt,yt-1,r,g,b);

                //Loop back to beginning of queue
                if(qs>=(qst+qSz)) qs=qst;
            }

            //Below current pixel
            if(MatchPixel(image,xt,yt+1,width,height,testColour))
            {
                *qs=xt;
                qs++;
                *qs=yt+1;
                qs++;
                image->SetRGB(xt,yt+1,r,g,b);
                if(qs>=(qst+qSz)) qs=qst;
            }

            //Left of current pixel
            if(MatchPixel(image,xt-1,yt,width,height,testColour))
            {
                *qs=xt-1;
                qs++;
                *qs=yt;
                qs++;
                image->SetRGB(xt-1,yt,r,g,b);
                if(qs>=(qst+qSz)) qs=qst;
            }

            //Right of current pixel
            if(MatchPixel(image,xt+1,yt,width,height,testColour))
            {
                *qs=xt+1;
                qs++;
                *qs=yt;
                qs++;
                image->SetRGB(xt+1,yt,r,g,b);
                if(qs>=(qst+qSz)) qs=qst;
            }

            //Retrieve current queue member
            qr+=2;

            //Loop back to the beginning
            if(qr>=(qst+qSz)) qr=qst;
            xt=*qr;
            yt=*(qr+1);

        //Go Back to beginning of loop
        }

        delete[] qst;
        }
    }
    else
    {
    //style is wxFLOOD_BORDER
    // fill up to testColor border - if already testColour don't do anything
    if (  image->GetRed(x,y)   != testColour.Red()
          || image->GetGreen(x,y) != testColour.Green()
          || image->GetBlue(x,y)  != testColour.Blue() )
    {
        //prepare memory for queue
        //queue save, start, read
        size_t *qs, *qst, *qr;

        //queue size (physical)
        long qSz= height * width * 2;
        qst = new size_t [qSz];

        //temporary x and y locations
        int xt, yt;

        for (int i=0; i < qSz; i++)
            qst[i] = 0;

        // start queue
        qs=qr=qst;
        *qs=xt=x;
        qs++;
        *qs=yt=y;
        qs++;

        image->SetRGB(xt,yt,r,g,b);

        //Main queue loop
        while (qr!=qs)
        {
            //Add new members to queue
            //Above current pixel
            if(!MatchBoundaryPixel(image,xt,yt-1,width,height,fillColour,testColour))
            {
                *qs=xt;
                qs++;
                *qs=yt-1;
                qs++;
                image->SetRGB(xt,yt-1,r,g,b);

                //Loop back to beginning of queue
                if(qs>=(qst+qSz)) qs=qst;
            }

            //Below current pixel
            if(!MatchBoundaryPixel(image,xt,yt+1,width,height,fillColour,testColour))
            {
                *qs=xt;
                qs++;
                *qs=yt+1;
                qs++;
                image->SetRGB(xt,yt+1,r,g,b);
                if(qs>=(qst+qSz)) qs=qst;
            }

            //Left of current pixel
            if(!MatchBoundaryPixel(image,xt-1,yt,width,height,fillColour,testColour))
            {
                *qs=xt-1;
                qs++;
                *qs=yt;
                qs++;
                image->SetRGB(xt-1,yt,r,g,b);
                if(qs>=(qst+qSz)) qs=qst;
            }

            //Right of current pixel
            if(!MatchBoundaryPixel(image,xt+1,yt,width,height,fillColour,testColour))
            {
                *qs=xt+1;
                qs++;
                *qs=yt;
                qs++;
                image->SetRGB(xt+1,yt,r,g,b);
                if(qs>=(qst+qSz)) qs=qst;
            }

            //Retrieve current queue member
            qr+=2;

            //Loop back to the beginning
            if(qr>=(qst+qSz)) qr=qst;
            xt=*qr;
            yt=*(qr+1);

        //Go Back to beginning of loop
        }

        delete[] qst;
        }
    }
    //all done,
}


bool wxDoFloodFill(wxDC *dc, wxCoord x, wxCoord y,
                   const wxColour& col, int style)
{
    if (dc->GetBrush().GetStyle() == wxTRANSPARENT)
        return true;

    int height = 0;
    int width  = 0;
    dc->GetSize(&width, &height);

    //it would be nice to fail if we don't get a sensible size...
    wxCHECK_MSG(width >= 1 && height >= 1, false,
                wxT("In FloodFill, dc.GetSize routine failed, method not supported by this DC"));

    const int x_dev = dc->LogicalToDeviceX(x);
    const int y_dev = dc->LogicalToDeviceY(y);

    // if start point is outside dc, can't do anything
    if (!wxRect(0, 0, width, height).Contains(x_dev, y_dev))
        return false;

    wxBitmap bitmap(width, height);
    wxMemoryDC memdc(bitmap);
    // match dc scales
    double sx, sy;
    dc->GetUserScale(&sx, &sy);
    memdc.SetUserScale(sx, sy);
    dc->GetLogicalScale(&sx, &sy);
    memdc.SetLogicalScale(sx, sy);

    // get logical size and origin
    const int w_log = dc->DeviceToLogicalXRel(width);
    const int h_log = dc->DeviceToLogicalYRel(height);
    const int x0_log = dc->DeviceToLogicalX(0);
    const int y0_log = dc->DeviceToLogicalY(0);

    memdc.Blit(0, 0, w_log, h_log, dc, x0_log, y0_log);
    memdc.SelectObject(wxNullBitmap);

    wxImage image = bitmap.ConvertToImage();
    wxImageFloodFill(&image, x_dev, y_dev, dc->GetBrush(), col, style,
                     dc->GetLogicalFunction());
    bitmap = wxBitmap(image);
    memdc.SelectObject(bitmap);
    dc->Blit(x0_log, y0_log, w_log, h_log, &memdc, 0, 0);

    return true;
}

#endif // wxUSE_IMAGE
