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

class DBLoaderHelper
{
	DeclareNoncopyableObject( DBLoaderHelper );

protected:
	IGameDatabase&	m_gamedb;
	wxInputStream&	m_reader;

	// temp areas used as buffers for accelerated loading of database content.  These strings are
	// allocated and grown only once, and then reused for the duration of the database loading
	// process; saving thousands of heapp allocation operations.

	wxString		m_dest;
	std::string		m_intermediate;

	key_pair		m_keyPair;

public:	
	DBLoaderHelper( wxInputStream& reader, IGameDatabase& db )
		: m_gamedb(db)
		, m_reader(reader)
	{
	}

	wxString ReadHeader();
	void ReadGames();

protected:
	void doError(bool doMsg = false);
	bool extractMultiLine();
	void extract();
};

void DBLoaderHelper::doError(bool doMsg) {
	if (doMsg) Console.Error("GameDatabase: Bad file data [%s]", m_dest.c_str());
	m_keyPair.Clear();
}

// Multiline Sections are in the form of:
//
// [section=value]
//   content
//   content
// [/section]
//
// ... where the =value part is OPTIONAL.
bool DBLoaderHelper::extractMultiLine() {

	if (m_dest[0] != L'[') return false;		// All multiline sections begin with a '['!

	if (!m_dest.EndsWith(L"]")) {
		doError(true);
		return false;
	}

	m_keyPair.key = m_dest;

	// Use Mid() to strip off the left and right side brackets.
	wxString midLine(m_dest.Mid(1, m_dest.Length()-2));
	wxString lvalue(midLine.BeforeFirst(L'=').Trim(true).Trim(false));
	//wxString rvalue(midLine.AfterFirst(L'=').Trim(true).Trim(false));

	wxString endString;
	endString.Printf( L"[/%s]", lvalue.c_str() );

	while(!m_reader.Eof()) {
		pxReadLine( m_reader, m_dest, m_intermediate );
		if (m_dest.CmpNoCase(endString) == 0) break;
		m_keyPair.value += m_dest + L"\n";
	}
	return true;
}

void DBLoaderHelper::extract() {

	if( !pxParseAssignmentString( m_dest, m_keyPair.key, m_keyPair.value ) ) return;
	if( m_keyPair.value.IsEmpty() ) doError(true);
}

wxString DBLoaderHelper::ReadHeader()
{
	wxString header;
	header.reserve(2048);

	while(!m_reader.Eof()) {
		pxReadLine(m_reader, m_dest, m_intermediate);
		m_dest.Trim(false).Trim(true);
		if( !(m_dest.IsEmpty() || m_dest.StartsWith(L"--") || m_dest.StartsWith( L"//" ) || m_dest.StartsWith( L";" )) ) break;
		header += m_dest + L'\n';
	}

	if( !m_dest.IsEmpty() )
	{
		m_keyPair.Clear();
		if( !extractMultiLine() ) extract();
	}
	return header;
}

void DBLoaderHelper::ReadGames()
{
	Game_Data* game = NULL;

	if (m_keyPair.IsOk())
	{
		game = m_gamedb.createNewGame(m_keyPair.value);
		game->writeString(m_keyPair.key, m_keyPair.value);
		//if( m_keyPair.CompareKey(m_gamedb.getBaseKey()) )
		//	game.id = m_keyPair.value;
	}

	while(!m_reader.Eof()) { // Fill game data, find new game, repeat...
		pthread_testcancel();
		pxReadLine(m_reader, m_dest, m_intermediate);
		m_dest.Trim(true).Trim(false);
		if( m_dest.IsEmpty() ) continue;

		m_keyPair.Clear();
		if (!extractMultiLine()) extract();

		if (!m_keyPair.IsOk()) continue;
		if (m_keyPair.CompareKey(m_gamedb.getBaseKey()))
			game = m_gamedb.createNewGame(m_keyPair.value);

		game->writeString( m_keyPair.key, m_keyPair.value );
	}
}

// --------------------------------------------------------------------------------------
//  AppGameDatabase  (implementations)
// --------------------------------------------------------------------------------------

AppGameDatabase& AppGameDatabase::LoadFromFile(const wxString& file, const wxString& key )
{
	if (!wxFileExists(file))
	{
		Console.Error(L"(GameDB) Database Not Found! [%s]", file.c_str());
		return *this;
	}

	wxFFileInputStream reader( file );

	if (!reader.IsOk())
	{
		//throw Exception::FileNotFound( file );
		Console.Error(L"(GameDB) Could not access file (permission denied?) [%s]", file.c_str());
	}

	DBLoaderHelper loader( reader, *this );

	u64 qpc_Start = GetCPUTicks();
	header = loader.ReadHeader();
	loader.ReadGames();
	u64 qpc_end = GetCPUTicks();

	Console.WriteLn( "(GameDB) %d games on record (loaded in %ums)",
		gHash.size(), (u32)(((qpc_end-qpc_Start)*1000) / GetTickFrequency()) );

	return *this;
}

// Saves changes to the database
void AppGameDatabase::SaveToFile(const wxString& file) {
	wxFFileOutputStream writer( file );
	pxWriteMultiline(writer, header);

	for(uint blockidx=0; blockidx<=m_BlockTableWritePos; ++blockidx)
	{
		if( !m_BlockTable[blockidx] ) continue;

		const uint endidx = (blockidx == m_BlockTableWritePos) ? m_CurBlockWritePos : m_GamesPerBlock;

		for( uint gameidx=0; gameidx<=endidx; ++gameidx )
		{
			const Game_Data& game( m_BlockTable[blockidx][gameidx] );
			KeyPairArray::const_iterator i(game.kList.begin());
			for ( ; i != game.kList.end(); ++i) {
				pxWriteMultiline(writer, i->toString() );
			}

			pxWriteLine(writer, L"---------------------------------------------");
		}
	}
}

AppGameDatabase* Pcsx2App::GetGameDatabase()
{
	pxAppResources& res( GetResourceCache() );

	ScopedLock lock( m_mtx_LoadingGameDB );
	if( !res.GameDB )
	{
		res.GameDB = new AppGameDatabase();
		res.GameDB->LoadFromFile();
	}
	return res.GameDB;
}

IGameDatabase* AppHost_GetGameDatabase()
{
	return wxGetApp().GetGameDatabase();
}
