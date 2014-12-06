#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"

extern std::mutex g_mutex_avcodec_open2;

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

#include "Emu/CPU/CPUThreadManager.h"
#include "cellPamf.h"
#include "cellAdec.h"

Module *cellAdec = nullptr;

AudioDecoder::AudioDecoder(AudioCodecType type, u32 addr, u32 size, vm::ptr<CellAdecCbMsg> func, u32 arg)
	: type(type)
	, memAddr(addr)
	, memSize(size)
	, memBias(0)
	, cbFunc(func)
	, cbArg(arg)
	, adecCb(nullptr)
	, is_closed(false)
	, is_finished(false)
	, just_started(false)
	, just_finished(false)
	, ctx(nullptr)
	, fmt(nullptr)
{
	av_register_all();
	avcodec_register_all();

	AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_ATRAC3P);
	if (!codec)
	{
		cellAdec->Error("AudioDecoder(): avcodec_find_decoder(ATRAC3P) failed");
		Emu.Pause();
		return;
	}
	fmt = avformat_alloc_context();
	if (!fmt)
	{
		cellAdec->Error("AudioDecoder(): avformat_alloc_context failed");
		Emu.Pause();
		return;
	}
	io_buf = (u8*)av_malloc(4096);
	fmt->pb = avio_alloc_context(io_buf, 256, 0, this, adecRead, NULL, NULL);
	if (!fmt->pb)
	{
		cellAdec->Error("AudioDecoder(): avio_alloc_context failed");
		Emu.Pause();
		return;
	}
}

AudioDecoder::~AudioDecoder()
{
	// TODO: check finalization
	AdecFrame af;
	while (frames.Pop(af, &sq_no_wait))
	{
		av_frame_unref(af.data);
		av_frame_free(&af.data);
	}
	if (ctx)
	{
		avcodec_close(ctx);
		avformat_close_input(&fmt);
	}
	if (fmt)
	{
		if (io_buf)
		{
			av_free(io_buf);
		}
		if (fmt->pb) av_free(fmt->pb);
		avformat_free_context(fmt);
	}
}

int adecRead(void* opaque, u8* buf, int buf_size)
{
	AudioDecoder& adec = *(AudioDecoder*)opaque;

	int res = 0;

next:
	if (adec.reader.has_ats)
	{
		u8 code1 = vm::read8(adec.reader.addr + 2);
		u8 code2 = vm::read8(adec.reader.addr + 3);
		adec.channels = (code1 >> 2) & 0x7;
		adec.frame_size = ((((u32)code1 & 0x3) << 8) | (u32)code2) * 8 + 8;
		adec.sample_rate = at3freq[code1 >> 5];

		adec.reader.size -= 8;
		adec.reader.addr += 8;
		adec.reader.has_ats = false;
	}

	if (!adec.reader.init)
	{
		OMAHeader oma(1 /* atrac3p id */, adec.sample_rate, adec.channels, adec.frame_size);
		if (buf_size < sizeof(oma))
		{
			cellAdec->Error("adecRead(): OMAHeader writing failed");
			Emu.Pause();
			return 0;
		}

		memcpy(buf, &oma, sizeof(oma));
		buf += sizeof(oma);
		buf_size -= sizeof(oma);
		res += sizeof(oma);

		adec.reader.init = true;
	}

	if (adec.reader.size < (u32)buf_size /*&& !adec.just_started*/)
	{
		AdecTask task;
		if (!adec.job.Peek(task, &adec.is_closed))
		{
			if (Emu.IsStopped()) cellAdec->Warning("adecRawRead() aborted");
			return 0;
		}

		switch (task.type)
		{
		case adecEndSeq:
		case adecClose:
		{
			buf_size = adec.reader.size;
		}
		break;
		
		case adecDecodeAu:
		{
			memcpy(buf, vm::get_ptr<void>(adec.reader.addr), adec.reader.size);
			
			buf += adec.reader.size;
			buf_size -= adec.reader.size;
			res += adec.reader.size;

			adec.cbFunc.call(*adec.adecCb, adec.id, CELL_ADEC_MSG_TYPE_AUDONE, adec.task.au.auInfo_addr, adec.cbArg);

			adec.job.Pop(adec.task, nullptr);

			adec.reader.addr = adec.task.au.addr;
			adec.reader.size = adec.task.au.size;
			adec.reader.has_ats = adec.use_ats_headers;
			//LOG_NOTICE(HLE, "Audio AU: size = 0x%x, pts = 0x%llx", adec.task.au.size, adec.task.au.pts);
		}
		break;
		
		default:
		{
			cellAdec->Error("adecRawRead(): unknown task (%d)", task.type);
			Emu.Pause();
			return -1;
		}
		}

		goto next;
	}
	else if (adec.reader.size < (u32)buf_size)
	{
		buf_size = adec.reader.size;
	}

	if (!buf_size)
	{
		return res;
	}
	else
	{
		memcpy(buf, vm::get_ptr<void>(adec.reader.addr), buf_size);

		adec.reader.addr += buf_size;
		adec.reader.size -= buf_size;
		return res + buf_size;
	}
}

u32 adecOpen(AudioDecoder* data)
{
	AudioDecoder& adec = *data;

	u32 adec_id = cellAdec->GetNewId(data);

	adec.id = adec_id;

	adec.adecCb = (PPUThread*)&Emu.GetCPU().AddThread(CPU_THREAD_PPU);
	adec.adecCb->SetName("Audio Decoder[" + std::to_string(adec_id) + "] Callback");
	adec.adecCb->SetEntry(0);
	adec.adecCb->SetPrio(1001);
	adec.adecCb->SetStackSize(0x10000);
	adec.adecCb->InitStack();
	adec.adecCb->InitRegs();
	adec.adecCb->DoRun();

	thread t("Audio Decoder[" + std::to_string(adec_id) + "] Thread", [&]()
	{
		cellAdec->Notice("Audio Decoder thread started");

		AdecTask& task = adec.task;

		while (true)
		{
			if (Emu.IsStopped() || adec.is_closed)
			{
				break;
			}

			//if (!adec.job.GetCountUnsafe() && adec.is_running)
			//{
			//	std::this_thread::sleep_for(std::chrono::milliseconds(1));
			//	continue;
			//}

			if (!adec.job.Pop(task, &adec.is_closed))
			{
				break;
			}

			switch (task.type)
			{
			case adecStartSeq:
			{
				// TODO: reset data
				cellAdec->Warning("adecStartSeq:");

				adec.reader.addr = 0;
				adec.reader.size = 0;
				adec.reader.init = false;
				adec.reader.has_ats = false;
				adec.just_started = true;

				adec.channels = task.at3p.channels;
				adec.frame_size = task.at3p.frame_size;
				adec.sample_rate = task.at3p.sample_rate;
				adec.use_ats_headers = task.at3p.ats_header == 1;
			}
			break;

			case adecEndSeq:
			{
				// TODO: finalize
				cellAdec->Warning("adecEndSeq:");
				adec.cbFunc.call(*adec.adecCb, adec.id, CELL_ADEC_MSG_TYPE_SEQDONE, CELL_OK, adec.cbArg);

				adec.just_finished = true;
			}
			break;

			case adecDecodeAu:
			{
				int err = 0;

				adec.reader.addr = task.au.addr;
				adec.reader.size = task.au.size;
				adec.reader.has_ats = adec.use_ats_headers;
				//LOG_NOTICE(HLE, "Audio AU: size = 0x%x, pts = 0x%llx", task.au.size, task.au.pts);

				if (adec.just_started)
				{
					adec.first_pts = task.au.pts;
					adec.last_pts = task.au.pts - 0x10000; // hack?
				}

				struct AVPacketHolder : AVPacket
				{
					AVPacketHolder(u32 size)
					{
						av_init_packet(this);

						if (size)
						{
							data = (u8*)av_calloc(1, size + FF_INPUT_BUFFER_PADDING_SIZE);
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
					}

				} au(0);

				if (adec.just_started && adec.just_finished)
				{
					avcodec_flush_buffers(adec.ctx);
					
					adec.reader.init = true; // wrong
					adec.just_finished = false;
					adec.just_started = false;
				}
				else if (adec.just_started) // deferred initialization
				{
					AVDictionary* opts = nullptr;
					av_dict_set(&opts, "probesize", "96", 0);
					err = avformat_open_input(&adec.fmt, NULL, av_find_input_format("oma"), &opts);
					if (err || opts)
					{
						cellAdec->Error("adecDecodeAu: avformat_open_input() failed (err=0x%x, opts=%d)", err, opts ? 1 : 0);
						Emu.Pause();
						break;
					}
					
					AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_ATRAC3P); // ???
					if (!codec)
					{
						cellAdec->Error("adecDecodeAu: avcodec_find_decoder() failed");
						Emu.Pause();
						break;
					}
					//err = avformat_find_stream_info(adec.fmt, NULL);
					//if (err)
					//{
					//	cellAdec->Error("adecDecodeAu: avformat_find_stream_info() failed");
					//	Emu.Pause();
					//	break;
					//}
					//if (!adec.fmt->nb_streams)
					//{
					//	cellAdec->Error("adecDecodeAu: no stream found");
					//	Emu.Pause();
					//	break;
					//}
					if (!avformat_new_stream(adec.fmt, codec))
					{
						cellAdec->Error("adecDecodeAu: avformat_new_stream() failed");
						Emu.Pause();
						break;
					}
					adec.ctx = adec.fmt->streams[0]->codec; // TODO: check data

					opts = nullptr;
					av_dict_set(&opts, "refcounted_frames", "1", 0);
					{
						std::lock_guard<std::mutex> lock(g_mutex_avcodec_open2);
						// not multithread-safe (???)
						err = avcodec_open2(adec.ctx, codec, &opts);
					}
					if (err || opts)
					{
						cellAdec->Error("adecDecodeAu: avcodec_open2() failed");
						Emu.Pause();
						break;
					}
					adec.just_started = false;
				}

				bool last_frame = false;

				while (true)
				{
					if (Emu.IsStopped() || adec.is_closed)
					{
						if (Emu.IsStopped()) cellAdec->Warning("adecDecodeAu: aborted");
						break;
					}

					last_frame = av_read_frame(adec.fmt, &au) < 0;
					if (last_frame)
					{
						//break;
						av_free(au.data);
						au.data = NULL;
						au.size = 0;
					}

					struct AdecFrameHolder : AdecFrame
					{
						AdecFrameHolder()
						{
							data = av_frame_alloc();
						}

						~AdecFrameHolder()
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
						cellAdec->Error("adecDecodeAu: av_frame_alloc() failed");
						Emu.Pause();
						break;
					}

					int got_frame = 0;

					int decode = avcodec_decode_audio4(adec.ctx, frame.data, &got_frame, &au);

					if (decode <= 0)
					{
						if (!last_frame && decode < 0)
						{
							cellAdec->Error("adecDecodeAu: AU decoding error(0x%x)", decode);
						}
						if (!got_frame && adec.reader.size == 0) break;
					}

					if (got_frame)
					{
						u64 ts = av_frame_get_best_effort_timestamp(frame.data);
						if (ts != AV_NOPTS_VALUE)
						{
							frame.pts = ts/* - adec.first_pts*/;
							adec.last_pts = frame.pts;
						}
						else
						{
							adec.last_pts += ((u64)frame.data->nb_samples) * 90000 / frame.data->sample_rate;
							frame.pts = adec.last_pts;
						}
						//frame.pts = adec.last_pts;
						//adec.last_pts += ((u64)frame.data->nb_samples) * 90000 / 48000; // ???
						frame.auAddr = task.au.addr;
						frame.auSize = task.au.size;
						frame.userdata = task.au.userdata;
						frame.size = frame.data->nb_samples * frame.data->channels * sizeof(float);

						if (frame.data->format != AV_SAMPLE_FMT_FLTP)
						{
							cellAdec->Error("adecDecodeaAu: unsupported frame format(%d)", frame.data->format);
							Emu.Pause();
							break;
						}

						//LOG_NOTICE(HLE, "got audio frame (pts=0x%llx, nb_samples=%d, ch=%d, sample_rate=%d, nbps=%d)",
							//frame.pts, frame.data->nb_samples, frame.data->channels, frame.data->sample_rate,
							//av_get_bytes_per_sample((AVSampleFormat)frame.data->format));

						if (adec.frames.Push(frame, &adec.is_closed))
						{
							frame.data = nullptr; // to prevent destruction
							adec.cbFunc.call(*adec.adecCb, adec.id, CELL_ADEC_MSG_TYPE_PCMOUT, CELL_OK, adec.cbArg);
						}
					}
				}

				adec.cbFunc.call(*adec.adecCb, adec.id, CELL_ADEC_MSG_TYPE_AUDONE, task.au.auInfo_addr, adec.cbArg);
			}
			break;

			case adecClose: break;

			default:
			{
				cellAdec->Error("Audio Decoder thread error: unknown task(%d)", task.type);
				Emu.Pause();
				return;
			}
			}
		}

		adec.is_finished = true;
		if (adec.is_closed) cellAdec->Notice("Audio Decoder thread ended");
		if (Emu.IsStopped()) cellAdec->Warning("Audio Decoder thread aborted");
	});

	t.detach();

	return adec_id;
}

bool adecCheckType(AudioCodecType type)
{
	switch (type)
	{
	case CELL_ADEC_TYPE_ATRACX: cellAdec->Notice("adecCheckType: ATRAC3plus"); break;
	case CELL_ADEC_TYPE_ATRACX_2CH: cellAdec->Notice("adecCheckType: ATRAC3plus 2ch"); break;

	case CELL_ADEC_TYPE_ATRACX_6CH:
	case CELL_ADEC_TYPE_ATRACX_8CH:
	case CELL_ADEC_TYPE_LPCM_PAMF:
	case CELL_ADEC_TYPE_AC3:
	case CELL_ADEC_TYPE_MP3:
	case CELL_ADEC_TYPE_ATRAC3:
	case CELL_ADEC_TYPE_MPEG_L2:
	case CELL_ADEC_TYPE_CELP:
	case CELL_ADEC_TYPE_M4AAC:
	case CELL_ADEC_TYPE_CELP8:
		cellAdec->Todo("Unimplemented audio codec type (%d)", type);
		break;
	default:
		return false;
	}

	return true;
}

int cellAdecQueryAttr(vm::ptr<CellAdecType> type, vm::ptr<CellAdecAttr> attr)
{
	cellAdec->Warning("cellAdecQueryAttr(type_addr=0x%x, attr_addr=0x%x)", type.addr(), attr.addr());

	if (!adecCheckType(type->audioCodecType)) return CELL_ADEC_ERROR_ARG;

	// TODO: check values
	attr->adecVerLower = 0x280000; // from dmux
	attr->adecVerUpper = 0x260000;
	attr->workMemSize = 256 * 1024; // 256 KB

	return CELL_OK;
}

int cellAdecOpen(vm::ptr<CellAdecType> type, vm::ptr<CellAdecResource> res, vm::ptr<CellAdecCb> cb, vm::ptr<u32> handle)
{
	cellAdec->Warning("cellAdecOpen(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)", 
		type.addr(), res.addr(), cb.addr(), handle.addr());

	if (!adecCheckType(type->audioCodecType)) return CELL_ADEC_ERROR_ARG;

	*handle = adecOpen(new AudioDecoder(type->audioCodecType, res->startAddr, res->totalMemSize, vm::ptr<CellAdecCbMsg>::make(cb->cbFunc.addr()), cb->cbArg));

	return CELL_OK;
}

int cellAdecOpenEx(vm::ptr<CellAdecType> type, vm::ptr<CellAdecResourceEx> res, vm::ptr<CellAdecCb> cb, vm::ptr<u32> handle)
{
	cellAdec->Warning("cellAdecOpenEx(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)", 
		type.addr(), res.addr(), cb.addr(), handle.addr());

	if (!adecCheckType(type->audioCodecType)) return CELL_ADEC_ERROR_ARG;

	*handle = adecOpen(new AudioDecoder(type->audioCodecType, res->startAddr, res->totalMemSize, vm::ptr<CellAdecCbMsg>::make(cb->cbFunc.addr()), cb->cbArg));

	return CELL_OK;
}

int cellAdecClose(u32 handle)
{
	cellAdec->Warning("cellAdecClose(handle=%d)", handle);

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	adec->is_closed = true;
	adec->job.Push(AdecTask(adecClose), &sq_no_wait);

	while (!adec->is_finished)
	{
		if (Emu.IsStopped())
		{
			cellAdec->Warning("cellAdecClose(%d) aborted", handle);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if (adec->adecCb) Emu.GetCPU().RemoveThread(adec->adecCb->GetId());
	Emu.GetIdManager().RemoveID(handle);
	return CELL_OK;
}

int cellAdecStartSeq(u32 handle, u32 param_addr)
{
	cellAdec->Warning("cellAdecStartSeq(handle=%d, param_addr=0x%x)", handle, param_addr);

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecTask task(adecStartSeq);

	switch (adec->type)
	{
	case CELL_ADEC_TYPE_ATRACX_2CH:
	{
		auto param = vm::ptr<const CellAdecParamAtracX>::make(param_addr);

		task.at3p.sample_rate = param->sampling_freq;
		task.at3p.channel_config = param->ch_config_idx;
		task.at3p.channels = param->nch_out;
		task.at3p.frame_size = param->nbytes;
		task.at3p.extra_config = param->extra_config_data;
		task.at3p.output = param->bw_pcm;
		task.at3p.downmix = param->downmix_flag;
		task.at3p.ats_header = param->au_includes_ats_hdr_flg;
		cellAdec->Todo("*** CellAdecParamAtracX: sr=%d, ch_cfg=%d(%d), frame_size=0x%x, extra=0x%x, output=%d, downmix=%d, ats_header=%d",
			task.at3p.sample_rate, task.at3p.channel_config, task.at3p.channels, task.at3p.frame_size, (u32&)task.at3p.extra_config, task.at3p.output, task.at3p.downmix, task.at3p.ats_header);
		break;
	}
	default:
	{
		cellAdec->Todo("cellAdecStartSeq(): Unimplemented audio codec type(%d)", adec->type);
		Emu.Pause();
		return CELL_OK;
	}
	}

	adec->job.Push(task, &adec->is_closed);
	return CELL_OK;
}

int cellAdecEndSeq(u32 handle)
{
	cellAdec->Warning("cellAdecEndSeq(handle=%d)", handle);

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	adec->job.Push(AdecTask(adecEndSeq), &adec->is_closed);
	return CELL_OK;
}

int cellAdecDecodeAu(u32 handle, vm::ptr<CellAdecAuInfo> auInfo)
{
	cellAdec->Log("cellAdecDecodeAu(handle=%d, auInfo_addr=0x%x)", handle, auInfo.addr());

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecTask task(adecDecodeAu);
	task.au.auInfo_addr = auInfo.addr();
	task.au.addr = auInfo->startAddr;
	task.au.size = auInfo->size;
	task.au.pts = ((u64)auInfo->pts.upper << 32) | (u64)auInfo->pts.lower;
	task.au.userdata = auInfo->userData;

	//cellAdec->Notice("cellAdecDecodeAu(): addr=0x%x, size=0x%x, pts=0x%llx", task.au.addr, task.au.size, task.au.pts);
	adec->job.Push(task, &adec->is_closed);
	return CELL_OK;
}

int cellAdecGetPcm(u32 handle, vm::ptr<float> outBuffer)
{
	cellAdec->Log("cellAdecGetPcm(handle=%d, outBuffer_addr=0x%x)", handle, outBuffer.addr());

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecFrame af;
	if (!adec->frames.Pop(af, &sq_no_wait))
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		return CELL_ADEC_ERROR_EMPTY;
	}

	AVFrame* frame = af.data;

	if (outBuffer)
	{
		// reverse byte order:
		if (frame->channels == 1)
		{
			float* in_f = (float*)frame->extended_data[0];
			for (u32 i = 0; i < af.size / 4; i++)
			{
				outBuffer[i] = in_f[i];
			}
		}
		else if (frame->channels == 2)
		{
			float* in_f[2];
			in_f[0] = (float*)frame->extended_data[0];
			in_f[1] = (float*)frame->extended_data[1];
			for (u32 i = 0; i < af.size / 8; i++)
			{
				outBuffer[i * 2 + 0] = in_f[0][i];
				outBuffer[i * 2 + 1] = in_f[1][i];
			}
		}
		else
		{
			cellAdec->Error("cellAdecGetPcm(): unsupported channel count (%d)", frame->channels);
			Emu.Pause();
		}
	}

	av_frame_unref(af.data);
	av_frame_free(&af.data);
	return CELL_OK;
}

int cellAdecGetPcmItem(u32 handle, vm::ptr<u32> pcmItem_ptr)
{
	cellAdec->Log("cellAdecGetPcmItem(handle=%d, pcmItem_ptr_addr=0x%x)", handle, pcmItem_ptr.addr());

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecFrame af;
	if (!adec->frames.Peek(af, &sq_no_wait))
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		return CELL_ADEC_ERROR_EMPTY;
	}

	AVFrame* frame = af.data;

	auto pcm = vm::ptr<CellAdecPcmItem>::make(adec->memAddr + adec->memBias);

	adec->memBias += 512;
	if (adec->memBias + 512 > adec->memSize)
	{
		adec->memBias = 0;
	}

	pcm->pcmHandle = 0; // ???
	pcm->pcmAttr.bsiInfo_addr = pcm.addr() + sizeof(CellAdecPcmItem);
	pcm->startAddr = 0x00000312; // invalid address (no output)
	pcm->size = af.size;
	pcm->status = CELL_OK;
	pcm->auInfo.pts.lower = (u32)af.pts;
	pcm->auInfo.pts.upper = af.pts >> 32;
	pcm->auInfo.size = af.auSize;
	pcm->auInfo.startAddr = af.auAddr;
	pcm->auInfo.userData = af.userdata;

	auto atx = vm::ptr<CellAdecAtracXInfo>::make(pcm.addr() + sizeof(CellAdecPcmItem));
	atx->samplingFreq = frame->sample_rate;
	atx->nbytes = frame->nb_samples * sizeof(float);
	if (frame->channels == 1)
	{
		atx->channelConfigIndex = 1;
	}
	else if (frame->channels == 2)
	{
		atx->channelConfigIndex = 2;
	}
	else
	{
		cellAdec->Error("cellAdecGetPcmItem(): unsupported channel count (%d)", frame->channels);
		Emu.Pause();
	}

	*pcmItem_ptr = pcm.addr();
	return CELL_OK;
}

void cellAdec_init(Module * pxThis)
{
	cellAdec = pxThis;

	REG_FUNC(cellAdec, cellAdecQueryAttr);
	REG_FUNC(cellAdec, cellAdecOpen);
	REG_FUNC(cellAdec, cellAdecOpenEx);
	REG_FUNC(cellAdec, cellAdecClose);
	REG_FUNC(cellAdec, cellAdecStartSeq);
	REG_FUNC(cellAdec, cellAdecEndSeq);
	REG_FUNC(cellAdec, cellAdecDecodeAu);
	REG_FUNC(cellAdec, cellAdecGetPcm);
	REG_FUNC(cellAdec, cellAdecGetPcmItem);
}
