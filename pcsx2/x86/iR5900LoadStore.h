/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#ifndef __IR5900LOADSTORE_H__
#define __IR5900LOADSTORE_H__
/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/

void recLB( void );
void recLBU( void );
void recLH( void );
void recLHU( void );
void recLW( void );
void recLWU( void );
void recLWL( void );
void recLWR( void );
void recLD( void );
void recLDR( void );
void recLDL( void );
void recLQ( void );
void recSB( void );
void recSH( void );
void recSW( void );
void recSWL( void );
void recSWR( void );
void recSD( void );
void recSDL( void );
void recSDR( void );
void recSQ( void );
void recLWC1( void );
void recSWC1( void );
void recLQC2( void );
void recSQC2( void );

// coissues
#ifdef PCSX2_VIRTUAL_MEM
void recLB_co( void );
void recLBU_co( void );
void recLH_co( void );
void recLHU_co( void );
void recLW_co( void );
void recLWU_co( void );
void recLWL_co( void );
void recLWR_co( void );
void recLD_co( void );
void recLDR_co( void );
void recLDL_co( void );
void recLQ_co( void );
void recSB_co( void );
void recSH_co( void );
void recSW_co( void );
void recSWL_co( void );
void recSWR_co( void );
void recSD_co( void );
void recSDL_co( void );
void recSDR_co( void );
void recSQ_co( void );
void recLWC1_co( void );
void recSWC1_co( void );
void recLQC2_co( void );
void recSQC2_co( void );

// coissue-X
void recLD_coX(int num);
void recLQ_coX(int num);
void recLWC1_coX(int num);
void recSD_coX(int num);
void recSQ_coX(int num);
void recSWC1_coX(int num);

#endif

#endif
