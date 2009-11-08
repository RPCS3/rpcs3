#pragma once

#include "IsoFileDescriptor.h"
#include "SectorSource.h"

class IsoFile
{
public:
	static const int sectorLength = 2048;

private:
	IsoFileDescriptor fileEntry;

	s64 currentOffset;
	s64 maxOffset;

	int currentSectorNumber;
	byte currentSector[sectorLength];
	int sectorOffset;

	SectorSource* internalReader;

	void makeDataAvailable();
	int  internalRead(byte* b, int off, int len);

public:

	IsoFile(SectorSource* reader, IsoFileDescriptor fileEntry);

	void seek(s64 offset);
	void seek(s64 offset, int ref_position);
	void reset();

	s64 skip(s64 n);
	s64 getFilePointer();

	bool eof();
	int  read();
	int  read(byte* b, int len);

	// Tool to read a specific value type, including structs.
	template<class T>
	T read()
	{
		if(sizeof(T)==1)
			return (T)read();
		else
		{
			T t;
			read((byte*)&t,sizeof(t));
			return t;
		}
	}

	std::string readLine();   // Assume null-termination
	std::wstring readLineW(); // (this one too)

	s64 getLength();

	const IsoFileDescriptor& getEntry();

	~IsoFile(void);
};
