#pragma once

struct IsoFileDescriptor
{
	struct FileDate // not 1:1 with iso9660 date struct
	{
		s32	year;
		u8	month;
		u8	day;
		u8	hour;
		u8	minute;
		u8	second;
		u8	gmtOffset; // Offset from Greenwich Mean Time in number of 15 min intervals from -48 (West) to + 52 (East)

	} date;

	u32		lba;
	u32		size;
	int		flags;
	wxString name;

	IsoFileDescriptor();
	IsoFileDescriptor(const u8* data, int length);
	
	bool IsFile() const { return !(flags & 2); }
	bool IsDir() const { return !IsFile(); }
};