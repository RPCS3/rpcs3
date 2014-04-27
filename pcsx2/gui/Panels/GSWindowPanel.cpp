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

using namespace pxSizerFlags;

// --------------------------------------------------------------------------------------
//  GSWindowSetting Implementation
// --------------------------------------------------------------------------------------

Panels::GSWindowSettingsPanel::GSWindowSettingsPanel( wxWindow* parent )
	: BaseApplicableConfigPanel_SpecificConfig( parent )
{
	const wxString aspect_ratio_labels[] =
	{
		_("Fit to Window/Screen"),
		_("Standard (4:3)"),
		_("Widescreen (16:9)")
	};

	m_text_Zoom = CreateNumericalTextCtrl( this, 5 );

	m_combo_AspectRatio	= new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		ArraySize(aspect_ratio_labels), aspect_ratio_labels, wxCB_READONLY );

	m_text_WindowWidth	= CreateNumericalTextCtrl( this, 5 );
	m_text_WindowHeight	= CreateNumericalTextCtrl( this, 5 );

	// Linux/Mac note: Exclusive Fullscreen mode is probably Microsoft specific, though
	// I'm not yet 100% certain of that.

	m_check_SizeLock	= new pxCheckBox( this, _("Disable window resize border") );
	m_check_HideMouse	= new pxCheckBox( this, _("Always hide mouse cursor") );
	m_check_CloseGS		= new pxCheckBox( this, _("Hide window when paused") );
	m_check_Fullscreen	= new pxCheckBox( this, _("Default to fullscreen mode on open") );
	m_check_VsyncEnable	= new pxCheckBox( this, _("Wait for Vsync on refresh") );
	m_check_ManagedVsync = new pxCheckBox( this, _("Dynamically toggle Vsync depending on frame rate (read tooltip!)") );
	m_check_DclickFullscreen = new pxCheckBox( this, _("Double-click toggles fullscreen mode") );
	//m_check_ExclusiveFS = new pxCheckBox( this, _("Use exclusive fullscreen mode (if available)") );

	m_text_Zoom->SetToolTip( pxEt( L"Zoom = 100: Fit the entire image to the window without any cropping.\nAbove/Below 100: Zoom In/Out\n0: Automatic-Zoom-In untill the black-bars are gone (Aspect ratio is kept, some of the image goes out of screen).\n  NOTE: Some games draw their own black-bars, which will not be removed with '0'.\n\nKeyboard: CTRL + NUMPAD-PLUS: Zoom-In, CTRL + NUMPAD-MINUS: Zoom-Out, CTRL + NUMPAD-*: Toggle 100/0"
	) );

	m_check_VsyncEnable->SetToolTip( pxEt( L"Vsync eliminates screen tearing but typically has a big performance hit. It usually only applies to fullscreen mode, and may not work with all GS plugins."
	) );
	
	m_check_ManagedVsync->SetToolTip( pxEt( L"Enables Vsync when the framerate is exactly at full speed. Should it fall below that, Vsync gets disabled to avoid further performance penalties. Note: This currently only works well with GSdx as GS plugin and with it configured to use DX10/11 hardware rendering. Any other plugin or rendering mode will either ignore it or produce a black frame that blinks whenever the mode switches. It also requires Vsync to be enabled."
	) );

	m_check_HideMouse->SetToolTip( pxEt( L"Check this to force the mouse cursor invisible inside the GS window; useful if using the mouse as a primary control device for gaming.  By default the mouse auto-hides after 2 seconds of inactivity."
	) );

	m_check_Fullscreen->SetToolTip( pxEt( L"Enables automatic mode switch to fullscreen when starting or resuming emulation. You can still toggle fullscreen display at any time using alt-enter."
	) );

/*
	m_check_ExclusiveFS->SetToolTip( pxEt( L"Fullscreen Exclusive Mode may look better on older CRTs and might be a little faster on older video cards, but typically can lead to memory leaks or random crashes when entering/leaving fullscreen mode."
	) );
*/
	m_check_CloseGS->SetToolTip( pxEt( L"Completely closes the often large and bulky GS window when pressing ESC or pausing the emulator."
	) );

	// ----------------------------------------------------------------------------
	//  Layout and Positioning

	wxBoxSizer& s_customsize( *new wxBoxSizer( wxHORIZONTAL ) );
	s_customsize	+= m_text_WindowWidth;
	s_customsize	+= Label( L"x" )	| StdExpand();
	s_customsize	+= m_text_WindowHeight;

	wxFlexGridSizer& s_AspectRatio( *new wxFlexGridSizer( 2, StdPadding, StdPadding ) );
	//s_AspectRatio.AddGrowableCol( 0 );
	s_AspectRatio.AddGrowableCol( 1 );

	s_AspectRatio += Label(_("Aspect Ratio:"))		| pxMiddle;
	s_AspectRatio += m_combo_AspectRatio			| pxExpand;
	s_AspectRatio += Label(_("Custom Window Size:"))| pxMiddle;
	s_AspectRatio += s_customsize					| pxAlignRight;

	s_AspectRatio	+= Label(_("Zoom:"))			| StdExpand();
	s_AspectRatio	+= m_text_Zoom;


	*this += s_AspectRatio				| StdExpand();
	*this += m_check_SizeLock;
	*this += m_check_HideMouse;
	*this += m_check_CloseGS;
	*this += new wxStaticLine( this )	| StdExpand();

	*this += m_check_Fullscreen;
	*this += m_check_DclickFullscreen;

	//*this += m_check_ExclusiveFS;
	*this += new wxStaticLine( this )	| StdExpand();

	*this += m_check_VsyncEnable;
	*this += m_check_ManagedVsync;

	wxBoxSizer* centerSizer = new wxBoxSizer( wxVERTICAL );
	*centerSizer += GetSizer()	| pxCenter;
	SetSizer( centerSizer, false );

	AppStatusEvent_OnSettingsApplied();
}

void Panels::GSWindowSettingsPanel::AppStatusEvent_OnSettingsApplied()
{
	ApplyConfigToGui( *g_Conf );
}

void Panels::GSWindowSettingsPanel::ApplyConfigToGui( AppConfig& configToApply, int flags )
{
	const AppConfig::GSWindowOptions& conf( configToApply.GSWindow );

	if( !(flags & AppConfig::APPLY_FLAG_FROM_PRESET) )	
	{//Presets don't control these values
		m_check_CloseGS		->SetValue( conf.CloseOnEsc );
		m_check_Fullscreen	->SetValue( conf.DefaultToFullscreen );
		m_check_HideMouse	->SetValue( conf.AlwaysHideMouse );
		m_check_SizeLock	->SetValue( conf.DisableResizeBorders );

		m_combo_AspectRatio	->SetSelection( (int)conf.AspectRatio );
		m_text_Zoom			->ChangeValue( conf.Zoom.ToString() );

		m_check_DclickFullscreen ->SetValue ( conf.IsToggleFullscreenOnDoubleClick );

		m_text_WindowWidth	->ChangeValue( wxsFormat( L"%d", conf.WindowSize.GetWidth() ) );
		m_text_WindowHeight	->ChangeValue( wxsFormat( L"%d", conf.WindowSize.GetHeight() ) );
	}

	m_check_VsyncEnable->SetValue( configToApply.EmuOptions.GS.VsyncEnable );
	m_check_VsyncEnable->Enable  ( !configToApply.EnablePresets );//grayed-out when presets are enabled

	m_check_ManagedVsync->SetValue( configToApply.EmuOptions.GS.ManagedVsync );
	m_check_ManagedVsync->Enable  ( !configToApply.EnablePresets );//grayed-out when presets are enabled
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
	appconf.Zoom			= Fixed100::FromString( m_text_Zoom->GetValue() );

	gsconf.VsyncEnable		= m_check_VsyncEnable->GetValue();
	gsconf.ManagedVsync		= m_check_ManagedVsync->GetValue();

	appconf.IsToggleFullscreenOnDoubleClick = m_check_DclickFullscreen->GetValue();

	long xr, yr = 1;

	if( !m_text_WindowWidth->GetValue().ToLong( &xr ) || !m_text_WindowHeight->GetValue().ToLong( &yr ) )
		throw Exception::CannotApplySettings( this )
			.SetDiagMsg(L"User submitted non-numeric window size parameters!")
			.SetUserMsg(_("Invalid window dimensions specified: Size cannot contain non-numeric digits! >_<"));

	appconf.WindowSize.x	= xr;
	appconf.WindowSize.y	= yr;
}
