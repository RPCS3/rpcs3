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
	
	wxString LabelAuthors = wxsFormat(
		L"Arcum42, avih, Refraction, drk||raziel, cottonvibes, gigaherz, "
		L"rama, Jake.Stine, saqib, pseudonym, gregory.hainaut"
		L"\n\n"
		L"%s: Alexey silinov, Aumatt, "
		L"Florin, goldfinger, Linuzappz, loser, "
		L"Nachbrenner, shadow, Zerofrog, tmkk"
		L"\n\n"
		L"%s: Bositman, ChaosCode, "
		L"CKemu, crushtest, GeneralPlot, "
		L"Krakatos, Parotaku, Rudy_X"
		L"\n\n"
		L"%s: CKemu, Falcon4ever",
		_("Previous versions"), _("Betatesting"), _("Webmasters"));


	wxString LabelGreets = wxsFormat( 
		L"Hiryu and Sjeep (libcdvd / iso filesystem), nneeve (fpu and vu), n1ckname (compilation guides), Shadow Lady"
		L"\n\n"
			L"%s: ChickenLiver (Lilypad), Efp (efp), "
		L"Gabest (Gsdx, Cdvdolio, Xpad),  Zeydlitz (ZZogl)"
		L"\n\n"
			L"%s: black_wd, Belmont, BGome, _Demo_, Dreamtime, "
			L"F|RES, Jake.Stine, MrBrown, razorblade, Seta-san, Skarmeth, feal87, Athos",
			_("Plugin Specialists"), _("Special thanks to"));

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

	aboutUs		+= Heading(_("Developers")).Bold()	| StdExpand();
	aboutUs		+= label_auth						| StdExpand();
	contribs	+= Heading(_("Contributors")).Bold()	| StdExpand();
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
	*this	+= new wxButton( this, wxID_OK, _("I've seen enough"))	| StdCenter();

	int bestHeight = GetBestSize().GetHeight();
	if( bestHeight < 400 ) bestHeight = 400;
	SetMinHeight( bestHeight );
}
