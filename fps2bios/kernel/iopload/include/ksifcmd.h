#ifndef __SIFCMD_H__
#define __SIFCMD_H__

#define SIFCMD_VER	0x101

#define SIF_CMDI_SYSTEM		0x80000000	// system function call
#define SIF_CMDC_CHANGE_SADDR	( SIF_CMDI_SYSTEM | 0x00000000)
#define SIF_CMDC_SET_SREG	( SIF_CMDI_SYSTEM | 0x00000001)
#define SIF_CMDC_INIT_CMD	( SIF_CMDI_SYSTEM | 0x00000002)
#define SIF_CMDC_RESET_CMD	( SIF_CMDI_SYSTEM | 0x00000003)

#define SIF_CMDC_RPC_END	( SIF_CMDI_SYSTEM | 0x00000008)
#define SIF_CMDC_RPC_BIND	( SIF_CMDI_SYSTEM | 0x00000009)
#define SIF_CMDC_RPC_CALL	( SIF_CMDI_SYSTEM | 0x0000000A)
#define SIF_CMDC_RPC_RDATA	( SIF_CMDI_SYSTEM | 0x0000000C)

typedef  struct {
	unsigned int	psize:8; // packet size [16->112]
	unsigned int	dsize:24;// extra data size
	unsigned int	daddr;   // extra data address
  	unsigned int	fcode;	 // function code
	unsigned int	opt;	 // optional user parameter
} SifCmdHdr;

typedef int (*cmdh_func) (SifCmdHdr*, void*);
typedef void *(*rpch_func)(u32 code, void *param1, int param2);

typedef struct {
	SifCmdHdr	hdr;
	void		*newaddr;
} SifCmdCSData;

typedef struct {
	SifCmdHdr	chdr;
	int		rno;
	unsigned int	value;
} SifCmdSRData;

typedef struct {
	SifCmdHdr	chdr;
	int		size;
	int		flag;
	char		arg[80];
} SifCmdResetData;

struct sifcmd_RPC_SERVER_DATA{	//t_rpc_server_data
   int				command;	/* 04	00 */

   rpch_func			func;		/* 05	01 */
   void				*buff;		/* 06	02 */
   int				size;		/* 07	03 */

   rpch_func 			cfunc;		/* 08	04 */
   void				*cbuff;		/* 09	05 */
   int				csize;		/* 10	06 */

   struct sifcmd_RPC_CLIENT_DATA*client;	/* 11	07 */
   void				*pkt_addr;	/* 12	08 */
   int				fno;		/* 13	09 */

   void				*receive;	/* 14	10 */
   int				rsize;		/* 15	11 */
   int				rmode;		/* 16	12 */
   int				rid;		/* 17	13 */

   struct sifcmd_RPC_SERVER_DATA*link;		/* 18	14 */
   struct sifcmd_RPC_SERVER_DATA*next;		/* 19	15 */
   struct sifcmd_RPC_DATA_QUEUE	*base;		/* 20	16 */
};


struct sifcmd_RPC_HEADER{	//t_rpc_header
   void				*pkt_addr;	/* 04	00 */
   u32				rpc_id;		/* 05	01 */
   int				tid;		/* 06	02 */
   u32			 	mode;		/* 07	03 */
};


struct sifcmd_RPC_CLIENT_DATA{	//t_rpc_client_data
   struct sifcmd_RPC_HEADER	hdr;
   u32				command;	/* 04 	08 */
   void				*buff, 		/* 05 	09 */
      				*cbuff;		/* 06 	10 */
   void				(*func)(void*);	/* 07 	11 */
   void				*param;		/* 08 	12*/
   struct sifcmd_RPC_SERVER_DATA*server;	/* 09 	13 */
};

struct sifcmd_RPC_RECEIVE_DATA{	//t_rpc_receive_data
   struct sifcmd_RPC_HEADER	hdr;
   void				*src,		/* 04 */
      				*dest;		/* 05 */
   int	                        size;		/* 06 */
};

struct sifcmd_RPC_DATA_QUEUE{	//t_rpc_data_queue
   int				key,		/* 00 */
      				active;		/* 01 */
   struct sifcmd_RPC_SERVER_DATA*link,		/* 02 */
      				*start,		/* 03 */
                                *end;		/* 04 */
   struct sifcmd_RPC_DATA_QUEUE	*next;  	/* 05 */
};

typedef struct {
	SifCmdHdr hdr;
	u32 rec_id;
	void *paddr;
	u32 pid;
} RPC_PACKET;

typedef struct {
	RPC_PACKET packet;
	struct sifcmd_RPC_CLIENT_DATA *client;
	u32 command;
	struct sifcmd_RPC_SERVER_DATA *server;
	void *buff, *cbuff;
} RPC_PACKET_END;

typedef struct {
	RPC_PACKET packet;
	struct sifcmd_RPC_CLIENT_DATA *client;
	u32 fno;
} RPC_PACKET_BIND;

typedef struct {
	RPC_PACKET_BIND packet;
	u32 size;
	void *receive;
	u32 rsize;
	u32 rmode;
	struct sifcmd_RPC_SERVER_DATA *server;
} RPC_PACKET_CALL;

typedef struct {
	RPC_PACKET packet;
	struct sifcmd_RPC_CLIENT_DATA *client;
	void *src;
	void *dst;
	u32 size;
} RPC_PACKET_RDATA;


typedef struct {
	cmdh_func	func;
	void		*data;
} SifCmdData;

void SifInitCmd();						//4 (21)
void SifAddCmdHandler(int pos, cmdh_func func, void *data);	//10(21)
void SifInitRpc(int mode);						//14(26)
void SifRegisterRpc(struct sifcmd_RPC_SERVER_DATA *sd, u32 cmd,
		       rpch_func func, void *buff,
		       rpch_func cfunc, void *cbuff,
		       struct sifcmd_RPC_DATA_QUEUE *dq);		//17(26)
void SifSetRpcQueue(struct sifcmd_RPC_DATA_QUEUE* dq, int thid);	//19(26)
void SifRpcLoop(struct sifcmd_RPC_DATA_QUEUE* dq);		//22(26)
int  SifGetOtherData(struct sifcmd_RPC_RECEIVE_DATA *rd,
		void *src, void *dst, int size, int mode);		//23(26)

#endif//__SIFCMD_H__
