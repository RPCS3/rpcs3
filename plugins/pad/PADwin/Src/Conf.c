#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "PAD.h"



#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#ifndef __WIN32__
#include <unistd.h>
#endif

void SaveConfig() {
    FILE *f;
    char cfg[255];
	int i, j;
#ifdef __WIN32__
    CreateDirectory("inis",NULL); 
	sprintf(cfg,"inis/padwinkeyb.ini");
#else
    sprintf(cfg,"%s/.PS2E", getenv("HOME"));
    mkdir(cfg, 0755);
    sprintf (cfg,"%s/.PS2E/PADwin.cfg",getenv("HOME"));
#endif
    f = fopen(cfg,"w");
    if (f == NULL) {
		printf("PADwin: Error writing to '%s'\n", cfg);
		return;
	}
	for (j=0; j<2; j++) {
		for (i=0; i<16; i++) {
			fprintf(f, "[%d][%d] = 0x%lx\n", j, i, conf.keys[j][i]);
		}
	}
	fprintf(f, "logging = %d\n", conf.log);
    fclose(f);
}

void LoadConfig() {
    FILE *f;
	char str[256];
    char cfg[255];
	int i, j;

        memset(&conf, 0, sizeof(conf));
#ifdef __WIN32__
	    conf.keys[0][0] = '1';			// L2
	    conf.keys[0][1] = '3';			// R2
	    conf.keys[0][2] = 'Q';			// L1
	    conf.keys[0][3] = 'E';			// R1
		conf.keys[0][4] = 'W';			// TRIANGLE
		conf.keys[0][5] = 'D';			// CIRCLE
		conf.keys[0][6] = 'X';			// CROSS
		conf.keys[0][7] = 'A';			// SQUARE
		conf.keys[0][8] = VK_DECIMAL;	// SELECT
		conf.keys[0][11] = VK_NUMPAD0;	// START
		conf.keys[0][12] = VK_NUMPAD8;	// UP
		conf.keys[0][13] = VK_NUMPAD6;	// RIGHT
		conf.keys[0][14] = VK_NUMPAD2;	// DOWN
		conf.keys[0][15] = VK_NUMPAD4;	// LEFT
#else
	conf.keys[0][0] = XK_1;			// L2
	conf.keys[0][1] = XK_3;			// R2
	conf.keys[0][2] = XK_q;			// L1
	conf.keys[0][3] = XK_e;			// R1
	conf.keys[0][4] = XK_w;			// TRIANGLE
	conf.keys[0][5] = XK_d;			// CIRCLE
	conf.keys[0][6] = XK_x;			// CROSS
	conf.keys[0][7] = XK_a;			// SQUARE
	conf.keys[0][8] = XK_KP_Delete;	// SELECT
	conf.keys[0][11] = XK_KP_Insert;// START
	conf.keys[0][12] = XK_KP_Up;	// UP
	conf.keys[0][13] = XK_KP_Right;	// RIGHT
	conf.keys[0][14] = XK_KP_Down;	// DOWN
	conf.keys[0][15] = XK_KP_Left;	// LEFT
#endif
	conf.log = 0;
#ifdef __WIN32__
	sprintf(cfg,"inis/padwinkeyb.ini");
#else
    sprintf (cfg,"%s/.PS2E/PADwin.cfg", getenv("HOME"));
#endif
    f = fopen(cfg,"r");
    if (f == NULL) return;

	for (j=0; j<2; j++) {
		for (i=0; i<16; i++) {
			sprintf(str, "[%d][%d] = 0x%%x\n", j, i);
			fscanf(f, str, &conf.keys[j][i]);
		}
	}
	fscanf(f, "logging = %d\n", &conf.log);
    fclose(f);
}


