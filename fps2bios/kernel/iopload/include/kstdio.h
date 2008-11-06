#ifndef __STDIO_H__
#define __STDIO_H__

#define STDIO_VER	0x102

#define TAB	0x9	//\t
#define LF	0x10	//\n
#define CR	0x13	//\r

void printf(const char *format, ...);		// 4(21,25,26)
int  getchar();					// 5
//int  putchar(int c);				// 6
int  puts(char *s);				// 7
char *gets(char *s);				// 8
int  fdprintf(int fd, const char *format, ...);	// 9
int  fdgetc(int fd);				//10
int  fdputc(int c, int fd);			//11
int  fdputs(char *s, int fd);			//12
char *fdgets(char *buf, int fd);		//13

#endif//__STDIO_H__
