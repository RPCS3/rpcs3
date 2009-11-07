#include "PrecompiledHeader.h"

#include "IsoFile.h"

using namespace std;

IsoFile::IsoFile(SectorSource* reader, IsoFileDescriptor fileEntry)
	: fileEntry(fileEntry)
{
	internalReader = reader;
	currentSectorNumber=fileEntry.lba;
	currentOffset = 0;
	sectorOffset = 0;
	maxOffset = fileEntry.size;

	if(maxOffset>0)
		reader->readSector(currentSector,currentSectorNumber);
}

void IsoFile::seek(__int64 offset)
{
	__int64 endOffset = offset;

	if(offset<0)
		throw new exception("Seek offset out of bounds.");

	int oldSectorNumber = currentSectorNumber;
	__int64 newOffset = endOffset;
	int newSectorNumber = fileEntry.lba + (int)(newOffset / sectorLength);
	if(oldSectorNumber != newSectorNumber)
	{
		internalReader->readSector(currentSector, newSectorNumber);
	}
	currentOffset = newOffset;
	currentSectorNumber = newSectorNumber;
	sectorOffset = (int)(currentOffset % sectorLength);
}

void IsoFile::seek(__int64 offset, int ref_position)
{
	if(ref_position == SEEK_SET)
		seek(offset);

	if(ref_position == SEEK_CUR)
		seek(currentOffset+offset);

	if(ref_position == SEEK_END)
		seek(fileEntry.size+offset);
}

void IsoFile::reset()
{
	seek(0);
}

__int64 IsoFile::skip(__int64 n)
{
	__int64 oldOffset = currentOffset;

	if(n<0)
		return n;

	seek(currentOffset+n);

	return currentOffset-oldOffset;
}

__int64 IsoFile::getFilePointer()
{
	return currentOffset;
}

bool IsoFile::eof()
{
	return (currentOffset == maxOffset);
}

void IsoFile::makeDataAvailable()
{
	if (sectorOffset >= sectorLength) {
		currentSectorNumber++;
		internalReader->readSector(currentSector, currentSectorNumber);
		sectorOffset -= sectorLength;
	}
}

int IsoFile::read()
{
	if(currentOffset >= maxOffset)
		throw Exception::EndOfStream();

	makeDataAvailable();

	currentOffset++;

	return currentSector[sectorOffset++];
}

int IsoFile::internalRead(byte* b, int off, int len)
{
	if (len > 0)
	{
		if (len > (maxOffset - currentOffset))
		{
			len = (int) (maxOffset - currentOffset);
		}

		memcpy(b + off, currentSector + sectorOffset, len);

		sectorOffset += len;
		currentOffset += len;
	}

	return len;
}

int IsoFile::read(byte* b, int len)
{
	if (b == NULL)
	{
		throw Exception::ObjectIsNull("b");
	}

	if (len < 0)
	{
		throw Exception::InvalidOperation("off<0 or len<0.");
	}

	int off=0;

	int totalLength = 0;

	int firstSector = internalRead(b, off, min(len, sectorLength - sectorOffset));
	off += firstSector;
	len -= firstSector;
	totalLength += firstSector;

	// Read whole sectors
	while ((len >= sectorLength) && (currentOffset < maxOffset))
	{
		makeDataAvailable();
		int n = internalRead(b, off, sectorLength);
		off += n;
		len -= n;
		totalLength += n;
	}

	// Read remaining, if any
	if (len > 0) {
		makeDataAvailable();
		int lastSector = internalRead(b, off, len);
		totalLength += lastSector;
	}

	return totalLength;
}

string IsoFile::readLine()
{
	string s;
	char c;

	s.clear();
	do {
		if(eof())
			break;

		c = read();

		if((c=='\n')||(c=='\r')||(c==0))
			break;

		s.append(1,c);
	} while(true);

	return s;
}

wstring IsoFile::readLineW()
{
	wstring s;
	wchar_t c;

	s.clear();
	do {
		if(eof())
			break;

		c = read<wchar_t>();

		if((c==L'\n')||(c==L'\r')||(c==0))
			break;

		s.append(1,c);
	} while(true);

	return s;
}

__int64 IsoFile::getLength()
{
	return maxOffset;
}

const IsoFileDescriptor& IsoFile::getEntry()
{
	return fileEntry;
}

IsoFile::~IsoFile(void) 
{

}
