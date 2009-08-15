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

wxString GetEnglish( const char* msg )
{
	return wxString::FromAscii(msg);
}

wxString GetTranslation( const char* msg )
{
	return wxGetTranslation( wxString::FromAscii(msg).c_str() );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
namespace Exception
{
	// ------------------------------------------------------------------------
	BaseException::~BaseException() throw() {}

	BaseException::BaseException( const wxString& msg_eng, const wxString& msg_xlt ) :
		m_message_eng( msg_eng ),
		m_message( msg_xlt ),
		m_stacktrace( wxEmptyString )		// unsupported yet
	{
		// Linux/GCC exception handling is still suspect (this is likely to do with GCC more
		// than linux), and fails to propagate exceptions up the stack from EErec code.  This
		// could likely be because of the EErec using EBP.  So to ensure the user at least
		// gets a log of the error, we output to console here in the constructor.

#ifdef __LINUX__
        //wxLogError( msg_eng.c_str() );
        Console::Error( msg_eng );
#endif
	}

	// given message is assumed to be a translation key, and will be stored in translated
	// and untranslated forms.
	BaseException::BaseException( const char* msg_eng ) :
		m_message_eng( GetEnglish( msg_eng ) ),
		m_message( GetTranslation( msg_eng ) ),
		m_stacktrace( wxEmptyString )		// unsupported yet
	{
#ifdef __LINUX__
        //wxLogError( m_message_eng.c_str() );
        Console::Error( msg_eng );
#endif
	}

	wxString BaseException::LogMessage() const
	{
		return m_message_eng + L"\n\n" + m_stacktrace;
	}

	// ------------------------------------------------------------------------
	wxString Stream::LogMessage() const
	{
		return wxsFormat(
			L"Stream exception: %s\n\tObject name: %s",
			m_message_eng.c_str(), StreamName.c_str()
		) + m_stacktrace;
	}

	wxString Stream::DisplayMessage() const
	{
		return m_message + L"\n\n" + StreamName;
	}

	// ------------------------------------------------------------------------
	wxString PluginFailure::LogMessage() const
	{
		return wxsFormat(
			L"%s plugin has encountered an error.\n\n",
			plugin_name.c_str()
		) + m_stacktrace;
	}

	wxString PluginFailure::DisplayMessage() const
	{
		return wxsFormat( m_message, plugin_name.c_str() );
	}

	// ------------------------------------------------------------------------
	wxString FreezePluginFailure::LogMessage() const
	{
		return wxsFormat(
			L"%s plugin returned an error while %s the state.\n\n",
			plugin_name.c_str(),
			freeze_action.c_str()
		) + m_stacktrace;
	}

	wxString FreezePluginFailure::DisplayMessage() const
	{
		return m_message;
	}

	// ------------------------------------------------------------------------
	wxString UnsupportedStateVersion::LogMessage() const
	{
		// Note: no stacktrace needed for this one...
		return wxsFormat( L"Unknown or unsupported savestate version: 0x%x", Version );
	}

	wxString UnsupportedStateVersion::DisplayMessage() const
	{
		// m_message contains a recoverable savestate error which is helpful to the user.
		return wxsFormat(
			m_message + L"\n\n" +
			wxsFormat( L"Unknown savestate version: 0x%x", Version )
		);
	}

	// ------------------------------------------------------------------------
	wxString StateCrcMismatch::LogMessage() const
	{
		// Note: no stacktrace needed for this one...
		return wxsFormat(
			L"Game/CDVD does not match the savestate CRC.\n"
			L"\tCdvd CRC: 0x%X\n\tGame CRC: 0x%X\n",
			Crc_Savestate, Crc_Cdvd
		);
	}

	wxString StateCrcMismatch::DisplayMessage() const
	{
		return wxsFormat(
			m_message + L"\n\n" +
			wxsFormat( 
				L"Savestate game/crc mismatch. Cdvd CRC: 0x%X Game CRC: 0x%X\n",
				Crc_Savestate, Crc_Cdvd
			)
		);
	}

	// ------------------------------------------------------------------------
	wxString IndexBoundsFault::LogMessage() const
	{
		return L"Index out of bounds on SafeArray: " + ArrayName +
			wxsFormat( L"(index=%d, size=%d)", BadIndex, ArrayLength );
	}

	wxString IndexBoundsFault::DisplayMessage() const
	{
		return m_message;
	}
}
