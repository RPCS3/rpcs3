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
#include <wx/spinctrl.h>

#include "BaseConfigPanel.h"

#include "Utilities/Threading.h"

namespace Panels
{
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
	class CpuPanelEE : public BaseApplicableConfigPanel
	{
	protected:
		wxRadioButton* m_Option_RecEE;
		wxRadioButton* m_Option_RecIOP;

	public:
		CpuPanelEE( wxWindow& parent, int idealWidth );
		void Apply();
	};

	class CpuPanelVU : public BaseApplicableConfigPanel
	{
	protected:
		wxRadioButton* m_Option_mVU0;
		wxRadioButton* m_Option_mVU1;
		wxRadioButton* m_Option_sVU0;
		wxRadioButton* m_Option_sVU1;

	public:
		CpuPanelVU( wxWindow& parent, int idealWidth );
		void Apply();
	};

	class BaseAdvancedCpuOptions : public BaseApplicableConfigPanel
	{
	protected:
		wxStaticBoxSizer& s_adv;
		wxStaticBoxSizer& s_round;
		wxStaticBoxSizer& s_clamp;

		wxRadioButton*	m_Option_Round[4];

		wxRadioButton*	m_Option_None;
		wxRadioButton*	m_Option_Normal;

		wxCheckBox*		m_Option_FTZ;
		wxCheckBox*		m_Option_DAZ;

	public:
		BaseAdvancedCpuOptions( wxWindow& parent, int idealWidth );
		virtual ~BaseAdvancedCpuOptions() throw() { }

	protected:
		void OnRestoreDefaults( wxCommandEvent& evt );
		void ApplyRoundmode( SSE_MXCSR& mxcsr );
	};

	class AdvancedOptionsFPU : public BaseAdvancedCpuOptions
	{
	protected:
		wxRadioButton* m_Option_ExtraSign;
		wxRadioButton* m_Option_Full;

	public:
		AdvancedOptionsFPU( wxWindow& parent, int idealWidth );
		virtual ~AdvancedOptionsFPU() throw() { }
		void Apply();
	};

	class AdvancedOptionsVU : public BaseAdvancedCpuOptions
	{
	protected:
		wxRadioButton*	m_Option_Extra;
		wxRadioButton*	m_Option_ExtraSign;

	public:
		AdvancedOptionsVU( wxWindow& parent, int idealWidth );
		virtual ~AdvancedOptionsVU() throw() { }
		void Apply();
	};


	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class FramelimiterPanel : public BaseApplicableConfigPanel
	{
	protected:
		wxCheckBox*		m_check_LimiterDisable;
		wxSpinCtrl*		m_spin_NominalPct;
		wxSpinCtrl*		m_spin_SlomoPct;
		wxSpinCtrl*		m_spin_TurboPct;

		wxTextCtrl*		m_text_BaseNtsc;
		wxTextCtrl*		m_text_BasePal;

		wxCheckBox*		m_SkipperEnable;
		wxCheckBox*		m_TurboSkipEnable;
		wxSpinCtrl*		m_spin_SkipThreshold;

		wxSpinCtrl*		m_spin_FramesToSkip;
		wxSpinCtrl*		m_spin_FramesToDraw;

	public:
		FramelimiterPanel( wxWindow& parent, int idealWidth );
		virtual ~FramelimiterPanel() throw() {}
		void Apply();
	};

	class VideoPanel : public BaseApplicableConfigPanel
	{
	protected:
		wxCheckBox*		m_check_CloseGS;
		//wxCheckBox*		m_check_CloseGS;
		//wxCheckBox*		m_;

	public:
		VideoPanel( wxWindow& parent, int idealWidth );
		virtual ~VideoPanel() throw() {}
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
		DirPickerPanel& SetToolTip( const wxString& tip );

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

