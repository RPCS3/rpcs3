#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "DEV9.h"
#include "socks.h"

int ExecCfg(char *arg) {
	char cfg[256];
	struct stat buf;

	strcpy(cfg, "./cfgDEV9linuz");
	if (stat(cfg, &buf) != -1) {
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}

	strcpy(cfg, "./cfg/cfgDEV9linuz");
	if (stat(cfg, &buf) != -1) {
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}

	sprintf(cfg, "%s/cfgDEV9linuz", getenv("HOME"));
	if (stat(cfg, &buf) != -1) {
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}

	printf("cfgDEV9linuz file not found!\n");
	return -1;
}

void SysMessage(char *fmt, ...) {
	va_list list;
	char msg[512];
	char cmd[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	sprintf(cmd, "message \"%s\"", msg);
	ExecCfg(cmd);
}

void DEV9configure() {
	ExecCfg("configure");
}

void DEV9about() {
	ExecCfg("about");
}

void *_DEV9thread(void *arg) {
	DEV9thread();
	return NULL;
}

pthread_t thread;

s32  _DEV9open() {
	pthread_create(&thread, NULL, _DEV9thread, NULL);

	return 0;
}

void _DEV9close() {
	ThreadRun = 0;
	pthread_join(thread, NULL);
}

