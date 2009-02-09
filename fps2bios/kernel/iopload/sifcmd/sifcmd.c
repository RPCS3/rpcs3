//[module]	SIFCMD
//[processor]	IOP
//[type]	ELF-IRX
//[name]	IOP_SIF_rpc_interface
//[version]	0x101
//[memory map]	
//[handlers]	
//[entry point]	sifcmd_start, sifcmd_stub
//[made by]	[RO]man (roman_ps2dev@hotmail.com)

#include "kloadcore.h"
#include "kintrman.h"
#include "ksifman.h"
#include "kthbase.h"
#include "ksifcmd.h"
#include "ksifman.h"

#define _dprintf(fmt, args...) \
	__printf("sifcmd: " fmt, ## args)

int sifInitRpc=0;

// CMD data
char sif1_rcvBuffer[8*16];			//8 qwords
char b[4*16];					//not used
struct tag_cmd_common {
	char		*sif1_rcvBuffer,	//+00
			*b,			//+04
			*saddr;			//+08
	SifCmdData	*sysCmdBuffer;		//+0C
	int		sysCmdBufferSize;	//+10
	SifCmdData	*cmdBuffer;		//+14
	int		cmdBufferSize,		//+18
			*Sreg,			//+1C
			systemStatusFlag;	//+20
	void		(*func)(int);		//+24
	int		param,			//+28
			_pad;			//+2C
} cmd_common;					//=30
SifCmdData	sysCmds[32];
int		Sreg[32];

// RPC data
int bufx[512], bufy[512];
struct tag_rpc_common{
	int		pid;
	RPC_PACKET	*paddr;
	int		size;
	RPC_PACKET	*paddr2;
	int		size2;
	void		*next;
	int		count;
	int		base;
	void		*queue;
	int		_pad0,
			_pad1,
			_pad2;
} rpc_common;


int _start();

///////////////////////////////////////////////////////////////////////
//////////////////////////// CMD //////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
void cmd80000001_SET_SREG(SifCmdSRData *packet, struct tag_cmd_common *common) {
	common->Sreg[packet->rno]=packet->value;
}

///////////////////////////////////////////////////////////////////////
void cmd80000000_CHANGE_SADDR(SifCmdCSData *packet, struct tag_cmd_common *common) {
	common->saddr=packet->newaddr;
}

///////////////////////////////////////////////////////////////////////[06]
int SifGetSreg(int index){
	return Sreg[index];
}

///////////////////////////////////////////////////////////////////////[07]
int SifSetSreg(int index, unsigned int value){
	return Sreg[index]=value;
}

///////////////////////////////////////////////////////////////////////
void *getCmdCommon(){
	return &cmd_common;
}

///////////////////////////////////////////////////////////////////////
void cmd80000002_INIT_CMD(SifCmdCSData *packet, struct tag_cmd_common *common){
    __printf("cmd80000002_INIT_CMD\n");
	if (packet->hdr.opt==0){
		iSetEventFlag(common->systemStatusFlag, 0x100);
		SifSetEEIOPflags(0x20000);
		common->saddr=packet->newaddr;
	}else
		iSetEventFlag(common->systemStatusFlag, 0x800);
}

///////////////////////////////////////////////////////////////////////[02]
int SifDeinitCmd(){
	int x;
	DisableIntr(INT_DMA10, &x);
	ReleaseIntrHandler(INT_DMA10);
	SifDeinit();
	return 0;
}

///////////////////////////////////////////////////////////////////////[04]
void SifInitCmd(){
    __printf("iopSifInitCmd\n");
	SifSetIOPEEflags(0x20000);
	WaitEventFlag(cmd_common.systemStatusFlag, 0x100, 0, 0);
}

///////////////////////////////////////////////////////////////////////[05]
void SifExitCmd(){
	int x;
	DisableIntr(INT_DMA10, &x);
	ReleaseIntrHandler(INT_DMA10);
}

///////////////////////////////////////////////////////////////////////[08]
SifCmdData *SifSetCmdBuffer(SifCmdData *cmdBuffer, int size){
	register SifCmdData *old;
	old=cmd_common.cmdBuffer;
	cmd_common.cmdBuffer=cmdBuffer;
	cmd_common.cmdBufferSize=size;
	return old;
}

///////////////////////////////////////////////////////////////////////[09]
SifCmdData *SifSetSysCmdBuffer(SifCmdData *sysCmdBuffer, int size){
	register SifCmdData *old;
	old=cmd_common.sysCmdBuffer;
	cmd_common.sysCmdBuffer=sysCmdBuffer;
	cmd_common.sysCmdBufferSize=size;
	return old;
}

///////////////////////////////////////////////////////////////////////[0A]
void SifAddCmdHandler(int pos, cmdh_func f, void *data){
	if (pos<0){
		cmd_common.sysCmdBuffer[pos & 0x1FFFFFFF].func=f;
		cmd_common.sysCmdBuffer[pos & 0x1FFFFFFF].data=data;
	}else{
		cmd_common.cmdBuffer   [pos & 0x1FFFFFFF].func=f;
		cmd_common.cmdBuffer   [pos & 0x1FFFFFFF].data=data;
	}
}

///////////////////////////////////////////////////////////////////////[0B]
void SifRemoveCmdHandler(unsigned int pos){
	if (pos<0)
		cmd_common.sysCmdBuffer[pos & 0x1FFFFFFF].func=0;
	else
		cmd_common.cmdBuffer   [pos & 0x1FFFFFFF].func=0;
}

///////////////////////////////////////////////////////////////////////
unsigned int sendCmd(unsigned int pos, int mode, SifCmdHdr *cp, int ps, void *src, void *dst, int size){
	u32 x;
	struct sifman_DMA dma[2];
	register int count, y;

	if (ps<16 || ps>112)	return 0;

	count=0;
	if (size>0){
		count=1;
		dma[0].addr=dst;
		dma[0].size=size;
		dma[0].attr=0;
		dma[0].data=src;
		cp->daddr=(u32)dst;
		cp->dsize=size;
	}else{
		cp->daddr=0;
		cp->dsize=0;
	}
	count++;
	cp->psize=ps;
	cp->fcode=pos;
	dma[count-1].data=cp;
	dma[count-1].attr=SIF_DMA_INT_O;	//calls SIF0 handler
	dma[count-1].size=ps;			//on EE side after transfer;)
	dma[count-1].addr=cmd_common.saddr;
	if (mode & 1)	//interrupt mode
		return SifSetDma(dma, count);
	else{
		CpuSuspendIntr(&x);
		y=SifSetDma(dma, count);
		CpuResumeIntr(x);
		return y;
	}
}

///////////////////////////////////////////////////////////////////////[0C]
unsigned int SifSendCmd(unsigned int pos, void *cp, int ps, void *src, void *dst, int size){
	return sendCmd(pos, 0, cp, ps, src, dst, size);
}

///////////////////////////////////////////////////////////////////////[0D]
unsigned int iSifSendCmd(unsigned int pos, void *cp, int ps, void *src, void *dst, int size){
	return sendCmd(pos, 1, cp, ps, src, dst, size);
}

///////////////////////////////////////////////////////////////////////[1A]
void SifSet1CB(void *func, int param){
	cmd_common.func =func;
	cmd_common.param=param;
}

///////////////////////////////////////////////////////////////////////[1B]
void SifReset1CB(){
	cmd_common.func =0;
	cmd_common.param=0;
}

///////////////////////////////////////////////////////////////////////
int SIF1_handler(void *common){
	int buf[112/4];
	register int i, ps;
	register SifCmdData *scd;
	SifCmdHdr *packet;
	struct tag_cmd_common *c = (struct tag_cmd_common *)common;

	if (c->func)
		c->func(c->param);

	packet=(SifCmdHdr*)c->sif1_rcvBuffer;

	if ((ps=packet->psize & 0xFF)==0){
		SifSetDChain();
		return 1;
	}
	packet->psize=0;

	ps=(ps+3<0 ? ps+6 : ps+3)/4;
	for (i=0; i<ps; i++)
		buf[i]=((u32*)packet)[i];

	SifSetDChain();
	packet=(SifCmdHdr*)buf;
	if (packet->fcode<0){
		if (packet->fcode & 0x7FFFFFFF>=c->sysCmdBufferSize)
			return 1;
		scd=&c->sysCmdBuffer[packet->fcode & 0x7FFFFFFF];
	}else{
		if (packet->fcode>=c->cmdBufferSize)
			return 1;
		scd=&c->cmdBuffer[packet->fcode];
	}
	if (scd->func)
		scd->func(packet, scd->data);

	return 1;
}

///////////////////////////////////////////////////////////////////////
//////////////////////////// RPC //////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
RPC_PACKET *rpc_get_packet(struct tag_rpc_common *common){
	u32 x;
	register int pid, i;
	RPC_PACKET *packet;

	CpuSuspendIntr(&x);

	for (i=0, packet=common->paddr;  i<common->size;  i++)
		if (packet[i].rec_id & 2==0)	goto found;
	for (i=0, packet=common->paddr2; i<common->size2; i++)
		if (packet[i].rec_id & 2==0)	goto found;

	CpuResumeIntr(x);
	return 0;

found:
	packet[i].rec_id |= 2;
	pid=++common->pid;
	if (pid == 1)
		common->pid++;
	packet[i].pid=pid;
	packet[i].paddr=&packet[i];
	CpuResumeIntr(x);
	return &packet[i];
}

///////////////////////////////////////////////////////////////////////
void rpc_free_packet(RPC_PACKET *packet){
	packet->pid     = 0;
	packet->rec_id &= 0xFFFFFFFD;	//~2
}

///////////////////////////////////////////////////////////////////////
RPC_PACKET *rpc_get_fpacket(struct tag_rpc_common *common){
	register int i;

	i=common->base % common->count;
	common->base=i+1;
	return (RPC_PACKET*)(((u8*)common->next)+i*64);
}

///////////////////////////////////////////////////////////////////////
void cmd80000008_END(RPC_PACKET_END *packet, struct tag_rpc_common *common){
	if (packet->command==0x8000000A){
		if (packet->client->func)
			packet->client->func(packet->client->param);
	}else
	if (packet->command==0x80000009){
		packet->client->server=packet->server;
		packet->client->buff  =packet->buff;
		packet->client->cbuff =packet->cbuff;
	}

	if (packet->client->hdr.tid>=0)
		iWakeupThread(packet->client->hdr.tid);

	rpc_free_packet(packet->client->hdr.pkt_addr);
	packet->client->hdr.pkt_addr=0;
}

///////////////////////////////////////////////////////////////////////
void cmd8000000C_RDATA(RPC_PACKET_RDATA *packet, struct tag_rpc_common *common){
	RPC_PACKET_END *epacket;

	epacket = (RPC_PACKET_END *)rpc_get_fpacket(common);

	epacket->packet.paddr = packet->packet.paddr;
	epacket->command = 0x8000000C;
	epacket->client  = packet->client;

	iSifSendCmd(0x80000008, epacket, 0x40, packet->src, packet->dst, packet->size);
}

///////////////////////////////////////////////////////////////////////[17]
int SifGetOtherData(struct sifcmd_RPC_RECEIVE_DATA *rd, void *src, void *dst, int size, int mode){
	RPC_PACKET_RDATA *packet;

	if ((packet=(RPC_PACKET_RDATA *)rpc_get_packet(&rpc_common))==0)
		return -1;

	rd->hdr.pkt_addr=packet;
	rd->hdr.rpc_id  =packet->packet.pid;
	packet->packet.paddr=packet;
	packet->client=(struct sifcmd_RPC_CLIENT_DATA*)rd;
	packet->src=src;
	packet->dst=dst;
	packet->size=size;

	if (mode & 1==0){
		rd->hdr.tid=GetThreadId();
		if (SifSendCmd(0x8000000C, packet, 0x40, 0, 0, 0)==0)
			return -2;
		SleepThread();
	}else{			//async
		rd->hdr.tid=-1;
		if (SifSendCmd(0x8000000C, packet, 0x40, 0, 0, 0)==0)
			return -2;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////
struct sifcmd_RPC_SERVER_DATA *search_svdata(u32 command, struct tag_rpc_common *common){
	struct sifcmd_RPC_SERVER_DATA *q;
	struct sifcmd_RPC_SERVER_DATA *s;

	for (q=common->queue; q; q=q->next) {
		for (s=q->link; s; s=s->link) {
			if (s->command==command)
				return s;
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////
void cmd80000009_BIND(RPC_PACKET_BIND *packet, struct tag_rpc_common *common){
	RPC_PACKET_END *epacket;
	struct sifcmd_RPC_SERVER_DATA *s;

	epacket = (RPC_PACKET_END *)rpc_get_fpacket(common);

	epacket->packet.paddr=packet->packet.paddr;
	epacket->command=0x80000009;
	epacket->client=packet->client;
	
	s = search_svdata(packet->fno, common);
	if (s == NULL){
		epacket->server=0;
		epacket->buff  =0;
		epacket->cbuff =0;
	}else{
		epacket->server=s;
		epacket->buff  =s->buff;
		epacket->cbuff =s->cbuff;
	}
	iSifSendCmd(0x80000008, epacket, 0x40, 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////[0F]
int SifBindRpc(struct sifcmd_RPC_CLIENT_DATA *client, unsigned int number, unsigned int mode) {
	RPC_PACKET_BIND *packet;

	client->command=0;
	client->server=0;

	packet = (RPC_PACKET_BIND *)rpc_get_packet(&rpc_common);
	if (packet==NULL) return -1;

	client->hdr.pkt_addr = packet;
	client->hdr.rpc_id   = packet->packet.pid;
	packet->packet.paddr = packet;
	packet->client = client;
	packet->fno    = number;
	
	if (mode & 1==0){
		client->hdr.tid=GetThreadId();
		if (SifSendCmd(0x80000009, packet, 0x40, 0, 0, 0)==0)
			return -2;
		SleepThread();
	}else{		//async
		client->hdr.tid=-1;
		if (SifSendCmd(0x80000009, packet, 0x40, 0, 0, 0)==0)
			return -2;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////
void cmd8000000A_CALL(RPC_PACKET_CALL *packet, struct tag_rpc_common *common){
	struct sifcmd_RPC_DATA_QUEUE *qd;

	qd = packet->server->base;
	if (qd->start==0)	qd->start=packet->server;
	else			qd->end->link=packet->server;
	qd->end=packet->server;
	packet->server->pkt_addr=packet->packet.packet.paddr;
	packet->server->client=packet->packet.client;
	packet->server->fno=packet->packet.fno;
	packet->server->size=packet->size;
	packet->server->receive=packet->receive;
	packet->server->rsize=packet->rsize;
	packet->server->rmode=packet->rmode;
	if ((qd->key>=0) && (qd->active==0))
		iWakeupThread(qd->key);
}

///////////////////////////////////////////////////////////////////////[10]
int SifCallRpc(struct sifcmd_RPC_CLIENT_DATA *client, unsigned int fno, unsigned int mode, void *send, int ssize, void *receive, int rsize, void (*end_func)(void*), void *end_para){
	RPC_PACKET_CALL *packet;

	if ((packet=(RPC_PACKET_CALL *)rpc_get_packet(&rpc_common))==0)
		return -1;
	client->hdr.pkt_addr=(void*)packet;
	client->func   = end_func;
	client->param  = end_para;
	client->hdr.rpc_id= packet->packet.packet.pid;

	packet->packet.packet.paddr  = packet;
	packet->packet.client = client;
	packet->packet.fno    = fno;
	packet->size   = ssize;
	packet->receive= receive;
	packet->rsize  = rsize;
	packet->server = client->server;

	if (mode & 1){
		packet->rmode=(end_func!=0);
		client->hdr.tid=-1;
		if (SifSendCmd(0x8000000A, packet, 0x40, send, client->buff, ssize))
			return 0;
		return -2;
	}else{
		packet->rmode=1;
		client->hdr.tid=GetThreadId();
		if (SifSendCmd(0x8000000A, packet, 0x40, send, client->buff, ssize)==0)
			return -2;
		SleepThread();
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////[12]
int SifCheckStatRpc(struct sifcmd_RPC_HEADER *rd){
	RPC_PACKET *packet = (RPC_PACKET*)rd->pkt_addr;
	return (rd->pkt_addr && 
	       (rd->rpc_id==packet->pid) && 
	       (packet->rec_id & 2));
}

///////////////////////////////////////////////////////////////////////[13]
void SifSetRpcQueue(struct sifcmd_RPC_DATA_QUEUE *qd, int key){
	u32 x;
	register struct sifcmd_RPC_DATA_QUEUE *q, *i;

	CpuSuspendIntr(&x);
	qd->key=key;
	qd->active=0;
	qd->link=0;
	qd->start=0;
	qd->end=0;
	qd->next=0;
	q = (struct sifcmd_RPC_DATA_QUEUE *)&rpc_common.queue;
	if (q) {
		for (i=q->next; i; i=q->next) q=q->next;
	}
	rpc_common.queue = qd;
	CpuResumeIntr(x);
}

///////////////////////////////////////////////////////////////////////[11]
void SifRegisterRpc(struct sifcmd_RPC_SERVER_DATA *sd, u32 command, 
		       rpch_func func, void *buff,
		       rpch_func cfunc, void *cbuff,
		       struct sifcmd_RPC_DATA_QUEUE *qd) {
	u32 x;
	register struct sifcmd_RPC_DATA_QUEUE *q, *i;

	CpuSuspendIntr(&x);
	sd->command=command;
	sd->func=func;
	sd->buff=buff;
	sd->next=0;
	sd->link=0;
	sd->cfunc=cfunc;
	sd->cbuff=cbuff;
	sd->base=qd;

	if (qd->link==0)
		qd->link=sd;
	else{
		for (q=qd->link, i=q->link; i; i=q->link)
			q=q->link;
		q->link=sd;
	}
	CpuResumeIntr(x);
}

///////////////////////////////////////////////////////////////////////[18]
struct sifcmd_RPC_SERVER_DATA *SifRemoveRpc(struct sifcmd_RPC_SERVER_DATA *sd, struct sifcmd_RPC_DATA_QUEUE *qd){
	u32 x;
	register struct sifcmd_RPC_SERVER_DATA *s;

	CpuSuspendIntr(&x);

	if ((s=qd->link)==sd)
		qd->link=s->link;
	else
		for ( ; s; s=s->link)
			if (s->link==sd){
				s->link=s->link->link;
				break;
			}
	CpuResumeIntr(x);
	return s;
}

///////////////////////////////////////////////////////////////////////[19]
struct sifcmd_RPC_DATA_QUEUE *SifRemoveRpcQueue(struct sifcmd_RPC_DATA_QUEUE *qd){
	u32 x;
	register struct sifcmd_RPC_DATA_QUEUE *q;

	CpuSuspendIntr(&x);

	q=rpc_common.queue;
	if (q==qd) {
		rpc_common.queue=q->next;
	} else {
		for (; q; q=q->next) {
			if (q->next==qd){
				q->next=q->next->next;
				break;
			}
		}
	}
	CpuResumeIntr(x);
	return q;
}

///////////////////////////////////////////////////////////////////////[14]
struct sifcmd_RPC_SERVER_DATA *SifGetNextRequest(struct sifcmd_RPC_DATA_QUEUE *qd){
	u32 x;
	register struct sifcmd_RPC_SERVER_DATA *s;

	CpuSuspendIntr(&x);

	if ((s=qd->start)==0)
		qd->active=0;
	else{
		qd->active=1;
		qd->start=qd->start->next;
	}

	CpuResumeIntr(x);
	return s;
}

///////////////////////////////////////////////////////////////////////[15]
void SifExecRequest(struct sifcmd_RPC_SERVER_DATA *sd){
	u32 x;
	register int size, id, count, i;
	register void *buff;
	RPC_PACKET_END *epacket;
	struct sifman_DMA dma[2];

	size=0;
	if (buff=sd->func(sd->fno, sd->buff, sd->size))
		size=sd->rsize;

	CpuSuspendIntr(&x);
	epacket=(RPC_PACKET_END *)rpc_get_fpacket(&rpc_common);
	CpuResumeIntr(x);

	epacket->command=0x8000000A;
	epacket->client=sd->client;
	count=0;
	if (sd->rmode){
		while (SifSendCmd(0x80000008, epacket, 0x40, buff, sd->receive, size)==0);
		return;
	}else{
		epacket->packet.pid=0;
		epacket->packet.rec_id=0;
		if (size>0){
			count=1;
			dma[count-1].data=buff;
			dma[count-1].size=size;
			dma[count-1].attr=0;
			dma[count-1].addr=sd->receive;
		}
		count++;
		dma[count-1].data=epacket;
		dma[count-1].size=0x40;
		dma[count-1].attr=0;
		dma[count-1].addr=sd->pkt_addr;
		do{
			CpuSuspendIntr(&x);
			id=SifSetDma(dma, count);
        		CpuResumeIntr(x);
			if (id)	break;
			i=0xFFFF; do --i; while (i!=-1);
		} while (id==0);
	}
}

///////////////////////////////////////////////////////////////////////[16]
void SifRpcLoop(struct sifcmd_RPC_DATA_QUEUE *qd){
	register struct sifcmd_RPC_SERVER_DATA *s;

	do{
		if (s=SifGetNextRequest(qd))
			SifExecRequest(s);
		else
			SleepThread();
	} while (1);
}

///////////////////////////////////////////////////////////////////////[0E]
void SifInitRpc(int mode){
	u32 x;
	_dprintf("%s\n", __FUNCTION__);

	SifInitCmd();
	CpuSuspendIntr(&x);

	if (sifInitRpc){
		CpuResumeIntr(x);		
	}else{
		sifInitRpc=1;
		rpc_common.paddr=(RPC_PACKET*)bufx;
		rpc_common.size=32;
		rpc_common.paddr2=0;
		rpc_common.size2=0;
		rpc_common.next=(RPC_PACKET*)bufy;
		rpc_common.count=32;
		rpc_common.base=0;
		rpc_common.pid=1;

		SifAddCmdHandler(0x80000008, (cmdh_func)cmd80000008_END,   &rpc_common);
		SifAddCmdHandler(0x80000009, (cmdh_func)cmd80000009_BIND,  &rpc_common);
		SifAddCmdHandler(0x8000000A, (cmdh_func)cmd8000000A_CALL,  &rpc_common);
		SifAddCmdHandler(0x8000000C, (cmdh_func)cmd8000000C_RDATA, &rpc_common);

		CpuResumeIntr(x);

		((SifCmdSRData*)bufx)->rno  =0;
		((SifCmdSRData*)bufx)->value=1;
		SifSendCmd(0x80000001, (void*)bufx, sizeof(SifCmdSRData), 0, 0, 0);
	}
	WaitEventFlag(GetSystemStatusFlag(), 0x800, 0, 0);
}

void _retonly() {}

struct export sifcmd_stub={
	0x41C00000,
	0,
	VER(1, 1),			// 1.1 => 0x101
	0,
	"sifcmd",
	(func)_start,			// entrypoint
	(func)_retonly,
	(func)SifDeinitCmd,
	(func)_retonly,
	(func)SifInitCmd,
	(func)SifExitCmd,
	(func)SifGetSreg,
	(func)SifSetSreg,
	(func)SifSetCmdBuffer,
	(func)SifSetSysCmdBuffer,
	(func)SifAddCmdHandler,
	(func)SifRemoveCmdHandler,
	(func)SifSendCmd,
	(func)iSifSendCmd,
	(func)SifInitRpc,
	(func)SifBindRpc,
	(func)SifCallRpc,
	(func)SifRegisterRpc,
	(func)SifCheckStatRpc,
	(func)SifSetRpcQueue,
	(func)SifGetNextRequest,
	(func)SifExecRequest,
	(func)SifRpcLoop,
	(func)SifGetOtherData,
	(func)SifRemoveRpc,
	(func)SifRemoveRpcQueue,
	(func)SifSet1CB,
	(func)SifReset1CB,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	0
};

//////////////////////////////entrypoint///////////////////////////////[00]
int _start(){
	register int *v, i;

	_dprintf("%s\n", __FUNCTION__);
	if (v=QueryBootMode(3)){
		_dprintf("bootmode: %x\n", v[1]);
		if (v[1] & 1){	printf("%s No SIF service(sifcmd)\n", __FUNCTION__);return 1;}
		if (v[1] & 2){	printf("%s No SIFCMD/RPC service\n", __FUNCTION__); return 1;}
	}

	if (SifCheckInit()==0)
		SifInit();

	if (RegisterLibraryEntries(&sifcmd_stub))	return 1;

	cmd_common.sif1_rcvBuffer=sif1_rcvBuffer;
	cmd_common.b=b;
	cmd_common.sysCmdBuffer=sysCmds;
	cmd_common.sysCmdBufferSize=32;
	cmd_common.saddr=0;
	cmd_common.cmdBuffer=0;
	cmd_common.cmdBufferSize=0;
	cmd_common.Sreg=Sreg;
	cmd_common.func=0;
	cmd_common.param=0;

	for (i=0; i<32; i++) {
		sysCmds[i].func=0;
		sysCmds[i].data=0;
	}
	for (i=0; i<32; i++) {
		Sreg[i]=0;
	}

	sysCmds[0].func=(cmdh_func)cmd80000000_CHANGE_SADDR;
	sysCmds[0].data=&cmd_common;

	sysCmds[1].func=(cmdh_func)cmd80000001_SET_SREG;
	sysCmds[1].data=&cmd_common;

	cmd_common.systemStatusFlag=GetSystemStatusFlag();
	
	sysCmds[2].func=(cmdh_func)cmd80000002_INIT_CMD;
	sysCmds[2].data=&cmd_common;

	RegisterIntrHandler(INT_DMA10, 1, SIF1_handler, (void*)&cmd_common);
	EnableIntr(INT_DMA10 | IMODE_DMA_IQE);

	SifSetIOPrcvaddr((u32)sif1_rcvBuffer);

	return 0;
}




