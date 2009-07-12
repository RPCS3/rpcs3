/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "App.h"
#include "MainFrame.h"
#include "DebugTools/Debug.h"

#include <wx/file.h>
#include <wx/textfile.h>

// This code was 'borrowed' from wxWidgets and then heavily modified to suite
// our needs.  I would have used some crafty subclassing instead except who
// ever wrote the code of wxWidgets had a peculiar love of the 'private' keyword,
// thus killing any possibility of subclassing in a useful manner.

//////////////////////////////////////////////////////////////////////////////////////////
// pass an uninitialized file object, the function will ask the user for the
// filename and try to open it, returns true on success (file was opened),
// false if file couldn't be opened/created and -1 if the file selection
// dialog was canceled
//
static bool OpenLogFile(wxFile& file, wxString& filename, wxWindow *parent)
{
    filename = wxSaveFileSelector(L"log", L"txt", L"log.txt", parent);
    if ( !filename ) return false; // canceled

    if( wxFile::Exists(filename) )
    {
        bool bAppend = false;
        wxString strMsg;
        strMsg.Printf(L"Append log to file '%s' (choosing [No] will overwrite it)?",
                      filename.c_str());

        switch ( wxMessageBox(strMsg, L"Question", wxICON_QUESTION | wxYES_NO | wxCANCEL) )
		{
			case wxYES:
				bAppend = true;
			break;

			case wxNO:
				bAppend = false;
			break;

			case wxCANCEL:
				return false;

			default:
				wxFAIL_MSG( L"invalid message box return value" );
        }

		return ( bAppend ) ?
			file.Open(filename, wxFile::write_append) :
            file.Create(filename, true /* overwrite */);
    }

	return file.Create(filename);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
ConsoleLogFrame::ConsoleLogFrame(MainEmuFrame *parent, const wxString& title) :
	wxFrame(parent, wxID_ANY, title),
	m_TextCtrl( *new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxHSCROLL | wxTE_READONLY | wxTE_RICH2 ) )
{
	m_TextCtrl.SetBackgroundColour( wxColor( 238, 240, 248 ) ); //wxColor( 48, 48, 64 ) );

    // create menu
    wxMenuBar *pMenuBar = new wxMenuBar;
    wxMenu *pMenu = new wxMenu;
    pMenu->Append(Menu_Save,  L"&Save...", L"Save log contents to file");
    pMenu->Append(Menu_Clear, L"C&lear", L"Clear the log contents");
    pMenu->AppendSeparator();
    pMenu->Append(Menu_Close, L"&Close",L"Close this window");

    pMenuBar->Append(pMenu, L"&Log");
    SetMenuBar(pMenuBar);

    // status bar for menu prompts
    CreateStatusBar();
    ClearColor();

    Connect( Menu_Close, wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( ConsoleLogFrame::OnClose ) );
    Connect( Menu_Save,  wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( ConsoleLogFrame::OnSave ) );
    Connect( Menu_Clear, wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( ConsoleLogFrame::OnClear ) );

    Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler(ConsoleLogFrame::OnCloseWindow) );
	Connect( wxEVT_MOVE, wxMoveEventHandler(ConsoleLogFrame::OnMoveAround) );
	Connect( wxEVT_SIZE, wxSizeEventHandler(ConsoleLogFrame::OnResize) );
}

ConsoleLogFrame::~ConsoleLogFrame() { }

void ConsoleLogFrame::OnMoveAround( wxMoveEvent& evt )
{
	// Docking check!  If the window position is within some amount
	// of the main window, enable docking.

	wxPoint topright( GetParent()->GetRect().GetTopRight() );
	wxRect snapzone( topright - wxSize( 8,8 ), wxSize( 16,16 ) );

	if( snapzone.Contains( GetPosition() ) )
		SetPosition( topright + wxSize( 1,0 ) );

	evt.Skip();
}

void ConsoleLogFrame::OnResize( wxSizeEvent& evt )
{
	g_Conf.ConLogBox.DisplaySize = GetSize();
	evt.Skip();
}

void ConsoleLogFrame::DoClose()
{
    // instead of closing just hide the window to be able to Show() it later
    Show(false);
    if( GetParent() != NULL )
		wxStaticCast( GetParent(), MainEmuFrame )->OnLogBoxHidden();
}

void ConsoleLogFrame::OnClose(wxMenuEvent& WXUNUSED(event))		{ DoClose(); }
void ConsoleLogFrame::OnCloseWindow(wxCloseEvent& WXUNUSED(event))	{ DoClose(); }

void ConsoleLogFrame::OnSave(wxMenuEvent& WXUNUSED(event))
{
    wxString filename;
    wxFile file;
    bool rc = OpenLogFile( file, filename, this );
    if ( !rc )
    {
        // canceled
        return;
    }

    // retrieve text and save it
    // -------------------------
    int nLines = m_TextCtrl.GetNumberOfLines();
    for ( int nLine = 0; nLine < nLines; nLine++ )
    {
		if( !file.Write(m_TextCtrl.GetLineText(nLine) + wxTextFile::GetEOL()) )
		{
			wxLogError( L"Can't save log contents to file." );
			return;
		}
	}

	wxLogStatus(this, L"Log saved to the file '%s'.", filename.c_str());
}

void ConsoleLogFrame::OnClear(wxMenuEvent& WXUNUSED(event))
{
    m_TextCtrl.Clear();
}

static const wxTextAttr tbl_color_codes[] =
{
	// Standard R, G, B format:
	wxTextAttr( wxColor(   0,  0,  0 ) ),
	wxTextAttr( wxColor( 128,  0,  0 ) ),
	wxTextAttr( wxColor(   0,128,  0 ) ),
	wxTextAttr( wxColor( 180,180,  0 ) ),
	wxTextAttr( wxColor(   0,  0,128 ) ),
	wxTextAttr( wxColor(   0,160,160 ) ),
	wxTextAttr( wxColor( 160,160,160 ) )
};

static const wxTextAttr color_default( wxColor( 0, 0, 0 ) );

// Note: SetColor currently does not work on Win32, but I suspect it *should* work when
// we enable unicode compilation.  (I really hope!)
void ConsoleLogFrame::SetColor( Console::Colors color )
{
	m_TextCtrl.SetDefaultStyle( tbl_color_codes[(int)color] );
}

void ConsoleLogFrame::ClearColor()
{
	m_TextCtrl.SetDefaultStyle( color_default );
}

void ConsoleLogFrame::Newline()
{
	Write( L"\n");
}

void ConsoleLogFrame::Write( const wxChar* text )
{
	// remove selection (WriteText is in fact ReplaceSelection)
#ifdef __WXMSW__
	wxTextPos nLen = m_TextCtrl.GetLastPosition();
	m_TextCtrl.SetSelection(nLen, nLen);
#endif

	m_TextCtrl.AppendText( text );
}

void ConsoleLogFrame::Write( const char* text )
{
	// remove selection (WriteText is in fact ReplaceSelection)
#ifdef __WXMSW__
	wxTextPos nLen = m_TextCtrl.GetLastPosition();
	m_TextCtrl.SetSelection(nLen, nLen);
#endif

	m_TextCtrl.AppendText( wxString::FromAscii(text) );
}

namespace Console
{
	// ------------------------------------------------------------------------
	void __fastcall SetTitle( const wxString& title )
	{
		if( ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame() )
			FrameHandle->SetTitle( title );
	}

	// ------------------------------------------------------------------------
	void __fastcall SetColor( Colors color )
	{
		if( ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame() )
			FrameHandle->SetColor( color );
	}

	// ------------------------------------------------------------------------
	void ClearColor()
	{
		if( ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame() )
			FrameHandle->ClearColor();
	}

	// ------------------------------------------------------------------------
	bool Newline()
	{
		if( ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame() )
			FrameHandle->Newline();

		if( emuLog != NULL )
			fputs( "\n", emuLog );
		return false;
	}

	// ------------------------------------------------------------------------
	bool __fastcall Write( const char* fmt )
	{
		if( ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame() )
			FrameHandle->Write( fmt );

		if( emuLog != NULL )
			fputs( fmt, emuLog );
		return false;
	}

	// ------------------------------------------------------------------------
	bool __fastcall Write( const wxString& fmt )
	{
		if( ConsoleLogFrame* FrameHandle = wxGetApp().GetConsoleFrame() )
			FrameHandle->Write( fmt );

		if( emuLog != NULL )
			fputs( fmt.ToAscii().data(), emuLog );
		return false;
	}
}

namespace Msgbox
{
	bool Alert( const wxString& text )
	{
		wxMessageBox( text, L"Pcsx2 Message", wxOK, wxGetApp().GetTopWindow() );
		return false;
	}

	bool OkCancel( const wxString& text )
	{
		int result = wxMessageBox( text, L"Pcsx2 Message", wxOK | wxCANCEL, wxGetApp().GetTopWindow() );
		return result == wxOK;
	}
}
