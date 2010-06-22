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

//#include "Common.h"
#include "AppConfig.h"
#include "Utilities/HashMap.h"

#include <wx/wfstream.h>

struct	key_pair;
struct	Game_Data;

typedef std::vector<key_pair>	KeyPairArray;

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

// --------------------------------------------------------------------------------------
//  Game_Data
// --------------------------------------------------------------------------------------
// This container is more or less required to be a simple struct (POD classification) --
// no virtuals and no inheritance.  This is because it is used in a std::vector, so POD
// makes things... smoother.
struct Game_Data
{
	wxString		id;				// Serial Identification Code
	KeyPairArray	kList;			// List of all (key, value) pairs for game data

	Game_Data(const wxString& _id = wxEmptyString)
		: id(_id) {}
	
	// Performs a case-insensitive compare of two IDs, returns TRUE if the IDs match
	// or FALSE if the ids differ in a case-insensitive way.
	bool CompareId( const wxString& _id ) const {
		return id.CmpNoCase(_id) == 0;
	}
	
	void clear() {
		id.clear();
		kList.clear();
	}

	bool keyExists(const wxChar* key);
	void deleteKey(const wxChar* key);
	wxString getString(const wxChar* key);
	void writeString(const wxString& key, const wxString& value);
	void writeBool(const wxString& key, bool value);

	bool IsOk() const {
		return !id.IsEmpty();
	}

	bool sectionExists(const wxChar* key, const wxString& value) {
		return keyExists(wxsFormat(L"[%s%s%s]", key, value.IsEmpty() ? L"" : L" = ", value.c_str()));
	}

	wxString getSection(const wxChar* key, const wxString& value) {
		return getString(wxsFormat(L"[%s%s%s]", key, value.IsEmpty() ? L"" : L" = ", value.c_str()));
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

	bool keyExists(const char* key) {
		return keyExists(fromUTF8(key));
	}

	wxString getString(const char* key) {
		return getString(fromUTF8(key));
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
};

// --------------------------------------------------------------------------------------
//  IGameDatabase
// --------------------------------------------------------------------------------------
class IGameDatabase
{
public:
	virtual ~IGameDatabase() throw() {}

	virtual wxString getBaseKey() const=0;
	virtual bool findGame(Game_Data& dest, const wxString& id)=0;
	virtual void addNewGame(const Game_Data& game)=0;
	virtual void updateGame(const Game_Data& game)=0;
};

class StringHashNoCase
{
public:
	StringHashNoCase() {}

	HashTools::hash_key_t operator()( const wxString& src ) const
	{
		return HashTools::Hash( (const char *)src.Lower().data(), src.length() * sizeof( wxChar ) );
	}
};

typedef std::vector<Game_Data>				GameDataArray;
typedef pxDictionary<int,StringHashNoCase>	GameDataHash;

// --------------------------------------------------------------------------------------
//  BaseGameDatabaseImpl 
// --------------------------------------------------------------------------------------
// [TODO] Create a version of this that uses google hashsets; should be several magnitudes
// faster that way.
class BaseGameDatabaseImpl : public IGameDatabase
{
public:
	GameDataArray	gList;			// List of all game data
	GameDataHash	gHash;			// hash table of game serials matched to their gList indexes!
	wxString		m_baseKey;

public:
	BaseGameDatabaseImpl()
	{
		m_baseKey = L"Serial";
	}

	virtual ~BaseGameDatabaseImpl() throw() {}

	wxString getBaseKey() const { return m_baseKey; }
	void setBaseKey( const wxString& key ) { m_baseKey = key; }

	bool findGame(Game_Data& dest, const wxString& id);
	void addNewGame(const Game_Data& game);
	void updateGame(const Game_Data& game);
};

extern IGameDatabase* AppHost_GetGameDatabase();
