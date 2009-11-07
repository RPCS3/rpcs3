#pragma once

#include <string>

typedef unsigned char byte;

class IsoFileDescriptor
{
public:
	struct FileDate // not 1:1 with iso9660 date struct
	{
		int  year;
		byte month;
		byte day;
		byte hour;
		byte minute;
		byte second;
		byte gmtOffset; // Offset from Greenwich Mean Time in number of 15 min intervals from -48 (West) to + 52 (East)

	} date;

	int lba;
	int size;
	int flags;
	std::string name; //[128+1];

	IsoFileDescriptor();

	IsoFileDescriptor(const byte* data, int length);

	~IsoFileDescriptor(void) {}
};
