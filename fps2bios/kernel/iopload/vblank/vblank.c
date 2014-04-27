#include "kloadcore.h"
#include "kintrman.h"
#include "ksysclib.h"
#include "kthbase.h"
#include "thevent.h"
#include "err.h"

#define _dprintf(fmt, args...) \
	__printf("vblank: " fmt, ## args)

struct DCLL
{
	struct DCLL	*next;
	struct DCLL	*prev;
	int	priority;
	int	(*function)();
	void	*this;			//'this' style pointer
};					//intrh_vblank, intrh_evblank, vblank_0, vblank_1
					//cannot access global v
struct VBHS
{
	int		statusFlag;		//set to 0 in 'start' function
	int		count;
	struct DCLL	list0;		//sentinel, dummy list head (interrupt 0 vblank)
	struct DCLL	list11;		//sentinel, dummy list head (interrupt 11 evblank)
	struct DCLL	free;		//available blocks
	struct DCLL	items[16];
} v;

int  _start();
struct VBHS *return_v();
int WaitVblankStart();
int WaitVblankEnd();
int WaitVblank();
int WaitNonVblank();
int RegisterVblankHandler(int number, int priority, int (*)(struct VBHS *), struct VBHS*);
int ReleaseVblankHandler(int number, int (*)(struct VBHS *));

void DCLL_del(struct DCLL *list);
void DCLL_add_before(struct DCLL *list, struct DCLL *new);
void DCLL_add_after(struct DCLL *list, struct DCLL *new);

///////////////////////////////////////////////////////////////////////
struct VBHS *return_v(){
	return &v;
}

///////////////////////////////////////////////////////////////////////
int RegisterVblankHandler(int number, int priority, int (*handler)(struct VBHS *), struct VBHS* this)
{
    struct DCLL *p, *list;
	u32 x;

	if (QueryIntrContext()){
		_dprintf("%s - ERROR_INTR_CONTEXT\n", __FUNCTION__);
		return ERROR_INTR_CONTEXT;
	}
	CpuSuspendIntr(&x);

	if (DCLL_is_only_sentinel(&v.free)){
		CpuResumeIntr(x);
		_dprintf("%s - ERROR_NO_MEM\n", __FUNCTION__);
		return ERROR_NO_MEM;
	}

	list=(number? &v.list11 : &v.list0);	//which list: 0 or 11?
	p=list->next;
	if (p != list){				//search for an existing instance in the list
		do{
			if (p->function == handler){
				CpuResumeIntr(x);		//intrman
				return ERROR_DOES_EXIST;//already added
			}
			p=p->next;
		}while(p != list);
		p = list->next;
	}
	while (p != list){			// while we have elements in list ...
		if (priority < p->priority)
			break;			//greater priority
		p=p->next;
	}

	struct DCLL* node = v.free.next;
	DCLL_del(node);
	node->function=handler;
	node->this    =this;
	node->priority=priority;
	DCLL_add_after(p, node);

	CpuResumeIntr(x);
	return ERROR_OK;
}

///////////////////////////////////////////////////////////////////////
int  ReleaseVblankHandler(int number, int (*handler)(struct VBHS *)){
    struct DCLL *p, *list;
	u32 x;

	if (QueryIntrContext())
		return ERROR_INTR_CONTEXT;
	CpuSuspendIntr(&x);

	for (list=(number?&v.list11:&v.list0), p=list->next; p!=list; p=p->next)
		if (p->function == handler){	//bingo
			DCLL_del(p);
			DCLL_add_before(&v.free, p);

			CpuResumeIntr(x);
			return ERROR_OK;
		}

	CpuResumeIntr(x);
	return ERROR_DOESNOT_EXIST;				//not found
}

///////////////////////////////////////////////////////////////////////
int intrh_vblank(struct VBHS *m)
{
    struct DCLL *p;
	struct DCLL *cp;

	if (m->count++ == 0){
		iSetEventFlag(GetSystemStatusFlag(), 0x200);
	}

	for (p = m->list0.next; p != &(m->list0); p = cp){
		cp=p->next;
		if ((p->function)(p->this)==0){
			DCLL_del(p);
			DCLL_add_before(&m->free, p);
		}
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////
int intrh_evblank(struct VBHS *m)
{
    struct DCLL *p, *cp;
	_dprintf("%s\n", __FUNCTION__);

	for (p=m->list11.next; p != &m->list11; p=cp ){
		cp=p->next;
		if ((p->function)(p->this)==0){
			DCLL_del(p);
			DCLL_add_before(&m->free, p);
		}
	}

	return 1;
}

///////////////////////////////////////////////////////////////////////
int vblankh_0(struct VBHS *m)
{
//	_dprintf("%s\n", __FUNCTION__);

	iSetEventFlag(m->statusFlag, 1);	//  0001b	//thevent
	iSetEventFlag(m->statusFlag, 2);	//  0010b	//thevent
	iClearEventFlag(m->statusFlag, ~(1|8));//~(1001b)=0xFFFFFFF6//thevent
	return 1;
}

///////////////////////////////////////////////////////////////////////
int vblankh_1(struct VBHS *m)
{
	_dprintf("%s\n", __FUNCTION__);

	iSetEventFlag(m->statusFlag, 4);	//  0100b	//thevent
	iSetEventFlag(m->statusFlag, 8);	//  1000b	//thevent
	iClearEventFlag(m->statusFlag, ~(2|4));//~(0110b)=0xFFFFFFF9//thevent
	return 1;
}

///////////////////////////////////////////////////////////////////////
int WaitVblankStart(){
	return WaitEventFlag(v.statusFlag, 1, 1, 0);		//thevent
}

///////////////////////////////////////////////////////////////////////
int WaitVblankEnd(){
	return WaitEventFlag(v.statusFlag, 4, 1, 0);		//thevent
}

///////////////////////////////////////////////////////////////////////
int WaitVblank(){
	return WaitEventFlag(v.statusFlag, 2, 1, 0);		//thevent
}

///////////////////////////////////////////////////////////////////////
int WaitNonVblank(){
	return WaitEventFlag(v.statusFlag, 8, 1, 0);		//thevent
}

///////////////////////////////////////////////////////////////////////
//static allocated Double Cyclic sentinel priority Linked List (whew)//
///////////////////////////////////////////////////////////////////////
void DCLL_make_sentinel(struct DCLL *list){
	list->next=list;
	list->prev=list;
}

///////////////////////////////////////////////////////////////////////
int  DCLL_is_only_sentinel(struct DCLL *list){
	return (list->next == list);
}

///////////////////////////////////////////////////////////////////////
//          x <-> list <-> y
// becomes
//          x <-> y
void DCLL_del(struct DCLL *list)
{
	struct DCLL* x = list->prev;
	struct DCLL* y = list->next;
	x->next = y;
	y->prev = x;
}

///////////////////////////////////////////////////////////////////////
void DCLL_add_before(struct DCLL *list, struct DCLL *new)
{
	new->next=list;
	new->prev=list->prev;
	list->prev=new;
	new->prev->next=new;
}

//
// list <-> x
// list <-> new <-> x
void DCLL_add_after(struct DCLL *list, struct DCLL *new)
{
	struct DCLL *x = list->next;
	new->prev = list;
	new->next = x;
	list->next = new;
	x->prev = new;
}

void _retonly(){}

//////////////////////////////entrypoint///////////////////////////////
struct export export_stub={
	0x41C00000,
	0,
	VER(1, 1),			// 1.1 => 0x101
	0,
	"vblank\0\0",
	(func)_start,			// entrypoint
	(func)_retonly,
	(func)_retonly,
	(func)return_v,
	(func)WaitVblankStart,		// 4
	(func)WaitVblankEnd,		// 5
	(func)WaitVblank,		// 6
	(func)WaitNonVblank,		// 7
	(func)RegisterVblankHandler,	// 8
	(func)ReleaseVblankHandler,	// 9
	0				// end of list
};

//////////////////////////////entrypoint///////////////////////////////
int _start(){
    iop_event_t e;
    u32 x;
	int i;

	CpuSuspendIntr(&x);

	if (RegisterLibraryEntries(&export_stub)){
		CpuResumeIntr(x);
		return 1;
	}

	memset(&v, 0, sizeof(struct VBHS));			// 352	//sysclib
	DCLL_make_sentinel(&v.list0);
	DCLL_make_sentinel(&v.list11);
	DCLL_make_sentinel(&v.free);
	for (i=0; i<16; i++){				//link 16 elements
		DCLL_add_after(&v.free, &v.items[i]);//to 'available' list
	}

	e.bits=0;
	e.attr=2;
	e.option=0;
	CreateEventFlag(&e);					//thevent

	v.statusFlag=0;
	RegisterVblankHandler(0, 128, vblankh_0, &v);
	RegisterVblankHandler(1, 128, vblankh_1, &v);
	RegisterIntrHandler( 0, 1, (intrh_func)intrh_vblank, &v);
	RegisterIntrHandler(11, 1, (intrh_func)intrh_evblank,&v);
	EnableIntr(0);
	EnableIntr(11);

	CpuResumeIntr(x);
	return 0;
}
