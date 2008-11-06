#ifndef __SYSCLIB_H__
#define __SYSCLIB_H__

#define SYSCLIB_VER	0x101

#include <stddef.h>

#define	_U	01
#define	_L	02
#define	_N	04
#define	_S	010
#define _P	020
#define _C	040
#define _X	0100
#define	_B	0200

extern const char	_ctype_[];

#ifndef __cplusplus
#define	isalpha(c)	((_ctype_+1)[(unsigned)(c)]&(_U|_L))
#define	isupper(c)	((_ctype_+1)[(unsigned)(c)]&_U)
#define	islower(c)	((_ctype_+1)[(unsigned)(c)]&_L)
#define	isdigit(c)	((_ctype_+1)[(unsigned)(c)]&_N)
#define	isxdigit(c)	((_ctype_+1)[(unsigned)(c)]&(_X|_N))
#define	isspace(c)	((_ctype_+1)[(unsigned)(c)]&_S)
#define ispunct(c)	((_ctype_+1)[(unsigned)(c)]&_P)
#define isalnum(c)	((_ctype_+1)[(unsigned)(c)]&(_U|_L|_N))
#define isprint(c)	((_ctype_+1)[(unsigned)(c)]&(_P|_U|_L|_N|_B))
#define	isgraph(c)	((_ctype_+1)[(unsigned)(c)]&(_P|_U|_L|_N))
#define iscntrl(c)	((_ctype_+1)[(unsigned)(c)]&_C)
#endif /* !__cplusplus */

#define isascii(c)	((unsigned)(c)<=0177)
#define toascii(c)	((c)&0177)

unsigned char look_ctype_table(int pos);
void* memset(void *s, int c, size_t n);
void  bzero(void *s, size_t n);
int   strcmp (const char *, const char *);
char* index  (const char *s, int c);
int   strlen (const char *);
char* strncpy(char *dest, const char *src, size_t n);
long int strtol(const char *nptr, char **endptr, int base);
//int   prnt(void (*func)(void*, int), int *v, const char *format, ...);	//18(17)

#endif//__SYSCLIB_H__
