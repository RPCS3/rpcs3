#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

extern "C"
{
#include "libswscale/swscale.h"
}

#include "cellVpost.h"

Module *cellVpost = nullptr;

int cellVpostQueryAttr(vm::ptr<const CellVpostCfgParam> cfgParam, vm::ptr<CellVpostAttr> attr)
{
	cellVpost->Warning("cellVpostQueryAttr(cfgParam_addr=0x%x, attr_addr=0x%x)", cfgParam.addr(), attr.addr());

	// TODO: check cfgParam and output values

	attr->delay = 0;
	attr->memSize = 4 * 1024 * 1024; // 4 MB
	attr->vpostVerLower = 0x280000; // from dmux
	attr->vpostVerUpper = 0x260000;

	return CELL_OK;
}

u32 vpostOpen(VpostInstance* data)
{
	u32 id = cellVpost->GetNewId(data);

	cellVpost->Notice("*** Vpost instance created (to_rgba=%d): id = %d", data->to_rgba, id);

	return id;
}

int cellVpostOpen(vm::ptr<const CellVpostCfgParam> cfgParam, vm::ptr<const CellVpostResource> resource, vm::ptr<u32> handle)
{
	cellVpost->Warning("cellVpostOpen(cfgParam_addr=0x%x, resource_addr=0x%x, handle_addr=0x%x)",
		cfgParam.addr(), resource.addr(), handle.addr());

	// TODO: check values
	*handle = vpostOpen(new VpostInstance(cfgParam->outPicFmt == CELL_VPOST_PIC_FMT_OUT_RGBA_ILV));
	return CELL_OK;
}

int cellVpostOpenEx(vm::ptr<const CellVpostCfgParam> cfgParam, vm::ptr<const CellVpostResourceEx> resource, vm::ptr<u32> handle)
{
	cellVpost->Warning("cellVpostOpenEx(cfgParam_addr=0x%x, resource_addr=0x%x, handle_addr=0x%x)",
		cfgParam.addr(), resource.addr(), handle.addr());

	// TODO: check values
	*handle = vpostOpen(new VpostInstance(cfgParam->outPicFmt == CELL_VPOST_PIC_FMT_OUT_RGBA_ILV));
	return CELL_OK;
}

int cellVpostClose(u32 handle)
{
	cellVpost->Warning("cellVpostClose(handle=0x%x)", handle);

	VpostInstance* vpost;
	if (!Emu.GetIdManager().GetIDData(handle, vpost))
	{
		return CELL_VPOST_ERROR_C_ARG_HDL_INVALID;
	}

	Emu.GetIdManager().RemoveID(handle);	
	return CELL_OK;
}

int cellVpostExec(u32 handle, vm::ptr<const u8> inPicBuff, vm::ptr<const CellVpostCtrlParam> ctrlParam,
				  vm::ptr<u8> outPicBuff, vm::ptr<CellVpostPictureInfo> picInfo)
{
	cellVpost->Log("cellVpostExec(handle=0x%x, inPicBuff_addr=0x%x, ctrlParam_addr=0x%x, outPicBuff_addr=0x%x, picInfo_addr=0x%x)",
		handle, inPicBuff.addr(), ctrlParam.addr(), outPicBuff.addr(), picInfo.addr());

	VpostInstance* vpost;
	if (!Emu.GetIdManager().GetIDData(handle, vpost))
	{
		return CELL_VPOST_ERROR_E_ARG_HDL_INVALID;
	}

	s32 w = ctrlParam->inWidth;
	u32 h = ctrlParam->inHeight;
	u32 ow = ctrlParam->outWidth;
	u32 oh = ctrlParam->outHeight;

	ctrlParam->inWindow; // ignored
	if (ctrlParam->inWindow.x) cellVpost->Notice("*** inWindow.x = %d", (u32)ctrlParam->inWindow.x);
	if (ctrlParam->inWindow.y) cellVpost->Notice("*** inWindow.y = %d", (u32)ctrlParam->inWindow.y);
	if (ctrlParam->inWindow.width != w) cellVpost->Notice("*** inWindow.width = %d", (u32)ctrlParam->inWindow.width);
	if (ctrlParam->inWindow.height != h) cellVpost->Notice("*** inWindow.height = %d", (u32)ctrlParam->inWindow.height);
	ctrlParam->outWindow; // ignored
	if (ctrlParam->outWindow.x) cellVpost->Notice("*** outWindow.x = %d", (u32)ctrlParam->outWindow.x);
	if (ctrlParam->outWindow.y) cellVpost->Notice("*** outWindow.y = %d", (u32)ctrlParam->outWindow.y);
	if (ctrlParam->outWindow.width != ow) cellVpost->Notice("*** outWindow.width = %d", (u32)ctrlParam->outWindow.width);
	if (ctrlParam->outWindow.height != oh) cellVpost->Notice("*** outWindow.height = %d", (u32)ctrlParam->outWindow.height);
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

	//u64 stamp0 = get_system_time();
	std::unique_ptr<u8[]> pA(new u8[w*h]);

	memset(pA.get(), (const u8)ctrlParam->outAlpha, w*h);

	//u64 stamp1 = get_system_time();

	SwsContext* sws = sws_getContext(w, h, AV_PIX_FMT_YUVA420P, ow, oh, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);

	//u64 stamp2 = get_system_time();

	const u8* in_data[4] = { &inPicBuff[0], &inPicBuff[w * h], &inPicBuff[w * h * 5 / 4], pA.get() };
	int in_line[4] = { w, w/2, w/2, w };
	u8* out_data[4] = { outPicBuff.get_ptr(), NULL, NULL, NULL };
	int out_line[4] = { static_cast<int>(ow*4), 0, 0, 0 };

	sws_scale(sws, in_data, in_line, 0, h, out_data, out_line);

	//u64 stamp3 = get_system_time();

	sws_freeContext(sws);

	//ConLog.Write("cellVpostExec() perf (access=%d, getContext=%d, scale=%d, finalize=%d)",
		//stamp1 - stamp0, stamp2 - stamp1, stamp3 - stamp2, get_system_time() - stamp3);
	return CELL_OK;
}

void cellVpost_init(Module *pxThis)
{
	cellVpost = pxThis;

	cellVpost->AddFunc(0x95e788c3, cellVpostQueryAttr);
	cellVpost->AddFunc(0xcd33f3e2, cellVpostOpen);
	cellVpost->AddFunc(0x40524325, cellVpostOpenEx);
	cellVpost->AddFunc(0x10ef39f6, cellVpostClose);
	cellVpost->AddFunc(0xabb8cc3d, cellVpostExec);
}
