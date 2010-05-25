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

#include "PrecompiledHeader.h"
#include "ConfigurationPanels.h"
#include "MemoryCardPanels.h"

#include "Dialogs/ConfigurationDialog.h"

#include <wx/filepicker.h>
#include <wx/ffile.h>
#include <wx/dir.h>

using namespace pxSizerFlags;
using namespace Panels;


// =====================================================================================================
//  MemoryCardInfoPanel  (implementations)
// =====================================================================================================
MemoryCardInfoPanel::MemoryCardInfoPanel( wxWindow* parent, uint slot )
	: BaseApplicableConfigPanel( parent, wxVERTICAL ) //, wxEmptyString )
{
	m_slot = slot;

	SetMinSize( wxSize(128, 48) );

	Connect( wxEVT_PAINT, wxPaintEventHandler(MemoryCardInfoPanel::paintEvent) );
	
	// [TODO] Add Unmount button.
}


void MemoryCardInfoPanel::paintEvent(wxPaintEvent & evt)
{
	wxPaintDC dc( this );
	pxWindowTextWriter writer( dc );

	writer.Bold();

	// Create DC and plot some text (and images!)
	writer.WriteLn( m_DisplayName );

	//dc.DrawCircle( dc.GetSize().GetWidth()/2, 24, dc.GetSize().GetWidth()/4 );

	if( !m_ErrorMessage.IsEmpty() )
	{
		writer.WriteLn();
		writer.WriteLn( m_ErrorMessage );
	}
	else if( m_cardInfo )
	{
		writer.Normal();

		writer.WriteLn( wxsFormat( L"%d MB (%s)",
			m_cardInfo->SizeInMB,
			m_cardInfo->IsFormatted ? _("Formatted") : _("Unformatted") )
		);
	}
}

void MemoryCardInfoPanel::Eject()
{
	m_cardInfo = NULL;
	Refresh();
}

void MemoryCardInfoPanel::Apply()
{
	if( m_cardInfo && m_cardInfo->Filename.GetFullName().IsEmpty() ) m_cardInfo = NULL;
	
	if( m_cardInfo )
	{
		wxFileName absfile( Path::Combine( g_Conf->Folders.MemoryCards, m_cardInfo->Filename ) );

		// The following checks should be theoretically unreachable, unless the user's
		// filesystem is changed form under our nose.  A little validation goes a
		// long way. :p
		
		if( absfile.IsDir() )
		{
			Eject();
			throw Exception::CannotApplySettings( this, 
				// Diagnostic
				wxsFormat( L"Memorycard in slot %u conflicts with an existing directory.", m_slot ),
				// Translated
				wxsFormat(
					_("Cannot use or create the memorycard in slot %u: the filename conflicts with an existing directory."),
					m_slot
				)
			);
		}

		if( !absfile.FileExists() )
		{
			Eject();
			throw Exception::CannotApplySettings( this, 
				// Diagnostic
				wxsFormat( L"Memorycard in slot %u is no longer valid.", m_slot ),
				// Translated
				wxsFormat(
					_("The configured memorycard in slot %u no longer exists.  Please create a new memory card, or leave the slot unmounted."),
					m_slot
				)
			);
		}

		g_Conf->Mcd[m_slot].Filename = m_cardInfo->Filename;
		g_Conf->Mcd[m_slot].Enabled = true;
	}
	else
	{
		// Card is either disabled or in an error state.

		g_Conf->Mcd[m_slot].Enabled = false;
		g_Conf->Mcd[m_slot].Filename.Clear();
	}
}

void MemoryCardInfoPanel::AppStatusEvent_OnSettingsApplied()
{
	m_cardInfo = NULL;

	// Collect Info and Format Strings

	wxString fname( g_Conf->Mcd[m_slot].Filename.GetFullPath() );
	if( fname.IsEmpty() )
	{
		m_DisplayName = _("No Card (empty)");
		m_cardInfo = NULL;
	}
	else
	{
		wxFileName absfile( Path::Combine( g_Conf->Folders.MemoryCards, fname ) );
		wxFileName relfile( fname );

		if( !m_cardInfo )
		{
			m_cardInfo = new McdListItem();
			if( !EnumerateMemoryCard( *m_cardInfo, absfile.GetFullPath() ) )
			{
				m_ErrorMessage = _("Read Error: Card is truncated or corrupted.");
			}
		}

		absfile.Normalize();
		relfile.Normalize();

		m_DisplayName = ( absfile == relfile ) ? relfile.GetFullName() : relfile.GetFullPath();
	}

	Refresh();
}
