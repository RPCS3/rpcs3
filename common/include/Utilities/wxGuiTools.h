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

// ----------------------------------------------------------------------------
// wxGuiTools.h
//
// This file is meant to contain utility classes for users of the wxWidgets library.
// All classes in this file are dependent on wxBase and wxCore libraries!  Meaning
// you will have to use wxCore header files and link against wxCore (GUI) to build
// them.  For tools which require only wxBase, see wxBaseTools.h
// ----------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------
//  pxTextWrapper
// --------------------------------------------------------------------------------------
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


extern bool pxIsValidWindowPosition( const wxWindow& window, const wxPoint& windowPos );
extern wxRect wxGetDisplayArea();

#endif
