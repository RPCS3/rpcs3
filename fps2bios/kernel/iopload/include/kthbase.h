#ifndef __THBASE_H__
#define __THBASE_H__

#define THBASE_VER	0x101

struct thbase_thread{
  unsigned int attr;
  unsigned int option;
  int (*entry)();
  unsigned int stackSize;
  unsigned int initPriority;
};

int	CreateThread(struct thbase_thread*);	//4 (21,26)	//returns thid
void	StartThread(int thid, int);		//6 (21,26)
int	GetThreadId();				//20(26)	//returns thid?
int	GetSystemStatusFlag();			//41(13,21)

#endif//__THBASE_H__
