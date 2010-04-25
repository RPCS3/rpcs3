/*  CDVDiso
 *  Copyright (C) 2002-2004  CDVDiso Team
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "string.h"

#include "CDVDiso.h"


void Compress(char *filename, int mode)
{
	struct stat buf;
	u32 lsn;
	u8 cdbuff[1024*16];
	char Zfile[256];
	int ret = 0;
	isoFile *src;
	isoFile *dst;

	if (mode == 1)
	{
		sprintf(Zfile, "%s.Z2", filename);
	}
	else
	{
		sprintf(Zfile, "%s.BZ2", filename);
	}

	if (stat(Zfile, &buf) != -1)
	{
		printf("'%s' already exists\n", Zfile);
		return;
		/*		sprintf(str, "'%s' already exists, overwrite?", Zfile);
				if (MessageBox(hDlg, str, "Question", MB_YESNO) != IDYES) {
					return;
				}*/
	}

	printf("src %s; dst %s\n", filename, Zfile);
	src = isoOpen(filename);
	if (src == NULL) return;

	if (mode == 1)
	{
		dst = isoCreate(Zfile, ISOFLAGS_Z2);
	}
	else
	{
		dst = isoCreate(Zfile, ISOFLAGS_BZ2);
	}
	isoSetFormat(dst, src->blockofs, src->blocksize, src->blocks);
	if (dst == NULL) return;

	for (lsn = 0; lsn < src->blocks; lsn++)
	{
		printf("block %d ", lsn);
		putchar(13);
		fflush(stdout);
		ret = isoReadBlock(src, cdbuff, lsn);
		if (ret == -1) break;
		ret = isoWriteBlock(dst, cdbuff, lsn);
		if (ret == -1) break;
	}
	isoClose(src);
	isoClose(dst);

	if (ret == -1)
	{
		printf("Error compressing iso image\n");
	}
	else
	{
		printf("Iso image compressed OK\n");
	}
}

void Decompress(char *filename)
{
	struct stat buf;
	char file[256];
	u8 cdbuff[10*2352];
	u32 lsn;
	isoFile *src;
	isoFile *dst;
	int ret = 0;

	src = isoOpen(filename);
	if (src == NULL) return;

	strcpy(file, filename);
	if (src->flags & ISOFLAGS_Z)
		file[strlen(file) - 2] = 0;
	else if (src->flags & ISOFLAGS_Z2)
		file[strlen(file) - 3] = 0;
	else if (src->flags & ISOFLAGS_BZ2)
		file[strlen(file) - 3] = 0;
	else
	{
		printf("%s is not a compressed image\n", filename);
		return;
	}

	if (stat(file, &buf) != -1)
	{
		char str[256];
		sprintf(str, "'%s' already exists", file);
		isoClose(src);
		return;
	}

	dst = isoCreate(file, 0);
	if (dst == NULL) return;
	isoSetFormat(dst, src->blockofs, src->blocksize, src->blocks);

	for (lsn = 0; lsn < src->blocks; lsn++)
	{
		printf("block %d ", lsn);
		putchar(13);
		fflush(stdout);
		ret = isoReadBlock(src, cdbuff, lsn);
		if (ret == -1) break;
		ret = isoWriteBlock(dst, cdbuff, lsn);
		if (ret == -1) break;
	}

	isoClose(src);
	isoClose(dst);

	if (ret == -1)
		printf("Error decompressing iso image\n");
	else
		printf("Iso image decompressed OK\n");
}


/*int main(int argc, char *argv[])
{
	if (argc < 3) return 0;

	switch (argv[1][0])
	{
		case 'c':
			Compress(argv[2], 1);
			break;
		case 'C':
			Compress(argv[2], 2);
			break;
		case 'd':
			Decompress(argv[2]);
			break;
		default: break;
	}

	return 0;
}*/
