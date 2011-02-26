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

#include <wx/listctrl.h>

extern wxString DiscSerial;

using namespace pxSizerFlags;


enum GameDataColumnId
{
	GdbCol_Serial = 0,
	GdbCol_Title,
	GdbCol_Region,
	GdbCol_Compat,
	GdbCol_Patches,

	GdbCol_Count
};

struct ListViewColumnInfo
{
	const wxChar*		name;
	int					width;
	wxListColumnFormat	align;
};

// --------------------------------------------------------------------------------------
//  GameDatabaseListView
// --------------------------------------------------------------------------------------
class GameDatabaseListView : public wxListView
{
	typedef wxListView _parent;

protected:
	wxArrayString	m_GamesInView;

public:
	virtual ~GameDatabaseListView() throw() { }
	GameDatabaseListView( wxWindow* parent );

	void CreateColumns();
	GameDatabaseListView& AddGame( const wxString& serial );
	GameDatabaseListView& RemoveGame( const wxString& serial );
	GameDatabaseListView& ClearAllGames();
	GameDatabaseListView& SortBy( GameDataColumnId column );

protected:
	// Overrides for wxLC_VIRTUAL
	virtual wxString OnGetItemText(long item, long column) const;
	virtual int OnGetItemImage(long item) const;
	virtual int OnGetItemColumnImage(long item, long column) const;
	virtual wxListItemAttr* OnGetItemAttr(long item) const;

	const ListViewColumnInfo& GetDefaultColumnInfo( uint idx ) const;
};

// --------------------------------------------------------------------------------------
//  GameDatabaseListView  (implementations)
// --------------------------------------------------------------------------------------
GameDatabaseListView::GameDatabaseListView( wxWindow* parent )
	: _parent( parent )
{
	CreateColumns();
}

const ListViewColumnInfo& GameDatabaseListView::GetDefaultColumnInfo( uint idx ) const
{
	static const ListViewColumnInfo columns[] =
	{
		{ L"Serial",		96,		wxLIST_FORMAT_LEFT		},
		{ L"Title",			132,	wxLIST_FORMAT_LEFT		},
		{ L"Region",		72,		wxLIST_FORMAT_CENTER	},
		{ L"Compat",		48,		wxLIST_FORMAT_CENTER	},
		{ L"Patches",		48,		wxLIST_FORMAT_CENTER	},
	};

	pxAssumeDev( idx < ArraySize(columns), "ListView column index is out of bounds." );
	return columns[idx];
}

void GameDatabaseListView::CreateColumns()
{
	for( int i=0; i<GdbCol_Count; ++i )
	{
		const ListViewColumnInfo& info = GetDefaultColumnInfo(i);
		InsertColumn( i, pxGetTranslation(info.name), info.align, info.width );
	}
}

GameDatabaseListView& GameDatabaseListView::AddGame( const wxString& serial )
{
	if (m_GamesInView.Index( serial, false ) != wxNOT_FOUND) return *this;
	m_GamesInView.Add( serial );
	
	return *this;
}

GameDatabaseListView& GameDatabaseListView::RemoveGame( const wxString& serial )
{
	m_GamesInView.Remove( serial );
	return *this;
}

GameDatabaseListView& GameDatabaseListView::ClearAllGames()
{
	m_GamesInView.Clear();
	return *this;
}


class BaseGameListSort
{
protected:
	IGameDatabase*	m_GameDB;
	bool			m_descending;

public:
	BaseGameListSort( bool descend )
	{
		m_GameDB = AppHost_GetGameDatabase();
		m_descending = descend;
	}

	virtual ~BaseGameListSort() throw() {}

	// Note: Return TRUE if the first value is less than the second value.
	bool operator()(const wxString& i1, const wxString& i2)
	{
		if (!m_GameDB || (i1 == i2)) return false;

		// note: Anything not in the database gets sorted to the bottom of the list ...
		Game_Data first, second;
		if (!m_GameDB->findGame(first, i1))		return false;
		if (!m_GameDB->findGame(second, i2))	return true;

		if (int retval = _doCompare(first, second))
			return m_descending ? (retval>0) : (retval<0);

		return 0;
	}

	virtual int _doCompare( const Game_Data& first, const Game_Data& second )=0;
};

class GLSort_bySerial : public BaseGameListSort
{
public:
	GLSort_bySerial( bool descend ) : BaseGameListSort( descend ) { }

protected:
	int _doCompare( const Game_Data& g1, const Game_Data& g2 )
	{
		return g1.getString("Serial").CmpNoCase( g2.getString("Serial") );
	}
};

class GLSort_byTitle : public BaseGameListSort
{
public:
	GLSort_byTitle( bool descend ) : BaseGameListSort( descend ) { }

protected:
	int _doCompare( const Game_Data& g1, const Game_Data& g2 )
	{
		return g1.getString("Name").Cmp( g2.getString("Name") );
	}
};

class GLSort_byRegion : public BaseGameListSort
{
public:
	GLSort_byRegion( bool descend ) : BaseGameListSort( descend )  { }

protected:
	int _doCompare( const Game_Data& g1, const Game_Data& g2 )
	{
		return g1.getString("Region").CmpNoCase( g2.getString("Region") );
	}
};

class GLSort_byCompat : public BaseGameListSort
{
public:
	GLSort_byCompat( bool descend ) : BaseGameListSort( descend ) { }

protected:
	int _doCompare( const Game_Data& g1, const Game_Data& g2 )
	{
		return g1.getInt("Compat") - g2.getInt("Compat");
	}
};

class GLSort_byPatches : public BaseGameListSort
{
public:
	GLSort_byPatches( bool descend ) : BaseGameListSort( descend ) { }

protected:
	int _doCompare( const Game_Data& g1, const Game_Data& g2 )
	{
		bool hasPatches1 = !g1.getString("[patches]").IsEmpty();
		bool hasPatches2 = !g2.getString("[patches]").IsEmpty();
		
		if (hasPatches1 == hasPatches2) return 0;

		return hasPatches1 ? -1 : 1;
	}
};

GameDatabaseListView& GameDatabaseListView::SortBy( GameDataColumnId column )
{
	const bool isDescending = false;

	wxArrayString::iterator begin	= m_GamesInView.begin();
	wxArrayString::iterator end		= m_GamesInView.end();

	// Note: std::sort does not pass predicate instances by reference, which means we can't use
	// object polymorphism to simplify the code below. --air

	switch( column )
	{
		case GdbCol_Serial:		std::sort(begin, end, GLSort_bySerial(isDescending));	break;
		case GdbCol_Title:		std::sort(begin, end, GLSort_byTitle(isDescending));	break;
		case GdbCol_Region:		std::sort(begin, end, GLSort_byRegion(isDescending));	break;
		case GdbCol_Compat:		std::sort(begin, end, GLSort_byCompat(isDescending));	break;
		case GdbCol_Patches:	std::sort(begin, end, GLSort_byPatches(isDescending));	break;

		// do not use jNO_DEFAULT here -- keeps release builds from crashing (it'll just
		// ignore the sort request!)
	}
	//m_GamesInView.(  );
	
	return *this;
}

// return the text for the given column of the given item
wxString GameDatabaseListView::OnGetItemText(long item, long column) const
{
	IGameDatabase* GameDB = AppHost_GetGameDatabase();

	if (!GameDB || (item < 0) || ((uint)item >= m_GamesInView.GetCount()))
		return _parent::OnGetItemText(item, column);

	Game_Data game;
	if (!GameDB->findGame(game, m_GamesInView[item]))
	{
		pxFail( "Unknown row index in GameDatabaseListView -- returning default value." );
		return _parent::OnGetItemText(item, column);
	}

	switch( column )
	{
		case GdbCol_Serial:		return m_GamesInView[item];
		case GdbCol_Title:		return game.getString("Name");
		case GdbCol_Region:		return game.getString("Region");
		case GdbCol_Compat:		return game.getString("Compat");
		case GdbCol_Patches:	return game.getString("[patches]").IsEmpty() ? L"No" : L"Yes";
	}

	pxFail( "Unknown column index in GameDatabaseListView -- returning an empty string." );
	return wxEmptyString;
}

// return the icon for the given item. In report view, OnGetItemImage will
// only be called for the first column. See OnGetItemColumnImage for
// details.
int GameDatabaseListView::OnGetItemImage(long item) const
{
	return _parent::OnGetItemImage( item );
}

// return the icon for the given item and column.
int GameDatabaseListView::OnGetItemColumnImage(long item, long column) const
{
	return _parent::OnGetItemColumnImage( item, column );
}

static wxListItemAttr m_ItemAttr;

// return the attribute for the item (may return NULL if none)
wxListItemAttr* GameDatabaseListView::OnGetItemAttr(long item) const
{
	m_ItemAttr = wxListItemAttr();		// Wipe it clean!

	// For eventual drag&drop ?
	//if( m_TargetedItem == item )
	//	m_ItemAttr.SetBackgroundColour( wxColour(L"Wheat") );

	return &m_ItemAttr;
}


#define blankLine() {	\
	sizer1+=5; sizer1+=5; sizer1+=Text(wxEmptyString); sizer1+=5; sizer1+=5;	\
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

	*this	+= new GameDatabaseListView( this ) | StdExpand();

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
