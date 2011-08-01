/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include "PrecompiledHeader.h"
#include "Common.h"
#include "Gif.h"
#include "Gif_Unit.h"

#define GIF_PARSE DevCon.WriteLn

static const char GifTag_ModeStr[4][16] = {
	"Packed", "Reglist", "Image", "Image2"
};

static const char GifTag_RegStr[16][16] = {
	"PRIM",    "RGBA",    "STQ",    "UV",
	"XYZF2",   "XYZ2",    "TEX0_1", "TEX0_2",
	"CLAMP_1", "CLAMP_2", "FOG",    "INVALID",
	"XYZF3",   "XYZ3",    "A+D",    "NOP"
};

void Gif_ParsePacket(u8* data, u32 size, GIF_PATH path) {
	Gif_Tag gifTag;
	u8* buffer = data;
	u32 offset = 0;
	GIF_PARSE("Path %d Transfer", path+1);
	for(;;) {
		if (!gifTag.isValid) { // Need new Gif Tag
			if (offset + 16 > size) return;

			gifTag.setTag(&buffer[offset], 1);
			
			GIF_PARSE("--Gif Tag [mode=%s][pre=%d][prim=%d][nregs=%d][nloop=%d][qwc=%d][EOP=%d]",
				GifTag_ModeStr[gifTag.tag.FLG], gifTag.tag.PRE, gifTag.tag.PRIM,
				gifTag.nRegs, gifTag.nLoop, gifTag.len/16, gifTag.tag.EOP);

			if (offset + 16 + gifTag.len > size) return;
			offset += 16;
		}

		switch(gifTag.tag.FLG) {
			case GIF_FLG_PACKED:
				for(u32 i = 0; i < gifTag.tag.NLOOP; i++) {
				for(u32 j = 0; j < gifTag.nRegs;     j++) {
					if (gifTag.regs[j] == GIF_REG_A_D) {
						GIF_PARSE("----[Reg=A+D(0x%x)][nreg=%d][nloop=%d]",
							buffer[offset+8], j, i);
					}
					else {
						GIF_PARSE("----[Reg=%s][nreg=%d][nloop=%d]",
							GifTag_RegStr[gifTag.regs[j]&0xf], j, i);
					}
					offset += 16; // 1 QWC
				}}
				break;
			case GIF_FLG_REGLIST:
				for(u32 j = 0; j < gifTag.nRegs; j++) {
					GIF_PARSE("----[Reg=%s][nreg=%d]", GifTag_RegStr[gifTag.regs[j]&0xf], j);
				}
				offset += gifTag.len; // Data length
				break;
			case GIF_FLG_IMAGE:
			case GIF_FLG_IMAGE2:
				offset += gifTag.len; // Data length
				break;
			jNO_DEFAULT;
		}

		// Reload gif tag next loop
		gifTag.isValid = false;
	}
}

void Gif_ParsePacket(GS_Packet& gsPack, GIF_PATH path) {
	Gif_ParsePacket(&gifUnit.gifPath[path].buffer[gsPack.offset], gsPack.size, path);
}
