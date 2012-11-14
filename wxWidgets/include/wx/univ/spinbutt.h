///////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/spinbutt.h
// Purpose:     universal version of wxSpinButton
// Author:      Vadim Zeitlin
// Modified by:
// Created:     21.01.01
// RCS-ID:      $Id: spinbutt.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 2001 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_SPINBUTT_H_
#define _WX_UNIV_SPINBUTT_H_

#include "wx/univ/scrarrow.h"

// ----------------------------------------------------------------------------
// wxSpinButton
// ----------------------------------------------------------------------------

// actions supported by this control
#define wxACTION_SPIN_INC    wxT("inc")
#define wxACTION_SPIN_DEC    wxT("dec")

class WXDLLEXPORT wxSpinButton : public wxSpinButtonBase,
                                 public wxControlWithArrows
{
public:
    wxSpinButton();
    wxSpinButton(wxWindow *parent,
                 wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxSP_VERTICAL | wxSP_ARROW_KEYS,
                 const wxString& name = wxSPIN_BUTTON_NAME);

    bool Create(wxWindow *parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxSP_VERTICAL | wxSP_ARROW_KEYS,
                const wxString& name = wxSPIN_BUTTON_NAME);

    // implement wxSpinButtonBase methods
    virtual int GetValue() const;
    virtual void SetValue(int val);
    virtual void SetRange(int minVal, int maxVal);

    // implement wxControlWithArrows methods
    virtual wxRenderer *GetRenderer() const { return m_renderer; }
    virtual wxWindow *GetWindow() { return this; }
    virtual bool IsVertical() const { return wxSpinButtonBase::IsVertical(); }
    virtual int GetArrowState(wxScrollArrows::Arrow arrow) const;
    virtual void SetArrowFlag(wxScrollArrows::Arrow arrow, int flag, bool set);
    virtual bool OnArrow(wxScrollArrows::Arrow arrow);
    virtual wxScrollArrows::Arrow HitTestArrow(const wxPoint& pt) const;

    // for wxStdSpinButtonInputHandler
    const wxScrollArrows& GetArrows() { return m_arrows; }

    virtual bool PerformAction(const wxControlAction& action,
                               long numArg = 0,
                               const wxString& strArg = wxEmptyString);

    static wxInputHandler *GetStdInputHandler(wxInputHandler *handlerDef);
    virtual wxInputHandler *DoGetStdInputHandler(wxInputHandler *handlerDef)
    {
        return GetStdInputHandler(handlerDef);
    }

protected:
    virtual wxSize DoGetBestClientSize() const;
    virtual void DoDraw(wxControlRenderer *renderer);
    virtual wxBorder GetDefaultBorder() const { return wxBORDER_NONE; }

    // the common part of all ctors
    void Init();

    // normalize the value to fit into min..max range
    int NormalizeValue(int value) const;

    // change the value by +1/-1 and send the event, return true if value was
    // changed
    bool ChangeValue(int inc);

    // get the rectangles for our 2 arrows
    void CalcArrowRects(wxRect *rect1, wxRect *rect2) const;

    // the current controls value
    int m_value;

private:
    // the object which manages our arrows
    wxScrollArrows m_arrows;

    // the state (combination of wxCONTROL_XXX flags) of the arrows
    int m_arrowsState[wxScrollArrows::Arrow_Max];

    DECLARE_DYNAMIC_CLASS(wxSpinButton)
};

// ----------------------------------------------------------------------------
// wxStdSpinButtonInputHandler: manages clicks on them (use arrows like
// wxStdScrollBarInputHandler) and processes keyboard events too
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStdSpinButtonInputHandler : public wxStdInputHandler
{
public:
    wxStdSpinButtonInputHandler(wxInputHandler *inphand);

    virtual bool HandleKey(wxInputConsumer *consumer,
                           const wxKeyEvent& event,
                           bool pressed);
    virtual bool HandleMouse(wxInputConsumer *consumer,
                             const wxMouseEvent& event);
    virtual bool HandleMouseMove(wxInputConsumer *consumer,
                                 const wxMouseEvent& event);
};

#endif // _WX_UNIV_SPINBUTT_H_

