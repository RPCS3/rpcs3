/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SIFCMD_H__
#define __SIFCMD_H__

/* from sifcmd.h */

#define SYSTEM_CMD	0x80000000

struct t_sif_cmd_header
{
   u32				size;
   void				*dest;
   int				command;
   u32				unknown;
};

struct t_sif_dma_transfer
{
   void				*src,
      				*dest;
   int				size;
   int				attr;
};

struct t_sif_handler
{
   void     			(*handler)	( void *a, void *b);
   void	 			*buff;
};

#define SYSTEM_CMD_CHANGE_SADDR	0x80000000
#define SYSTEM_CMD_INIT_CMD	0x80000002
struct t_sif_saddr{
	struct t_sif_cmd_header	hdr;		//+00
	void			*newaddr;	//+10
};						//=14

#define SYSTEM_CMD_SET_SREG	0x80000001
struct t_sif_sreg{
	struct t_sif_cmd_header	hdr;		//+00
	int			index;		//+10
	unsigned int		value;		//+14
};						//=18

#define SYSTEM_CMD_RESET	0x80000003
struct t_sif_reset{
	struct t_sif_cmd_header	hdr;		//+00
	int			size,		//+10
				flag;		//+14
	char			data[80];	//+18
};						//=68

/* end of sifcmd.h */

/* from sifsrpc.h */

struct t_sif_rpc_rend
{
   struct t_sif_cmd_header	sifcmd;
   int				rec_id;		/* 04 */
   void				*pkt_addr;	/* 05 */
   int				rpc_id;		/* 06 */
                                
   struct t_rpc_client_data	*client;	/* 7 */
   u32                          command;	/* 8 */
   struct t_rpc_server_data	*server;	/* 9 */
   void				*buff,		/* 10 */
      				*buff2;		/* 11 */
};

struct t_sif_rpc_other_data
{
   struct t_sif_cmd_header	sifcmd;
   int				rec_id;		/* 04 */
   void				*pkt_addr;	/* 05 */
   int				rpc_id;		/* 06 */
                                
   struct t_rpc_receive_data	*receive;	/* 07 */
   void				*src;		/* 08 */
   void				*dest;		/* 09 */
   int				size;		/* 10 */
};

struct t_sif_rpc_bind
{
   struct t_sif_cmd_header	sifcmd;
   int				rec_id;		/* 04 */
   void				*pkt_addr;	/* 05 */
   int				rpc_id;		/* 06 */
   struct t_rpc_client_data	*client;	/* 07 */
   int				rpc_number;	/* 08 */
};

struct t_sif_rpc_call
{
   struct t_sif_cmd_header	sifcmd;
   int				rec_id;		/* 04 */
   void				*pkt_addr;	/* 05 */
   int				rpc_id;		/* 06 */
   struct t_rpc_client_data	*client;	/* 07 */
   int				rpc_number;	/* 08 */
   int				send_size;	/* 09 */
   void				*receive;	/* 10 */
   int				rec_size;	/* 11 */
   int				has_async_ef;	/* 12 */
   struct t_rpc_server_data	*server;	/* 13 */
};

struct t_rpc_server_data
{
   int				command;	/* 04	00 */

   void  *			(*func)(u32, void *, int);	/* 05	01 */
   void				*buff;		/* 06	02 */
   int				size;		/* 07	03 */

   void  * 			(*func2)(u32, void *, int); /* 08	04 */
   void				*buff2;		/* 09	05 */
   int				size2;		/* 10	06 */

   struct t_rpc_client_data	*client;	/* 11	07 */
   void				*pkt_addr;	/* 12	08 */
   int				rpc_number;	/* 13	09 */

   void				*receive;	/* 14	10 */
   int				rec_size;	/* 15	11 */
   int				has_async_ef;	/* 16	12 */
   int				rec_id;		/* 17	13 */

   struct t_rpc_server_data	*link;		/* 18	14 */
   struct r_rpc_server_data	*next;		/* 19	15 */
   struct t_rpc_data_queue	*queued_object;	/* 20	16 */
};


struct t_rpc_header
{
   void				*pkt_addr;	/* 04	00 */
   u32				rpc_id;		/* 05	01 */
   int				sema_id;	/* 06	02 */
   u32			 	mode;		/* 07	03 */
};


struct t_rpc_client_data
{
   struct t_rpc_header		hdr;
   u32				command;	/* 04 	08 */
   void				*buff, 		/* 05 	09 */
      				*buff2;		/* 06 	10 */
   void     			(*end_function)	( void *);	/* 07 	11 */
   void				*end_param;	/* 08 	12*/
   struct t_rpc_server_data	*server;	/* 09 	13 */
};

struct t_rpc_receive_data
{
   struct t_rpc_header		hdr;
   void				*src,		/* 04 */
      				*dest;		/* 05 */
   int	                        size;		/* 06 */
};

struct t_rpc_data_queue
{
   int				thread_id,	/* 00 */
      				active;		/* 01 */
   struct t_rpc_server_data	*svdata_ref,	/* 02 */
      				*start,		/* 03 */
                                *end;		/* 04 */
   struct t_rpc_data_queue	*next;  	/* 05 */
};

/* end of sifrpc.h */

#endif//__SIFCMD_H__
