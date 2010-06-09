
#include "PrecompiledHeader.h"
#include "DataBase_Loader.h"

//------------------------------------------------------------------
// DataBase_Loader - Private Methods
//------------------------------------------------------------------

void DataBase_Loader::doError(const wxString& line, key_pair& keyPair, bool doMsg) {
	if (doMsg) Console.Error("DataBase_Loader: Bad file data [%s]", line.c_str());
	keyPair.key.clear();
}

// Multiline Sections are in the form of:
//
// [section=value]
//   content
//   content
// [/section]
//
// ... where the =value part is OPTIONAL.
void DataBase_Loader::extractMultiLine(key_pair& keyPair, wxInputStream& ffile) {

	if (!keyPair.key.EndsWith(L"]")) {
		doError(keyPair.key, keyPair, true);
		return;
	}

	// Use Mid() to strip off the left and right side brackets.
	ParsedAssignmentString set( keyPair.key.Mid(1, keyPair.key.Length()-2) );
	
	wxString endString;
	endString.Printf( L"[/%s]", set.lvalue.c_str() );

	for(;;) {
		pxReadLine( ffile, m_dest, m_intermediate );
		if (m_dest == endString) break;
		keyPair.value += m_dest + L"\n";
	}
}

void DataBase_Loader::extract(const wxString& line, key_pair& keyPair, wxInputStream& reader) {
	keyPair.key = line;
	keyPair.value.clear();

	if( line.IsEmpty() ) return;

	if (keyPair.key[0] == L'[') {
		extractMultiLine(keyPair, reader);
		return;
	}
	
	if( !pxParseAssignmentString( line, keyPair.key, keyPair.value ) ) return;
	if( keyPair.value.IsEmpty() )
		doError(line, keyPair, true);
}
