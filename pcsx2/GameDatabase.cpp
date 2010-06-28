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

BaseGameDatabaseImpl::BaseGameDatabaseImpl()
	: gHash( 9400 )
	, m_baseKey( L"Serial" )
{
	m_BlockTable.reserve(48);

	m_CurBlockWritePos		= 0;
	m_BlockTableWritePos	= 0;

	m_GamesPerBlock			= 256;

	m_BlockTable.push_back(NULL);
}

BaseGameDatabaseImpl::~BaseGameDatabaseImpl() throw()
{
	for(uint blockidx=0; blockidx<=m_BlockTableWritePos; ++blockidx)
	{
		if( !m_BlockTable[blockidx] ) continue;

		const uint endidx = (blockidx == m_BlockTableWritePos) ? m_CurBlockWritePos : m_GamesPerBlock;

		for( uint gameidx=0; gameidx<endidx; ++gameidx )
			m_BlockTable[blockidx][gameidx].~Game_Data();

		safe_free( m_BlockTable[blockidx] );
	}
}

// Sets the current game to the one matching the serial id given
// Returns true if game found, false if not found...
bool BaseGameDatabaseImpl::findGame(Game_Data& dest, const wxString& id) {

	GameDataHash::const_iterator iter( gHash.find(id) );
	if( iter == gHash.end() ) {
		dest.clear();
		return false;
	}
	dest = *iter->second;
	return true;
}

Game_Data* BaseGameDatabaseImpl::createNewGame( const wxString& id )
{
	if(!m_BlockTable[m_BlockTableWritePos])
		m_BlockTable[m_BlockTableWritePos] = (Game_Data*)malloc(m_GamesPerBlock * sizeof(Game_Data));

	Game_Data* block = m_BlockTable[m_BlockTableWritePos];
	Game_Data* retval = &block[m_CurBlockWritePos];

	gHash[id] = retval;

	new (retval) Game_Data(id);

	if( ++m_CurBlockWritePos >= m_GamesPerBlock )
	{
		++m_BlockTableWritePos;
		m_CurBlockWritePos = 0;
		m_BlockTable.push_back(NULL);
	}

	return retval;
}

void BaseGameDatabaseImpl::updateGame(const Game_Data& game)
{
	GameDataHash::const_iterator iter( gHash.find(game.id) );

	if( iter == gHash.end() ) {
		*(createNewGame( game.id )) = game;
	}
	else
	{
		// Re-assign existing vector/array entry!
		*gHash[game.id] = game;
	}
}

// Searches the current game's data to see if the given key exists
bool Game_Data::keyExists(const wxChar* key) const {
	KeyPairArray::const_iterator it( kList.begin() );
	for ( ; it != kList.end(); ++it) {
		if (it->CompareKey(key)) {
			return true;
		}
	}
	return false;
}

// Totally Deletes the specified key/pair value from the current game's data
void Game_Data::deleteKey(const wxChar* key) {
	KeyPairArray::iterator it( kList.begin() );
	for ( ; it != kList.end(); ++it) {
		if (it->CompareKey(key)) {
			kList.erase(it);
			return;
		}
	}
}

// Gets a string representation of the 'value' for the given key
wxString Game_Data::getString(const wxChar* key) const {
	KeyPairArray::const_iterator it( kList.begin() );
	for ( ; it != kList.end(); ++it) {
		if (it->CompareKey(key)) {
			return it->value;
		}
	}
	return wxString();
}

void Game_Data::writeString(const wxString& key, const wxString& value) {
	KeyPairArray::iterator it( kList.begin() );
	for ( ; it != kList.end(); ++it) {
		if (it->CompareKey(key)) {
			if( value.IsEmpty() )
				kList.erase(it);
			else
				it->value = value;
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