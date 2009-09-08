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

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/propdlg.h>

#include "wxHelpers.h"

class wxListbook;

namespace Dialogs
{
	class ConfigurationDialog : public wxDialogWithHelpers
	{
	protected:
		wxListbook&		m_listbook;
		wxArrayString	m_labels;

	public:
		virtual ~ConfigurationDialog();
		ConfigurationDialog(wxWindow* parent, int id=wxID_ANY);

	protected:
		template< typename T >
		void AddPage( const char* label, int iconid );

		void OnOk_Click( wxCommandEvent& evt );
		void OnCancel_Click( wxCommandEvent& evt );
		void OnApply_Click( wxCommandEvent& evt );

		virtual void OnSomethingChanged( wxCommandEvent& evt )
		{
			evt.Skip();
			if( (evt.GetId() != wxID_OK) && (evt.GetId() != wxID_CANCEL) && (evt.GetId() != wxID_APPLY) )
			{
				FindWindow( wxID_APPLY )->Enable();
			}
		}
	};
	
	
	class BiosSelectorDialog : public wxDialogWithHelpers
	{
	protected:

	public:
		virtual ~BiosSelectorDialog() {}
		BiosSelectorDialog(wxWindow* parent );

	protected:
		void OnOk_Click( wxCommandEvent& evt );
		void OnDoubleClicked( wxCommandEvent& evt );
	};
}
