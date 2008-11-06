#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "../../nsx2.h"
#include "../gs.h"
#include "../../iniapi/iniapi.h"


void SaveConfig() {
    FILE *f;
    char cfg[255];

    sprintf (cfg, "%s/.PS2E", getenv("HOME"));
	mkdir(cfg, 0x755);
    sprintf (cfg, "%s/.PS2E/GSsoftx.cfg", getenv("HOME"));
    f = fopen(cfg,"w");
    if (f == NULL) return;
    fwrite(&conf, 1, sizeof(conf), f);
    fclose(f);
}

void LoadConfig() {
    FILE *f;
    char cfg[255];
    int result;

	memset(&conf, 0, sizeof(conf));

	/* Read config File */
	result =  getini(nSX2ConfigFile,"Video","Width",cfg);
	if ( result == 0 )	conf.mode.width  = 320;
	else conf.mode.width = atoi ( cfg );
	
	result =  getini(nSX2ConfigFile,"Video","Height",cfg);
	if ( result == 0 )	conf.mode.height = 240;
	else conf.mode.height = atoi ( cfg );

	result =  getini(nSX2ConfigFile,"Video","Bpp",cfg);
	if ( result == 0 )		conf.mode.bpp = 16;
	else conf.mode.bpp = atoi ( cfg );	

	result =  getini(nSX2ConfigFile,"Video","Stretch",cfg);
	if ( result == 0 )		conf.stretch    = 1;
	else conf.stretch = atoi ( cfg );	
	/*
	conf.mode.width  = 640;
	conf.mode.height = 480;
	conf.mode.bpp    = 16;
	conf.fullscreen  = 0;
	conf.stretch     = 0;
	*/
	
	conf.fullscreen  = 0;

    sprintf (cfg, "%s/.PS2E/GSsoftx.cfg", getenv("HOME"));
    f = fopen(cfg,"r");
    if (f == NULL) return;
    fread(&conf, 1, sizeof(conf), f);
    fclose(f);
}

