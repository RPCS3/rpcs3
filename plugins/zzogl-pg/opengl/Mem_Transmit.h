#ifndef MEM_TRANSMIT_H_INCLUDED
#define MEM_TRANSMIT_H_INCLUDED

#include "GS.h"
#include "Mem.h"

#define DSTPSM gs.dstbuf.psm
extern int tempX, tempY;
extern int pitch, area, fracX; 
extern int nSize;
extern u8* pstart;

// transfers whole rows
template <class T>
static __forceinline bool TransmitHostLocalY_(_writePixel_0 wp, u32 widthlimit, u32 endY, const T *pbuf) 
{
	assert( (nSize%widthlimit) == 0 && widthlimit <= 4 );
	if ((gs.imageEndX-gs.trxpos.dx) % widthlimit) 
	{
		// GS_LOG("Bad Transmission! %d %d, psm: %d\n", gs.trxpos.dx, gs.imageEndX, DSTPSM);
		
		for(; tempY < endY; ++tempY) 
		{
			for(; tempX < gs.imageEndX && nSize > 0; tempX += 1, nSize -= 1, pbuf += 1) 
			{
				/* write as many pixel at one time as possible */
				wp(pstart, tempX%2048, tempY%2048, pbuf[0], gs.dstbuf.bw);
			}
		}
	}
	for(; tempY < endY; ++tempY) 
	{
		for(; tempX < gs.imageEndX && nSize > 0; tempX += widthlimit, nSize -= widthlimit, pbuf += widthlimit)
		{
			
			/* write as many pixel at one time as possible */
			if( nSize < widthlimit ) return false;
			
			wp(pstart, tempX%2048, tempY%2048, pbuf[0], gs.dstbuf.bw);
			
			if( widthlimit > 1 ) 
			{ 
				wp(pstart, (tempX+1)%2048, tempY%2048, pbuf[1], gs.dstbuf.bw);
				
				if( widthlimit > 2 ) 
				{ 
					wp(pstart, (tempX+2)%2048, tempY%2048, pbuf[2], gs.dstbuf.bw); 
										
					if( widthlimit > 3 ) 
					{ 
						wp(pstart, (tempX+3)%2048, tempY%2048, pbuf[3], gs.dstbuf.bw); 
					} 
				} 
			} 
		} 
		
		if ( tempX >= gs.imageEndX ) 
		{ 
			assert(tempX == gs.imageEndX);
			tempX = gs.trxpos.dx; 
		} 
		else 
		{ 
			assert( gs.imageTransfer == -1 || nSize*sizeof(T)/4 == 0 ); 
			return false; 
		} 
	} 
	return true;
} 

template <class T>
static __forceinline bool TransmitHostLocalX_(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *pbuf) 
{
	for(u32 tempi = 0; tempi < blockheight; ++tempi) 
	{ 
		for(tempX = startX; tempX < gs.imageEndX; tempX++, pbuf++) 
		{ 
			wp(pstart, tempX%2048, (tempY+tempi)%2048, pbuf[0], gs.dstbuf.bw); 
		} 
		pbuf += pitch - fracX; 
	} 
	return true;
} 

// transfers whole rows
template <class T>
static __forceinline bool TransmitHostLocalY_24(_writePixel_0 wp, u32 widthlimit, u32 endY, const T *pbuf) 
{
	if (widthlimit != 8 || ((gs.imageEndX-gs.trxpos.dx)%widthlimit)) 
	{
		//GS_LOG("Bad Transmission! %d %d, psm: %d\n", gs.trxpos.dx, gs.imageEndX, DSTPSM);
		for(; tempY < endY; ++tempY) 
		{ 
			for(; tempX < gs.imageEndX && nSize > 0; tempX += 1, nSize -= 1, pbuf += 3) 
			{
				wp(pstart, tempX%2048, tempY%2048, *(u32*)(pbuf), gs.dstbuf.bw);
			} 
			
			if( tempX >= gs.imageEndX ) 
			{ 
				assert(gs.imageTransfer == -1 || tempX == gs.imageEndX); 
				tempX = gs.trxpos.dx; 
			}
			else 
			{ 
				assert( gs.imageTransfer == -1 || nSize == 0 ); 
				return false;
			}
		}
	}
	else 
	{
		assert( /*(nSize%widthlimit) == 0 &&*/ widthlimit == 8 );
		for(; tempY < endY; ++tempY) 
		{
			for(; tempX < gs.imageEndX && nSize > 0; tempX += widthlimit, nSize -= widthlimit, pbuf += 3*widthlimit) 
			{
				if (nSize < widthlimit) return false;
				
				/* write as many pixel at one time as possible */
				
				wp(pstart, tempX%2048, tempY%2048, *(u32*)(pbuf+0), gs.dstbuf.bw); 
				wp(pstart, (tempX+1)%2048, tempY%2048, *(u32*)(pbuf+3), gs.dstbuf.bw); 
				wp(pstart, (tempX+2)%2048, tempY%2048, *(u32*)(pbuf+6), gs.dstbuf.bw); 
				wp(pstart, (tempX+3)%2048, tempY%2048, *(u32*)(pbuf+9), gs.dstbuf.bw); 
				wp(pstart, (tempX+4)%2048, tempY%2048, *(u32*)(pbuf+12), gs.dstbuf.bw); 
				wp(pstart, (tempX+5)%2048, tempY%2048, *(u32*)(pbuf+15), gs.dstbuf.bw); 
				wp(pstart, (tempX+6)%2048, tempY%2048, *(u32*)(pbuf+18), gs.dstbuf.bw); 
				wp(pstart, (tempX+7)%2048, tempY%2048, *(u32*)(pbuf+21), gs.dstbuf.bw); 
			} 
			
			if (tempX >= gs.imageEndX) 
			{ 
				assert(gs.imageTransfer == -1 || tempX == gs.imageEndX); 
				tempX = gs.trxpos.dx; 
			} 
			else 
			{
				if ( nSize < 0 ) 
				{
					/* extracted too much */
					assert( (nSize%3)==0 && nSize > -24 );
					tempX += nSize/3;
					nSize = 0;
				}
				assert( gs.imageTransfer == -1 || nSize == 0 );
				return false;
			}
		}
	}
	return true;
}

// transmit until endX, don't check size since it has already been prevalidated
template <class T>
static __forceinline bool TransmitHostLocalX_24(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *pbuf) 
{
	for(u32 tempi = 0; tempi < blockheight; ++tempi) 
	{ 
		for(tempX = startX; tempX < gs.imageEndX; tempX++, pbuf += 3) 
		{ 
			wp(pstart, tempX%2048, (tempY+tempi)%2048, *(u32*)pbuf, gs.dstbuf.bw); 
		} 
		pbuf += 3*(pitch-fracX); 
	} 
	return true;
} 

// meant for 4bit transfers
template <class T>
static __forceinline bool TransmitHostLocalY_4(_writePixel_0 wp, u32 widthlimit, u32 endY, const T *pbuf) 
{
	for(; tempY < endY; ++tempY) 
	{
		for(; tempX < gs.imageEndX && nSize > 0; tempX += widthlimit, nSize -= widthlimit) 
		{
			/* write as many pixel at one time as possible */ 
			wp(pstart, tempX%2048, tempY%2048, *pbuf&0x0f, gs.dstbuf.bw); 
			wp(pstart, (tempX+1)%2048, tempY%2048, *pbuf>>4, gs.dstbuf.bw); 
			pbuf++; 
			if ( widthlimit > 2 ) 
			{ 
				wp(pstart, (tempX+2)%2048, tempY%2048, *pbuf&0x0f, gs.dstbuf.bw); 
				wp(pstart, (tempX+3)%2048, tempY%2048, *pbuf>>4, gs.dstbuf.bw); 
				pbuf++; 
				
				if( widthlimit > 4 ) 
				{ 
					wp(pstart, (tempX+4)%2048, tempY%2048, *pbuf&0x0f, gs.dstbuf.bw); 
					wp(pstart, (tempX+5)%2048, tempY%2048, *pbuf>>4, gs.dstbuf.bw); 
					pbuf++; 
					
					if( widthlimit > 6 ) 
					{ 
						wp(pstart, (tempX+6)%2048, tempY%2048, *pbuf&0x0f, gs.dstbuf.bw); 
						wp(pstart, (tempX+7)%2048, tempY%2048, *pbuf>>4, gs.dstbuf.bw); 
						pbuf++; 
					} 
				} 
			} 
		} 
		
		if ( tempX >= gs.imageEndX ) 
		{ 
			tempX = gs.trxpos.dx; 
		} 
		else 
		{ 
			assert( gs.imageTransfer == -1 || (nSize/32) == 0 ); 
			return false;
		} 
	} 
	return true;
} 

// transmit until endX, don't check size since it has already been prevalidated
template <class T>
static __forceinline bool TransmitHostLocalX_4(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *pbuf) 
{ 
	for(u32 tempi = 0; tempi < blockheight; ++tempi)
	{
		for(tempX = startX; tempX < gs.imageEndX; tempX+=2, pbuf++) 
		{
			wp(pstart, tempX%2048, (tempY+tempi)%2048, pbuf[0]&0x0f, gs.dstbuf.bw);
			wp(pstart, (tempX+1)%2048, (tempY+tempi)%2048, pbuf[0]>>4, gs.dstbuf.bw);
		}
		pbuf += (pitch-fracX)/2;
	}
	return true;
}

// calculate pitch in source buffer
static __forceinline u32 TransPitch(u32 pitch, u32 size)
{
	return pitch * size / 8;
}

static __forceinline u32 TransPitch2(u32 pitch, u32 size)
{
	if (size == 4) return pitch / 2;
	if (size == 24) return pitch * 3;
	return pitch;
}

#endif // MEM_TRANSMIT_H_INCLUDED
