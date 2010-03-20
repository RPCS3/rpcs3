#ifndef MEM_TRANSMIT_H_INCLUDED
#define MEM_TRANSMIT_H_INCLUDED

#include "GS.h"
#include "Mem.h"

#define DSTPSM gs.dstbuf.psm

// transfers whole rows
#define TRANSMIT_HOSTLOCAL_Y_(psm, T, widthlimit, endY) { \
	assert( (nSize%widthlimit) == 0 && widthlimit <= 4 ); \
	if( (gs.imageEndX-gs.trxpos.dx)%widthlimit ) { \
		/*GS_LOG("Bad Transmission! %d %d, psm: %d\n", gs.trxpos.dx, gs.imageEndX, DSTPSM);*/ \
		for(; i < endY; ++i) { \
			for(; j < gs.imageEndX && nSize > 0; j += 1, nSize -= 1, pbuf += 1) { \
				/* write as many pixel at one time as possible */ \
				writePixel##psm##_0(pstart, j%2048, i%2048, pbuf[0], gs.dstbuf.bw); \
			} \
		} \
	} \
	for(; i < endY; ++i) { \
		for(; j < gs.imageEndX && nSize > 0; j += widthlimit, nSize -= widthlimit, pbuf += widthlimit) { \
			/* write as many pixel at one time as possible */ \
			if( nSize < widthlimit ) goto End; \
			writePixel##psm##_0(pstart, j%2048, i%2048, pbuf[0], gs.dstbuf.bw); \
			\
			if( widthlimit > 1 ) { \
				writePixel##psm##_0(pstart, (j+1)%2048, i%2048, pbuf[1], gs.dstbuf.bw); \
				\
				if( widthlimit > 2 ) { \
					writePixel##psm##_0(pstart, (j+2)%2048, i%2048, pbuf[2], gs.dstbuf.bw); \
					\
					if( widthlimit > 3 ) { \
						writePixel##psm##_0(pstart, (j+3)%2048, i%2048, pbuf[3], gs.dstbuf.bw); \
					} \
				} \
			} \
		} \
		\
		if( j >= gs.imageEndX ) { assert(j == gs.imageEndX); j = gs.trxpos.dx; } \
		else { assert( gs.imageTransfer == -1 || nSize*sizeof(T)/4 == 0 ); goto End; } \
	} \
} \

// transmit until endX, don't check size since it has already been prevalidated
#define TRANSMIT_HOSTLOCAL_X_(psm, T, widthlimit, blockheight, startX) { \
	for(int tempi = 0; tempi < blockheight; ++tempi) { \
		for(j = startX; j < gs.imageEndX; j++, pbuf++) { \
			writePixel##psm##_0(pstart, j%2048, (i+tempi)%2048, pbuf[0], gs.dstbuf.bw); \
		} \
		pbuf += pitch-fracX; \
	} \
} \

//template <class T>
//static __forceinline void TransmitHostLocalX_(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX) 
//{
//	for(int tempi = 0; tempi < blockheight; ++tempi) 
//	{ 
//		for(j = startX; j < gs.imageEndX; j++, pbuf++) 
//		{ 
//			wp(pstart, j%2048, (i+tempi)%2048, pbuf[0], gs.dstbuf.bw); 
//		} 
//		pbuf += pitch - fracX; 
//	} 
//} 

// transfers whole rows
#define TRANSMIT_HOSTLOCAL_Y_24(psm, T, widthlimit, endY) { \
	if( widthlimit != 8 || ((gs.imageEndX-gs.trxpos.dx)%widthlimit) ) { \
		/*GS_LOG("Bad Transmission! %d %d, psm: %d\n", gs.trxpos.dx, gs.imageEndX, DSTPSM);*/ \
		for(; i < endY; ++i) { \
			for(; j < gs.imageEndX && nSize > 0; j += 1, nSize -= 1, pbuf += 3) { \
				writePixel##psm##_0(pstart, j%2048, i%2048, *(u32*)(pbuf), gs.dstbuf.bw); \
			} \
			\
			if( j >= gs.imageEndX ) { assert(gs.imageTransfer == -1 || j == gs.imageEndX); j = gs.trxpos.dx; } \
			else { assert( gs.imageTransfer == -1 || nSize == 0 ); goto End; } \
		} \
	} \
	else { \
		assert( /*(nSize%widthlimit) == 0 &&*/ widthlimit == 8 ); \
		for(; i < endY; ++i) { \
			for(; j < gs.imageEndX && nSize > 0; j += widthlimit, nSize -= widthlimit, pbuf += 3*widthlimit) { \
				if( nSize < widthlimit ) goto End; \
				/* write as many pixel at one time as possible */ \
				writePixel##psm##_0(pstart, j%2048, i%2048, *(u32*)(pbuf+0), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+1)%2048, i%2048, *(u32*)(pbuf+3), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+2)%2048, i%2048, *(u32*)(pbuf+6), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+3)%2048, i%2048, *(u32*)(pbuf+9), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+4)%2048, i%2048, *(u32*)(pbuf+12), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+5)%2048, i%2048, *(u32*)(pbuf+15), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+6)%2048, i%2048, *(u32*)(pbuf+18), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+7)%2048, i%2048, *(u32*)(pbuf+21), gs.dstbuf.bw); \
			} \
			\
			if( j >= gs.imageEndX ) { assert(gs.imageTransfer == -1 || j == gs.imageEndX); j = gs.trxpos.dx; } \
			else { \
				if( nSize < 0 ) { \
					/* extracted too much */ \
					assert( (nSize%3)==0 && nSize > -24 ); \
					j += nSize/3; \
					nSize = 0; \
				} \
				assert( gs.imageTransfer == -1 || nSize == 0 ); \
				goto End; \
			} \
		} \
	} \
} \

// transmit until endX, don't check size since it has already been prevalidated
#define TRANSMIT_HOSTLOCAL_X_24(psm, T, widthlimit, blockheight, startX) { \
	for(int tempi = 0; tempi < blockheight; ++tempi) { \
		for(j = startX; j < gs.imageEndX; j++, pbuf += 3) { \
			writePixel##psm##_0(pstart, j%2048, (i+tempi)%2048, *(u32*)pbuf, gs.dstbuf.bw); \
		} \
		pbuf += 3*(pitch-fracX); \
	} \
} \


// meant for 4bit transfers
#define TRANSMIT_HOSTLOCAL_Y_4(psm, T, widthlimit, endY) { \
	for(; i < endY; ++i) { \
		for(; j < gs.imageEndX && nSize > 0; j += widthlimit, nSize -= widthlimit) { \
			/* write as many pixel at one time as possible */ \
			writePixel##psm##_0(pstart, j%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
			writePixel##psm##_0(pstart, (j+1)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
			pbuf++; \
			if( widthlimit > 2 ) { \
				writePixel##psm##_0(pstart, (j+2)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+3)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
				pbuf++; \
				\
				if( widthlimit > 4 ) { \
					writePixel##psm##_0(pstart, (j+4)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
					writePixel##psm##_0(pstart, (j+5)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
					pbuf++; \
					\
					if( widthlimit > 6 ) { \
						writePixel##psm##_0(pstart, (j+6)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
						writePixel##psm##_0(pstart, (j+7)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
						pbuf++; \
					} \
				} \
			} \
		} \
		\
		if( j >= gs.imageEndX ) { j = gs.trxpos.dx; } \
		else { assert( gs.imageTransfer == -1 || (nSize/32) == 0 ); goto End; } \
	} \
} \

// transmit until endX, don't check size since it has already been prevalidated
#define TRANSMIT_HOSTLOCAL_X_4(psm, T, widthlimit, blockheight, startX) { \
	for(int tempi = 0; tempi < blockheight; ++tempi) { \
		for(j = startX; j < gs.imageEndX; j+=2, pbuf++) { \
			writePixel##psm##_0(pstart, j%2048, (i+tempi)%2048, pbuf[0]&0x0f, gs.dstbuf.bw); \
			writePixel##psm##_0(pstart, (j+1)%2048, (i+tempi)%2048, pbuf[0]>>4, gs.dstbuf.bw); \
		} \
		pbuf += (pitch-fracX)/2; \
	} \
} \

#define TRANSMIT_HOSTLOCAL_X(th, psm, T, widthlimit, blockheight, startX) \
	TRANSMIT_HOSTLOCAL_X##th(psm, T, widthlimit, blockheight, startX)
#define TRANSMIT_HOSTLOCAL_Y(th, psm, T, widthlimit, endY) \
	TRANSMIT_HOSTLOCAL_Y##th(psm,T,widthlimit,endY)
// calculate pitch in source buffer


template <class T>
static __forceinline int TransmitPitch_(int pitch) { return (pitch * sizeof(T)); }
template <class T>
static __forceinline int TransmitPitch_24(int pitch) { return (pitch * 3); }
template <class T>
static __forceinline int TransmitPitch_4(int pitch) { return (pitch/2); }

#define TRANSMIT_PITCH_(pitch, T) TransmitPitch_<T>(pitch)
#define TRANSMIT_PITCH_24(pitch, T) TransmitPitch_24<T>(pitch)
#define TRANSMIT_PITCH_4(pitch, T) TransmitPitch_4<T>(pitch)

#endif // MEM_TRANSMIT_H_INCLUDED
