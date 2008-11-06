/*  GSsoft 
 *  Copyright (C) 2002-2004  GSsoft Team
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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "GS.h"

void SaveConfig() {
    FILE *f;
    char cfg[255];

    sprintf (cfg, "%s/.PS2E", getenv("HOME"));
	mkdir(cfg, 0755);
    sprintf (cfg, "%s/.PS2E/GSsoftx.cfg", getenv("HOME"));
    f = fopen(cfg,"w");
    if (f == NULL) return;
	fprintf(f, "fmode.width  = %d\n", conf.fmode.width);
	fprintf(f, "fmode.height = %d\n", conf.fmode.height);
	fprintf(f, "wmode.width  = %d\n", conf.wmode.width);
	fprintf(f, "wmode.height = %d\n", conf.wmode.height);
	fprintf(f, "fullscreen   = %d\n", conf.fullscreen);
	fprintf(f, "fps          = %d\n", conf.fps);
	fprintf(f, "frameskip    = %d\n", conf.frameskip);
	fprintf(f, "record       = %d\n", conf.record);
	fprintf(f, "cache        = %d\n", conf.cache);
	fprintf(f, "cachesize    = %d\n", conf.cachesize);
	fprintf(f, "codec        = %d\n", conf.codec);
#ifdef GS_LOG
	fprintf(f, "log          = %d\n", conf.log);
#endif
    fclose(f);
}

void LoadConfig() {
    FILE *f;
    char cfg[255];

	memset(&conf, 0, sizeof(conf));
	conf.fmode.width  = 320;
	conf.fmode.height = 240;
	conf.wmode.width  = 320;
	conf.wmode.height = 240;
	conf.fullscreen  = 1;
	conf.cachesize	 = 128;
	conf.codec       = 0;

    sprintf (cfg, "%s/.PS2E/GSsoftx.cfg", getenv("HOME"));
    f = fopen(cfg, "r");
    if (f == NULL) return;
	fscanf(f, "fmode.width  = %d\n", &conf.fmode.width);
	fscanf(f, "fmode.height = %d\n", &conf.fmode.height);
	fscanf(f, "wmode.width  = %d\n", &conf.wmode.width);
	fscanf(f, "wmode.height = %d\n", &conf.wmode.height);
	fscanf(f, "fullscreen   = %d\n", &conf.fullscreen);
	fscanf(f, "fps          = %d\n", &conf.fps);
	fscanf(f, "frameskip    = %d\n", &conf.frameskip);
	fscanf(f, "record       = %d\n", &conf.record);
	fscanf(f, "cache        = %d\n", &conf.cache);
	fscanf(f, "cachesize    = %d\n", &conf.cachesize);
	fscanf(f, "codec        = %d\n", &conf.codec);
#ifdef GS_LOG
	fscanf(f, "log          = %d\n", &conf.log);
#endif
    fclose(f);

	if (conf.fullscreen) cmode = &conf.fmode;
	else cmode = &conf.wmode;
}

