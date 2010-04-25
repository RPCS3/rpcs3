#ifndef __EXCEPMAN_H__
#define __EXCEPMAN_H__

#define EXCEPMAN_VER	0x101

#define EXCEP_MAX	16

struct exHandler {
	void *next;	// next points to the next funccode :/
	int	info;
	u32 funccode[0];
};

void *GetExHandlersTable();											// 3

int RegisterExceptionHandler(int code, struct exHandler *handler);	// 4
int RegisterPriorityExceptionHandler(int code, int priority,
									 struct exHandler *handler);	// 5
int RegisterDefaultExceptionHandler(struct exHandler *handler);		// 6
int ReleaseExceptionHandler(int excode, struct exHandler *handler);	// 7
int ReleaseDefaultExceptionHandler(struct exHandler *handler);		// 8

#endif//__EXCEPMAN_H__
