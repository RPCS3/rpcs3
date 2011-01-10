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

// All of the options screens in PCSX2 are implemented as panels which can be bound to
// either their own dialog boxes, or made into children of a paged Properties box.  The
// paged Properties box is generally superior design, and there's a good chance we'll not
// want to deviate form that design anytime soon.  But there's no harm in keeping nice
// encapsulation of panels regardless, just in case we want to shuffle things around. :)

#pragma once

#include <wx/statline.h>
#include <wx/dnd.h>

#include "AppCommon.h"
#include "ApplyState.h"


namespace Panels
{
	// --------------------------------------------------------------------------------------
	//  DirPickerPanel
	// --------------------------------------------------------------------------------------
	// A simple panel which provides a specialized configurable directory picker with a
	// "[x] Use Default setting" option, which enables or disables the panel.
	//
	class DirPickerPanel : public BaseApplicableConfigPanel
	{
		typedef BaseApplicableConfigPanel _parent;

	protected:
		FoldersEnum_t		m_FolderId;
		wxDirPickerCtrl*	m_pickerCtrl;
		pxCheckBox*			m_checkCtrl;

	public:
		DirPickerPanel( wxWindow* parent, FoldersEnum_t folderid, const wxString& label, const wxString& dialogLabel );
		DirPickerPanel( wxWindow* parent, FoldersEnum_t folderid, const wxString& dialogLabel );
		virtual ~DirPickerPanel() throw() { }

		void Reset();
		wxDirName GetPath() const;
		void SetPath( const wxString& src );

		DirPickerPanel& SetStaticDesc( const wxString& msg );
		DirPickerPanel& SetToolTip( const wxString& tip );

		wxWindowID GetId() const;
		wxWindowID GetPanelId() const { return m_windowId; }

		// Overrides!
		
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
		bool Enable( bool enable=true );

	protected:
		void Init( FoldersEnum_t folderid, const wxString& dialogLabel, bool isCompact );

		void UseDefaultPath_Click( wxCommandEvent &event );
		void Explore_Click( wxCommandEvent &event );
		void UpdateCheckStatus( bool someNoteworthyBoolean );
	};

	// --------------------------------------------------------------------------------------
	//  DocsFolderPickerPanel / LanguageSelectionPanel
	// --------------------------------------------------------------------------------------
	class DocsFolderPickerPanel : public BaseApplicableConfigPanel
	{
	protected:
		pxRadioPanel*	m_radio_UserMode;
		DirPickerPanel*	m_dirpicker_custom;

	public:
		virtual ~DocsFolderPickerPanel() throw() { }
		DocsFolderPickerPanel( wxWindow* parent, bool isFirstTime = true );

		void Apply();
		void AppStatusEvent_OnSettingsApplied();
		
		DocsModeType GetDocsMode() const;
		wxWindowID GetDirPickerId() const { return m_dirpicker_custom ? m_dirpicker_custom->GetId() : 0; }

	protected:
		void OnRadioChanged( wxCommandEvent& evt );
	};

	class LanguageSelectionPanel : public BaseApplicableConfigPanel
	{
	protected:
		LangPackList	m_langs;
		wxComboBox*		m_picker;

	public:
		virtual ~LanguageSelectionPanel() throw() { }
		LanguageSelectionPanel( wxWindow* parent );

		void Apply();
		void AppStatusEvent_OnSettingsApplied();

	protected:
		void OnApplyLanguage_Clicked( wxCommandEvent& evt );
	};

	// --------------------------------------------------------------------------------------
	//  CpuPanelEE / CpuPanelVU : Sub Panels
	// --------------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------------
	//  BaseAdvancedCpuOptions
	// --------------------------------------------------------------------------------------
	class BaseAdvancedCpuOptions : public BaseApplicableConfigPanel_SpecificConfig
	{

	protected:
		pxRadioPanel*	m_RoundModePanel;
		pxRadioPanel*	m_ClampModePanel;

		pxCheckBox*		m_Option_FTZ;
		pxCheckBox*		m_Option_DAZ;

	public:
		BaseAdvancedCpuOptions( wxWindow* parent );
		virtual ~BaseAdvancedCpuOptions() throw() { }

		void RestoreDefaults();

	protected:
		void OnRestoreDefaults( wxCommandEvent& evt );
		void ApplyRoundmode( SSE_MXCSR& mxcsr );
	};

	// --------------------------------------------------------------------------------------
	//  AdvancedOptionsFPU / AdvancedOptionsVU
	// --------------------------------------------------------------------------------------
	class AdvancedOptionsFPU : public BaseAdvancedCpuOptions
	{
	public:
		AdvancedOptionsFPU( wxWindow* parent );
		virtual ~AdvancedOptionsFPU() throw() { }
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
        void ApplyConfigToGui( AppConfig& configToApply, bool manuallyPropagate=false );
	};

	class AdvancedOptionsVU : public BaseAdvancedCpuOptions
	{
	public:
		AdvancedOptionsVU( wxWindow* parent );
		virtual ~AdvancedOptionsVU() throw() { }
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
        void ApplyConfigToGui( AppConfig& configToApply, bool manuallyPropagate=false );

	};

    // --------------------------------------------------------------------------------------
	//  CpuPanelEE / CpuPanelVU : Actual Panels
	// --------------------------------------------------------------------------------------
	class CpuPanelEE : public BaseApplicableConfigPanel_SpecificConfig
	{
	protected:
		pxRadioPanel*	    m_panel_RecEE;
		pxRadioPanel*	    m_panel_RecIOP;
        AdvancedOptionsFPU* m_advancedOptsFpu;

	public:
		CpuPanelEE( wxWindow* parent );
		virtual ~CpuPanelEE() throw() {}

		void Apply();
		void AppStatusEvent_OnSettingsApplied();
        void ApplyConfigToGui(AppConfig& configToApply, bool manuallyPropagate=false);

	protected:
		void OnRestoreDefaults( wxCommandEvent& evt );
	};

	class CpuPanelVU : public BaseApplicableConfigPanel_SpecificConfig
	{
	protected:
		pxRadioPanel*	    m_panel_VU0;
		pxRadioPanel*	    m_panel_VU1;
        Panels::AdvancedOptionsVU*  m_advancedOptsVu;

	public:
		CpuPanelVU( wxWindow* parent );
		virtual ~CpuPanelVU() throw() {}

		void Apply();
		void AppStatusEvent_OnSettingsApplied();
        void ApplyConfigToGui( AppConfig& configToApply, bool manuallyPropagate=false );

	protected:
		void OnRestoreDefaults( wxCommandEvent& evt );
	};

	// --------------------------------------------------------------------------------------
	//  FrameSkipPanel
	// --------------------------------------------------------------------------------------
	class FrameSkipPanel : public BaseApplicableConfigPanel_SpecificConfig
	{
	protected:
		wxSpinCtrl*		m_spin_FramesToSkip;
		wxSpinCtrl*		m_spin_FramesToDraw;
		//pxCheckBox*		m_check_EnableSkip;
		//pxCheckBox*		m_check_EnableSkipOnTurbo;

		pxRadioPanel*	m_radio_SkipMode;

	public:
		FrameSkipPanel( wxWindow* parent );
		virtual	~FrameSkipPanel() throw() {}

		void Apply();
		void AppStatusEvent_OnSettingsApplied();
        void ApplyConfigToGui( AppConfig& configToApply, bool manuallyPropagate=false );
	};

	// --------------------------------------------------------------------------------------
	//  FramelimiterPanel
	// --------------------------------------------------------------------------------------
	class FramelimiterPanel : public BaseApplicableConfigPanel_SpecificConfig
	{
	protected:
		pxCheckBox*		m_check_LimiterDisable;
		wxSpinCtrl*		m_spin_NominalPct;
		wxSpinCtrl*		m_spin_SlomoPct;
		wxSpinCtrl*		m_spin_TurboPct;

		wxTextCtrl*		m_text_BaseNtsc;
		wxTextCtrl*		m_text_BasePal;

		wxCheckBox*		m_SkipperEnable;
		wxCheckBox*		m_TurboSkipEnable;
		wxSpinCtrl*		m_spin_SkipThreshold;

	public:
		FramelimiterPanel( wxWindow* parent );
		virtual ~FramelimiterPanel() throw() {}

		void Apply();
		void AppStatusEvent_OnSettingsApplied();
        void ApplyConfigToGui( AppConfig& configToApply, bool manuallyPropagate=false );
	};

	// --------------------------------------------------------------------------------------
	//  GSWindowSettingsPanel
	// --------------------------------------------------------------------------------------
	class GSWindowSettingsPanel : public BaseApplicableConfigPanel_SpecificConfig
	{
	protected:
		wxComboBox*		m_combo_AspectRatio;

		pxCheckBox*		m_check_CloseGS;
		pxCheckBox*		m_check_SizeLock;
		pxCheckBox*		m_check_VsyncEnable;
		pxCheckBox*		m_check_Fullscreen;
		pxCheckBox*		m_check_ExclusiveFS;
		pxCheckBox*		m_check_HideMouse;

		wxTextCtrl*		m_text_WindowWidth;
		wxTextCtrl*		m_text_WindowHeight;

	public:
		GSWindowSettingsPanel( wxWindow* parent );
		virtual ~GSWindowSettingsPanel() throw() {}
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
        void ApplyConfigToGui( AppConfig& configToApply, bool manuallyPropagate=false );
	};

	class VideoPanel : public BaseApplicableConfigPanel_SpecificConfig
	{
	protected:
		pxCheckBox*		    m_check_SynchronousGS;
		pxCheckBox*		    m_check_DisableOutput;
        FrameSkipPanel*     m_span;
	    FramelimiterPanel*  m_fpan;

	public:
		VideoPanel( wxWindow* parent );
		virtual ~VideoPanel() throw() {}
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
        void ApplyConfigToGui( AppConfig& configToApply, bool manuallyPropagate=false );

	protected:
		void OnOpenWindowSettings( wxCommandEvent& evt );
	};

	// --------------------------------------------------------------------------------------
	//  SpeedHacksPanel
	// --------------------------------------------------------------------------------------
	class SpeedHacksPanel : public BaseApplicableConfigPanel_SpecificConfig
	{
	protected:
		wxFlexGridSizer* s_table;

		pxCheckBox*		m_check_Enable;
		wxButton*		m_button_Defaults;

		wxSlider*		m_slider_eecycle;
		wxSlider*		m_slider_vustealer;
		pxStaticText*	m_msg_eecycle;
		pxStaticText*	m_msg_vustealer;

		pxCheckBox*		m_check_intc;
		pxCheckBox*		m_check_waitloop;
		pxCheckBox*		m_check_fastCDVD;
		pxCheckBox*		m_check_vuFlagHack;
		pxCheckBox*		m_check_vuBlockHack;
		pxCheckBox*		m_check_vuMinMax;

	public:
		virtual ~SpeedHacksPanel() throw() {}
		SpeedHacksPanel( wxWindow* parent );
		void Apply();
		void EnableStuff( AppConfig* configToUse=NULL );
		void AppStatusEvent_OnSettingsApplied();
		void ApplyConfigToGui( AppConfig& configToApply, bool manuallyPropagate=false );

	protected:
		const wxChar* GetEEcycleSliderMsg( int val );
		const wxChar* GetVUcycleSliderMsg( int val );
		void SetEEcycleSliderMsg();
		void SetVUcycleSliderMsg();

		void OnEnable_Toggled( wxCommandEvent& evt );
		void Defaults_Click( wxCommandEvent& evt );
		void Slider_Click(wxScrollEvent &event);
		void EECycleRate_Scroll(wxScrollEvent &event);
		void VUCycleRate_Scroll(wxScrollEvent &event);
	};

	// --------------------------------------------------------------------------------------
	//  GameFixesPanel
	// --------------------------------------------------------------------------------------
	class GameFixesPanel : public BaseApplicableConfigPanel_SpecificConfig
	{
	protected:
		pxCheckBox*			m_checkbox[GamefixId_COUNT];
		pxCheckBox*			m_check_Enable;

	public:
		GameFixesPanel( wxWindow* parent );
		virtual ~GameFixesPanel() throw() { }
		void EnableStuff( AppConfig* configToUse=NULL );
		void OnEnable_Toggled( wxCommandEvent& evt );
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
        void ApplyConfigToGui( AppConfig& configToApply, bool manuallyPropagate=false );
	};

	// --------------------------------------------------------------------------------------
	//  GameDatabasePanel
	// --------------------------------------------------------------------------------------
	class GameDatabasePanel : public BaseApplicableConfigPanel
	{
	protected:
		//wxTextCtrl*	searchBox;
		//wxComboBox*	searchType;
		//wxListBox*	searchList;
		wxButton*	searchBtn;
		wxTextCtrl*	serialBox;
		wxTextCtrl*	nameBox;
		wxTextCtrl*	regionBox;
		wxTextCtrl*	compatBox;
		wxTextCtrl*	commentBox;
		wxTextCtrl*	patchesBox;
		pxCheckBox*	gameFixes[GamefixId_COUNT];

	public:
		GameDatabasePanel( wxWindow* parent );
		virtual ~GameDatabasePanel() throw() { }
		void Apply();
		void AppStatusEvent_OnSettingsApplied();

	protected:
		void PopulateFields( const wxString& serial=wxEmptyString );
		bool WriteFieldsToDB();
		void Search_Click( wxCommandEvent& evt );
	};

	class SettingsDirPickerPanel : public DirPickerPanel
	{
	public:
		SettingsDirPickerPanel( wxWindow* parent );
	};

	// --------------------------------------------------------------------------------------
	//  BasePathsPanel / StandardPathsPanel
	// --------------------------------------------------------------------------------------
	class BasePathsPanel : public wxPanelWithHelpers
	{
	public:
		BasePathsPanel( wxWindow* parent );

	protected:
	};

	class StandardPathsPanel : public BasePathsPanel
	{
	public:
		StandardPathsPanel( wxWindow* parent );
	};

	// --------------------------------------------------------------------------------------
	//  AppearanceThemesPanel
	// --------------------------------------------------------------------------------------
	class AppearanceThemesPanel : public BaseApplicableConfigPanel
	{
		typedef BaseApplicableConfigPanel _parent;

	public:
		virtual ~AppearanceThemesPanel() throw();
		AppearanceThemesPanel( wxWindow* parent );

		void Apply();
		void AppStatusEvent_OnSettingsApplied();
	};

	// --------------------------------------------------------------------------------------
	//  BaseSelectorPanel
	// --------------------------------------------------------------------------------------
	class BaseSelectorPanel: public BaseApplicableConfigPanel
	{
		typedef BaseApplicableConfigPanel _parent;

	public:
		virtual ~BaseSelectorPanel() throw();
		BaseSelectorPanel( wxWindow* parent );

		virtual void RefreshSelections();

		virtual bool Show( bool visible=true );
		virtual void OnShown();
		virtual void OnFolderChanged( wxFileDirPickerEvent& evt );

	protected:
		void OnRefreshSelections( wxCommandEvent& evt );

		// This method is called when the enumeration contents have changed.  The implementing
		// class should populate or re-populate listbox/selection components when invoked.
		// 
		virtual void DoRefresh()=0;

		// This method is called when an event has indicated that the enumeration status of the
		// selector may have changed.  The implementing class should re-enumerate the folder/source
		// data and return either TRUE (enumeration status unchanged) or FALSE (enumeration status
		// changed).
		//
		// If the implementation returns FALSE, then the BaseSelectorPanel will invoke a call to
		// DoRefresh() [which also must be implemented]
		//
		virtual bool ValidateEnumerationStatus()=0;
	
		void OnShow(wxShowEvent& evt);
	};

	// --------------------------------------------------------------------------------------
	//  ThemeSelectorPanel
	// --------------------------------------------------------------------------------------
	class ThemeSelectorPanel : public BaseSelectorPanel
	{
		typedef BaseSelectorPanel _parent;

	protected:
		ScopedPtr<wxArrayString>	m_ThemeList;
		wxListBox*					m_ComboBox;
		DirPickerPanel*				m_FolderPicker;

	public:
		virtual ~ThemeSelectorPanel() throw();
		ThemeSelectorPanel( wxWindow* parent );

	protected:
		virtual void Apply();
		virtual void AppStatusEvent_OnSettingsApplied();
		virtual void DoRefresh();
		virtual bool ValidateEnumerationStatus();	
	};

	// --------------------------------------------------------------------------------------
	//  BiosSelectorPanel
	// --------------------------------------------------------------------------------------
	class BiosSelectorPanel : public BaseSelectorPanel
	{
	protected:
		ScopedPtr<wxArrayString>	m_BiosList;
		wxListBox*					m_ComboBox;
		DirPickerPanel*				m_FolderPicker;

	public:
		BiosSelectorPanel( wxWindow* parent );
		virtual ~BiosSelectorPanel() throw();

	protected:
		virtual void Apply();
		virtual void AppStatusEvent_OnSettingsApplied();
		virtual void DoRefresh();
		virtual bool ValidateEnumerationStatus();
	};

	// --------------------------------------------------------------------------------------
	//  PluginSelectorPanel
	// --------------------------------------------------------------------------------------
	class PluginSelectorPanel: public BaseSelectorPanel,
		public EventListener_Plugins
	{
	protected:
		// ----------------------------------------------------------------------------
		class EnumeratedPluginInfo
		{
		public:
			uint PassedTest;		// msk specifying which plugin types passed the mask test.
			uint TypeMask;			// indicates which combo boxes it should be listed in
			wxString Name;			// string to be pasted into the combo box
			wxString Version[PluginId_Count];

			EnumeratedPluginInfo()
			{
				PassedTest	= 0;
				TypeMask	= 0;
			}
		};

		// ----------------------------------------------------------------------------
		class EnumThread : public Threading::pxThread
		{
		public:
			SafeList<EnumeratedPluginInfo> Results;		// array of plugin results.

		protected:
			PluginSelectorPanel&	m_master;
			ScopedBusyCursor		m_hourglass;
		public:
			virtual ~EnumThread() throw()
			{
				pxThread::Cancel();
			}

			EnumThread( PluginSelectorPanel& master );
			void DoNextPlugin( int evtidx );

		protected:
			void ExecuteTaskInThread();
		};

		// ----------------------------------------------------------------------------
		// This panel contains all of the plugin combo boxes.  We stick them
		// on a panel together so that we can hide/show the whole mess easily.
		class ComboBoxPanel : public wxPanelWithHelpers
		{
		protected:
			wxComboBox*		m_combobox[PluginId_Count];
			wxButton*		m_configbutton[PluginId_Count];
			DirPickerPanel& m_FolderPicker;

		public:
			ComboBoxPanel( PluginSelectorPanel* parent );
			wxComboBox& Get( PluginsEnum_t pid ) { return *m_combobox[pid]; }
			wxButton& GetConfigButton( PluginsEnum_t pid ) { return *m_configbutton[pid]; }
			wxDirName GetPluginsPath() const { return m_FolderPicker.GetPath(); }
			DirPickerPanel& GetDirPicker() { return m_FolderPicker; }
			void Reset();

		};

		// ----------------------------------------------------------------------------
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

	protected:
		StatusPanel*	m_StatusPanel;
		ComboBoxPanel*	m_ComponentBoxes;
		bool			m_Canceled;

		ScopedPtr<wxArrayString>	m_FileList;	// list of potential plugin files
		ScopedPtr<EnumThread>		m_EnumeratorThread;

	public:
		virtual ~PluginSelectorPanel() throw();
		PluginSelectorPanel( wxWindow* parent );

		void CancelRefresh();		// used from destructor, stays non-virtual
		void Apply();

	protected:
		void DispatchEvent( const PluginEventType& evt );

		void OnConfigure_Clicked( wxCommandEvent& evt );
		void OnShowStatusBar( wxCommandEvent& evt );
		void OnPluginSelected( wxCommandEvent& evt );

		virtual void OnProgress( wxCommandEvent& evt );
		virtual void OnEnumComplete( wxCommandEvent& evt );

		virtual void AppStatusEvent_OnSettingsApplied();

		virtual void DoRefresh();
		virtual bool ValidateEnumerationStatus();

		int FileCount() const { return m_FileList->Count(); }
		const wxString& GetFilename( int i ) const { return (*m_FileList)[i]; }

		friend class EnumThread;
	};
}

