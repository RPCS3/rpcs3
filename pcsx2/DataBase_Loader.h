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

#include "File_Reader.h"
#include "AppConfig.h"

struct key_pair {
	string key;
	string value;
	
	key_pair() {}
	key_pair(const string& _key, const string& _value)
		: key(_key) , value(_value) {}
	string toString() {
		string t;
			
		if (key[0] == '[') {
			t  = key + "\n";
			t += value;
			
			stringstream ss(key);
			string t2;
			ss >>  t2;
			t += "[/" + t2.substr(1, t2.length()-1);
			if (t2.compare(t)) t += "]";
		}
		else {
			t = key;
			for (int a = 6 - key.length(); a > 0; a--) {
				t += " "; // Padding for nice formatting on small key-names
			}
			t += " = " + value;
		}
		return t;
	}
};

class Game_Data {
public:
	string id;				// Serial Identification Code 
	deque<key_pair> kList;	// List of all (key, value) pairs for game data
	Game_Data(const string& _id) 
		: id(_id) {}
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
private:
	template<class T> string toString(T value) {
		stringstream ss(ios_base::in | ios_base::out);
		string tString;
		ss <<  value;
		ss >>  tString;
		return tString;
	}
	string toLower(const string& s) {
		string retval( s );
		for (uint i = 0; i < s.length(); i++) {
			char& c = retval[i];
			if (c >= 'A' && c <= 'Z') {
				c += 'a' - 'A';
			}
		}
		return retval;
	}
	bool strCompare(const string& s1, const string& s2) {
		const string t1( toLower(s1) );
		const string t2( toLower(s2) );
		return !t1.compare(t2);
	}
	bool isComment(const string& s) {
		const string sub( s.substr(0, 2) );
		return (sub.compare("--") == 0) || (sub.compare("//") == 0);
	}
	void doError(const string& line, key_pair& keyPair, bool doMsg = false) {
		if (doMsg) Console.Error("DataBase_Loader: Bad file data [%s]", line.c_str());
		keyPair.key.clear();
	}
	void extractMultiLine(const string& line, key_pair& keyPair, File_Reader& reader, const stringstream& ss) {
		string t;
		string endString;
		endString = "[/" + keyPair.key.substr(1, keyPair.key.length()-1);
		if (keyPair.key[keyPair.key.length()-1] != ']') {
			endString += "]";
			keyPair.key = line;
		}
		for(;;) {
			t = reader.getLine();
			
			if (!t.compare(endString)) break;
			keyPair.value += t + "\n";
		}
	}
	void extract(const string& line, key_pair& keyPair, File_Reader& reader) {
		stringstream ss(line);
		string t;
		keyPair.key.clear();
		keyPair.value.clear();
		ss >> keyPair.key;
		if (!line.length() || isComment(keyPair.key)) {
			doError(line, keyPair);
			return; 
		}
		if (keyPair.key[0] == '[') {
			extractMultiLine(line, keyPair, reader, ss);
			return;
		}
		ss >> t;
		if (t.compare("=") != 0) {
			doError(line, keyPair, true);
			return; 
		}
		ss >> t;
		if (isComment(t)) {
			doError(line, keyPair, true);
			return;
		}
		keyPair.value = t;
		while (!ss.eof() && !ss.fail()) {
			ss >> t;
			if (isComment(t)) break; 
			keyPair.value += " " + t;
		}
		if (ss.fail()) {
			doError(line, keyPair);
			return;
		}
	}
public:
	deque<Game_Data*> gList; // List of all game data
	Game_Data*    curGame;   // Current game data
	String_Stream header;    // Header of the database
	string		  baseKey;	 // Key to separate games by ("Serial")

	DataBase_Loader(const string& file, const string& key = "Serial", const string& value = "" ) {
		curGame	= NULL;
		baseKey = key;
		if (!fileExists(file)) {
			Console.Error("DataBase_Loader: DataBase Not Found! [%s]", file.c_str());
		}
		File_Reader   reader(file);
		key_pair      keyPair;
		string        s0;
		Game_Data*	  game = NULL;
		try {
			for(;;) {
				for(;;) { // Find first game
					s0 = reader.getLine();
					extract(s0, keyPair, reader);
					if (keyPair.key.compare(key) == 0) break;
					header.write(s0);
					header.write("\n");
				}
				game = new Game_Data(keyPair.value);
				game->kList.push_back(keyPair);
				for (;;) { // Fill game data, find new game, repeat...
					s0 = reader.getLine();
					extract(s0, keyPair, reader);
					if (keyPair.key.compare("")  == 0) continue;
					if (keyPair.key.compare(key) == 0) {
						gList.push_back(game);
						game = new Game_Data(keyPair.value);
					}
					game->kList.push_back(keyPair);
				}
			}
		}
		catch(int& i) { // Add Last Game if EOF
			if (i==1 && game) gList.push_back(game);
		}
		if (!value.compare("")) return;
		if (setGame(value)) Console.WriteLn("DataBase_Loader: Found Game! [%s]",     value.c_str());
		else				Console.Warning("DataBase_Loader: Game Not Found! [%s]", value.c_str());
	}

	virtual ~DataBase_Loader() throw() {
		deque<Game_Data*>::iterator it = gList.begin();
		for ( ; it != gList.end(); ++it) {
			delete *it;
		}
	}

	// Sets the current game to the one matching the serial id given
	// Returns true if game found, false if not found...
	bool setGame(const string& id) {
		deque<Game_Data*>::iterator it = gList.begin();
		for ( ; it != gList.end(); ++it) {
			if (strCompare(it[0]->id, id)) {
				curGame =  it[0];
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
	void saveToFile(const string& file = "DataBase.dbf") {
		File_Writer writer(file);
		writer.write(header.toString());
		deque<Game_Data*>::iterator it = gList.begin();
		for ( ; it != gList.end(); ++it) {
			deque<key_pair>::iterator i = it[0]->kList.begin();
			for ( ; i != it[0]->kList.end(); ++i) {
				writer.write(i[0].toString() + "\n");
			}
			writer.write("---------------------------------------------\n");
		}
	}

	// Adds new game data to the database, and sets curGame to the new game...
	// If searchDB is true, it searches the database to see if game already exists.
	void addGame(const string& id, bool searchDB = true) {
		if (searchDB && setGame(id)) return;
		Game_Data* game = new Game_Data(id);
		key_pair   kp(baseKey, id);
		game->kList.push_back(kp);
		gList.push_back(game);
		curGame = game;
	}

	// Searches the current game's data to see if the given key exists
	bool keyExists(const string& key) {
		if (curGame) {
			deque<key_pair>::iterator it = curGame->kList.begin();
			for ( ; it != curGame->kList.end(); ++it) {
				if (strCompare(it[0].key, key)) {
					return true;
				}
			}
		}
		else Console.Error("DataBase_Loader: Game not set!");
		return false;
	}

	// Gets a string representation of the 'value' for the given key
	string getString(const string& key) {
		if (curGame) {
			deque<key_pair>::iterator it = curGame->kList.begin();
			for ( ; it != curGame->kList.end(); ++it) {
				if (strCompare(it[0].key, key)) {
					return it[0].value;
				}
			}
		}
		else Console.Error("DataBase_Loader: Game not set!");
		return string();
	}

	// Gets a wxString representation of the 'value' for the given key
	wxString getStringWX(const string& key) {
		return wxString(fromUTF8(getString(key).c_str()));
	}

	// Gets a double representation of the 'value' for the given key
	double getDouble(const string& key) {
		return atof(getString(key).c_str());
	}

	// Gets a float representation of the 'value' for the given key
	float getFloat(const string& key) {
		return (float)atof(getString(key).c_str());
	}

	// Gets an integer representation of the 'value' for the given key
	int getInt(const string& key) {
		return strtoul(getString(key).c_str(), NULL, 0);
	}

	// Gets a u8 representation of the 'value' for the given key
	u8 getU8(const string& key) {
		return (u8)atoi(getString(key).c_str());
	}

	// Gets a bool representation of the 'value' for the given key
	bool getBool(const string& key) {
		return !!atoi(getString(key).c_str());
	}

	// Write a string value to the specified key
	void writeString(const string& key, const string& value) {
		if (curGame) {
			deque<key_pair>::iterator it = curGame->kList.begin();
			for ( ; it != curGame->kList.end(); ++it) {
				if (strCompare(it[0].key, key)) {
					it[0].value = value;
					return;
				}
			}
			key_pair tKey(key, value);
			curGame->kList.push_back(tKey);
		}
		else Console.Error("DataBase_Loader: Game not set!");
	}

	// Write a wxString value to the specified key
	void writeStringWX(const string& key, const wxString& value) {
		writeString(key, value.ToUTF8().data());
	}

	// Write a double value to the specified key
	void writeDouble(const string& key, double value) {
		writeString(key, toString(value));
	}
	
	// Write a float value to the specified key
	void writeFloat(const string& key, float value) {
		writeString(key, toString(value));
	}
	
	// Write an integer value to the specified key
	void writeInt(const string& key, int value) {
		writeString(key, toString(value));
	}
	
	// Write a u8 value to the specified key
	void writeU8(const string& key, u8 value) {
		writeString(key, toString(value));
	}
	
	// Write a bool value to the specified key
	void writeBool(const string& key, bool value) {
		writeString(key, toString(value?1:0));
	}
};

static wxString compatToStringWX(int compat) {
	switch (compat) {
		case 6:  return wxString(L"Perfect");
		case 5:  return wxString(L"Playable");
		case 4:  return wxString(L"In-Game");
		case 3:  return wxString(L"Menu");
		case 2:  return wxString(L"Intro");
		case 1:  return wxString(L"Nothing");
		default: return wxString(L"Unknown");
	}
}

#define checkGamefix(gFix) {										\
	if (gameDB->keyExists(#gFix)) {									\
		SetGameFixConfig().gFix = gameDB->getBool(#gFix);			\
		Console.WriteLn("Loading Gamefix: " #gFix);					\
	}																\
}

// Load Game Settings found in database
// (game fixes, round modes, clamp modes, etc...)
static void loadGameSettings(DataBase_Loader* gameDB) {
	if (gameDB && gameDB->gameLoaded()) {
		SSE_MXCSR  eeMX = EmuConfig.Cpu.sseMXCSR;
		SSE_MXCSR  vuMX = EmuConfig.Cpu.sseVUMXCSR;
		int eeRM = eeMX.GetRoundMode();
		int vuRM = vuMX.GetRoundMode();
		bool rm = 0;
		if (gameDB->keyExists("eeRoundMode")) { eeRM = gameDB->getInt("eeRoundMode"); rm=1; }
		if (gameDB->keyExists("vuRoundMode")) { vuRM = gameDB->getInt("vuRoundMode"); rm=1; }
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
		}
		if (gameDB->keyExists("vuClampMode")) {
			int clampMode = gameDB->getInt("vuClampMode");
			Console.WriteLn("Game DataBase: Changing VU0/VU1 clamp mode [mode=%d]", clampMode);
			SetRecompilerConfig().vuOverflow		= clampMode >= 1;
			SetRecompilerConfig().vuExtraOverflow	= clampMode >= 2;
			SetRecompilerConfig().vuSignOverflow	= clampMode >= 3;
		}
		checkGamefix(VuAddSubHack);
		checkGamefix(VuClipFlagHack);
		checkGamefix(FpuCompareHack);
		checkGamefix(FpuMulHack);
		checkGamefix(FpuNegDivHack);
		checkGamefix(XgKickHack);
		checkGamefix(IPUWaitHack);
		checkGamefix(EETimingHack);
	}
}

extern ScopedPtr<DataBase_Loader> GameDB;
