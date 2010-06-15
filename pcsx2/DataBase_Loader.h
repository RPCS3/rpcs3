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

#pragma once

#include "Common.h"
#include "File_Reader.h"
#include "AppConfig.h"
#include <wx/wfstream.h>

struct key_pair {
	wxString key;
	wxString value;
	
	key_pair() {}
	key_pair(const wxString& _key, const wxString& _value)
		: key(_key) , value(_value) {}

	void Clear() {
		key.clear();
		value.clear();
	}

	// Performs case-insensitive compare against the key value.
	bool CompareKey( const wxString& cmpto ) const {
		return key.CmpNoCase(cmpto) == 0;
	}
	
	bool IsOk() const {
		return !key.IsEmpty();
	}

	wxString toString() const {
		if (key[0] == '[') {
			pxAssumeDev( key.EndsWith(L"]"), "Malformed multiline key detected: missing end bracket!" );

			// Terminating tag must be written without the "rvalue" -- in the form of:
			//   [/patches]
			// Use Mid() to strip off the left and right side brackets.
			wxString midLine(key.Mid(1, key.Length()-2));
			wxString keyLvalue(midLine.BeforeFirst(L'=').Trim(true).Trim(false));

			return wxsFormat( L"%s\n%s[/%s]\n",
				key.c_str(), value.c_str(), keyLvalue.c_str()
			);
		}
		else {
			// Note: 6 char padding on the l-value makes things look nicer.
			return wxsFormat(L"%-6s = %s\n", key.c_str(), value.c_str() );
		}
	
	}
};

class Game_Data {
public:
	wxString		id;				// Serial Identification Code 
	deque<key_pair>	kList;			// List of all (key, value) pairs for game data
	Game_Data(const wxString& _id = wxEmptyString)
		: id(_id) {}
		
	void NewSerial( const wxString& _id ) {
		id = _id;
		kList.clear();
	}
	
	bool IsOk() const {
		return !id.IsEmpty();
	}
};

// DataBase_Loader:
// Give the starting Key and Value you're looking for,
// and it will extract the necessary data from the database.
// Example:
// ---------------------------------------------
// Serial = SLUS-20486
// Name   = Marvel vs. Capcom 2
// Region = NTSC-U
// ---------------------------------------------
// To Load this game data, use "Serial" as the initial Key
// then specify "SLUS-20486" as the value in the constructor.
// After the constructor loads the game data, you can use the
// DataBase_Loader class's methods to get the other key's values.
// Such as dbLoader.getString("Region") returns "NTSC-U"

class DataBase_Loader {
protected:
	bool isComment(const wxString& s);
	void doError(const wxString& line, key_pair& keyPair, bool doMsg = false);
	bool extractMultiLine(const wxString& line, key_pair& keyPair, wxInputStream& reader);
	void extract(const wxString& line, key_pair& keyPair, wxInputStream& reader);
	
	const wxString	m_emptyString;	// empty string for returning stuff .. never modify!
	wxString		m_dest;
	std::string		m_intermediate;

public:
	deque<Game_Data> gList;			// List of all game data
	Game_Data*		curGame;		// Current game data
	wxString		header;			// Header of the database
	wxString		baseKey;		// Key to separate games by ("Serial")

	DataBase_Loader(const wxString& file = L"GameIndex.dbf", const wxString& key = L"Serial", const wxString& value = wxEmptyString )
		: baseKey( key )
	{
		curGame	= NULL;
		if (!wxFileExists(file)) {
			Console.Error(L"DataBase_Loader: DataBase Not Found! [%s]", file.c_str());
		}
		wxFFileInputStream reader( file );
		key_pair      keyPair;
		wxString      s0;
		Game_Data	  game;

		try {
			while(!reader.Eof()) {
				while(!reader.Eof()) { // Find first game
					pxReadLine(reader, s0, m_intermediate);
					extract(s0.Trim(true).Trim(false), keyPair, reader);
					if (keyPair.CompareKey(key)) break;
					header += s0 + L'\n';
				}
				game.NewSerial( keyPair.value );
				game.kList.push_back(keyPair);
				
				while(!reader.Eof()) { // Fill game data, find new game, repeat...
					pxReadLine(reader, s0, m_intermediate);
					extract(s0.Trim(true).Trim(false), keyPair, reader);
					if (!keyPair.IsOk()) continue;
					if (keyPair.CompareKey(key)) {
						gList.push_back(game);
						game.NewSerial(keyPair.value);
					}
					game.kList.push_back(keyPair);
				}
			}
		}
		catch( Exception::EndOfStream& ) {}

		if (game.IsOk()) gList.push_back(game);

		if (value.IsEmpty()) return;
		if (setGame(value)) Console.WriteLn(L"DataBase_Loader: Found Game! [%s]",     value.c_str());
		else				Console.Warning(L"DataBase_Loader: Game Not Found! [%s]", value.c_str());
	}

	virtual ~DataBase_Loader() throw() {
		// deque deletes its contents automatically.
		Console.WriteLn( "(GameDB) Destroying..." );
	}

	// Sets the current game to the one matching the serial id given
	// Returns true if game found, false if not found...
	bool setGame(const wxString& id) {
		deque<Game_Data>::iterator it = gList.begin();
		for ( ; it != gList.end(); ++it) {
			if (it[0].id == id) {
				curGame = &it[0];
				return true;
			}
		}
		curGame = NULL;
		return false;
	}

	// Returns true if a game is currently loaded into the database
	// Returns false if otherwise (this means you need to call setGame()
	// or it could mean the game was not found in the database at all...)
	bool gameLoaded() {
		return !!curGame;
	}

	// Saves changes to the database
	void saveToFile(const wxString& file = L"GameIndex.dbf") {
		wxFFileOutputStream writer( file );
		pxWriteMultiline(writer, header);
		deque<Game_Data>::iterator it = gList.begin();
		for ( ; it != gList.end(); ++it) {
			deque<key_pair>::iterator i = it[0].kList.begin();
			for ( ; i != it[0].kList.end(); ++i) {
				pxWriteMultiline(writer, i[0].toString() );
			}
			pxWriteLine(writer, L"---------------------------------------------");
		}
	}

	// Adds new game data to the database, and sets curGame to the new game...
	// If searchDB is true, it searches the database to see if game already exists.
	void addGame(const wxString& id, bool searchDB = true) {
		if (searchDB && setGame(id)) return;
		Game_Data	game(id);
		key_pair	kp(baseKey, id);
		game.kList.push_back(kp);
		gList.push_back(game);
		curGame = &(gList.end()-1)[0];
	}

	// Searches the current game's data to see if the given key exists
	bool keyExists(const wxChar* key) {
		if (curGame) {
			deque<key_pair>::iterator it = curGame->kList.begin();
			for ( ; it != curGame->kList.end(); ++it) {
				if (it[0].CompareKey(key)) {
					return true;
				}
			}
		}
		else Console.Error("DataBase_Loader: Game not set!");
		return false;
	}

	// Totally Deletes the specified key/pair value from the current game's data
	void deleteKey(const wxChar* key) {
		if (curGame) {
			deque<key_pair>::iterator it = curGame->kList.begin();
			for ( ; it != curGame->kList.end(); ++it) {
				if (it[0].CompareKey(key)) {
					curGame->kList.erase(it);
					return;
				}
			}
		}
		else Console.Error("DataBase_Loader: Game not set!");
	}

	// Gets a string representation of the 'value' for the given key
	wxString getString(const wxChar* key) {
		if (curGame) {
			deque<key_pair>::iterator it = curGame->kList.begin();
			for ( ; it != curGame->kList.end(); ++it) {
				if (it[0].CompareKey(key)) {
					return it[0].value;
				}
			}
		}
		else Console.Error("DataBase_Loader: Game not set!");
		return wxString();
	}

	bool sectionExists(const wxChar* key, const wxString& value) {
		return keyExists( wxsFormat(L"[%s = %s]", key, value.c_str()) );
	}

	wxString getSection(const wxChar* key, const wxString& value) {
		return getString( wxsFormat(L"[%s = %s]", key, value.c_str()) );
	}
	
	// Gets an integer representation of the 'value' for the given key
	int getInt(const wxChar* key) {
		return wxStrtoul(getString(key), NULL, 0);
	}

	// Gets a u8 representation of the 'value' for the given key
	u8 getU8(const wxChar* key) {
		return (u8)wxAtoi(getString(key));
	}

	// Gets a bool representation of the 'value' for the given key
	bool getBool(const wxChar* key) {
		return !!wxAtoi(getString(key));
	}

	wxString getString(const char* key) {
		return getString(fromUTF8(key));
	}

	bool keyExists(const char* key) {
		return keyExists(fromUTF8(key));
	}

	int getInt(const char* key) {
		return getInt(fromUTF8(key));
	}

	u8 getU8(const char* key) {
		return getU8(fromUTF8(key));
	}

	bool getBool(const char* key) {
		return getBool(fromUTF8(key));
	}

	// Write a string value to the specified key
	void writeString(const wxString& key, const wxString& value) {
		if (curGame) {
			deque<key_pair>::iterator it = curGame->kList.begin();
			for ( ; it != curGame->kList.end(); ++it) {
				if (it[0].CompareKey(key)) {
					it[0].value = value;
					return;
				}
			}
			key_pair tKey(key, value);
			curGame->kList.push_back(tKey);
		}
		else Console.Error("DataBase_Loader: Game not set!");
	}

	// Write a bool value to the specified key
	void writeBool(const wxString& key, bool value) {
		writeString(key, value ? L"1" : L"0");
	}
};

static wxString compatToStringWX(int compat) {
	switch (compat) {
		case 6:  return L"Perfect";
		case 5:  return L"Playable";
		case 4:  return L"In-Game";
		case 3:  return L"Menu";
		case 2:  return L"Intro";
		case 1:  return L"Nothing";
		default: return L"Unknown";
	}
}

#define checkGamefix(gFix) {										\
	if (gameDB->keyExists(#gFix)) {									\
		SetGameFixConfig().gFix = gameDB->getBool(#gFix);			\
		Console.WriteLn("Loading Gamefix: " #gFix);					\
		gf++;														\
	}																\
}

// Load Game Settings found in database
// (game fixes, round modes, clamp modes, etc...)
// Returns number of gamefixes set
static int loadGameSettings(DataBase_Loader* gameDB) {
	if (gameDB && gameDB->gameLoaded()) {
		SSE_MXCSR  eeMX = EmuConfig.Cpu.sseMXCSR;
		SSE_MXCSR  vuMX = EmuConfig.Cpu.sseVUMXCSR;
		int eeRM = eeMX.GetRoundMode();
		int vuRM = vuMX.GetRoundMode();
		bool rm  = 0;
		int  gf  = 0;
		if (gameDB->keyExists("eeRoundMode")) { eeRM = gameDB->getInt("eeRoundMode"); rm=1; gf++; }
		if (gameDB->keyExists("vuRoundMode")) { vuRM = gameDB->getInt("vuRoundMode"); rm=1; gf++; }
		if (rm && eeRM<4 && vuRM<4) { 
			Console.WriteLn("Game DataBase: Changing roundmodes! [ee=%d] [vu=%d]", eeRM, vuRM);
			SetCPUState(eeMX.SetRoundMode((SSE_RoundMode)eeRM), vuMX.SetRoundMode((SSE_RoundMode)vuRM));
		}
		if (gameDB->keyExists("eeClampMode")) {
			int clampMode = gameDB->getInt("eeClampMode");
			Console.WriteLn("Game DataBase: Changing EE/FPU clamp mode [mode=%d]", clampMode);
			SetRecompilerConfig().fpuOverflow		= clampMode >= 1;
			SetRecompilerConfig().fpuExtraOverflow	= clampMode >= 2;
			SetRecompilerConfig().fpuFullMode		= clampMode >= 3;
			gf++;
		}
		if (gameDB->keyExists("vuClampMode")) {
			int clampMode = gameDB->getInt("vuClampMode");
			Console.WriteLn("Game DataBase: Changing VU0/VU1 clamp mode [mode=%d]", clampMode);
			SetRecompilerConfig().vuOverflow		= clampMode >= 1;
			SetRecompilerConfig().vuExtraOverflow	= clampMode >= 2;
			SetRecompilerConfig().vuSignOverflow	= clampMode >= 3;
			gf++;
		}
		checkGamefix(VuAddSubHack);
		checkGamefix(VuClipFlagHack);
		checkGamefix(FpuCompareHack);
		checkGamefix(FpuMulHack);
		checkGamefix(FpuNegDivHack);
		checkGamefix(XgKickHack);
		checkGamefix(IPUWaitHack);
		checkGamefix(EETimingHack);
		checkGamefix(SkipMPEGHack);
		return gf;
	}
	return 0;
}

extern DataBase_Loader* AppHost_GetGameDatabase();