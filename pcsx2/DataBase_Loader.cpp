
#include "PrecompiledHeader.h"
#include "DataBase_Loader.h"

//------------------------------------------------------------------
// DataBase_Loader - Private Methods
//------------------------------------------------------------------

//template<class T> string DataBase_Loader::toString(const T& value);

string DataBase_Loader::toLower(const string& s) {
	string retval( s );
	for (uint i = 0; i < s.length(); i++) {
		char& c = retval[i];
		if (c >= 'A' && c <= 'Z') {
			c += 'a' - 'A';
		}
	}
	return retval;
}

bool DataBase_Loader::strCompare(const string& s1, const string& s2) {
	const string t1( toLower(s1) );
	const string t2( toLower(s2) );
	return !t1.compare(t2);
}

bool DataBase_Loader::isComment(const string& s) {
	const string sub( s.substr(0, 2) );
	return (sub.compare("--") == 0) || (sub.compare("//") == 0);
}

void DataBase_Loader::doError(const string& line, key_pair& keyPair, bool doMsg) {
	if (doMsg) Console.Error("DataBase_Loader: Bad file data [%s]", line.c_str());
	keyPair.key.clear();
}

void DataBase_Loader::extractMultiLine(const string& line, key_pair& keyPair, File_Reader& reader, const stringstream& ss) {
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

void DataBase_Loader::extract(const string& line, key_pair& keyPair, File_Reader& reader) {
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
