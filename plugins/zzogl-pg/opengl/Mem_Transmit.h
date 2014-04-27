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

#ifndef MEM_TRANSMIT_H_INCLUDED
#define MEM_TRANSMIT_H_INCLUDED

#include "GS.h"
#include "Mem.h"

extern int tempX, tempY;
extern int pitch, area, fracX;
extern int nSize;
extern u8* pstart;

extern char* psm_name[64];

// transfers whole rows
template <class T>
static __forceinline const T *TransmitHostLocalY_(_writePixel_0 wp, s32 widthlimit, int endY, const T *buf)
{
	assert((nSize % widthlimit) == 0 && widthlimit <= 4);

	if ((gs.imageEnd.x - gs.trxpos.dx) % widthlimit)
	{
		// ZZLog::GS_Log("Bad Transmission! %d %d, psm: %d", gs.trxpos.dx, gs.imageEnd.x, gs.dstbuf.psm);

		for (; tempY < endY; ++tempY)
		{
			for (; tempX < gs.imageEnd.x && nSize > 0; tempX += 1, nSize -= 1, buf += 1)
			{
				/* write as many pixel at one time as possible */
				wp(pstart, tempX % 2048, tempY % 2048, buf[0], gs.dstbuf.bw);
			}
		}
	}

	for (; tempY < endY; ++tempY)
	{
		for (; tempX < gs.imageEnd.x && nSize > 0; tempX += widthlimit, nSize -= widthlimit, buf += widthlimit)
		{

			/* write as many pixel at one time as possible */
			if (nSize < widthlimit) return NULL;

			wp(pstart, tempX % 2048, tempY % 2048, buf[0], gs.dstbuf.bw);

			if (widthlimit > 1)
			{
				wp(pstart, (tempX + 1) % 2048, tempY % 2048, buf[1], gs.dstbuf.bw);

				if (widthlimit > 2)
				{
					wp(pstart, (tempX + 2) % 2048, tempY % 2048, buf[2], gs.dstbuf.bw);

					if (widthlimit > 3)
					{
						wp(pstart, (tempX + 3) % 2048, tempY % 2048, buf[3], gs.dstbuf.bw);
					}
				}
			}
		}

		if (tempX >= gs.imageEnd.x)
		{
			assert(tempX == gs.imageEnd.x);
			tempX = gs.trxpos.dx;
		}
		else
		{
			assert(gs.transferring == false || nSize*sizeof(T) / 4 == 0);
			return NULL;
		}
	}

	return buf;
}

// transfers whole rows
template <class T>
static __forceinline const T *TransmitHostLocalY_24(_writePixel_0 wp, s32 widthlimit, int endY, const T *buf)
{
	if (widthlimit != 8 || ((gs.imageEnd.x - gs.trxpos.dx) % widthlimit))
	{
		//ZZLog::GS_Log("Bad Transmission! %d %d, psm: %d", gs.trxpos.dx, gs.imageEnd.x, gs.dstbuf.psm);
		for (; tempY < endY; ++tempY)
		{
			for (; tempX < gs.imageEnd.x && nSize > 0; tempX += 1, nSize -= 1, buf += 3)
			{
				wp(pstart, tempX % 2048, tempY % 2048, *(u32*)(buf), gs.dstbuf.bw);
			}

			if (tempX >= gs.imageEnd.x)
			{
				assert(gs.transferring == false || tempX == gs.imageEnd.x);
				tempX = gs.trxpos.dx;
			}
			else
			{
				assert(gs.transferring == false || nSize == 0);
				return NULL;
			}
		}
	}
	else
	{
		assert(/*(nSize%widthlimit) == 0 &&*/ widthlimit == 8);

		for (; tempY < endY; ++tempY)
		{
			for (; tempX < gs.imageEnd.x && nSize > 0; tempX += widthlimit, nSize -= widthlimit, buf += 3 * widthlimit)
			{
				if (nSize < widthlimit) return NULL;

				/* write as many pixel at one time as possible */

				wp(pstart, tempX % 2048, tempY % 2048, *(u32*)(buf + 0), gs.dstbuf.bw);
				wp(pstart, (tempX + 1) % 2048, tempY % 2048, *(u32*)(buf + 3), gs.dstbuf.bw);
				wp(pstart, (tempX + 2) % 2048, tempY % 2048, *(u32*)(buf + 6), gs.dstbuf.bw);
				wp(pstart, (tempX + 3) % 2048, tempY % 2048, *(u32*)(buf + 9), gs.dstbuf.bw);
				wp(pstart, (tempX + 4) % 2048, tempY % 2048, *(u32*)(buf + 12), gs.dstbuf.bw);
				wp(pstart, (tempX + 5) % 2048, tempY % 2048, *(u32*)(buf + 15), gs.dstbuf.bw);
				wp(pstart, (tempX + 6) % 2048, tempY % 2048, *(u32*)(buf + 18), gs.dstbuf.bw);
				wp(pstart, (tempX + 7) % 2048, tempY % 2048, *(u32*)(buf + 21), gs.dstbuf.bw);
			}

			if (tempX >= gs.imageEnd.x)
			{
				assert(gs.transferring == false || tempX == gs.imageEnd.x);
				tempX = gs.trxpos.dx;
			}
			else
			{
				if (nSize < 0)
				{
					/* extracted too much */
					assert((nSize % 3) == 0 && nSize > -24);
					tempX += nSize / 3;
					nSize = 0;
				}

				assert(gs.transferring == false || nSize == 0);

				return NULL;
			}
		}
	}

	return buf;
}

// meant for 4bit transfers
template <class T>
static __forceinline const T *TransmitHostLocalY_4(_writePixel_0 wp, s32 widthlimit, int endY, const T *buf)
{
	for (; tempY < endY; ++tempY)
	{
		for (; tempX < gs.imageEnd.x && nSize > 0; tempX += widthlimit, nSize -= widthlimit)
		{
			/* write as many pixel at one time as possible */
			wp(pstart, tempX % 2048, tempY % 2048, *buf&0x0f, gs.dstbuf.bw);
			wp(pstart, (tempX + 1) % 2048, tempY % 2048, *buf >> 4, gs.dstbuf.bw);
			buf++;

			if (widthlimit > 2)
			{
				wp(pstart, (tempX + 2) % 2048, tempY % 2048, *buf&0x0f, gs.dstbuf.bw);
				wp(pstart, (tempX + 3) % 2048, tempY % 2048, *buf >> 4, gs.dstbuf.bw);
				buf++;

				if (widthlimit > 4)
				{
					wp(pstart, (tempX + 4) % 2048, tempY % 2048, *buf&0x0f, gs.dstbuf.bw);
					wp(pstart, (tempX + 5) % 2048, tempY % 2048, *buf >> 4, gs.dstbuf.bw);
					buf++;

					if (widthlimit > 6)
					{
						wp(pstart, (tempX + 6) % 2048, tempY % 2048, *buf&0x0f, gs.dstbuf.bw);
						wp(pstart, (tempX + 7) % 2048, tempY % 2048, *buf >> 4, gs.dstbuf.bw);
						buf++;
					}
				}
			}
		}

		if (tempX >= gs.imageEnd.x)
		{
			tempX = gs.trxpos.dx;
		}
		else
		{
			assert(gs.transferring == false || (nSize / 32) == 0);
			return NULL;
		}
	}

	return buf;
}

template <class T>
static __forceinline const T *TransmitHostLocalY(u32 psm, _writePixel_0 wp, s32 widthlimit, int endY, const T *buf)
{
	//ZZLog::WriteLn("TransmitHostLocalY: psm == %s, bimode == 0x%x", psm_name[psm], PSMT_BITMODE(psm));
	switch (PSMT_BITMODE(psm))
	{
		case 1:
			return TransmitHostLocalY_24<T>(wp, widthlimit, endY, buf);
		case 4:
			return TransmitHostLocalY_4<T>(wp, widthlimit, endY, buf);
		default:
			return TransmitHostLocalY_<T>(wp, widthlimit, endY, buf);
	}

	assert(0);

	return NULL;
}

template <class T>
static __forceinline const T *TransmitHostLocalX_(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *buf)
{
	for (u32 tempi = 0; tempi < blockheight; ++tempi)
	{
		for (tempX = startX; tempX < gs.imageEnd.x; tempX++, buf++)
		{
			wp(pstart, tempX % 2048, (tempY + tempi) % 2048, buf[0], gs.dstbuf.bw);
		}

		buf += pitch - fracX;
	}

	return buf;
}

// transmit until endX, don't check size since it has already been prevalidated
template <class T>
static __forceinline const T *TransmitHostLocalX_24(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *buf)
{
	for (u32 tempi = 0; tempi < blockheight; ++tempi)
	{
		for (tempX = startX; tempX < gs.imageEnd.x; tempX++, buf += 3)
		{
			wp(pstart, tempX % 2048, (tempY + tempi) % 2048, *(u32*)buf, gs.dstbuf.bw);
		}

		buf += 3 * (pitch - fracX);
	}

	return buf;
}

// transmit until endX, don't check size since it has already been prevalidated
template <class T>
static __forceinline const T *TransmitHostLocalX_4(_writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *buf)
{
	for (u32 tempi = 0; tempi < blockheight; ++tempi)
	{
		for (tempX = startX; tempX < gs.imageEnd.x; tempX += 2, buf++)
		{
			wp(pstart, tempX % 2048, (tempY + tempi) % 2048, buf[0]&0x0f, gs.dstbuf.bw);
			wp(pstart, (tempX + 1) % 2048, (tempY + tempi) % 2048, buf[0] >> 4, gs.dstbuf.bw);
		}

		buf += (pitch - fracX) / 2;
	}

	return buf;
}

template <class T>
static __forceinline const T *TransmitHostLocalX(u32 psm, _writePixel_0 wp, u32 widthlimit, u32 blockheight, u32 startX, const T *buf)
{
	//ZZLog::WriteLn("TransmitHostLocalX: psm == %s, bimode == 0x%x", psm_name[psm], PSMT_BITMODE(psm));
	switch (PSMT_BITMODE(psm))
	{
		case 1:
			return TransmitHostLocalX_24<T>(wp, widthlimit, blockheight, startX, buf);
		case 4:
			return TransmitHostLocalX_4<T>(wp, widthlimit, blockheight, startX, buf);
		default:
			return TransmitHostLocalX_<T>(wp, widthlimit, blockheight, startX, buf);
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
