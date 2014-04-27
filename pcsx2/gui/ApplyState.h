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

#pragma once

#include <list>
#include <wx/wizard.h>

#include "AppCommon.h"

class BaseApplicableConfigPanel;
class BaseApplicableDialog;

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE( pxEvt_ApplySettings, -1 )
END_DECLARE_EVENT_TYPES()

namespace Exception
{
	// --------------------------------------------------------------------------
	// Exception used to perform an abort of Apply/Ok action on settings panels.
	// When thrown, the user receives a popup message containing the information
	// specified in the exception message, and is returned to the settings dialog
	// to correct the invalid input fields.
	//
	class CannotApplySettings : public BaseException
	{
		DEFINE_EXCEPTION_COPYTORS( CannotApplySettings, BaseException )
		DEFINE_EXCEPTION_MESSAGES( CannotApplySettings )

	public:
		bool						IsVerbose;

	protected:
		BaseApplicableConfigPanel*	m_Panel;
		
	protected:
		CannotApplySettings() { IsVerbose = true; }

	public:
		explicit CannotApplySettings( BaseApplicableConfigPanel* thispanel )
		{
			SetBothMsgs(pxL("Cannot apply new settings, one of the settings is invalid."));
			m_Panel = thispanel;
			IsVerbose = true;
		}

		virtual CannotApplySettings& Quiet() { IsVerbose = false; return *this; }
		BaseApplicableConfigPanel* GetPanel() { return m_Panel; }
	};
}

typedef std::list<BaseApplicableConfigPanel*> PanelApplyList_t;

struct ApplyStateStruct
{
	// collection of ApplicableConfigPanels that belong to a dialog box.
	PanelApplyList_t PanelList;

	// Current book page being initialized.  Any apply objects created will use
	// this page as their "go here on error" page. (used to take the user to the
	// page with the option that failed apply validation).
	int CurOwnerPage;

	// TODO : Rename me to CurOwnerBook, or rename the one above to ParentPage.
	wxBookCtrlBase* ParentBook;

	ApplyStateStruct()
	{
		CurOwnerPage	= wxID_NONE;
		ParentBook		= NULL;
	}

	void SetCurrentPage( int page )
	{
		CurOwnerPage = page;
	}

	void ClearCurrentPage()
	{
		CurOwnerPage = wxID_NONE;
	}

	void StartBook( wxBookCtrlBase* book );
	void StartWizard();
	bool ApplyAll();
	bool ApplyPage( int pageid );
	void DoCleanup() throw();
};

class IApplyState
{
protected:
	ApplyStateStruct	m_ApplyState;

public:
	virtual ApplyStateStruct& GetApplyState() { return m_ApplyState; }
};

// --------------------------------------------------------------------------------------
//  BaseApplicableDialog
// --------------------------------------------------------------------------------------
class BaseApplicableDialog
	: public wxDialogWithHelpers
	, public IApplyState
{
	DECLARE_DYNAMIC_CLASS_NO_COPY(BaseApplicableDialog)

public:
	BaseApplicableDialog() {}
	virtual ~BaseApplicableDialog() throw();

	// Must return the same thing as GetNameStatic; a name ideal for use in uniquely
	// identifying dialogs.  (this version is the 'instance' version, which is called
	// by BaseConfigurationDialog to assign the wxWidgets dialog name, and for saving
	// screenshots to disk)
	virtual wxString GetDialogName() const;

protected:
	void Init();

	BaseApplicableDialog(wxWindow* parent, const wxString& title, const pxDialogCreationFlags& cflags = pxDialogCreationFlags() );

	virtual void OnSettingsApplied( wxCommandEvent& evt );

	// Note: This method *will* be called automatically after a successful Apply, but will not
	// be called after a failed Apply (canceled due to error).
	virtual void AppStatusEvent_OnSettingsApplied() {}

	virtual void AppStatusEvent_OnUiSettingsLoadSave( const AppSettingsEventInfo& ) {}
	virtual void AppStatusEvent_OnExit() {}
};

// --------------------------------------------------------------------------------------
//  BaseApplicableConfigPanel
// --------------------------------------------------------------------------------------
// Extends the Panel class to add an Apply() method, which is invoked from the parent
// window (usually the ConfigurationDialog) when either Ok or Apply is clicked.
//
// Thread Safety: None.  This class is only safe when used from the GUI thread, as it uses
//   static vars and assumes that only one ApplicableConfig system is available to the
//   user at any time (ie, a singular modal dialog).
//
class BaseApplicableConfigPanel
	: public wxPanelWithHelpers
{
protected:
	int				m_OwnerPage;
	wxBookCtrlBase*	m_OwnerBook;

	EventListenerHelper_AppStatus<BaseApplicableConfigPanel>	m_AppStatusHelper;

public:
	virtual ~BaseApplicableConfigPanel() throw();

	BaseApplicableConfigPanel( wxWindow* parent, wxOrientation orient=wxVERTICAL );
	BaseApplicableConfigPanel( wxWindow* parent, wxOrientation orient, const wxString& staticLabel );

	int GetOwnerPage() const { return m_OwnerPage; }
	wxBookCtrlBase* GetOwnerBook() { return m_OwnerBook; }

	void SetFocusToMe();
	IApplyState* FindApplyStateManager() const;

	// Returns true if this ConfigPanel belongs to the specified page.  Useful for doing
	// selective application of options for specific pages.
	bool IsOnPage( int pageid ) { return m_OwnerPage == pageid; }

	// This method attempts to assign the settings for the panel into the given
	// configuration structure (which is typically a copy of g_Conf).  If validation
	// of form contents fails, the function should throw Exception::CannotApplySettings.
	// If no exceptions are thrown, then the operation is assumed a success. :)
	virtual void Apply()=0;

	//Enable the apply button manually if required (in case the auto-trigger doesn't kick in, e.g. when clicking a button)
	void SomethingChanged();


	void Init();

	// Mandatory override: As a rule for proper interface design, all deriving classes need
	// to implement this OnSettingsApplied function.  There's no implementation of an options/
	// settings panel that does not heed the changes of application status/settings changes. ;)
	//
	// Note: This method *will* be called automatically after a successful Apply, but will not
	// be called after a failed Apply (canceled due to error).
	virtual void AppStatusEvent_OnSettingsApplied()=0;

	virtual void AppStatusEvent_OnUiSettingsLoadSave( const AppSettingsEventInfo& ) {}
	virtual void AppStatusEvent_OnVmSettingsLoadSave( const AppSettingsEventInfo& ) {}
	virtual void AppStatusEvent_OnExit() {}

	virtual bool IsSpecificConfig(){return false;} //lame RTTI

protected:
	virtual void OnSettingsApplied( wxCommandEvent& evt );
};

class BaseApplicableConfigPanel_SpecificConfig : public BaseApplicableConfigPanel
{
    //This class only differs from BaseApplicableConfigPanel by systematically allowing the gui to be updated
    //from a specific config (used by the presets system to trash not g_Conf in case the user pressed "Cancel").
    //Every panel that the Presets affect should be derived from this and not from BaseApplicableConfigPanel.
    //
    //Multiple inheritance would have been better (also for cases of non BaseApplicableConfigPanel which are effected by presets),
    //but multiple inheritance sucks. So, subclass.
    //NOTE: because ApplyConfigToGui is called manually and not via an event, it must consider manuallyPropagate and call sub panels.
public:
	virtual bool IsSpecificConfig(){return true;};
	BaseApplicableConfigPanel_SpecificConfig( wxWindow* parent, wxOrientation orient=wxVERTICAL );
	BaseApplicableConfigPanel_SpecificConfig( wxWindow* parent, wxOrientation orient, const wxString& staticLabel );

	//possible flags are: AppConfig: APPLY_FLAG_MANUALLY_PROPAGATE and APPLY_FLAG_IS_FROM_PRESET
	virtual void ApplyConfigToGui(AppConfig& configToApply, int flags=0)=0;
};

class ApplicableWizardPage : public wxWizardPageSimple, public IApplyState
{
	DECLARE_DYNAMIC_CLASS_NO_COPY(ApplicableWizardPage)

public:
	ApplicableWizardPage(
		wxWizard* parent=NULL,
		wxWizardPage *prev = NULL,
		wxWizardPage *next = NULL,
		const wxBitmap& bitmap = wxNullBitmap
	);

	virtual ~ApplicableWizardPage() throw() { m_ApplyState.DoCleanup(); }
	
	virtual bool PrepForApply();
};

