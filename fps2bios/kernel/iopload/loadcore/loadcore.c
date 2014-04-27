
#include <tamtypes.h>

#include "ksysmem.h"
#include "kloadcore.h"
#include "iopdebug.h"
#include "iopelf.h"
#include "romdir.h"
#include "irx.h"		// ps2sdk file for IMPORT_MAGIC

#define BOOTMODE_START	*((volatile u32**)0x3F0)
#define BOOTMODE_END	*((volatile u32**)0x3F4)

void _start(BOOT_PARAMS *init); // has to be declared first!

struct tag_LC_internals {
	struct export* let_next, *let_prev;
	struct export* mda_next, *mda_prev;//.prev==free
	imageInfo *image_info; // 0x800?0x830?
	int		module_count;
	int		module_index;
} lc_internals;

u32 free;
u32 sysmem_00;
u32 modules_count;
u32	module_index;
u32 *place;	// addr of 8 * 31 = 8 * linesInIopbtconf	//place[2][31]
u32	bootmodes[17]; // was 16?
u32 bootmodes_size;
int debug=0;

u32 bm_end;

#define _dprintf(fmt, args...) \
	if (debug > 0) __printf("loadcore:%d: " fmt, __LINE__, ## args)

void retonly();
struct tag_LC_internals* GetLibraryEntryTable();
void FlushIcache();
void FlushDcache();
int RegisterLibraryEntries(struct export *es);
int  ReleaseLibraryEntries(struct export *e);
int  _LinkImports(u32 *addr, int size);
int  _UnlinkImports(void *addr, int size);
int  RegisterNonAutoLinkEntries(struct export *e);
int  QueryLibraryEntryTable(struct export *e);
u32 *QueryBootMode(int id);
void RegisterBootMode(struct bootmode *b);
int  SetNonAutoLinkFlag(struct export *e);
int  UnsetNonAutoLinkFlag(struct export *e);
void _LinkModule(imageInfo *ii);
void _UnlinkModule(imageInfo *ii);
int  _RegisterBootupCBFunc(int (*function)(int *, int), int priority, int *result);
void _SetCacheCtrl(u32 val);
int  _ReadModuleHeader(void *image, fileInfo *result);
int  _LoadModule(void *image, fileInfo *fi);
u32  _FindImageInfo(void *addr);

struct export loadcore_stub __attribute__((section(".text"))) ={
	EXPORT_MAGIC,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"loadcore",
	(func)_start,	// entrypoint
	(func)retonly,
	(func)retonly,
	(func)GetLibraryEntryTable,
	(func)FlushIcache,
	(func)FlushDcache,
	(func)RegisterLibraryEntries,
	(func)ReleaseLibraryEntries,
	(func)_LinkImports,
	(func)_UnlinkImports,
	(func)RegisterNonAutoLinkEntries,
	(func)QueryLibraryEntryTable,
	(func)QueryBootMode,
	(func)RegisterBootMode,
	(func)SetNonAutoLinkFlag,
	(func)UnsetNonAutoLinkFlag,
	(func)_LinkModule,
	(func)_UnlinkModule,
	(func)retonly,
	(func)retonly,
	(func)_RegisterBootupCBFunc,
	(func)_SetCacheCtrl,
	(func)_ReadModuleHeader,
	(func)_LoadModule,
	(func)_FindImageInfo,
	0
};

///////////////////////////////////////////////////////////////////////
void retonly(){}

///////////////////////////////////////////////////////////////////////
void RegisterBootMode(struct bootmode *b) {
	int i;

	_dprintf("%s\n", __FUNCTION__);
	if (((b->len + 1) * 4) < (16-bootmodes_size)) {
		u32 *p = &bootmodes[bootmodes_size];
		for (i=0; i<b->len + 1; i++) p[i]=((u32*)b)[i];
		p[i]=0;
		bootmodes_size+= b->len + 1;
	}
}

///////////////////////////////////////////////////////////////////////
u32 *QueryBootMode(int id) {
	u32 *b;

	for (b = (u32*)&bootmodes[0]; *b; b += ((struct bootmode*)b)->len + 1)
		if (id == ((struct bootmode*)b)->id)
			return b;
	return NULL;
}

///////////////////////////////////////////////////////////////////////
int  match_name(struct export *src, struct export *dst){
	return (*(int*)(src->name+0) == *(int*)(dst->name+0)) &&
	       (*(int*)(src->name+4) == *(int*)(dst->name+4));
}

///////////////////////////////////////////////////////////////////////
int  match_version_major(struct export *src, struct export *dst){
	return ((src->version>>8) - (dst->version>>8));
}

///////////////////////////////////////////////////////////////////////
int  match_version_minor(struct export *src, struct export *dst){
	return ((unsigned char)src->version - (unsigned char)dst->version);
}

///////////////////////////////////////////////////////////////////////
int  fix_imports(struct import *imp, struct export *exp){
    func	*ef;
    struct func_stub *fs;
    int  count=0, ordinal;

	for (ef=exp->func; *ef; ef++){
		count++;		//count number of exported functions
	}
	_dprintf("%s (%d functions)\n", __FUNCTION__, count);

	for (fs=imp->func; fs->jr_ra; fs++) {
		if ((fs->addiu0 >> 26) != INS_ADDIU)	break;

		ordinal = fs->addiu0 & 0xFFFF;
		if (ordinal < count) {
//			_dprintf("%s linking ordinal %d to %x\n", __FUNCTION__, ordinal, exp->func[ordinal]);
			fs->jr_ra=(((u32)exp->func[ordinal]>>2) & 0x3FFFFFF) | INS_J;
		} else {
			fs->jr_ra=INS_JR_RA;
		}
	}

	imp->flags |=FLAG_IMPORT_QUEUED;
	return 0;
}

///////////////////////////////////////////////////////////////////////
// Check the structure of the import table.
// Return 0 if a bad or empty import table is detected.
// Return a non-zero value if a valid import table is detected.
//
int check_import_table(struct import* imp){
    struct func_stub *f;

	if (imp->magic != IMPORT_MAGIC)
		return 0;
	for (f=imp->func; f->jr_ra; f++){
		if (f->addiu0 >> 26 != INS_ADDIU)
			return 0;
		if ((f->jr_ra!=INS_JR_RA) && (f->jr_ra>26!=INS_JR))
			return 0;
	}
	if (f->addiu0)
		return 0;
	return (imp->func < f);
}

///////////////////////////////////////////////////////////////////////[OK]

// Return 0 if successful
int  link_client(struct import *imp){
	struct export *e;

//	_dprintf("%s\n", __FUNCTION__);
	for (e=lc_internals.let_next; e; e=(struct export*)e->magic_link) {
		if (debug > 0){
			// Zero terminate the name before printing it
			char ename[9], iname[9];
			*(int*)ename = *(int*)e->name;   *(int*)(ename+4) = *(int*)(e->name+4);  ename[8] = 0;
			*(int*)iname = *(int*)imp->name; *(int*)(iname+4) = *(int*)(imp->name+4);iname[8] = 0;

			//__printf("loadcore: %s: %s, %s\n", __FUNCTION__, ename, iname);
		}

		if (!(e->flags & FLAG_NO_AUTO_LINK)){
            if ( match_name(e, (struct export*)imp)){
		        if (match_version_major(e, (struct export*)imp)==0) {
					fix_imports(imp, e);
					imp->next=(struct import*)e->next;
					e->next=(struct export*)imp;
					FlushIcache();
					return 0;
				} else {
//					_dprintf("%s: version does not match\n", __FUNCTION__);
				}
			} else {
//				_dprintf("%s: name does not match\n", __FUNCTION__);
			}
		} else {
//			_dprintf("%s: e->flags bit 0 is 0\n", __FUNCTION__);
		}
	}
	_dprintf("%s: FAILED to find a match\n", __FUNCTION__);
	return -1;
}

///////////////////////////////////////////////////////////////////////[OK]
int  _LinkImports(u32 *addr, int size)
{
	struct import *p;
	int i;

	_dprintf("%s: %x, %d\n", __FUNCTION__, addr, size);
	for (i=0; i<size/4; i++, addr++) {
		p = (struct import *)addr;
		if ((p->magic == IMPORT_MAGIC) &&
			check_import_table(p) &&
		    ((p->flags & 7) == 0) &&
		    link_client(p)) {
			_UnlinkImports(p, size);
			return -1;
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////[OK]
int  unlink_client(struct import *i1, struct import *i2){
	struct import *i, *tmp;

	if (i1->next == i2){
		i1->next=i2->next;
		return 0;
	}
	for (i = i1->next; i->next;) {
		tmp=i->next;
		if (tmp==i2) {
			i->next=tmp->next;
			tmp->next=NULL;
			return 0;
		}
		i=tmp;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////[OK]
void restore_imports(struct import* imp){
    struct func_stub *f;

	for (f=imp->func; (f->jr_ra) && ((f->addiu0 >> 26) == INS_ADDIU); f++)
		f->jr_ra=INS_JR_RA;
}

///////////////////////////////////////////////////////////////////////[OK]
int  _UnlinkImports(void *addr, int size)
{
    struct export *e;
    struct export *i;
    struct export *tmp;
    void 		*limit = addr + (size & ~0x3);

	for (e = (struct export*)lc_internals.let_next; e; e=(struct export*)e->magic_link){
		for (i = e->next; i; i=i->next) {
			if (((u32)i >= (u32)addr) && ((u32)i < (u32)limit)) {
				if (unlink_client((struct import*)e, (struct import*)i))
					return -1;
				i->flags &= ~0x7;
				restore_imports((struct import*)i);
			}
		}
		if (((u32)e >= (u32)addr) && ((u32)e < (u32)limit))
			ReleaseLibraryEntries(e);
	}
/*
	for (i=let.mda; i->next; )
		if ((i->next >= addr) && (i->next < limit)){
			i->next->flags &= ~0x7;
			restore_imports(i->next);
			tmp = i->next->next;
			i->next->next=NULL;
			i->next=tmp;
		}else
			i=i->next;
*/
	return 0;
}

///////////////////////////////////////////////////////////////////////
int  SetNonAutoLinkFlag(struct export *e)
{
	return (e->flags |= FLAG_NO_AUTO_LINK);
}

///////////////////////////////////////////////////////////////////////
int  UnsetNonAutoLinkFlag(struct export *e)
{
	return (e->flags &= ~FLAG_NO_AUTO_LINK);
}

///////////////////////////////////////////////////////////////////////
int  _RegisterBootupCBFunc(int (*function)(int *, int), int priority, int *result)
{
    int x;
    register int r;
	if (place==NULL){
		x=1;
		r=function(&x, 0);
		if (result)	*result=r;
		return 0;
	}

    __asm__("move %0, $gp\n" : "=r"(x) : );

	place[0]=(u32)function + (priority & 3);
	place[1]=x;
	place[2]=0;
	place+=2;
	return 1;
}

///////////////////////////////////////////////////////////////////////
void _LinkModule(imageInfo *ii)
{
    imageInfo* p;
	for (p=lc_internals.image_info; p->next && (p->next < (u32)ii); p=(imageInfo*)p->next);

	ii->next=p->next;
	p->next=(u32)ii;

	ii->modid=module_index++;
	modules_count++;
}

///////////////////////////////////////////////////////////////////////
void _UnlinkModule(imageInfo *ii)
{
     imageInfo *p;
	if (ii)
		for (p=lc_internals.image_info; p->next; p=(imageInfo*)p->next)
			if (p->next == (u32)ii){
				p->next=((imageInfo*)p->next)->next;
				modules_count--;
				return;
			}
}

u32  _FindImageInfo(void *addr)
{
	register imageInfo *ii;
	for (ii=lc_internals.image_info; ii; ii=(imageInfo*)ii->next)
		if(((u32)addr>=ii->p1_vaddr) &&
		   ((u32)addr <ii->p1_vaddr + ii->text_size + ii->data_size + ii->bss_size))
			return (u32)ii;
	return 0;
}

///////////////////////////////////////////////////////////////////////
int RegisterLibraryEntries(struct export *es){
    struct export *p;
	struct export *plast;
	struct export *pnext;
	struct export *tmp;
	struct export *snext;

	if ((es == NULL) || (es->magic_link != EXPORT_MAGIC))
		return -1;

	if (debug > 0){
		// Zero terminate the name before printing it
		char ename[9];
		*(int*)ename = *(int*)es->name;
		*(int*)(ename+4) = *(int*)(es->name+4);
		ename[8] = 0;

		__printf("loadcore: %s (%x): %s, %x\n", __FUNCTION__, es, es->name, es->version);
	}


	plast=NULL;
	for (p=(struct export*)lc_internals.let_next; p; p=(struct export*)p->magic_link) {
		if (match_name(es, p) == 0 ||
			match_version_major(es, p) == 0) continue;

		if (match_version_minor(es, p) == 0)
			return -1;
		_dprintf("%s: found match\n", __FUNCTION__);
		pnext = p->next;
		p->next = NULL;

		for (tmp = pnext; tmp; ) {
			if (tmp->flags & FLAG_NO_AUTO_LINK) {
				pnext->magic_link = (u32)tmp;
				pnext = tmp->next;
				tmp->next = NULL;
			} else {
				tmp->next=plast;
				plast=tmp;
			}
			tmp=tmp->next;
		}
	}

    if( lc_internals.mda_next ) {
        for (tmp = lc_internals.mda_next; tmp->next; tmp=tmp->next) { //free
            if ((match_name(es, tmp->next)) &&
                (match_version_major(es, tmp->next)==0)){
                _dprintf("%s: freeing module\n", __FUNCTION__);
                snext=tmp->next->next;
                tmp->next->next=plast;
                plast=tmp->next;
                tmp->next=snext;
            } else tmp=tmp->next;
        }
    }

    es->next=0;
	while (plast) {
		snext=plast->next;
		fix_imports((struct import*)plast, es);
		plast->next=es->next;
		es->next=plast;
		plast=snext;
	}
	es->flags &= ~FLAG_NO_AUTO_LINK;
	es->magic_link=(u32)lc_internals.let_next;
    lc_internals.let_next=es;
	FlushIcache();

	return 0;
}

///////////////////////////////////////////////////////////////////////
int  ReleaseLibraryEntries(struct export *e)
{
	register struct export *n, *p, *next, *prev;

	p = lc_internals.let_next;
	while ((p) && (p!=e)){
		prev=p;
		p=(struct export*)prev->magic_link;
	}
	if (p != e)	// if (0 != e)
		return -1;		//japanese BUG for e==p==0

	n               =e->next;
	e   ->next      =0;

	prev->magic_link=e->magic_link;
	e   ->magic_link=0x41C00000;

	while(n) {
		next = n->next;
        if (link_client((struct import*)n)){
			restore_imports((struct import*)n);
			n->flags=(n->flags & ~2) | 4;
			n->next = lc_internals.mda_prev;
			lc_internals.mda_prev=n;
		}
		n=next;
	}

    return 0;
}

///////////////////////////////////////////////////////////////////////
int  RegisterNonAutoLinkEntries(struct export *e)
{
	if ((e == NULL) || (e->magic_link != EXPORT_MAGIC)){
		return -1;
	}
	e->flags |= FLAG_NO_AUTO_LINK;
	e->magic_link = (u32)lc_internals.let_next;	// --add as first
	lc_internals.let_next = e;				// /
	FlushIcache();

	return 0;
}

///////////////////////////////////////////////////////////////////////
int  QueryLibraryEntryTable(struct export *e)
{
	struct export *p = lc_internals.let_next;
	while (p){
		if ((match_name(p, e)) && (match_version_major(p, e)==0)){
			return (int)p->func;
		}
		p=(struct export*)p->magic_link;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////
struct tag_LC_internals* GetLibraryEntryTable(){
	return &lc_internals;
}

///////////////////////////////////////////////////////////////////////
void _FlushIcache() {
	u32 status;
	u32 s1450;
	u32 s1578;
	u32 icache;
	u32 *p;

	__asm__ ("mfc0 %0, $12\n" : "=r"(status) : );

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	s1450 = *(int*)0xBF801450;
	*(int*)0xBF801450&= ~1;
	*(int*)0xBF801450;

	s1578 = *(int*)0xBF801578;
	*(int*)0xBF801578 = 0;
	*(int*)0xBF801578;

	icache = *(int*)0xFFFE0130;
	*(int*)0xFFFE0130 = 0xC04;
	*(int*)0xFFFE0130;

	__asm__ ("mtc0 %0, $12\n" :: "r"(0x10000));

	for (p=0; p<(u32*)0x400; p+=4)	// 4KB instruction cache
		*p=0;

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	*(int*)0xFFFE0130 = icache;
	*(int*)0xFFFE0130;
	*(int*)0xBF801578 = s1578;
	*(int*)0xBF801578;
	*(int*)0xBF801450 = s1450;
	*(int*)0xBF801450;

	__asm__ ("mtc0 %0, $12\n" : : "r"(status) );
}

void FlushIcache() {
	__asm__ (
		"la   $26, %0\n"
		"lui  $27, 0xA000\n"
		"or   $26, $27\n"
		"jr   $26\n"
		"nop\n"
		: : "i"(_FlushIcache)
	);
}

///////////////////////////////////////////////////////////////////////
void _FlushDcache() {
	u32 status;
	u32 s1450;
	u32 s1578;
	u32 icache;
	u32 *p;

	__asm__ ("mfc0 %0, $12\n" : "=r"(status) : );

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	s1450 = *(int*)0xBF801450;
	*(int*)0xBF801450&= ~1;
	*(int*)0xBF801450;

	s1578 = *(int*)0xBF801578;
	*(int*)0xBF801578 = 0;
	*(int*)0xBF801578;

	icache = *(int*)0xFFFE0130;
	*(int*)0xFFFE0130 = 0xC4;
	*(int*)0xFFFE0130;

	__asm__ ("mtc0 %0, $12\n" :: "r"(0x10000));

	for (p=0; p<(u32*)0x100; p+=4)	// 1KB data cache
		*p=0;

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	*(int*)0xFFFE0130 = icache;
	*(int*)0xFFFE0130;
	*(int*)0xBF801578 = s1578;
	*(int*)0xBF801578;
	*(int*)0xBF801450 = s1450;
	*(int*)0xBF801450;

	__asm__ ("mtc0 %0, $12\n" : : "r"(status) );
}

void FlushDcache(){
	__asm__ (
		"la   $26, %0\n"
		"lui  $27, 0xA000\n"
		"or   $26, $27\n"
		"jr   $26\n"
		"nop\n"
		: : "i"(_FlushDcache)
	);
}

///////////////////////////////////////////////////////////////////////
void _SetIcache(u32 val) {
	u32 status;

	__asm__ ("mfc0 %0, $12\n" : "=r"(status) : );

	__asm__ ("mtc0 %0, $12\n" :: "r"(0));

	*(int*)0xFFFE0130 = val;
	*(int*)0xFFFE0130;

	__asm__ ("mtc0 %0, $12\n" : : "r"(status) );
}

void _SetCacheCtrl(u32 val)
{
	__asm__ (
		"la   $26, %0\n"
		"lui  $27, 0xA000\n"
		"or   $26, $27\n"
		"jr   $26\n"
		"nop\n"
		: : "i"(_SetIcache)
	);
}

///////////////////////////////////////////////////////////////////////
int  _ReadModuleHeader(void *image, fileInfo *result)
{
	COFF_HEADER *coffhdr = image;
	COFF_scnhdr *section = (COFF_scnhdr*)((char*)image+sizeof(COFF_HEADER));//0x4C

	if ((coffhdr->f_magic==0x162) &&		//COFF loading
	    (coffhdr->opthdr.magic == 0x107) &&
	    (coffhdr->f_nscns < 32) &&
	    ((coffhdr->f_opthdr & 0x2FFFF) == 0x20038) &&
	    (section->s_paddr == coffhdr->opthdr.text_start)){

		if (coffhdr->opthdr.vstamp == 0x7001)
			return -1;
		result->type	=1;
		result->entry	=coffhdr->opthdr.entry;
		result->gp_value	=coffhdr->opthdr.gp_value;
		result->p1_vaddr	=coffhdr->opthdr.text_start;
		result->text_size	=coffhdr->opthdr.tsize;
		result->data_size	=coffhdr->opthdr.dsize;
		result->bss_size	=coffhdr->opthdr.bsize;
		result->p1_memsz	=coffhdr->opthdr.bss_start+coffhdr->opthdr.bsize-coffhdr->opthdr.text_start;
		result->moduleinfo	=(moduleInfo*)coffhdr->opthdr.moduleinfo;
		return result->type;
	}else{
        ELF_HEADER* eh = (ELF_HEADER*)image;
		ELF_PHR* ph= (ELF_PHR*)((char*)eh +eh->e_phoff);
		//if ((eh->e_ident[EI_CLASS] != ELFCLASS32) ||
        //(eh->e_ident[EI_DATA]) != ELFDATA2LSB))	return -1;//break
        if( *(u16*)(eh->e_ident+4) != 0x101 )
            return -1;
		if (eh->e_machine != EM_MIPS)
            return -1;//break
		if (eh->e_phentsize != sizeof(ELF_PHR))
            return -1;//break
		if (eh->e_phnum != 2)
            return -1;//break
		if (ph[0].p_type != PT_SCE_IOPMOD)
            return -1;//break
		if (eh->e_type != ET_SCE_IOPRELEXEC){
			if (eh->e_type != eh->e_phnum )//ET_EXEC)
                return -1;//only
			result->type=3;
		}else
			result->type=4;
		ELF_IOPMOD* im= (ELF_IOPMOD*)((char*)image + ph[0].p_offset);
		result->entry	=im->entry;
		result->gp_value=im->gp_value;
		result->p1_vaddr=ph[1].p_vaddr;
		result->text_size=im->text_size;
		result->data_size=im->data_size;
		result->bss_size=im->bss_size;
		result->p1_memsz=ph[1].p_memsz;
		result->moduleinfo=( moduleInfo*)im->moduleinfo;
		return result->type;
	}
	return result->type=-1;
}

#define MODULE_TYPE_COFF	1
#define MODULE_TYPE_2		2
#define MODULE_TYPE_EXEC	3
#define MODULE_TYPE_IOPRELEXEC	4

///////////////////////////////////////////////////////////////////////
void setImageInfo(fileInfo *fi, imageInfo *ii)
{
	ii->next	=0;
	ii->name	=NULL;
	ii->version	=0;
	ii->flags	=0;
	ii->modid	=0;
	if ((int)fi->moduleinfo != -1){
		ii->name	=fi->moduleinfo->name;
		ii->version	=fi->moduleinfo->version;
	}
	ii->entry	=fi->entry;
	ii->gp_value	=fi->gp_value;
	ii->p1_vaddr	=fi->p1_vaddr;
	ii->text_size	=fi->text_size;
	ii->data_size	=fi->data_size;
	ii->bss_size	=fi->bss_size;
}

///////////////////////////////////////////////////////////////////////
void load_type_1(COFF_HEADER *image){
	SHDR* s0=(SHDR*)( (char*)image + *(int*)((char*)image+0x60) );
	lc_memcpy(s0, image->opthdr.text_start, image->opthdr.tsize);
	lc_memcpy((char*)s0+image->opthdr.tsize, image->opthdr.data_start, image->opthdr.dsize);
	if (image->opthdr.bss_start && image->opthdr.bsize)
		lc_zeromem((void*)image->opthdr.bss_start, image->opthdr.bsize/4);
}

///////////////////////////////////////////////////////////////////////
void load_type_3(void *image){
	ELF_PHR *ph=(ELF_PHR*)((char*)image+((ELF_HEADER*)image)->e_phoff);
	lc_memcpy(image+ph[1].p_offset, ph[1].p_vaddr, ph[1].p_filesz);
	if (ph[1].p_filesz < ph[1].p_memsz)
		lc_zeromem(ph[1].p_vaddr+ph[1].p_filesz,
			  (ph[1].p_memsz-ph[1].p_filesz)/4);
}

///////////////////////////////////////////////////////////////////////
#define R_MIPS_32 2
#define R_MIPS_26 4
#define R_MIPS_HI16 5
#define R_MIPS_LO16 6

void load_type_4(ELF_HEADER *image, fileInfo *fi)
{
    ELF_PHR *ph=(ELF_PHR*)((char*)image+image->e_phoff);
    ELF_SHR* sh = (ELF_SHR*)((char*)image+image->e_shoff);
    ELF_REL* rel;
    int i,j,scount;
    u32* b, *b2, tmp;

    //ph[0] - .iopmod, skip
    fi->entry	+= fi->p1_vaddr;
    fi->gp_value	+= fi->p1_vaddr;
    if ((int)fi->moduleinfo != -1)
        fi->moduleinfo = (moduleInfo*)((int)fi->moduleinfo + fi->p1_vaddr);
    lc_memcpy((char*)image+ph[1].p_offset, fi->p1_vaddr, ph[1].p_filesz);

	if (ph[1].p_filesz < ph[1].p_memsz) {
        lc_zeromem(ph[1].p_vaddr+ph[1].p_filesz+fi->p1_vaddr, (ph[1].p_memsz-ph[1].p_filesz));
    }

	for (i=1; i<image->e_shnum; i++) {
		if (sh[i].sh_type==SHT_REL) {
            ELF_REL* rel =(ELF_REL*)(sh[i].sh_offset + (char*)image);
            scount=sh[i].sh_size / sh[i].sh_entsize;
			for (j=0; j<scount; j++) {
				b=(u32*)(fi->p1_vaddr + rel[j].r_offset);
				switch((u8)rel[j].r_info){
				case R_MIPS_LO16:
					*b=(*b & 0xFFFF0000) | (((*b & 0x0000FFFF) + fi->p1_vaddr) & 0xFFFF);
					break;
				case R_MIPS_32:
					*b+=fi->p1_vaddr;
					break;
				case R_MIPS_26:
                    *b = (*b & 0xfc000000) | (((*b & 0x03ffffff) + (fi->p1_vaddr >> 2)) & 0x03ffffff);
					break;
				case R_MIPS_HI16:
                    b2  =(u32*)(rel[j+1].r_offset + fi->p1_vaddr);
                    ++j;

                    tmp = (*b << 16) + (int)(*(s16*)b2) + fi->p1_vaddr;
                    *b = (*b&0xffff0000) | ((((tmp>>15)+1)>>1)&0xffff);
                    *b2 = (*b2&0xffff0000) | (tmp&0xffff);
					break;
				}
			}
        }
    }
}

int  _LoadModule(void *image, fileInfo *fi)
{
    u32* ptr;
    int i;
	switch (fi->type){
	case MODULE_TYPE_COFF:		load_type_1(image);	break;
	case MODULE_TYPE_EXEC:		load_type_3(image);	break;
	case MODULE_TYPE_IOPRELEXEC:   	load_type_4(image, fi);	break;
	default:	return -1;
	}

	setImageInfo(fi, (imageInfo*)(fi->p1_vaddr-0x30));

	return 0;
}

///////////////////////////////////////////////////////////////////////[OK]
int lc_memcpy(void *src,void *dest,int len){
    char* _src = (char*)src;
    char* _dst = (char*)dest;
	for (len=len; len>0; len--)
        *_dst++=*_src++;
}

///////////////////////////////////////////////////////////////////////[OK]
int lc_zeromem(void *addr,int len){
	for (; len>0; len--)	*(char*)addr++=0;
}

int  lc_strlen(char *s)
{
    int len;
	if (s==NULL)	return 0;
	for (len=0; *s++; len++);
	return len;
}

int recursive_set_a2(int* start, int* end, int val)
{
    *start++ = val;
    if( start < end )
        return recursive_set_a2(start, end, val);
    return 0;
}

void*  lc_memcpy_overlapping(char *dst,char *src,int len){
	if (dst==NULL)	return 0;

	if (dst>=src)
		while(--len>=0)
			*(dst+len)=*(src+len);
	else
		while (len-->0)
			*dst++=*src++;

	return dst;
}

//////////////////////////////entrypoint///////////////////////////////
void _start(BOOT_PARAMS *init) {
	*(int*)0xFFFE0130 = 0x1e988;
	__asm__ (
		"addiu $26, $0, 0\n"
		"mtc0  $26, $12\n");
		//"move  $fp, %0\n"
		//"move  $sp, %0\n"
		//: : "r"((init->ramMBSize << 20) - 0x40));
	__asm__ (
		"j     loadcore_start\n"
	);
}

extern void* _ftext, *_etext, *_end;
typedef int (*IopEntryFn)(u32,void*,void*,void*);

void loadcore_start(BOOT_PARAMS *pInitParams)
{
    fileInfo fi;
	void (*entry)();
	u32 offset;
	u32 status = 0x401;
    int bm;
	int i;
    void** s0; // pointer in module list to current module?
    BOOT_PARAMS params;
    u32 s1, s2, s3, sp, a2;

	_dprintf("%s\n", __FUNCTION__);

	// Write 0x401 into the co-processor status register?
	// This enables interrupts generally, and disables (masks) them all except hardware interrupt 0?

    lc_memcpy(pInitParams,&params,sizeof(BOOT_PARAMS));
	BOOTMODE_END = BOOTMODE_START = bootmodes;

    //_dprintf("module: %x\n", params.firstModuleAddr);

	lc_internals.let_next = (struct export*)params.firstModuleAddr;
	lc_internals.let_prev = (struct export*)params.firstModuleAddr;
	lc_internals.let_next->next = 0;
	lc_internals.module_count = 2;			// SYSMEM + LOADCORE
	lc_internals.mda_prev = lc_internals.mda_next = NULL;
	lc_internals.module_index = 3;			// next available index

	for (i=0; i<17; i++){
		bootmodes[i]=0;
	}
	bootmodes_size=0;

	bm = params.bootInfo | 0x00040000;
	RegisterBootMode((struct bootmode*)&bm);

    lc_internals.image_info = (imageInfo*)((u32)lc_internals.let_prev - 0x30);
	lc_internals.image_info->modid = 1;		// SYSMEM is the first module
	lc_internals.image_info->next = (u32)&_ftext - 0x30;
	((imageInfo*)lc_internals.image_info->next)->modid = 2;	// LOADCORE is the second module

	// find & fix LOADCORE imports (to SYSMEM)
	_LinkImports((u32*)&_ftext, (u32)&_etext - (u32)&_ftext);

	RegisterLibraryEntries(&loadcore_stub);

    // reserve LOADCORE memory
	AllocSysMemory(2, (u32)((u32)&_end - (((u32)&_ftext - 0x30) >> 8 << 8)), (void*)((((u32)&_ftext - 0x30) >> 8 << 8) & 0x1FFFFFFF));

	if (params.pos)
		params.pos = (u32)AllocSysMemory(2, params.size, (void*)params.pos);

	sp=(u32)alloca(0x10);
	s0 = (void**)((sp - 0xDF0) & 0x1FFFFF00);//=0x001ff100
	recursive_set_a2((int*)s0, (void*)(sp+0x10), 0x11111111);
	if ((u32)s0 < QueryMemSize())
		AllocSysMemory(2, QueryMemSize() - (u32)s0, s0);

	if (params.udnlString){
        int v0 = lc_strlen(params.udnlString);
		int* v1 = (int*)alloca((v0 + 8 + 8) >> 3 << 3);
		lc_memcpy_overlapping((char*)&v1[6], params.udnlString, v0+1);
		params.udnlString = (char*)&v1[6];
		v1[4] = 0x01050000;
		v1[5] = (int)&v1[6];
		RegisterBootMode((struct bootmode*)&v1[4]);	// BTUPDATER bootmode 5
	}

	a2 = (params.numConfLines+1) * 4;
	s0 = alloca((a2 + 7) >> 3 << 3) + 0x10;
	lc_memcpy_overlapping((char*)s0, (char*)params.moduleAddrs, a2);	//0x30020

	s1 = 0;
	params.moduleAddrs = (u32**)s0;
	s2 = (u32)alloca(params.numConfLines << 3) + 0x10;
    place = (u32*)s2;
	*place = 0;
	i = -1;
    s3 = 1;

	_dprintf("loading modules: %d\n", params.numConfLines);
    s0 += 2; // skip first two: SYSMEM, LOADCORE
    for (; *s0; s0+=1) {
		if ((u32)*s0 & 1){
			if (((u32)*s0 & 0xF) == s3)
				s1 = (u32)*s0>>2;
		}else{
			i += 1;
			i &= 0xF;

            _dprintf("load module from %x\n", *s0);
			switch(_ReadModuleHeader(*s0, &fi)){
			case MODULE_TYPE_COFF:
			case MODULE_TYPE_EXEC:
				a2 = ((fi.p1_vaddr - 0x30) >> 8 << 8) & 0x1FFFFFFF;
				if (NULL == AllocSysMemory(2, fi.p1_memsz + fi.p1_vaddr - a2, (void*)a2))
					goto HALT;
				break;

			case MODULE_TYPE_2:
			case MODULE_TYPE_IOPRELEXEC:
				if (fi.p1_vaddr = (u32)((s1 == 0) ?
						AllocSysMemory(0, fi.p1_memsz+0x30, 0) :
                                   AllocSysMemory(2, fi.p1_memsz+0x30, (void*)s1)) )
					fi.p1_vaddr += 0x30;
				else
					goto HALT;
				break;

			default:
                _dprintf("could not find module at %x\n", *s0);
				goto HALT;
			}

			_LoadModule(*s0, &fi);
            _dprintf("loading module %s: at offset %x, memsz=%x, type=%x, entry=%x\n", fi.moduleinfo->name, fi.p1_vaddr, fi.p1_memsz, fi.type, fi.entry);

			if (0 == _LinkImports((u32*)fi.p1_vaddr, fi.text_size)) {

				FlushIcache();
				__asm__("move $gp, %0\n" : : "r"(fi.gp_value));
                // call the entry point
				s1 = ((IopEntryFn)fi.entry)(0, NULL, s0, NULL);

				if ((s1 & 3) == 0){
					_LinkModule((imageInfo*)(fi.p1_vaddr - 0x30));
					if (s1 & ~3)
						_RegisterBootupCBFunc((int (*)(int *, int))(s1 & ~3), BOOTUPCB_NORMAL, 0);
				}else{
					_UnlinkImports((void*)fi.p1_vaddr, fi.text_size);
					FreeSysMemory((void*)((fi.p1_vaddr - 0x30) >> 8 << 8));
				}
			}else {
				FreeSysMemory((void*)((fi.p1_vaddr - 0x30) >> 8 << 8));
            }

			s1 = 0;
		}
    }

	if (params.pos)
		FreeSysMemory((void*)params.pos);

	for (i=BOOTUPCB_FIRST; i<BOOTUPCB_PRIORITIES; i++){
		if (i == BOOTUPCB_LAST)
			place = 0;

		for (s0=(void**)s2; *s0; s0+=2)
			if (((u32)*s0 & 3) == i){
                __asm__("move $gp, %0\n" : : "r"(s0[1]));
				((int (*)(int *, int))((u32)*s0 & ~3))(((u32)*s0 & 3) == BOOTUPCB_LAST ? (int*)&s0[2] : (int*)s0, 1);
			}

		if (i == BOOTUPCB_NORMAL){
			while ((u32)s2 < (u32)s0){
				s0-=2;	// -8 bytes
				if (((u32)*s0 & 3) == BOOTUPCB_LAST)
					break;
				*s0 = 0;
			}
		}
	}

HALT:
	*(char*)0x80000000 = 2;
	goto HALT;
}

