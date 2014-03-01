#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellPamf.h"

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavutil\imgutils.h"
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
	attr->memSize = 4 * 1024 * 1024;
	attr->cmdDepth = 16;
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

			if (vdec.has_picture) // hack
			{
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
					Callback cb;
					cb.SetAddr(vdec.cbFunc);
					cb.Handle(vdec.id, CELL_VDEC_MSG_TYPE_SEQDONE, 0, vdec.cbArg);
					cb.Branch(false);
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
							this->size -= FF_INPUT_BUFFER_PADDING_SIZE; // ????????????????????
						}

						~vdecPacket()
						{
							av_free_packet(this);
						}

					} au(task.size);

					if (task.pts || task.dts)
					{
						vdec.pts = task.pts;
						vdec.dts = task.dts;
					}
					au.pts = vdec.pts;
					au.dts = vdec.dts;
					
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

					//vdec.ctx->flags |= CODEC_FLAG_TRUNCATED;
					//vdec.ctx->flags2 |= CODEC_FLAG2_CHUNKS;

					int decode = avcodec_decode_video2(vdec.ctx, vdec.frame, &got_picture, &au);
					if (decode < 0)
					{
						ConLog.Error("vdecDecodeAu: AU decoding error(%d)", decode);
						break;
					}
				
					ConLog.Write("Frame decoded (pts=0x%llx, dts=0x%llx, addr=0x%x, result=0x%x)",
						au.pts, au.dts, task.addr, decode);


					Callback cb;
					cb.SetAddr(vdec.cbFunc);
					cb.Handle(vdec.id, CELL_VDEC_MSG_TYPE_AUDONE, 0, vdec.cbArg);
					cb.Branch(false);

					if (got_picture)
					{
						ConLog.Write("got_picture (%d, vdec: pts=0x%llx, dts=0x%llx)", got_picture, vdec.pts, vdec.dts);

						vdec.pts += 3003;
						vdec.dts += 3003;
						
						/*if (vdec.out_data[0]) av_freep(vdec.out_data[0]);

						int err = av_image_alloc(vdec.out_data, vdec.linesize, vdec.ctx->width, vdec.ctx->height, vdec.ctx->pix_fmt, 1);
						if (err < 0)
						{
							ConLog.Error("vdecDecodeAu: av_image_alloc failed(%d)", err);
							Emu.Pause();
							return;
						}

						vdec.buf_size = err;

						av_image_copy(vdec.out_data, vdec.linesize, (const u8**)(vdec.frame->data), vdec.frame->linesize,
							vdec.ctx->pix_fmt, vdec.ctx->width, vdec.ctx->height);*/
						vdec.buf_size = a128(av_image_get_buffer_size(vdec.ctx->pix_fmt, vdec.ctx->width, vdec.ctx->height, 1));

						vdec.dts = task.dts;
						vdec.pts = task.pts;
						vdec.userdata = task.userData;
						vdec.has_picture = true;

						Callback cb;
						cb.SetAddr(vdec.cbFunc);
						cb.Handle(vdec.id, CELL_VDEC_MSG_TYPE_PICOUT, 0, vdec.cbArg);
						cb.Branch(false);
					}
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
					return;
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
	cellVdec.Warning("cellVdecGetPicture(handle=%d, format_addr=0x%x, out_addr=0x%x)", handle, format.GetAddr(), out_addr);

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	if (!format.IsGood())
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	if (!vdec->has_picture)
	{
		return CELL_VDEC_ERROR_EMPTY;
	}

	if (out_addr)
	{
		if (!Memory.IsGoodAddr(out_addr, vdec->buf_size))
		{
			return CELL_VDEC_ERROR_FATAL;
		}

		if (format->formatType != CELL_VDEC_PICFMT_YUV420_PLANAR)
		{
			cellVdec.Error("cellVdecGetPicture: TODO: unknown formatType(%d)", (u32)format->formatType);
			return CELL_OK;
		}
		if (format->colorMatrixType != CELL_VDEC_COLOR_MATRIX_TYPE_BT709)
		{
			cellVdec.Error("cellVdecGetPicture: TODO: unknown colorMatrixType(%d)", (u32)format->colorMatrixType);
			return CELL_OK;
		}

		AVFrame& frame = *vdec->frame;

		u8* buf = (u8*)malloc(vdec->buf_size);
		if (!buf)
		{
			cellVdec.Error("cellVdecGetPicture: malloc failed (out of memory)");
			Emu.Pause();
			return CELL_OK;
		}

		// TODO: zero padding bytes

		int err = av_image_copy_to_buffer(buf, vdec->buf_size, frame.data, frame.linesize, vdec->ctx->pix_fmt, frame.width, frame.height, 1);
		if (err < 0)
		{
			cellVdec.Error("cellVdecGetPicture: av_image_copy_to_buffer failed(%d)", err);
			Emu.Pause();
		}

		if (!Memory.CopyFromReal(out_addr, buf, vdec->buf_size))
		{
			cellVdec.Error("cellVdecGetPicture: data copying failed");
			Emu.Pause();
		}

		/*
		u32 size0 = frame.linesize[0] * frame.height;
		u32 size1 = frame.linesize[1] * frame.height / 2;
		u32 size2 = frame.linesize[2] * frame.height / 2;
		ConLog.Write("*** size0=0x%x, size1=0x%x, size2=0x%x, buf_size=0x%x (res=0x%x)", size0, size1, size2, vdec->buf_size, err);
		*/

		free(buf);
	}

	vdec->has_picture = false;
	return CELL_OK;
}

int cellVdecGetPicItem(u32 handle, mem32_t picItem_ptr)
{
	cellVdec.Warning("cellVdecGetPicItem(handle=%d, picItem_ptr_addr=0x%x)", handle, picItem_ptr.GetAddr());

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	if (!picItem_ptr.IsGood())
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	if (!vdec->has_picture)
	{
		return CELL_VDEC_ERROR_EMPTY;
	}

	mem_ptr_t<CellVdecPicItem> info(vdec->memAddr);

	info->codecType = vdec->type;
	info->startAddr = 0x00000123; // invalid value (no address for picture)
	info->size = vdec->buf_size;
	info->auNum = 1;
	info->auPts[0].lower = vdec->pts;
	info->auPts[0].upper = vdec->pts >> 32;
	info->auPts[1].lower = 0xffffffff;
	info->auPts[1].upper = 0xffffffff;
	info->auDts[0].lower = vdec->dts;
	info->auDts[0].upper = vdec->dts >> 32;
	info->auDts[1].lower = 0xffffffff;
	info->auDts[1].upper = 0xffffffff;
	info->auUserData[0] = vdec->userdata;
	info->auUserData[1] = 0;
	info->status = CELL_OK;
	info->attr = CELL_VDEC_PICITEM_ATTR_NORMAL;
	info->picInfo_addr = vdec->memAddr + sizeof(CellVdecPicItem);

	mem_ptr_t<CellVdecAvcInfo> avc(vdec->memAddr + sizeof(CellVdecPicItem));

	avc->horizontalSize = vdec->frame->width; // ???
	avc->verticalSize = vdec->frame->height;
	switch (vdec->frame->pict_type)
	{
	case AV_PICTURE_TYPE_I: avc->pictureType[0] = CELL_VDEC_AVC_PCT_I; break;
	case AV_PICTURE_TYPE_P: avc->pictureType[0] = CELL_VDEC_AVC_PCT_P; break;
	case AV_PICTURE_TYPE_B: avc->pictureType[0] = CELL_VDEC_AVC_PCT_B; break;
	default: avc->pictureType[0] = CELL_VDEC_AVC_PCT_UNKNOWN; break; // ???
	}
	avc->pictureType[1] = CELL_VDEC_AVC_PCT_UNKNOWN; // ???
	avc->idrPictureFlag = false; // ???
	avc->aspect_ratio_idc = CELL_VDEC_AVC_ARI_SAR_UNSPECIFIED; // ???
	avc->sar_height = 0;
	avc->sar_width = 0;
	avc->pic_struct = CELL_VDEC_AVC_PSTR_FRAME; // ???
	avc->picOrderCount[0] = 0; // ???
	avc->picOrderCount[1] = 0;
	avc->vui_parameters_present_flag = true; // ???
	avc->frame_mbs_only_flag = true; // ??? progressive
	avc->video_signal_type_present_flag = true; // ???
	avc->video_format = CELL_VDEC_AVC_VF_COMPONENT; // ???
	avc->video_full_range_flag = false; // ???
	avc->colour_description_present_flag = true;
	avc->colour_primaries = CELL_VDEC_AVC_CP_ITU_R_BT_709_5; // ???
	avc->transfer_characteristics = CELL_VDEC_AVC_TC_ITU_R_BT_709_5;
	avc->matrix_coefficients = CELL_VDEC_AVC_MXC_ITU_R_BT_709_5; // important
	avc->timing_info_present_flag = true;
	avc->frameRateCode = CELL_VDEC_AVC_FRC_30000DIV1001; // important (!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!)
	avc->fixed_frame_rate_flag = true;
	avc->low_delay_hrd_flag = true; // ???
	avc->entropy_coding_mode_flag = true; // ???
	avc->nalUnitPresentFlags = 0; // ???
	avc->ccDataLength[0] = 0;
	avc->ccDataLength[1] = 0;
	avc->reserved[0] = 0;
	avc->reserved[1] = 0;
	
	picItem_ptr = info.GetAddr();

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