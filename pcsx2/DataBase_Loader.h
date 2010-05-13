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

struct key_pair {
	string key;
	string value;
	key_pair(string _key, string _value)
		: key(_key) , value(_value) {}
};

class Game_Data {
public:
	string id;				// Serial Identification Code 
	deque<key_pair> kList;	// List of all (key, value) pairs for game data
	Game_Data(string _id) 
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
	bool isComment(string& s) {
		string sub = s.substr(0, 2);
		return (sub.compare("--") == 0) || (sub.compare("//") == 0);
	}
	void doError(string& line, key_pair& keyPair, bool doMsg = false) {
		if (doMsg) Console.Error("DataBase_Loader: Bad file data [%s]", line.c_str());
		keyPair.key = "";
	}
	void extract(string& line, key_pair& keyPair) {
		int eol = line.rfind("\r");
		if (eol != string::npos) line = line.substr(0, eol);
		
		stringstream ss(line);
		string t;
		keyPair.key   = "";
		keyPair.value = "";
		ss >> keyPair.key;
		if (!line.length() || isComment(keyPair.key)) {
			doError(line, keyPair);
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
			keyPair.value += " ";
			keyPair.value += t;
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

	DataBase_Loader(string file, string key, string value = "") {
		curGame	= NULL;
		baseKey = key;
		if (!fileExists(file)) {
			Console.Error("DataBase_Loader: DataBase Not Found! [%s]", file.c_str());
		}
		File_Reader   reader(file);
		key_pair      keyPair("", "");
		string        s0;
		try {
			for(;;) {
				for(;;) { // Find first game
					s0 = reader.getLine();
					extract(s0, keyPair);
					if (keyPair.key.compare(key) == 0) break;
					header.write(s0);
					header.write("\n");
				}
				Game_Data* game = new Game_Data(keyPair.value);
				game->kList.push_back(keyPair);
				for (;;) { // Fill game data, find new game, repeat...
					s0 = reader.getLine();
					extract(s0, keyPair);
					if (keyPair.key.compare("")  == 0) continue;
					if (keyPair.key.compare(key) == 0) {
						gList.push_back(game);
						game = new Game_Data(keyPair.value);
					}
					game->kList.push_back(keyPair);
				}
			}
		}
		catch(int& i) { i = 0; }
		if (!value.compare("")) return;
		if (setGame(value)) Console.WriteLn("DataBase_Loader: Found Game! [%s]",     value.c_str());
		else				Console.Warning("DataBase_Loader: Game Not Found! [%s]", value.c_str());
	}

	~DataBase_Loader() {
		deque<Game_Data*>::iterator it = gList.begin();
		for ( ; it != gList.end(); ++it) {
			delete it[0];
		}
	}

	// Sets the current game to the one matching the serial id given
	// Returns true if game found, false if not found...
	bool setGame(string id) {
		deque<Game_Data*>::iterator it = gList.begin();
		for ( ; it != gList.end(); ++it) {
			if (!it[0]->id.compare(id)) {
				curGame = it[0];
				return true;
			}
		}
		curGame = NULL;
		return false;
	}

	// Saves changes to the database
	void saveToFile(string file = "DataBase.dbf") {
		File_Writer writer(file);
		writer.write(header.toString());
		deque<Game_Data*>::iterator it = gList.begin();
		for ( ; it != gList.end(); ++it) {
			deque<key_pair>::iterator i = it[0]->kList.begin();
			for ( ; i != it[0]->kList.end(); ++i) {
				writer.write(i[0].key);
				for (int a = 6 - i[0].key.length(); a > 0; a--) {
					writer.write(" "); // Padding for nice formatting on small key-names
				}
				writer.write(" = ");
				writer.write(i[0].value);
				writer.write("\n");
			}
			writer.write("---------------------------------------------\n");
		}
	}

	// Adds new game data to the database, and sets curGame to the new game...
	// If searchDB is true, it searches the database to see if game already exists.
	void addGame(string id, bool searchDB = true) {
		if (searchDB && setGame(id)) return;
		Game_Data* game = new Game_Data(id);
		key_pair   kp(baseKey, id);
		game->kList.push_back(kp);
		gList.push_back(game);
		curGame = game;
	}

	// Gets a string representation of the 'value' for the given key
	string getString(string key) {
		if (curGame) {
			deque<key_pair>::iterator it = curGame->kList.begin();
			for ( ; it != curGame->kList.end(); ++it) {
				if (!it[0].key.compare(key)) {
					return it[0].value;
				}
			}
		}
		else Console.Error("DataBase_Loader: Game not set!");
		return string("");
	}

	// Gets a wxString representation of the 'value' for the given key
	wxString getStringWX(string key) {
		string s = getString(key);
		return wxString(fromAscii(s.c_str()));
	}

	// Gets a double representation of the 'value' for the given key
	double getDouble(string key) {
		string v = getString(key);
		return atof(v.c_str());
	}

	// Gets a float representation of the 'value' for the given key
	float getFloat(string key) {
		string v = getString(key);
		return (float)atof(v.c_str());
	}

	// Gets an integer representation of the 'value' for the given key
	int getInt(string key) {
		string v = getString(key);
		return strtoul(v.c_str(), NULL, 0);
	}

	// Gets a u8 representation of the 'value' for the given key
	u8 getU8(string key) {
		string v = getString(key);
		return (u8)atoi(v.c_str());
	}

	// Gets a bool representation of the 'value' for the given key
	u8 getBool(string key) {
		string v = getString(key);
		return !!atoi(v.c_str());
	}

	// Write a string value to the specified key
	void writeString(string key, string value) {
		if (curGame) {
			deque<key_pair>::iterator it = curGame->kList.begin();
			for ( ; it != curGame->kList.end(); ++it) {
				if (!it[0].key.compare(key)) {
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
	void writeStringWX(string key, wxString value) {
		writeString(key, string(value.ToUTF8().data()));
	}

	// Write a double value to the specified key
	void writeDouble(string key, double value) {
		writeString(key, toString(value));
	}
	
	// Write a float value to the specified key
	void writeFloat(string key, float value) {
		writeString(key, toString(value));
	}
	
	// Write an integer value to the specified key
	void writeInt(string key, int value) {
		writeString(key, toString(value));
	}
	
	// Write a u8 value to the specified key
	void writeU8(string key, u8 value) {
		writeString(key, toString(value));
	}
	
	// Write a bool value to the specified key
	void writeBool(string key, bool value) {
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

extern ScopedPtr<DataBase_Loader> GameDB;
