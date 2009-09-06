/////////////////////////////////////////////////////////////////////////////
// Name:        wx/layout.h
// Purpose:     OBSOLETE layout constraint classes, use sizers instead
// Author:      Julian Smart
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: layout.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) 1998 Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LAYOUT_H_
#define _WX_LAYOUT_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/object.h"

// X stupidly defines these in X.h
#ifdef Above
    #undef Above
#endif
#ifdef Below
    #undef Below
#endif

#if wxUSE_CONSTRAINTS

// ----------------------------------------------------------------------------
// forward declrations
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxWindowBase;
class WXDLLIMPEXP_FWD_CORE wxLayoutConstraints;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define wxLAYOUT_DEFAULT_MARGIN 0

enum wxEdge
{
    wxLeft, wxTop, wxRight, wxBottom, wxWidth, wxHeight,
    wxCentre, wxCenter = wxCentre, wxCentreX, wxCentreY
};

enum wxRelationship
{
    wxUnconstrained = 0,
    wxAsIs,
    wxPercentOf,
    wxAbove,
    wxBelow,
    wxLeftOf,
    wxRightOf,
    wxSameAs,
    wxAbsolute
};

// ----------------------------------------------------------------------------
// wxIndividualLayoutConstraint: a constraint on window position
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxIndividualLayoutConstraint : public wxObject
{
public:
    wxIndividualLayoutConstraint();

    // note that default copy ctor and assignment operators are ok

    virtual ~wxIndividualLayoutConstraint(){}

    void Set(wxRelationship rel, wxWindowBase *otherW, wxEdge otherE, int val = 0, int marg = wxLAYOUT_DEFAULT_MARGIN);

    //
    // Sibling relationships
    //
    void LeftOf(wxWindowBase *sibling, int marg = wxLAYOUT_DEFAULT_MARGIN);
    void RightOf(wxWindowBase *sibling, int marg = wxLAYOUT_DEFAULT_MARGIN);
    void Above(wxWindowBase *sibling, int marg = wxLAYOUT_DEFAULT_MARGIN);
    void Below(wxWindowBase *sibling, int marg = wxLAYOUT_DEFAULT_MARGIN);

    //
    // 'Same edge' alignment
    //
    void SameAs(wxWindowBase *otherW, wxEdge edge, int marg = wxLAYOUT_DEFAULT_MARGIN);

    // The edge is a percentage of the other window's edge
    void PercentOf(wxWindowBase *otherW, wxEdge wh, int per);

    //
    // Edge has absolute value
    //
    void Absolute(int val);

    //
    // Dimension is unconstrained
    //
    void Unconstrained() { relationship = wxUnconstrained; }

    //
    // Dimension is 'as is' (use current size settings)
    //
    void AsIs() { relationship = wxAsIs; }

    //
    // Accessors
    //
    wxWindowBase *GetOtherWindow() { return otherWin; }
    wxEdge GetMyEdge() const { return myEdge; }
    void SetEdge(wxEdge which) { myEdge = which; }
    void SetValue(int v) { value = v; }
    int GetMargin() { return margin; }
    void SetMargin(int m) { margin = m; }
    int GetValue() const { return value; }
    int GetPercent() const { return percent; }
    int GetOtherEdge() const { return otherEdge; }
    bool GetDone() const { return done; }
    void SetDone(bool d) { done = d; }
    wxRelationship GetRelationship() { return relationship; }
    void SetRelationship(wxRelationship r) { relationship = r; }

    // Reset constraint if it mentions otherWin
    bool ResetIfWin(wxWindowBase *otherW);

    // Try to satisfy constraint
    bool SatisfyConstraint(wxLayoutConstraints *constraints, wxWindowBase *win);

    // Get the value of this edge or dimension, or if this
    // is not determinable, -1.
    int GetEdge(wxEdge which, wxWindowBase *thisWin, wxWindowBase *other) const;

protected:
    // To be allowed to modify the internal variables
    friend class wxIndividualLayoutConstraint_Serialize;

    // 'This' window is the parent or sibling of otherWin
    wxWindowBase *otherWin;

    wxEdge myEdge;
    wxRelationship relationship;
    int margin;
    int value;
    int percent;
    wxEdge otherEdge;
    bool done;

    DECLARE_DYNAMIC_CLASS(wxIndividualLayoutConstraint)
};

// ----------------------------------------------------------------------------
// wxLayoutConstraints: the complete set of constraints for a window
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxLayoutConstraints : public wxObject
{
public:
    // Edge constraints
    wxIndividualLayoutConstraint left;
    wxIndividualLayoutConstraint top;
    wxIndividualLayoutConstraint right;
    wxIndividualLayoutConstraint bottom;
    // Size constraints
    wxIndividualLayoutConstraint width;
    wxIndividualLayoutConstraint height;
    // Centre constraints
    wxIndividualLayoutConstraint centreX;
    wxIndividualLayoutConstraint centreY;

    wxLayoutConstraints();

    // note that default copy ctor and assignment operators are ok

    virtual ~wxLayoutConstraints(){}

    bool SatisfyConstraints(wxWindowBase *win, int *noChanges);
    bool AreSatisfied() const
    {
        return left.GetDone() && top.GetDone() &&
               width.GetDone() && height.GetDone();
    }

    DECLARE_DYNAMIC_CLASS(wxLayoutConstraints)
};

#endif // wxUSE_CONSTRAINTS

#endif // _WX_LAYOUT_H_
