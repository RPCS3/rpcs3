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

#include "IsoFS.h"
#include "IsoFile.h"

//////////////////////////////////////////////////////////////////////////
// IsoDirectory
//////////////////////////////////////////////////////////////////////////

//u8		filesystemType;	// 0x01 = ISO9660, 0x02 = Joliet, 0xFF = NULL
//u8		volID[5];		// "CD001"


wxString IsoDirectory::FStype_ToString() const
{
	switch( m_fstype )
	{
		case FStype_ISO9660:	return L"ISO9660";		break;
		case FStype_Joliet:		return L"Joliet";		break;
	}

	return wxsFormat( L"Unrecognized Code (0x%x)", m_fstype );
}

// Used to load the Root directory from an image
IsoDirectory::IsoDirectory(SectorSource& r)
	: internalReader(r)
{
	IsoFileDescriptor rootDirEntry;
	bool isValid = false;
	bool done = false;
	uint i = 16;

	m_fstype = FStype_ISO9660;

	while( !done )
	{
		u8 sector[2048];
		internalReader.readSector(sector,i);
		if( memcmp( &sector[1], "CD001", 5 ) == 0 )
		{
		    switch (sector[0])
		    {
		        case 0:
                    DevCon.WriteLn( Color_Green, "(IsoFS) Block 0x%x: Boot partition info.", i );
                    break;

                case 1:
                    DevCon.WriteLn( "(IsoFS) Block 0x%x: Primary partition info.", i );
                    rootDirEntry.Load( sector+156, 38 );
                    isValid = true;
                    break;

                case 2:
                    // Probably means Joliet (long filenames support), which PCSX2 doesn't care about.
                    DevCon.WriteLn( Color_Green, "(IsoFS) Block 0x%x: Extended partition info.", i );
                    m_fstype = FStype_Joliet;
                    break;

				case 0xff:
					// Null terminator.  End of partition information.
					done = true;
				break;

				default:
					Console.Error( "(IsoFS) Unknown partition type ID=%d, encountered at block 0x%x", sector[0], i );
				break;
			}
		}
		else
		{
			sector[9] = 0;
			Console.Error( "(IsoFS) Invalid partition descriptor encountered at block 0x%x: '%s'", i, &sector[1] );
			break;		// if no valid root partition was found, an exception will be thrown below.
		}

		++i;
	}

	if( !isValid )
		throw Exception::FileNotFound(L"IsoFileSystem")	// FIXME: Should report the name of the ISO here...
			.SetDiagMsg(L"IsoFS could not find the root directory on the ISO image.");

	DevCon.WriteLn( L"(IsoFS) Filesystem is " + FStype_ToString() );
	Init( rootDirEntry );
}

// Used to load a specific directory from a file descriptor
IsoDirectory::IsoDirectory(SectorSource& r, IsoFileDescriptor directoryEntry)
	: internalReader(r)
{
	Init(directoryEntry);
}

IsoDirectory::~IsoDirectory() throw()
{
}

void IsoDirectory::Init(const IsoFileDescriptor& directoryEntry)
{
	// parse directory sector
	IsoFile dataStream (internalReader, directoryEntry);

	files.clear();

	uint remainingSize = directoryEntry.size;

	u8 b[257];

	while(remainingSize>=4) // hm hack :P
	{
		b[0] = dataStream.read<u8>();

		if(b[0]==0)
		{
			break; // or continue?
		}

		remainingSize -= b[0];

		dataStream.read(b+1, b[0]-1);

		files.push_back(IsoFileDescriptor(b, b[0]));
	}

	b[0] = 0;
}

const IsoFileDescriptor& IsoDirectory::GetEntry(int index) const
{
	return files[index];
}

int IsoDirectory::GetIndexOf(const wxString& fileName) const
{
	for(unsigned int i=0;i<files.size();i++)
	{
		if(files[i].name == fileName) return i;
	}

	throw Exception::FileNotFound(fileName);
}

const IsoFileDescriptor& IsoDirectory::GetEntry(const wxString& fileName) const
{
	return GetEntry(GetIndexOf(fileName));
}

IsoFileDescriptor IsoDirectory::FindFile(const wxString& filePath) const
{
	pxAssert( !filePath.IsEmpty() );

	// wxWidgets DOS-style parser should work fine for ISO 9660 path names.  Only practical difference
	// is case sensitivity, and that won't matter for path splitting.
	wxFileName parts( filePath, wxPATH_DOS );
	IsoFileDescriptor info;
	const IsoDirectory* dir = this;
	ScopedPtr<IsoDirectory> deleteme;

	// walk through path ("." and ".." entries are in the directories themselves, so even if the
	// path included . and/or .., it still works)

	for(uint i=0; i<parts.GetDirCount(); ++i)
	{
		info = dir->GetEntry(parts.GetDirs()[i]);
		if(info.IsFile()) throw Exception::FileNotFound( filePath );

		dir = deleteme = new IsoDirectory(internalReader, info);
	}

	if( !parts.GetFullName().IsEmpty() )
		info = dir->GetEntry(parts.GetFullName());

	return info;
}

bool IsoDirectory::IsFile(const wxString& filePath) const
{
	if( filePath.IsEmpty() ) return false;
	return (FindFile(filePath).flags&2) != 2;
}

bool IsoDirectory::IsDir(const wxString& filePath) const
{
	if( filePath.IsEmpty() ) return false;
	return (FindFile(filePath).flags&2) == 2;
}

u32 IsoDirectory::GetFileSize( const wxString& filePath ) const
{
	return FindFile( filePath ).size;
}

IsoFileDescriptor::IsoFileDescriptor()
{
	lba = 0;
	size = 0;
	flags = 0;
}

IsoFileDescriptor::IsoFileDescriptor(const u8* data, int length)
{
	Load( data, length );
}

void IsoFileDescriptor::Load( const u8* data, int length )
{
	lba		= (u32&)data[2];
	size	= (u32&)data[10];

	date.year      = data[18] + 1900;
	date.month     = data[19];
	date.day       = data[20];
	date.hour      = data[21];
	date.minute    = data[22];
	date.second    = data[23];
	date.gmtOffset = data[24];

	flags = data[25];

	int fileNameLength = data[32];

	if(fileNameLength==1)
	{
		u8 c = data[33];

		switch(c)
		{
			case 0:	name = L"."; break;
			case 1: name = L".."; break;
			default: name = (wxChar)c;
		}
	}
	else
	{
		// copy string and up-convert from ascii to wxChar

		const u8* fnsrc = data+33;
		const u8* fnend = fnsrc+fileNameLength;

		while( fnsrc != fnend )
		{
			name += (wxChar)*fnsrc;
			++fnsrc;
		}
	}
}
