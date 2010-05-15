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

using namespace std;

class File_Reader {
private:
	char buff[2048];
	template<class T> T _read() {
		if (fs->eof())  throw 1;
		T t; (*fs) >> t; 
		if (fs->fail()) throw 1;
		return t;
	}
public:
	fstream* fs;
	File_Reader(string filename) {
		fs = new fstream(filename.c_str(), ios_base::in);
	}
	~File_Reader() {
		if (fs) fs->close();
		delete fs;
	}
	template<class T> void read(T &t) {
		long pos = fs->tellp();
		string s = _read<string>(); 
		if (s.length() >= 2) {
			if (s[0] == '/' && s[1] == '/') {
				fs->seekp(pos);
				fs->getline(buff, 1024);
				read(t);
				return;
			}
		}
		fs->seekp(pos);
		t = _read<T>();
	}
	void readRaw(void* ptr, int size) {
		u8* p = (u8*)ptr;
		for (int i = 0; i < size; i++) {
			p[i] = _read<u8>();
		}
	}
	void ignoreLine() {
		fs->getline(buff, sizeof(buff));
	}
	string getLine() {
		if (fs->eof())  throw 1;
		fs->getline(buff, sizeof(buff));
		if (fs->fail()) throw 1;
		return string(buff);
	}
	template<class T> void readLine(T& str) {
		if (fs->eof())  throw 1;
		fs->getline(buff, sizeof(buff));
		if (fs->fail()) throw 1;
		string t(buff);
		str = t;
	}
};

class File_Writer {
public:
	fstream* fs;
	File_Writer(string filename) {
		fs = new fstream(filename.c_str(), ios_base::out);
	}
	~File_Writer() {
		if (fs) fs->close();
		delete fs;
	}
	template<class T> void write(T t) {
		(*fs) << t;
	}
	void writeRaw(void* ptr, int size) {
		u8* p = (u8*)ptr;
		for (int i = 0; i < size; i++) {
			write(p[i]);
		}
	}
};

class String_Stream {
private:
	char buff[2048];
public:
	stringstream* ss;
	String_Stream()  {
		ss = new stringstream(stringstream::in | stringstream::out);
	}
	String_Stream(string& str) {
		ss = new stringstream(str, stringstream::in | stringstream::out);
	}
	~String_Stream() {
		delete ss;
	}
	template<class T> void write(T t) {
		(*ss) << t;
	}
	template<class T> void read(T &t) {
		(*ss) >> t;
	}
	string toString() {
		return ss->str();
	}
	string getLine() {
		ss->getline(buff, sizeof(buff));
		return string(buff);
	}
	wxString getLineWX() {
		ss->getline(buff, sizeof(buff));
		return wxString(fromAscii(buff));
	}
	bool finished() {
		return ss->eof() || ss->fail();
	}
};

static bool fileExists(string file) {
	FILE  *f = fopen(file.c_str(), "r");
	if   (!f)  return false;
	fclose(f);
	return true;
}
