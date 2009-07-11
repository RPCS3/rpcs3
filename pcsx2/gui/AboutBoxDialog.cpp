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
#include "Misc.h"
#include "App.h"
#include "Dialogs/ModalPopups.h"
#include "wxHelpers.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/ps2_silver.h"

#include <wx/mstream.h>
#include <wx/hyperlink.h>

using namespace wxHelpers;

namespace Dialogs {

//////////////////////////////////////////////////////////////////////////////////////////
// Helper class for creating wxStaticText labels which are aligned to center.
// (automates the passing of wxDefaultSize and wxDefaultPosition)
//
class StaticTextCentered : public wxStaticText
{
public:
	StaticTextCentered( wxWindow* parent, const wxString& text, int id=wxID_ANY ) :
		wxStaticText( parent, id, text, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER )
	{
	}
};


AboutBoxDialog::AboutBoxDialog( wxWindow* parent, int id ):
	wxDialog( parent, id, _("About Pcsx2"), parent->GetPosition()-wxSize( 32, 32 ) ),
	m_bitmap_logo( this, wxID_ANY, wxGetApp().GetLogoBitmap(),
		wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN ),
	m_bitmap_ps2system( this, wxID_ANY, wxBitmap( EmbeddedImage<png_ps2_silver>().Get() ),
		wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN )
{
	static const wxString LabelAuthors = wxString::FromAscii(
		"PCSX2, a PS2 emulator\n\n"
		"Active Devs: Arcum42, Refraction,"
		"drk||raziel, cottonvibes, gigaherz,"
		"rama, Jake.Stine, saqib, Tmkk"
		"\n\n"
		"Inactive devs: Alexey silinov, Aumatt,"
		"Florin, goldfinger, Linuzappz, loser,"
		"Nachbrenner, shadow, Zerofrog"
		"\n\n"
		"Betatesting: Bositman, ChaosCode,"
		"CKemu, crushtest, GeneralPlot,"
		"Krakatos, Parotaku, Rudy_X"
		"\n\n"
		"Webmasters: CKemu, Falcon4ever"
	);

	static const wxString LabelGreets = wxString::FromAscii(
		"Contributors: Hiryu and Sjeep for libcvd (the iso parsing and\n"
		"filesystem driver code), nneeve, pseudonym\n"
		"\n"
		"Plugin Specialists: ChickenLiver (Lilypad), Efp (efp),\n"
		"Gabest (Gsdx, Cdvdolio, Xpad)\n"
		"\n"
		"Special thanks to: black_wd, Belmont, BGome, _Demo_, Dreamtime,\n"
		"F|RES, MrBrown, razorblade, Seta-san, Skarmeth"
	);

	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );

	// This sizer holds text of the authors and a logo!
	wxBoxSizer& AuthLogoSizer = *new wxBoxSizer( wxHORIZONTAL );

	// this sizer holds text of the contributors/testers, and a ps2 image!
	wxBoxSizer& ContribSizer = *new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer& aboutUs = *new wxStaticBoxSizer( wxVERTICAL, this );
	wxStaticBoxSizer& contribs = *new wxStaticBoxSizer( wxVERTICAL, this );

	StaticTextCentered* label_auth   = new StaticTextCentered( this, LabelAuthors );
	StaticTextCentered* label_greets = new StaticTextCentered( this, LabelGreets );

	label_auth->Wrap( m_bitmap_logo.GetSize().GetWidth() / 2 );
	label_greets->Wrap( (m_bitmap_logo.GetSize().GetWidth() * 4) / 3 );

	aboutUs.Add( label_auth, SizerFlags::StdSpace() );
	contribs.Add( label_greets, SizerFlags::StdExpand() );

	AuthLogoSizer.Add( &aboutUs );
	AuthLogoSizer.AddSpacer( 7 );
	AuthLogoSizer.Add( &m_bitmap_logo, wxSizerFlags().Border( wxALL, 4 ) );

	ContribSizer.AddStretchSpacer( 1 );
	ContribSizer.Add( &m_bitmap_ps2system, SizerFlags::StdSpace() );
	ContribSizer.AddStretchSpacer( 1 );
	ContribSizer.Add( &contribs, wxSizerFlags(7).HorzBorder().Expand() );

	mainSizer.Add( &AuthLogoSizer, SizerFlags::StdSpace() );

	mainSizer.Add( new wxHyperlinkCtrl(
		this, wxID_ANY, L"Pcsx2 Official Website and Forums" , L"http://www.pcsx2.net" ),
		wxSizerFlags(1).Center().Border( wxALL, 3 ) );
	mainSizer.Add( new wxHyperlinkCtrl(
		this, wxID_ANY, L"Pcsx2 Official Svn Repository at Googlecode" , L"http://code.google.com/p/pcsx2" ),
		wxSizerFlags(1).Center().Border( wxALL, 3 ) );

	mainSizer.Add( &ContribSizer, SizerFlags::StdExpand() );

	mainSizer.Add( new wxButton( this, wxID_OK, L"I've seen enough"), SizerFlags::StdCenter() );
	SetSizerAndFit( &mainSizer );
}

}	// end namespace Dialogs
