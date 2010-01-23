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

#include "AppCommon.h"
#include "ApplyState.h"


// --------------------------------------------------------------------------------------
//  pxUniformTable
// --------------------------------------------------------------------------------------
// TODO : Move this class to the common Utilities lib, if it proves useful. :)
/*
class pxUniformTable : public wxFlexGridSizer
{
protected:	
	int								m_curcell;
	SafeArray<wxPanelWithHelpers*>	m_panels;

public:
	pxUniformTable( wxWindow* parent, int numcols, int numrows, const wxString* staticBoxLabels=NULL );
	virtual ~pxUniformTable() throw() {}

	int GetCellCount() const
	{
		return m_panels.GetLength();
	}

	wxPanelWithHelpers* operator()( int col, int row )
	{
		return m_panels[(col*GetCols()) + row];
	}

	wxPanelWithHelpers* operator[]( int cellidx )
	{
		return m_panels[cellidx];
	}
};
*/
namespace Panels
{
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class UsermodeSelectionPanel : public BaseApplicableConfigPanel
	{
	protected:
		pxRadioPanel*	m_radio_UserMode;

	public:
		virtual ~UsermodeSelectionPanel() throw() { }
		UsermodeSelectionPanel( wxWindow* parent, bool isFirstTime = true );

		void Apply();
		void AppStatusEvent_OnSettingsApplied();
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
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
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class CpuPanelEE : public BaseApplicableConfigPanel
	{
	protected:
		pxRadioPanel*	m_panel_RecEE;
		pxRadioPanel*	m_panel_RecIOP;

	public:
		CpuPanelEE( wxWindow* parent );
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
	};

	class CpuPanelVU : public BaseApplicableConfigPanel
	{
	protected:
		pxRadioPanel*	m_panel_VU0;
		pxRadioPanel*	m_panel_VU1;

	public:
		CpuPanelVU( wxWindow* parent );
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
	};

	class BaseAdvancedCpuOptions : public BaseApplicableConfigPanel
	{
	protected:
		//wxStaticBoxSizer& s_round;
		//wxStaticBoxSizer& s_clamp;

		pxRadioPanel*	m_RoundModePanel;
		pxRadioPanel*	m_ClampModePanel;

		pxCheckBox*		m_Option_FTZ;
		pxCheckBox*		m_Option_DAZ;

	public:
		BaseAdvancedCpuOptions( wxWindow* parent );
		virtual ~BaseAdvancedCpuOptions() throw() { }

	protected:
		void OnRestoreDefaults( wxCommandEvent& evt );
		void ApplyRoundmode( SSE_MXCSR& mxcsr );
	};

	class AdvancedOptionsFPU : public BaseAdvancedCpuOptions
	{
	public:
		AdvancedOptionsFPU( wxWindow* parent );
		virtual ~AdvancedOptionsFPU() throw() { }
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
	};

	class AdvancedOptionsVU : public BaseAdvancedCpuOptions
	{
	public:
		AdvancedOptionsVU( wxWindow* parent );
		virtual ~AdvancedOptionsVU() throw() { }
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
	};

	// --------------------------------------------------------------------------------------
	//  FrameSkipPanel
	// --------------------------------------------------------------------------------------
	class FrameSkipPanel : public BaseApplicableConfigPanel
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
	};
	
	// --------------------------------------------------------------------------------------
	//  FramelimiterPanel
	// --------------------------------------------------------------------------------------
	class FramelimiterPanel : public BaseApplicableConfigPanel
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
	};
	
	// --------------------------------------------------------------------------------------
	//  GSWindowSettingsPanel
	// --------------------------------------------------------------------------------------
	class GSWindowSettingsPanel : public BaseApplicableConfigPanel
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
	};

	class VideoPanel : public BaseApplicableConfigPanel
	{
	protected:
		pxCheckBox*		m_check_SynchronousGS;
		pxCheckBox*		m_check_DisableOutput;

		wxButton*		m_button_OpenWindowSettings;

	public:
		VideoPanel( wxWindow* parent );
		virtual ~VideoPanel() throw() {}
		void Apply();
		void AppStatusEvent_OnSettingsApplied();

	protected:
		void OnOpenWindowSettings( wxCommandEvent& evt );
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class SpeedHacksPanel : public BaseApplicableConfigPanel
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
		pxCheckBox*		m_check_b1fc0;
		pxCheckBox*		m_check_IOPx2;
		pxCheckBox*		m_check_vuFlagHack;
		pxCheckBox*		m_check_vuMinMax;

	public:
		SpeedHacksPanel( wxWindow* parent );
		void Apply();
		void EnableStuff();
		void AppStatusEvent_OnSettingsApplied();
		void AppStatusEvent_OnSettingsApplied( const Pcsx2Config::SpeedhackOptions& opt );

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

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class GameFixesPanel: public BaseApplicableConfigPanel
	{
	protected:
		pxCheckBox*			m_checkbox[NUM_OF_GAME_FIXES];

	public:
		GameFixesPanel( wxWindow* parent );
		void Apply();
		void AppStatusEvent_OnSettingsApplied();
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
		pxCheckBox*			m_checkCtrl;

	public:
		DirPickerPanel( wxWindow* parent, FoldersEnum_t folderid, const wxString& label, const wxString& dialogLabel );
		virtual ~DirPickerPanel() throw() { }

		void Apply();
		void AppStatusEvent_OnSettingsApplied();

		void Reset();
		wxDirName GetPath() const;

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
	public:
		BasePathsPanel( wxWindow* parent );

	protected:
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class StandardPathsPanel : public BasePathsPanel
	{
	public:
		StandardPathsPanel( wxWindow* parent );
	};

	// --------------------------------------------------------------------------------------
	//  BaseSelectorPanel
	// --------------------------------------------------------------------------------------
	class BaseSelectorPanel: public BaseApplicableConfigPanel
	{
	public:
		virtual ~BaseSelectorPanel() throw();
		BaseSelectorPanel( wxWindow* parent );

		virtual bool Show( bool visible=true );
		virtual void OnRefresh( wxCommandEvent& evt );
		virtual void OnShown();
		virtual void OnFolderChanged( wxFileDirPickerEvent& evt );

	protected:
		virtual void DoRefresh()=0;
		virtual bool ValidateEnumerationStatus()=0;
	};

	// --------------------------------------------------------------------------------------
	//  BiosSelectorPanel
	// --------------------------------------------------------------------------------------
	class BiosSelectorPanel : public BaseSelectorPanel
	{
	protected:
		ScopedPtr<wxArrayString>	m_BiosList;
		wxListBox&					m_ComboBox;
		DirPickerPanel&				m_FolderPicker;

	public:
		BiosSelectorPanel( wxWindow* parent, int idealWidth=wxDefaultCoord );
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
		PluginSelectorPanel( wxWindow* parent, int idealWidth=wxDefaultCoord );

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

