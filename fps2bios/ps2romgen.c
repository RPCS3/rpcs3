// little prog for bios image repack by Florin Sasu 2002-10-03

#include <stdio.h>
#include <string.h>

struct romdir{
	char	name[10];
	short	ext;
	int		size;
} rd[200];			//increase the number if needed; 200 is enough

char buffer[10000];

#define min(a,b)	(a<b?a:b)

void fillfile(FILE *f, int size, char c){
	memset(buffer, c, 10000);
	while(size>0){
		fwrite(buffer, 1, min(size, 10000), f);
		size-=10000;
	}
}

void writefile(FILE *f, char *name, int offset, int size){
	FILE *fi=fopen(name, "rb");
	if (fi) fseek(fi, 0, SEEK_END);
	if ((!fi) || (ftell(fi)!=size)){
		printf("Could not find a file %s of %d bytes\n", name, size);
		if (fi) fclose(fi);
		return;
	}

	printf("%10s\t%8X\t%8X\n", name, offset, size);
	fillfile(f, offset-ftell(f), 0);
	fseek(fi, 0, SEEK_SET);
	while (size>0){
		fread(buffer, 1, min(10000, size), fi);
		fwrite(buffer, 1, min(10000, size), f);
		size-=10000;
	}

	fclose(fi);
}

int main(int argc, char* argv[]){
	int i, n, offset;
	FILE *f;
//////////////////////////////////
	printf("PS2 ROMGEN v0.1 Florin Sasu 2002-10-03 (florinsasu@yahoo.com) no padding\n");
	if (argc<2){
		printf("Usage: ps2biosgen <filename>\n\tfilename=name of the biosfile to create\n");
		printf("\n\tPut in the same directory with ps2romgen all the files from bios\n");
		return 1;
	}
//////////////////////////////////
	f=fopen("ROMDIR", "rb");
	if (!f){
		printf("Could not find the ROMDIR file\n");
		return 1;
	}

	for (n=0; !feof(f) && (fread(&rd[n], 16, 1, f)==1) && rd[n].name[0]; n++);

	fclose(f);
//////////////////////////////////
	printf("\n      Name   Offset(hex)       Size(hex)"
		   "\n----------------------------------------\n");
	f=fopen(argv[1], "wb");

	for (i=0, offset=0; i<n; offset=(rd[i++].size+offset+15) & 0xFFFFFFF0)
		if (rd[i].name[0]!='-')
			writefile(f, rd[i].name, offset, rd[i].size);

	fclose(f);
//////////////////////////////////
	printf("Done (%s)\n", argv[1]);

	return 0;
}

