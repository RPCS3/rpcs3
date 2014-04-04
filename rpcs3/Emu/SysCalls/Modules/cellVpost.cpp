#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

extern "C"
{
#include "libswscale/swscale.h"
}

#include "cellVpost.h"

void cellVpost_init();
Module cellVpost(0x0008, cellVpost_init);

int cellVpostQueryAttr(const mem_ptr_t<CellVpostCfgParam> cfgParam, mem_ptr_t<CellVpostAttr> attr)
{
	cellVpost.Warning("cellVpostQueryAttr(cfgParam_addr=0x%x, attr_addr=0x%x)", cfgParam.GetAddr(), attr.GetAddr());

	if (!cfgParam.IsGood()) return CELL_VPOST_ERROR_Q_ARG_CFG_NULL;
	if (!attr.IsGood()) return CELL_VPOST_ERROR_Q_ARG_ATTR_NULL;

	// TODO: check cfgParam and output values

	attr->delay = 0;
	attr->memSize = 4 * 1024 * 1024;
	attr->vpostVerLower = 0x280000; // from dmux
	attr->vpostVerUpper = 0x260000;

	return CELL_OK;
}

u32 vpostOpen(VpostInstance* data)
{
	u32 id = cellVpost.GetNewId(data);

	ConLog.Write("*** Vpost instance created (to_rgba=%d): id = %d", data->to_rgba, id);

	return id;
}

int cellVpostOpen(const mem_ptr_t<CellVpostCfgParam> cfgParam, const mem_ptr_t<CellVpostResource> resource, mem32_t handle)
{
	cellVpost.Warning("cellVpostOpen(cfgParam_addr=0x%x, resource_addr=0x%x, handle_addr=0x%x)",
		cfgParam.GetAddr(), resource.GetAddr(), handle.GetAddr());

	if (!cfgParam.IsGood()) return CELL_VPOST_ERROR_O_ARG_CFG_NULL;
	if (!resource.IsGood()) return CELL_VPOST_ERROR_O_ARG_RSRC_NULL;
	if (!handle.IsGood()) return CELL_VPOST_ERROR_O_ARG_HDL_NULL;

	// TODO: check values
	handle = vpostOpen(new VpostInstance(cfgParam->outPicFmt == CELL_VPOST_PIC_FMT_OUT_RGBA_ILV));
	return CELL_OK;
}

int cellVpostOpenEx(const mem_ptr_t<CellVpostCfgParam> cfgParam, const mem_ptr_t<CellVpostResourceEx> resource, mem32_t handle)
{
	cellVpost.Warning("cellVpostOpenEx(cfgParam_addr=0x%x, resource_addr=0x%x, handle_addr=0x%x)",
		cfgParam.GetAddr(), resource.GetAddr(), handle.GetAddr());

	if (!cfgParam.IsGood()) return CELL_VPOST_ERROR_O_ARG_CFG_NULL;
	if (!resource.IsGood()) return CELL_VPOST_ERROR_O_ARG_RSRC_NULL;
	if (!handle.IsGood()) return CELL_VPOST_ERROR_O_ARG_HDL_NULL;

	// TODO: check values
	handle = vpostOpen(new VpostInstance(cfgParam->outPicFmt == CELL_VPOST_PIC_FMT_OUT_RGBA_ILV));
	return CELL_OK;
}

int cellVpostClose(u32 handle)
{
	cellVpost.Warning("cellVpostClose(handle=0x%x)", handle);

	VpostInstance* vpost;
	if (!Emu.GetIdManager().GetIDData(handle, vpost))
	{
		return CELL_VPOST_ERROR_C_ARG_HDL_INVALID;
	}

	Emu.GetIdManager().RemoveID(handle);	
	return CELL_OK;
}

int cellVpostExec(u32 handle, const u32 inPicBuff_addr, const mem_ptr_t<CellVpostCtrlParam> ctrlParam,
				  u32 outPicBuff_addr, mem_ptr_t<CellVpostPictureInfo> picInfo)
{
	cellVpost.Log("cellVpostExec(handle=0x%x, inPicBuff_addr=0x%x, ctrlParam_addr=0x%x, outPicBuff_addr=0x%x, picInfo_addr=0x%x)",
		handle, inPicBuff_addr, ctrlParam.GetAddr(), outPicBuff_addr, picInfo.GetAddr());

	VpostInstance* vpost;
	if (!Emu.GetIdManager().GetIDData(handle, vpost))
	{
		return CELL_VPOST_ERROR_E_ARG_HDL_INVALID;
	}

	if (!ctrlParam.IsGood())
	{
		return CELL_VPOST_ERROR_E_ARG_CTRL_INVALID;
	}

	u32 w = ctrlParam->inWidth;
	u32 h = ctrlParam->inHeight;
	u32 ow = ctrlParam->outWidth;
	u32 oh = ctrlParam->outHeight;

	if (!Memory.IsGoodAddr(inPicBuff_addr, w*h*3/2))
	{
		return CELL_VPOST_ERROR_E_ARG_INPICBUF_INVALID;
	}

	if (!Memory.IsGoodAddr(outPicBuff_addr, ow*oh*4))
	{
		return CELL_VPOST_ERROR_E_ARG_OUTPICBUF_INVALID;
	}

	if (!picInfo.IsGood())
	{
		return CELL_VPOST_ERROR_E_ARG_PICINFO_NULL;
	}

	ctrlParam->inWindow; // ignored
	if (ctrlParam->inWindow.x) ConLog.Warning("*** inWindow.x = %d", (u32)ctrlParam->inWindow.x);
	if (ctrlParam->inWindow.y) ConLog.Warning("*** inWindow.y = %d", (u32)ctrlParam->inWindow.y);
	if (ctrlParam->inWindow.width != w) ConLog.Warning("*** inWindow.width = %d", (u32)ctrlParam->inWindow.width);
	if (ctrlParam->inWindow.height != h) ConLog.Warning("*** inWindow.height = %d", (u32)ctrlParam->inWindow.height);
	ctrlParam->outWindow; // ignored
	if (ctrlParam->outWindow.x) ConLog.Warning("*** outWindow.x = %d", (u32)ctrlParam->outWindow.x);
	if (ctrlParam->outWindow.y) ConLog.Warning("*** outWindow.y = %d", (u32)ctrlParam->outWindow.y);
	if (ctrlParam->outWindow.width != ow) ConLog.Warning("*** outWindow.width = %d", (u32)ctrlParam->outWindow.width);
	if (ctrlParam->outWindow.height != oh) ConLog.Warning("*** outWindow.height = %d", (u32)ctrlParam->outWindow.height);
	ctrlParam->execType; // ignored
	ctrlParam->scalerType; // ignored
	ctrlParam->ipcType; // ignored

	picInfo->inWidth = w; // copy
	picInfo->inHeight = h; // copy
	picInfo->inDepth = CELL_VPOST_PIC_DEPTH_8; // fixed
	picInfo->inScanType = CELL_VPOST_SCAN_TYPE_P; // TODO
	picInfo->inPicFmt = CELL_VPOST_PIC_FMT_IN_YUV420_PLANAR; // fixed
	picInfo->inChromaPosType = ctrlParam->inChromaPosType; // copy
	picInfo->inPicStruct = CELL_VPOST_PIC_STRUCT_PFRM; // TODO
	picInfo->inQuantRange = ctrlParam->inQuantRange; // copy
	picInfo->inColorMatrix = ctrlParam->inColorMatrix; // copy

	picInfo->outWidth = ow; // copy
	picInfo->outHeight = oh; // copy
	picInfo->outDepth = CELL_VPOST_PIC_DEPTH_8; // fixed
	picInfo->outScanType = CELL_VPOST_SCAN_TYPE_P; // TODO
	picInfo->outPicFmt = CELL_VPOST_PIC_FMT_OUT_RGBA_ILV; // TODO
	picInfo->outChromaPosType = ctrlParam->inChromaPosType; // ignored
	picInfo->outPicStruct = picInfo->inPicStruct; // ignored
	picInfo->outQuantRange = ctrlParam->inQuantRange; // ignored
	picInfo->outColorMatrix = ctrlParam->inColorMatrix; // ignored

	picInfo->userData = ctrlParam->userData; // copy
	picInfo->reserved1 = 0;
	picInfo->reserved2 = 0;

	u64 stamp0 = get_system_time();
	
	u8* pY = (u8*)malloc(w*h); // color planes
	u8* pU = (u8*)malloc(w*h/4);
	u8* pV = (u8*)malloc(w*h/4);
	u8* pA = (u8*)malloc(w*h);
	u32* res = (u32*)malloc(ow*oh*4); // RGBA interleaved output
	const u8 alpha = ctrlParam->outAlpha;

	if (!Memory.CopyToReal(pY, inPicBuff_addr, w*h))
	{
		cellVpost.Error("cellVpostExec: data copying failed(pY)");
		Emu.Pause();
	}

	if (!Memory.CopyToReal(pU, inPicBuff_addr + w*h, w*h/4))
	{
		cellVpost.Error("cellVpostExec: data copying failed(pU)");
		Emu.Pause();
	}

	if (!Memory.CopyToReal(pV, inPicBuff_addr + w*h + w*h/4, w*h/4))
	{
		cellVpost.Error("cellVpostExec: data copying failed(pV)");
		Emu.Pause();
	}

	memset(pA, alpha, w*h);

	u64 stamp1 = get_system_time();

	SwsContext* sws = sws_getContext(w, h, AV_PIX_FMT_YUVA420P, ow, oh, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);

	u64 stamp2 = get_system_time();

	u8* in_data[4] = { pY, pU, pV, pA };
	int in_line[4] = { w, w/2, w/2, w };
	u8* out_data[4] = { (u8*)res, NULL, NULL, NULL };
	int out_line[4] = { ow*4, 0, 0, 0 };

	sws_scale(sws, in_data, in_line, 0, h, out_data, out_line);

	sws_freeContext(sws);

	u64 stamp3 = get_system_time();

	if (!Memory.CopyFromReal(outPicBuff_addr, res, ow*oh*4))
	{
		cellVpost.Error("cellVpostExec: data copying failed(result)");
		Emu.Pause();
	}

	free(pY);
	free(pU);
	free(pV);
	free(pA);
	free(res);

	//ConLog.Write("cellVpostExec() perf (access=%d, getContext=%d, scale=%d, finalize=%d)",
		//stamp1 - stamp0, stamp2 - stamp1, stamp3 - stamp2, get_system_time() - stamp3);
	return CELL_OK;
}

void cellVpost_init()
{
	cellVpost.AddFunc(0x95e788c3, cellVpostQueryAttr);
	cellVpost.AddFunc(0xcd33f3e2, cellVpostOpen);
	cellVpost.AddFunc(0x40524325, cellVpostOpenEx);
	cellVpost.AddFunc(0x10ef39f6, cellVpostClose);
	cellVpost.AddFunc(0xabb8cc3d, cellVpostExec);
}