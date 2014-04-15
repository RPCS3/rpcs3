#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellPamf.h"

SMutexGeneral g_mutex_avcodec_open2;

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
}

#include "cellVdec.h"

void cellVdec_init();
Module cellVdec(0x0005, cellVdec_init);

int vdecRead(void* opaque, u8* buf, int buf_size)
{
	VideoDecoder& vdec = *(VideoDecoder*)opaque;

	int res = 0;

next:
	if (vdec.reader.size < (u32)buf_size /*&& !vdec.just_started*/)
	{
		while (vdec.job.IsEmpty())
		{
			if (Emu.IsStopped())
			{
				ConLog.Warning("vdecRead() aborted");
				return 0;
			}
			Sleep(1);
		}

		switch (vdec.job.Peek().type)
		{
		case vdecEndSeq:
			{
				buf_size = vdec.reader.size;
			}
			break;
		case vdecDecodeAu:
			{
				if (!Memory.CopyToReal(buf, vdec.reader.addr, vdec.reader.size))
				{
					ConLog.Error("vdecRead: data reading failed (reader.size=0x%x)", vdec.reader.size);
					Emu.Pause();
					return 0;
				}

				buf += vdec.reader.size;
				buf_size -= vdec.reader.size;
				res += vdec.reader.size;

				/*Callback cb;
				cb.SetAddr(vdec.cbFunc);
				cb.Handle(vdec.id, CELL_VDEC_MSG_TYPE_AUDONE, CELL_OK, vdec.cbArg);
				cb.Branch(false);*/
				vdec.vdecCb->ExecAsCallback(vdec.cbFunc, false, vdec.id, CELL_VDEC_MSG_TYPE_AUDONE, CELL_OK, vdec.cbArg);

				vdec.job.Pop(vdec.task);

				vdec.reader.addr = vdec.task.addr;
				vdec.reader.size = vdec.task.size;
				//ConLog.Write("Video AU: size = 0x%x, pts = 0x%llx, dts = 0x%llx", vdec.task.size, vdec.task.pts, vdec.task.dts);
			}
			break;
		default:
			ConLog.Error("vdecRead(): sequence error (task %d)", vdec.job.Peek().type);
			return 0;
		}
		
		goto next;
	}
	else if (vdec.reader.size < (u32)buf_size)
	{
		buf_size = vdec.reader.size;
	}

	if (!buf_size)
	{
		return res;
	}
	else if (!Memory.CopyToReal(buf, vdec.reader.addr, buf_size))
	{
		ConLog.Error("vdecRead: data reading failed (buf_size=0x%x)", buf_size);
		Emu.Pause();
		return 0;
	}
	else
	{
		vdec.reader.addr += buf_size;
		vdec.reader.size -= buf_size;
		return res + buf_size;
	}
}

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

	vdec.vdecCb = &Emu.GetCPU().AddThread(CPU_THREAD_PPU);

	u32 vdec_id = cellVdec.GetNewId(data);

	vdec.id = vdec_id;

	vdec.vdecCb->SetName("Video Decoder[" + std::to_string(vdec_id) + "] Callback");

	thread t("Video Decoder[" + std::to_string(vdec_id) + "] Thread", [&]()
	{
		ConLog.Write("Video Decoder enter()");

		VdecTask& task = vdec.task;

		while (true)
		{
			if (Emu.IsStopped())
			{
				break;
			}

			if (vdec.job.IsEmpty() && vdec.is_running)
			{
				Sleep(1);
				continue;
			}

			if (vdec.frames.GetCount() >= 50)
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
					ConLog.Warning("vdecStartSeq:");

					vdec.reader.addr = 0;
					vdec.reader.size = 0;
					vdec.is_running = true;
					vdec.just_started = true;
				}
				break;

			case vdecEndSeq:
				{
					// TODO: finalize
					ConLog.Warning("vdecEndSeq:");

					vdec.vdecCb->ExecAsCallback(vdec.cbFunc, false, vdec.id, CELL_VDEC_MSG_TYPE_SEQDONE, CELL_OK, vdec.cbArg);
					/*Callback cb;
					cb.SetAddr(vdec.cbFunc);
					cb.Handle(vdec.id, CELL_VDEC_MSG_TYPE_SEQDONE, CELL_OK, vdec.cbArg);
					cb.Branch(true); // ???*/

					avcodec_close(vdec.ctx);
					avformat_close_input(&vdec.fmt);

					vdec.is_running = false;
				}
				break;

			case vdecDecodeAu:
				{
					int err;

					if (task.mode != CELL_VDEC_DEC_MODE_NORMAL)
					{
						ConLog.Error("vdecDecodeAu: unsupported decoding mode(%d)", task.mode);
						break;
					}

					vdec.reader.addr = task.addr;
					vdec.reader.size = task.size;
					//ConLog.Write("Video AU: size = 0x%x, pts = 0x%llx, dts = 0x%llx", task.size, task.pts, task.dts);

					if (vdec.just_started)
					{
						vdec.first_pts = task.pts;
						vdec.last_pts = task.pts;
						vdec.first_dts = task.dts;
					}

					struct AVPacketHolder : AVPacket
					{
						AVPacketHolder(u32 size)
						{
							av_init_packet(this);

							if (size)
							{
								data = (u8*)av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);
								memset(data + size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
								this->size = size + FF_INPUT_BUFFER_PADDING_SIZE;
							}
							else
							{
								data = NULL;
								size = 0;
							}
						}

						~AVPacketHolder()
						{
							av_free(data);
							//av_free_packet(this);
						}

					} au(0);

					if (vdec.just_started) // deferred initialization
					{
						err = avformat_open_input(&vdec.fmt, NULL, av_find_input_format("mpeg"), NULL);
						if (err)
						{
							ConLog.Error("vdecDecodeAu: avformat_open_input() failed");
							Emu.Pause();
							break;
						}
						AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264); // ???
						if (!codec)
						{
							ConLog.Error("vdecDecodeAu: avcodec_find_decoder() failed");
							Emu.Pause();
							break;
						}
						/*err = avformat_find_stream_info(vdec.fmt, NULL);
						if (err)
						{
							ConLog.Error("vdecDecodeAu: avformat_find_stream_info() failed");
							Emu.Pause();
							break;
						}
						if (!vdec.fmt->nb_streams)
						{
							ConLog.Error("vdecDecodeAu: no stream found");
							Emu.Pause();
							break;
						}*/
						if (!avformat_new_stream(vdec.fmt, codec))
						{
							ConLog.Error("vdecDecodeAu: avformat_new_stream() failed");
							Emu.Pause();
							break;
						}
						vdec.ctx = vdec.fmt->streams[0]->codec; // TODO: check data
						
						AVDictionary* opts = nullptr;
						av_dict_set(&opts, "refcounted_frames", "1", 0);
						{
							SMutexGeneralLocker lock(g_mutex_avcodec_open2);
							// not multithread-safe
							err = avcodec_open2(vdec.ctx, codec, &opts);
						}
						if (err)
						{
							ConLog.Error("vdecDecodeAu: avcodec_open2() failed");
							Emu.Pause();
							break;
						}
						//vdec.ctx->flags |= CODEC_FLAG_TRUNCATED;
						//vdec.ctx->flags2 |= CODEC_FLAG2_CHUNKS;
						vdec.just_started = false;
					}

					bool last_frame = false;

					while (true)
					{
						if (Emu.IsStopped())
						{
							ConLog.Warning("vdecDecodeAu aborted");
							return;
						}

						last_frame = av_read_frame(vdec.fmt, &au) < 0;
						if (last_frame)
						{
							//break;
							av_free(au.data);
							au.data = NULL;
							au.size = 0;
						}

						struct VdecFrameHolder : VdecFrame
						{
							VdecFrameHolder()
							{
								data = av_frame_alloc();
							}

							~VdecFrameHolder()
							{
								if (data)
								{
									av_frame_unref(data);
									av_frame_free(&data);
								}
							}

						} frame;

						if (!frame.data)
						{
							ConLog.Error("vdecDecodeAu: av_frame_alloc() failed");
							Emu.Pause();
							break;
						}

						int got_picture = 0;

						int decode = avcodec_decode_video2(vdec.ctx, frame.data, &got_picture, &au);

						if (decode <= 0)
						{
							if (!last_frame && decode < 0)
							{
								ConLog.Error("vdecDecodeAu: AU decoding error(0x%x)", decode);
							}
							if (!got_picture && vdec.reader.size == 0) break; // video end?
						}

						if (got_picture)
						{
							u64 ts = av_frame_get_best_effort_timestamp(frame.data);
							if (ts != AV_NOPTS_VALUE)
							{
								frame.pts = ts/* - vdec.first_pts*/; // ???
								vdec.last_pts = frame.pts;
							}
							else
							{
								vdec.last_pts += vdec.ctx->time_base.num * 90000 / (vdec.ctx->time_base.den / vdec.ctx->ticks_per_frame);
								frame.pts = vdec.last_pts;
							}
							//frame.pts = vdec.last_pts;
							//vdec.last_pts += 3754;
							frame.dts = (frame.pts - vdec.first_pts) + vdec.first_dts;
							frame.userdata = task.userData;

							//ConLog.Write("got picture (pts=0x%llx, dts=0x%llx)", frame.pts, frame.dts);	

							vdec.frames.Push(frame); // !!!!!!!!
							frame.data = nullptr; // to prevent destruction

							vdec.vdecCb->ExecAsCallback(vdec.cbFunc, false, vdec.id, CELL_VDEC_MSG_TYPE_PICOUT, CELL_OK, vdec.cbArg);
							/*Callback cb;
							cb.SetAddr(vdec.cbFunc);
							cb.Handle(vdec.id, CELL_VDEC_MSG_TYPE_PICOUT, CELL_OK, vdec.cbArg);
							cb.Branch(false);*/
						}
					}

					vdec.vdecCb->ExecAsCallback(vdec.cbFunc, false, vdec.id, CELL_VDEC_MSG_TYPE_AUDONE, CELL_OK, vdec.cbArg);
					/*Callback cb;
					cb.SetAddr(vdec.cbFunc);
					cb.Handle(vdec.id, CELL_VDEC_MSG_TYPE_AUDONE, CELL_OK, vdec.cbArg);
					cb.Branch(false);*/
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
				break;

			default:
				ConLog.Error("Video Decoder error: unknown task(%d)", task.type);
			}
		}

		vdec.is_finished = true;
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

	while (!vdec->is_finished || !vdec->frames.IsEmpty())
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("cellVdecClose(%d) aborted", handle);
			break;
		}
		Sleep(1);
	}

	if (vdec->vdecCb) Emu.GetCPU().RemoveThread(vdec->vdecCb->GetId());
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
	cellVdec.Warning("cellVdecEndSeq(handle=%d)", handle);

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	/*if (!vdec->job.IsEmpty())
	{
		Sleep(1);
		return CELL_VDEC_ERROR_BUSY; // ???
	}

	if (!vdec->frames.IsEmpty())
	{
		Sleep(1);
		return CELL_VDEC_ERROR_BUSY; // ???
	}*/

	while (!vdec->job.IsEmpty() || !vdec->frames.IsEmpty())
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("cellVdecEndSeq(%d) aborted", handle);
			return CELL_OK;
		}
		Sleep(1);
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
	cellVdec.Log("cellVdecGetPicture(handle=%d, format_addr=0x%x, out_addr=0x%x)", handle, format.GetAddr(), out_addr);

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	if (!format.IsGood())
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	if (vdec->frames.IsEmpty())
	{
		return CELL_VDEC_ERROR_EMPTY;
	}

	if (out_addr)
	{
		u32 buf_size = a128(av_image_get_buffer_size(vdec->ctx->pix_fmt, vdec->ctx->width, vdec->ctx->height, 1));

		if (!Memory.IsGoodAddr(out_addr, buf_size))
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

		VdecFrame vf;

		vdec->frames.Pop(vf);

		AVFrame& frame = *vf.data;

		u8* buf = (u8*)malloc(buf_size);

		// TODO: zero padding bytes

		int err = av_image_copy_to_buffer(buf, buf_size, frame.data, frame.linesize, vdec->ctx->pix_fmt, frame.width, frame.height, 1);
		if (err < 0)
		{
			cellVdec.Error("cellVdecGetPicture: av_image_copy_to_buffer failed(%d)", err);
			Emu.Pause();
		}

		if (!Memory.CopyFromReal(out_addr, buf, buf_size))
		{
			cellVdec.Error("cellVdecGetPicture: data copying failed");
			Emu.Pause();
		}

		av_frame_unref(vf.data);
		av_frame_free(&vf.data);
		free(buf);
	}

	return CELL_OK;
}

int cellVdecGetPicItem(u32 handle, mem32_t picItem_ptr)
{
	cellVdec.Log("cellVdecGetPicItem(handle=%d, picItem_ptr_addr=0x%x)", handle, picItem_ptr.GetAddr());

	VideoDecoder* vdec;
	if (!Emu.GetIdManager().GetIDData(handle, vdec))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	if (!picItem_ptr.IsGood())
	{
		return CELL_VDEC_ERROR_FATAL;
	}

	VdecFrame& vf = vdec->frames.Peek();

	if (vdec->frames.IsEmpty())
	{
		Sleep(1);
		return CELL_VDEC_ERROR_EMPTY;
	}

	AVFrame& frame = *vf.data;

	mem_ptr_t<CellVdecPicItem> info(vdec->memAddr + vdec->memBias);

	vdec->memBias += 512;
	if (vdec->memBias + 512 > vdec->memSize)
	{
		vdec->memBias = 0;
	}

	info->codecType = vdec->type;
	info->startAddr = 0x00000123; // invalid value (no address for picture)
	info->size = a128(av_image_get_buffer_size(vdec->ctx->pix_fmt, vdec->ctx->width, vdec->ctx->height, 1));
	info->auNum = 1;
	info->auPts[0].lower = vf.pts;
	info->auPts[0].upper = vf.pts >> 32;
	info->auPts[1].lower = 0xffffffff;
	info->auPts[1].upper = 0xffffffff;
	info->auDts[0].lower = vf.dts;
	info->auDts[0].upper = vf.dts >> 32;
	info->auDts[1].lower = 0xffffffff;
	info->auDts[1].upper = 0xffffffff;
	info->auUserData[0] = vf.userdata;
	info->auUserData[1] = 0;
	info->status = CELL_OK;
	info->attr = CELL_VDEC_PICITEM_ATTR_NORMAL;
	info->picInfo_addr = info.GetAddr() + sizeof(CellVdecPicItem);

	mem_ptr_t<CellVdecAvcInfo> avc(info.GetAddr() + sizeof(CellVdecPicItem));

	avc->horizontalSize = frame.width;
	avc->verticalSize = frame.height;
	switch (frame.pict_type)
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
	if (vdec->ctx->time_base.num == 1001)
	{
		if (vdec->ctx->time_base.den == 48000 && vdec->ctx->ticks_per_frame == 2)
		{
			avc->frameRateCode = CELL_VDEC_AVC_FRC_24000DIV1001;
		}
		else if (vdec->ctx->time_base.den == 60000 && vdec->ctx->ticks_per_frame == 2)
		{
			avc->frameRateCode = CELL_VDEC_AVC_FRC_30000DIV1001;
		}
		else
		{
			ConLog.Error("cellVdecGetPicItem: unsupported time_base.den (%d)", vdec->ctx->time_base.den);
			Emu.Pause();
		}
	}
	else
	{
		ConLog.Error("cellVdecGetPicItem: unsupported time_base.num (%d)", vdec->ctx->time_base.num);
		Emu.Pause();
	}
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

	av_register_all();
	avcodec_register_all();
}
