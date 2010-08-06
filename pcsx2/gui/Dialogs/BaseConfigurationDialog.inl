/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include <wx/listbook.h>

template< typename T >
void Dialogs::BaseConfigurationDialog::AddPage( const wxChar* label, int iconid )
{
	const int curidx = m_labels.Add( label );
	m_ApplyState.SetCurrentPage( curidx );
	m_listbook->AddPage( new T( m_listbook ), wxGetTranslation( label ),
		( 0 == GetConfSettingsTabName().CmpNoCase(label) ), iconid );
}
