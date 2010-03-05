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
#include "AppCommon.h"

#include "Dialogs/ModalPopups.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/Dualshock.h"

#include <wx/mstream.h>
#include <wx/hyperlink.h>

using namespace pxSizerFlags;

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

Dialogs::AboutBoxDialog::AboutBoxDialog( wxWindow* parent )
	: wxDialogWithHelpers( parent, _("About PCSX2"), wxVERTICAL )
	, m_bitmap_dualshock( this, wxID_ANY, wxBitmap( EmbeddedImage<res_Dualshock>().Get() ),
		wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN
	)
{
	SetName( GetNameStatic() );

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

	aboutUs		+= label_auth		| StdExpand();
	contribs	+= label_greets		| StdExpand();

	AuthLogoSizer	+= aboutUs;
	AuthLogoSizer	+= 7;
	AuthLogoSizer	+= contribs;

	ContribSizer	+= pxStretchSpacer( 1 );
	ContribSizer	+= m_bitmap_dualshock	| StdSpace();
	ContribSizer	+= pxStretchSpacer( 1 );

	// Main (top-level) layout 

	*this	+= Text(_("PCSX2  -  Playstation 2 Emulator"));
	*this	+= AuthLogoSizer										| StdSpace();

	*this	+= new wxHyperlinkCtrl( this, wxID_ANY,
		_("Pcsx2 Official Website and Forums"), L"http://www.pcsx2.net"
	) | wxSizerFlags(1).Center().Border( wxALL, 3 );

	*this	+= new wxHyperlinkCtrl( this, wxID_ANY,
		_("Pcsx2 Official Svn Repository at Googlecode"), L"http://code.google.com/p/pcsx2"
	) | wxSizerFlags(1).Center().Border( wxALL, 3 );

	*this	+= ContribSizer											| StdExpand();
	*this	+= new wxButton( this, wxID_OK, L"I've seen enough")	| StdCenter();
}
