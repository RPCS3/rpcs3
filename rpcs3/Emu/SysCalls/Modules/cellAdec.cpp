#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"

extern std::mutex g_mutex_avcodec_open2;

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

#include "cellPamf.h"
#include "cellAdec.h"

extern Module cellAdec;

#define ADEC_ERROR(...) { cellAdec.Error(__VA_ARGS__); Emu.Pause(); return; } // only for decoder thread

AudioDecoder::AudioDecoder(s32 type, u32 addr, u32 size, vm::ptr<CellAdecCbMsg> func, u32 arg)
	: type(type)
	, memAddr(addr)
	, memSize(size)
	, memBias(0)
	, cbFunc(func)
	, cbArg(arg)
	, is_closed(false)
	, is_finished(false)
	, just_started(false)
	, just_finished(false)
	, codec(nullptr)
	, input_format(nullptr)
	, ctx(nullptr)
	, fmt(nullptr)
{
	av_register_all();
	avcodec_register_all();

	switch (type)
	{
	case CELL_ADEC_TYPE_ATRACX:
	case CELL_ADEC_TYPE_ATRACX_2CH:
	case CELL_ADEC_TYPE_ATRACX_6CH:
	case CELL_ADEC_TYPE_ATRACX_8CH:
	{
		codec = avcodec_find_decoder(AV_CODEC_ID_ATRAC3P);
		input_format = av_find_input_format("oma");
		break;
	}
	case CELL_ADEC_TYPE_MP3:
	{
		codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
		input_format = av_find_input_format("mp3");
		break;
	}
	default:
	{
		ADEC_ERROR("AudioDecoder(): unknown type (0x%x)", type);
	}
	}
	
	if (!codec)
	{
		ADEC_ERROR("AudioDecoder(): avcodec_find_decoder() failed");
	}
	if (!input_format)
	{
		ADEC_ERROR("AudioDecoder(): av_find_input_format() failed");
	}
	fmt = avformat_alloc_context();
	if (!fmt)
	{
		ADEC_ERROR("AudioDecoder(): avformat_alloc_context() failed");
	}
	io_buf = (u8*)av_malloc(4096);
	fmt->pb = avio_alloc_context(io_buf, 256, 0, this, adecRead, NULL, NULL);
	if (!fmt->pb)
	{
		ADEC_ERROR("AudioDecoder(): avio_alloc_context() failed");
	}
}

AudioDecoder::~AudioDecoder()
{
	// TODO: check finalization
	AdecFrame af;
	while (frames.try_pop(af))
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
	if (adecIsAtracX(adec.type) && adec.reader.has_ats)
	{
		u8 code1 = vm::read8(adec.reader.addr + 2);
		u8 code2 = vm::read8(adec.reader.addr + 3);
		adec.ch_cfg = (code1 >> 2) & 0x7;
		adec.frame_size = ((((u32)code1 & 0x3) << 8) | (u32)code2) * 8 + 8;
		adec.sample_rate = at3freq[code1 >> 5];

		adec.reader.size -= 8;
		adec.reader.addr += 8;
		adec.reader.has_ats = false;
	}

	if (adecIsAtracX(adec.type) && !adec.reader.init)
	{
		OMAHeader oma(1 /* atrac3p id */, adec.sample_rate, adec.ch_cfg, adec.frame_size);
		if (buf_size < sizeof(oma))
		{
			cellAdec.Error("adecRead(): OMAHeader writing failed");
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
		if (!adec.job.peek(task, 0, &adec.is_closed))
		{
			if (Emu.IsStopped()) cellAdec.Warning("adecRawRead() aborted");
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

			adec.cbFunc(*adec.adecCb, adec.id, CELL_ADEC_MSG_TYPE_AUDONE, adec.task.au.auInfo_addr, adec.cbArg);

			adec.job.pop(adec.task);

			adec.reader.addr = adec.task.au.addr;
			adec.reader.size = adec.task.au.size;
			adec.reader.has_ats = adec.use_ats_headers;
			//LOG_NOTICE(HLE, "Audio AU: size = 0x%x, pts = 0x%llx", adec.task.au.size, adec.task.au.pts);
		}
		break;
		
		default:
		{
			cellAdec.Error("adecRawRead(): unknown task (%d)", task.type);
			Emu.Pause();
			return -1;
		}
		}

		goto next;
	}
	// TODO:: Syphurith: I don't know whether we should keep this else-if now. Since the if condition is same with this one.
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

void adecOpen(u32 adec_id) // TODO: call from the constructor
{
	const auto sptr = Emu.GetIdManager().get<AudioDecoder>(adec_id);
	AudioDecoder& adec = *sptr;

	adec.id = adec_id;

	adec.adecCb = Emu.GetIdManager().make_ptr<PPUThread>(fmt::format("Demuxer[0x%x] Thread", adec_id));
	adec.adecCb->prio = 1001;
	adec.adecCb->stack_size = 0x10000;
	adec.adecCb->custom_task = [sptr](PPUThread& CPU)
	{
		AudioDecoder& adec = *sptr;
		AdecTask& task = adec.task;

		while (true)
		{
			if (Emu.IsStopped() || adec.is_closed)
			{
				break;
			}

			if (!adec.job.pop(task, &adec.is_closed))
			{
				break;
			}

			switch (task.type)
			{
			case adecStartSeq:
			{
				// TODO: reset data
				cellAdec.Warning("adecStartSeq:");

				adec.reader.addr = 0;
				adec.reader.size = 0;
				adec.reader.init = false;
				adec.reader.has_ats = false;
				adec.just_started = true;

				if (adecIsAtracX(adec.type))
				{
					adec.ch_cfg = task.at3p.channel_config;
					adec.ch_out = task.at3p.channels;
					adec.frame_size = task.at3p.frame_size;
					adec.sample_rate = task.at3p.sample_rate;
					adec.use_ats_headers = task.at3p.ats_header == 1;
				}
				break;
			}

			case adecEndSeq:
			{
				// TODO: finalize
				cellAdec.Warning("adecEndSeq:");
				adec.cbFunc(CPU, adec.id, CELL_ADEC_MSG_TYPE_SEQDONE, CELL_OK, adec.cbArg);

				adec.just_finished = true;
				break;
			}

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
					adec.last_pts = task.au.pts;
					if (adecIsAtracX(adec.type)) adec.last_pts -= 0x10000; // hack
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
					err = avformat_open_input(&adec.fmt, NULL, adec.input_format, &opts);
					if (err || opts)
					{
						ADEC_ERROR("adecDecodeAu: avformat_open_input() failed (err=0x%x, opts=%d)", err, opts ? 1 : 0);
					}
					//err = avformat_find_stream_info(adec.fmt, NULL);
					//if (err || !adec.fmt->nb_streams)
					//{
					//	ADEC_ERROR("adecDecodeAu: avformat_find_stream_info() failed (err=0x%x, nb_streams=%d)", err, adec.fmt->nb_streams);
					//}
					if (!avformat_new_stream(adec.fmt, adec.codec))
					{
						ADEC_ERROR("adecDecodeAu: avformat_new_stream() failed");
					}
					adec.ctx = adec.fmt->streams[0]->codec; // TODO: check data

					opts = nullptr;
					av_dict_set(&opts, "refcounted_frames", "1", 0);
					{
						std::lock_guard<std::mutex> lock(g_mutex_avcodec_open2);
						// not multithread-safe (???)
						err = avcodec_open2(adec.ctx, adec.codec, &opts);
					}
					if (err || opts)
					{
						ADEC_ERROR("adecDecodeAu: avcodec_open2() failed (err=0x%x, opts=%d)", err, opts ? 1 : 0);
					}
					adec.just_started = false;
				}

				bool last_frame = false;

				while (true)
				{
					if (Emu.IsStopped() || adec.is_closed)
					{
						if (Emu.IsStopped()) cellAdec.Warning("adecDecodeAu: aborted");
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
						ADEC_ERROR("adecDecodeAu: av_frame_alloc() failed");
					}

					int got_frame = 0;

					int decode = avcodec_decode_audio4(adec.ctx, frame.data, &got_frame, &au);

					if (decode <= 0)
					{
						if (decode < 0)
						{
							cellAdec.Error("adecDecodeAu: AU decoding error(0x%x)", decode);
						}
						if (!got_frame && adec.reader.size == 0) break;
					}

					if (got_frame)
					{
						//u64 ts = av_frame_get_best_effort_timestamp(frame.data);
						//if (ts != AV_NOPTS_VALUE)
						//{
						//	frame.pts = ts/* - adec.first_pts*/;
						//	adec.last_pts = frame.pts;
						//}
						adec.last_pts += ((u64)frame.data->nb_samples) * 90000 / frame.data->sample_rate;
						frame.pts = adec.last_pts;

						s32 nbps = av_get_bytes_per_sample((AVSampleFormat)frame.data->format);
						switch (frame.data->format)
						{
						case AV_SAMPLE_FMT_FLTP: break;
						case AV_SAMPLE_FMT_S16P: break;
						default:
						{
							ADEC_ERROR("adecDecodeAu: unsupported frame format(%d)", frame.data->format);
						}
						}
						frame.auAddr = task.au.addr;
						frame.auSize = task.au.size;
						frame.userdata = task.au.userdata;
						frame.size = frame.data->nb_samples * frame.data->channels * nbps;

						//LOG_NOTICE(HLE, "got audio frame (pts=0x%llx, nb_samples=%d, ch=%d, sample_rate=%d, nbps=%d)",
							//frame.pts, frame.data->nb_samples, frame.data->channels, frame.data->sample_rate, nbps);

						if (adec.frames.push(frame, &adec.is_closed))
						{
							frame.data = nullptr; // to prevent destruction
							adec.cbFunc(CPU, adec.id, CELL_ADEC_MSG_TYPE_PCMOUT, CELL_OK, adec.cbArg);
						}
					}
				}

				adec.cbFunc(CPU, adec.id, CELL_ADEC_MSG_TYPE_AUDONE, task.au.auInfo_addr, adec.cbArg);
				break;
			}

			case adecClose:
			{
				break;
			}

			default:
			{
				ADEC_ERROR("AudioDecoder thread error: unknown task(%d)", task.type);
			}
			}
		}

		adec.is_finished = true;

	};

	adec.adecCb->Run();
	adec.adecCb->Exec();
}

bool adecCheckType(s32 type)
{
	switch (type)
	{
	case CELL_ADEC_TYPE_ATRACX: cellAdec.Notice("adecCheckType(): ATRAC3plus"); break;
	case CELL_ADEC_TYPE_ATRACX_2CH: cellAdec.Notice("adecCheckType(): ATRAC3plus 2ch"); break;
	case CELL_ADEC_TYPE_ATRACX_6CH: cellAdec.Notice("adecCheckType(): ATRAC3plus 6ch"); break;
	case CELL_ADEC_TYPE_ATRACX_8CH: cellAdec.Notice("adecCheckType(): ATRAC3plus 8ch"); break;
	case CELL_ADEC_TYPE_MP3: cellAdec.Notice("adecCheckType(): MP3"); break;

	case CELL_ADEC_TYPE_LPCM_PAMF:
	case CELL_ADEC_TYPE_AC3:
	case CELL_ADEC_TYPE_ATRAC3:
	case CELL_ADEC_TYPE_MPEG_L2:
	case CELL_ADEC_TYPE_CELP:
	case CELL_ADEC_TYPE_M4AAC:
	case CELL_ADEC_TYPE_CELP8:
	{
		cellAdec.Todo("Unimplemented audio codec type (%d)", type);
		Emu.Pause();
		break;
	}	
	default: return false;
	}

	return true;
}

s32 cellAdecQueryAttr(vm::ptr<CellAdecType> type, vm::ptr<CellAdecAttr> attr)
{
	cellAdec.Warning("cellAdecQueryAttr(type=*0x%x, attr=*0x%x)", type, attr);

	if (!adecCheckType(type->audioCodecType))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	// TODO: check values
	attr->adecVerLower = 0x280000; // from dmux
	attr->adecVerUpper = 0x260000;
	attr->workMemSize = 256 * 1024; // 256 KB

	return CELL_OK;
}

s32 cellAdecOpen(vm::ptr<CellAdecType> type, vm::ptr<CellAdecResource> res, vm::ptr<CellAdecCb> cb, vm::ptr<u32> handle)
{
	cellAdec.Warning("cellAdecOpen(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	if (!adecCheckType(type->audioCodecType))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	adecOpen(*handle = Emu.GetIdManager().make<AudioDecoder>(type->audioCodecType, res->startAddr, res->totalMemSize, cb->cbFunc, cb->cbArg));

	return CELL_OK;
}

s32 cellAdecOpenEx(vm::ptr<CellAdecType> type, vm::ptr<CellAdecResourceEx> res, vm::ptr<CellAdecCb> cb, vm::ptr<u32> handle)
{
	cellAdec.Warning("cellAdecOpenEx(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	if (!adecCheckType(type->audioCodecType))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	adecOpen(*handle = Emu.GetIdManager().make<AudioDecoder>(type->audioCodecType, res->startAddr, res->totalMemSize, cb->cbFunc, cb->cbArg));

	return CELL_OK;
}

s32 _nid_df982d2c(vm::ptr<CellAdecType> type, vm::ptr<CellAdecResourceEx> res, vm::ptr<CellAdecCb> cb, vm::ptr<u32> handle)
{
	return cellAdecOpenEx(type, res, cb, handle);
}

s32 cellAdecClose(u32 handle)
{
	cellAdec.Warning("cellAdecClose(handle=0x%x)", handle);

	const auto adec = Emu.GetIdManager().get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	adec->is_closed = true;
	adec->job.try_push(AdecTask(adecClose));

	while (!adec->is_finished)
	{
		CHECK_EMU_STATUS;

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	}

	Emu.GetIdManager().remove<PPUThread>(adec->adecCb->GetId());
	Emu.GetIdManager().remove<AudioDecoder>(handle);
	return CELL_OK;
}

s32 cellAdecStartSeq(u32 handle, u32 param)
{
	cellAdec.Warning("cellAdecStartSeq(handle=0x%x, param=*0x%x)", handle, param);

	const auto adec = Emu.GetIdManager().get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecTask task(adecStartSeq);

	switch (adec->type)
	{
	case CELL_ADEC_TYPE_ATRACX:
	case CELL_ADEC_TYPE_ATRACX_2CH:
	case CELL_ADEC_TYPE_ATRACX_6CH:
	case CELL_ADEC_TYPE_ATRACX_8CH:
	{
		const auto atx = vm::cptr<CellAdecParamAtracX>::make(param);

		task.at3p.sample_rate = atx->sampling_freq;
		task.at3p.channel_config = atx->ch_config_idx;
		task.at3p.channels = atx->nch_out;
		task.at3p.frame_size = atx->nbytes;
		task.at3p.extra_config = atx->extra_config_data;
		task.at3p.output = atx->bw_pcm;
		task.at3p.downmix = atx->downmix_flag;
		task.at3p.ats_header = atx->au_includes_ats_hdr_flg;
		cellAdec.Todo("*** CellAdecParamAtracX: sr=%d, ch_cfg=%d(%d), frame_size=0x%x, extra=0x%x, output=%d, downmix=%d, ats_header=%d",
			task.at3p.sample_rate, task.at3p.channel_config, task.at3p.channels, task.at3p.frame_size, (u32&)task.at3p.extra_config, task.at3p.output, task.at3p.downmix, task.at3p.ats_header);
		break;
	}
	case CELL_ADEC_TYPE_MP3:
	{
		const auto mp3 = vm::cptr<CellAdecParamMP3>::make(param);

		cellAdec.Todo("*** CellAdecParamMP3: bw_pcm=%d", mp3->bw_pcm);
		break;
	}
	default:
	{
		cellAdec.Todo("cellAdecStartSeq(): Unimplemented audio codec type(%d)", adec->type);
		Emu.Pause();
		return CELL_OK;
	}
	}

	adec->job.push(task, &adec->is_closed);
	return CELL_OK;
}

s32 cellAdecEndSeq(u32 handle)
{
	cellAdec.Warning("cellAdecEndSeq(handle=0x%x)", handle);

	const auto adec = Emu.GetIdManager().get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	adec->job.push(AdecTask(adecEndSeq), &adec->is_closed);
	return CELL_OK;
}

s32 cellAdecDecodeAu(u32 handle, vm::ptr<CellAdecAuInfo> auInfo)
{
	cellAdec.Log("cellAdecDecodeAu(handle=0x%x, auInfo=*0x%x)", handle, auInfo);

	const auto adec = Emu.GetIdManager().get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecTask task(adecDecodeAu);
	task.au.auInfo_addr = auInfo.addr();
	task.au.addr = auInfo->startAddr;
	task.au.size = auInfo->size;
	task.au.pts = ((u64)auInfo->pts.upper << 32) | (u64)auInfo->pts.lower;
	task.au.userdata = auInfo->userData;

	//cellAdec.Notice("cellAdecDecodeAu(): addr=0x%x, size=0x%x, pts=0x%llx", task.au.addr, task.au.size, task.au.pts);
	adec->job.push(task, &adec->is_closed);
	return CELL_OK;
}

s32 cellAdecGetPcm(u32 handle, vm::ptr<float> outBuffer)
{
	cellAdec.Log("cellAdecGetPcm(handle=0x%x, outBuffer=*0x%x)", handle, outBuffer);

	const auto adec = Emu.GetIdManager().get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecFrame af;
	if (!adec->frames.try_pop(af))
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		return CELL_ADEC_ERROR_EMPTY;
	}

	std::unique_ptr<AVFrame, void(*)(AVFrame*)> frame(af.data, [](AVFrame* frame)
	{
		av_frame_unref(frame);
		av_frame_free(&frame);
	});

	if (outBuffer)
	{
		// reverse byte order:
		if (frame->format == AV_SAMPLE_FMT_FLTP && frame->channels == 1)
		{
			float* in_f = (float*)frame->extended_data[0];
			for (u32 i = 0; i < af.size / 4; i++)
			{
				outBuffer[i] = in_f[i];
			}
		}
		else if (frame->format == AV_SAMPLE_FMT_FLTP && frame->channels == 2)
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
		else if (frame->format == AV_SAMPLE_FMT_FLTP && frame->channels == 6)
		{
			float* in_f[6];
			in_f[0] = (float*)frame->extended_data[0];
			in_f[1] = (float*)frame->extended_data[1];
			in_f[2] = (float*)frame->extended_data[2];
			in_f[3] = (float*)frame->extended_data[3];
			in_f[4] = (float*)frame->extended_data[4];
			in_f[5] = (float*)frame->extended_data[5];
			for (u32 i = 0; i < af.size / 24; i++)
			{
				outBuffer[i * 6 + 0] = in_f[0][i];
				outBuffer[i * 6 + 1] = in_f[1][i];
				outBuffer[i * 6 + 2] = in_f[2][i];
				outBuffer[i * 6 + 3] = in_f[3][i];
				outBuffer[i * 6 + 4] = in_f[4][i];
				outBuffer[i * 6 + 5] = in_f[5][i];
			}
		}
		else if (frame->format == AV_SAMPLE_FMT_FLTP && frame->channels == 8)
		{
			float* in_f[8];
			in_f[0] = (float*)frame->extended_data[0];
			in_f[1] = (float*)frame->extended_data[1];
			in_f[2] = (float*)frame->extended_data[2];
			in_f[3] = (float*)frame->extended_data[3];
			in_f[4] = (float*)frame->extended_data[4];
			in_f[5] = (float*)frame->extended_data[5];
			in_f[6] = (float*)frame->extended_data[6];
			in_f[7] = (float*)frame->extended_data[7];
			for (u32 i = 0; i < af.size / 24; i++)
			{
				outBuffer[i * 8 + 0] = in_f[0][i];
				outBuffer[i * 8 + 1] = in_f[1][i];
				outBuffer[i * 8 + 2] = in_f[2][i];
				outBuffer[i * 8 + 3] = in_f[3][i];
				outBuffer[i * 8 + 4] = in_f[4][i];
				outBuffer[i * 8 + 5] = in_f[5][i];
				outBuffer[i * 8 + 6] = in_f[6][i];
				outBuffer[i * 8 + 7] = in_f[7][i];
			}
		}
		else if (frame->format == AV_SAMPLE_FMT_S16P && frame->channels == 1)
		{
			s16* in_i = (s16*)frame->extended_data[0];
			for (u32 i = 0; i < af.size / 2; i++)
			{
				outBuffer[i] = (float)in_i[i] / 0x8000;
			}
		}
		else if (frame->format == AV_SAMPLE_FMT_S16P && frame->channels == 2)
		{
			s16* in_i[2];
			in_i[0] = (s16*)frame->extended_data[0];
			in_i[1] = (s16*)frame->extended_data[1];
			for (u32 i = 0; i < af.size / 4; i++)
			{
				outBuffer[i * 2 + 0] = (float)in_i[0][i] / 0x8000;
				outBuffer[i * 2 + 1] = (float)in_i[1][i] / 0x8000;
			}
		}
		else
		{
			cellAdec.Fatal("cellAdecGetPcm(): unsupported frame format (channels=%d, format=%d)", frame->channels, frame->format);
		}
	}

	return CELL_OK;
}

s32 cellAdecGetPcmItem(u32 handle, vm::pptr<CellAdecPcmItem> pcmItem)
{
	cellAdec.Log("cellAdecGetPcmItem(handle=0x%x, pcmItem=**0x%x)", handle, pcmItem);

	const auto adec = Emu.GetIdManager().get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecFrame af;
	if (!adec->frames.try_peek(af))
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		return CELL_ADEC_ERROR_EMPTY;
	}

	AVFrame* frame = af.data;

	const auto pcm = vm::ptr<CellAdecPcmItem>::make(adec->memAddr + adec->memBias);

	adec->memBias += 512;
	if (adec->memBias + 512 > adec->memSize)
	{
		adec->memBias = 0;
	}

	pcm->pcmHandle = 0; // ???
	pcm->pcmAttr.bsiInfo_addr = pcm.addr() + sizeof32(CellAdecPcmItem);
	pcm->startAddr = 0x00000312; // invalid address (no output)
	pcm->size = af.size;
	pcm->status = CELL_OK;
	pcm->auInfo.pts.lower = (u32)(af.pts);
	pcm->auInfo.pts.upper = (u32)(af.pts >> 32);
	pcm->auInfo.size = af.auSize;
	pcm->auInfo.startAddr = af.auAddr;
	pcm->auInfo.userData = af.userdata;

	if (adecIsAtracX(adec->type))
	{
		auto atx = vm::ptr<CellAdecAtracXInfo>::make(pcm.addr() + sizeof32(CellAdecPcmItem));

		atx->samplingFreq = frame->sample_rate;
		atx->nbytes = frame->nb_samples * sizeof32(float);
		if (frame->channels == 1)
		{
			atx->channelConfigIndex = 1;
		}
		else if (frame->channels == 2)
		{
			atx->channelConfigIndex = 2;
		}
		else if (frame->channels == 6)
		{
			atx->channelConfigIndex = 6;
		}
		else if (frame->channels == 8)
		{
			atx->channelConfigIndex = 7;
		}
		else
		{
			cellAdec.Error("cellAdecGetPcmItem(): unsupported channel count (%d)", frame->channels);
			Emu.Pause();
		}
	}
	else if (adec->type == CELL_ADEC_TYPE_MP3)
	{
		auto mp3 = vm::ptr<CellAdecMP3Info>::make(pcm.addr() + sizeof32(CellAdecPcmItem));

		// TODO
		memset(mp3.get_ptr(), 0, sizeof(CellAdecMP3Info));
	}

	*pcmItem = pcm;
	return CELL_OK;
}

Module cellAdec("cellAdec", []()
{
	REG_FUNC(cellAdec, cellAdecQueryAttr);
	REG_FUNC(cellAdec, cellAdecOpen);
	REG_FUNC(cellAdec, cellAdecOpenEx);
	REG_UNNAMED(cellAdec, df982d2c);
	REG_FUNC(cellAdec, cellAdecClose);
	REG_FUNC(cellAdec, cellAdecStartSeq);
	REG_FUNC(cellAdec, cellAdecEndSeq);
	REG_FUNC(cellAdec, cellAdecDecodeAu);
	REG_FUNC(cellAdec, cellAdecGetPcm);
	REG_FUNC(cellAdec, cellAdecGetPcmItem);
});
