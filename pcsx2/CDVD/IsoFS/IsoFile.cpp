/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

IsoFile::IsoFile(SectorSource& reader, const wxString& filename)
	: internalReader(reader)
	, fileEntry(IsoDirectory(reader).FindFile(filename))
{
	Init();
}

IsoFile::IsoFile(const IsoDirectory& dir, const wxString& filename)
	: internalReader(dir.GetReader())
	, fileEntry(dir.FindFile(filename))
{
	Init();
}

IsoFile::IsoFile(SectorSource& reader, const IsoFileDescriptor& fileEntry)
	: internalReader(reader)
	, fileEntry(fileEntry)
{
	Init();
}

void IsoFile::Init()
{
	//pxAssertDev( fileEntry.IsFile(), "IsoFile Error: Filename points to a directory." );

	currentSectorNumber	= fileEntry.lba;
	currentOffset		= 0;
	sectorOffset		= 0;
	maxOffset			= std::max<u32>( 0, fileEntry.size );

	if(maxOffset > 0)
		internalReader.readSector(currentSector, currentSectorNumber);
}

IsoFile::~IsoFile() throw()
{
}

u32 IsoFile::seek(u32 absoffset)
{
	u32 endOffset = absoffset;

	int oldSectorNumber = currentSectorNumber;
	u32 newOffset = endOffset;
	int newSectorNumber = fileEntry.lba + (int)(newOffset / sectorLength);
	if(oldSectorNumber != newSectorNumber)
	{
		internalReader.readSector(currentSector, newSectorNumber);
	}
	currentOffset = newOffset;
	currentSectorNumber = newSectorNumber;
	sectorOffset = (int)(currentOffset % sectorLength);
	
	return currentOffset;
}

// Returns the new offset in the file.  Out-of-bounds seeks are automatically truncated at 0
// and fileLength.
u32 IsoFile::seek(s64 offset, wxSeekMode ref_position)
{
	switch( ref_position )
	{
		case wxFromStart:
			pxAssertDev( offset >= 0 && offset <= (s64)ULONG_MAX, "Invalid seek position from start." );
			return seek(offset);

		case wxFromCurrent:
			// truncate negative values to zero, and positive values to 4gb
			return seek( std::min( std::max<s64>(0, (s64)currentOffset+offset), (s64)ULONG_MAX ) );

		case wxFromEnd:
			// truncate negative values to zero, and positive values to 4gb
			return seek( std::min( std::max<s64>(0, (s64)fileEntry.size+offset), (s64)ULONG_MAX ) );

		jNO_DEFAULT;
	}
	
	return 0;		// unreachable
}

void IsoFile::reset()
{
	seek(0);
}

// Returns the number of bytes actually skipped.
s32 IsoFile::skip(s32 n)
{
	s32 oldOffset = currentOffset;

	if(n<0)
		return 0;

	seek(currentOffset+n);

	return currentOffset-oldOffset;
}

u32 IsoFile::getSeekPos() const
{
	return currentOffset;
}

bool IsoFile::eof() const
{
	return (currentOffset == maxOffset);
}

// loads the current sector index into the CurrentSector buffer.
void IsoFile::makeDataAvailable()
{
	if (sectorOffset >= sectorLength)
	{
		currentSectorNumber++;
		internalReader.readSector(currentSector, currentSectorNumber);
		sectorOffset -= sectorLength;
	}
}

u8 IsoFile::readByte()
{
	if(currentOffset >= maxOffset)
		throw Exception::EndOfStream();

	makeDataAvailable();

	currentOffset++;

	return currentSector[sectorOffset++];
}

// Reads data from a single sector at a time.  Reads cannot cross sector boundaries.
int IsoFile::internalRead(void* dest, int off, int len)
{
	if (len > 0)
	{
		size_t slen = len;
		if (slen > (maxOffset - currentOffset))
		{
			slen = (int) (maxOffset - currentOffset);
		}

		memcpy_fast((u8*)dest + off, currentSector + sectorOffset, slen);

		sectorOffset += slen;
		currentOffset += slen;
		return slen;
	}
	return 0;
}

// returns the number of bytes actually read.
s32 IsoFile::read(void* dest, s32 len)
{
	pxAssert( dest != NULL );
	pxAssert( len >= 0 );		// should we silent-fail on negative length reads?  prolly not...

	if( len <= 0 ) return 0;

	int off = 0;

	int totalLength = 0;

	int firstSector = internalRead(dest, off, std::min(len, sectorLength - sectorOffset));
	off += firstSector;
	len -= firstSector;
	totalLength += firstSector;

	// Read whole sectors
	while ((len >= sectorLength) && (currentOffset < maxOffset))
	{
		makeDataAvailable();
		int n = internalRead(dest, off, sectorLength);
		off += n;
		len -= n;
		totalLength += n;
	}

	// Read remaining, if any
	if (len > 0) {
		makeDataAvailable();
		int lastSector = internalRead(dest, off, len);
		totalLength += lastSector;
	}

	return totalLength;
}

// Reads data until it reaches a newline character (either \n, \r, or ASCII-Z).  The caller is
// responsible for handling files with DOS-style newlines (CR/LF pairs), if needed.  The resulting
// string has no newlines.
//
// Read data is unformatted 8 bit / Ascii.  If the source file is known to be UTF8, use the fromUTF8()
// conversion helper provided by PCSX2 utility classes.
//
std::string IsoFile::readLine()
{
	std::string s;
	s.reserve( 512 );

	while( !eof() )
	{
		u8 c = read<u8>();

		if((c=='\n') || (c=='\r') || (c==0))
			break;

		s += c;
	}

	return s;
}

u32 IsoFile::getLength()
{
	return maxOffset;
}

const IsoFileDescriptor& IsoFile::getEntry()
{
	return fileEntry;
}
