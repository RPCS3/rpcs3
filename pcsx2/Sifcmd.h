/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SIFCMD_H__
#define __SIFCMD_H__

/* from sifcmd.h */

#define SYSTEM_CMD	0x80000000

struct t_sif_cmd_header
{
	u32 size;
	void *dest;
	s32 command;
	u32 unknown;
};

struct t_sif_dma_transfer
{
	void *src;
	void *dest;
	s32 size;
	s32 attr;
};

struct t_sif_handler
{
	void	(*handler)(void *a, void *b);
	void *buff;
};

#define SYSTEM_CMD_CHANGE_SADDR	0x80000000
#define SYSTEM_CMD_INIT_CMD	0x80000002
struct t_sif_saddr
{
	struct t_sif_cmd_header	hdr;		//+00
	void *newaddr;	//+10
};						//=14

#define SYSTEM_CMD_SET_SREG	0x80000001
struct t_sif_sreg
{
	struct t_sif_cmd_header	hdr;		//+00
	s32 index;		//+10
	u32 value;		//+14
};						//=18

#define SYSTEM_CMD_RESET	0x80000003
struct t_sif_reset
{
	struct t_sif_cmd_header	hdr;		//+00
	s32 size;		//+10
	s32 flag;		//+14
	char data[80];	//+18
};						//=68

/* end of sifcmd.h */

/* from sifsrpc.h */

struct t_sif_rpc_rend
{
	struct t_sif_cmd_header	sifcmd;
	s32 rec_id;		/* 04 */
	void *pkt_addr;	/* 05 */
	s32 rpc_id;		/* 06 */

	struct t_rpc_client_data	*client;	/* 7 */
	u32 command;	/* 8 */
	struct t_rpc_server_data	*server;	/* 9 */
	void *buff;		/* 10 */
	void *buff2;		/* 11 */
};

struct t_sif_rpc_other_data
{
	struct t_sif_cmd_header	sifcmd;
	s32 rec_id;		/* 04 */
	void *pkt_addr;	/* 05 */
	s32 rpc_id;		/* 06 */

	struct t_rpc_receive_data	*receive;	/* 07 */
	void *src;		/* 08 */
	void *dest;		/* 09 */
	s32 size;		/* 10 */
};

struct t_sif_rpc_bind
{
	struct t_sif_cmd_header	sifcmd;
	s32 rec_id;		/* 04 */
	void *pkt_addr;	/* 05 */
	s32 rpc_id;		/* 06 */
	struct t_rpc_client_data	*client;	/* 07 */
	s32 rpc_number;	/* 08 */
};

struct t_sif_rpc_call
{
	struct t_sif_cmd_header	sifcmd;
	s32 rec_id;		/* 04 */
	void *pkt_addr;	/* 05 */
	s32 rpc_id;		/* 06 */
	struct t_rpc_client_data	*client;	/* 07 */
	s32 rpc_number;	/* 08 */
	s32 send_size;	/* 09 */
	void *receive;	/* 10 */
	s32 rec_size;	/* 11 */
	s32 has_async_ef;	/* 12 */
	struct t_rpc_server_data	*server;	/* 13 */
};

struct t_rpc_server_data
{
	s32 command;	/* 04	00 */

	void  *(*func)(u32, void *, int);	/* 05	01 */
	void *buff;		/* 06	02 */
	s32 size;		/* 07	03 */

	void  *(*func2)(u32, void *, int);    /* 08	04 */
	void *buff2;		/* 09	05 */
	s32 size2;		/* 10	06 */

	struct t_rpc_client_data	*client;	/* 11	07 */
	void *pkt_addr;	/* 12	08 */
	s32 rpc_number;	/* 13	09 */

	void *receive;	/* 14	10 */
	s32 rec_size;	/* 15	11 */
	s32 has_async_ef;	/* 16	12 */
	s32 rec_id;		/* 17	13 */

	struct t_rpc_server_data	*link;		/* 18	14 */
	struct r_rpc_server_data	*next;		/* 19	15 */
	struct t_rpc_data_queue	*queued_object;	/* 20	16 */
};


struct t_rpc_header
{
	void *pkt_addr;	/* 04	00 */
	u32 rpc_id;		/* 05	01 */
	s32 sema_id;	/* 06	02 */
	u32 mode;		/* 07	03 */
};


struct t_rpc_client_data
{
	struct t_rpc_header		hdr;
	u32 command;	/* 04 	08 */
	void *buff; 		/* 05 	09 */
	void *buff2;		/* 06 	10 */
	void	(*end_function)(void *);	/* 07 	11 */
	void *end_param;	/* 08 	12*/
	struct t_rpc_server_data *server;	/* 09 	13 */
};

struct t_rpc_receive_data
{
	struct t_rpc_header hdr;
	void *src;		/* 04 */
	void *dest;		/* 05 */
	s32 size;		/* 06 */
};

struct t_rpc_data_queue
{
	s32 thread_id;	/* 00 */
	s32 active;		/* 01 */
	struct t_rpc_server_data *svdata_ref;	/* 02 */
	struct t_rpc_server_data *start;		/* 03 */
	struct t_rpc_server_data *end;		/* 04 */
	struct t_rpc_data_queue *next;  	/* 05 */
};

/* end of sifrpc.h */

#endif//__SIFCMD_H__
