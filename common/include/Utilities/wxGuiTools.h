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

#pragma once

#include "Dependencies.h"

// ----------------------------------------------------------------------------
// wxGuiTools.h
//
// This file is meant to contain utility classes for users of the wxWidgets library.
// All classes in this file are dependent on wxBase and wxCore libraries!  Meaning
// you will have to use wxCore header files and link against wxCore (GUI) to build
// them.  For tools which require only wxBase, see wxBaseTools.h
// ----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////////////////
// pxTextWrapperBase
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
