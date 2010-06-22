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
#include "GameDatabase.h"

// Sets the current game to the one matching the serial id given
// Returns true if game found, false if not found...
bool BaseGameDatabaseImpl::findGame(Game_Data& dest, const wxString& id) {

	GameDataHash::const_iterator iter( gHash.find(id) );
	if( iter == gHash.end() ) {
		dest.clear();
		return false;
	}
	dest = gList[iter->second];
	return true;
}

void BaseGameDatabaseImpl::addNewGame(const Game_Data& game)
{
	gList.push_back(game);
	gHash[game.id] = gList.size()-1;
}

void BaseGameDatabaseImpl::updateGame(const Game_Data& game)
{
	GameDataHash::const_iterator iter( gHash.find(game.id) );

	if( iter == gHash.end() ) {
		gList.push_back(game);
		gHash[game.id] = gList.size()-1;
	}
	else
	{
		// Re-assign existing vector/array entry!
		gList[gHash[game.id]] = game;
	}
}

// Searches the current game's data to see if the given key exists
bool Game_Data::keyExists(const wxChar* key) {
	KeyPairArray::iterator it( kList.begin() );
	for ( ; it != kList.end(); ++it) {
		if (it[0].CompareKey(key)) {
			return true;
		}
	}
	return false;
}

// Totally Deletes the specified key/pair value from the current game's data
void Game_Data::deleteKey(const wxChar* key) {
	KeyPairArray::iterator it( kList.begin() );
	for ( ; it != kList.end(); ++it) {
		if (it[0].CompareKey(key)) {
			kList.erase(it);
			return;
		}
	}
}

// Gets a string representation of the 'value' for the given key
wxString Game_Data::getString(const wxChar* key) {
	KeyPairArray::iterator it( kList.begin() );
	for ( ; it != kList.end(); ++it) {
		if (it[0].CompareKey(key)) {
			return it[0].value;
		}
	}
	return wxString();
}

void Game_Data::writeString(const wxString& key, const wxString& value) {
	KeyPairArray::iterator it( kList.begin() );
	for ( ; it != kList.end(); ++it) {
		if (it[0].CompareKey(key)) {
			if( value.IsEmpty() )
				kList.erase(it);
			else
				it[0].value = value;
			return;
		}
	}
	if( !value.IsEmpty() ) {
		kList.push_back(key_pair(key, value));
	}
}

// Write a bool value to the specified key
void Game_Data::writeBool(const wxString& key, bool value) {
	writeString(key, value ? L"1" : L"0");
}