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

// All of the options screens in PCSX2 are implemented as panels which can be bound to
// either their own dialog boxes, or made into children of a paged Properties box.  The
// paged Properties box is generally superior design, and there's a good chance we'll not
// want to deviate form that design anytime soon.  But there's no harm in keeping nice
// encapsulation of panels regardless, just in case we want to shuffle things around. :)

#pragma once

#include <wx/image.h>
#include <wx/statline.h>
#include <wx/bookctrl.h>

#include <list>

#include "AppConfig.h"
#include "wxHelpers.h"
#include "Utilities/SafeArray.h"
#include "Utilities/Threading.h"
#include "Utilities/EventSource.h"

class wxListBox;

// Annoyances of C++ forward declarations.  Having to type in this red tape mess
// is keeping me form eating a good sandwich right now... >_<
namespace Panels
{
	class BaseApplicableConfigPanel;
}

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
	public:
		bool		IsVerbose;

	protected:
		Panels::BaseApplicableConfigPanel* m_Panel;

	public:
		DEFINE_EXCEPTION_COPYTORS( CannotApplySettings )

		explicit CannotApplySettings( Panels::BaseApplicableConfigPanel* thispanel, const char* msg=wxLt("Cannot apply new settings, one of the settings is invalid."), bool isVerbose = true )
		{
			BaseException::InitBaseEx( msg );
			m_Panel = thispanel;
			IsVerbose = isVerbose;
		}

		explicit CannotApplySettings( Panels::BaseApplicableConfigPanel* thispanel, const wxString& msg_eng, const wxString& msg_xlt )
		{
			BaseException::InitBaseEx( msg_eng, msg_xlt );
			m_Panel = thispanel;
			IsVerbose = true;
		}

		Panels::BaseApplicableConfigPanel* GetPanel()
		{
			return m_Panel;
		}
	};
}

namespace Panels
{
	typedef std::list<BaseApplicableConfigPanel*> PanelApplyList_t;

	struct StaticApplyState
	{
		// Static collection of ApplicableConfigPanels currently available to the user.
		PanelApplyList_t PanelList;

		// Current book page being initialized.  Any apply objects created will use
		// this page as their "go here on error" page. (used to take the user to the
		// page with the option that failed apply validation).
		int CurOwnerPage;

		// TODO : Rename me to CurOwnerBook, or rename the one above to ParentPage.
		wxBookCtrlBase* ParentBook;

		StaticApplyState() :
			PanelList()
		,	CurOwnerPage( wxID_NONE )
		,	ParentBook( NULL )
		{
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
		bool ApplyAll( bool saveOnSuccess=true );
		bool ApplyPage( int pageid, bool saveOnSuccess=true );
		void DoCleanup() throw();
	};

	extern StaticApplyState g_ApplyState;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Extends the Panel class to add an Apply() method, which is invoked from the parent
	// window (usually the ConfigurationDialog) when either Ok or Apply is clicked.
	//
	// Thread Safety: None.  This class is only safe when used from the GUI thread, as it uses
	//   static vars and assumes that only one ApplicableConfig system is available to the
	//   user at any time (ie, a singular modal dialog).
	//
	class BaseApplicableConfigPanel : public wxPanelWithHelpers
	{
	protected:
		int m_OwnerPage;
		wxBookCtrlBase* m_OwnerBook;

	public:
		virtual ~BaseApplicableConfigPanel()
		{
			g_ApplyState.PanelList.remove( this );
		}

		BaseApplicableConfigPanel( wxWindow* parent, int idealWidth ) :
			wxPanelWithHelpers( parent, idealWidth )
		,	m_OwnerPage( g_ApplyState.CurOwnerPage )
		,	m_OwnerBook( g_ApplyState.ParentBook )
		{
			g_ApplyState.PanelList.push_back( this );
		}

		int GetOwnerPage() const { return m_OwnerPage; }
		wxBookCtrlBase* GetOwnerBook() { return m_OwnerBook; }

		void SetFocusToMe()
		{
			if( (m_OwnerBook == NULL) || (m_OwnerPage == wxID_NONE) ) return;
			m_OwnerBook->SetSelection( m_OwnerPage );
		}

		// Returns true if this ConfigPanel belongs to the specified page.  Useful for doing
		// selective application of options for specific pages.
		bool IsOnPage( int pageid ) { return m_OwnerPage == pageid; }

		// This method attempts to assign the settings for the panel into the given
		// configuration structure (which is typically a copy of g_Conf).  If validation
		// of form contents fails, the function should throw Exception::CannotApplySettings.
		// If no exceptions are thrown, then the operation is assumed a success. :)
		virtual void Apply()=0;
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class UsermodeSelectionPanel : public BaseApplicableConfigPanel
	{
	protected:
		wxRadioButton* m_radio_user;
		wxRadioButton* m_radio_cwd;

	public:
		virtual ~UsermodeSelectionPanel() { }
		UsermodeSelectionPanel( wxWindow& parent, int idealWidth=wxDefaultCoord, bool isFirstTime = true );

		void Apply();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class LanguageSelectionPanel : public BaseApplicableConfigPanel
	{
	protected:
		LangPackList	m_langs;
		wxComboBox*		m_picker;

	public:
		virtual ~LanguageSelectionPanel() { }
		LanguageSelectionPanel( wxWindow& parent, int idealWidth=wxDefaultCoord );

		void Apply();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class CpuPanel : public BaseApplicableConfigPanel
	{
	protected:
		wxRadioButton* m_Option_RecEE;
		wxRadioButton* m_Option_RecIOP;
		wxRadioButton* m_Option_mVU0;
		wxRadioButton* m_Option_mVU1;

		wxRadioButton* m_Option_sVU0;
		wxRadioButton* m_Option_sVU1;

	public:
		CpuPanel( wxWindow& parent, int idealWidth );
		void Apply();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class VideoPanel : public BaseApplicableConfigPanel
	{
	protected:

	public:
		VideoPanel( wxWindow& parent, int idealWidth );
		void Apply();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class SpeedHacksPanel : public BaseApplicableConfigPanel
	{
	protected:
		wxSlider* m_slider_eecycle;
		wxSlider* m_slider_vustealer;
		wxStaticText* m_msg_eecycle;
		wxStaticText* m_msg_vustealer;

		wxCheckBox* m_check_intc;
		wxCheckBox* m_check_b1fc0;
		wxCheckBox* m_check_IOPx2;
		wxCheckBox* m_check_vuFlagHack;
		wxCheckBox* m_check_vuMinMax;

	public:
		SpeedHacksPanel( wxWindow& parent, int idealWidth );
		void Apply();

	protected:
		const wxChar* GetEEcycleSliderMsg( int val );
		const wxChar* GetVUcycleSliderMsg( int val );

		void Slider_Click(wxScrollEvent &event);
		void EECycleRate_Scroll(wxScrollEvent &event);
		void VUCycleRate_Scroll(wxScrollEvent &event);
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class GameFixesPanel: public BaseApplicableConfigPanel
	{
	protected:
		wxCheckBox*			m_checkbox[NUM_OF_GAME_FIXES];

	public:
		GameFixesPanel( wxWindow& parent, int idealWidth );
		void Apply();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// DirPickerPanel
	// A simple panel which provides a specialized configurable directory picker with a
	// "[x] Use Default setting" option, which enables or disables the panel.
	//
	class DirPickerPanel : public BaseApplicableConfigPanel
	{
	protected:
		FoldersEnum_t		m_FolderId;
		wxDirPickerCtrl*	m_pickerCtrl;
		wxCheckBox*			m_checkCtrl;

	public:
		DirPickerPanel( wxWindow* parent, FoldersEnum_t folderid, const wxString& label, const wxString& dialogLabel );
		virtual ~DirPickerPanel() { }

		void Apply();
		void Reset();
		wxDirName GetPath() const { return wxDirName( m_pickerCtrl->GetPath() ); }

		DirPickerPanel& SetStaticDesc( const wxString& msg );

	protected:
		void UseDefaultPath_Click( wxCommandEvent &event );
		void Explore_Click( wxCommandEvent &event );
		void UpdateCheckStatus( bool someNoteworthyBoolean );
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class SettingsDirPickerPanel : public DirPickerPanel
	{
	public:
		SettingsDirPickerPanel( wxWindow* parent );
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class BasePathsPanel : public wxPanelWithHelpers
	{
	protected:
		wxBoxSizer& s_main;

	public:
		BasePathsPanel( wxWindow& parent, int idealWidth=wxDefaultCoord );

	protected:
		DirPickerPanel& AddDirPicker( wxBoxSizer& sizer, FoldersEnum_t folderid,
			const wxString& label, const wxString& popupLabel );
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class StandardPathsPanel : public BasePathsPanel
	{
	public:
		StandardPathsPanel( wxWindow& parent, int idealWidth );
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class BaseSelectorPanel: public BaseApplicableConfigPanel
	{
	protected:
		EventListenerBinding<int> m_ReloadSettingsBinding;

	public:
		virtual ~BaseSelectorPanel() throw();
		BaseSelectorPanel( wxWindow& parent, int idealWidth );

		virtual bool Show( bool visible=true );
		virtual void OnRefresh( wxCommandEvent& evt );
		virtual void OnShown();
		virtual void OnFolderChanged( wxFileDirPickerEvent& evt );

	protected:
		virtual void DoRefresh()=0;
		virtual bool ValidateEnumerationStatus()=0;
		virtual void ReloadSettings()=0;

		static void __evt_fastcall OnAppliedSettings( void* me, int& whatever );
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class BiosSelectorPanel : public BaseSelectorPanel
	{
	protected:
		ScopedPtr<wxArrayString>	m_BiosList;
		wxListBox&		m_ComboBox;
		DirPickerPanel&	m_FolderPicker;

	public:
		BiosSelectorPanel( wxWindow& parent, int idealWidth );
		virtual ~BiosSelectorPanel() throw();

	protected:
		virtual void Apply();
		virtual void DoRefresh();
		virtual bool ValidateEnumerationStatus();

		virtual void ReloadSettings();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class PluginSelectorPanel: public BaseSelectorPanel
	{
	protected:
		static const int NumPluginTypes = 7;

		// ------------------------------------------------------------------------
		// PluginSelectorPanel Subclasses
		// ------------------------------------------------------------------------

		class EnumeratedPluginInfo
		{
		public:
			uint PassedTest;		// msk specifying which plugin types passed the mask test.
			uint TypeMask;			// indicates which combo boxes it should be listed in
			wxString Name;			// string to be pasted into the combo box
			wxString Version[NumPluginTypes];

			EnumeratedPluginInfo() :
				PassedTest( 0 )
			,	TypeMask( 0 )
			,	Name()
			{
			}
		};

		class EnumThread : public Threading::PersistentThread
		{
		public:
			SafeList<EnumeratedPluginInfo> Results;		// array of plugin results.

		protected:
			PluginSelectorPanel&	m_master;
			ScopedBusyCursor		m_hourglass;
		public:
			virtual ~EnumThread() throw()
			{
				PersistentThread::Cancel();
			}

			EnumThread( PluginSelectorPanel& master );
			void DoNextPlugin( int evtidx );

		protected:
			void ExecuteTaskInThread();
		};

		// This panel contains all of the plugin combo boxes.  We stick them
		// on a panel together so that we can hide/show the whole mess easily.
		class ComboBoxPanel : public wxPanelWithHelpers
		{
		protected:
			wxComboBox*		m_combobox[NumPluginTypes];
			DirPickerPanel& m_FolderPicker;

		public:
			ComboBoxPanel( PluginSelectorPanel* parent );
			wxComboBox& Get( int i ) { return *m_combobox[i]; }
			wxDirName GetPluginsPath() const { return m_FolderPicker.GetPath(); }
			DirPickerPanel& GetDirPicker() { return m_FolderPicker; }
			void Reset();
		};

		class StatusPanel : public wxPanelWithHelpers
		{
		protected:
			wxGauge&		m_gauge;
			wxStaticText&	m_label;
			int				m_progress;

		public:
			StatusPanel( wxWindow* parent );

			void SetGaugeLength( int len );
			void AdvanceProgress( const wxString& msg );
			void Reset();
		};

	// ------------------------------------------------------------------------
	//  PluginSelectorPanel Members
	// ------------------------------------------------------------------------

	protected:
		StatusPanel&	m_StatusPanel;
		ComboBoxPanel&	m_ComponentBoxes;
		bool			m_Canceled;

		ScopedPtr<wxArrayString>	m_FileList;	// list of potential plugin files
		ScopedPtr<EnumThread>		m_EnumeratorThread;

	public:
		virtual ~PluginSelectorPanel() throw();
		PluginSelectorPanel( wxWindow& parent, int idealWidth );

		void CancelRefresh();		// used from destructor, stays non-virtual
		void Apply();

	protected:
		void OnConfigure_Clicked( wxCommandEvent& evt );
		void OnShowStatusBar( wxCommandEvent& evt );
		virtual void OnProgress( wxCommandEvent& evt );
		virtual void OnEnumComplete( wxCommandEvent& evt );

		virtual void ReloadSettings();

		virtual void DoRefresh();
		virtual bool ValidateEnumerationStatus();

		int FileCount() const { return m_FileList->Count(); }
		const wxString& GetFilename( int i ) const { return (*m_FileList)[i]; }

		friend class EnumThread;
	};
}
