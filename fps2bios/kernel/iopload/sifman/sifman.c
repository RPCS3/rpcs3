
#include "kloadcore.h"
#include "kintrman.h"
#include "kdmacman.h"
#include "ksifman.h"

#define min(a, b)	((a)<(b)?(a):(b))
#define max(a, b)	((a)>(b)?(a):(b))

#define CONFIG_1450		(*(volatile int*)0xBF801450)

#define BD0	(*(volatile int*)0xBD000000)	//mscom
#define BD1	(*(volatile int*)0xBD000010)	//smcom
#define BD2	(*(volatile int*)0xBD000020)	//msflag
#define BD3	(*(volatile int*)0xBD000030)	//smflag
#define BD4	(*(volatile int*)0xBD000040)
#define BD5	(*(volatile int*)0xBD000050)
#define BD6	(*(volatile int*)0xBD000060)
#define BD7	(*(volatile int*)0xBD000070)


int debug=1;

#define _dprintf(fmt, args...) \
	if (debug > 0) __printf("sifman: " fmt, ## args)


struct sifData{
	int	data,
		words,
		count,
		addr;
};

int	sifSIF2Init	=0,
	sifInit		=0;

struct sm_internal{
	short		id,		//+000	some id...?!?
			res1;		//+002	not used
	int		index;		//+004	current position in dma buffer
	struct sifData	*crtbuf;	//+008	address of current used buffer
	struct sifData	buf1[32];	//+00C	first buffer
	struct sifData	buf2[32];	//+20C	second buffer
	int		(*function)(int);//+40C	a function ?!?
	int		param;		//+410	a parameter for function
	int		res2;		//+414	not used
} vars;

struct sifData	one;
int		res3, res4;

int _start(int argc, char* argv);
void RegisterSif0Handler();


///////////////////////////////////////////////////////////////////////
int getBD2_loopchanging(){
    register int a, b;
    a=BD2;
	b=BD2;
	while (a!=b) {
		a=b;
		b=BD2;
	}
	return a;
}

///////////////////////////////////////////////////////////////////////
int getBD3_loopchanging(){
    register int a, b;
    a=BD3;
	b=BD3;
	while (a!=b) {
		a=b;
		b=BD3;
	}
	return a;
}

///////////////////////////////////////////////////////////////////////[04]
void SifSIF2Init(){
	if (sifSIF2Init!=0) return;

	DMAch_SIF2_CHCR	= 0;	//reset ch. 2
	DMAch_DPCR|= 0x800;		//enable dma ch. 2
	DMAch_DPCR;
	sifSIF2Init	=1;
}

///////////////////////////////////////////////////////////////////////[05]
void SifInit(){
    u32 x, y;
	u32 msflag;

	_dprintf("%s: sifInit=%d\n", __FUNCTION__, sifInit);
	if (sifInit!=0) return;

	DMAch_DPCR2	|=0x8800;	//enable dma ch. 9 and 10
	DMAch_SIF9_CHCR	= 0;
	DMAch_SIFA_CHCR	= 0;
	SifSIF2Init();

	if (CONFIG_1450 & 0x10)
		CONFIG_1450 |= 0x10;
	CONFIG_1450 |= 0x1;

	RegisterSif0Handler();

	CpuSuspendIntr(&x);				//intrman
	CpuEnableIntr();				//intrman

	_dprintf("%s: waiting EE...\n", __FUNCTION__);
	do {		//EE kernel sif ready
		CpuSuspendIntr(&y);			//intrman
		msflag = getBD2_loopchanging();
		CpuResumeIntr(y);			//intrman
	} while(!(msflag & 0x10000));

	_dprintf("%s: EE ready\n", __FUNCTION__);
	CpuResumeIntr(x);				//intrman
	SifSetDChain();
	SifSetIOPrcvaddr(0);	//sif1 receive buffer
	BD3 = 0x10000;			//IOPEE_sifman_init
	BD3;
	sifInit		=1;
	_dprintf("%s ok\n", __FUNCTION__);
}

///////////////////////////////////////////////////////////////////////[02]
void SifDeinit(){
	int x;

	DisableIntr(INT_DMA9, &x);
	ReleaseIntrHandler(INT_DMA9);
	DMAch_SIF9_CHCR=0;
	DMAch_SIFA_CHCR=0;
	if (CONFIG_1450 & 0x10)
		CONFIG_1450 |= 0x10;
}

///////////////////////////////////////////////////////////////////////[29]
int SifCheckInit(){
	return sifInit;
}

///////////////////////////////////////////////////////////////////////[06]
void SifSetDChain(){
	if ((BD4 & 0x40) == 0) BD4=0x40;
	DMAch_SIFA_CHCR		=0;
	DMAch_SIFA_BCR_size	=32;
	DMAch_SIFA_CHCR		=DMAf_08|DMAf_CO|DMAf_TR|DMAf_30;//EE->IOP (to memory)
}

///////////////////////////////////////////////////////////////////////[30]
void SifSet0CB(int (*_function)(int), int _param){
	vars.function=_function;
	vars.param=_param;
}

///////////////////////////////////////////////////////////////////////[31]
void SifReset0CB(){
	vars.function=NULL;
	vars.param=0;
}

///////////////////////////////////////////////////////////////////////
int Sif0Handler(void *common) {
	struct sm_internal *vars = (struct sm_internal *)common;
    int var_10;
	if (vars->function)
		vars->function(vars->param);

	if ((DMAch_SIF9_CHCR & DMAf_TR == 0) && (vars->index>0)) {
		DMAch_SIF9_CHCR	=0;
		DMAch_SIF9_TADR =(int)vars->crtbuf;
		DMAch_SIF9_BCR	=32;
		if (BD4 & 0x20 == 0)	BD4 = 0x20;
		vars->id++;
		if (vars->crtbuf == vars->buf1){
			vars->index=0;
			vars->crtbuf=vars->buf2;
		}else
			vars->crtbuf=vars->buf1;
		DMAch_SIF9_CHCR = DMAf_DR|DMAf_08|DMAf_CO|DMAf_LI|DMAf_TR;//IOP->EE (from memory)
		var_10=DMAch_SIF9_CHCR;
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////
void RegisterSif0Handler(){
    u32 x;
	vars.index=0;
	vars.crtbuf=vars.buf1;
	vars.function=NULL;
	vars.param=0;
	CpuSuspendIntr(&x);					//intrman
	RegisterIntrHandler(INT_DMA9, 1, Sif0Handler, &vars);	//intrman
	EnableIntr(INT_DMA9);					//intrman
	CpuResumeIntr(x);					//intrman
}

///////////////////////////////////////////////////////////////////////
void enqueue(struct sifman_DMA *psd){
	vars.crtbuf[vars.index].data=(u32)psd->data & 0xFFFFFF;	//16MB addressability
	if (psd->attr & SIF_DMA_INT_I)
		vars.crtbuf[vars.index].data |= 0x40000000;
	vars.crtbuf[vars.index].words=(psd->size + 3)/4 & 0xFFFFFF;
	vars.crtbuf[vars.index].count = ((psd->size + 3)/16 + (((psd->size + 3)/4) & 3))
			| 0x10000000;
	if (psd->attr & SIF_DMA_INT_O)
		vars.crtbuf[vars.index].count |=0x80000000;
	vars.crtbuf[vars.index].addr = (u32)psd->addr & 0x1FFFFFFF;	//512MB addresability
	vars.index++;
}

///////////////////////////////////////////////////////////////////////[07]
u32 SifSetDma(struct sifman_DMA *psd, int len){
    int var_20;
    register unsigned int ret;
	int i;

	if (32-vars.index < len)	return 0;	//no place

	ret = vars.id;
	ret = (ret << 16) | ((vars.index & 0xFF) << 8) | (len & 0xFF);

	if (vars.index)
	vars.crtbuf[vars.index-1].data &= 0x7FFFFFFF;
	for (i=0; i<len; i++)
		enqueue(&psd[i]);
	vars.crtbuf[vars.index-1].data |= 0x80000000;

	if ((DMAch_SIF9_CHCR & DMAf_TR) == 0) {
		DMAch_SIF9_CHCR=0;
		DMAch_SIF9_TADR=(int)vars.crtbuf;

		if ((BD4 & 0x20) == 0)	BD4=0x20;

		DMAch_SIF9_BCR_size=32;

		vars.index=0;
		vars.id++;
		if (vars.crtbuf==vars.buf1)vars.crtbuf=vars.buf2;
			//stupid :D (why the if?)

		DMAch_SIF9_CHCR=DMAf_DR|DMAf_08|DMAf_CO|DMAf_LI|DMAf_TR;//IOP->EE
		var_20=DMAch_SIF9_CHCR;	//?!?
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////[08]
int SifDmaStat(u32 id){
	if (DMAch_SIF9_CHCR & DMAf_TR){
		if (id>>16   == vars.id)	return 1;  //waiting in queue
		if (id>>16+1 == vars.id)	return 0;  //running
	}
	return -1;	//terminated
}

///////////////////////////////////////////////////////////////////////[09]
void SifSend(struct sifman_DMA sd){
	sd.size = sd.size /4 + ((sd.size & 3) > 0);
	one.data = ((u32)sd.data & 0xFFFFFF) | 0x80000000;
	one.words= sd.size & 0xFFFFFF;
	if (sd.attr & SIF_DMA_INT_I)	     one.data |= 0x40000000;
	one.count = (sd.size/4 + ((sd.size & 3)>0)) |  0x10000000;
	if (sd.attr & SIF_DMA_INT_O)	     one.count|= 0x80000000;
	one.addr = (u32)sd.addr & 0xFFFFFFF;

	if (BD4 & 0x20 == 0)	BD4=0x20;

	DMAch_SIF9_CHCR=0;
	DMAch_SIF9_TADR=(u32)&one;
	DMAch_SIF9_BCR_size=32;
	DMAch_SIF9_CHCR=DMAf_DR|DMAf_08|DMAf_CO|DMAf_LI|DMAf_TR; //IOP->EE
	DMAch_SIF9_CHCR;
}

///////////////////////////////////////////////////////////////////////[10]
void SifSendSync(){
	while (DMAch_SIF9_CHCR & DMAf_TR);
}

///////////////////////////////////////////////////////////////////////[11]
int  SifIsSending(){
	return (DMAch_SIF9_CHCR & DMAf_TR);
}

///////////////////////////////////////////////////////////////////////[12]
void SifSetSIF0DMA(void *data, int size, int attr){
	size=size/4 + ((size & 3)>0);

	if (BD4 & 0x20 == 0)	BD4=0x20;

	DMAch_SIF9_CHCR=0;
	DMAch_SIF9_MADR=(u32)data & 0xFFFFFF;
	DMAch_SIF9_BCR_size=32;
	DMAch_SIF9_BCR_count=size/32 + ((size & 0x1F)>0);
	DMAch_SIF9_CHCR=DMAf_DR|DMAf_CO|DMAf_TR;
	DMAch_SIF9_CHCR;
}

///////////////////////////////////////////////////////////////////////[13]
void SifSendSync0(){
	while (DMAch_SIF9_CHCR & DMAf_TR);
}

///////////////////////////////////////////////////////////////////////[14]
int  SifIsSending0(){
	return (DMAch_SIF9_CHCR & DMAf_TR);
}

///////////////////////////////////////////////////////////////////////[15]
void SifSetSIF1DMA(void *data, int size, int attr){
	size=size/4 + ((size & 3)>0);

	if (BD4 & 0x40 == 0)	BD4=0x40;

	DMAch_SIFA_CHCR=0;
	DMAch_SIFA_MADR=(u32)data & 0xFFFFFF;
	DMAch_SIFA_BCR_size=32;
	DMAch_SIFA_BCR_count=size/32 + ((size & 0x1F)>0);
	DMAch_SIFA_CHCR=DMAf_CO|DMAf_TR|
	    (attr & SIF_DMA_BSN?DMAf_30:0);
	DMAch_SIFA_CHCR;
}

///////////////////////////////////////////////////////////////////////[16]
void SifSendSync1(){
	while (DMAch_SIFA_CHCR & DMAf_TR);
}

///////////////////////////////////////////////////////////////////////[17]
int  SifIsSending1(){
	return (DMAch_SIFA_CHCR & DMAf_TR);
}

///////////////////////////////////////////////////////////////////////[18]
void SifSetSIF2DMA(void *data, int size, int attr){
	size=size/4 + ((size & 3)>0);

	if (BD4 & 0x80 == 0)	BD4=0x80;

	DMAch_SIF2_CHCR=0;
	DMAch_SIF2_MADR=(u32)data & 0xFFFFFF;
	DMAch_SIF2_BCR_size=min(size, 32);
	DMAch_SIF2_BCR_count=size/32 + ((size & 0x1F)>0);
	DMAch_SIF2_CHCR=DMAf_CO |DMAf_TR|
	    ((attr & SIF_TO_EE  ?DMAf_DR:
	     (attr & SIF_DMA_BSN?DMAf_30:0)));
	DMAch_SIF2_CHCR;
}

///////////////////////////////////////////////////////////////////////[19]
void SifSendSync2(){
	while (DMAch_SIF2_CHCR & DMAf_TR);
}

///////////////////////////////////////////////////////////////////////[20]
int  SifIsSending2(){
	return (DMAch_SIF2_CHCR & DMAf_TR);
}

///////////////////////////////////////////////////////////////////////[21]
int  SifGetEEIOPflags(){
	return getBD2_loopchanging();
}

///////////////////////////////////////////////////////////////////////[22]
int  SifSetEEIOPflags(int val){
	BD2=val;
	return getBD2_loopchanging();
}

///////////////////////////////////////////////////////////////////////[23]
int  SifGetIOPEEflags(){
	return getBD3_loopchanging();
}

///////////////////////////////////////////////////////////////////////[24]
int  SifSetIOPEEflags(int val){
	BD3=val;
	return getBD3_loopchanging();
}

///////////////////////////////////////////////////////////////////////[25]
int  SifGetEErcvaddr(){
	return BD0;
}

///////////////////////////////////////////////////////////////////////[26]
int  SifGetIOPrcvaddr(){
	return BD1;
}

///////////////////////////////////////////////////////////////////////[27]
int  SifSetIOPrcvaddr(int val){
	BD1=val;
	return BD1;
}

///////////////////////////////////////////////////////////////////////[28]
void SifSet1450_2(){
	CONFIG_1450 |= 2;
	CONFIG_1450 &= ~2;
	CONFIG_1450;
}

///////////////////////////////////////////////////////////////////////[01,03,32,33,34,35]
void _retonly(){}

//////////////////////////////entrypoint///////////////////////////////
struct export sifman_stub={
	0x41C00000,
	0,
	VER(1, 1),			// 1.1 => 0x101
	0,
	"sifman",
	(func)_start,			// entrypoint
	(func)_retonly,
	(func)SifDeinit,
	(func)_retonly,
	(func)SifSIF2Init,
	(func)SifInit,
	(func)SifSetDChain,
	(func)SifSetDma,
	(func)SifDmaStat,
	(func)SifSend,
	(func)SifSendSync,
	(func)SifIsSending,
	(func)SifSetSIF0DMA,
	(func)SifSendSync0,
	(func)SifIsSending0,
	(func)SifSetSIF1DMA,
	(func)SifSendSync1,
	(func)SifIsSending1,
	(func)SifSetSIF2DMA,
	(func)SifSendSync2,
	(func)SifIsSending2,
	(func)SifGetEEIOPflags,
	(func)SifSetEEIOPflags,
	(func)SifGetIOPEEflags,
	(func)SifSetIOPEEflags,
	(func)SifGetEErcvaddr,
	(func)SifGetIOPrcvaddr,
	(func)SifSetIOPrcvaddr,
	(func)SifSet1450_2,
	(func)SifCheckInit,
	(func)SifSet0CB,
	(func)SifReset0CB,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	0
};

//////////////////////////////entrypoint///////////////////////////////[00]
int _start(int argc, char* argv){

	return RegisterLibraryEntries(&sifman_stub) > 0;

}

