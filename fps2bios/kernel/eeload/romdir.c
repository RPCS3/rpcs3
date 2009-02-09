/***************************************************************
* romdir.c, based over Alex Lau (http://alexlau.8k.com) RomDir *
****************************************************************/
#include "romdir.h"

struct romdir *base = NULL;

struct romdir *romdirInit() {
	u8 *mem;

	for (mem=(u8*)0xbfc00000; (u32)mem<0xbfc01000; mem++) {
		if (mem[0] == 'R' && mem[1] == 'E' &&
			mem[2] == 'S' && mem[3] == 'E' &&
			mem[4] == 'T')
			break;
	}
	if ((u32)mem == 0xbfc01000) return NULL;

	return (struct romdir*)mem;
}

struct rominfo *romdirGetFile(char *name, struct rominfo *ri) {
	struct romdir *rd;
//	struct romdir *base;
	int i;

	if (base == NULL) {
		base = romdirInit();
		if (base == NULL) return NULL;
	}

	ri->fileOffset = 0;
	for (rd = base; rd->fileName[0] != 0; rd++) {
		for (i=0; i<10 && name[i] != 0; i++) {
			if (rd->fileName[i] != name[i]) break;
		}
		if (rd->fileName[i] != name[i]) {
			ri->fileOffset+= (rd->fileSize + 15) & ~0xF;
			continue;
		}
		
		ri->fileSize = rd->fileSize;
		return ri;
	}

	return NULL;
}
