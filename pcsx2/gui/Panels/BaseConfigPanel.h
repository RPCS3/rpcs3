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

#include <list>

#include "AppConfig.h"
#include "wxHelpers.h"

#include "Utilities/SafeArray.h"
#include "Utilities/EventSource.h"
#include "Utilities/wxGuiTools.h"

class wxListBox;
class wxBookCtrlBase;

// Annoyances of C++ forward declarations.  Having to type in this red tape mess
// is keeping me from eating a good sandwich right now... >_<
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

		StaticApplyState()
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
		bool ApplyAll( bool saveOnSuccess=true );
		bool ApplyPage( int pageid, bool saveOnSuccess=true );
		void DoCleanup() throw();
	};

	extern StaticApplyState g_ApplyState;

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
	class BaseApplicableConfigPanel : public wxPanelWithHelpers
	{
	protected:
		int				m_OwnerPage;
		wxBookCtrlBase*	m_OwnerBook;

	public:
		virtual ~BaseApplicableConfigPanel()
		{
			g_ApplyState.PanelList.remove( this );
		}

		BaseApplicableConfigPanel( wxWindow* parent, int idealWidth )
			: wxPanelWithHelpers( parent, idealWidth )
		{
			m_OwnerPage = g_ApplyState.CurOwnerPage;
			m_OwnerBook = g_ApplyState.ParentBook;

			g_ApplyState.PanelList.push_back( this );
		}

		int GetOwnerPage() const { return m_OwnerPage; }
		wxBookCtrlBase* GetOwnerBook() { return m_OwnerBook; }

		void SetFocusToMe();

		// Returns true if this ConfigPanel belongs to the specified page.  Useful for doing
		// selective application of options for specific pages.
		bool IsOnPage( int pageid ) { return m_OwnerPage == pageid; }

		// This method attempts to assign the settings for the panel into the given
		// configuration structure (which is typically a copy of g_Conf).  If validation
		// of form contents fails, the function should throw Exception::CannotApplySettings.
		// If no exceptions are thrown, then the operation is assumed a success. :)
		virtual void Apply()=0;
	};
}
