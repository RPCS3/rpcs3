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
#include "ConfigurationPanels.h"

using namespace pxSizerFlags;

// --------------------------------------------------------------------------------------
//  GSWindowSetting Implementation
// --------------------------------------------------------------------------------------

Panels::GSWindowSettingsPanel::GSWindowSettingsPanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	const wxString aspect_ratio_labels[] =
	{
		_("Fit to Window/Screen"),
		_("Standard (4:3)"),
		_("Widescreen (16:9)")
	};

	m_combo_AspectRatio	= new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		ArraySize(aspect_ratio_labels), aspect_ratio_labels, wxCB_READONLY );

	m_text_WindowWidth	= CreateNumericalTextCtrl( this, 5 );
	m_text_WindowHeight	= CreateNumericalTextCtrl( this, 5 );

	// Linux/Mac note: Exclusive Fullscreen mode is probably Microsoft specific, though
	// I'm not yet 100% certain of that.

	m_check_SizeLock	= new pxCheckBox( this, _("Disable window resize border") );
	m_check_HideMouse	= new pxCheckBox( this, _("Always hide mouse cursor") );
	m_check_CloseGS		= new pxCheckBox( this, _("Hide window on suspend") );
	m_check_Fullscreen	= new pxCheckBox( this, _("Default to fullscreen mode on open") );
	m_check_VsyncEnable	= new pxCheckBox( this, _("Wait for vsync on refresh") );
	m_check_ExclusiveFS = new pxCheckBox( this, _("Use exclusive fullscreen mode (if available)") );
	
	m_check_VsyncEnable->SetToolTip( pxE( ".Tooltips:Video:Vsync",
		L"Vsync eliminates screen tearing but typically has a big performance hit. "
		L"It usually only applies to fullscreen mode, and may not work with all GS plugins."
	) );
	
	m_check_HideMouse->SetToolTip( pxE( ".Tooltips:Video:HideMouse",
		L"Check this to force the mouse cursor invisible inside the GS window; useful if using "
		L"the mouse as a primary control device for gaming.  By default the mouse auto-hides after "
		L"2 seconds of inactivity."
	) );

	m_check_Fullscreen->SetToolTip( pxE( ".Tooltips:Video:Fullscreen",
		L"Enables automatic mode switch to fullscreen when starting or resuming emulation. "
		L"You can still toggle fullscreen display at any time using alt-enter."
	) );

	m_check_ExclusiveFS->SetToolTip( pxE( ".Video:FullscreenExclusive",
		L"Fullscreen Exclusive Mode may look better on older CRTs and might be a little faster on older video cards, "
		L"but typically can lead to memory leaks or random crashes when entering/leaving fullscreen mode."
	) );

	m_check_CloseGS->SetToolTip( pxE( ".Tooltips:Video:HideGS",
		L"Completely closes the often large and bulky GS window when pressing "
		L"ESC or suspending the emulator."
	) );
	
	// ----------------------------------------------------------------------------
	//  Layout and Positioning

	wxBoxSizer& s_customsize( *new wxBoxSizer( wxHORIZONTAL ) );
	s_customsize	+= m_text_WindowWidth;
	s_customsize	+= Text( L"x" );
	s_customsize	+= m_text_WindowHeight;

	wxFlexGridSizer& s_AspectRatio( *new wxFlexGridSizer( 2, StdPadding, StdPadding ) );
	//s_AspectRatio.AddGrowableCol( 0 );
	s_AspectRatio.AddGrowableCol( 1 );

	s_AspectRatio += Text(_("Aspect Ratio:"))		| pxMiddle;
	s_AspectRatio += m_combo_AspectRatio			| pxExpand;
	s_AspectRatio += Text(_("Custom Window Size:"))	| pxMiddle;
	s_AspectRatio += s_customsize					| pxAlignRight;

	*this += s_AspectRatio				| StdExpand();
	*this += m_check_SizeLock;
	*this += m_check_HideMouse;
	*this += m_check_CloseGS;
	*this += new wxStaticLine( this )	| StdExpand();

	*this += m_check_Fullscreen;
	*this += m_check_ExclusiveFS;
	*this += m_check_VsyncEnable;
	
	wxBoxSizer* centerSizer = new wxBoxSizer( wxVERTICAL );
	*centerSizer += GetSizer()	| pxCenter;
	SetSizer( centerSizer, false );

	OnSettingsChanged();
}

void Panels::GSWindowSettingsPanel::OnSettingsChanged()
{
	const AppConfig::GSWindowOptions& conf( g_Conf->GSWindow );

	m_check_CloseGS		->SetValue( conf.CloseOnEsc );
	m_check_Fullscreen	->SetValue( conf.DefaultToFullscreen );
	m_check_HideMouse	->SetValue( conf.AlwaysHideMouse );
	m_check_SizeLock	->SetValue( conf.DisableResizeBorders );

	m_combo_AspectRatio	->SetSelection( (int)conf.AspectRatio );

	m_check_VsyncEnable	->SetValue( g_Conf->EmuOptions.GS.VsyncEnable );

	m_text_WindowWidth	->SetValue( wxsFormat( L"%d", conf.WindowSize.GetWidth() ) );
	m_text_WindowHeight	->SetValue( wxsFormat( L"%d", conf.WindowSize.GetHeight() ) );

}

void Panels::GSWindowSettingsPanel::Apply()
{
	AppConfig::GSWindowOptions& appconf( g_Conf->GSWindow );
	Pcsx2Config::GSOptions& gsconf( g_Conf->EmuOptions.GS );

	appconf.CloseOnEsc				= m_check_CloseGS	->GetValue();
	appconf.DefaultToFullscreen		= m_check_Fullscreen->GetValue();
	appconf.AlwaysHideMouse			= m_check_HideMouse	->GetValue();
	appconf.DisableResizeBorders	= m_check_SizeLock	->GetValue();

	appconf.AspectRatio		= (AspectRatioType)m_combo_AspectRatio->GetSelection();

	gsconf.VsyncEnable		= m_check_VsyncEnable->GetValue();

	long xr, yr;

	if( !m_text_WindowWidth->GetValue().ToLong( &xr ) || !m_text_WindowHeight->GetValue().ToLong( &yr ) )
		throw Exception::CannotApplySettings( this,
			L"User submitted non-numeric window size parameters!",
			_("Invalid window dimensions specified: Size cannot contain non-numeric digits! >_<")
		);

	appconf.WindowSize.x	= xr;
	appconf.WindowSize.y	= yr;
}
