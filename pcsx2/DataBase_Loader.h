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

#include "File_Reader.h"

struct key_pair {
	string key;
	string value;
	key_pair(string _key, string _value)
		: key(_key) , value(_value) {}
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
	bool isComment(string& s) {
		string sub = s.substr(0, 2);
		return (sub.compare("--") == 0) || (sub.compare("//") == 0);
	}
	void doError(string& line, key_pair& keyPair, bool doMsg = false) {
		if (doMsg) Console.Error("DataBase_Loader: Bad file data [%s]", line.c_str());
		keyPair.key = "";
	}
	void extract(string& line, key_pair& keyPair) {
		stringstream ss(line);
		string t;
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
		for (;!ss.eof() && !ss.fail();) {
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
	deque<key_pair> kList;
	DataBase_Loader(string file, string key, string value) {
		if (!fileExists(file)) {
			Console.Error("DataBase_Loader: DataBase Not Found! [%s]", file.c_str());
		}
		File_Reader reader(file);
		key_pair    keyPair("", "");
		string      s0, s1;
		try {
			for(;;) {
				keyPair.key   = "";
				keyPair.value = "";
				s0 = reader.getLine();
				extract(s0, keyPair);
				if ((keyPair.key.compare(key)     == 0) 
				&&	(keyPair.value.compare(value) == 0)) break;
			}
			Console.WriteLn("DataBase_Loader: Found Game! [%s]", value.c_str());
			kList.push_back(keyPair);
			for (;;) {
				s0 = reader.getLine();
				extract(s0, keyPair);
				if (keyPair.key.compare("")  == 0) continue;
				if (keyPair.key.compare(key) == 0) break;
				kList.push_back(keyPair);
			}
		}
		catch(...) {}
		if (!kList.size()) Console.Warning("DataBase_Loader: Game Not Found! [%s]", value.c_str());
	}
	~DataBase_Loader() {}
	string getString(string key) {
		deque<key_pair>::iterator it = kList.begin();
		for ( ; it != kList.end(); ++it) {
			if (!it[0].key.compare(key)) {
				return it[0].value;
			}
		}
		return string("???");
	}
	double getDouble(string key) {
		string v = getString(key);
		return atof(v.c_str());
	}
	float getFloat(string key) {
		string v = getString(key);
		return (float)atof(v.c_str());
	}
	int getInt(string key) {
		string v = getString(key);
		return strtoul(v.c_str(), NULL, 0);
	}
	u8 getU8(string key) {
		string v = getString(key);
		return (u8)atoi(v.c_str());
	}
};
