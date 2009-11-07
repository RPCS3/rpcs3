#include "PrecompiledHeader.h"

#include "IsoFS.h"

#include <vector>

using namespace std;

void Tokenize(const string& str, vector<string>& tokens, const string& delimiters = " ")
{
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	string::size_type pos     = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos)
	{
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		lastPos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, lastPos);
	}
}

//////////////////////////////////////////////////////////////////////////
// IsoDirectory
//////////////////////////////////////////////////////////////////////////

IsoDirectory::IsoDirectory(SectorSource* r)
{
	byte sector[2048];

	r->readSector(sector,16);

	IsoFileDescriptor rootDirEntry(sector+156,38);

	Init(r, rootDirEntry);
}

IsoDirectory::IsoDirectory(SectorSource* r, IsoFileDescriptor directoryEntry)
{
	Init(r, directoryEntry);
}

void IsoDirectory::Init(SectorSource* r, IsoFileDescriptor directoryEntry)
{
	// parse directory sector
	IsoFile dataStream (r, directoryEntry);

	internalReader = r;

	files.clear();

	int remainingSize = directoryEntry.size;

	byte b[257];

	while(remainingSize>=4) // hm hack :P
	{
		b[0] = dataStream.read<byte>();

		if(b[0]==0)
		{
			break; // or continue?
		}

		remainingSize-=b[0];

		dataStream.read(b+1, b[0]-1);

		IsoFileDescriptor file(b,b[0]);
		files.push_back(file);
	}

	b[0] = 0;
}

IsoFileDescriptor IsoDirectory::GetEntry(int index)
{
	return files[index];
}

int IsoDirectory::GetIndexOf(string fileName)
{
	for(unsigned int i=0;i<files.size();i++)
	{
		string file = files[i].name;
		if(file.compare(fileName)==0)
		{
			return i;
		}
	}

	throw Exception::FileNotFound( fileName.c_str() );
}

IsoFileDescriptor IsoDirectory::GetEntry(string fileName)
{
	return GetEntry(GetIndexOf(fileName));
}

IsoFileDescriptor IsoDirectory::FindFile(string filePath)
{
	IsoFileDescriptor info;
	IsoDirectory dir = *this;

	// this was supposed to be a vector<string>, but this damn hotfix kills stl
	// if you have the Windows SDK 6.1 installed after vs2008 sp1
	vector<string> path;

	Tokenize(filePath,path,"\\/");

	// walk through path ("." and ".." entries are in the directories themselves, so even if the path included . and/or .., it should still work)
	for(int i=0;i<path.size();i++)
	{
		string pathName = path[i];
		info = dir.GetEntry(pathName);
		if((info.flags&2)==2) // if it's a directory
		{
			dir  = IsoDirectory(internalReader, info);
		}
	}

	return info;
}

IsoFile IsoDirectory::OpenFile(string filePath)
{
	IsoFileDescriptor info = FindFile(filePath);
	if((info.flags&2)==2) // if it's a directory
	{
		throw Exception::InvalidArgument("Filename points to a directory.");
	}

	return IsoFile(internalReader, info);
}

IsoDirectory::~IsoDirectory(void)
{
}

//////////////////////////////////////////////////////////////////////////
// IsoFileDescriptor
//////////////////////////////////////////////////////////////////////////

IsoFileDescriptor::IsoFileDescriptor()
{
	lba = 0;
	size = 0;
	flags = 0;
	name = "";
}

IsoFileDescriptor::IsoFileDescriptor(const byte* data, int length)
{
	lba = *(const int*)(data+2);
	size = *(const int*)(data+10);
	date.year      = data[18] + 1900;
	date.month     = data[19];
	date.day       = data[20];
	date.hour      = data[21];
	date.minute    = data[22];
	date.second    = data[23];
	date.gmtOffset = data[24];

	flags = data[25];

	if((lba<0)||(length<0))
	{
		// would be nice to have some exceptio nthis fits in
		throw Exception::InvalidOperation("WTF?! Size or lba < 0?!");
	}

	int fileNameLength = data[32];

	name.clear();

	if(fileNameLength==1)
	{
		char c = *(const char*)(data+33);

		switch(c)
		{
		case 0:	name = "."; break;
		case 1: name = ".."; break;
		default: name.append(1,c);
		}
	}
	else if (fileNameLength>0)
		name.append((const char*)(data+33),fileNameLength);
}
