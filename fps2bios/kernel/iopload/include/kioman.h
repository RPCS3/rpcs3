#ifndef __IOMAN_H__
#define __IOMAN_H__

#define IOMAN_VER	0x102

#define STDIN	0
#define STDOUT	1

struct ioman_FUNCS{
	int  init(struct ioman_DRV *drv);				//00
	void term(struct ioman_DRV *drv);				//01
	int  format(char *filename);					//02
	int  open(int fd, char *filename, int flag);			//03
	int  close(int fd);						//04
	int  read(int fd, void *buf, int nbyte);			//05
	int  write(int fd, void *buf, int nbyte);			//06
	int  lseek(int fd, int offset, int whence);			//07
	int  ioctl(int fd, int request, int arg);			//08
	int  remove(char *filename);					//09
	int  mkdir(char *filename);					//10
	int  rmdir(char *filename);					//11
	int  dopen(char *filename);					//12
	int  dclose(int fd);						//13
	int  dread(int fd, void *buf);					//14
	int  getstat(char *filename, void *buf);			//15
	int  chstat(char *filename, void *buf, unsigned int mask);	//16
};

struct ioman_DRV{	//struct fileio_driver from ps2lib
	char			*driver;	//+00
	int			unk1;		//+04
	int			version;	//+08
	char			*description;	//+0C
	struct ioman_FUNCS	*f_list;	//+10
};						//=14

int  open(char *filename, int flag);				// 4(26)
int  close(int fd);						// 5(26)
int  read(int fd, void *buf, long count);			// 6(17,26)
int  write(int fd, void *buf, long count);			// 7(17,26)
int  lseek(int fd, int offset, int whence);			// 8(26)
int  ioctl(int fd, int request, int arg);			// 9(26)
int  remove(char *filename);					//10(26)
int  mkdir(char *filename);					//11(26)
int  rmdir(char *filename);					//12(26)
int  dopen(char *filename);					//13(26)
int  dclose(int fd);						//14(26)
int  dread(int fd, void *buf);					//15(26)
int  getstat(char *filename, void *buf);			//16(26)
int  chstat(char *filename, void *buf, unsigned int mask);	//17(26)
int  format(char *filename);					//18(26)
int  AddDrv(struct ioman_DRV *drv);				//20(26)
int  DelDrv(char *device);					//21(26)

#endif//__IOMAN_H__

