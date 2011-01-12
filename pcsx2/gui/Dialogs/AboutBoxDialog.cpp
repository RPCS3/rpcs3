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
#include "App.h"
#include "AppCommon.h"

#include "Dialogs/ModalPopups.h"

#include "Resources/EmbeddedImage.h"
#include "Resources/Dualshock.h"

#include <wx/mstream.h>
#include <wx/hyperlink.h>

using namespace pxSizerFlags;

// --------------------------------------------------------------------------------------
//  AboutBoxDialog  Implementation
// --------------------------------------------------------------------------------------

Dialogs::AboutBoxDialog::AboutBoxDialog( wxWindow* parent )
	: wxDialogWithHelpers( parent, AddAppName(_("About %s")), pxDialogFlags().Resize().MinWidth( 460 ) )
	, m_bitmap_dualshock( this, wxID_ANY, wxBitmap( EmbeddedImage<res_Dualshock>().Get() ),
		wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN
	)
{
	// [TODO] : About box should be upgraded to use scrollable read-only text boxes.
	
	static const wxString LabelAuthors = fromUTF8(
		"Arcum42, Refraction, drk||raziel, cottonvibes, gigaherz, "
		"rama, Jake.Stine, saqib, pseudonym, gregory.hainaut"
		"\n\n"
		"Previous versions: Alexey silinov, Aumatt, "
		"Florin, goldfinger, Linuzappz, loser, "
		"Nachbrenner, shadow, Zerofrog, tmkk"
		"\n\n"
		"Betatesting: Bositman, ChaosCode, "
		"CKemu, crushtest, GeneralPlot, "
		"Krakatos, Parotaku, Rudy_X"
		"\n\n"
		"Webmasters: CKemu, Falcon4ever"
	);

	static const wxString LabelGreets = fromUTF8(
		"Hiryu and Sjeep (libcdvd / iso filesystem), nneeve (fpu and vu), n1ckname (compilation guides)"
		"\n\n"
		"Plugin Specialists: ChickenLiver (Lilypad), Efp (efp), "
		"Gabest (Gsdx, Cdvdolio, Xpad),  Zeydlitz (ZZogl)"
		"\n\n"
		"Special thanks to: black_wd, Belmont, BGome, _Demo_, Dreamtime, "
		"F|RES, MrBrown, razorblade, Seta-san, Skarmeth, feal87, Athos"
	);

	// This sizer holds text of the authors and a logo!
	wxFlexGridSizer& AuthLogoSizer = *new wxFlexGridSizer( 2, 0, StdPadding );
	AuthLogoSizer.AddGrowableCol(0, 4);
	AuthLogoSizer.AddGrowableCol(1, 3);

	// this sizer holds text of the contributors/testers, and a ps2 image!
	wxBoxSizer& ContribSizer = *new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer& aboutUs	= *new wxStaticBoxSizer( wxVERTICAL, this );
	wxStaticBoxSizer& contribs	= *new wxStaticBoxSizer( wxVERTICAL, this );

	pxStaticText& label_auth	= Text( LabelAuthors ).SetMinWidth(240);
	pxStaticText& label_greets	= Text( LabelGreets ).SetMinWidth(200);

	aboutUs		+= Heading(L"Developers").Bold()	| StdExpand();
	aboutUs		+= label_auth						| StdExpand();
	contribs	+= Heading(L"Contributors").Bold()	| StdExpand();
	contribs	+= label_greets						| StdExpand();

	AuthLogoSizer	+= aboutUs		| StdExpand();
	AuthLogoSizer	+= contribs		| StdExpand();

	ContribSizer	+= pxStretchSpacer( 1 );
	ContribSizer	+= m_bitmap_dualshock	| StdSpace();
	ContribSizer	+= pxStretchSpacer( 1 );

	// Main (top-level) layout

	*this	+= StdPadding;
	*this	+= Text(wxGetApp().GetAppName()).Bold();
	*this	+= Text(_("A Playstation 2 Emulator"));
	*this	+= AuthLogoSizer						| StdExpand();

	*this	+= new wxHyperlinkCtrl( this, wxID_ANY,
		_("PCSX2 Official Website and Forums"), L"http://www.pcsx2.net"
	) | pxProportion(1).Center().Border( wxALL, 3 );

	*this	+= new wxHyperlinkCtrl( this, wxID_ANY,
		_("PCSX2 Official Svn Repository at Googlecode"), L"http://code.google.com/p/pcsx2"
	) | pxProportion(1).Center().Border( wxALL, 3 );

	*this	+= ContribSizer											| StdExpand();
	*this	+= new wxButton( this, wxID_OK, L"I've seen enough")	| StdCenter();

	int bestHeight = GetBestSize().GetHeight();
	if( bestHeight < 400 ) bestHeight = 400;
	SetMinHeight( bestHeight );
}
