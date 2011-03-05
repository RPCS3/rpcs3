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
#include "MainFrame.h"
#include "Utilities/IniInterface.h"
#include "Utilities/HashMap.h"
#include "Dialogs/ModalPopups.h"

#include <wx/stdpaths.h>

#ifdef __WXMSW__
#include "wx/msw/regconf.h"
#endif

DocsModeType			DocsFolderMode = DocsFolder_User;
bool					UseDefaultSettingsFolder = true;
bool					UseDefaultPluginsFolder = true;
bool					UseDefaultThemesFolder = true;


wxDirName				CustomDocumentsFolder;
wxDirName				SettingsFolder;

wxDirName				InstallFolder;
wxDirName				PluginsFolder;
wxDirName				ThemesFolder;

// The UserLocalData folder can be redefined depending on whether or not PCSX2 is in
// "portable install" mode or not.  when PCSX2 has been configured for portable install, the
// UserLocalData folder is the current working directory.
//
InstallationModeType		InstallationMode;

static wxFileName GetPortableIniPath()
{
	wxString programFullPath = wxStandardPaths::Get().GetExecutablePath();
	wxDirName programDir( wxFileName(programFullPath).GetPath() );

	return programDir + "portable.ini";
}

static wxString GetMsg_PortableModeRights()
{
	return pxE( "!Notice:PortableModeRights",
		L"Please ensure that these folders are created and that your user account is granted "
		L"write permissions to them -- or re-run PCSX2 with elevated (administrator) rights, which "
		L"should grant PCSX2 the ability to create the necessary folders itself.  If you "
		L"do not have elevated rights on this computer, then you will need to switch to User "
		L"Documents mode (click button below)."
	);
};

bool Pcsx2App::TestUserPermissionsRights( const wxDirName& testFolder, wxString& createFailedStr, wxString& accessFailedStr )
{
	// We need to individually verify read/write permission for each PCSX2 user documents folder.
	// If any of the folders are not writable, then the user should be informed asap via
	// friendly and courteous dialog box!

	const wxDirName PermissionFolders[] = 
	{
		PathDefs::Base::Settings(),
		PathDefs::Base::MemoryCards(),
		PathDefs::Base::Savestates(),
		PathDefs::Base::Snapshots(),
		PathDefs::Base::Logs(),
		#ifdef PCSX2_DEVBUILD
		PathDefs::Base::Dumps(),
		#endif
	};

	FastFormatUnicode createme, accessme;

	for (uint i=0; i<ArraySize(PermissionFolders); ++i)
	{
		wxDirName folder( testFolder + PermissionFolders[i] );

		if (!folder.Mkdir())
			createme += L"\t" + folder.ToString() + L"\n";

		if (!folder.IsWritable())
			accessme += L"\t" + folder.ToString() + L"\n";
	}

	if (!accessme.IsEmpty())
	{
		accessFailedStr = (wxString)_("The following folders exist, but are not writable:") + L"\n" + accessme;
	}
	
	if (!createme.IsEmpty())
	{
		createFailedStr = (wxString)_("The following folders are missing and cannot be created:") + L"\n" + createme;
	}

	return (createFailedStr.IsEmpty() && accessFailedStr.IsEmpty());
}

// Portable installations are assumed to be run in either administrator rights mode, or run
// from "insecure media" such as a removable flash drive.  In these cases, the default path for
// PCSX2 user documents becomes ".", which is the current working directory.
//
// Portable installation mode is typically enabled via the presence of an INI file in the
// same directory that PCSX2 is installed to.
//
wxConfigBase* Pcsx2App::TestForPortableInstall()
{
	InstallationMode = InstallMode_Registered;

	const wxFileName portableIniFile( GetPortableIniPath() );
	const wxDirName portableDocsFolder( portableIniFile.GetPath() );

	if (Startup.PortableMode || portableIniFile.FileExists())
	{
		wxString FilenameStr = portableIniFile.GetFullPath();
		if (Startup.PortableMode)
			Console.WriteLn( L"(UserMode) Portable mode requested via commandline switch!" );
		else
			Console.WriteLn( L"(UserMode) Found portable install ini @ %s", FilenameStr.c_str() );

		// Just because the portable ini file exists doesn't mean we can actually run in portable
		// mode.  In order to determine our read/write permissions to the PCSX2, we must try to
		// modify the configured documents folder, and catch any ensuing error.

		ScopedPtr<wxFileConfig> conf_portable( OpenFileConfig( portableIniFile.GetFullPath() ) );
		conf_portable->SetRecordDefaults(false);

		while( true )
		{
			wxString accessFailedStr, createFailedStr;
			if (TestUserPermissionsRights( portableDocsFolder, createFailedStr, accessFailedStr )) break;
		
			wxDialogWithHelpers dialog( NULL, AddAppName(_("Portable mode error - %s")) );

			wxTextCtrl* scrollText = new wxTextCtrl(
				&dialog, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
				wxTE_READONLY | wxTE_MULTILINE | wxTE_WORDWRAP
			);

			if (!createFailedStr.IsEmpty())
				scrollText->AppendText( createFailedStr + L"\n" );

			if (!accessFailedStr.IsEmpty())
				scrollText->AppendText( accessFailedStr + L"\n" );

			dialog += dialog.Heading( _("PCSX2 has been installed as a portable application but cannot run due to the following errors:" ) );
			dialog += scrollText | pxExpand.Border(wxALL, 16);
			dialog += 6;
			dialog += dialog.Text( GetMsg_PortableModeRights() );
			
			// [TODO] : Add url for platform-relevant user permissions tutorials?  (low priority)

			wxWindowID result = pxIssueConfirmation( dialog,
				MsgButtons().Retry().Cancel().Custom(_("Switch to User Documents Mode"), "switchmode")
			);
			
			switch (result)
			{
				case wxID_CANCEL:
					throw Exception::StartupAborted( L"User canceled portable mode due to insufficient user access/permissions." );

				case wxID_RETRY:
					// do nothing (continues while loop)
				break;
				
				case pxID_CUSTOM:
					wxDialogWithHelpers dialog2( NULL, AddAppName(_("%s is switching to local install mode.")) );
					dialog2 += dialog2.Heading( _("Try to remove the file called \"portable.ini\" from your installation directory manually." ) );
					dialog2 += 6;
					pxIssueConfirmation( dialog2, MsgButtons().OK() );
					conf_portable.DetachPtr(); // Not sure but can't hurt
					
					return NULL;
			}

		}
	
		// Success -- all user-based folders have write access.  PCSX2 should be able to run error-free!
		// Force-set the custom documents mode, and set the 

		InstallationMode = InstallMode_Portable;
		DocsFolderMode = DocsFolder_Custom;
		CustomDocumentsFolder = portableDocsFolder;
		return conf_portable.DetachPtr();
	}
	
	return NULL;
}

// Reset RunWizard so the FTWizard is run again on next PCSX2 start.
void Pcsx2App::WipeUserModeSettings()
{	
	if (InstallationMode == InstallMode_Portable)
	{
		// Remove the portable.ini entry "RunWizard" conforming to this instance of PCSX2.
		wxFileName portableIniFile( GetPortableIniPath() );
		ScopedPtr<wxFileConfig> conf_portable( OpenFileConfig( portableIniFile.GetFullPath() ) );
		conf_portable->DeleteEntry(L"RunWizard");
	}
	else 
	{
		// Remove the registry entry "RunWizard" conforming to this instance of PCSX2.
		ScopedPtr<wxConfigBase> conf_install( OpenInstallSettingsFile() );
		conf_install->DeleteEntry(L"RunWizard");
	}
}

static void DoFirstTimeWizard()
{
	// first time startup, so give the user the choice of user mode:
	while(true)
	{
		// PCSX2's FTWizard allows improptu restarting of the wizard without cancellation.
		// This is typically used to change the user's language selection.

		FirstTimeWizard wiz( NULL );
		if( wiz.RunWizard( wiz.GetFirstPage() ) ) break;
		if (wiz.GetReturnCode() != pxID_RestartWizard)
			throw Exception::StartupAborted( L"User canceled FirstTime Wizard." );

		Console.WriteLn( Color_StrongBlack, "Restarting First Time Wizard!" );
	}
}

wxConfigBase* Pcsx2App::OpenInstallSettingsFile()
{
	// Implementation Notes:
	//
	// As of 0.9.8 and beyond, PCSX2's versioning should be strong enough to base ini and
	// plugin compatibility on version information alone.  This in turn allows us to ditch
	// the old system (CWD-based ini file mess) in favor of a system that simply stores
	// most core application-level settings in the registry.

	ScopedPtr<wxConfigBase> conf_install;

#ifdef __WXMSW__
	conf_install = new wxRegConfig();
#else
	// FIXME!!  Linux / Mac
	// Where the heck should this information be stored?

	wxDirName usrlocaldir( PathDefs::GetUserLocalDataDir() );
	//wxDirName usrlocaldir( wxStandardPaths::Get().GetDataDir() );
	if( !usrlocaldir.Exists() )
	{
		Console.WriteLn( L"Creating UserLocalData folder: " + usrlocaldir.ToString() );
		usrlocaldir.Mkdir();
	}

	wxFileName usermodefile( GetAppName() + L"-reg.ini" );
	usermodefile.SetPath( usrlocaldir.ToString() );
	conf_install = OpenFileConfig( usermodefile.GetFullPath() );
#endif

	return conf_install.DetachPtr();
}


void Pcsx2App::EstablishAppUserMode()
{
	ScopedPtr<wxConfigBase> conf_install;

	conf_install = TestForPortableInstall();
	if (!conf_install)
		conf_install = OpenInstallSettingsFile();	

	conf_install->SetRecordDefaults(false);

	//  Run the First Time Wizard!
	// ----------------------------
	// Wizard is only run once.  The status of the wizard having been run is stored in
	// the installation ini file, which can be either the portable install (useful for admins)
	// or the registry/user local documents position.

	bool runWiz;
	conf_install->Read( L"RunWizard", &runWiz, true );

	App_LoadInstallSettings( conf_install );

	if( !Startup.ForceWizard && !runWiz )
	{
		AppConfig_OnChangedSettingsFolder( false );
		return;
	}

	DoFirstTimeWizard();

	// Save user's new settings
	App_SaveInstallSettings( conf_install );
	AppConfig_OnChangedSettingsFolder( true );
	AppSaveSettings();

	// Wizard completed successfully, so let's not torture the user with this crap again!
	conf_install->Write( L"RunWizard", false );
}

