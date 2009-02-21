/*  ecma119.h
 *  Copyright (C) 2002-2005  PCSX2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */

#ifndef ECMA119_H
#define ECMA119_H

// #ifndef __LINUX__
// #ifdef __linux__
// #define __LINUX__
// #endif /* __linux__ */
// #endif /* No __LINUX__ */

// #define CDVDdefs
// #include "PS2Edefs.h"

// ECMA119 was sent to ISO to be fast-tracked into ISO 9660
// ECMA119 can be found at http://www.ecma.ch, somewhere.
// Throughout these definitions, number pairs in both big-endian and
//   little-endian varieties are stored next to each other. To separate
//   the pairs a 'le' suffix has been attached to little-endian numbers, and
//   a 'be' suffix to big-endian ones.

// All 'unused' entries should be set to (or tested for) 0x00.

#ifdef _WIN32
#pragma pack(1)
#endif /* _WIN32 */
struct ECMA119ASCIITime
{
	char year[4];
	char month[2];
	char day[2];
	char hour[2];
	char min[2];
	char sec[2];
	char hundredthsec[2];
	char offsetgmt; // 15 min intervals, from -48 to +52
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */

struct ECMA119DateTime
{
	unsigned char year; // 1900+year, that is.
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char sec;
	signed char offsetgmt; // In 15 min intervals, from -48 to +52
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */

struct ECMA119DirectoryRecord
{
	unsigned char reclen; // Length of this record
	unsigned char attlen; // Extended Attribute Record Length

	unsigned long externlocle; // Location of Extent
	unsigned long externlocbe;
	struct ECMA119DateTime recorded; // Recording Date and Time
	unsigned char flags; // File Flags
	unsigned char interleave; // Interleave Gap Size

	unsigned short seqnole; // Volume Sequence No.
	unsigned short seqnobe;
	unsigned short idlen;
	char id[223];
	// Note: sometimes a OS uses the end of this record for it's own use.
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */

struct ECMA119RootDirectoryRecord
{
	unsigned char reclen; // Length of this record
	unsigned char attlen; // Extended Attribute Record Length

	unsigned long externlocle; // Location of Extent
	unsigned long externlocbe;
	struct ECMA119DateTime recorded; // Recording Date and Time
	unsigned char flags; // File Flags
	unsigned char interleave; // Interleave Gap Size

	unsigned short seqnole; // Volume Sequence No.
	unsigned short seqnobe;
	unsigned short idlen;
	char id[1]; // Probably for the '.' (But I'm just guessing :)
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */
struct ECMA119VolumeID
{
	unsigned char voltype;
	// 0 = Boot Record
	// 1 = Primary Volume Descriptor (PrimaryVolume below)
	// 2 = Supplementary Volume Descriptor
	// 3 = Partition Descriptor (PartitionVolume below)
	// 4 - 254 Reserved
	// 255 = End-of-Descriptor-Set

	char stdid[5]; // Standard Identifier. Always "CD001"

	unsigned char version;
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */
struct ECMA119PrimaryVolume
{
	struct ECMA119VolumeID id;
	// id.voltype should be 1 (for this type of volume)
	// id.version should be 1 (standard)

	char unused1;

	char systemid[32];
	char volumeid[32];
	char unused2[8];
	unsigned long numblocksle; // Total logical blocks. (on Media? or just
	unsigned long numblocksbe; // in volume?)

	char unused3[32];

	unsigned short volumesetsizele; // Volume Set size of the volume. (?)
	unsigned short volumesetsizebe;
	unsigned short ordinalle; // Count of which descriptor this is in the Volume
	unsigned short ordinalbe; // set.

	unsigned short blocksizele; // Size of a Logical Block
	unsigned short blocksizebe;
	unsigned long pathtablesizele; // Path Table Size
	unsigned long pathtablesizebe;

	unsigned long typelpathtablelocation; // (le) Location of a Type L Path Table

	unsigned long typelopttablelocation; // (le) Location of an Optional Type L

	unsigned long typempathtablelocation; // (be) Location of a Type M Path Table

	unsigned long typemopttablelocation; // (be) Location of an Optional Type M

	struct ECMA119RootDirectoryRecord root;

	char volumesetid[128]; // Volume Set ID

	char publisher[128]; // Publisher

	char datapreparer[128]; // Data Preparer

	char application[128]; // Application ID

	char copyrightfile[37]; // Copyright File Identifier

	char abstractfile[37]; // Abstract File Identifier

	char bibliograchicfile[37]; // Bibliographic File Identifier

	struct ECMA119ASCIITime volcreatedate; // Date Created
	struct ECMA119ASCIITime volmodifydate; // Last Date Modified
	struct ECMA119ASCIITime volexpiredate; // Date data expires
	struct ECMA119ASCIITime voleffectivedata; // Date data becomes accurate (effective)
	unsigned char filestructversion; // File Structure Version
	// Should be 1 = Standard

	char unused4;

	char applicationuse[512]; // For use by an application

	char unused5[653]; // Rounds this out to 2048 bytes...
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */
// struct ECMA119PartitionVolume {
//   struct ECMA119VolumeID id;
// #ifdef _WIN32
// };
// #else
// } __attribute__ ((packed));
// #endif /* _WIN32 */
#ifdef _WIN32
#pragma pack()
#endif /* _WIN32 */

extern int ValidateECMA119PrimaryVolume(struct ECMA119PrimaryVolume *volume);
// extern int ValidateECMA119PartitionVolume(struct ECMA119PartitionVolume volume);

#endif /* ECMA119_H */
