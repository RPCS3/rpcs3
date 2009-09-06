/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/region.cpp
// Purpose:     generic wxRegion class
// Author:      David Elliott
// Modified by:
// Created:     2004/04/12
// RCS-ID:      $Id: regiong.cpp 41444 2006-09-25 18:18:26Z VZ $
// Copyright:   (c) 2004 David Elliott
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/region.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
#endif

// ========================================================================
// Classes to interface with X.org code
// ========================================================================

typedef struct Box
{
    wxCoord x1, x2, y1, y2;
} Box, BOX, BoxRec, *BoxPtr;

typedef struct REGION *Region;

struct REGION
{
public:
    // Default constructor initializes nothing
    REGION() {}

    REGION(const wxRect& rect)
    {
        rects = &extents;
        numRects = 1;
        extents.x1 = rect.x;
        extents.y1 = rect.y;
        extents.x2 = rect.x + rect.width;
        extents.y2 = rect.y + rect.height;
        size = 1;
    }

    BoxPtr GetBox(int i)
    {
        return i < numRects ? rects + i : NULL;
    }

    // X.org methods
    static bool XClipBox(
        Region r,
        wxRect *rect);
    static bool XOffsetRegion(
        register Region pRegion,
        register int x,
        register int y);
    static bool XIntersectRegion(
        Region reg1,
        Region reg2,          /* source regions     */
        register Region newReg);               /* destination Region */
    static bool XUnionRegion(
        Region reg1,
        Region reg2,             /* source regions     */
        Region newReg);                  /* destination Region */
    static bool XSubtractRegion(
        Region regM,
        Region regS,
        register Region regD);
    static bool XXorRegion(Region sra, Region srb, Region dr);
    static bool XEmptyRegion(
        Region r);
    static bool XEqualRegion(Region r1, Region r2);
    static bool XPointInRegion(
        Region pRegion,
        int x, int y);
    static wxRegionContain XRectInRegion(
        register Region region,
        int rx, int ry,
        unsigned int rwidth, unsigned int rheight);

protected:
    static Region XCreateRegion(void);
    static void miSetExtents (
        Region pReg);
    static bool XDestroyRegion(Region r);
    static int miIntersectO (
        register Region pReg,
        register BoxPtr r1,
        BoxPtr r1End,
        register BoxPtr r2,
        BoxPtr r2End,
        wxCoord y1,
        wxCoord y2);
    static void miRegionCopy(
        register Region dstrgn,
        register Region rgn);
    static int miCoalesce(
        register Region pReg, /* Region to coalesce */
        int prevStart, /* Index of start of previous band */
        int curStart); /* Index of start of current band */
    static void miRegionOp(
        register Region newReg, /* Place to store result */
        Region reg1, /* First region in operation */
        Region reg2, /* 2d region in operation */
        int (*overlapFunc)(
            register Region     pReg,
            register BoxPtr     r1,
            BoxPtr              r1End,
            register BoxPtr     r2,
            BoxPtr              r2End,
            wxCoord             y1,
            wxCoord             y2), /* Function to call for over-
                                      * lapping bands */
        int (*nonOverlap1Func)(
            register Region     pReg,
            register BoxPtr     r,
            BoxPtr              rEnd,
            register wxCoord    y1,
            register wxCoord    y2), /* Function to call for non-
                                      * overlapping bands in region
                                      * 1 */
        int (*nonOverlap2Func)(
            register Region     pReg,
            register BoxPtr     r,
            BoxPtr              rEnd,
            register wxCoord    y1,
            register wxCoord    y2)); /* Function to call for non-
                                       * overlapping bands in region
                                       * 2 */
    static int miUnionNonO (
        register Region pReg,
        register BoxPtr r,
        BoxPtr rEnd,
        register wxCoord y1,
        register wxCoord y2);
    static int miUnionO (
        register Region pReg,
        register BoxPtr r1,
        BoxPtr r1End,
        register BoxPtr r2,
        BoxPtr r2End,
        register wxCoord y1,
        register wxCoord y2);
    static int miSubtractNonO1 (
        register Region pReg,
        register BoxPtr r,
        BoxPtr rEnd,
        register wxCoord y1,
        register wxCoord y2);
    static int miSubtractO (
        register Region pReg,
        register BoxPtr r1,
        BoxPtr r1End,
        register BoxPtr r2,
        BoxPtr r2End,
        register wxCoord y1,
        register wxCoord y2);
protected:
    long size;
    long numRects;
    Box *rects;
    Box extents;
};

// ========================================================================
// wxRegionRefData
// ========================================================================

class wxRegionRefData : public wxObjectRefData,
                        public REGION
{
public:
    wxRegionRefData()
        : wxObjectRefData(),
          REGION()
    {
        size = 1;
        numRects = 0;
        rects = ( BOX * )malloc( (unsigned) sizeof( BOX ));
        extents.x1 = 0;
        extents.x2 = 0;
        extents.y1 = 0;
        extents.y2 = 0;
    }

    wxRegionRefData(const wxPoint& topLeft, const wxPoint& bottomRight)
        : wxObjectRefData(),
          REGION()
    {
        rects = (BOX*)malloc(sizeof(BOX));
        size = 1;
        numRects = 1;
        extents.x1 = topLeft.x;
        extents.y1 = topLeft.y;
        extents.x2 = bottomRight.x;
        extents.y2 = bottomRight.y;
        *rects = extents;
    }

    wxRegionRefData(const wxRect& rect)
        : wxObjectRefData(),
          REGION(rect)
    {
        rects = (BOX*)malloc(sizeof(BOX));
        *rects = extents;
    }

    wxRegionRefData(const wxRegionRefData& refData)
        : wxObjectRefData(),
          REGION()
    {
        size = refData.size;
        numRects = refData.numRects;
        rects = (Box*)malloc(numRects*sizeof(Box));
        memcpy(rects, refData.rects, numRects*sizeof(Box));
        extents = refData.extents;
    }

    virtual ~wxRegionRefData()
    {
        free(rects);
    }

private:
    // Don't allow this
    wxRegionRefData(const REGION&);
};

// ========================================================================
// wxRegionGeneric
// ========================================================================
//IMPLEMENT_DYNAMIC_CLASS(wxRegionGeneric, wxGDIObject)

#define M_REGIONDATA ((wxRegionRefData *)m_refData)
#define M_REGIONDATA_OF(rgn) ((wxRegionRefData *)(rgn.m_refData))

// ----------------------------------------------------------------------------
// wxRegionGeneric construction
// ----------------------------------------------------------------------------

wxRegionGeneric::wxRegionGeneric()
{
}

wxRegionGeneric::~wxRegionGeneric()
{
}

wxRegionGeneric::wxRegionGeneric(wxCoord x, wxCoord y, wxCoord w, wxCoord h)
{
    m_refData = new wxRegionRefData(wxRect(x,y,w,h));
}

wxRegionGeneric::wxRegionGeneric(const wxRect& rect)
{
    m_refData = new wxRegionRefData(rect);
}

wxRegionGeneric::wxRegionGeneric(const wxPoint& topLeft, const wxPoint& bottomRight)
{
    m_refData = new wxRegionRefData(topLeft, bottomRight);
}

void wxRegionGeneric::Clear()
{
    UnRef();
}

wxObjectRefData *wxRegionGeneric::CreateRefData() const
{
    return new wxRegionRefData;
}

wxObjectRefData *wxRegionGeneric::CloneRefData(const wxObjectRefData *data) const
{
    return new wxRegionRefData(*(wxRegionRefData *)data);
}

bool wxRegionGeneric::DoIsEqual(const wxRegion& region) const
{
    return REGION::XEqualRegion(M_REGIONDATA,M_REGIONDATA_OF(region));
}

bool wxRegionGeneric::DoGetBox(wxCoord& x, wxCoord& y, wxCoord&w, wxCoord &h) const
{
    if ( !m_refData )
        return false;

    wxRect rect;
    REGION::XClipBox(M_REGIONDATA,&rect);
    x = rect.x;
    y = rect.y;
    w = rect.width;
    h = rect.height;
    return true;
}

// ----------------------------------------------------------------------------
// wxRegionGeneric operations
// ----------------------------------------------------------------------------

bool wxRegionGeneric::DoUnionWithRect(const wxRect& rect)
{
    if ( rect.IsEmpty() )
    {
        // nothing to do
        return true;
    }

    AllocExclusive();
    REGION region(rect);
    return REGION::XUnionRegion(&region,M_REGIONDATA,M_REGIONDATA);
}

bool wxRegionGeneric::DoUnionWithRegion(const wxRegion& region)
{
    AllocExclusive();
    return REGION::XUnionRegion(M_REGIONDATA_OF(region),M_REGIONDATA,M_REGIONDATA);
}

bool wxRegionGeneric::DoIntersect(const wxRegion& region)
{
    AllocExclusive();
    return REGION::XIntersectRegion(M_REGIONDATA_OF(region),M_REGIONDATA,M_REGIONDATA);
}

bool wxRegionGeneric::DoSubtract(const wxRegion& region)
{
    if ( region.IsEmpty() )
    {
        // nothing to do
        return true;
    }

    return REGION::XSubtractRegion(M_REGIONDATA_OF(region),M_REGIONDATA,M_REGIONDATA);
}

bool wxRegionGeneric::DoXor(const wxRegion& region)
{
    AllocExclusive();
    return REGION::XXorRegion(M_REGIONDATA_OF(region),M_REGIONDATA,M_REGIONDATA);
}

bool wxRegionGeneric::DoOffset(wxCoord x, wxCoord y)
{
    AllocExclusive();
    return REGION::XOffsetRegion(M_REGIONDATA, x, y);
}

// ----------------------------------------------------------------------------
// wxRegionGeneric comparison
// ----------------------------------------------------------------------------

bool wxRegionGeneric::IsEmpty() const
{
    wxASSERT(m_refData);
    return REGION::XEmptyRegion(M_REGIONDATA);
}

// Does the region contain the point (x,y)?
wxRegionContain wxRegionGeneric::DoContainsPoint(wxCoord x, wxCoord y) const
{
    wxASSERT(m_refData);
    return REGION::XPointInRegion(M_REGIONDATA,x,y) ? wxInRegion : wxOutRegion;
}

// Does the region contain the rectangle rect?
wxRegionContain wxRegionGeneric::DoContainsRect(const wxRect& rect) const
{
    wxASSERT(m_refData);
    return REGION::XRectInRegion(M_REGIONDATA,rect.x,rect.y,rect.width,rect.height);
}

// ========================================================================
// wxRegionIteratorGeneric
// ========================================================================
//IMPLEMENT_DYNAMIC_CLASS(wxRegionIteratorGeneric,wxObject)

wxRegionIteratorGeneric::wxRegionIteratorGeneric()
{
    m_current = 0;
}

wxRegionIteratorGeneric::wxRegionIteratorGeneric(const wxRegionGeneric& region)
:   m_region(region)
{
    m_current = 0;
}

wxRegionIteratorGeneric::wxRegionIteratorGeneric(const wxRegionIteratorGeneric& iterator)
:   m_region(iterator.m_region)
{
    m_current = iterator.m_current;
}

void wxRegionIteratorGeneric::Reset(const wxRegionGeneric& region)
{
    m_region = region;
    m_current = 0;
}

bool wxRegionIteratorGeneric::HaveRects() const
{
    return M_REGIONDATA_OF(m_region)->GetBox(m_current);
}

wxRegionIteratorGeneric& wxRegionIteratorGeneric::operator++()
{
    ++m_current;
    return *this;
}

wxRegionIteratorGeneric wxRegionIteratorGeneric::operator++(int)
{
    wxRegionIteratorGeneric copy(*this);
    ++*this;
    return copy;
}

wxRect wxRegionIteratorGeneric::GetRect() const
{
    wxASSERT(m_region.m_refData);
    const Box *box = M_REGIONDATA_OF(m_region)->GetBox(m_current);
    wxASSERT(box);
    return wxRect
    (   box->x1
    ,   box->y1
    ,   box->x2 - box->x1
    ,   box->y2 - box->y1
    );
}

long wxRegionIteratorGeneric::GetX() const
{
    wxASSERT(m_region.m_refData);
    const Box *box = M_REGIONDATA_OF(m_region)->GetBox(m_current);
    wxASSERT(box);
    return box->x1;
}

long wxRegionIteratorGeneric::GetY() const
{
    wxASSERT(m_region.m_refData);
    const Box *box = M_REGIONDATA_OF(m_region)->GetBox(m_current);
    wxASSERT(box);
    return box->y1;
}

long wxRegionIteratorGeneric::GetW() const
{
    wxASSERT(m_region.m_refData);
    const Box *box = M_REGIONDATA_OF(m_region)->GetBox(m_current);
    wxASSERT(box);
    return box->x2 - box->x1;
}

long wxRegionIteratorGeneric::GetH() const
{
    wxASSERT(m_region.m_refData);
    const Box *box = M_REGIONDATA_OF(m_region)->GetBox(m_current);
    wxASSERT(box);
    return box->y2 - box->y1;
}

wxRegionIteratorGeneric::~wxRegionIteratorGeneric()
{
}


// ========================================================================
// The guts (from X.org)
// ========================================================================

/************************************************************************

Copyright 1987, 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

/*  1 if two BOXs overlap.
 *  0 if two BOXs do not overlap.
 *  Remember, x2 and y2 are not in the region
 */
#define EXTENTCHECK(r1, r2) \
    ((r1)->x2 > (r2)->x1 && \
     (r1)->x1 < (r2)->x2 && \
     (r1)->y2 > (r2)->y1 && \
     (r1)->y1 < (r2)->y2)

/*
 *   Check to see if there is enough memory in the present region.
 */
#define MEMCHECK(reg, rect, firstrect){\
        if ((reg)->numRects >= ((reg)->size - 1)){\
          (firstrect) = (BOX *) realloc \
          ((char *)(firstrect), (unsigned) (2 * (sizeof(BOX)) * ((reg)->size)));\
          if ((firstrect) == 0)\
            return(0);\
          (reg)->size *= 2;\
          (rect) = &(firstrect)[(reg)->numRects];\
         }\
       }

#define EMPTY_REGION(pReg) pReg->numRects = 0

#define REGION_NOT_EMPTY(pReg) pReg->numRects

#define INBOX(r, x, y) \
      ( ( ((r).x2 >  x)) && \
        ( ((r).x1 <= x)) && \
        ( ((r).y2 >  y)) && \
        ( ((r).y1 <= y)) )

/*
 * The functions in this file implement the Region abstraction, similar to one
 * used in the X11 sample server. A Region is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each Region is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (y1) will have the same lower-right y
 * coordinate (y2) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */

/* Create a new empty region */
Region REGION::XCreateRegion(void)
{
    Region temp = new REGION;

    if (!temp)
        return (Region) NULL;

    temp->rects = ( BOX * )malloc( (unsigned) sizeof( BOX ));

    if (!temp->rects)
    {
        free((char *) temp);
        return (Region) NULL;
    }
    temp->numRects = 0;
    temp->extents.x1 = 0;
    temp->extents.y1 = 0;
    temp->extents.x2 = 0;
    temp->extents.y2 = 0;
    temp->size = 1;
    return( temp );
}

bool REGION::XClipBox(Region r, wxRect *rect)
{
    rect->x = r->extents.x1;
    rect->y = r->extents.y1;
    rect->width = r->extents.x2 - r->extents.x1;
    rect->height = r->extents.y2 - r->extents.y1;
    return true;
}

/*-
 *-----------------------------------------------------------------------
 * miSetExtents --
 *    Reset the extents of a region to what they should be. Called by
 *    miSubtract and miIntersect b/c they can't figure it out along the
 *    way or do so easily, as miUnion can.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    The region's 'extents' structure is overwritten.
 *
 *-----------------------------------------------------------------------
 */
void REGION::
miSetExtents (Region pReg)
{
    register BoxPtr pBox,
                    pBoxEnd,
                    pExtents;

    if (pReg->numRects == 0)
    {
        pReg->extents.x1 = 0;
        pReg->extents.y1 = 0;
        pReg->extents.x2 = 0;
        pReg->extents.y2 = 0;
        return;
    }

    pExtents = &pReg->extents;
    pBox = pReg->rects;
    pBoxEnd = &pBox[pReg->numRects - 1];

    /*
     * Since pBox is the first rectangle in the region, it must have the
     * smallest y1 and since pBoxEnd is the last rectangle in the region,
     * it must have the largest y2, because of banding. Initialize x1 and
     * x2 from  pBox and pBoxEnd, resp., as good things to initialize them
     * to...
     */
    pExtents->x1 = pBox->x1;
    pExtents->y1 = pBox->y1;
    pExtents->x2 = pBoxEnd->x2;
    pExtents->y2 = pBoxEnd->y2;

    assert(pExtents->y1 < pExtents->y2);
    while (pBox <= pBoxEnd)
    {
        if (pBox->x1 < pExtents->x1)
        {
            pExtents->x1 = pBox->x1;
        }
        if (pBox->x2 > pExtents->x2)
        {
            pExtents->x2 = pBox->x2;
        }
        pBox++;
    }
    assert(pExtents->x1 < pExtents->x2);
}

bool REGION::
XDestroyRegion(
    Region r)
{
    free( (char *) r->rects );
    delete r;
    return true;
}

/* TranslateRegion(pRegion, x, y)
   translates in place
   added by raymond
*/

bool REGION::
XOffsetRegion(
    register Region pRegion,
    register int x,
    register int y)
{
    register int nbox;
    register BOX *pbox;

    pbox = pRegion->rects;
    nbox = pRegion->numRects;

    while(nbox--)
    {
        pbox->x1 += x;
        pbox->x2 += x;
        pbox->y1 += y;
        pbox->y2 += y;
        pbox++;
    }
    pRegion->extents.x1 += x;
    pRegion->extents.x2 += x;
    pRegion->extents.y1 += y;
    pRegion->extents.y2 += y;
    return 1;
}

/*======================================================================
 *  Region Intersection
 *====================================================================*/
/*-
 *-----------------------------------------------------------------------
 * miIntersectO --
 *    Handle an overlapping band for miIntersect.
 *
 * Results:
 *    None.
 *
 * Side Effects:
 *    Rectangles may be added to the region.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
int REGION::
miIntersectO (
    register Region     pReg,
    register BoxPtr     r1,
    BoxPtr              r1End,
    register BoxPtr     r2,
    BoxPtr              r2End,
    wxCoord             y1,
    wxCoord             y2)
{
    register wxCoord    x1;
    register wxCoord    x2;
    register BoxPtr     pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    while ((r1 != r1End) && (r2 != r2End))
    {
        x1 = wxMax(r1->x1,r2->x1);
        x2 = wxMin(r1->x2,r2->x2);

        /*
         * If there's any overlap between the two rectangles, add that
         * overlap to the new region.
         * There's no need to check for subsumption because the only way
         * such a need could arise is if some region has two rectangles
         * right next to each other. Since that should never happen...
         */
        if (x1 < x2)
        {
            assert(y1<y2);

            MEMCHECK(pReg, pNextRect, pReg->rects);
            pNextRect->x1 = x1;
            pNextRect->y1 = y1;
            pNextRect->x2 = x2;
            pNextRect->y2 = y2;
            pReg->numRects += 1;
            pNextRect++;
            assert(pReg->numRects <= pReg->size);
        }

        /*
         * Need to advance the pointers. Shift the one that extends
         * to the right the least, since the other still has a chance to
         * overlap with that region's next rectangle, if you see what I mean.
         */
        if (r1->x2 < r2->x2)
        {
            r1++;
        }
        else if (r2->x2 < r1->x2)
        {
            r2++;
        }
        else
        {
            r1++;
            r2++;
        }
    }
    return 0; /* lint */
}

bool REGION::
XIntersectRegion(
    Region reg1,
    Region reg2, /* source regions     */
    register Region newReg) /* destination Region */
{
   /* check for trivial reject */
    if ( (!(reg1->numRects)) || (!(reg2->numRects))  ||
        (!EXTENTCHECK(&reg1->extents, &reg2->extents)))
        newReg->numRects = 0;
    else
        miRegionOp (newReg, reg1, reg2,
                    miIntersectO, NULL, NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the same. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents(newReg);
    return 1;
}

void REGION::
miRegionCopy(
    register Region dstrgn,
    register Region rgn)

{
    if (dstrgn != rgn) /*  don't want to copy to itself */
    {
        if (dstrgn->size < rgn->numRects)
        {
            if (dstrgn->rects)
            {
                BOX *prevRects = dstrgn->rects;

                dstrgn->rects = (BOX *)
                       realloc((char *) dstrgn->rects,
                               (unsigned) rgn->numRects * (sizeof(BOX)));
                if (!dstrgn->rects)
                {
                    free(prevRects);
                    return;
                }
            }
            dstrgn->size = rgn->numRects;
        }
        dstrgn->numRects = rgn->numRects;
        dstrgn->extents.x1 = rgn->extents.x1;
        dstrgn->extents.y1 = rgn->extents.y1;
        dstrgn->extents.x2 = rgn->extents.x2;
        dstrgn->extents.y2 = rgn->extents.y2;

        memcpy((char *) dstrgn->rects, (char *) rgn->rects,
                (int) (rgn->numRects * sizeof(BOX)));
    }
}

/*======================================================================
 * Generic Region Operator
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miCoalesce --
 *    Attempt to merge the boxes in the current band with those in the
 *    previous one. Used only by miRegionOp.
 *
 * Results:
 *    The new index for the previous band.
 *
 * Side Effects:
 *    If coalescing takes place:
 *        - rectangles in the previous band will have their y2 fields
 *          altered.
 *        - pReg->numRects will be decreased.
 *
 *-----------------------------------------------------------------------
 */
/* static int*/
int REGION::
miCoalesce(
    register Region pReg,     /* Region to coalesce */
    int prevStart,            /* Index of start of previous band */
    int curStart)             /* Index of start of current band */
{
    register BoxPtr pPrevBox; /* Current box in previous band */
    register BoxPtr pCurBox;  /* Current box in current band */
    register BoxPtr pRegEnd;  /* End of region */
    int         curNumRects;  /* Number of rectangles in current
                               * band */
    int        prevNumRects;  /* Number of rectangles in previous
                               * band */
    int              bandY1;  /* Y1 coordinate for current band */

    pRegEnd = &pReg->rects[pReg->numRects];

    pPrevBox = &pReg->rects[prevStart];
    prevNumRects = curStart - prevStart;

    /*
     * Figure out how many rectangles are in the current band. Have to do
     * this because multiple bands could have been added in miRegionOp
     * at the end when one region has been exhausted.
     */
    pCurBox = &pReg->rects[curStart];
    bandY1 = pCurBox->y1;
    for (curNumRects = 0;
         (pCurBox != pRegEnd) && (pCurBox->y1 == bandY1);
         curNumRects++)
    {
        pCurBox++;
    }

    if (pCurBox != pRegEnd)
    {
        /*
         * If more than one band was added, we have to find the start
         * of the last band added so the next coalescing job can start
         * at the right place... (given when multiple bands are added,
         * this may be pointless -- see above).
         */
        pRegEnd--;
        while (pRegEnd[-1].y1 == pRegEnd->y1)
        {
            pRegEnd--;
        }
        curStart = pRegEnd - pReg->rects;
        pRegEnd = pReg->rects + pReg->numRects;
    }

    if ((curNumRects == prevNumRects) && (curNumRects != 0))
    {
        pCurBox -= curNumRects;
        /*
         * The bands may only be coalesced if the bottom of the previous
         * matches the top scanline of the current.
         */
        if (pPrevBox->y2 == pCurBox->y1)
        {
        /*
         * Make sure the bands have boxes in the same places. This
         * assumes that boxes have been added in such a way that they
         * cover the most area possible. I.e. two boxes in a band must
         * have some horizontal space between them.
         */
            do
            {
                if ((pPrevBox->x1 != pCurBox->x1) ||
                    (pPrevBox->x2 != pCurBox->x2))
                {
                    /*
                     * The bands don't line up so they can't be coalesced.
                     */
                    return (curStart);
                }
                pPrevBox++;
                pCurBox++;
                prevNumRects -= 1;
            } while (prevNumRects != 0);

            pReg->numRects -= curNumRects;
            pCurBox -= curNumRects;
            pPrevBox -= curNumRects;

            /*
             * The bands may be merged, so set the bottom y of each box
             * in the previous band to that of the corresponding box in
             * the current band.
             */
            do
            {
                pPrevBox->y2 = pCurBox->y2;
                pPrevBox++;
                pCurBox++;
                curNumRects -= 1;
            } while (curNumRects != 0);

            /*
             * If only one band was added to the region, we have to backup
             * curStart to the start of the previous band.
             *
             * If more than one band was added to the region, copy the
             * other bands down. The assumption here is that the other bands
             * came from the same region as the current one and no further
             * coalescing can be done on them since it's all been done
             * already... curStart is already in the right place.
             */
            if (pCurBox == pRegEnd)
            {
                curStart = prevStart;
            }
            else
            {
                do
                {
                    *pPrevBox++ = *pCurBox++;
                } while (pCurBox != pRegEnd);
            }

        }
    }
    return (curStart);
}

/*-
 *-----------------------------------------------------------------------
 * miRegionOp --
 *        Apply an operation to two regions. Called by miUnion, miInverse,
 *        miSubtract, miIntersect...
 *
 * Results:
 *        None.
 *
 * Side Effects:
 *        The new region is overwritten.
 *
 * Notes:
 *        The idea behind this function is to view the two regions as sets.
 *        Together they cover a rectangle of area that this function divides
 *        into horizontal bands where points are covered only by one region
 *        or by both. For the first case, the nonOverlapFunc is called with
 *        each the band and the band's upper and lower extents. For the
 *        second, the overlapFunc is called to process the entire band. It
 *        is responsible for clipping the rectangles in the band, though
 *        this function provides the boundaries.
 *        At the end of each band, the new region is coalesced, if possible,
 *        to reduce the number of rectangles in the region.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
void REGION::
miRegionOp(
    register Region         newReg,                              /* Place to store result */
    Region                  reg1,                                /* First region in operation */
    Region                  reg2,                                /* 2d region in operation */
    int                      (*overlapFunc)(
        register Region     pReg,
        register BoxPtr     r1,
        BoxPtr              r1End,
        register BoxPtr     r2,
        BoxPtr              r2End,
        wxCoord               y1,
        wxCoord               y2),              /* Function to call for over-
                                                 * lapping bands */
    int                      (*nonOverlap1Func)(
        register Region     pReg,
        register BoxPtr     r,
        BoxPtr              rEnd,
        register wxCoord      y1,
        register wxCoord      y2),              /* Function to call for non-
                                                 * overlapping bands in region
                                                 * 1 */
    int                      (*nonOverlap2Func)(
        register Region     pReg,
        register BoxPtr     r,
        BoxPtr              rEnd,
        register wxCoord      y1,
        register wxCoord      y2))              /* Function to call for non-
                                                 * overlapping bands in region
                                                 * 2 */
{
    register BoxPtr        r1; /* Pointer into first region */
    register BoxPtr        r2; /* Pointer into 2d region */
    BoxPtr              r1End; /* End of 1st region */
    BoxPtr              r2End; /* End of 2d region */
    register wxCoord     ybot; /* Bottom of intersection */
    register wxCoord     ytop; /* Top of intersection */
    BoxPtr           oldRects; /* Old rects for newReg */
    int              prevBand; /* Index of start of
                                * previous band in newReg */
    int               curBand; /* Index of start of current
                                * band in newReg */
    register BoxPtr r1BandEnd; /* End of current band in r1 */
    register BoxPtr r2BandEnd; /* End of current band in r2 */
    wxCoord               top; /* Top of non-overlapping
                                * band */
    wxCoord               bot; /* Bottom of non-overlapping
                                * band */

    /*
     * Initialization:
     *        set r1, r2, r1End and r2End appropriately, preserve the important
     * parts of the destination region until the end in case it's one of
     * the two source regions, then mark the "new" region empty, allocating
     * another array of rectangles for it to use.
     */
    r1 = reg1->rects;
    r2 = reg2->rects;
    r1End = r1 + reg1->numRects;
    r2End = r2 + reg2->numRects;

    oldRects = newReg->rects;

    EMPTY_REGION(newReg);

    /*
     * Allocate a reasonable number of rectangles for the new region. The idea
     * is to allocate enough so the individual functions don't need to
     * reallocate and copy the array, which is time consuming, yet we don't
     * have to worry about using too much memory. I hope to be able to
     * nuke the realloc() at the end of this function eventually.
     */
    newReg->size = wxMax(reg1->numRects,reg2->numRects) * 2;

    newReg->rects = (BoxPtr)malloc((unsigned) (sizeof(BoxRec) * newReg->size));

    if (!newReg->rects)
    {
        newReg->size = 0;
        return;
    }

    /*
     * Initialize ybot and ytop.
     * In the upcoming loop, ybot and ytop serve different functions depending
     * on whether the band being handled is an overlapping or non-overlapping
     * band.
     *         In the case of a non-overlapping band (only one of the regions
     * has points in the band), ybot is the bottom of the most recent
     * intersection and thus clips the top of the rectangles in that band.
     * ytop is the top of the next intersection between the two regions and
     * serves to clip the bottom of the rectangles in the current band.
     *        For an overlapping band (where the two regions intersect), ytop clips
     * the top of the rectangles of both regions and ybot clips the bottoms.
     */
    if (reg1->extents.y1 < reg2->extents.y1)
        ybot = reg1->extents.y1;
    else
        ybot = reg2->extents.y1;

    /*
     * prevBand serves to mark the start of the previous band so rectangles
     * can be coalesced into larger rectangles. qv. miCoalesce, above.
     * In the beginning, there is no previous band, so prevBand == curBand
     * (curBand is set later on, of course, but the first band will always
     * start at index 0). prevBand and curBand must be indices because of
     * the possible expansion, and resultant moving, of the new region's
     * array of rectangles.
     */
    prevBand = 0;

    do
    {
        curBand = newReg->numRects;

        /*
         * This algorithm proceeds one source-band (as opposed to a
         * destination band, which is determined by where the two regions
         * intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
         * rectangle after the last one in the current band for their
         * respective regions.
         */
        r1BandEnd = r1;
        while ((r1BandEnd != r1End) && (r1BandEnd->y1 == r1->y1))
        {
            r1BandEnd++;
        }

        r2BandEnd = r2;
        while ((r2BandEnd != r2End) && (r2BandEnd->y1 == r2->y1))
        {
            r2BandEnd++;
        }

        /*
         * First handle the band that doesn't intersect, if any.
         *
         * Note that attention is restricted to one band in the
         * non-intersecting region at once, so if a region has n
         * bands between the current position and the next place it overlaps
         * the other, this entire loop will be passed through n times.
         */
        if (r1->y1 < r2->y1)
        {
            top = wxMax(r1->y1,ybot);
            bot = wxMin(r1->y2,r2->y1);

            if ((top != bot) && (nonOverlap1Func != NULL))
            {
                (* nonOverlap1Func) (newReg, r1, r1BandEnd, top, bot);
            }

            ytop = r2->y1;
        }
        else if (r2->y1 < r1->y1)
        {
            top = wxMax(r2->y1,ybot);
            bot = wxMin(r2->y2,r1->y1);

            if ((top != bot) && (nonOverlap2Func != NULL))
            {
                (* nonOverlap2Func) (newReg, r2, r2BandEnd, top, bot);
            }

            ytop = r1->y1;
        }
        else
        {
            ytop = r1->y1;
        }

        /*
         * If any rectangles got added to the region, try and coalesce them
         * with rectangles from the previous band. Note we could just do
         * this test in miCoalesce, but some machines incur a not
         * inconsiderable cost for function calls, so...
         */
        if (newReg->numRects != curBand)
        {
            prevBand = miCoalesce (newReg, prevBand, curBand);
        }

        /*
         * Now see if we've hit an intersecting band. The two bands only
         * intersect if ybot > ytop
         */
        ybot = wxMin(r1->y2, r2->y2);
        curBand = newReg->numRects;
        if (ybot > ytop)
        {
            (* overlapFunc) (newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);

        }

        if (newReg->numRects != curBand)
        {
            prevBand = miCoalesce (newReg, prevBand, curBand);
        }

        /*
         * If we've finished with a band (y2 == ybot) we skip forward
         * in the region to the next band.
         */
        if (r1->y2 == ybot)
        {
            r1 = r1BandEnd;
        }
        if (r2->y2 == ybot)
        {
            r2 = r2BandEnd;
        }
    } while ((r1 != r1End) && (r2 != r2End));

    /*
     * Deal with whichever region still has rectangles left.
     */
    curBand = newReg->numRects;
    if (r1 != r1End)
    {
        if (nonOverlap1Func != NULL)
        {
            do
            {
                r1BandEnd = r1;
                while ((r1BandEnd < r1End) && (r1BandEnd->y1 == r1->y1))
                {
                    r1BandEnd++;
                }
                (* nonOverlap1Func) (newReg, r1, r1BandEnd,
                                     wxMax(r1->y1,ybot), r1->y2);
                r1 = r1BandEnd;
            } while (r1 != r1End);
        }
    }
    else if ((r2 != r2End) && (nonOverlap2Func != NULL))
    {
        do
        {
            r2BandEnd = r2;
            while ((r2BandEnd < r2End) && (r2BandEnd->y1 == r2->y1))
            {
                 r2BandEnd++;
            }
            (* nonOverlap2Func) (newReg, r2, r2BandEnd,
                                wxMax(r2->y1,ybot), r2->y2);
            r2 = r2BandEnd;
        } while (r2 != r2End);
    }

    if (newReg->numRects != curBand)
    {
        (void) miCoalesce (newReg, prevBand, curBand);
    }

    /*
     * A bit of cleanup. To keep regions from growing without bound,
     * we shrink the array of rectangles to match the new number of
     * rectangles in the region. This never goes to 0, however...
     *
     * Only do this stuff if the number of rectangles allocated is more than
     * twice the number of rectangles in the region (a simple optimization...).
     */
    if (newReg->numRects < (newReg->size >> 1))
    {
        if (REGION_NOT_EMPTY(newReg))
        {
            BoxPtr prev_rects = newReg->rects;
            newReg->size = newReg->numRects;
            newReg->rects = (BoxPtr) realloc ((char *) newReg->rects,
                                   (unsigned) (sizeof(BoxRec) * newReg->size));
            if (! newReg->rects)
                newReg->rects = prev_rects;
        }
        else
        {
            /*
             * No point in doing the extra work involved in an realloc if
             * the region is empty
             */
            newReg->size = 1;
            free((char *) newReg->rects);
            newReg->rects = (BoxPtr) malloc(sizeof(BoxRec));
        }
    }
    free ((char *) oldRects);
    return;
}

/*======================================================================
 *            Region Union
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miUnionNonO --
 *        Handle a non-overlapping band for the union operation. Just
 *        Adds the rectangles into the region. Doesn't have to check for
 *        subsumption or anything.
 *
 * Results:
 *        None.
 *
 * Side Effects:
 *        pReg->numRects is incremented and the final rectangles overwritten
 *        with the rectangles we're passed.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
int REGION::
miUnionNonO (
    register Region        pReg,
    register BoxPtr        r,
    BoxPtr                 rEnd,
    register wxCoord       y1,
    register wxCoord       y2)
{
    register BoxPtr        pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    assert(y1 < y2);

    while (r != rEnd)
    {
        assert(r->x1 < r->x2);
        MEMCHECK(pReg, pNextRect, pReg->rects);
        pNextRect->x1 = r->x1;
        pNextRect->y1 = y1;
        pNextRect->x2 = r->x2;
        pNextRect->y2 = y2;
        pReg->numRects += 1;
        pNextRect++;

        assert(pReg->numRects<=pReg->size);
        r++;
    }
    return 0;        /* lint */
}


/*-
 *-----------------------------------------------------------------------
 * miUnionO --
 *        Handle an overlapping band for the union operation. Picks the
 *        left-most rectangle each time and merges it into the region.
 *
 * Results:
 *        None.
 *
 * Side Effects:
 *        Rectangles are overwritten in pReg->rects and pReg->numRects will
 *        be changed.
 *
 *-----------------------------------------------------------------------
 */

/* static void*/
int REGION::
miUnionO (
    register Region        pReg,
    register BoxPtr        r1,
    BoxPtr                 r1End,
    register BoxPtr        r2,
    BoxPtr                 r2End,
    register wxCoord       y1,
    register wxCoord       y2)
{
    register BoxPtr        pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

#define MERGERECT(r) \
    if ((pReg->numRects != 0) &&  \
        (pNextRect[-1].y1 == y1) &&  \
        (pNextRect[-1].y2 == y2) &&  \
        (pNextRect[-1].x2 >= r->x1))  \
    {  \
        if (pNextRect[-1].x2 < r->x2)  \
        {  \
            pNextRect[-1].x2 = r->x2;  \
            assert(pNextRect[-1].x1<pNextRect[-1].x2); \
        }  \
    }  \
    else  \
    {  \
        MEMCHECK(pReg, pNextRect, pReg->rects);  \
        pNextRect->y1 = y1;  \
        pNextRect->y2 = y2;  \
        pNextRect->x1 = r->x1;  \
        pNextRect->x2 = r->x2;  \
        pReg->numRects += 1;  \
        pNextRect += 1;  \
    }  \
    assert(pReg->numRects<=pReg->size);\
    r++;

    assert (y1<y2);
    while ((r1 != r1End) && (r2 != r2End))
    {
        if (r1->x1 < r2->x1)
        {
            MERGERECT(r1);
        }
        else
        {
            MERGERECT(r2);
        }
    }

    if (r1 != r1End)
    {
        do
        {
            MERGERECT(r1);
        } while (r1 != r1End);
    }
    else while (r2 != r2End)
    {
        MERGERECT(r2);
    }
    return 0;        /* lint */
}

bool REGION::
XUnionRegion(
    Region           reg1,
    Region          reg2,             /* source regions     */
    Region           newReg)                  /* destination Region */
{
    /*  checks all the simple cases */

    /*
     * Region 1 and 2 are the same or region 1 is empty
     */
    if ( (reg1 == reg2) || (!(reg1->numRects)) )
    {
        if (newReg != reg2)
            miRegionCopy(newReg, reg2);
        return 1;
    }

    /*
     * if nothing to union (region 2 empty)
     */
    if (!(reg2->numRects))
    {
        if (newReg != reg1)
            miRegionCopy(newReg, reg1);
        return 1;
    }

    /*
     * Region 1 completely subsumes region 2
     */
    if ((reg1->numRects == 1) &&
        (reg1->extents.x1 <= reg2->extents.x1) &&
        (reg1->extents.y1 <= reg2->extents.y1) &&
        (reg1->extents.x2 >= reg2->extents.x2) &&
        (reg1->extents.y2 >= reg2->extents.y2))
    {
        if (newReg != reg1)
            miRegionCopy(newReg, reg1);
        return 1;
    }

    /*
     * Region 2 completely subsumes region 1
     */
    if ((reg2->numRects == 1) &&
        (reg2->extents.x1 <= reg1->extents.x1) &&
        (reg2->extents.y1 <= reg1->extents.y1) &&
        (reg2->extents.x2 >= reg1->extents.x2) &&
        (reg2->extents.y2 >= reg1->extents.y2))
    {
        if (newReg != reg2)
            miRegionCopy(newReg, reg2);
        return 1;
    }

    miRegionOp (newReg, reg1, reg2, miUnionO,
                    miUnionNonO, miUnionNonO);

    newReg->extents.x1 = wxMin(reg1->extents.x1, reg2->extents.x1);
    newReg->extents.y1 = wxMin(reg1->extents.y1, reg2->extents.y1);
    newReg->extents.x2 = wxMax(reg1->extents.x2, reg2->extents.x2);
    newReg->extents.y2 = wxMax(reg1->extents.y2, reg2->extents.y2);

    return 1;
}

/*======================================================================
 *                       Region Subtraction
 *====================================================================*/

/*-
 *-----------------------------------------------------------------------
 * miSubtractNonO --
 *        Deal with non-overlapping band for subtraction. Any parts from
 *        region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *        None.
 *
 * Side Effects:
 *        pReg may be affected.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
int REGION::
miSubtractNonO1 (
    register Region        pReg,
    register BoxPtr        r,
    BoxPtr                    rEnd,
    register wxCoord          y1,
    register wxCoord           y2)
{
    register BoxPtr        pNextRect;

    pNextRect = &pReg->rects[pReg->numRects];

    assert(y1<y2);

    while (r != rEnd)
    {
        assert(r->x1<r->x2);
        MEMCHECK(pReg, pNextRect, pReg->rects);
        pNextRect->x1 = r->x1;
        pNextRect->y1 = y1;
        pNextRect->x2 = r->x2;
        pNextRect->y2 = y2;
        pReg->numRects += 1;
        pNextRect++;

        assert(pReg->numRects <= pReg->size);

        r++;
    }
    return 0;        /* lint */
}

/*-
 *-----------------------------------------------------------------------
 * miSubtractO --
 *        Overlapping band subtraction. x1 is the left-most point not yet
 *        checked.
 *
 * Results:
 *        None.
 *
 * Side Effects:
 *        pReg may have rectangles added to it.
 *
 *-----------------------------------------------------------------------
 */
/* static void*/
int REGION::
miSubtractO (
    register Region        pReg,
    register BoxPtr        r1,
    BoxPtr                    r1End,
    register BoxPtr        r2,
    BoxPtr                    r2End,
    register wxCoord          y1,
    register wxCoord          y2)
{
    register BoxPtr        pNextRect;
    register int          x1;

    x1 = r1->x1;

    assert(y1<y2);
    pNextRect = &pReg->rects[pReg->numRects];

    while ((r1 != r1End) && (r2 != r2End))
    {
        if (r2->x2 <= x1)
        {
            /*
             * Subtrahend missed the boat: go to next subtrahend.
             */
            r2++;
        }
        else if (r2->x1 <= x1)
        {
            /*
             * Subtrahend preceeds minuend: nuke left edge of minuend.
             */
            x1 = r2->x2;
            if (x1 >= r1->x2)
            {
                /*
                 * Minuend completely covered: advance to next minuend and
                 * reset left fence to edge of new minuend.
                 */
                r1++;
                if (r1 != r1End)
                    x1 = r1->x1;
            }
            else
            {
                /*
                 * Subtrahend now used up since it doesn't extend beyond
                 * minuend
                 */
                r2++;
            }
        }
        else if (r2->x1 < r1->x2)
        {
            /*
             * Left part of subtrahend covers part of minuend: add uncovered
             * part of minuend to region and skip to next subtrahend.
             */
            assert(x1<r2->x1);
            MEMCHECK(pReg, pNextRect, pReg->rects);
            pNextRect->x1 = x1;
            pNextRect->y1 = y1;
            pNextRect->x2 = r2->x1;
            pNextRect->y2 = y2;
            pReg->numRects += 1;
            pNextRect++;

            assert(pReg->numRects<=pReg->size);

            x1 = r2->x2;
            if (x1 >= r1->x2)
            {
                /*
                 * Minuend used up: advance to new...
                 */
                r1++;
                if (r1 != r1End)
                    x1 = r1->x1;
            }
            else
            {
                /*
                 * Subtrahend used up
                 */
                r2++;
            }
        }
        else
        {
            /*
             * Minuend used up: add any remaining piece before advancing.
             */
            if (r1->x2 > x1)
            {
                MEMCHECK(pReg, pNextRect, pReg->rects);
                pNextRect->x1 = x1;
                pNextRect->y1 = y1;
                pNextRect->x2 = r1->x2;
                pNextRect->y2 = y2;
                pReg->numRects += 1;
                pNextRect++;
                assert(pReg->numRects<=pReg->size);
            }
            r1++;
            if (r1 != r1End)
                x1 = r1->x1;
        }
    }

    /*
     * Add remaining minuend rectangles to region.
     */
    while (r1 != r1End)
    {
        assert(x1<r1->x2);
        MEMCHECK(pReg, pNextRect, pReg->rects);
        pNextRect->x1 = x1;
        pNextRect->y1 = y1;
        pNextRect->x2 = r1->x2;
        pNextRect->y2 = y2;
        pReg->numRects += 1;
        pNextRect++;

        assert(pReg->numRects<=pReg->size);

        r1++;
        if (r1 != r1End)
        {
            x1 = r1->x1;
        }
    }
    return 0;        /* lint */
}

/*-
 *-----------------------------------------------------------------------
 * miSubtract --
 *        Subtract regS from regM and leave the result in regD.
 *        S stands for subtrahend, M for minuend and D for difference.
 *
 * Results:
 *        true.
 *
 * Side Effects:
 *        regD is overwritten.
 *
 *-----------------------------------------------------------------------
 */

bool REGION::XSubtractRegion(Region regM, Region regS, register Region regD)
{
   /* check for trivial reject */
    if ( (!(regM->numRects)) || (!(regS->numRects))  ||
        (!EXTENTCHECK(&regM->extents, &regS->extents)) )
    {
        miRegionCopy(regD, regM);
        return true;
    }

    miRegionOp (regD, regM, regS, miSubtractO,
                    miSubtractNonO1, NULL);

    /*
     * Can't alter newReg's extents before we call miRegionOp because
     * it might be one of the source regions and miRegionOp depends
     * on the extents of those regions being the unaltered. Besides, this
     * way there's no checking against rectangles that will be nuked
     * due to coalescing, so we have to examine fewer rectangles.
     */
    miSetExtents (regD);
    return true;
}

bool REGION::XXorRegion(Region sra, Region srb, Region dr)
{
    Region tra = XCreateRegion();

    wxCHECK_MSG( tra, false, wxT("region not created") );

    Region trb = XCreateRegion();

    wxCHECK_MSG( trb, false, wxT("region not created") );

    (void) XSubtractRegion(sra,srb,tra);
    (void) XSubtractRegion(srb,sra,trb);
    (void) XUnionRegion(tra,trb,dr);
    XDestroyRegion(tra);
    XDestroyRegion(trb);
    return 0;
}

/*
 * Check to see if the region is empty.  Assumes a region is passed
 * as a parameter
 */
bool REGION::XEmptyRegion(Region r)
{
    if( r->numRects == 0 ) return true;
    else  return false;
}

/*
 *        Check to see if two regions are equal
 */
bool REGION::XEqualRegion(Region r1, Region r2)
{
    int i;

    if( r1->numRects != r2->numRects ) return false;
    else if( r1->numRects == 0 ) return true;
    else if ( r1->extents.x1 != r2->extents.x1 ) return false;
    else if ( r1->extents.x2 != r2->extents.x2 ) return false;
    else if ( r1->extents.y1 != r2->extents.y1 ) return false;
    else if ( r1->extents.y2 != r2->extents.y2 ) return false;
    else for( i=0; i < r1->numRects; i++ ) {
            if ( r1->rects[i].x1 != r2->rects[i].x1 ) return false;
            else if ( r1->rects[i].x2 != r2->rects[i].x2 ) return false;
            else if ( r1->rects[i].y1 != r2->rects[i].y1 ) return false;
            else if ( r1->rects[i].y2 != r2->rects[i].y2 ) return false;
    }
    return true;
}

bool REGION::XPointInRegion(Region pRegion, int x, int y)
{
    int i;

    if (pRegion->numRects == 0)
        return false;
    if (!INBOX(pRegion->extents, x, y))
        return false;
    for (i=0; i<pRegion->numRects; i++)
    {
        if (INBOX (pRegion->rects[i], x, y))
            return true;
    }
    return false;
}

wxRegionContain REGION::XRectInRegion(register Region region,
                                      int rx, int ry,
                                      unsigned int rwidth,
                                      unsigned int rheight)
{
    register BoxPtr pbox;
    register BoxPtr pboxEnd;
    Box rect;
    register BoxPtr prect = &rect;
    int      partIn, partOut;

    prect->x1 = rx;
    prect->y1 = ry;
    prect->x2 = rwidth + rx;
    prect->y2 = rheight + ry;

    /* this is (just) a useful optimization */
    if ((region->numRects == 0) || !EXTENTCHECK(&region->extents, prect))
        return(wxOutRegion);

    partOut = false;
    partIn = false;

    /* can stop when both partOut and partIn are true, or we reach prect->y2 */
    for (pbox = region->rects, pboxEnd = pbox + region->numRects;
         pbox < pboxEnd;
         pbox++)
    {

        if (pbox->y2 <= ry)
           continue;        /* getting up to speed or skipping remainder of band */

        if (pbox->y1 > ry)
        {
           partOut = true;        /* missed part of rectangle above */
           if (partIn || (pbox->y1 >= prect->y2))
              break;
           ry = pbox->y1;        /* x guaranteed to be == prect->x1 */
        }

        if (pbox->x2 <= rx)
           continue;                /* not far enough over yet */

        if (pbox->x1 > rx)
        {
           partOut = true;        /* missed part of rectangle to left */
           if (partIn)
              break;
        }

        if (pbox->x1 < prect->x2)
        {
            partIn = true;        /* definitely overlap */
            if (partOut)
               break;
        }

        if (pbox->x2 >= prect->x2)
        {
           ry = pbox->y2;        /* finished with this band */
           if (ry >= prect->y2)
              break;
           rx = prect->x1;        /* reset x out to left again */
        } else
        {
            /*
             * Because boxes in a band are maximal width, if the first box
             * to overlap the rectangle doesn't completely cover it in that
             * band, the rectangle must be partially out, since some of it
             * will be uncovered in that band. partIn will have been set true
             * by now...
             */
            break;
        }

    }

    return(partIn ? ((ry < prect->y2) ? wxPartRegion : wxInRegion) :
                wxOutRegion);
}
