#ifndef __THSEMAP_H__
#define __THSEMAP_H__

#define THSEMAP_VER	0x101

struct thsema_sema{
    unsigned int	attr;
    unsigned int	option;
    int			initCount;
    int			maxCount;
};

int CreateSema(struct thsema_sema* sema);	// 4(15)
int SignalSema(int semaid);			// 6(15)
int WaitSema(int semaid);			// 8(15)

#endif//__THSEMAP_H__
