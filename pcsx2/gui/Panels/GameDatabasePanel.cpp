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
#include "AppGameDatabase.h"
#include "ConfigurationPanels.h"

extern wxString DiscSerial;
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

	wxStaticBoxSizer& sizer2 = *new wxStaticBoxSizer(wxVERTICAL, this, _("Gamefixes"));
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

void Panels::GameDatabasePanel::PopulateFields( const wxString& id ) {
	IGameDatabase* GameDB = AppHost_GetGameDatabase();
	if (!pxAssert(GameDB)) return;

	Game_Data game;
	if (GameDB->findGame(game, id.IsEmpty() ? SysGetDiscID() : id))
	{
		serialBox ->SetLabel(game.getString("Serial"));
		nameBox   ->SetLabel(game.getString("Name"));
		regionBox ->SetLabel(game.getString("Region"));
		compatBox ->SetLabel(game.getString("Compat"));
		commentBox->SetLabel(game.getString("[comments]"));
		patchesBox->SetLabel(game.getString("[patches]"));

		for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i)
		{
			wxString keyName (EnumToString(i)); keyName += L"Hack";
			if( game.keyExists(keyName) )
				gameFixes[i]->SetValue(game.getBool(keyName));
			else
				gameFixes[i]->SetIndeterminate();
		}
	}
	else {
		serialBox ->SetLabel(wxEmptyString);
		nameBox   ->SetLabel(wxEmptyString);
		regionBox ->SetLabel(wxEmptyString);
		compatBox ->SetLabel(wxEmptyString);
		commentBox->SetLabel(wxEmptyString);
		patchesBox->SetLabel(wxEmptyString);
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
	if (!GameDB) return false;

	if (serialBox->GetValue().IsEmpty()) return false;

	Game_Data game;
	GameDB->findGame(game, serialBox->GetValue());

	game.id = serialBox->GetValue();

	game.writeString(L"Serial",		serialBox->GetValue());
	game.writeString(L"Name",		nameBox->GetValue());
	game.writeString(L"Region",		regionBox->GetValue());
	game.writeString(L"Compat",		compatBox->GetValue());
	game.writeString(L"[comments]",	commentBox->GetValue());
	game.writeString(L"[patches]",	patchesBox->GetValue());
	
	for (GamefixId i=GamefixId_FIRST; i < pxEnumEnd; ++i) {
		wxString keyName (EnumToString(i)); keyName += L"Hack";

		if (gameFixes[i]->IsIndeterminate())
			game.deleteKey(keyName);
		else
			game.writeBool(keyName, gameFixes[i]->GetValue());
	}
	GameDB->updateGame(game);
	return true;
}

void Panels::GameDatabasePanel::Search_Click(wxCommandEvent& evt) {
	IGameDatabase* GameDB = AppHost_GetGameDatabase();
	if( !GameDB ) return;

	PopulateFields( serialBox->GetValue() );
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
