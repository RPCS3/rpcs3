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

#include "wxHelpers.h"
#include "Utilities/SafeArray.h"
#include "Utilities/Threading.h"

namespace Exception
{
	// --------------------------------------------------------------------------
	// Exception used to perform an abort of Apply/Ok action on settings panels.
	// When thrown, the user recieves a popup message containing the information
	// specified in the exception message, and is returned to the settings dialog
	// to correct the invalid input fields.
	//
	class CannotApplySettings : public BaseException
	{
	public:
		virtual ~CannotApplySettings() throw() {}
		CannotApplySettings( const CannotApplySettings& src ) : BaseException( src ) {}

		explicit CannotApplySettings( const char* msg=wxLt("Cannot apply new settings, one of the settings is invalid.") ) :
			BaseException( msg ) {}

		explicit CannotApplySettings( const wxString& msg_eng, const wxString& msg_xlt ) :
			BaseException( msg_eng, msg_xlt ) { }
	};
}

namespace Panels
{
	//////////////////////////////////////////////////////////////////////////////////////////
	// Extends the Panel class to add an Apply() method, which is invoked from the parent
	// window (usually the ConfigurationDialog) when either Ok or Apply is clicked.
	//
	class BaseApplicableConfigPanel : public wxPanelWithHelpers
	{
	public:
		virtual ~BaseApplicableConfigPanel() { }
		BaseApplicableConfigPanel( wxWindow* parent ) : 
			wxPanelWithHelpers( parent, wxID_ANY ) { }
	
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
		UsermodeSelectionPanel( wxWindow* parent );
		
		void Apply( AppConfig& conf );
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class SpeedHacksPanel : public BaseApplicableConfigPanel
	{
	public:
		SpeedHacksPanel(wxWindow& parent);
		void Apply( AppConfig& conf );

	protected:
		void IOPCycleDouble_Click(wxCommandEvent &event);
		void WaitCycleExt_Click(wxCommandEvent &event);
		void INTCSTATSlow_Click(wxCommandEvent &event);
		void IdleLoopFF_Click(wxCommandEvent &event);
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class GameFixesPanel: public BaseApplicableConfigPanel
	{
	public:
		GameFixesPanel(wxWindow& parent);
		void Apply( AppConfig& conf );
		
		void FPUCompareHack_Click(wxCommandEvent &event);
		void FPUMultHack_Click(wxCommandEvent &event);
		void TriAce_Click(wxCommandEvent &event);
		void GodWar_Click(wxCommandEvent &event);
		void Ok_Click(wxCommandEvent &event);
		void Cancel_Click(wxCommandEvent &event);
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class PathsPanel : public BaseApplicableConfigPanel
	{
	protected:
		class DirPickerPanel : public wxPanelWithHelpers
		{
		protected:
			wxDirName (*m_GetDefaultFunc)();
			wxDirPickerCtrl* m_pickerCtrl;
			wxCheckBox* m_checkCtrl;

		public:
			DirPickerPanel( wxWindow* parent, const wxDirName& initPath, wxDirName (*getDefault)(), const wxString& label, const wxString& dialogLabel );

		protected:
			void UseDefaultPath_Click(wxCommandEvent &event);
		};

		class MyBasePanel : public wxPanelWithHelpers
		{
		protected:
			wxBoxSizer& s_main;

		public:
			MyBasePanel(wxWindow& parent, int id=wxID_ANY);

		protected:
			void AddDirPicker( wxBoxSizer& sizer, const wxDirName& initPath, wxDirName (*getDefaultFunc)(),
				const wxString& label, const wxString& popupLabel, enum ExpandedMsgEnum tooltip );
		};

		class StandardPanel : public MyBasePanel
		{
		public:
			StandardPanel(wxWindow& parent, int id=wxID_ANY);
		};

		class AdvancedPanel : public MyBasePanel
		{
		public:
			AdvancedPanel(wxWindow& parent, int id=wxID_ANY);
		};

	public:
		PathsPanel(wxWindow& parent);
		void Apply( AppConfig& conf );
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class PluginSelectorPanel: public BaseApplicableConfigPanel
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
	
		class EnumThread : public Threading::Thread
		{
		public:
			EnumeratedPluginInfo* Results;		// array of plugin results.

		protected:
			PluginSelectorPanel& m_master;
			volatile bool m_cancel;			
		public:
			virtual ~EnumThread();
			EnumThread( PluginSelectorPanel& master );
			void Close();

		protected:
			int Callback();
		};
		
		// This panel contains all of the plugin combo boxes.  We stick them
		// on a panel together so that we can hide/show the whole mess easily.
		class ComboBoxPanel : public wxPanelWithHelpers
		{
		protected:
			wxComboBox* m_combobox[NumPluginTypes];

		public:
			ComboBoxPanel( PluginSelectorPanel* parent );
			wxComboBox& Get( int i ) { return *m_combobox[i]; }
			void Reset();
		};

		class StatusPanel : public wxPanelWithHelpers
		{
		protected:
			wxGauge&		m_gauge;
			wxStaticText&	m_label;
			int				m_progress;
			
		public:
			StatusPanel( wxWindow* parent, int pluginCount );
			void AdvanceProgress( const wxString& msg );
			void Reset();
		};

	// ------------------------------------------------------------------------
	// PluginSelectorPanel Members
	// ------------------------------------------------------------------------

	protected:
		wxArrayString	m_FileList;	// list of potential plugin files
		StatusPanel&	m_StatusPanel;
		ComboBoxPanel&	m_ComboBoxes;
		bool			m_Uninitialized;
		EnumThread*		m_EnumeratorThread;

	public:
		virtual ~PluginSelectorPanel();
		PluginSelectorPanel(wxWindow& parent);
		virtual void OnShow( wxShowEvent& evt );
		virtual void OnRefresh( wxCommandEvent& evt );
		virtual void OnProgress( wxCommandEvent& evt );
		virtual void OnEnumComplete( wxCommandEvent& evt );
		
		void Apply( AppConfig& conf );
	
	protected:
		void DoRefresh();
		int FileCount() const { return m_FileList.Count(); }
		const wxString& GetFilename( int i ) const { return m_FileList[i]; }
		friend class EnumThread;
	};
}
