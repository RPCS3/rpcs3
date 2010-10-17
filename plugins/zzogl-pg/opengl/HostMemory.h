/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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
 
#ifndef HOSTMEMORY_H_INCLUDED
#define HOSTMEMORY_H_INCLUDED

extern int GPU_TEXWIDTH;

extern u8* g_pBasePS2Mem;
extern u8* g_pbyGSMemory;

class GSMemory
{
	public:
		void init();
		void destroy();
		u8* get();
		u8* get(u32 addr);
		u8* get_raw(u32 addr);
};

extern u8* g_pbyGSClut;		// the temporary clut buffer

class GSClut
{
	public:
		void init();
		void destroy();
		u8* get();
		u8* get(u32 addr);
		u8* get_raw(u32 addr);
};

// The size in bytes of x strings (of texture).
inline int MemorySize(int x) 
{
	return 4 * GPU_TEXWIDTH * x;
}

// Return the address in memory of data block for string x. 
inline u8* MemoryAddress(int x) 
{
	return g_pbyGSMemory + MemorySize(x);
}

template <u32 mult>
inline u8* _MemoryAddress(int x) 
{
	return g_pbyGSMemory + mult * x;
}

extern void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw);


// called when trxdir is accessed. If host is involved, transfers memory to temp buffer byTransferBuf.
// Otherwise performs the transfer. TODO: Perhaps divide the transfers into chunks?
extern void InitTransferHostLocal();
extern void TransferHostLocal(const void* pbyMem, u32 nQWordSize);

extern void InitTransferLocalHost();
extern void TransferLocalHost(void* pbyMem, u32 nQWordSize);

extern void TransferLocalLocal();

extern void TerminateLocalHost();
extern void TerminateHostLocal();

#endif // HOSTMEMORY_H_INCLUDED
