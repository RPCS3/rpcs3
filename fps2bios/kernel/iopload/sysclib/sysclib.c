#include <tamtypes.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include "kloadcore.h"
#include "stdarg.h"
#include "ksysclib.h"


typedef struct _jmp_buf{
    int	ra,
	sp,
	fp,
	s0,
	s1,
	s2,
	s3,
	s4,
	s5,
	s6,
	s7,
	gp;
} *jmp_buf;

int _start(int argc, char* argv[]);


unsigned char ctype_table[128]={
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x08, 0x08, 0x08, 0x08, 0x08, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,

	0x18, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,

	0x10, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x10, 0x10, 0x10, 0x10, 0x10,

	0x10, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x10, 0x10, 0x10, 0x10, 0x20
};

int setjmp (jmp_buf jb){
	asm __volatile__(
		"sw $31,  0($4)\n"	//$ra, JUMP_BUFFER.ra($a0)
		"sw $28,  4($4)\n"	//$gp, JUMP_BUFFER.gp($a0)
		"sw $29,  8($4)\n"	//$sp, JUMP_BUFFER.sp($a0)
		"sw $30, 12($4)\n"	//$fp, JUMP_BUFFER.fp($a0)
		"sw $16, 16($4)\n"	//$s0, JUMP_BUFFER.s0($a0)
		"sw $17, 20($4)\n"	//$s1, JUMP_BUFFER.s1($a0)
		"sw $18, 24($4)\n"	//$s2, JUMP_BUFFER.s2($a0)
		"sw $19, 28($4)\n"	//$s3, JUMP_BUFFER.s3($a0)
		"sw $20, 32($4)\n"	//$s4, JUMP_BUFFER.s4($a0)
		"sw $21, 36($4)\n"	//$s5, JUMP_BUFFER.s5($a0)
		"sw $22, 40($4)\n"	//$s6, JUMP_BUFFER.s6($a0)
		"sw $23, 44($4)\n"	//$s7, JUMP_BUFFER.s7($a0)
	);
	return 0;
}

int longjmp(void *jb, int u){
	asm __volatile__(
		"lw $31,  0($4)\n"	//$ra, JUMP_BUFFER.ra($a0)
		"lw $28,  4($4)\n"	//$gp, JUMP_BUFFER.gp($a0)
		"lw $29,  8($4)\n"	//$sp, JUMP_BUFFER.sp($a0)
		"lw $30, 12($4)\n"	//$fp, JUMP_BUFFER.fp($a0)
		"lw $16, 16($4)\n"	//$s0, JUMP_BUFFER.s0($a0)
		"lw $17, 20($4)\n"	//$s1, JUMP_BUFFER.s1($a0)
		"lw $18, 24($4)\n"	//$s2, JUMP_BUFFER.s2($a0)
		"lw $19, 28($4)\n"	//$s3, JUMP_BUFFER.s3($a0)
		"lw $20, 32($4)\n"	//$s4, JUMP_BUFFER.s4($a0)
		"lw $21, 36($4)\n"	//$s5, JUMP_BUFFER.s5($a0)
		"lw $22, 40($4)\n"	//$s6, JUMP_BUFFER.s6($a0)
		"lw $23, 44($4)\n"	//$s7, JUMP_BUFFER.s7($a0)
	);
	return u;
}

#define CHAR(a) (((a)<<24)>>24)
unsigned char look_ctype_table(int);

int toupper (int ch){
    if (look_ctype_table(CHAR(ch)) & 0x0002)
	return CHAR(ch-32);
    return CHAR(ch);
}

int tolower (int ch){
    if (look_ctype_table(CHAR(ch)) & 0x0001)
	return CHAR(ch+32);
    return CHAR(ch);
}

unsigned char look_ctype_table(int pos){
    return ctype_table[pos];
}

void* get_ctype_table(){
    return ctype_table;
}

void* memchr(void *s, int ch, int len){
    if (s && len>0) {
		ch &= 0xFF;
		do {
		    if (*(unsigned char*)s == ch)
			return s;
		} while(s++, --len>0);

    }
    return NULL;
}

int memcmp(const void *a, const void *b, size_t len){
    if (len == 0) return 0;

	do {
	    if (*(unsigned char*)a != *(unsigned char*)b) {
			if (*(unsigned char*)a < *(unsigned char*)b) return 1;
			else         return -1;
		}
	} while (a++, b++, --len>0);

    return 0;
}

void* memcpy(void *dest, const void *src, size_t len){
	void *temp;

	if (dest == NULL) return NULL;

//	if (((int)dest | (int)src | len) & 3 == 0)
//	    return wmemcopy(dest, src, len);
	for(temp=dest; len>0; len--)
	    *(unsigned char*)temp++ = *(unsigned char*)src++;
	return dest;
}

void *memmove (void *dest, const void *src, size_t len){
    void *temp;

    if (dest==NULL) return NULL;

    temp=dest;
    if (dest>=src)
	for (len--; len>=0; len--)
	    *(unsigned char*)(temp+len)=*(unsigned char*)(src+len);
    else
	for (     ; len>0;  len--)
	    *(unsigned char*)temp++    =*(unsigned char*)src++;
    return dest;
}

void *memset (void *dest, int ch, size_t len){
    void *temp;

    if (dest == NULL) return NULL;

/*	if ((ch==0) && (((int)dest | len) & 3 == 0))
	    return wmemset(dest, 0, len);
	else
*/	    for (temp=dest; len>0; len--)
		*(unsigned char*)temp++ = ch;
    return dest;
}

int bcmp (const void *a, const void *b, size_t len){
    return memcmp(a, b, len);
}

void bcopy (const void *src, void *dest, size_t len){
    memmove(dest, src, len);
}

void bzero (void *dest, size_t len){
    memset(dest, 0, len);
}

char *strcat(char *dest, const char *src) {
	char *s = dest;

	while (*dest) dest++;
	while (*dest++ = *src++);

	return s;
}

char *strncat(char *dest, const char *src, size_t n) {
	char *s = dest;

	while (*dest) dest++;
	while (n-- > 0 && (*dest++ = *src++));
	*dest = '\0';

	return s;
}

char *strchr(const char *s, int c) {
	unsigned char _c = (unsigned int)c;

	while (*s && *s != c) s++;
	if (*s != c) s = NULL;

	return (char *)s;
}

char *strrchr(const char *s, int c) {
	const char *last = NULL;

	if (c) {
		while (s = strchr(s, c)) {
			last = s++;
		}
	} else {
		last = strchr(s, c);
	}

	return (char *)last;
}

int strcmp(const char *s1, const char *s2) {
	while (*s1 != '\0' && *s1 == *s2) {
		s1++; s2++;
	}

	return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {
	if (n == 0) return 0;

	while (n-- != 0 && *s1 == *s2) {
		if (n == 0 || *s1 == '\0') break;
		s1++; s2++;
	}

	return (*(unsigned char *)s1) - (*(unsigned char *)s2);
}

char *strcpy(char *dest, const char *src) {
	char *s = dest;

	while (*dest++ = *src++);

	return s;
}

char *strncpy(char *dest, const char *src, size_t n) {
	char *s = dest;

	while (*dest++ = *src++);

	return s;
}

size_t strspn(const char *s, const char *accept) {
	const char *_s = s;
	const char *c;

	while (*s) {
		for (c = accept; *c; c++) {
			if (*s == *c) break;
		}
		if (*c == '\0') break;
		s++;
	}

	return s - _s;
}

size_t strcspn(const char *s, const char *reject) {
	const char *_s = s;
	const char *c;

	while (*s) {
		for (c = reject; *c; c++) {
			if (*s == *c) break;
		}
		if (*c) break;
		s++;
	}

	return s - _s;
}

char *index(const char *s, int c) {
	return strchr(s, c);
}

char *rindex(const char *s, int c) {
	return strrchr(s, c);
}

size_t strlen(const char *s) {
	const char *start = s;

	while (*s) s++;

	return s - start;
}

char *strpbrk(const char *s, const char *accept) {
	const char *c = accept;

	if (!*s) return NULL;

	while (*s) {
		for (c = accept; *c; c++) {
			if (*s == *c) break;
		}
		if (*c) break;
		s++;
	}

	if (*c == '\0') s = NULL;

	return (char *)s;
}

char *strstr(const char *haystack, const char *needle) {
	if (*haystack == 0) {
		if (*needle) return (char *) NULL;
		return (char *) haystack;
	}

	while (*haystack) {
		size_t i;
		i = 0;

		for (;;) {
			if (needle[i] == 0) {
				return (char *) haystack;
			}

			if (needle[i] != haystack[i]) break;
			i++;
		}
		haystack++;
	}

	return (char *) NULL;
}

#define _CTYPE_DATA_0_127 \
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, \
	_C,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C,	_C, \
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, \
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, \
	_S|_B,	_P,	_P,	_P,	_P,	_P,	_P,	_P, \
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, \
	_N,	_N,	_N,	_N,	_N,	_N,	_N,	_N, \
	_N,	_N,	_P,	_P,	_P,	_P,	_P,	_P, \
	_P,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U, \
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U, \
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U, \
	_U,	_U,	_U,	_P,	_P,	_P,	_P,	_P, \
	_P,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L, \
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L, \
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L, \
	_L,	_L,	_L,	_P,	_P,	_P,	_P,	_C

#define _CTYPE_DATA_128_256 \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0


const char _ctype_[1 + 256] = {
	0,
	_CTYPE_DATA_0_127,
	_CTYPE_DATA_128_256
};

char *strtok(char *s, const char *delim) {
}

long _strtol_r(const char *nptr, char **endptr, int base) {
	register const char *s = nptr;
	register unsigned long acc;
	register int c;
	register unsigned long cutoff;
	register int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
               if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
//		rptr->_errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}

long int strtol(const char *nptr, char **endptr, int base) {
	return _strtol_r (nptr, endptr, base);
}

unsigned long int strtoul(const char *nptr, char **endptr, int base) {
}

void atob() {
}

wchar_t *wmemcpy(wchar_t *dest, const wchar_t *src, size_t n) {
}

wchar_t *wmemset(wchar_t *wcs, wchar_t wc, size_t n) {
}

void prnt() {
}

int sprintf(char *str, const char *format, ...) {
}

int vsprintf(char *str, const char *format, va_list ap) {
}




///////////////////////////////////////////////////////////////////////
void _retonly(){}

struct export sysclib_stub={
	0x41C00000,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"sysclib",
	(func)_start,	// entrypoint
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)setjmp,
	(func)longjmp,
	(func)toupper,
	(func)tolower,
	(func)look_ctype_table,
	(func)get_ctype_table,
	(func)memchr,
	(func)memcmp,
	(func)memcpy,
	(func)memmove,
	(func)memset,
	(func)bcmp,
	(func)bcopy,
	(func)bzero,
	(func)prnt,
	(func)sprintf,
	(func)strcat,
	(func)strchr,
	(func)strcmp,
	(func)strcpy,
	(func)strcspn,
	(func)index,
	(func)rindex,
	(func)strlen,
	(func)strncat,
	(func)strncmp,
	(func)strncpy,
	(func)strpbrk,
	(func)strrchr,
	(func)strspn,
	(func)strstr,
	(func)strtok,
	(func)strtol,
	(func)atob,
	(func)strtoul,
	(func)wmemcpy,
	(func)wmemset,
	(func)vsprintf,
	0
};

struct export stdio_stub={
	0x41C00000,
	0,
	VER(1, 2),	// 1.2 => 0x102
	0,
	"stdio",
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
};

//////////////////////////////entrypoint///////////////////////////////
int _start(int argc, char* argv[]){
	int ret = RegisterLibraryEntries(&sysclib_stub);
	return RegisterLibraryEntries(&stdio_stub) + ret;
}

