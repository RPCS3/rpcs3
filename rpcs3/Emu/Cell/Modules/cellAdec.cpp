#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_sync.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#ifndef AV_INPUT_BUFFER_PADDING_SIZE
#define AV_INPUT_BUFFER_PADDING_SIZE FF_INPUT_BUFFER_PADDING_SIZE
#endif
}

#include "cellPamf.h"
#include "cellAdec.h"

#include <mutex>
#include <thread>

extern std::mutex g_mutex_avcodec_open2;

logs::channel cellAdec("cellAdec");

class AudioDecoder : public ppu_thread
{
public:
	squeue_t<AdecTask> job;
	volatile bool is_closed;
	volatile bool is_finished;
	bool just_started;
	bool just_finished;

	AVCodec* codec;
	AVInputFormat* input_format;
	AVCodecContext* ctx;
	AVFormatContext* fmt;
	u8* io_buf;

	struct AudioReader
	{
		u32 addr;
		u32 size;
		bool init;
		bool has_ats;

		AudioReader()
			: init(false)
		{
		}

	} reader;

	squeue_t<AdecFrame> frames;

	const s32 type;
	const u32 memAddr;
	const u32 memSize;
	const vm::ptr<CellAdecCbMsg> cbFunc;
	const u32 cbArg;
	u32 memBias;

	AdecTask task;
	u64 last_pts, first_pts;

	u32 ch_out;
	u32 ch_cfg;
	u32 frame_size;
	u32 sample_rate;
	bool use_ats_headers;

	AudioDecoder(s32 type, u32 addr, u32 size, vm::ptr<CellAdecCbMsg> func, u32 arg)
		: ppu_thread("HLE Audio Decoder")
		, type(type)
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
			fmt::throw_exception("Unknown type (0x%x)" HERE, type);
		}
		}

		if (!codec)
		{
			fmt::throw_exception("avcodec_find_decoder() failed" HERE);
		}
		if (!input_format)
		{
			fmt::throw_exception("av_find_input_format() failed" HERE);
		}
		fmt = avformat_alloc_context();
		if (!fmt)
		{
			fmt::throw_exception("avformat_alloc_context() failed" HERE);
		}
		io_buf = (u8*)av_malloc(4096);
		fmt->pb = avio_alloc_context(io_buf, 256, 0, this, adecRead, NULL, NULL);
		if (!fmt->pb)
		{
			fmt::throw_exception("avio_alloc_context() failed" HERE);
		}
	}

	~AudioDecoder()
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

	virtual void cpu_task() override
	{
		while (true)
		{
			if (Emu.IsStopped() || is_closed)
			{
				break;
			}

			if (!job.pop(task, &is_closed))
			{
				break;
			}

			switch (task.type)
			{
			case adecStartSeq:
			{
				// TODO: reset data
				cellAdec.warning("adecStartSeq:");

				reader.addr = 0;
				reader.size = 0;
				reader.init = false;
				reader.has_ats = false;
				just_started = true;

				if (adecIsAtracX(type))
				{
					ch_cfg = task.at3p.channel_config;
					ch_out = task.at3p.channels;
					frame_size = task.at3p.frame_size;
					sample_rate = task.at3p.sample_rate;
					use_ats_headers = task.at3p.ats_header == 1;
				}
				break;
			}

			case adecEndSeq:
			{
				// TODO: finalize
				cellAdec.warning("adecEndSeq:");
				cbFunc(*this, id, CELL_ADEC_MSG_TYPE_SEQDONE, CELL_OK, cbArg);
				lv2_obj::sleep(*this);

				just_finished = true;
				break;
			}

			case adecDecodeAu:
			{
				int err = 0;

				reader.addr = task.au.addr;
				reader.size = task.au.size;
				reader.has_ats = use_ats_headers;
				//LOG_NOTICE(HLE, "Audio AU: size = 0x%x, pts = 0x%llx", task.au.size, task.au.pts);

				if (just_started)
				{
					first_pts = task.au.pts;
					last_pts = task.au.pts;
					if (adecIsAtracX(type)) last_pts -= 0x10000; // hack
				}

				struct AVPacketHolder : AVPacket
				{
					AVPacketHolder(u32 size)
					{
						av_init_packet(this);

						if (size)
						{
							data = (u8*)av_calloc(1, size + AV_INPUT_BUFFER_PADDING_SIZE);
							this->size = size + AV_INPUT_BUFFER_PADDING_SIZE;
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

				if (just_started && just_finished)
				{
					avcodec_flush_buffers(ctx);
					
					reader.init = true; // wrong
					just_finished = false;
					just_started = false;
				}
				else if (just_started) // deferred initialization
				{
					AVDictionary* opts = nullptr;
					av_dict_set(&opts, "probesize", "96", 0);
					err = avformat_open_input(&fmt, NULL, input_format, &opts);
					if (err || opts)
					{
						fmt::throw_exception("avformat_open_input() failed (err=0x%x, opts=%d)" HERE, err, opts ? 1 : 0);
					}
					//err = avformat_find_stream_info(fmt, NULL);
					//if (err || !fmt->nb_streams)
					//{
					//	ADEC_ERROR("adecDecodeAu: avformat_find_stream_info() failed (err=0x%x, nb_streams=%d)", err, fmt->nb_streams);
					//}
					if (!avformat_new_stream(fmt, codec))
					{
						fmt::throw_exception("avformat_new_stream() failed" HERE);
					}
					ctx = fmt->streams[0]->codec; // TODO: check data

					opts = nullptr;
					av_dict_set(&opts, "refcounted_frames", "1", 0);
					{
						std::lock_guard<std::mutex> lock(g_mutex_avcodec_open2);
						// not multithread-safe (???)
						err = avcodec_open2(ctx, codec, &opts);
					}
					if (err || opts)
					{
						fmt::throw_exception("avcodec_open2() failed (err=0x%x, opts=%d)" HERE, err, opts ? 1 : 0);
					}
					just_started = false;
				}

				bool last_frame = false;

				while (true)
				{
					if (Emu.IsStopped() || is_closed)
					{
						if (Emu.IsStopped()) cellAdec.warning("adecDecodeAu: aborted");
						break;
					}

					last_frame = av_read_frame(fmt, &au) < 0;
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
						fmt::throw_exception("av_frame_alloc() failed" HERE);
					}

					int got_frame = 0;

					int decode = avcodec_decode_audio4(ctx, frame.data, &got_frame, &au);

					if (decode <= 0)
					{
						if (decode < 0)
						{
							cellAdec.error("adecDecodeAu: AU decoding error(0x%x)", decode);
						}
						if (!got_frame && reader.size == 0) break;
					}

					if (got_frame)
					{
						//u64 ts = av_frame_get_best_effort_timestamp(frame.data);
						//if (ts != AV_NOPTS_VALUE)
						//{
						//	frame.pts = ts/* - first_pts*/;
						//	last_pts = frame.pts;
						//}
						last_pts += ((u64)frame.data->nb_samples) * 90000 / frame.data->sample_rate;
						frame.pts = last_pts;

						s32 nbps = av_get_bytes_per_sample((AVSampleFormat)frame.data->format);
						switch (frame.data->format)
						{
						case AV_SAMPLE_FMT_FLTP: break;
						case AV_SAMPLE_FMT_S16P: break;
						default:
						{
							fmt::throw_exception("Unsupported frame format(%d)" HERE, frame.data->format);
						}
						}
						frame.auAddr = task.au.addr;
						frame.auSize = task.au.size;
						frame.userdata = task.au.userdata;
						frame.size = frame.data->nb_samples * frame.data->channels * nbps;

						//LOG_NOTICE(HLE, "got audio frame (pts=0x%llx, nb_samples=%d, ch=%d, sample_rate=%d, nbps=%d)",
							//frame.pts, frame.data->nb_samples, frame.data->channels, frame.data->sample_rate, nbps);

						if (frames.push(frame, &is_closed))
						{
							frame.data = nullptr; // to prevent destruction
							cbFunc(*this, id, CELL_ADEC_MSG_TYPE_PCMOUT, CELL_OK, cbArg);
							lv2_obj::sleep(*this);
						}
					}
				}

				cbFunc(*this, id, CELL_ADEC_MSG_TYPE_AUDONE, task.au.auInfo_addr, cbArg);
				lv2_obj::sleep(*this);
				break;
			}

			case adecClose:
			{
				break;
			}

			default:
			{
				fmt::throw_exception("Unknown task(%d)" HERE, (u32)task.type);
			}
			}
		}

		is_finished = true;
	}
};

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
			cellAdec.error("adecRead(): OMAHeader writing failed");
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
			if (Emu.IsStopped()) cellAdec.warning("adecRawRead() aborted");
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
			std::memcpy(buf, vm::base(adec.reader.addr), adec.reader.size);

			buf += adec.reader.size;
			buf_size -= adec.reader.size;
			res += adec.reader.size;

			adec.cbFunc(adec, adec.id, CELL_ADEC_MSG_TYPE_AUDONE, adec.task.au.auInfo_addr, adec.cbArg);

			adec.job.pop(adec.task);

			adec.reader.addr = adec.task.au.addr;
			adec.reader.size = adec.task.au.size;
			adec.reader.has_ats = adec.use_ats_headers;
			//LOG_NOTICE(HLE, "Audio AU: size = 0x%x, pts = 0x%llx", adec.task.au.size, adec.task.au.pts);
		}
		break;

		default:
		{
			cellAdec.error("adecRawRead(): unknown task (%d)", (u32)task.type);
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
		std::memcpy(buf, vm::base(adec.reader.addr), buf_size);

		adec.reader.addr += buf_size;
		adec.reader.size -= buf_size;
		return res + buf_size;
	}
}

bool adecCheckType(s32 type)
{
	switch (type)
	{
	case CELL_ADEC_TYPE_ATRACX: cellAdec.notice("adecCheckType(): ATRAC3plus"); break;
	case CELL_ADEC_TYPE_ATRACX_2CH: cellAdec.notice("adecCheckType(): ATRAC3plus 2ch"); break;
	case CELL_ADEC_TYPE_ATRACX_6CH: cellAdec.notice("adecCheckType(): ATRAC3plus 6ch"); break;
	case CELL_ADEC_TYPE_ATRACX_8CH: cellAdec.notice("adecCheckType(): ATRAC3plus 8ch"); break;
	case CELL_ADEC_TYPE_MP3: cellAdec.notice("adecCheckType(): MP3"); break;

	case CELL_ADEC_TYPE_LPCM_PAMF:
	case CELL_ADEC_TYPE_AC3:
	case CELL_ADEC_TYPE_ATRAC3:
	case CELL_ADEC_TYPE_MPEG_L2:
	case CELL_ADEC_TYPE_CELP:
	case CELL_ADEC_TYPE_M4AAC:
	case CELL_ADEC_TYPE_CELP8:
	{
		cellAdec.todo("Unimplemented audio codec type (%d)", type);
		Emu.Pause();
		break;
	}	
	default: return false;
	}

	return true;
}

s32 cellAdecQueryAttr(vm::ptr<CellAdecType> type, vm::ptr<CellAdecAttr> attr)
{
	cellAdec.warning("cellAdecQueryAttr(type=*0x%x, attr=*0x%x)", type, attr);

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
	cellAdec.warning("cellAdecOpen(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	if (!adecCheckType(type->audioCodecType))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	auto&& adec = idm::make_ptr<ppu_thread, AudioDecoder>(type->audioCodecType, res->startAddr, res->totalMemSize, cb->cbFunc, cb->cbArg);

	*handle = adec->id;

	adec->run();

	return CELL_OK;
}

s32 cellAdecOpenEx(vm::ptr<CellAdecType> type, vm::ptr<CellAdecResourceEx> res, vm::ptr<CellAdecCb> cb, vm::ptr<u32> handle)
{
	cellAdec.warning("cellAdecOpenEx(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	if (!adecCheckType(type->audioCodecType))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	auto&& adec = idm::make_ptr<ppu_thread, AudioDecoder>(type->audioCodecType, res->startAddr, res->totalMemSize, cb->cbFunc, cb->cbArg);

	*handle = adec->id;

	adec->run();

	return CELL_OK;
}

s32 cellAdecOpenExt(vm::ptr<CellAdecType> type, vm::ptr<CellAdecResourceEx> res, vm::ptr<CellAdecCb> cb, vm::ptr<u32> handle)
{
	cellAdec.warning("cellAdecOpenExt(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	return cellAdecOpenEx(type, res, cb, handle);
}

s32 cellAdecClose(u32 handle)
{
	cellAdec.warning("cellAdecClose(handle=0x%x)", handle);

	const auto adec = idm::get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	adec->is_closed = true;
	adec->job.try_push(AdecTask(adecClose));

	while (!adec->is_finished)
	{
		thread_ctrl::wait_for(1000); // hack
	}

	idm::remove<ppu_thread>(handle);
	return CELL_OK;
}

s32 cellAdecStartSeq(u32 handle, u32 param)
{
	cellAdec.warning("cellAdecStartSeq(handle=0x%x, param=*0x%x)", handle, param);

	const auto adec = idm::get<AudioDecoder>(handle);

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
		cellAdec.todo("*** CellAdecParamAtracX: sr=%d, ch_cfg=%d(%d), frame_size=0x%x, extra=0x%x, output=%d, downmix=%d, ats_header=%d",
			task.at3p.sample_rate, task.at3p.channel_config, task.at3p.channels, task.at3p.frame_size, (u32&)task.at3p.extra_config, task.at3p.output, task.at3p.downmix, task.at3p.ats_header);
		break;
	}
	case CELL_ADEC_TYPE_MP3:
	{
		const auto mp3 = vm::cptr<CellAdecParamMP3>::make(param);

		cellAdec.todo("*** CellAdecParamMP3: bw_pcm=%d", mp3->bw_pcm);
		break;
	}
	default:
	{
		cellAdec.todo("cellAdecStartSeq(): Unimplemented audio codec type(%d)", adec->type);
		Emu.Pause();
		return CELL_OK;
	}
	}

	adec->job.push(task, &adec->is_closed);
	return CELL_OK;
}

s32 cellAdecEndSeq(u32 handle)
{
	cellAdec.warning("cellAdecEndSeq(handle=0x%x)", handle);

	const auto adec = idm::get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	adec->job.push(AdecTask(adecEndSeq), &adec->is_closed);
	return CELL_OK;
}

s32 cellAdecDecodeAu(u32 handle, vm::ptr<CellAdecAuInfo> auInfo)
{
	cellAdec.trace("cellAdecDecodeAu(handle=0x%x, auInfo=*0x%x)", handle, auInfo);

	const auto adec = idm::get<AudioDecoder>(handle);

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

	//cellAdec.notice("cellAdecDecodeAu(): addr=0x%x, size=0x%x, pts=0x%llx", task.au.addr, task.au.size, task.au.pts);
	adec->job.push(task, &adec->is_closed);
	return CELL_OK;
}

s32 cellAdecGetPcm(u32 handle, vm::ptr<float> outBuffer)
{
	cellAdec.trace("cellAdecGetPcm(handle=0x%x, outBuffer=*0x%x)", handle, outBuffer);

	const auto adec = idm::get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecFrame af;
	if (!adec->frames.try_pop(af))
	{
		//std::this_thread::sleep_for(1ms); // hack
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
			fmt::throw_exception("Unsupported frame format (channels=%d, format=%d)" HERE, frame->channels, frame->format);
		}
	}

	return CELL_OK;
}

s32 cellAdecGetPcmItem(u32 handle, vm::pptr<CellAdecPcmItem> pcmItem)
{
	cellAdec.trace("cellAdecGetPcmItem(handle=0x%x, pcmItem=**0x%x)", handle, pcmItem);

	const auto adec = idm::get<AudioDecoder>(handle);

	if (!adec)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecFrame af;
	if (!adec->frames.try_peek(af))
	{
		//std::this_thread::sleep_for(1ms); // hack
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
	pcm->pcmAttr.bsiInfo_addr = pcm.addr() + SIZE_32(CellAdecPcmItem);
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
		auto atx = vm::ptr<CellAdecAtracXInfo>::make(pcm.addr() + SIZE_32(CellAdecPcmItem));

		atx->samplingFreq = frame->sample_rate;
		atx->nbytes = frame->nb_samples * SIZE_32(float);
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
			cellAdec.error("cellAdecGetPcmItem(): unsupported channel count (%d)", frame->channels);
			Emu.Pause();
		}
	}
	else if (adec->type == CELL_ADEC_TYPE_MP3)
	{
		auto mp3 = vm::ptr<CellAdecMP3Info>::make(pcm.addr() + SIZE_32(CellAdecPcmItem));

		// TODO
		memset(mp3.get_ptr(), 0, sizeof(CellAdecMP3Info));
	}

	*pcmItem = pcm;
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellAdec)("cellAdec", []()
{
	static ppu_static_module cell_libac3dec("cell_libac3dec");
	static ppu_static_module cellAtrac3dec("cellAtrac3dec");
	static ppu_static_module cellAtracXdec("cellAtracXdec");
	static ppu_static_module cellCelpDec("cellCelpDec");
	static ppu_static_module cellDTSdec("cellDTSdec");
	static ppu_static_module cellM2AACdec("cellM2AACdec");
	static ppu_static_module cellM2BCdec("cellM2BCdec");
	static ppu_static_module cellM4AacDec("cellM4AacDec");
	static ppu_static_module cellMP3dec("cellMP3dec");
	static ppu_static_module cellTRHDdec("cellTRHDdec");
	static ppu_static_module cellWMAdec("cellWMAdec");
	static ppu_static_module cellDTSLBRdec("cellDTSLBRdec");
	static ppu_static_module cellDDPdec("cellDDPdec");
	static ppu_static_module cellM4AacDec2ch("cellM4AacDec2ch");
	static ppu_static_module cellDTSHDdec("cellDTSHDdec");
	static ppu_static_module cellMPL1dec("cellMPL1dec");
	static ppu_static_module cellMP3Sdec("cellMP3Sdec");
	static ppu_static_module cellM4AacDec2chmod("cellM4AacDec2chmod");
	static ppu_static_module cellCelp8Dec("cellCelp8Dec");
	static ppu_static_module cellWMAPROdec("cellWMAPROdec");
	static ppu_static_module cellWMALSLdec("cellWMALSLdec");
	static ppu_static_module cellDTSHDCOREdec("cellDTSHDCOREdec");
	static ppu_static_module cellAtrac3multidec("cellAtrac3multidec");

	REG_FUNC(cellAdec, cellAdecQueryAttr);
	REG_FUNC(cellAdec, cellAdecOpen);
	REG_FUNC(cellAdec, cellAdecOpenEx);
	REG_FUNC(cellAdec, cellAdecOpenExt); // 0xdf982d2c
	REG_FUNC(cellAdec, cellAdecClose);
	REG_FUNC(cellAdec, cellAdecStartSeq);
	REG_FUNC(cellAdec, cellAdecEndSeq);
	REG_FUNC(cellAdec, cellAdecDecodeAu);
	REG_FUNC(cellAdec, cellAdecGetPcm);
	REG_FUNC(cellAdec, cellAdecGetPcmItem);
});
