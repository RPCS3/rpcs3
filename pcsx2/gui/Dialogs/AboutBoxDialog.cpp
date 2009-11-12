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

#include "PrecompiledHeader.h"
#include "App.h"
#include "Dialogs/ModalPopups.h"
#include "wxHelpers.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/Dualshock.h"

#include <wx/mstream.h>
#include <wx/hyperlink.h>

using namespace wxHelpers;

namespace Dialogs
{
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

}

// --------------------------------------------------------------------------------------
//  AboutBoxDialog  Implementation
// --------------------------------------------------------------------------------------

Dialogs::AboutBoxDialog::AboutBoxDialog( wxWindow* parent, int id ):
	wxDialogWithHelpers( parent, id, _("About PCSX2"), false ),
	m_bitmap_dualshock( this, wxID_ANY, wxBitmap( EmbeddedImage<res_Dualshock>().Get() ),
		wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN )
{
	static const wxString LabelAuthors = fromUTF8(
		"Developers"
		"\n\n"
		"v0.9.6+: Arcum42, Refraction, "
		"drk||raziel, cottonvibes, gigaherz, "
		"rama, Jake.Stine, saqib, Tmkk, pseudonym"
		"\n\n"
		"Previous versions: Alexey silinov, Aumatt, "
		"Florin, goldfinger, Linuzappz, loser, "
		"Nachbrenner, shadow, Zerofrog"
		"\n\n"
		"Betatesting: Bositman, ChaosCode, "
		"CKemu, crushtest, GeneralPlot, "
		"Krakatos, Parotaku, Rudy_X"
		"\n\n"
		"Webmasters: CKemu, Falcon4ever"
	);

	static const wxString LabelGreets = fromUTF8(
		"Contributors"
		"\n\n"
		"Hiryu and Sjeep (libcdvd / iso filesystem), nneeve (fpu and vu)"
		"\n\n"
		"Plugin Specialists: ChickenLiver (Lilypad), Efp (efp), "
		"Gabest (Gsdx, Cdvdolio, Xpad),  Zeydlitz (ZZogl)"
		"\n\n"
		"Special thanks to: black_wd, Belmont, BGome, _Demo_, Dreamtime, "
		"F|RES, MrBrown, razorblade, Seta-san, Skarmeth, feal87"
	);

	wxBoxSizer& mainSizer = *new wxBoxSizer( wxVERTICAL );

	AddStaticText( mainSizer, _("PCSX2  -  Playstation 2 Emulator") );

	// This sizer holds text of the authors and a logo!
	wxBoxSizer& AuthLogoSizer = *new wxBoxSizer( wxHORIZONTAL );

	// this sizer holds text of the contributors/testers, and a ps2 image!
	wxBoxSizer& ContribSizer = *new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer& aboutUs = *new wxStaticBoxSizer( wxVERTICAL, this );
	wxStaticBoxSizer& contribs = *new wxStaticBoxSizer( wxVERTICAL, this );

	StaticTextCentered* label_auth   = new StaticTextCentered( this, LabelAuthors );
	StaticTextCentered* label_greets = new StaticTextCentered( this, LabelGreets );

	label_auth->Wrap( 340 );
	label_greets->Wrap( 200 );

	aboutUs.Add( label_auth, SizerFlags::StdExpand() );
	contribs.Add( label_greets, SizerFlags::StdExpand() );

	AuthLogoSizer.Add( &aboutUs );
	AuthLogoSizer.AddSpacer( 7 );
	AuthLogoSizer.Add( &contribs );

	ContribSizer.AddStretchSpacer( 1 );
	ContribSizer.Add( &m_bitmap_dualshock, SizerFlags::StdSpace() );
	ContribSizer.AddStretchSpacer( 1 );

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

	CenterOnScreen();
}
