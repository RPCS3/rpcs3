#pragma once

#include <wx/wx.h>
#include <wx/filepicker.h>

namespace wxHelpers
{
	extern wxCheckBox&		AddCheckBoxTo( wxWindow* parent, wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString, int wrapLen=wxDefaultCoord );
	extern wxRadioButton&	AddRadioButtonTo( wxWindow* parent, wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString, int wrapLen=wxDefaultCoord, bool isFirst = false );
	extern wxStaticText&	AddStaticTextTo(wxWindow* parent, wxSizer& sizer, const wxString& label, int alignFlags=wxALIGN_CENTRE, int wrapLen=wxDefaultCoord );

	namespace SizerFlags
	{
		extern wxSizerFlags StdSpace();
		extern wxSizerFlags StdCenter();
		extern wxSizerFlags StdExpand();
		extern wxSizerFlags TopLevelBox();
		extern wxSizerFlags SubGroup();
		extern wxSizerFlags StdButton();
		extern wxSizerFlags Checkbox();
	};
}

//////////////////////////////////////////////////////////////////////////////////////////
// pxTextWrapper
// this class is used to wrap the text on word boundary: wrapping is done by calling
// OnStartLine() and OnOutputLine() functions.  This class by itself can be used as a
// line counting tool, but produces no formatted text output.
//
// [class "borrowed" from wxWidgets private code, and renamed to avoid possible conflicts
//  with future editions of wxWidgets which might make it public.  Why this isn't publicly
//  available already in wxBase I'll never know-- air]
//
class pxTextWrapperBase
{
protected:
	bool m_eol;
	int m_linecount;

public:
	virtual ~pxTextWrapperBase() { }

    pxTextWrapperBase() :
		m_eol( false )
	,	m_linecount( 0 )
	{
	}

    // win is used for getting the font, text is the text to wrap, width is the
    // max line width or -1 to disable wrapping
    void Wrap( const wxWindow *win, const wxString& text, int widthMax );

	int GetLineCount() const
	{
		return m_linecount;
	}

protected:
    // line may be empty
    virtual void OnOutputLine(const wxString& line) { }

    // called at the start of every new line (except the very first one)
    virtual void OnNewLine() { }

    void DoOutputLine(const wxString& line)
    {
        OnOutputLine(line);
		m_linecount++;
        m_eol = true;
    }

    // this function is a destructive inspector: when it returns true it also
    // resets the flag to false so calling it again wouldn't return true any
    // more
    bool IsStartOfNewLine()
    {
        if ( !m_eol )
            return false;

        m_eol = false;
        return true;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////
// pxTextWrapper
// This class extends pxTextWrapperBase and adds the ability to retrieve the formatted
// result of word wrapping.
//
class pxTextWrapper : public pxTextWrapperBase
{
protected:
	wxString m_text;

public:
	pxTextWrapper() : pxTextWrapperBase()
	,	m_text()
	{
	}

    const wxString& GetResult() const
    {
		return m_text;
    }

protected:
    virtual void OnOutputLine(const wxString& line)
    {
        m_text += line;
    }

    virtual void OnNewLine()
    {
        m_text += L'\n';
    }
};


//////////////////////////////////////////////////////////////////////////////////////////
//
class wxDialogWithHelpers : public wxDialog
{
protected:
	bool m_hasContextHelp;

public:
	wxDialogWithHelpers(wxWindow* parent, int id, const wxString& title, bool hasContextHelp, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize );

	wxCheckBox&		AddCheckBox( wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString );
	wxStaticText&	AddStaticText(wxSizer& sizer, const wxString& label, int alignFlags=wxALIGN_CENTRE, int size=wxDefaultCoord );
    void AddOkCancel( wxSizer& sizer, bool hasApply=false );

protected:
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class wxPanelWithHelpers : public wxPanel
{
protected:
	const int m_idealWidth;
	bool m_StartNewRadioGroup;

public:
	wxPanelWithHelpers( wxWindow* parent, int idealWidth=wxDefaultCoord );
	wxPanelWithHelpers( wxWindow* parent, const wxPoint& pos, const wxSize& size=wxDefaultSize );

	wxCheckBox&		AddCheckBox( wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString );
	wxRadioButton&	AddRadioButton( wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString );
	wxStaticText&	AddStaticText(wxSizer& sizer, const wxString& label, int alignFlags=wxALIGN_CENTRE, int size=wxDefaultCoord );

	int GetIdealWidth() const { return m_idealWidth; }
	bool HasIdealWidth() const { return m_idealWidth != wxDefaultCoord; }

protected:	
	void StartRadioGroup()
	{
		m_StartNewRadioGroup = true;
	}
};
