#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "DEV9.h"

void LoadConf() {
	FILE *f;
	char cfg[256];

    sprintf(cfg, "%s/.PS2E/DEV9linuz.cfg", getenv("HOME"));
	f = fopen(cfg, "r");
	if (f == NULL) {
		strcpy(config.Eth, ETH_DEF);
		strcpy(config.Hdd, HDD_DEF);
		config.HddSize = 1024;
		return;
	}
	fscanf(f, "Eth   = %[^\n]\n", config.Eth);
	fscanf(f, "Hdd   = %[^\n]\n", config.Hdd);
	fscanf(f, "HddSize = %d\n", &config.HddSize);
	if (*config.Eth == 0) strcpy(config.Eth, ETH_DEF);
	if (*config.Hdd == 0) strcpy(config.Hdd, HDD_DEF);
	if (config.HddSize == 0) config.HddSize = 1024;
	fclose(f);
}

void SaveConf() {
	FILE *f;
	char cfg[256];

    sprintf(cfg, "%s/.PS2E", getenv("HOME"));
	mkdir(cfg, 0755);
    sprintf(cfg, "%s/.PS2E/DEV9linuz.cfg", getenv("HOME"));
	f = fopen(cfg, "w");
	if (f == NULL)
		return;
	fprintf(f, "Eth   = %s\n", config.Eth);
	fprintf(f, "Hdd   = %s\n", config.Hdd);
	fprintf(f, "HddSize = %d\n", config.HddSize);
	fclose(f);
}

