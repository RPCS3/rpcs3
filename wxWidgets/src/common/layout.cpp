/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/layout.cpp
// Purpose:     Constraint layout system classes
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: layout.cpp 39627 2006-06-08 06:57:39Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// =============================================================================
// declarations
// =============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_CONSTRAINTS

#include "wx/layout.h"

#ifndef WX_PRECOMP
    #include "wx/window.h"
    #include "wx/utils.h"
    #include "wx/dialog.h"
    #include "wx/msgdlg.h"
    #include "wx/intl.h"
#endif


IMPLEMENT_DYNAMIC_CLASS(wxIndividualLayoutConstraint, wxObject)
IMPLEMENT_DYNAMIC_CLASS(wxLayoutConstraints, wxObject)


inline void wxGetAsIs(wxWindowBase* win, int* w, int* h)
{
#if 1
    // The old way.  Works for me.
    win->GetSize(w, h);
#endif

#if 0
    // Vadim's change.  Breaks wxPython's LayoutAnchors
    win->GetBestSize(w, h);
#endif

#if 0
    // Proposed compromise.  Doesn't work.
    int sw, sh, bw, bh;
    win->GetSize(&sw, &sh);
    win->GetBestSize(&bw, &bh);
    if (w)
        *w = wxMax(sw, bw);
    if (h)
        *h = wxMax(sh, bh);
#endif
}


wxIndividualLayoutConstraint::wxIndividualLayoutConstraint()
{
    myEdge = wxTop;
    relationship = wxUnconstrained;
    margin = 0;
    value = 0;
    percent = 0;
    otherEdge = wxTop;
    done = false;
    otherWin = (wxWindowBase *) NULL;
}

void wxIndividualLayoutConstraint::Set(wxRelationship rel, wxWindowBase *otherW, wxEdge otherE, int val, int marg)
{
    if (rel == wxSameAs)
    {
        // If Set is called by the user with wxSameAs then call SameAs to do
        // it since it will actually use wxPercent instead.
        SameAs(otherW, otherE, marg);
        return;
    }

    relationship = rel;
    otherWin = otherW;
    otherEdge = otherE;

    if ( rel == wxPercentOf )
    {
        percent = val;
    }
    else
    {
        value = val;
    }

    margin = marg;
}

void wxIndividualLayoutConstraint::LeftOf(wxWindowBase *sibling, int marg)
{
    Set(wxLeftOf, sibling, wxLeft, 0, marg);
}

void wxIndividualLayoutConstraint::RightOf(wxWindowBase *sibling, int marg)
{
    Set(wxRightOf, sibling, wxRight, 0, marg);
}

void wxIndividualLayoutConstraint::Above(wxWindowBase *sibling, int marg)
{
    Set(wxAbove, sibling, wxTop, 0, marg);
}

void wxIndividualLayoutConstraint::Below(wxWindowBase *sibling, int marg)
{
    Set(wxBelow, sibling, wxBottom, 0, marg);
}

//
// 'Same edge' alignment
//
void wxIndividualLayoutConstraint::SameAs(wxWindowBase *otherW, wxEdge edge, int marg)
{
    Set(wxPercentOf, otherW, edge, 100, marg);
}

// The edge is a percentage of the other window's edge
void wxIndividualLayoutConstraint::PercentOf(wxWindowBase *otherW, wxEdge wh, int per)
{
    Set(wxPercentOf, otherW, wh, per);
}

//
// Edge has absolute value
//
void wxIndividualLayoutConstraint::Absolute(int val)
{
    value = val;
    relationship = wxAbsolute;
}

// Reset constraint if it mentions otherWin
bool wxIndividualLayoutConstraint::ResetIfWin(wxWindowBase *otherW)
{
    if (otherW == otherWin)
    {
        myEdge = wxTop;
        relationship = wxAsIs;
        margin = 0;
        value = 0;
        percent = 0;
        otherEdge = wxTop;
        otherWin = (wxWindowBase *) NULL;
        return true;
    }

    return false;
}

// Try to satisfy constraint
bool wxIndividualLayoutConstraint::SatisfyConstraint(wxLayoutConstraints *constraints, wxWindowBase *win)
{
    if (relationship == wxAbsolute)
    {
        done = true;
        return true;
    }

    switch (myEdge)
    {
        case wxLeft:
        {
            switch (relationship)
            {
                case wxLeftOf:
                {
                    // We can know this edge if: otherWin is win's
                    // parent, or otherWin has a satisfied constraint,
                    // or otherWin has no constraint.
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos - margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxRightOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos + margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxPercentOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = (int)(edgePos*(((float)percent)*0.01) + margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxUnconstrained:
                {
                    // We know the left-hand edge position if we know
                    // the right-hand edge and we know the width; OR if
                    // we know the centre and the width.
                    if (constraints->right.GetDone() && constraints->width.GetDone())
                    {
                        value = (constraints->right.GetValue() - constraints->width.GetValue() + margin);
                        done = true;
                        return true;
                    }
                    else if (constraints->centreX.GetDone() && constraints->width.GetDone())
                    {
                        value = (int)(constraints->centreX.GetValue() - (constraints->width.GetValue()/2) + margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxAsIs:
                {
                    int y;
                    win->GetPosition(&value, &y);
                    done = true;
                    return true;
                }
                default:
                    break;
            }
            break;
        }
        case wxRight:
        {
            switch (relationship)
            {
                case wxLeftOf:
                {
                    // We can know this edge if: otherWin is win's
                    // parent, or otherWin has a satisfied constraint,
                    // or otherWin has no constraint.
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos - margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxRightOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos + margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxPercentOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = (int)(edgePos*(((float)percent)*0.01) - margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxUnconstrained:
                {
                    // We know the right-hand edge position if we know the
                    // left-hand edge and we know the width, OR if we know the
                    // centre edge and the width.
                    if (constraints->left.GetDone() && constraints->width.GetDone())
                    {
                        value = (constraints->left.GetValue() + constraints->width.GetValue() - margin);
                        done = true;
                        return true;
                    }
                    else if (constraints->centreX.GetDone() && constraints->width.GetDone())
                    {
                        value = (int)(constraints->centreX.GetValue() + (constraints->width.GetValue()/2) - margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxAsIs:
                {
                    int x, y;
                    int w, h;
                    wxGetAsIs(win, &w, &h);
                    win->GetPosition(&x, &y);
                    value = x + w;
                    done = true;
                    return true;
                }
                default:
                    break;
            }
            break;
        }
        case wxTop:
        {
            switch (relationship)
            {
                case wxAbove:
                {
                    // We can know this edge if: otherWin is win's
                    // parent, or otherWin has a satisfied constraint,
                    // or otherWin has no constraint.
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos - margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxBelow:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos + margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxPercentOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = (int)(edgePos*(((float)percent)*0.01) + margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxUnconstrained:
                {
                    // We know the top edge position if we know the bottom edge
                    // and we know the height; OR if we know the centre edge and
                    // the height.
                    if (constraints->bottom.GetDone() && constraints->height.GetDone())
                    {
                        value = (constraints->bottom.GetValue() - constraints->height.GetValue() + margin);
                        done = true;
                        return true;
                    }
                    else if (constraints->centreY.GetDone() && constraints->height.GetDone())
                    {
                        value = (constraints->centreY.GetValue() - (constraints->height.GetValue()/2) + margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxAsIs:
                {
                    int x;
                    win->GetPosition(&x, &value);
                    done = true;
                    return true;
                }
                default:
                    break;
            }
            break;
        }
        case wxBottom:
        {
            switch (relationship)
            {
                case wxAbove:
                {
                    // We can know this edge if: otherWin is win's parent,
                    // or otherWin has a satisfied constraint, or
                    // otherWin has no constraint.
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos + margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxBelow:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos - margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxPercentOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = (int)(edgePos*(((float)percent)*0.01) - margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxUnconstrained:
                {
                    // We know the bottom edge position if we know the top edge
                    // and we know the height; OR if we know the centre edge and
                    // the height.
                    if (constraints->top.GetDone() && constraints->height.GetDone())
                    {
                        value = (constraints->top.GetValue() + constraints->height.GetValue() - margin);
                        done = true;
                        return true;
                    }
                    else if (constraints->centreY.GetDone() && constraints->height.GetDone())
                    {
                        value = (constraints->centreY.GetValue() + (constraints->height.GetValue()/2) - margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxAsIs:
                {
                    int x, y;
                    int w, h;
                    wxGetAsIs(win, &w, &h);
                    win->GetPosition(&x, &y);
                    value = h + y;
                    done = true;
                    return true;
                }
                default:
                    break;
            }
            break;
        }
        case wxCentreX:
        {
            switch (relationship)
            {
                case wxLeftOf:
                {
                    // We can know this edge if: otherWin is win's parent, or
                    // otherWin has a satisfied constraint, or otherWin has no
                    // constraint.
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos - margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxRightOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos + margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxPercentOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = (int)(edgePos*(((float)percent)*0.01) + margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxUnconstrained:
                {
                    // We know the centre position if we know
                    // the left-hand edge and we know the width, OR
                    // the right-hand edge and the width
                    if (constraints->left.GetDone() && constraints->width.GetDone())
                    {
                        value = (int)(constraints->left.GetValue() + (constraints->width.GetValue()/2) + margin);
                        done = true;
                        return true;
                    }
                    else if (constraints->right.GetDone() && constraints->width.GetDone())
                    {
                        value = (int)(constraints->left.GetValue() - (constraints->width.GetValue()/2) + margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                default:
                    break;
            }
            break;
        }
        case wxCentreY:
        {
            switch (relationship)
            {
                case wxAbove:
                {
                    // We can know this edge if: otherWin is win's parent,
                    // or otherWin has a satisfied constraint, or otherWin
                    // has no constraint.
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos - margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxBelow:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = edgePos + margin;
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxPercentOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = (int)(edgePos*(((float)percent)*0.01) + margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxUnconstrained:
                {
                    // We know the centre position if we know
                    // the top edge and we know the height, OR
                    // the bottom edge and the height.
                    if (constraints->bottom.GetDone() && constraints->height.GetDone())
                    {
                        value = (int)(constraints->bottom.GetValue() - (constraints->height.GetValue()/2) + margin);
                        done = true;
                        return true;
                    }
                    else if (constraints->top.GetDone() && constraints->height.GetDone())
                    {
                        value = (int)(constraints->top.GetValue() + (constraints->height.GetValue()/2) + margin);
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                default:
                    break;
            }
            break;
        }
        case wxWidth:
        {
            switch (relationship)
            {
                case wxPercentOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = (int)(edgePos*(((float)percent)*0.01));
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxAsIs:
                {
                    if (win)
                    {
                        int h;
                        wxGetAsIs(win, &value, &h);
                        done = true;
                        return true;
                    }
                    else return false;
                }
                case wxUnconstrained:
                {
                    // We know the width if we know the left edge and the right edge, OR
                    // if we know the left edge and the centre, OR
                    // if we know the right edge and the centre
                    if (constraints->left.GetDone() && constraints->right.GetDone())
                    {
                        value = constraints->right.GetValue() - constraints->left.GetValue();
                        done = true;
                        return true;
                    }
                    else if (constraints->centreX.GetDone() && constraints->left.GetDone())
                    {
                        value = (int)(2*(constraints->centreX.GetValue() - constraints->left.GetValue()));
                        done = true;
                        return true;
                    }
                    else if (constraints->centreX.GetDone() && constraints->right.GetDone())
                    {
                        value = (int)(2*(constraints->right.GetValue() - constraints->centreX.GetValue()));
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                default:
                    break;
            }
            break;
        }
        case wxHeight:
        {
            switch (relationship)
            {
                case wxPercentOf:
                {
                    int edgePos = GetEdge(otherEdge, win, otherWin);
                    if (edgePos != -1)
                    {
                        value = (int)(edgePos*(((float)percent)*0.01));
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                case wxAsIs:
                {
                    if (win)
                    {
                        int w;
                        wxGetAsIs(win, &w, &value);
                        done = true;
                        return true;
                    }
                    else return false;
                }
                case wxUnconstrained:
                {
                    // We know the height if we know the top edge and the bottom edge, OR
                    // if we know the top edge and the centre, OR
                    // if we know the bottom edge and the centre
                    if (constraints->top.GetDone() && constraints->bottom.GetDone())
                    {
                        value = constraints->bottom.GetValue() - constraints->top.GetValue();
                        done = true;
                        return true;
                    }
                    else if (constraints->top.GetDone() && constraints->centreY.GetDone())
                    {
                        value = (int)(2*(constraints->centreY.GetValue() - constraints->top.GetValue()));
                        done = true;
                        return true;
                    }
                    else if (constraints->bottom.GetDone() && constraints->centreY.GetDone())
                    {
                        value = (int)(2*(constraints->bottom.GetValue() - constraints->centreY.GetValue()));
                        done = true;
                        return true;
                    }
                    else
                        return false;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    return false;
}

// Get the value of this edge or dimension, or if this is not determinable, -1.
int wxIndividualLayoutConstraint::GetEdge(wxEdge which,
                                          wxWindowBase *thisWin,
                                          wxWindowBase *other) const
{
    // If the edge or dimension belongs to the parent, then we know the
    // dimension is obtainable immediately. E.g. a wxExpandSizer may contain a
    // button (but the button's true parent is a panel, not the sizer)
    if (other->GetChildren().Find((wxWindow*)thisWin))
    {
        switch (which)
        {
            case wxLeft:
                {
                    return 0;
                }
            case wxTop:
                {
                    return 0;
                }
            case wxRight:
                {
                    int w, h;
                    other->GetClientSizeConstraint(&w, &h);
                    return w;
                }
            case wxBottom:
                {
                    int w, h;
                    other->GetClientSizeConstraint(&w, &h);
                    return h;
                }
            case wxWidth:
                {
                    int w, h;
                    other->GetClientSizeConstraint(&w, &h);
                    return w;
                }
            case wxHeight:
                {
                    int w, h;
                    other->GetClientSizeConstraint(&w, &h);
                    return h;
                }
            case wxCentreX:
            case wxCentreY:
                {
                    int w, h;
                    other->GetClientSizeConstraint(&w, &h);
                    if (which == wxCentreX)
                        return (int)(w/2);
                    else
                        return (int)(h/2);
                }
            default:
                return -1;
        }
    }
    switch (which)
    {
        case wxLeft:
            {
                wxLayoutConstraints *constr = other->GetConstraints();
                // If no constraints, it means the window is not dependent
                // on anything, and therefore we know its value immediately
                if (constr)
                {
                    if (constr->left.GetDone())
                        return constr->left.GetValue();
                    else
                        return -1;
                }
                else
                {
                    int x, y;
                    other->GetPosition(&x, &y);
                    return x;
                }
            }
        case wxTop:
            {
                wxLayoutConstraints *constr = other->GetConstraints();
                // If no constraints, it means the window is not dependent
                // on anything, and therefore we know its value immediately
                if (constr)
                {
                    if (constr->top.GetDone())
                        return constr->top.GetValue();
                    else
                        return -1;
                }
                else
                {
                    int x, y;
                    other->GetPosition(&x, &y);
                    return y;
                }
            }
        case wxRight:
            {
                wxLayoutConstraints *constr = other->GetConstraints();
                // If no constraints, it means the window is not dependent
                // on anything, and therefore we know its value immediately
                if (constr)
                {
                    if (constr->right.GetDone())
                        return constr->right.GetValue();
                    else
                        return -1;
                }
                else
                {
                    int x, y, w, h;
                    other->GetPosition(&x, &y);
                    other->GetSize(&w, &h);
                    return (int)(x + w);
                }
            }
        case wxBottom:
            {
                wxLayoutConstraints *constr = other->GetConstraints();
                // If no constraints, it means the window is not dependent
                // on anything, and therefore we know its value immediately
                if (constr)
                {
                    if (constr->bottom.GetDone())
                        return constr->bottom.GetValue();
                    else
                        return -1;
                }
                else
                {
                    int x, y, w, h;
                    other->GetPosition(&x, &y);
                    other->GetSize(&w, &h);
                    return (int)(y + h);
                }
            }
        case wxWidth:
            {
                wxLayoutConstraints *constr = other->GetConstraints();
                // If no constraints, it means the window is not dependent
                // on anything, and therefore we know its value immediately
                if (constr)
                {
                    if (constr->width.GetDone())
                        return constr->width.GetValue();
                    else
                        return -1;
                }
                else
                {
                    int w, h;
                    other->GetSize(&w, &h);
                    return w;
                }
            }
        case wxHeight:
            {
                wxLayoutConstraints *constr = other->GetConstraints();
                // If no constraints, it means the window is not dependent
                // on anything, and therefore we know its value immediately
                if (constr)
                {
                    if (constr->height.GetDone())
                        return constr->height.GetValue();
                    else
                        return -1;
                }
                else
                {
                    int w, h;
                    other->GetSize(&w, &h);
                    return h;
                }
            }
        case wxCentreX:
            {
                wxLayoutConstraints *constr = other->GetConstraints();
                // If no constraints, it means the window is not dependent
                // on anything, and therefore we know its value immediately
                if (constr)
                {
                    if (constr->centreX.GetDone())
                        return constr->centreX.GetValue();
                    else
                        return -1;
                }
                else
                {
                    int x, y, w, h;
                    other->GetPosition(&x, &y);
                    other->GetSize(&w, &h);
                    return (int)(x + (w/2));
                }
            }
        case wxCentreY:
            {
                wxLayoutConstraints *constr = other->GetConstraints();
                // If no constraints, it means the window is not dependent
                // on anything, and therefore we know its value immediately
                if (constr)
                {
                    if (constr->centreY.GetDone())
                        return constr->centreY.GetValue();
                    else
                        return -1;
                }
                else
                {
                    int x, y, w, h;
                    other->GetPosition(&x, &y);
                    other->GetSize(&w, &h);
                    return (int)(y + (h/2));
                }
            }
        default:
            break;
    }
    return -1;
}

wxLayoutConstraints::wxLayoutConstraints()
{
    left.SetEdge(wxLeft);
    top.SetEdge(wxTop);
    right.SetEdge(wxRight);
    bottom.SetEdge(wxBottom);
    centreX.SetEdge(wxCentreX);
    centreY.SetEdge(wxCentreY);
    width.SetEdge(wxWidth);
    height.SetEdge(wxHeight);
}

bool wxLayoutConstraints::SatisfyConstraints(wxWindowBase *win, int *nChanges)
{
    int noChanges = 0;

    bool done = width.GetDone();
    bool newDone = (done ? true : width.SatisfyConstraint(this, win));
    if (newDone != done)
        noChanges ++;

    done = height.GetDone();
    newDone = (done ? true : height.SatisfyConstraint(this, win));
    if (newDone != done)
        noChanges ++;

    done = left.GetDone();
    newDone = (done ? true : left.SatisfyConstraint(this, win));
    if (newDone != done)
        noChanges ++;

    done = top.GetDone();
    newDone = (done ? true : top.SatisfyConstraint(this, win));
    if (newDone != done)
        noChanges ++;

    done = right.GetDone();
    newDone = (done ? true : right.SatisfyConstraint(this, win));
    if (newDone != done)
        noChanges ++;

    done = bottom.GetDone();
    newDone = (done ? true : bottom.SatisfyConstraint(this, win));
    if (newDone != done)
        noChanges ++;

    done = centreX.GetDone();
    newDone = (done ? true : centreX.SatisfyConstraint(this, win));
    if (newDone != done)
        noChanges ++;

    done = centreY.GetDone();
    newDone = (done ? true : centreY.SatisfyConstraint(this, win));
    if (newDone != done)
        noChanges ++;

    *nChanges = noChanges;

    return AreSatisfied();
}

#endif // wxUSE_CONSTRAINTS
