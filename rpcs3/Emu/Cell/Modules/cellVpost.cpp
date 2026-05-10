#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
extern "C"
{
#include "libswscale/swscale.h"
}
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#include "cellVpost.h"

LOG_CHANNEL(cellVpost);

template<>
void fmt_class_string<CellVpostError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_VPOST_ERROR_Q_ARG_CFG_NULL);
			STR_CASE(CELL_VPOST_ERROR_Q_ARG_CFG_INVALID);
			STR_CASE(CELL_VPOST_ERROR_Q_ARG_ATTR_NULL);
			STR_CASE(CELL_VPOST_ERROR_O_ARG_CFG_NULL);
			STR_CASE(CELL_VPOST_ERROR_O_ARG_CFG_INVALID);
			STR_CASE(CELL_VPOST_ERROR_O_ARG_RSRC_NULL);
			STR_CASE(CELL_VPOST_ERROR_O_ARG_RSRC_INVALID);
			STR_CASE(CELL_VPOST_ERROR_O_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_ERROR_O_FATAL_QUERY_FAIL);
			STR_CASE(CELL_VPOST_ERROR_O_FATAL_CREATEMON_FAIL);
			STR_CASE(CELL_VPOST_ERROR_O_FATAL_INITSPURS_FAIL);
			STR_CASE(CELL_VPOST_ERROR_C_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_ERROR_C_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_ERROR_C_FATAL_LOCKMON_FAIL);
			STR_CASE(CELL_VPOST_ERROR_C_FATAL_UNLOCKMON_FAIL);
			STR_CASE(CELL_VPOST_ERROR_C_FATAL_DESTROYMON_FAIL);
			STR_CASE(CELL_VPOST_ERROR_C_FATAL_FINSPURS_FAIL);
			STR_CASE(CELL_VPOST_ERROR_E_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_ERROR_E_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_ERROR_E_ARG_INPICBUF_NULL);
			STR_CASE(CELL_VPOST_ERROR_E_ARG_INPICBUF_INVALID);
			STR_CASE(CELL_VPOST_ERROR_E_ARG_CTRL_NULL);
			STR_CASE(CELL_VPOST_ERROR_E_ARG_CTRL_INVALID);
			STR_CASE(CELL_VPOST_ERROR_E_ARG_OUTPICBUF_NULL);
			STR_CASE(CELL_VPOST_ERROR_E_ARG_OUTPICBUF_INVALID);
			STR_CASE(CELL_VPOST_ERROR_E_ARG_PICINFO_NULL);
			STR_CASE(CELL_VPOST_ERROR_E_FATAL_LOCKMON_FAIL);
			STR_CASE(CELL_VPOST_ERROR_E_FATAL_UNLOCKMON_FAIL);
			STR_CASE(CELL_VPOST_ENT_ERROR_Q_ARG_ATTR_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_O_ARG_RSRC_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_O_ARG_RSRC_INVALID);
			STR_CASE(CELL_VPOST_ENT_ERROR_O_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_O_FATAL_QUERY_FAIL);
			STR_CASE(CELL_VPOST_ENT_ERROR_O_FATAL_CSPUCORE_FAIL);
			STR_CASE(CELL_VPOST_ENT_ERROR_C_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_C_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_ENT_ERROR_C_FATAL_SNDCMD_FAIL);
			STR_CASE(CELL_VPOST_ENT_ERROR_C_FATAL_RCVRES_FAIL);
			STR_CASE(CELL_VPOST_ENT_ERROR_C_FATAL_DSPUCORE_FAIL);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_INPICBUF_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_INPICBUF_INVALID);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_INPICINFO_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_INPICINFO_INVALID);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_CTRL_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_CTRL_INVALID);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_COMB_INVALID);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_OUTPICBUF_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_OUTPICBUF_INVALID);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_ARG_OUTPICINFO_NULL);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_FATAL_SNDCMD_FAIL);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_FATAL_RCVRES_FAIL);
			STR_CASE(CELL_VPOST_ENT_ERROR_E_FATAL_SPUCORE_FAIL);
			STR_CASE(CELL_VPOST_IPC_ERROR_Q_ARG_ATTR_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_O_ARG_RSRC_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_O_ARG_RSRC_INVALID);
			STR_CASE(CELL_VPOST_IPC_ERROR_O_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_O_FATAL_QUERY_FAIL);
			STR_CASE(CELL_VPOST_IPC_ERROR_O_FATAL_CSPUCORE_FAIL);
			STR_CASE(CELL_VPOST_IPC_ERROR_C_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_C_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_IPC_ERROR_C_FATAL_SNDCMD_FAIL);
			STR_CASE(CELL_VPOST_IPC_ERROR_C_FATAL_RCVRES_FAIL);
			STR_CASE(CELL_VPOST_IPC_ERROR_C_FATAL_DSPUCORE_FAIL);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_INPICBUF_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_INPICBUF_INVALID);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_INPICINFO_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_INPICINFO_INVALID);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_CTRL_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_CTRL_INVALID);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_COMB_INVALID);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_OUTPICBUF_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_OUTPICBUF_INVALID);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_ARG_OUTPICINFO_NULL);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_FATAL_SNDCMD_FAIL);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_FATAL_RCVRES_FAIL);
			STR_CASE(CELL_VPOST_IPC_ERROR_E_FATAL_SPUCORE_FAIL);
			STR_CASE(CELL_VPOST_VSC_ERROR_Q_ARG_ATTR_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_O_ARG_RSRC_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_O_ARG_RSRC_INVALID);
			STR_CASE(CELL_VPOST_VSC_ERROR_O_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_O_FATAL_QUERY_FAIL);
			STR_CASE(CELL_VPOST_VSC_ERROR_O_FATAL_CSPUCORE_FAIL);
			STR_CASE(CELL_VPOST_VSC_ERROR_C_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_C_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_VSC_ERROR_C_FATAL_SNDCMD_FAIL);
			STR_CASE(CELL_VPOST_VSC_ERROR_C_FATAL_RCVRES_FAIL);
			STR_CASE(CELL_VPOST_VSC_ERROR_C_FATAL_DSPUCORE_FAIL);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_INPICBUF_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_INPICBUF_INVALID);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_INPICINFO_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_INPICINFO_INVALID);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_CTRL_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_CTRL_INVALID);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_COMB_INVALID);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_OUTPICBUF_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_OUTPICBUF_INVALID);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_ARG_OUTPICINFO_NULL);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_FATAL_SNDCMD_FAIL);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_FATAL_RCVRES_FAIL);
			STR_CASE(CELL_VPOST_VSC_ERROR_E_FATAL_SPUCORE_FAIL);
			STR_CASE(CELL_VPOST_CSC_ERROR_Q_ARG_ATTR_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_O_ARG_RSRC_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_O_ARG_RSRC_INVALID);
			STR_CASE(CELL_VPOST_CSC_ERROR_O_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_O_FATAL_QUERY_FAIL);
			STR_CASE(CELL_VPOST_CSC_ERROR_O_FATAL_CSPUCORE_FAIL);
			STR_CASE(CELL_VPOST_CSC_ERROR_C_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_C_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_CSC_ERROR_C_FATAL_SNDCMD_FAIL);
			STR_CASE(CELL_VPOST_CSC_ERROR_C_FATAL_RCVRES_FAIL);
			STR_CASE(CELL_VPOST_CSC_ERROR_C_FATAL_DSPUCORE_FAIL);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_HDL_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_HDL_INVALID);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_INPICBUF_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_INPICBUF_INVALID);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_INPICINFO_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_INPICINFO_INVALID);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_CTRL_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_CTRL_INVALID);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_COMB_INVALID);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_OUTPICBUF_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_OUTPICBUF_INVALID);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_ARG_OUTPICINFO_NULL);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_FATAL_SNDCMD_FAIL);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_FATAL_RCVRES_FAIL);
			STR_CASE(CELL_VPOST_CSC_ERROR_E_FATAL_SPUCORE_FAIL);
		}

		return unknown;
	});
}

error_code cellVpostQueryAttr(vm::cptr<CellVpostCfgParam> cfgParam, vm::ptr<CellVpostAttr> attr)
{
	cellVpost.warning("cellVpostQueryAttr(cfgParam=*0x%x, attr=*0x%x)", cfgParam, attr);

	// TODO: check cfgParam and output values

	attr->delay = 0;
	attr->memSize = 4 * 1024 * 1024; // 4 MB
	attr->vpostVerLower = 0x280000; // from dmux
	attr->vpostVerUpper = 0x260000;

	return CELL_OK;
}

error_code cellVpostOpen(vm::cptr<CellVpostCfgParam> cfgParam, vm::cptr<CellVpostResource> resource, vm::ptr<u32> handle)
{
	cellVpost.warning("cellVpostOpen(cfgParam=*0x%x, resource=*0x%x, handle=*0x%x)", cfgParam, resource, handle);

	// TODO: check values
	*handle = idm::make<VpostInstance>(cfgParam->outPicFmt == CELL_VPOST_PIC_FMT_OUT_RGBA_ILV);
	return CELL_OK;
}

error_code cellVpostOpenEx(vm::cptr<CellVpostCfgParam> cfgParam, vm::cptr<CellVpostResourceEx> resource, vm::ptr<u32> handle)
{
	cellVpost.warning("cellVpostOpenEx(cfgParam=*0x%x, resource=*0x%x, handle=*0x%x)", cfgParam, resource, handle);

	// TODO: check values
	*handle = idm::make<VpostInstance>(cfgParam->outPicFmt == CELL_VPOST_PIC_FMT_OUT_RGBA_ILV);
	return CELL_OK;
}

error_code cellVpostClose(u32 handle)
{
	cellVpost.warning("cellVpostClose(handle=0x%x)", handle);

	const auto vpost = idm::get_unlocked<VpostInstance>(handle);

	if (!vpost)
	{
		return CELL_VPOST_ERROR_C_ARG_HDL_INVALID;
	}

	idm::remove<VpostInstance>(handle);
	return CELL_OK;
}

error_code cellVpostExec(u32 handle, vm::cptr<u8> inPicBuff, vm::cptr<CellVpostCtrlParam> ctrlParam, vm::ptr<u8> outPicBuff, vm::ptr<CellVpostPictureInfo> picInfo)
{
	cellVpost.trace("cellVpostExec(handle=0x%x, inPicBuff=*0x%x, ctrlParam=*0x%x, outPicBuff=*0x%x, picInfo=*0x%x)", handle, inPicBuff, ctrlParam, outPicBuff, picInfo);

	const auto vpost = idm::get_unlocked<VpostInstance>(handle);

	if (!vpost)
	{
		return CELL_VPOST_ERROR_E_ARG_HDL_INVALID;
	}

	u32 w = ctrlParam->inWidth;
	u32 h = ctrlParam->inHeight;
	u32 ow = ctrlParam->outWidth;
	u32 oh = ctrlParam->outHeight;

	//ctrlParam->inWindow; // ignored
	if (ctrlParam->inWindow.x) cellVpost.notice("*** inWindow.x = %d", ctrlParam->inWindow.x);
	if (ctrlParam->inWindow.y) cellVpost.notice("*** inWindow.y = %d", ctrlParam->inWindow.y);
	if (ctrlParam->inWindow.width != w) cellVpost.notice("*** inWindow.width = %d", ctrlParam->inWindow.width);
	if (ctrlParam->inWindow.height != h) cellVpost.notice("*** inWindow.height = %d", ctrlParam->inWindow.height);
	//ctrlParam->outWindow; // ignored
	if (ctrlParam->outWindow.x) cellVpost.notice("*** outWindow.x = %d", ctrlParam->outWindow.x);
	if (ctrlParam->outWindow.y) cellVpost.notice("*** outWindow.y = %d", ctrlParam->outWindow.y);
	if (ctrlParam->outWindow.width != ow) cellVpost.notice("*** outWindow.width = %d", ctrlParam->outWindow.width);
	if (ctrlParam->outWindow.height != oh) cellVpost.notice("*** outWindow.height = %d", ctrlParam->outWindow.height);
	//ctrlParam->execType; // ignored
	//ctrlParam->scalerType; // ignored
	//ctrlParam->ipcType; // ignored

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

	//u64 stamp0 = get_guest_system_time();
	std::unique_ptr<u8[]> pA(new u8[w*h]);

	memset(pA.get(), ctrlParam->outAlpha, w*h);

	//u64 stamp1 = get_guest_system_time();

	vpost->sws = sws_getCachedContext(vpost->sws, w, h, AV_PIX_FMT_YUVA420P, ow, oh, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);

	//u64 stamp2 = get_guest_system_time();

	const u8* in_data[4] = { &inPicBuff[0], &inPicBuff[w * h], &inPicBuff[w * h * 5 / 4], pA.get() };
	int ws = w;
	int in_line[4] = { ws, ws/2, ws/2, ws };
	u8* out_data[4] = { outPicBuff.get_ptr(), nullptr, nullptr, nullptr };
	int out_line[4] = { static_cast<int>(ow * 4), 0, 0, 0 };

	sws_scale(vpost->sws, in_data, in_line, 0, h, out_data, out_line);

	//ConLog.Write("cellVpostExec() perf (access=%d, getContext=%d, scale=%d, finalize=%d)",
		//stamp1 - stamp0, stamp2 - stamp1, stamp3 - stamp2, get_guest_system_time() - stamp3);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellVpost)("cellVpost", []()
{
	REG_FUNC(cellVpost, cellVpostQueryAttr);
	REG_FUNC(cellVpost, cellVpostOpen);
	REG_FUNC(cellVpost, cellVpostOpenEx);
	//REG_FUNC(cellVpost, cellVpostOpenExt); // 0x9f1795df
	REG_FUNC(cellVpost, cellVpostClose);
	REG_FUNC(cellVpost, cellVpostExec);
});
