#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"

SysCallBase sc_resc("cellResc");

struct CellRescSrc
{
	u32	format;
	u32	pitch;
	u16	width;
	u16	height;
	u32	offset;
};

int cellRescSetSrc(const int idx, const u32 src_addr)
{
	sc_resc.Warning("cellRescSetSrc(idx=0x%x, src_addr=0x%x)", idx, src_addr);
	const CellRescSrc& src = *(CellRescSrc*)Memory.GetMemFromAddr(src_addr);
	sc_resc.Warning(" *** format=%d", src.format);
	sc_resc.Warning(" *** pitch=%d", src.pitch);
	sc_resc.Warning(" *** width=%d", src.width);
	sc_resc.Warning(" *** height=%d", src.height);
	sc_resc.Warning(" *** offset=0x%x", src.offset);
	//Emu.GetGSManager().GetRender().SetData(src.offset, 800, 600);
	//Emu.GetGSManager().GetRender().Draw();
	return 0;
}

int cellRescSetBufferAddress(const u32 colorBuffers_addr, const u32 vertexArray_addr, const u32 fragmentShader_addr)
{
	sc_resc.Warning("cellRescSetBufferAddress(colorBuffers_addr=0x%x, vertexArray_addr=0x%x, fragmentShader_add=0x%x)",
		colorBuffers_addr, vertexArray_addr, fragmentShader_addr);
	return 0;
}