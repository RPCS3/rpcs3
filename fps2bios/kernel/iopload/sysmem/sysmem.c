//[module]	SYSMEM
//[processor]	IOP
//[type]	ELF-IRX
//[name]	System_Memory_Manager
//[version]	0x101
//[memory map]	0x00001500(0x000015C3) in RAM
//[handlers]	see fileio/80000003
//[entry point]	sysmem_start, sysmem_stub(it is handeled specialy by loadcore)
//[made by]	[RO]man (roman_ps2dev@hotmail.com) september 2002

#define min(a, b)	((a)<(b)?(a):(b))
#define max(a, b)	((a)>(b)?(a):(b))

#include <tamtypes.h>

#include "ksysmem.h"
#include "kloadcore.h"
#include "iopdebug.h"

#define EIGHTMEGSm256	(8*1024*1024-256)	//0x007FFF00
#define ALIGN256	0xFFFFFF00

#define mALLOCATED(info)	 (info & 1)
#define mADDRESS(info)		((info >> 1) & 0x7FFF)
#define mSIZE(info)		 (info >> 17)
//[000000000000000 0  000000000000000 1]
//[ size of block |?|address of block|a]located
//upper 15bits are the size of block in 256bytes units [8MB addressability]
//unknown 1 bit
//15bits for address of block in 256bytes units [8MB addressability; 15+8 bits]
//the lower bit means that block is allocated or not (1/0)

struct allocELEM{	//sizeof()=8
	struct allocELEM	*next;
/*	unsigned int    size     :15,
			unknown  :1,	// info could be declared this way
			addr     :15,
			allocated:1;	*/
	unsigned int		info;	// here is stored data about
					// an allocable memory chunk
};

#define smFIRST		0
#define smFIRSTF	2		//first one that can be freed
#define smCHECK		27
#define smLAST		30
#define smMAX		(smLAST+1)	//max number of elements = 31

struct allocTABLE{	//sizeof()=256=0x100
	struct allocTABLE	*next;		//+00
	struct allocELEM	list[31];	//+04 8 * 31 = 248 = 0xF8;
	int			dummy;
};

char* (*_Kprintf)(unsigned int, ...);
unsigned int unk;

///////////////////////////////////////////////////////////////////////
unsigned int		memsize;

void		*_start(u32 iopmemsize);
void		*sysmem_init_memory();
void		sysmem_retonly();
unsigned int	*sysmem_return_addr_of_memsize();

//////////////////////////////entrypoint///////////////////////////////
struct export sysmem_stub __attribute__((section(".text"))) = {
	0x41C00000,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"sysmem",
	(func)_start,	// entrypoint
	(func)sysmem_init_memory,
	(func)sysmem_retonly,
	(func)sysmem_return_addr_of_memsize,
	(func)AllocSysMemory,
	(func)FreeSysMemory,
	(func)QueryMemSize,
	(func)QueryMaxFreeMemSize,
	(func)QueryTotalFreeMemSize,
	(func)QueryBlockTopAddress,
	(func)QueryBlockSize,
	(func)sysmem_retonly,
	(func)sysmem_retonly,
	(func)sysmem_retonly,
	(func)Kprintf,
	(func)sysmem_call15_set_Kprintf,
	0
};

struct allocTABLE	*alloclist;
char _alloclist[0x200]; // temp buffer
//struct allocTABLE	_alloclist; // has to be last!, also temp!

void Kputc(u8 c) {
	*((u8*)0x1f80380c) = c;
}

void Kputs(u8 *s) {
	while (*s != 0) {
		Kputc(*s++);
	}
}

void Kprintnum(unsigned int n)
{
    char chars[16] = "0123456789abcdef";
    int i = 0;
    while(i < 8) {
        Kputc(chars[n>>28]);
        n <<= 4;
        ++i;
    }
}


///////////////////////////////////////////////////////////////////////[OK]
void sysmem_retonly(){}

//////////////////////////////entrypoint///////////////////////////////[OK]
void *_start(u32 iopmemsize)
{
	iopmemsize=min(EIGHTMEGSm256, iopmemsize);

	alloclist = (struct allocTABLE*)(((u32)&_alloclist + 255)  & 0xFFFFFF00); // round up
	memsize = iopmemsize & 0xFFFFFF00;
    
    //__printf("sysmem_start: %x, %x\n", memsize, ((u32)alloclist+sizeof(struct allocTABLE)));
    //Kprintnum((int)alloclist);  Kputs(" alloclist\n");
    //Kprintnum((int)&alloclist);  Kputs(" &alloclist\n");

	if (memsize >= ((u32)alloclist+sizeof(struct allocTABLE)))	//alloctable must fit memory
		return sysmem_init_memory();
	alloclist = NULL;
	return NULL;
}

///////////////////////////////////////////////////////////////////////
extern void* _end;
void *sysmem_init_memory(){	//mark all mem as not allocated, available
	unsigned int		i;
	struct allocELEM	*p;

    //__printf("system_init_memory\n");

	if (alloclist == NULL) return NULL;

	alloclist->next=NULL;
	for (i=smFIRST; i<smMAX; i++){
		alloclist->list[i].next=&alloclist->list[i+1];
		alloclist->list[i].info=0;
	}
	alloclist->list[smLAST].next=NULL;

	alloclist->list[smFIRST].info &= 0x0001FFFF;
	alloclist->list[smFIRST].info |= (((memsize<0 ?
				 memsize+255:memsize) >> 8) << 17);
    
    // _end ~= alloclist + sizeof(struct alocTABLE);

    //this is not NULL! but the starting address of the first 0x1500 bytes allocated
	if ( AllocSysMemory(ALLOC_FIRST, (int)alloclist, NULL)==0) {
        if (alloclist==AllocSysMemory(ALLOC_FIRST, sizeof(struct allocTABLE)-4, NULL)){ //alloc allocated allocation table;-)
            for (p=alloclist->list; p; p=p->next) {
				if (!mALLOCATED(p->info)) {
					return (void*)((mADDRESS(p->info))<<8);//next free block address
                }
            }
			return NULL;
		}
	}
	alloclist=NULL;
	return NULL;
}

///////////////////////////////////////////////////////////////////////[OK]
unsigned int QueryMemSize(){
	if (alloclist)
		return memsize;//2*1024*1024-256=0x001FFF00
	return 0;
}

///////////////////////////////////////////////////////////////////////[OK]
unsigned int QueryMaxFreeMemSize(){
    unsigned int maxfree=0;
    struct allocELEM  *p;

	if (alloclist)
		for (p=alloclist->list; p; p=p->next)
			if (!mALLOCATED(p->info))
				maxfree = max(maxfree, mSIZE(p->info));
	return maxfree<<8;	// => 256bytes allocation units
}

///////////////////////////////////////////////////////////////////////[OK]
unsigned int QueryTotalFreeMemSize(){
    unsigned int freesize=0;
    struct allocELEM  *p;

	if (alloclist)
		for (p=alloclist->list; p; p=p->next)
			if (!mALLOCATED(p->info))
				freesize+=mSIZE(p->info);
	return freesize<<8;	// => 256bytes allocation units
}

void *alloc(int flags, int size, void *mem);
void maintain();
///////////////////////////////////////////////////////////////////////[OK]
void *AllocSysMemory(int flags, int size, void *mem){
	if (alloclist && (flags<3)){
        void *r=alloc(flags, size, mem);
		maintain();
        //Kprintnum((int)r); Kputs("\n");
		return (void*)r;
	}
	return NULL;
}

int  free(void *mem);
///////////////////////////////////////////////////////////////////////[OK]
int  FreeSysMemory(void *mem){
     struct allocTABLE	*table;
     int		r;

	for (table=alloclist; table; table=table->next)	//must not be among
		if (mem==table)				//the allocation tables
			return -1;
	if (r=free(mem))
		return r;
	maintain();
	return r;	//r==0 ;-)
}

struct allocELEM *findblock(void *a);
///////////////////////////////////////////////////////////////////////[OK]
void *QueryBlockTopAddress(void *address){
    struct allocELEM *p;

	if (p=findblock(address))
		return (void*)((mADDRESS(p->info) << 8) +
                     (mALLOCATED(p->info) ? USED : FREE));
	return (void*)-1;
}

///////////////////////////////////////////////////////////////////////[OK]
int  QueryBlockSize(void *address){
    struct allocELEM *p;

	if (p=findblock(address))
		return (mSIZE(p->info) << 8) |
                  (mALLOCATED(p->info) ? USED : FREE);
	return -1;
}

///////////////////////////////////////////////////////////////////////[OK]
unsigned int *sysmem_return_addr_of_memsize(){
	return &memsize;
}

///////////////////////////////////////////////////////////////////////
void *alloc(int flags, int size, void *mem){
     struct allocELEM	*a, *last, *k;
     unsigned int	bsize,		//block size in 256bytes units
			baddress,	//block address in 256bytes units
			i, tmp;
     void		*address;

	bsize = (size+255) >> 8;	// round to upper
	if (bsize==0)		return NULL;

	switch (flags){
	case ALLOC_FIRST:
		for (a = alloclist->list; a; a=a->next)
			if ((!mALLOCATED(a->info)))
				if (mSIZE(a->info) >= bsize)
					break;
		if (a==NULL) return NULL;

		if (mSIZE(a->info) == bsize){
			a->info|=1;			//alloc it
			return (void*)(mADDRESS(a->info) << 8);	//all done, how quick!
		}

		address = (void*)(mADDRESS(a->info) << 8);
		i = a->info;

		a->info=((a->info | 1) & 0x1FFFF) |	//alloc block
			  (bsize << 17);		//of wanted size

		i = (i & 0xFFFF0001) | (((mADDRESS(i) + bsize) & 0x7FFF) << 1);
		i = (i & 0x1FFFF) | ((mSIZE(i) - bsize) << 17);

		a=a->next;
		while (mSIZE(i)>0){
			tmp=a->info;
			a->info=i;
			i=tmp;
			a = a->next;
		}
		return address;

	case ALLOC_LAST:
		last=NULL;
		for (a = alloclist->list; a; a=a->next)
			if ((!mALLOCATED(a->info)))
				if (mSIZE(a->info) >= bsize)
					last=a;		//use last one suitable
		a = last;
		if (a==0) return NULL;

		if (mSIZE(a->info) == bsize){
			a->info|=1;			//alloc it
			return (void*)(mADDRESS(a->info) << 8);
		}

		a->info = (a->info & 0x0001FFFF) | 
		   ((mSIZE(a->info) - bsize) << 17);	//put rest of block

		i =  	(((i & 0xFFFF0001)//this line has no use; this is stupid
			| ((mADDRESS(a->info) + mSIZE(a->info)) & 0x7FFF) << 1)	//calculate address of block to use
			& 0x0001FFFF)	//keep only address (and alloc' state)
			| (bsize << 17)	//put size
			| 1;		//mark as allocated
		a=a->next;

		address = (void*)(mADDRESS(i) << 8); //address in bytes
		while (mSIZE(i)>0){
			tmp=a->info;
			a->info=i;
			i=tmp;
			a = a->next;
		}
		return address;

	case ALLOC_LATER:
		if ((unsigned int)mem & 0xFF) return NULL;
		baddress = (unsigned int)mem >> 8;		//addr in 256bytes units
		for (a = alloclist->list; a ; a=a->next){
			if (baddress < mADDRESS(a->info)) return NULL;//cannot realloc
			if ((!mALLOCATED(a->info)) &&
			     (mSIZE(a->info) + mADDRESS(a->info) >= baddress + bsize))
				break;
		}
		if (a==0) return NULL;
                
		if (mADDRESS(a->info) < baddress){
			tmp = mADDRESS(a->info) + mSIZE(a->info) - baddress;
			a->info= (a->info & 0x1FFFF) |
			   ((mSIZE(a->info) - tmp) << 17);

			i =(((i & 0xFFFF0001) |	//stupid compiler
			     ((mADDRESS(a->info) + mSIZE(a->info)) & 0x7FFF) << 1)
			    & 0x1FFFE)
			    | (tmp << 17);

			k = a = a->next;
			while (mSIZE(i)>0){
				tmp=a->info;
				a->info=i;
				i=tmp;
				a=a->next;
			}
			a = k;
		}

		if (mSIZE(a->info)==bsize){
			a->info|=1;			//alloc it
			return (void*)(mADDRESS(a->info) << 8);
		}

		address = (void*)(mADDRESS(a->info) << 8);
		i = a->info;

		a->info=((a->info | 1) & 0x1FFFF) |	//alloc block
			  (bsize << 17);

		i = (i & 0xFFFF0001) | (((mADDRESS(i) + bsize) & 0x7FFF) << 1);
		i = (i & 0x1FFFF) | ((mSIZE(i) - bsize) << 17);

		a=a->next;
		while (mSIZE(i)>0){
			tmp=a->info;
			a->info=i;
			i=tmp;
			a = a->next;
		}
		return address;

	}

	return NULL;
}
/***original	while (mSIZE(i)>0){
			x=a->next;		//stupid, n'est ce pas?
			tmp=a->info;
			a->next=z;		//stupid, n'est ce pas?
			a->info=i;
			a->next=x;		//stupid, n'est ce pas?
			z=x;			//stupid, n'est ce pas?
			i=tmp;
			a = a->next;
		}	***/

///////////////////////////////////////////////////////////////////////[OK]
int  free(void *mem){
     int		skip;	// counter (0,1,2) of elements to remove
     struct allocELEM	*a,	// an element
			*p,	// prev
			*n;	// next

	if ((unsigned int)mem & 0xFF)		return -1;	// only 256byte multiple

	for (a=&alloclist->list[smFIRSTF], p=NULL; a; a=a->next){
		if ((mSIZE(a->info) && (mADDRESS(a->info)==
						((unsigned int)mem >> 8))))	// convert to 256byte units
			break;
		p=a;
	}

	if (a==NULL)		return -1;	//block not found

	if (!mALLOCATED(a->info))return -1;	//cannot free a freed block

	n=NULL;	
	skip=0;	

	a->info&=0xFFFFFFFE;	//free block

	if ((a->next) && (!mALLOCATED(a->next->info))){//bind with next free block
		n = a->next;
		skip = 1;
		a->info = (a->info & 0x1FFFE) |
		    ((mSIZE(a->info) + mSIZE(a->next->info)) << 17);
	}

	if (p && (!mALLOCATED(p->info))){	//bind with previous free block
		n=a;
		skip++;			// or skip=2;
		p->info=(p->info & 0x1FFFF) | 
		  ((mSIZE(p->info) + mSIZE(a->info)) << 17);
	}

	if (skip){
		a=n;
		while (--skip!=-1)
			a=a->next;
		while (a){
			n->info=a->info;
			a=a->next;
			n=n->next;
		}
	}
	return 0;
}
/***original	while (a){
			t1=n->next;
			t2=a->next;
			t3=a->info;
			n->next=t2;		//stupid compiler, see above
			n->info=t3;
			n->next=t1;
			a=a->next;
			n=t1;
		}***/

///////////////////////////////////////////////////////////////////////[OK]
void maintain(){
     struct allocTABLE	*table, *new;
     int		i;

	for (table=alloclist; table->next; table = table->next)	//+0x00
		;	//move to last non-NULL
	if (mSIZE(table->list[smCHECK].info)>0)		//+0xE0	//27 (i.e.28th)
		// a new table is needed; alloc it
		if (new=(struct allocTABLE*)alloc(0, sizeof(struct allocTABLE), 0)){
			table->next=new;		// link it
			table->list[smLAST].next=&new->list[smFIRST];// link it's table elements
			table=table->next;		// move to it
			table->next=NULL;		// mark as end of tables
			for (i=0; i<smMAX; i++){	// init table elements
				table->list[i].next=&table->list[i+1];
				table->list[i].info=0;
			}
			table->list[smLAST].next=NULL;
		}

	table=alloclist;
	if (table->next){
		while (table->next->next)
			table=table->next;
		if (table->next)	//redundant check
			if (mSIZE(table->list[smCHECK].info)==0){
				struct allocTABLE *t=table->next;
				table->list[smLAST].next=NULL;
				table->next		=NULL;
				free(t);
			}
	}
}

///////////////////////////////////////////////////////////////////////[OK]
struct allocELEM *findblock(void *a){ // it finds the block for a given address
     struct allocELEM *p;
	for (p = alloclist->list; p; p=p->next)
		if (((unsigned int)a >= (mADDRESS(p->info) << 8)) && 
		    ((unsigned int)a <  (mADDRESS(p->info) << 8) + mSIZE(p->info)))
			break;
	return p;
}

///////////////////////////////////////////////////////////////////////[OK]
char *Kprintf(const char *format,...){
	if (_Kprintf)
		return _Kprintf(unk, format, (void*)(format+4));//it is not accurate since mips
												// have the 2nd, 3rd & 4th parameters
												// in regs and not in stack,
												// but you got the idea
	return NULL;
}

///////////////////////////////////////////////////////////////////////[OK]
void sysmem_call15_set_Kprintf(char* (*newKprintf)(unsigned int, const char*, ...), unsigned int newunk){
	if (_Kprintf && newKprintf)
		newKprintf(newunk, _Kprintf(0));
	unk      = newunk;
	_Kprintf = newKprintf;
}
