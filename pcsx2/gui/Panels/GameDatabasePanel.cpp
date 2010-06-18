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
#include "App.h"
#include "ConfigurationPanels.h"

extern wxString DiscID;
using namespace pxSizerFlags;

#define blankLine() {												\
	sizer1+=5; sizer1+=5; sizer1+=Text(L""); sizer1+=5; sizer1+=5;	\
}

#define placeTextBox(wxBox, txt) {	\
	sizer1	+= Label(_(txt));		\
	sizer1	+= 5;					\
	sizer1	+= wxBox | pxCenter;	\
	sizer1	+= 5;					\
	sizer1	+= 5;					\
}

wxTextCtrl* CreateMultiLineTextCtrl( wxWindow* parent, int digits, long flags = 0 )
{
	wxTextCtrl* ctrl = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	pxFitToDigits(ctrl, digits);
	return ctrl;
}

Panels::GameDatabasePanel::GameDatabasePanel( wxWindow* parent )
	: BaseApplicableConfigPanel( parent )
{
	//if (!GameDB) GameDB = new GameDatabase();
	IGameDatabase* GameDB = AppHost_GetGameDatabase();
	pxAssume( GameDB != NULL );
	
	searchBtn  = new wxButton  (this, wxID_ANY, _("Search"));

	serialBox  = CreateNumericalTextCtrl(this, 40, wxTE_LEFT);
	nameBox    = CreateNumericalTextCtrl(this, 40, wxTE_LEFT);
	regionBox  = CreateNumericalTextCtrl(this, 40, wxTE_LEFT);
	compatBox  = CreateNumericalTextCtrl(this, 40, wxTE_LEFT);
	commentBox = CreateMultiLineTextCtrl(this, 40, wxTE_LEFT);
	patchesBox = CreateMultiLineTextCtrl(this, 40, wxTE_LEFT);

	for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
		gameFixes[i] = new pxCheckBox(this, EnumToString(i), wxCHK_3STATE | wxCHK_ALLOW_3RD_STATE_FOR_USER );

	*this	+= Heading(_("Game Database Editor")).Bold() | StdExpand();
	*this	+= Heading(_("This panel lets you add and edit game titles, game fixes, and game patches.")) | StdExpand();

	wxFlexGridSizer& sizer1(*new wxFlexGridSizer(5, StdPadding));
	sizer1.AddGrowableCol(0);

	blankLine();
	sizer1	+= Label(L"Serial: ");	
	sizer1	+= 5;
	sizer1	+= serialBox | pxCenter;
	sizer1	+= 5;
	sizer1	+= searchBtn;

	placeTextBox(nameBox,    "Name: ");
	placeTextBox(regionBox,  "Region: ");
	placeTextBox(compatBox,  "Compatibility: ");
	placeTextBox(commentBox, "Comments: ");
	placeTextBox(patchesBox, "Patches: ");

	blankLine();

	wxStaticBoxSizer& sizer2 = *new wxStaticBoxSizer(wxVERTICAL, this, _("PCSX2 Gamefixes"));
	wxFlexGridSizer&  sizer3(*new wxFlexGridSizer(3, 0, StdPadding*4));
	sizer3.AddGrowableCol(0);

	for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
		sizer3 += gameFixes[i];

	sizer2 += sizer3 | StdCenter();

	*this	+= sizer1  | pxCenter;
	*this	+= sizer2  | pxCenter;
	
	Connect(searchBtn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GameDatabasePanel::Search_Click));
	PopulateFields();
}

void Panels::GameDatabasePanel::PopulateFields() {
	IGameDatabase* GameDB = AppHost_GetGameDatabase();

	if (GameDB->gameLoaded()) {
		serialBox ->SetLabel(GameDB->getString("Serial"));
		nameBox   ->SetLabel(GameDB->getString("Name"));
		regionBox ->SetLabel(GameDB->getString("Region"));
		compatBox ->SetLabel(GameDB->getString("Compat"));
		commentBox->SetLabel(GameDB->getString("[comments]"));
		patchesBox->SetLabel(GameDB->getString("[patches]"));

		for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
		{
			wxString keyName (EnumToString(i)); keyName += L"Hack";
			if( GameDB->keyExists(keyName) )
				gameFixes[i]->SetValue(GameDB->getBool(keyName));
			else
				gameFixes[i]->SetIndeterminate();
		}
	}
	else {
		serialBox ->SetLabel(L"");
		nameBox   ->SetLabel(L"");
		regionBox ->SetLabel(L"");
		compatBox ->SetLabel(L"");
		commentBox->SetLabel(L"");
		patchesBox->SetLabel(L"");
		for (int i = 0; i < GamefixId_COUNT; i++) {
			gameFixes[i]->SetValue(0);
		}
	}
}

#define writeTextBoxToDB(_key, _value) {								\
	if (_value.IsEmpty()) 	GameDB->deleteKey(wxT(_key));				\
	else					GameDB->writeString(wxT(_key), _value);		\
}

// returns True if the database is modified, or FALSE if no changes to save.
bool Panels::GameDatabasePanel::WriteFieldsToDB() {
	IGameDatabase* GameDB = AppHost_GetGameDatabase();

	if (serialBox->GetValue().IsEmpty()) return false;

	// Only write the serial if its been changed
	if( !GameDB->setGame(serialBox->GetValue()) )
		GameDB->writeString(L"Serial",		serialBox->GetValue());

	GameDB->writeString(L"Name",		nameBox->GetValue());
	GameDB->writeString(L"Region",		regionBox->GetValue());
	GameDB->writeString(L"Compat",		compatBox->GetValue());
	GameDB->writeString(L"[comments]",	commentBox->GetValue());
	GameDB->writeString(L"[patches]",	patchesBox->GetValue());
	
	for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i) {
		wxString keyName (EnumToString(i)); keyName += L"Hack";

		if (gameFixes[i]->IsIndeterminate())
			GameDB->deleteKey(keyName);
		else
			GameDB->writeBool(keyName, gameFixes[i]->GetValue());
	}
	return true;
}

void Panels::GameDatabasePanel::Search_Click(wxCommandEvent& evt) {
	IGameDatabase* GameDB = AppHost_GetGameDatabase();
	wxString wxStr = serialBox->GetValue();

	if( wxStr.IsEmpty() ) wxStr = DiscID;

	bool bySerial  = 1;//searchType->GetSelection()==0;
	if  (bySerial) GameDB->setGame(wxStr);
	
	PopulateFields();
	evt.Skip();
}

void Panels::GameDatabasePanel::Apply() {
	AppGameDatabase* GameDB = wxGetApp().GetGameDatabase();
	if( WriteFieldsToDB() )
	{
		Console.WriteLn("Saving changes to Game Database...");
		GameDB->SaveToFile();
	}
}

void Panels::GameDatabasePanel::AppStatusEvent_OnSettingsApplied()
{
}
