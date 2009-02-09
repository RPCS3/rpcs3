#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc, char *argv[]) {
	char str[256];
	FILE *f;
	time_t t;
	int version;
	int build;

	printf("fps2bios romver generator\n");
	if (argc < 3){
		printf("usage: %s version build\n", argv[0]);
		return 1;
	}
	f = fopen("ROMVER", "wb");
	if (f == NULL) {
		printf("failed to create ROMVER\n");
		return 1;
	}

	t = time(NULL);
	strftime(str, 256, "%Y%m%d", localtime(&t));
	version = strtol(argv[1], (char**)NULL, 0);
	build   = strtol(argv[2], (char**)NULL, 0);
	fprintf(f, "%2.2d%2.2dPD%s\n", version, build, str);
	fputc(0, f);

	fclose(f);
	
	return 0;
}
