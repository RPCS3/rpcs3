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
#include "DataBase_Loader.h"
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
	if (!GameDB) GameDB = new DataBase_Loader();
	searchBtn  = new wxButton  (this, wxID_DEFAULT, L"Search");

	serialBox  = CreateNumericalTextCtrl(this, 40, wxTE_LEFT);
	nameBox    = CreateNumericalTextCtrl(this, 40, wxTE_LEFT);
	regionBox  = CreateNumericalTextCtrl(this, 40, wxTE_LEFT);
	compatBox  = CreateNumericalTextCtrl(this, 40, wxTE_LEFT);
	commentBox = CreateMultiLineTextCtrl(this, 40, wxTE_LEFT);
	patchesBox = CreateMultiLineTextCtrl(this, 40, wxTE_LEFT);

	gameFixes[0] = new pxCheckBox(this, L"VuAddSubHack");
	gameFixes[1] = new pxCheckBox(this, L"VuClipFlagHack");
	gameFixes[2] = new pxCheckBox(this, L"FpuCompareHack");
	gameFixes[3] = new pxCheckBox(this, L"FpuMulHack");
	gameFixes[4] = new pxCheckBox(this, L"FpuNegDivHack");
	gameFixes[5] = new pxCheckBox(this, L"XgKickHack");
	gameFixes[6] = new pxCheckBox(this, L"IPUWaitHack");
	gameFixes[7] = new pxCheckBox(this, L"EETimingHack");
	gameFixes[8] = new pxCheckBox(this, L"SkipMPEGHack");

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
	wxFlexGridSizer&  sizer3(*new wxFlexGridSizer(3));
	sizer3.AddGrowableCol(0);

	for (int i = 0; i < NUM_OF_GAME_FIXES; i++) {
		sizer3 += gameFixes[i];
	}

	sizer2 += sizer3 | pxCenter;

	*this	+= sizer1  | pxCenter;
	*this	+= sizer2  | pxCenter;
	
	Connect(searchBtn->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(GameDatabasePanel::Search_Click));
	PopulateFields();
}

void Panels::GameDatabasePanel::PopulateFields() {
	if (GameDB->gameLoaded()) {
		serialBox ->SetLabel(GameDB->getString("Serial"));
		nameBox   ->SetLabel(GameDB->getString("Name"));
		regionBox ->SetLabel(GameDB->getString("Region"));
		compatBox ->SetLabel(GameDB->getString("Compat"));
		commentBox->SetLabel(GameDB->getString("[comments]"));
		patchesBox->SetLabel(GameDB->getString("[patches]"));
		gameFixes[0]->SetValue(GameDB->getBool("VuAddSubHack"));
		gameFixes[1]->SetValue(GameDB->getBool("VuClipFlagHack"));
		gameFixes[2]->SetValue(GameDB->getBool("FpuCompareHack"));
		gameFixes[3]->SetValue(GameDB->getBool("FpuMulHack"));
		gameFixes[4]->SetValue(GameDB->getBool("FpuNegDivHack"));
		gameFixes[5]->SetValue(GameDB->getBool("XgKickHack"));
		gameFixes[6]->SetValue(GameDB->getBool("IPUWaitHack"));
		gameFixes[7]->SetValue(GameDB->getBool("EETimingHack"));
		gameFixes[8]->SetValue(GameDB->getBool("SkipMPEGHack"));
	}
	else {
		serialBox ->SetLabel(L"");
		nameBox   ->SetLabel(L"");
		regionBox ->SetLabel(L"");
		compatBox ->SetLabel(L"");
		commentBox->SetLabel(L"");
		patchesBox->SetLabel(L"");
		for (int i = 0; i < NUM_OF_GAME_FIXES; i++) {
			gameFixes[i]->SetValue(0);
		}
	}
}

#define writeTextBoxToDB(_key, _value) {								\
	if (_value.IsEmpty()) 	GameDB->deleteKey(wxT(_key));				\
	else					GameDB->writeString(wxT(_key), _value);		\
}

#define writeGameFixToDB(_key, _value) {								\
	if (!_value)	GameDB->deleteKey(wxT(_key));						\
	else			GameDB->writeBool(wxT(_key), _value);				\
}

// returns True if the database is modified, or FALSE if no changes to save.
bool Panels::GameDatabasePanel::WriteFieldsToDB() {
	wxString wxStr( serialBox->GetValue() );
	
	if (wxStr.IsEmpty()) return false;
	if (wxStr != GameDB->getString("Serial")) {
		GameDB->addGame(wxStr);
	}

	writeTextBoxToDB("Name",			nameBox->GetValue());
	writeTextBoxToDB("Region",			regionBox->GetValue());
	writeTextBoxToDB("Compat",			compatBox->GetValue());
	writeTextBoxToDB("[comments]",		commentBox->GetValue());
	writeTextBoxToDB("[patches]",		patchesBox->GetValue());
	writeGameFixToDB("VuAddSubHack",	gameFixes[0]->GetValue());
	writeGameFixToDB("VuClipFlagHack",	gameFixes[1]->GetValue());
	writeGameFixToDB("FpuCompareHack",	gameFixes[2]->GetValue());
	writeGameFixToDB("FpuMulHack",		gameFixes[3]->GetValue());
	writeGameFixToDB("FpuNegDivHack",	gameFixes[4]->GetValue());
	writeGameFixToDB("XgKickHack",		gameFixes[5]->GetValue());
	writeGameFixToDB("IPUWaitHack",		gameFixes[6]->GetValue());
	writeGameFixToDB("EETimingHack",	gameFixes[7]->GetValue());
	writeGameFixToDB("SkipMPEGHack",	gameFixes[8]->GetValue());
	return true;
}

void Panels::GameDatabasePanel::Search_Click(wxCommandEvent& evt) {
	wxString wxStr = serialBox->GetValue();
	
	if( wxStr.IsEmpty() ) wxStr = DiscID;
	
	bool bySerial  = 1;//searchType->GetSelection()==0;
	if  (bySerial) GameDB->setGame(wxStr);
	
	PopulateFields();
	evt.Skip();
}

void Panels::GameDatabasePanel::Apply() {
	if( WriteFieldsToDB() )
	{
		Console.WriteLn("Saving changes to Game Database...");
		GameDB->saveToFile();
	}
}

void Panels::GameDatabasePanel::AppStatusEvent_OnSettingsApplied()
{
}

/*
	//#define lineIndent(_wxSizer, txt) {_wxSizer+=5;_wxSizer+=5;_wxSizer+=Heading(txt);_wxSizer+=5;_wxSizer+=5;}

	//searchBox  = CreateNumericalTextCtrl(this, 40, wxTE_LEFT);
	//searchType = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	//searchList = new wxListBox (this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_SORT | wxLB_NEEDED_SB);
	//searchList->SetFont   (wxFont(searchList->GetFont().GetPointSize()+1, wxFONTFAMILY_MODERN, wxNORMAL, wxNORMAL, false, L"Lucida Console"));
	//searchList->SetMinSize(wxSize(wxDefaultCoord, std::max(searchList->GetMinSize().GetHeight(), 96)));

	searchType->Append(L"Game Serial", (void*)0);
	searchType->Append(L"Game Name"  , (void*)0);
	searchType->SetSelection(0);

	sizer1	+= searchType;
	sizer1	+= 5;
	sizer1	+= searchBox | pxCenter;
	sizer1	+= 5;
	sizer1	+= searchBtn;

	sizer1	+= 5;
	sizer1	+= 5;
	sizer1	+= searchList  | StdExpand();// pxCenter;
	sizer1	+= 5;
	sizer1	+= 5;

	lineIndent(sizer1, L"");
	sizer1	+= 5;
	sizer1	+= 5;
	sizer1	+= new wxStaticLine(this) | StdExpand();
	sizer1	+= 5;
	sizer1	+= 5;
	lineIndent(sizer1, L"");
	lineIndent(sizer1, L"Game Info");
*/
