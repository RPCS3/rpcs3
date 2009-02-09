/***************************************************************
* romdir.c, based over Alex Lau (http://alexlau.8k.com) RomDir *
****************************************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXFILES 200
#define DIRENTRY_SIZE 16
#define BUFFSIZE 16384

struct __attribute__ ((__packed__)) romdir {
	/*following variable must place in designed order*/
	char fileName[10];
	unsigned short extInfoSize;
	unsigned long fileSize;
} rd;

int main(int argc, char *argv[]) {
	struct stat buf;
	FILE *romdir;
	FILE *extinfo;
	int i, j;

	printf("fps2bios romdir generator\n");
	if (argc < 2){
		printf("usage: %s infile1 [infile2...]\n", argv[0]);
		return 1;
	}

	romdir = fopen("ROMDIR", "wb");
	if (romdir == NULL) {
		printf("failed to create ROMDIR\n");
		return 1;
	}

	extinfo = fopen("EXTINFO", "wb");
	if (extinfo == NULL) {
		printf("failed to create EXTINFO\n");
		return 1;
	}

	for (i=1; i<argc; i++) {
		if (strcmp(argv[i], "ROMDIR") == 0) {
			strncpy(rd.fileName, argv[i], 9);
			rd.extInfoSize = 0;
			rd.fileSize = (argc-1)*16+16;
			fwrite(&rd, 1, 16, romdir);
			continue;
		}
		if (stat(argv[i], &buf) == -1) {
			printf("warning: %s file is missing\n", argv[i]);
			continue;
		}
		for (j=0; j<9; j++) {
			if (argv[i][j] == ',') break;
			rd.fileName[j] = argv[i][j];
		}
		memset(rd.fileName+j, 0, 10-j);
		rd.fileSize = buf.st_size;

		if (argv[i][j] == ',') {
//			for (j=0; j<256; j++) {
//			}
//			rd.extInfoSize+= j;
		} else { // no extInfo
			rd.extInfoSize = 0;
		}

		fwrite(&rd, 1, 16, romdir);
	}
	memset(&rd, 0, sizeof(rd));
	fwrite(&rd, 1, 16, romdir);

	fclose(romdir);
	fclose(extinfo);
	
	return 0;
}
