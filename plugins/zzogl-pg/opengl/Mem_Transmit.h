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
static __forceinline const T *TransmitHostLocalY_(_writePixel_0 wp, u32 widthlimit, u32 endY, const T *buf)
{
	assert( (nSize%widthlimit) == 0 && widthlimit <= 4 );
	if ((gs.imageEndX-gs.trxpos.dx) % widthlimit)
	{
		// ZZLog::GS_Log("Bad Transmission! %d %d, psm: %d", gs.trxpos.dx, gs.imageEndX, DSTPSM);

		for(; tempY < endY; ++tempY)
		{
			for(; tempX < gs.imageEndX && nSize > 0; tempX += 1, nSize -= 1, buf += 1)
			{
				/* write as many pixel at one time as possible */
				wp(pstart, tempX%2048, tempY%2048, buf[0], gs.dstbuf.bw);
			}
		}
	}
	for(; tempY < endY; ++tempY)
	{
		for(; tempX < gs.imageEndX && nSize > 0; tempX += widthlimit, nSize -= widthlimit, buf += widthlimit)
		{

			/* write as many pixel at one time as possible */
			if( nSize < widthlimit ) return NULL;

			wp(pstart, tempX%2048, tempY%2048, buf[0], gs.dstbuf.bw);

			if( widthlimit > 1 )
			{
				wp(pstart, (tempX+1)%2048, tempY%2048, buf[1], gs.dstbuf.bw);

				if( widthlimit > 2 )
				{
					wp(pstart, (tempX+2)%2048, tempY%2048, buf[2], gs.dstbuf.bw);

					if( widthlimit > 3 )
					{
						wp(pstart, (tempX+3)%2048, tempY%2048, buf[3], gs.dstbuf.bw);
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
			return NULL;
		}
	}
	return buf;
}

// transfers whole rows
template <class T>
static __forceinline const T *TransmitHostLocalY_24(_writePixel_0 wp, u32 widthlimit, u32 endY, const T *buf)
{
	if (widthlimit != 8 || ((gs.imageEndX-gs.trxpos.dx)%widthlimit))
	{
		//ZZLog::GS_Log("Bad Transmission! %d %d, psm: %d", gs.trxpos.dx, gs.imageEndX, DSTPSM);
		for(; tempY < endY; ++tempY)
		{
			for(; tempX < gs.imageEndX && nSize > 0; tempX += 1, nSize -= 1, buf += 3)
			{
				wp(pstart, tempX%2048, tempY%2048, *(u32*)(buf), gs.dstbuf.bw);
			}

			if( tempX >= gs.imageEndX )
			{
				assert(gs.imageTransfer == -1 || tempX == gs.imageEndX);
				tempX = gs.trxpos.dx;
			}
			else
			{
				assert( gs.imageTransfer == -1 || nSize == 0 );
				return NULL;
			}
		}
	}
	else
	{
		assert( /*(nSize%widthlimit) == 0 &&*/ widthlimit == 8 );
		for(; tempY < endY; ++tempY)
		{
			for(; tempX < gs.imageEndX && nSize > 0; tempX += widthlimit, nSize -= widthlimit, buf += 3*widthlimit)
			{
				if (nSize < widthlimit) return NULL;

				/* write as many pixel at one time as possible */

				wp(pstart, tempX%2048, tempY%2048, *(u32*)(buf+0), gs.dstbuf.bw);
				wp(pstart, (tempX+1)%2048, tempY%2048, *(u32*)(buf+3), gs.dstbuf.bw);
				wp(pstart, (tempX+2)%2048, tempY%2048, *(u32*)(buf+6), gs.dstbuf.bw);
				wp(pstart, (tempX+3)%2048, tempY%2048, *(u32*)(buf+9), gs.dstbuf.bw);
				wp(pstart, (tempX+4)%2048, tempY%2048, *(u32*)(buf+12), gs.dstbuf.bw);
				wp(pstart, (tempX+5)%2048, tempY%2048, *(u32*)(buf+15), gs.dstbuf.bw);
				wp(pstart, (tempX+6)%2048, tempY%2048, *(u32*)(buf+18), gs.dstbuf.bw);
				wp(pstart, (tempX+7)%2048, tempY%2048, *(u32*)(buf+21), gs.dstbuf.bw);
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
				return NULL;
			}
		}
	}
	return buf;
}

// meant for 4bit transfers
template <class T>
static __forceinline const T *TransmitHostLocalY_4(_writePixel_0 wp, u32 widthlimit, u32 endY, const T *buf)
{
	for(; tempY < endY; ++tempY)
	{
		for(; tempX < gs.imageEndX && nSize > 0; tempX += widthlimit, nSize -= widthlimit)
		{
			/* write as many pixel at one time as possible */
			wp(pstart, tempX%2048, tempY%2048, *buf&0x0f, gs.dstbuf.bw);
			wp(pstart, (tempX+1)%2048, tempY%2048, *buf>>4, gs.dstbuf.bw);
			buf++;
			if ( widthlimit > 2 )
			{
				wp(pstart, (tempX+2)%2048, tempY%2048, *buf&0x0f, gs.dstbuf.bw);
				wp(pstart, (tempX+3)%2048, tempY%2048, *buf>>4, gs.dstbuf.bw);
				buf++;

				if( widthlimit > 4 )
				{
					wp(pstart, (tempX+4)%2048, tempY%2048, *buf&0x0f, gs.dstbuf.bw);
					wp(pstart, (tempX+5)%2048, tempY%2048, *buf>>4, gs.dstbuf.bw);
					buf++;

					if( widthlimit > 6 )
					{
						wp(pstart, (tempX+6)%2048, tempY%2048, *buf&0x0f, gs.dstbuf.bw);
						wp(pstart, (tempX+7)%2048, tempY%2048, *buf>>4, gs.dstbuf.bw);
						buf++;
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
			return NULL;
		}
	}
	return buf;
}

template <class T>
 static __forceinline const T *TransmitHostLocalY(TransferData data, _writePixel_0 wp, u32 widthlimit, u32 endY, const T *buf)
 {
 	switch (data.psm)
 	{
 		case PSM_: return TransmitHostLocalY_<T>(wp, widthlimit, endY, buf);
 		case PSM_4_: return TransmitHostLocalY_4<T>(wp, widthlimit, endY, buf);
 		case PSM_24_: return TransmitHostLocalY_24<T>(wp, widthlimit, endY, buf);
 	}
 	assert(0);
 	return NULL;
 }

template <class T>
static __forceinline const T *TransmitHostLocalX_(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *buf)
{
	for(u32 tempi = 0; tempi < blockheight; ++tempi)
	{
		for(tempX = startX; tempX < gs.imageEndX; tempX++, buf++)
		{
			wp(pstart, tempX%2048, (tempY+tempi)%2048, buf[0], gs.dstbuf.bw);
		}
		buf += pitch - fracX;
	}
	return buf;
}

// transmit until endX, don't check size since it has already been prevalidated
template <class T>
static __forceinline const T *TransmitHostLocalX_24(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *buf)
{
	for(u32 tempi = 0; tempi < blockheight; ++tempi)
	{
		for(tempX = startX; tempX < gs.imageEndX; tempX++, buf += 3)
		{
			wp(pstart, tempX%2048, (tempY+tempi)%2048, *(u32*)buf, gs.dstbuf.bw);
		}
		buf += 3*(pitch-fracX);
	}
	return buf;
}

// transmit until endX, don't check size since it has already been prevalidated
template <class T>
static __forceinline const T *TransmitHostLocalX_4(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *buf)
{
	for(u32 tempi = 0; tempi < blockheight; ++tempi)
	{
		for(tempX = startX; tempX < gs.imageEndX; tempX+=2, buf++)
		{
			wp(pstart, tempX%2048, (tempY+tempi)%2048, buf[0]&0x0f, gs.dstbuf.bw);
			wp(pstart, (tempX+1)%2048, (tempY+tempi)%2048, buf[0]>>4, gs.dstbuf.bw);
		}
		buf += (pitch-fracX)/2;
	}
	return buf;
}

template <class T>
 static __forceinline const T *TransmitHostLocalX(TransferData data, _writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *buf)
 {
 	switch (data.psm)
 	{
 		case PSM_: return TransmitHostLocalX_<T>(wp, widthlimit, blockheight, startX, buf);
 		case PSM_4_: return TransmitHostLocalX_4<T>(wp, widthlimit, blockheight, startX, buf);
 		case PSM_24_: return TransmitHostLocalX_24<T>(wp, widthlimit, blockheight, startX, buf);
 	}
 	assert(0);
 	return NULL;
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
