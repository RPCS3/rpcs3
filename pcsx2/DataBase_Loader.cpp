
#include "PrecompiledHeader.h"
#include "DataBase_Loader.h"

//------------------------------------------------------------------
// DataBase_Loader - Private Methods
//------------------------------------------------------------------

void DataBase_Loader::doError(const wxString& line, key_pair& keyPair, bool doMsg) {
	if (doMsg) Console.Error("DataBase_Loader: Bad file data [%s]", line.c_str());
	keyPair.Clear();
}

// Multiline Sections are in the form of:
//
// [section=value]
//   content
//   content
// [/section]
//
// ... where the =value part is OPTIONAL.
bool DataBase_Loader::extractMultiLine(const wxString& line, key_pair& keyPair, wxInputStream& ffile) {

	if (line[0] != L'[') return false;		// All multiline sections begin with a '['!

	if (!line.EndsWith(L"]")) {
		doError(line, keyPair, true);
		return false;
	}

	keyPair.key = line;

	// Use Mid() to strip off the left and right side brackets.
	wxString midLine(line.Mid(1, line.Length()-2));
	wxString lvalue(midLine.BeforeFirst(L'=').Trim(true).Trim(false));
	//wxString rvalue(midLine.AfterFirst(L'=').Trim(true).Trim(false));

	wxString endString;
	endString.Printf( L"[/%s]", lvalue.c_str() );

	while(!ffile.Eof()) {
		pxReadLine( ffile, m_dest, m_intermediate );
		if (m_dest == endString) break;
		keyPair.value += m_dest + L"\n";
	}
	return true;
}

void DataBase_Loader::extract(const wxString& line, key_pair& keyPair, wxInputStream& reader) {
	keyPair.Clear();

	if( line.IsEmpty() ) return;

	if( extractMultiLine(line, keyPair, reader) ) return;
	if( !pxParseAssignmentString( line, keyPair.key, keyPair.value ) ) return;
	if( keyPair.value.IsEmpty() )
		doError(line, keyPair, true);
}
