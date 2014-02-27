#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellPamf.h"

extern "C"
{
#include "libavcodec\avcodec.h"
}

#include "cellVdec.h"

void cellVdec_init();
Module cellVdec(0x0005, cellVdec_init);

u32 vdecQueryAttr(CellVdecCodecType type, u32 profile, u32 spec_addr /* may be 0 */, mem_ptr_t<CellVdecAttr> attr)
{
	switch (type) // TODO: check profile levels
	{
	case CELL_VDEC_CODEC_TYPE_AVC: cellVdec.Warning("cellVdecQueryAttr: AVC (profile=%d)", profile); break;
	case CELL_VDEC_CODEC_TYPE_MPEG2: cellVdec.Error("TODO: MPEG2 not supported"); break;
	case CELL_VDEC_CODEC_TYPE_DIVX: cellVdec.Error("TODO: DIVX not supported"); break;
	default: return CELL_VDEC_ERROR_ARG;
	}

	// TODO: check values
	attr->decoderVerLower = 0x280000; // from dmux
	attr->decoderVerUpper = 0x260000;
	attr->memSize = 64 * 1024 * 1024;
	attr->cmdDepth = 15;
	return CELL_OK;
}

u32 vdecOpen(VideoDecoder* data)
{
	VideoDecoder& vdec = *data;

	u32 vdec_id = cellVdec.GetNewId(data);

	vdec.id = vdec_id;

	thread t("Video Decoder[" + std::to_string(vdec_id) + "] Thread", [&]()
	{
		ConLog.Write("Video Decoder enter()");

		VdecTask task;

		while (true)
		{
			if (Emu.IsStopped())
			{
				break;
			}

			if (vdec.job.IsEmpty() && vdec.is_running)
			{
				// TODO: default task (not needed?)
				Sleep(1);
				continue;
			}

			if (!vdec.job.Pop(task))
			{
				break;
			}

			switch (task.type)
			{
			case vdecStartSeq:
				{
					// TODO: reset data
					ConLog.Warning("vdecStartSeq()");
					vdec.is_running = true;
				}
				break;

			case vdecEndSeq:
				{
					// TODO: send callback
					ConLog.Warning("vdecEndSeq()");
					vdec.is_running = false;
				}
				break;

			case vdecDecodeAu:
				{
					struct vdecPacket : AVPacket
					{
						vdecPacket(u32 size)
						{
							av_new_packet(this, size + FF_INPUT_BUFFER_PADDING_SIZE);
							memset(data + size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
						}

						~vdecPacket()
						{
							av_free_packet(this);
						}

					} au(task.size);

					au.pts = task.pts;
					au.dts = task.dts;
					
					if (task.mode != CELL_VDEC_DEC_MODE_NORMAL)
					{
						ConLog.Error("vdecDecodeAu: unsupported decoding mode(%d)", task.mode);
						break;
					}

					if (!Memory.CopyToReal(au.data, task.addr, task.size))
					{
						ConLog.Error("vdecDecodeAu: AU data accessing failed(addr=0x%x, size=0x%x)", task.addr, task.size);
						break;
					}

					int got_picture = 0;

					int decode = avcodec_decode_video2(vdec.ctx, vdec.frame, &got_picture, &au);
					if (decode < 0)
					{
						ConLog.Error("vdecDecodeAu: AU decoding error(%d)", decode);
						break;
					}
				
					ConLog.Write("Frame decoded (%d)", decode);
				}
				break;

			case vdecClose:
				{
					vdec.is_finished = true;
					ConLog.Write("Video Decoder exit");
					return;
				}

			case vdecSetFrameRate:
				{
					ConLog.Error("TODO: vdecSetFrameRate(%d)", task.frc);
				}

			default:
				ConLog.Error("Video Decoder error: unknown task(%d)", task.type);
				return;
			}
		}

		ConLog.Warning("Video Decoder aborted");
	});

	t.detach();

	return vdec_id;
}

int cellVdecQueryAttr(const mem_ptr_t<CellVdecType> type, mem_ptr_t<CellVdecAttr> attr)
{
	cellVdec.Warning("cellVdecQueryAttr(type_addr=0x%x, attr_addr=0x%x)", type.GetAddr(), attr.GetAddr());

	if (!type.IsGood() || !attr.IsGood())
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	return vdecQueryAttr(type->codecType, type->profileLevel, 0, attr);
}

int cellVdecQueryAttrEx(const mem_ptr_t<CellVdecTypeEx> type, mem_ptr_t<CellVdecAttr> attr)
{
	cellVdec.Warning("cellVdecQueryAttrEx(type_addr=0x%x, attr_addr=0x%x)", type.GetAddr(), attr.GetAddr());

	if (!type.IsGood() || !attr.IsGood())
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	return vdecQueryAttr(type->codecType, type->profileLevel, type->codecSpecificInfo_addr, attr);
}

int cellVdecOpen(const mem_ptr_t<CellVdecType> type, const mem_ptr_t<CellVdecResource> res, const mem_ptr_t<CellVdecCb> cb, mem32_t handle)
{
	cellVdec.Warning("cellVdecOpen(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)",
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());

	if (!type.IsGood() || !res.IsGood() || !cb.IsGood() || !handle.IsGood())
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(res->memAddr, res->memSize) || !Memory.IsGoodAddr(cb->cbFunc))
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	handle = vdecOpen(new VideoDecoder(type->codecType, type->profileLevel, res->memAddr, res->memSize, cb->cbFunc, cb->cbArg));

	return CELL_OK;
}

int cellVdecOpenEx(const mem_ptr_t<CellVdecTypeEx> type, const mem_ptr_t<CellVdecResourceEx> res, const mem_ptr_t<CellVdecCb> cb, mem32_t handle)
{
	cellVdec.Warning("cellVdecOpenEx(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)",
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());

	if (!type.IsGood() || !res.IsGood() || !cb.IsGood() || !handle.IsGood())
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(res->memAddr, res->memSize) || !Memory.IsGoodAddr(cb->cbFunc))
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	handle = vdecOpen(new VideoDecoder(type->codecType, type->profileLevel, res->memAddr, res->memSize, cb->cbFunc, cb->cbArg));

	return CELL_OK;
}

int cellVdecClose(u32 handle)
{
	cellVdec.Warning("cellVdecClose(handle=%d)", handle);

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	vdec->job.Push(VdecTask(vdecClose));

	while (!vdec->is_finished)
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("cellVdecClose(%d) aborted", handle);
			break;
		}
		Sleep(1);
	}

	Emu.GetIdManager().RemoveID(handle);
	return CELL_OK;
}

int cellVdecStartSeq(u32 handle)
{
	cellVdec.Log("cellVdecStartSeq(handle=%d)", handle);

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	vdec->job.Push(VdecTask(vdecStartSeq));
	return CELL_OK;
}

int cellVdecEndSeq(u32 handle)
{
	cellVdec.Log("cellVdecEndSeq(handle=%d)", handle);

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	vdec->job.Push(VdecTask(vdecEndSeq));
	return CELL_OK;
}

int cellVdecDecodeAu(u32 handle, CellVdecDecodeMode mode, const mem_ptr_t<CellVdecAuInfo> auInfo)
{
	cellVdec.Log("cellVdecDecodeAu(handle=%d, mode=0x%x, auInfo_addr=0x%x)", handle, mode, auInfo.GetAddr());

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	// TODO: check info
	VdecTask task(vdecDecodeAu);
	task.mode = mode;
	task.addr = auInfo->startAddr;
	task.size = auInfo->size;
	task.dts = (u64)auInfo->dts.lower | ((u64)auInfo->dts.upper << 32);
	task.pts = (u64)auInfo->pts.lower | ((u64)auInfo->pts.upper << 32);
	task.userData = auInfo->userData;
	task.specData = auInfo->codecSpecificData;

	vdec->job.Push(task);
	return CELL_OK;
}

int cellVdecGetPicture(u32 handle, const mem_ptr_t<CellVdecPicFormat> format, u32 out_addr)
{
	cellVdec.Error("cellVdecGetPicture(handle=%d, format_addr=0x%x, out_addr=0x%x)", handle, format.GetAddr(), out_addr);
	return CELL_OK;
}

int cellVdecGetPicItem(u32 handle, const u32 picItem_ptr_addr)
{
	cellVdec.Error("cellVdecGetPicItem(handle=%d, picItem_ptr_addr=0x%x)", handle, picItem_ptr_addr);
	return CELL_OK;
}

int cellVdecSetFrameRate(u32 handle, CellVdecFrameRate frc)
{
	cellVdec.Log("cellVdecSetFrameRate(handle=%d, frc=0x%x)", handle, frc);

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	// TODO: check frc value and set frame rate
	VdecTask task(vdecSetFrameRate);
	task.frc = frc;

	vdec->job.Push(task);
	return CELL_OK;
}

void cellVdec_init()
{
	cellVdec.AddFunc(0xff6f6ebe, cellVdecQueryAttr);
	cellVdec.AddFunc(0xc982a84a, cellVdecQueryAttrEx);
	cellVdec.AddFunc(0xb6bbcd5d, cellVdecOpen);
	cellVdec.AddFunc(0x0053e2d8, cellVdecOpenEx);
	cellVdec.AddFunc(0x16698e83, cellVdecClose);
	cellVdec.AddFunc(0xc757c2aa, cellVdecStartSeq);
	cellVdec.AddFunc(0x824433f0, cellVdecEndSeq);
	cellVdec.AddFunc(0x2bf4ddd2, cellVdecDecodeAu);
	cellVdec.AddFunc(0x807c861a, cellVdecGetPicture);
	cellVdec.AddFunc(0x17c702b9, cellVdecGetPicItem);
	cellVdec.AddFunc(0xe13ef6fc, cellVdecSetFrameRate);

	avcodec_register_all();
}