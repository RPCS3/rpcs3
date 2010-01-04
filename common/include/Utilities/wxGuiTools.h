/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#if wxUSE_GUI

#include "Dependencies.h"
#include "ScopedPtr.h"
#include <stack>

#include <wx/wx.h>

class pxStaticText;
class pxStaticHeading;
class pxCheckBox;

#define wxSF		wxSizerFlags()

// --------------------------------------------------------------------------------------
//  pxAlignment / pxStretchType
// --------------------------------------------------------------------------------------
// These are full blown class types instead of enumerations because wxSizerFlags has an
// implicit conversion from integer (silly design flaw creating more work for me!)
//
struct pxAlignmentType
{
	enum
	{
		Centre,
		Center = Centre,
		Middle,
		Left,
		Right,
		Top,
		Bottom
	};

	int intval;

	wxSizerFlags Apply( wxSizerFlags flags=wxSizerFlags() ) const;
	
	wxSizerFlags operator& ( const wxSizerFlags& _flgs ) const
	{
		return Apply( _flgs );
	}

	wxSizerFlags Border( int dir, int padding ) const
	{
		return Apply().Border( dir, padding );
	}

	wxSizerFlags Proportion( int prop ) const
	{
		return Apply().Proportion( intval );
	}

	operator wxSizerFlags() const
	{
		return Apply();
	}
};

struct pxStretchType
{
	enum
	{
		Shrink,
		Expand,
		Shaped,
		ReserveHidden,
		FixedMinimum
	};
	
	int intval;

	wxSizerFlags Apply( wxSizerFlags flags=wxSizerFlags() ) const;

	wxSizerFlags operator& ( const wxSizerFlags& _flgs ) const
	{
		return Apply( _flgs );
	}

	wxSizerFlags Border( int dir, int padding ) const
	{
		return Apply().Border( dir, padding );
	}

	wxSizerFlags Proportion( int prop ) const
	{
		return Apply().Proportion( intval );
	}

	operator wxSizerFlags() const
	{
		return Apply();
	}
};

class pxProportion
{
	int intval;

	pxProportion( int prop )
	{
		intval = prop;
	}

	wxSizerFlags Apply( wxSizerFlags flags=wxSizerFlags() ) const;

	wxSizerFlags operator& ( const wxSizerFlags& _flgs ) const
	{
		return Apply( _flgs );
	}

	operator wxSizerFlags() const
	{
		return Apply();
	}
};

extern const pxAlignmentType
	pxCentre,	// Horizontal centered alignment
	pxCenter,
	pxMiddle,	// vertical centered alignment

	pxAlignLeft,
	pxAlignRight,
	pxAlignTop,
	pxAlignBottom;

extern const pxStretchType
	pxShrink,
	pxExpand,
	pxShaped,
	pxReserveHidden,
	pxFixedMinimum;

// --------------------------------------------------------------------------------------
//  pxWindowAndFlags
// --------------------------------------------------------------------------------------
// This struct is a go-between for combining windows and sizer flags in "neat" fashion.
// To create the struct, use the | operator, like so:
//
//   myPanel | wxSizerFlags().Expand()
//
// Implementation Note:  This struct is a template as it allows us to use a special
// version of the += operator that retains the type information of the window, in case
// the window implements its own += overloads (one example is pxStaticText).  Without the
// template, the type of the window would only be known as "wxWindow" when it's added to the
// sizer, and would thus fail to invoke the correct operator overload.
//
template< typename WinType >
struct pxWindowAndFlags
{
	WinType*		window;
	wxSizerFlags	flags;
};


extern wxSizerFlags operator& ( const wxSizerFlags& _flgs, const wxSizerFlags& _flgs2 );

template< typename WinType >
pxWindowAndFlags<WinType> operator | ( WinType* _win, const wxSizerFlags& _flgs )
{
	pxWindowAndFlags<WinType> result = { _win, _flgs };
	return result;
}

template< typename WinType >
pxWindowAndFlags<WinType> operator | ( WinType& _win, const wxSizerFlags& _flgs )
{
	pxWindowAndFlags<WinType> result = { &_win, _flgs };
	return result;
}

// --------------------------------------------------------------------------------------
//  wxSizer Operator +=  .. a wxSizer.Add() Substitute
// --------------------------------------------------------------------------------------
// This set of operators is the *recommended* method for adding windows to sizers, not just
// because it's a lot prettier but also because it allows controls like pxStaticText to over-
// ride default sizerflags behavior.
//
// += operator works on either sizers, wxDialogs or wxPanels.  In the latter case, the window
// is added to the dialog/panel's toplevel sizer (wxPanel.GetSizer() is used).  If the panel
// has no sizer set via SetSizer(), an assertion is generated.
//

extern void operator+=( wxSizer& target, wxWindow* src );
extern void operator+=( wxSizer& target, wxSizer* src );
extern void operator+=( wxSizer& target, wxWindow& src );
extern void operator+=( wxSizer& target, wxSizer& src );

extern void operator+=( wxSizer* target, wxWindow& src );
extern void operator+=( wxSizer* target, wxSizer& src );

extern void operator+=( wxSizer& target, int spacer );
extern void operator+=( wxWindow& target, int spacer );

// ----------------------------------------------------------------------------
// Important: This template is needed in order to retain window type information and
// invoke the proper overloaded version of += (which is used by pxStaticText and other
// classes to perform special actions when added to sizers).
template< typename WinType >
void operator+=( wxWindow& target, WinType* src )
{
	if( !pxAssert( target.GetSizer() != NULL ) ) return;
	*target.GetSizer() += src;
}

template< typename WinType >
void operator+=( wxWindow& target, WinType& src )
{
	if( !pxAssert( target.GetSizer() != NULL ) ) return;
	*target.GetSizer() += src;
}

template< typename WinType >
void operator+=( wxWindow& target, const pxWindowAndFlags<WinType>& src )
{
	if( !pxAssert( target.GetSizer() != NULL ) ) return;
	*target.GetSizer() += src;
}

template< typename WinType >
void operator+=( wxSizer& target, const pxWindowAndFlags<WinType>& src )
{
	target.Add( src.window, src.flags );
}

// ----------------------------------------------------------------------------
// Pointer Versions!  (note that C++ requires one of the two operator params be a
// "poper" object type (non-pointer), so that's why some of these are missing.

template< typename WinType >
void operator+=( wxWindow* target, WinType& src )
{
	if( !pxAssert( target != NULL ) ) return;
	if( !pxAssert( target->GetSizer() != NULL ) ) return;
	*target->GetSizer() += src;
}

template< typename WinType >
void operator+=( wxWindow* target, const pxWindowAndFlags<WinType>& src )
{
	if( !pxAssert( target != NULL ) ) return;
	if( !pxAssert( target->GetSizer() != NULL ) ) return;
	*target->GetSizer() += src;
}

template< typename WinType >
void operator+=( wxSizer* target, const pxWindowAndFlags<WinType>& src )
{
	if( !pxAssert( target != NULL ) ) return;
	target->Add( src.window, src.flags );
}

// ----------------------------------------------------------------------------
// wxGuiTools.h
//
// This file is meant to contain utility classes for users of the wxWidgets library.
// All classes in this file are dependent on wxBase and wxCore libraries!  Meaning
// you will have to use wxCore header files and link against wxCore (GUI) to build
// them.  For tools which require only wxBase, see wxBaseTools.h
// ----------------------------------------------------------------------------

namespace pxSizerFlags
{
	static const int StdPadding = 6;
	
	extern wxSizerFlags StdSpace();
	extern wxSizerFlags StdCenter();
	extern wxSizerFlags StdExpand();
	extern wxSizerFlags TopLevelBox();
	extern wxSizerFlags SubGroup();
	extern wxSizerFlags StdButton();
	extern wxSizerFlags Checkbox();
};

// --------------------------------------------------------------------------------------
//  wxDialogWithHelpers
// --------------------------------------------------------------------------------------
class wxDialogWithHelpers : public wxDialog
{
	DECLARE_DYNAMIC_CLASS_NO_COPY(wxDialogWithHelpers)

protected:
	bool				m_hasContextHelp;
	int					m_idealWidth;
	wxBoxSizer*			m_extraButtonSizer;

public:
	wxDialogWithHelpers();
	wxDialogWithHelpers(wxWindow* parent, const wxString& title, bool hasContextHelp=false, bool resizable=false );
	wxDialogWithHelpers(wxWindow* parent, const wxString& title, wxOrientation orient);
	virtual ~wxDialogWithHelpers() throw();

    void Init();
	void AddOkCancel( wxSizer& sizer, bool hasApply=false );

	virtual void SmartCenterFit();
	virtual int ShowModal();
	virtual bool Show( bool show=true );

	virtual pxStaticText*		Text( const wxString& label );
	virtual pxStaticHeading*	Heading( const wxString& label );

	virtual wxDialogWithHelpers& SetIdealWidth( int newWidth ) { m_idealWidth = newWidth; return *this; }

	int GetIdealWidth() const { return m_idealWidth; }
	bool HasIdealWidth() const { return m_idealWidth != wxDefaultCoord; }

protected:
	void OnActivate(wxActivateEvent& evt);
	void OnOkCancel(wxCommandEvent& evt);
	void OnCloseWindow(wxCloseEvent& event);
};

// --------------------------------------------------------------------------------------
//  wxPanelWithHelpers
// --------------------------------------------------------------------------------------
// Overview of Helpers provided by this class:
//  * Simpler constructors that have wxID, position, and size parameters removed (We never
//    use them in pcsx2)
//
//  * Automatic 'primary box sizer' creation, assigned via SetSizer() -- use GetSizer()
//    to retrieve it, or use the "*this += window;" syntax to add windows directly to it.
//
//  * Built-in support for StaticBoxes (aka groupboxes).  Create one at construction with
//    a wxString label, or add one "after the fact" using AddFrame.
//
//  * Propagates IdealWidth settings from parenting wxPanelWithHelpers classes, and auto-
//    matically adjusts the width based on the sizer type (groupsizers get truncated to
//    account for borders).
//
class wxPanelWithHelpers : public wxPanel
{
	DECLARE_DYNAMIC_CLASS_NO_COPY(wxPanelWithHelpers)

protected:
	int		m_idealWidth;

public:
	wxPanelWithHelpers( wxWindow* parent, wxOrientation orient, const wxString& staticBoxLabel );
	wxPanelWithHelpers( wxWindow* parent, wxOrientation orient );
	wxPanelWithHelpers( wxWindow* parent, const wxPoint& pos, const wxSize& size=wxDefaultSize );
	explicit wxPanelWithHelpers( wxWindow* parent=NULL );
	
	wxPanelWithHelpers* AddFrame( const wxString& label, wxOrientation orient=wxVERTICAL );

	pxStaticText*		Text( const wxString& label );
	pxStaticHeading*	Heading( const wxString& label );

	// TODO : Propagate to children?
	wxPanelWithHelpers& SetIdealWidth( int width ) { m_idealWidth = width;  return *this; }
	int GetIdealWidth() const { return m_idealWidth; }
	bool HasIdealWidth() const { return m_idealWidth != wxDefaultCoord; }
	
protected:
	void Init();
};


// --------------------------------------------------------------------------------------
//  pxTextWrapperBase
// --------------------------------------------------------------------------------------
// this class is used to wrap the text on word boundary: wrapping is done by calling
// OnStartLine() and OnOutputLine() functions.  This class by itself can be used as a
// line counting tool, but produces no formatted text output.
//
// [class "borrowed" from wxWidgets private code, made public, and renamed to avoid possible
//  conflicts with future editions of wxWidgets which might make it public.  Why this isn't
//  publicly available already in wxBase I'll never know-- air]
//
class pxTextWrapperBase
{
protected:
	bool	m_eol;
	int		m_linecount;

public:
	virtual ~pxTextWrapperBase() throw() { }

    pxTextWrapperBase()
	{
		m_eol = false;
		m_linecount = 0;
	}

    // win is used for getting the font, text is the text to wrap, width is the
    // max line width or -1 to disable wrapping
    pxTextWrapperBase& Wrap( const wxWindow& win, const wxString& text, int widthMax );

	int GetLineCount() const
	{
		return m_linecount;
	}

protected:
    // line may be empty
    virtual void OnOutputLine(const wxString& line) { }

    // called at the start of every new line (except the very first one)
    virtual void OnNewLine() { }

    void DoOutputLine(const wxString& line);
    bool IsStartOfNewLine();
};

// --------------------------------------------------------------------------------------
//  pxTextWrapper
// --------------------------------------------------------------------------------------
// This class extends pxTextWrapperBase and adds the ability to retrieve the formatted
// result of word wrapping.
//
class pxTextWrapper : public pxTextWrapperBase
{
	typedef pxTextWrapperBase _parent;
	
protected:
	wxString m_text;

public:
	pxTextWrapper() : pxTextWrapperBase() { }
	virtual ~pxTextWrapper() throw() { }

    const wxString& GetResult() const
    {
		return m_text;
    }

	pxTextWrapper& Wrap( const wxWindow& win, const wxString& text, int widthMax );

protected:
    virtual void OnOutputLine(const wxString& line);
    virtual void OnNewLine();
};

// --------------------------------------------------------------------------------------
//  MoreStockCursors
// --------------------------------------------------------------------------------------
// Because (inexplicably) the ArrowWait cursor isn't in wxWidgets stock list.
//
class MoreStockCursors
{
protected:
	ScopedPtr<wxCursor>	m_arrowWait;

public:
	MoreStockCursors() { }
	virtual ~MoreStockCursors() throw() { }
	const wxCursor& GetArrowWait();
};

enum BusyCursorType
{
	Cursor_NotBusy,
	Cursor_KindaBusy,
	Cursor_ReallyBusy,
};

extern MoreStockCursors StockCursors;

// --------------------------------------------------------------------------------------
//  ScopedBusyCursor
// --------------------------------------------------------------------------------------
// ... because wxWidgets wxBusyCursor doesn't really do proper nesting (doesn't let me 
// override a partially-busy cursor with a really busy one)

class ScopedBusyCursor
{
protected:
	static std::stack<BusyCursorType>	m_cursorStack;
	static BusyCursorType				m_defBusyType;

public:
	ScopedBusyCursor( BusyCursorType busytype );
	virtual ~ScopedBusyCursor() throw();
	
	static void SetDefault( BusyCursorType busytype );
	static void SetManualBusyCursor( BusyCursorType busytype );
};

// --------------------------------------------------------------------------------------
//  pxFitToDigits
// --------------------------------------------------------------------------------------
// Fits a given text or spinner control to the number of digits requested, since by default
// they're usually way over-sized.

extern void pxFitToDigits( wxWindow* win, int digits );
extern void pxFitToDigits( wxSpinCtrl* win, int digits );
extern wxTextCtrl* CreateNumericalTextCtrl( wxWindow* parent, int digits );


//////////////////////////////////////////////////////////////////////////////////////////////

extern bool pxDialogExists( const wxString& name );
extern bool pxIsValidWindowPosition( const wxWindow& window, const wxPoint& windowPos );
extern wxRect wxGetDisplayArea();

extern wxString pxFormatToolTipText( wxWindow* wind, const wxString& src );
extern void pxSetToolTip( wxWindow* wind, const wxString& src );
extern void pxSetToolTip( wxWindow& wind, const wxString& src );
extern wxFont pxGetFixedFont( int ptsize=8, int weight=wxNORMAL );


#endif
