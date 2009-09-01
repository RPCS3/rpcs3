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

// All of the options screens in PCSX2 are implemented as panels which can be bound to
// either their own dialog boxes, or made into children of a paged Properties box.  The
// paged Properties box is generally superior design, and there's a good chance we'll not
// want to deviate form that design anytime soon.  But there's no harm in keeping nice
// encapsulation of panels regardless, just in case we want to shuffle things around. :)

#pragma once

#include <wx/image.h>
#include <wx/statline.h>
#include <wx/bookctrl.h>
#include <wx/listbox.h>

#include <list>

#include "AppConfig.h"
#include "wxHelpers.h"
#include "Utilities/SafeArray.h"
#include "Utilities/Threading.h"

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
	protected:
		Panels::BaseApplicableConfigPanel* m_Panel;

	public:
		virtual ~CannotApplySettings() throw() {}

		explicit CannotApplySettings( Panels::BaseApplicableConfigPanel* thispanel, const char* msg=wxLt("Cannot apply new settings, one of the settings is invalid.") )
		{
			BaseException::InitBaseEx( msg );
			m_Panel = thispanel;
		}

		explicit CannotApplySettings( Panels::BaseApplicableConfigPanel* thispanel, const wxString& msg_eng, const wxString& msg_xlt )
		{
			BaseException::InitBaseEx( msg_eng, msg_xlt );
			m_Panel = thispanel;
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

		// Crappy hack to handle the UseAdminMode option, which can't be part of AppConfig
		// because AppConfig depends on this value to initialize itself.
		bool UseAdminMode;
		
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
		bool ApplyAll();
		bool ApplyPage( int pageid, bool saveOnSuccess=true );
		void DoCleanup();
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
		virtual void Apply( AppConfig& conf )=0;
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

		void Apply( AppConfig& conf );
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

		void Apply( AppConfig& conf );
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
		void Apply( AppConfig& conf );
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class VideoPanel : public BaseApplicableConfigPanel
	{
	protected:

	public:
		VideoPanel( wxWindow& parent, int idealWidth );
		void Apply( AppConfig& conf );
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

	public:
		SpeedHacksPanel( wxWindow& parent, int idealWidth );
		void Apply( AppConfig& conf );

	protected:
		const wxChar* GetEEcycleSliderMsg( int val );
		const wxChar* GetVUcycleSliderMsg( int val );

		void EECycleRate_Scroll(wxScrollEvent &event);
		void VUCycleRate_Scroll(wxScrollEvent &event);
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class GameFixesPanel: public BaseApplicableConfigPanel
	{
	public:
		GameFixesPanel( wxWindow& parent, int idealWidth );
		void Apply( AppConfig& conf );
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

		void Apply( AppConfig& conf );
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
		StandardPathsPanel( wxWindow& parent );
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class BaseSelectorPanel: public BaseApplicableConfigPanel
	{
	protected:
	
	public:
		virtual ~BaseSelectorPanel();
		BaseSelectorPanel( wxWindow& parent, int idealWidth );

		virtual bool Show( bool visible=true );
		virtual void OnRefresh( wxCommandEvent& evt );
		virtual void OnShown();
		virtual void OnFolderChanged( wxFileDirPickerEvent& evt );

	protected:
		virtual void DoRefresh()=0;
		virtual bool ValidateEnumerationStatus()=0;
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class BiosSelectorPanel : public BaseSelectorPanel
	{
	protected:
		wxListBox&		m_ComboBox;
		DirPickerPanel&	m_FolderPicker;
		wxArrayString*	m_BiosList;
		
	public:
		BiosSelectorPanel( wxWindow& parent, int idealWidth );
		virtual ~BiosSelectorPanel();

	protected:
		virtual void Apply( AppConfig& conf );
		virtual void DoRefresh();
		virtual bool ValidateEnumerationStatus();
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
			EnumeratedPluginInfo* Results;		// array of plugin results.

		protected:
			PluginSelectorPanel& m_master;
			volatile bool m_cancel;			

		public:
			virtual ~EnumThread();
			EnumThread( PluginSelectorPanel& master );
			void Cancel();

		protected:
			sptr ExecuteTask();
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
	// PluginSelectorPanel Members
	// ------------------------------------------------------------------------

	protected:
		wxArrayString*	m_FileList;	// list of potential plugin files
		StatusPanel&	m_StatusPanel;
		ComboBoxPanel&	m_ComponentBoxes;
		EnumThread*		m_EnumeratorThread;

	public:
		virtual ~PluginSelectorPanel();
		PluginSelectorPanel( wxWindow& parent, int idealWidth );
		virtual void CancelRefresh();

		void Apply( AppConfig& conf );

	protected:
		void OnConfigure_Clicked( wxCommandEvent& evt );
		virtual void OnProgress( wxCommandEvent& evt );
		virtual void OnEnumComplete( wxCommandEvent& evt );

		virtual void DoRefresh();
		virtual bool ValidateEnumerationStatus();
		
		int FileCount() const { return m_FileList->Count(); }
		const wxString& GetFilename( int i ) const { return (*m_FileList)[i]; }
		friend class EnumThread;
	};
}
