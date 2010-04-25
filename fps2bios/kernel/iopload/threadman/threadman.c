//[module]	THREADMAN
//[processor]	IOP
//[type]	ELF-IRX
//[name]	Multi_Thread_Manager
//[version]	0x101
//[memory map]	0xBF802070
//[handlers]	-
//[entry point]	start, thbase_stub, thevent_stub, thsemap_stub,
//		thmsgbx_stub, thfpool_stub, thvpool_stub, thrdman_stub
//[made by]	[RO]man (roman_ps2dev@hotmail.com) november 2002 - january 2003

#include "err.h"
#include "ksysmem.h"
#include "kloadcore.h"
#include "kintrman.h"
#include "kstdio.h"
#include "ksysclib.h"
#include "ktimrman.h"
#include "kheaplib.h"
#include "iopdebug.h"
#include "kthbase.h"

int debug=1;

#define _dprintf(fmt, args...) \
	if (debug > 0) __printf("threadman:" fmt, ## args)

int _start();


void _retonly() {}

struct export thbase_stub={
	0x41C00000,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"thbase",
	(func)_start,	// entrypoint
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly, // 0x08
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly, // 0x10
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly, // 0x18
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly, // 0x20
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly, // 0x28
	(func)_retonly,
	0
};

struct export thevent_stub={
	0x41C00000,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"thevent",
	(func)_start,	// entrypoint
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly, // 0x08
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	0
};

struct export thsemap_stub={
	0x41C00000,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"thsemap",
	(func)_start,	// entrypoint
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly, // 0x08
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	0
};


int _start() {
	int x;

	_dprintf("_start\n");

//	if (RegisterNonAutoLinkEntries(&thrdman_stub))	return 1;
	if (RegisterLibraryEntries(&thbase_stub))	return 1;

	CpuSuspendIntr(&x);

	RegisterLibraryEntries(&thevent_stub);
	RegisterLibraryEntries(&thsemap_stub);
//	RegisterLibraryEntries(&thmsgbx_stub);
//	RegisterLibraryEntries(&thfpool_stub);
//	RegisterLibraryEntries(&thvpool_stub);

	CpuEnableIntr();
}

#if 0
unsigned int ready;
unsigned int _263;
unsigned int _14;

///////////////////////////////////////////////////////////////////////
unsigned int *threadman_callx(){
	return &ready;
}

///////////////////////////////////////////////////////////////////////
int thbase_start(int argc, char* argv[]){
	int x;

	if (RegisterNonAutoLinkEntries(&thrdman_stub))	return 1;
	if (RegisterLibraryEntries(&thbase_stub))	return 1;

	CpuSuspendIntr(&x);

	RegisterLibraryEntries(&thevent_stub);
	RegisterLibraryEntries(&thsemap_stub);
	RegisterLibraryEntries(&thmsgbx_stub);
	RegisterLibraryEntries(&thfpool_stub);
	RegisterLibraryEntries(&thvpool_stub);

	memset(&ready, 0, 1220);			//305 x 4 => zero all bss

	_unk3=8;
	_2=NULL;
	LL_reset(&a);
	LL_reset(&b);
	LL_reset(&c);
	LL_reset(&d);
	LL_reset(&e);
	LL_reset(&f);
	LL_reset(&g);
	LL_reset(&h);
	LL_reset(&i);
	LL_reset(&threads_list);

	for (j=0; j<128; j++)
		LL_reset(&prio_lists[j]);
	hp=CreateHeap(0x800, 1);		//s3=&hp
	th=allocheap(0x7F01, sizeof(struct TCB));
	th->id=++ids;//short
	th->thp_stackSize=512;
	th->stack=AllocSysMemory(ALLOC_LAST, 512, 0);
	th->prio=th->thp_priority=127;//LOWEST_PRIORITY+1
	th->thp_attr=TH_C;
	th->status=THS_READY;
	th->gp=gp;
	th->thp_entry=infinite_call;
	th->area=th->stack + ((th->thp_stackSize >> 2) << 2) - 0xB8;
	memset(th->area, 0, 0xB8);
	th->area->field_00=0xFFFFFFFE;
	th->area->H78=th->area->H74==&th->area->regs;
	th->area->ExitThread=ExitThread;
	th->area->gp=th->gp;
	th->area->attr=(th->attr & TH_COP)   | 0x404;
	th->area->attr|=th->attr & TH_UMODE;
	th->area->entry=th->thp_entry;
	th->area->ei=1;
	_2.next=th;
	_2.prev=th;
	enqueue(th);

	th=allocheap(0x7F01, sizeof(struct TCB));
	th->id=++ids;//short
	th->thp_stackSize=QueryBlockSize(&j);
	th->stack=QueryBlockTopAddress(&j);
	th->thp_priority=8;			//MODULE_INIT_PRIORITY
	th->prio=1;				//HIGHEST_PRIORITY
	th->thp_attr=0x2000000;
	th->status=THS_RUN;
	th->gp=gp;
	th->next_2=_2.next;
	_2.next=th;
	ready.prev=ready.next=th;
	th->next=NULL;
	th->prev=NULL;
	SetCtxSwitchHandler(handler1_switchcontexts);
	SetCtxSwitchReqHandler(handler2_morethan1ready);
	sub_50A0();

	efp.attr=2;
	efp.initPattern=0;
	efp.option=0;
	systemStatusFlag=CreateEventFlag(&efp);

	if (v0=QueryBootMode(4))
		SetEventFlag(systemStatusFlag, 1<<(v0->H0 & 3));//H0 short

	loadcore_call20_registerFunc(&lc20_2, 2, 0);
	loadcore_call20_registerFunc(&lc20_3, 3, 0);
	CpuEnableIntr();
	return 0;
}

///////////////////////////////////////////////////////////////////////
int lc20_2(){
	CpuEnableIntr();

	return 0;
}

///////////////////////////////////////////////////////////////////////
int lc20_3(int a){
	_dprintf("%s\n", __FUNCTION__);
	CpuEnableIntr();

	ChangeThreadPriority(0, 126);//LOWEST_PRIORIT
	if (a->H0 == 0) {
		for (;;) {
			DelayThread(1000000);	//never exit
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////
void infinite_call(){
	infinite_call();
}

///////////////////////////////////////////////////////////////////////
void syscall_32(){
	li      $v0, 0x20
	syscall  #0
}

///////////////////////////////////////////////////////////////////////
void enqueue(struct TCB *th){
	flags[th->prio>>5] |= 1 << (th->prio & 0x1F);
	LL_add_before(&prio_lists[th->prio], th);
}

///////////////////////////////////////////////////////////////////////clone?
void enqueue2(struct TCB *th){
	flags[th->prio>>5] |= 1 << (th->prio & 0x1F);
	LL_add_before(&prio_lists[th->prio], th);
}

///////////////////////////////////////////////////////////////////////
void unqueue(ll, prio){
	if (LL_0_1_elements(ll)
		flags[prio>>5] &= ~(1 << (prio & 0x1F))
	LL_remove(ll);
}

///////////////////////////////////////////////////////////////////////
int run(struct TCB *th, int x){
	if (th->prio >= ready.next->prio){
		th->status=THS_READY;
		enqueue(th);
		CpuResumeIntr(x);
		return KE_OK;
	}else{
		ready.next->status=THS_READY;
		enqueue2(ready.next);
		th->status=THS_RUN;
		ready->prev=th;
		return syscall_32(0, 0, x, 0);
	}
}

///////////////////////////////////////////////////////////////////////
int getHighestUsed(){
	int i;
	for (i=0; i<4; i++)
		if (flags[i])
			return (i<<5)+getHighestUsed_32(flags[i]);
	return 128;	//LOWEST_PRIORITY+2
}

///////////////////////////////////////////////////////////////////////
void switch(){
	int hp;

	ready.prev=ready.next;
	if (ready.next->status != THS_RUN){
		if (unk3 & 4)
			Kprintf("    not THS_RUN ");
		hp=getHighestUsed();
		if (hp<128){
			if (unk3 & 4)
				Kprintf(" readyq = %x, newrun = %x:%d, prio = %d",
					&prio_lists[hp], prio_lists[hp].next, prio_lists[hp].next->id, hp);
			unqueue(prio_lists[hp].next, hp);
			prio_lists[hp].next->status=THS_RUN;
			ready.prev=prio_lists[hp].next;
		}else
			Kprintf("Panic: not found executable Thread\n");
	}else
		if ((hp=getHighestUsed())<128){
			if (unk3 & 4)
				Kprintf("    THS_RUN cp=%d : hp=%d ", ready.next->prio, hp);
			if (hp<ready.next->prio){
				if (unk3 & 4)
					Kprintf("  readyq = %x, newrun = %x:%d, prio = %d",
						&prio_list[hp], prio_list[hp].next, prio_list[hp].next->id, hp);
				unqueue(prio_list[hp].next, hp);
				prio_list[hp].next->status=ready.next->status;
				ready.prev=prio_lists[hp].next;
				ready.next->status=THS_READY;
				enqueue2(ready.next);
			}
		}else
			Kprintf("Panic: not found ready Thread\n");
	if (unk3 & 4)
		Kprintf("\n");
}

///////////////////////////////////////////////////////////////////////
void stack_overflow(struct TCB *th){
	register ImageInfo ii;
	register char *name;
	Kprintf("\nThread (thid=%x, #%d) stack overflow\n Stack = %x, Stack size = %x, SP=%x\n",
		(th<<5) | ((th->id & 0x3F) << 1) | 1, th->id & 0xFFFF, th->stack, th->thp_stackSize, th->area);
	if ((ii=loadcore_call24_findImageInfo(th->thp_entry)) &&
	    (name=ii->H04))
		Kprintf(" Module Name = %s\n", name);
	__asm break #0x400
);

///////////////////////////////////////////////////////////////////////
struct TCB *handler1_switchcontexts(struct AREA *area){
	s0=&unk4;
	unk4++;
	if (unk3 & 3){
		if (1==unk3 & 3)
			Kprintf("[%3d->", ready.next->id);
		if (2==unk3 & 3)
			Kprintf("switch_context(%x:%x,pc=%x,ei=%x =>%x:%d)\n",
				area, area->field_00, area->entry, area->ei, ready.next, ready.next->id);
	}
	ready.next->area=area;
	if (area < ready.next->stack)
		stack_overflow(ready.next);
	if (ready.prev->next==NULL)
		switch();
	if (ready.prev != ready.next){
		s0=GetTimerCounter(timid);
		ready.next->field_30=moveregs(0, s0-www.prev, ready.next->field_30);
		www.prev=s0;
	}
	if (unk3 & 3){
		if (unk3 & 3 == 1)
			Kprintf("%3d]", ready.prev->id);
		if (unk3 & 3 == 2)
			Kprintf("   switch_context --> %x:%x,pc=%x,ei=%x =>%x:%d\n"
				ready.prev->area, ready.prev->area->field_00, ready.prev->area->entry,
				ready.prev->area->ei, ready.prev, ready.prev->id);
	}
	if (unk3 & 0x20){
		0xBF802070=~(1 << ((ready.prev->id-1) & 7));
	}
	return ready.prev;
}

///////////////////////////////////////////////////////////////////////
int handler2_morethan1ready(){
	return (0 < (ready.prev^ready.next));
	//return  (ready.prev!=ready.next);
}

///////////////////////////////////////////////////////////////////////
void queue_on_prio(struct TCB *th, void *fld, int prio){
	register struct TCB *p;

	LL_remove(th);
	for (p=fld->H14; p !=&fld->H14 && prio>=p->prio; p=p->next);
	LL_add_before(p, th);
}

///////////////////////////////////////////////////////////////////////[04]
int CreateThread(struct ThreadParam *param){
	int x;
	register struct TCB *th;

	if (QueryIntrContext())		return KE_ILLEGAL_CONTEXT;
	if (param->attr & 0x1CFFFFF7)	return KE_ILLEGAL_ATTR;
	if (param->initPriority-1>=126)	return KE_ILLEGAL_PRIORITY;
	if (param->entry & 3)		return KE_ILLEGAL_ENTRY;
	if (param->stackSize<0x130)	return KE_ILLEGAL_STACK_SIZE;

	CpuSuspendIntr(&x);
	if (th=allocheap(0x7F01, sizeof(struct TCB))){	//0x50
		param->stackSize = (param->stackSize+0xFF) & 0xFFFFFF00;
		stack=AllocSysMemory(ALLOC_LAST, param->stackSize, NULL);
		if (!stack)
			freeheap(th);
	}
	if (!th || !stack){
		CpuResumeIntr(x);
		return KE_NO_MEMORY;
	}
	th->id=++ids;
	th->thp_entry=param->entry;
	th->stack=stack;
	th->thp_stackSize;
	th->thp_priority=param->initPriority;
	th->thp_attr=attr;
	th->status=THS_DORMANT;
	th->thp_option=param->option;
	th->gp=$gp;
	th->next_2=&_2;
	_2.next=th;
	LL_add_before(threads_list, th);
	if (!(th->thp_attr & TH_NO_FILLSTACK))
		memset(th->stack, 0xFF, th->thp_stackSize-48);
	CpuResumeIntr(x);
	return ((int)th<<5) | ((th->id & 0x3F) << 1) | 1;
}

///////////////////////////////////////////////////////////////////////[05]
int DeleteThread(int thid){
	int x;
	register struct TCB *th, *p;

	if (QueryIntrContext())		return KE_ILLEGAL_CONTEXT;
	if (thid==0)			return KE_ILLEGAL_THID;

	CpuSuspendIntr(&x);
	th=(struct TCB *)(thid>>7<<2);
	if ((th->_7F01 != 0x7F01) ||
	    (((thid>>1) & 0x3F) != (th->id & 0x3F)) ||
	    (_2.prev == th)){
		CpuResumeIntr(x);
		return KE_UNKNOWN_THID;
	}
	if (th->status != THS_DORMANT){
		CpuResumeIntr(x);
		return KE_NOT_DORMANT;
	}
	FreeSysMemory(th->stack);
	LL_remove(th);

	if (_2.next==th)
		_2.next=th->next_2;
	else
		for (p=_2.next; p->next_2; p=p->next_2)
			if (p->next_2==th){
				p->next_2=th->next_2;
				break;
			}

	freeheap(th);
	CpuResumeIntr(x);
	return KE_OK;
}

///////////////////////////////////////////////////////////////////////
int startThread(struct TCB *th,int x){
	th->field_1C=0;
	th->field_20=0;
	th->field_1E=0;
	th->prio=thp_priority;
	th->area->field_00=0xFFFFFFFE;
	th->area->field_78=th->area->field_74=&th->area->regs;
	th->area->ExitThread=ExitThread;
	th->area->gp=th->gp;
	th->area->attr=(th->attr & TH_COP)   | 0x404;
	th->area->attr|=th->attr & TH_UMODE;
	th->area->entry->th->thp_entry;
	th->ei=1;
	LL_remove(th);
	return run(th, x);
}

///////////////////////////////////////////////////////////////////////[06]
int StartThread(int thid,u_long arg){
	int x;
	register struct TCB *th, *p;

	if (QueryIntrContext())		return KE_ILLEGAL_CONTEXT;
	if (thid==0)			return KE_ILLEGAL_THID;

	CpuSuspendIntr(&x);
	th=(struct TCB *)(thid>>7<<2);
	if ((th->_7F01 != 0x7F01) ||
	    (((thid>>1) & 0x3F) != (th->id & 0x3F))){
		CpuResumeIntr(x);
		return KE_UNKNOWN_THID;
	}
	if (th->status != THS_DORMANT){
		CpuResumeIntr(x);
		return KE_NOT_DORMANT;
	}
	th->area = th->stack + ((th->thp_stackSize >> 2) << 2) - 0xB8;
	memset(th->area, 0, 0xB8);
	th->area->arg=arg;
	return startThread(th, x);
}

///////////////////////////////////////////////////////////////////////[07]
int StartThreadArgs(int thid,int args,void *argp){
	int x;
	register struct TCB *th, *p;
	register void *_argp;

	if (QueryIntrContext())		return KE_ILLEGAL_CONTEXT;
	if (thid==0)			return KE_ILLEGAL_THID;

	CpuSuspendIntr(&x);
	th=(struct TCB *)(thid>>7<<2);
	if ((th->_7F01 != 0x7F01) ||
	    (((thid>>1) & 0x3F) != (th->id & 0x3F))){
		CpuResumeIntr(x);
		return KE_UNKNOWN_THID;
	}
	if (th->status != THS_DORMANT){
		CpuResumeIntr(x);
		return KE_NOT_DORMANT;
	}

	_argp=th->stack + ((th->thp_stackSize >> 2) - ((args+3) >> 2)) << 2
	if (args>0 && argp)
		memcpy(_argp, argp, args);

	memset(th->area, 0, 0xB8); //BUG: these instructions should be switched!
	th->area = th->stack + ((th->thp_stackSize >> 2) << 2) - 0xB8;

	th->area->arg =args;
	th->area->argp=_argp;
	return startThread(th, x);
}

///////////////////////////////////////////////////////////////////////[08]
int ExitThread(){
	th=ready->next;
	if (QueryIntrContext())		return KE_ILLEGAL_CONTEXT
	CpuSuspendIntr(&x);
	th->status=THS_DORMANT;
	LL_add_before(&threads_list, th);
	a0=0;
	do{
		ready.prev=0;
		syscall_32(0, 0, x, 0);
		Kprintf("panic ! Thread DORMANT !\n");
		__asm break    #0x400
	while(1);
}

///////////////////////////////////////////////////////////////////////[09]
int ExitDeleteThread(){
	return KE_ERROR;
}

///////////////////////////////////////////////////////////////////////[10]
int TerminateThread(int thid){
	int x;
	register struct TCB *th, *p;

	if (QueryIntrContext())		return KE_ILLEGAL_CONTEXT;
	if (thid==0)			return KE_ILLEGAL_THID;

	CpuSuspendIntr(&x);
	th=(struct TCB *)(thid>>7<<2);
	if (ready.next==th){
		CpuResumeIntr(x);
		return KE_ILLEGAL_THID;
	}
	if ((th->_7F01 != 0x7F01) ||
	    (((thid>>1) & 0x3F) != (th->id & 0x3F))){
		CpuResumeIntr(x);
		return KE_UNKNOWN_THID;
	}
	if (th->status == THS_DORMANT){
		CpuResumeIntr(x);
		return KE_DORMANT;
	}
	LL_remove(th);
	if (th->status==THS_WAIT)
		if (th->field_1C==2)
			CancelAlarm(alarmhandler, th);
		else
			if (th->field_1C>=2) && (th->field_1C<8)
				th->field_20->H10--;
	th->status=THS_DORMANT;
	LL_add_before(&threads_list, th);

	CpuResumeIntr(x);
	return KE_OK;
}

///////////////////////////////////////////////////////////////////////
void LL_add_before(struct ll *list, struct ll *new){
	new->next=list;
	new->prev=list->prev;
	list->prev=new;
	new->prev->next=new;
}

///////////////////////////////////////////////////////////////////////:)
int getHighestUsed_32(int flags_32){
	v=-4;
	do{
		i=flags_32<<28;
		flags_32>>=4;
		v+=4;
	}while(i==0);
	return v + ((0x1020103 >> ((i>>26) & 0x1C)) & 0xF);
}

#endif
