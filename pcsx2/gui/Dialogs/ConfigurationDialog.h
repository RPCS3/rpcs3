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

#pragma once

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/propdlg.h>

#include "AppCommon.h"
#include "ApplyState.h"

namespace Dialogs
{
	class BaseApplicableDialog : public wxDialogWithHelpers, public IApplyState
	{
		DECLARE_DYNAMIC_CLASS_NO_COPY(BaseApplicableDialog)

	public:
		BaseApplicableDialog() {}

	protected:
		BaseApplicableDialog(wxWindow* parent, const wxString& title );
		BaseApplicableDialog(wxWindow* parent, const wxString& title, wxOrientation sizerOrient );

	public:
		virtual ~BaseApplicableDialog() throw();
		//ApplyStateStruct& GetApplyState() { return m_ApplyState; }
	};

	// --------------------------------------------------------------------------------------
	//  BaseConfigurationDialog
	// --------------------------------------------------------------------------------------
	class BaseConfigurationDialog : public BaseApplicableDialog
	{
	protected:
		wxListbook&			m_listbook;
		wxArrayString		m_labels;

	public:
		virtual ~BaseConfigurationDialog() throw();
		BaseConfigurationDialog(wxWindow* parent, const wxString& title, wxImageList& bookicons, int idealWidth);

	protected:
		template< typename T >
		void AddPage( const char* label, int iconid );

		void OnOk_Click( wxCommandEvent& evt );
		void OnCancel_Click( wxCommandEvent& evt );
		void OnApply_Click( wxCommandEvent& evt );
		void OnScreenshot_Click( wxCommandEvent& evt );
		void OnCloseWindow( wxCloseEvent& evt );

		virtual void OnSomethingChanged( wxCommandEvent& evt );
		virtual wxString& GetConfSettingsTabName() const=0;
	};

	// --------------------------------------------------------------------------------------
	//  SysConfigDialog
	// --------------------------------------------------------------------------------------
	class SysConfigDialog : public BaseConfigurationDialog
	{
	protected:

	public:
		virtual ~SysConfigDialog() throw() {}
		SysConfigDialog(wxWindow* parent=NULL);
		static const wxChar* GetNameStatic() { return L"Dialog:CoreSettings"; }

	protected:
		virtual wxString& GetConfSettingsTabName() const { return g_Conf->SysSettingsTabName; }
	};

	// --------------------------------------------------------------------------------------
	//  AppConfigDialog
	// --------------------------------------------------------------------------------------
	class AppConfigDialog : public BaseConfigurationDialog
	{
	protected:

	public:
		virtual ~AppConfigDialog() throw() {}
		AppConfigDialog(wxWindow* parent=NULL);
		static const wxChar* GetNameStatic() { return L"Dialog:AppSettings"; }

	protected:
		virtual wxString& GetConfSettingsTabName() const { return g_Conf->AppSettingsTabName; }
	};

	// --------------------------------------------------------------------------------------
	//  BiosSelectorDialog
	// --------------------------------------------------------------------------------------
	class BiosSelectorDialog : public BaseApplicableDialog
	{
	protected:

	public:
		virtual ~BiosSelectorDialog()  throw() {}
		BiosSelectorDialog( wxWindow* parent=NULL );

		static const wxChar* GetNameStatic() { return L"Dialog:BiosSelector"; }

	protected:
		void OnOk_Click( wxCommandEvent& evt );
		void OnDoubleClicked( wxCommandEvent& evt );
	};

	// --------------------------------------------------------------------------------------
	//  CreateMemoryCardDialog
	// --------------------------------------------------------------------------------------
	class CreateMemoryCardDialog : public BaseApplicableDialog
	{
	protected:
		wxFilePickerCtrl*	m_filepicker;
		pxRadioPanel*		m_radio_CardSize;

	#ifdef __WXMSW__
		pxCheckBox*			m_check_CompressNTFS;
	#endif

	public:
		virtual ~CreateMemoryCardDialog()  throw() {}
		CreateMemoryCardDialog( wxWindow* parent, uint port, uint slot, const wxString& filepath=wxEmptyString );

		static const wxChar* GetNameStatic() { return L"Dialog:CreateMemoryCard"; }

	protected:
		void OnOk_Click( wxCommandEvent& evt );
		void OnDoubleClicked( wxCommandEvent& evt );
	};
}
