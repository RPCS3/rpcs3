#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

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

//void cellAdec_init();
//Module cellAdec(0x0006, cellAdec_init);
Module *cellAdec = nullptr;

AudioDecoder::AudioDecoder(AudioCodecType type, u32 addr, u32 size, u32 func, u32 arg)
	: type(type)
	, memAddr(addr)
	, memSize(size)
	, memBias(0)
	, cbFunc(func)
	, cbArg(arg)
	, adecCb(nullptr)
	, is_running(false)
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
	fmt->pb = avio_alloc_context(io_buf, 4096, 0, this, adecRead, NULL, NULL);
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
	if (ctx)
	{
		for (u32 i = frames.GetCount() - 1; ~i; i--)
		{
			AdecFrame& af = frames.Peek(i);
			av_frame_unref(af.data);
			av_frame_free(&af.data);
		}
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

int adecRawRead(void* opaque, u8* buf, int buf_size)
{
	AudioDecoder& adec = *(AudioDecoder*)opaque;

	int res = 0;

next:
	if (adec.reader.size < (u32)buf_size /*&& !adec.just_started*/)
	{
		while (!adec.job.GetCountUnsafe())
		{
			if (Emu.IsStopped())
			{
				cellAdec->Warning("adecRawRead(): aborted");
				return 0;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		switch (adec.job.Peek().type)
		{
		case adecEndSeq:
		case adecClose:
			{
				buf_size = adec.reader.size;
			}
			break;
		case adecDecodeAu:
			{
				memcpy(buf, Memory + adec.reader.addr, adec.reader.size);

				buf += adec.reader.size;
				buf_size -= adec.reader.size;
				res += adec.reader.size;

				adec.adecCb->ExecAsCallback(adec.cbFunc, false, adec.id, CELL_ADEC_MSG_TYPE_AUDONE, adec.task.au.auInfo_addr, adec.cbArg);

				adec.job.Pop(adec.task);

				adec.reader.addr = adec.task.au.addr;
				adec.reader.size = adec.task.au.size;
				//LOG_NOTICE(HLE, "Audio AU: size = 0x%x, pts = 0x%llx", adec.task.au.size, adec.task.au.pts);
			}
			break;
		default:
			cellAdec->Error("adecRawRead(): sequence error (task %d)", adec.job.Peek().type);
			return -1;
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
		memcpy(buf, Memory + adec.reader.addr, buf_size);

		adec.reader.addr += buf_size;
		adec.reader.size -= buf_size;
		return res + buf_size;
	}
}

int adecRead(void* opaque, u8* buf, int buf_size)
{
	AudioDecoder& adec = *(AudioDecoder*)opaque;

	int res = 0;

	if (adec.reader.rem_size && adec.reader.rem)
	{
		if (buf_size < (int)adec.reader.rem_size)
		{
			cellAdec->Error("adecRead(): too small buf_size (rem_size = %d, buf_size = %d)", adec.reader.rem_size, buf_size);
			Emu.Pause();
			return 0;
		}

		memcpy(buf, adec.reader.rem, adec.reader.rem_size);
		free(adec.reader.rem);
		adec.reader.rem = nullptr;
		buf += adec.reader.rem_size;
		buf_size -= adec.reader.rem_size;
		res += adec.reader.rem_size;
		adec.reader.rem_size = 0;
	}

	while (buf_size)
	{
		u8 header[8];
		if (adecRawRead(opaque, header, 8) < 8) break;
		if (header[0] != 0x0f || header[1] != 0xd0)
		{
			cellAdec->Error("adecRead(): 0x0FD0 header not found");
			Emu.Pause();
			return -1;
		}

		if (!adec.reader.init)
		{
			OMAHeader oma(1 /* atrac3p id */, header[2], header[3]);
			if (buf_size < sizeof(oma) + 8)
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
		else
		{
		}

		u32 size = (((header[2] & 0x3) << 8) | header[3]) * 8 + 8; // data to be read before next header

		//LOG_NOTICE(HLE, "*** audio block read: size = 0x%x", size);

		if (buf_size < (int)size)
		{
			if (adecRawRead(opaque, buf, buf_size) < buf_size) break; // ???
			res += buf_size;
			size -= buf_size;
			buf_size = 0;
			
			adec.reader.rem = (u8*)malloc(size);
			adec.reader.rem_size = size;
			if (adecRawRead(opaque, adec.reader.rem, size) < (int)size) break; // ???
		}
		else
		{
			if (adecRawRead(opaque, buf, size) < (int)size) break; // ???
			buf += size;
			buf_size -= size;
			res += size;
		}
	}

	return res;
}

u32 adecOpen(AudioDecoder* data)
{
	AudioDecoder& adec = *data;

	adec.adecCb = &Emu.GetCPU().AddThread(CPU_THREAD_PPU);

	u32 adec_id = cellAdec->GetNewId(data);

	adec.id = adec_id;

	adec.adecCb->SetName("Audio Decoder[" + std::to_string(adec_id) + "] Callback");

	thread t("Audio Decoder[" + std::to_string(adec_id) + "] Thread", [&]()
	{
		cellAdec->Notice("Audio Decoder thread started");

		AdecTask& task = adec.task;

		while (true)
		{
			if (Emu.IsStopped())
			{
				break;
			}

			if (!adec.job.GetCountUnsafe() && adec.is_running)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			/*if (adec.frames.GetCount() >= 50)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}*/

			if (!adec.job.Pop(task))
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
				if (adec.reader.rem) free(adec.reader.rem);
				adec.reader.rem = nullptr;
				adec.reader.rem_size = 0;
				adec.is_running = true;
				adec.just_started = true;
			}
			break;

			case adecEndSeq:
			{
				// TODO: finalize
				cellAdec->Warning("adecEndSeq:");

				/*Callback cb;
				cb.SetAddr(adec.cbFunc);
				cb.Handle(adec.id, CELL_ADEC_MSG_TYPE_SEQDONE, CELL_OK, adec.cbArg);
				cb.Branch(true); // ???*/
				adec.adecCb->ExecAsCallback(adec.cbFunc, true, adec.id, CELL_ADEC_MSG_TYPE_SEQDONE, CELL_OK, adec.cbArg);

				adec.is_running = false;
				adec.just_finished = true;
			}
			break;

			case adecDecodeAu:
			{
				int err = 0;

				adec.reader.addr = task.au.addr;
				adec.reader.size = task.au.size;
				//LOG_NOTICE(HLE, "Audio AU: size = 0x%x, pts = 0x%llx", task.au.size, task.au.pts);

				if (adec.just_started)
				{
					adec.first_pts = task.au.pts;
					adec.last_pts = task.au.pts - 0x10000; // hack
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
						//av_free_packet(this);
					}

				} au(0);

				/*{
					wxFile dump;
					dump.Open(wxString::Format("audio pts-0x%llx.dump", task.au.pts), wxFile::write);
					u8* buf = (u8*)malloc(task.au.size);
					if (Memory.CopyToReal(buf, task.au.addr, task.au.size)) dump.Write(buf, task.au.size);
					free(buf);
					dump.Close();
				}*/

				if (adec.just_started && adec.just_finished)
				{
					avcodec_flush_buffers(adec.ctx);
					adec.reader.init = true;
					adec.just_finished = false;
					adec.just_started = false;
				}
				else if (adec.just_started) // deferred initialization
				{
					err = avformat_open_input(&adec.fmt, NULL, av_find_input_format("oma"), NULL);
					if (err)
					{
						cellAdec->Error("adecDecodeAu: avformat_open_input() failed");
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

					AVDictionary* opts = nullptr;
					av_dict_set(&opts, "refcounted_frames", "1", 0);
					{
						std::lock_guard<std::mutex> lock(g_mutex_avcodec_open2);
						// not multithread-safe (???)
						err = avcodec_open2(adec.ctx, codec, &opts);
					}
					if (err)
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
					if (Emu.IsStopped())
					{
						cellAdec->Warning("adecDecodeAu: aborted");
						return;
					}

					/*if (!adec.ctx) // fake
					{
						AdecFrame frame;
						frame.pts = task.au.pts;
						frame.auAddr = task.au.addr;
						frame.auSize = task.au.size;
						frame.userdata = task.au.userdata;
						frame.size = 4096;
						frame.data = nullptr;
						adec.frames.Push(frame);

						adec.adecCb->ExecAsCallback(adec.cbFunc, false, adec.id, CELL_ADEC_MSG_TYPE_PCMOUT, CELL_OK, adec.cbArg);

						break;
					}*/

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
							adec.last_pts += ((u64)frame.data->nb_samples) * 90000 / 48000;
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
						if (frame.data->channels != 2)
						{
							cellAdec->Error("adecDecodeAu: unsupported channel count (%d)", frame.data->channels);
							Emu.Pause();
							break;
						}

						//LOG_NOTICE(HLE, "got audio frame (pts=0x%llx, nb_samples=%d, ch=%d, sample_rate=%d, nbps=%d)",
							//frame.pts, frame.data->nb_samples, frame.data->channels, frame.data->sample_rate,
							//av_get_bytes_per_sample((AVSampleFormat)frame.data->format));

						adec.frames.Push(frame);
						frame.data = nullptr; // to prevent destruction

						/*Callback cb;
						cb.SetAddr(adec.cbFunc);
						cb.Handle(adec.id, CELL_ADEC_MSG_TYPE_PCMOUT, CELL_OK, adec.cbArg);
						cb.Branch(false);*/
						adec.adecCb->ExecAsCallback(adec.cbFunc, false, adec.id, CELL_ADEC_MSG_TYPE_PCMOUT, CELL_OK, adec.cbArg);
					}
				}
						
				/*Callback cb;
				cb.SetAddr(adec.cbFunc);
				cb.Handle(adec.id, CELL_ADEC_MSG_TYPE_AUDONE, task.au.auInfo_addr, adec.cbArg);
				cb.Branch(false);*/
				adec.adecCb->ExecAsCallback(adec.cbFunc, false, adec.id, CELL_ADEC_MSG_TYPE_AUDONE, task.au.auInfo_addr, adec.cbArg);
			}
			break;

			case adecClose:
			{
				adec.is_finished = true;
				cellAdec->Notice("Audio Decoder thread ended");
				return;
			}

			default:
				cellAdec->Error("Audio Decoder thread error: unknown task(%d)", task.type);
			}
		}
		adec.is_finished = true;
		cellAdec->Warning("Audio Decoder thread aborted");
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

int cellAdecQueryAttr(mem_ptr_t<CellAdecType> type, mem_ptr_t<CellAdecAttr> attr)
{
	cellAdec->Warning("cellAdecQueryAttr(type_addr=0x%x, attr_addr=0x%x)", type.GetAddr(), attr.GetAddr());

	if (!adecCheckType(type->audioCodecType)) return CELL_ADEC_ERROR_ARG;

	// TODO: check values
	attr->adecVerLower = 0x280000; // from dmux
	attr->adecVerUpper = 0x260000;
	attr->workMemSize = 4 * 1024 * 1024;

	return CELL_OK;
}

int cellAdecOpen(mem_ptr_t<CellAdecType> type, mem_ptr_t<CellAdecResource> res, mem_ptr_t<CellAdecCb> cb, mem32_t handle)
{
	cellAdec->Warning("cellAdecOpen(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)", 
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());

	if (!adecCheckType(type->audioCodecType)) return CELL_ADEC_ERROR_ARG;

	handle = adecOpen(new AudioDecoder(type->audioCodecType, res->startAddr, res->totalMemSize, cb->cbFunc, cb->cbArg));

	return CELL_OK;
}

int cellAdecOpenEx(mem_ptr_t<CellAdecType> type, mem_ptr_t<CellAdecResourceEx> res, mem_ptr_t<CellAdecCb> cb, mem32_t handle)
{
	cellAdec->Warning("cellAdecOpenEx(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)", 
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());

	if (!adecCheckType(type->audioCodecType)) return CELL_ADEC_ERROR_ARG;

	handle = adecOpen(new AudioDecoder(type->audioCodecType, res->startAddr, res->totalMemSize, cb->cbFunc, cb->cbArg));

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

	adec->job.Push(AdecTask(adecClose));

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
	cellAdec->Log("cellAdecStartSeq(handle=%d, param_addr=0x%x)", handle, param_addr);

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecTask task(adecStartSeq);
	/*if (adec->type == CELL_ADEC_TYPE_ATRACX_2CH)
	{

	}
	else*/
	{
		cellAdec->Todo("cellAdecStartSeq(): initialization");
	}
	
	adec->job.Push(task);
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

	adec->job.Push(AdecTask(adecEndSeq));
	return CELL_OK;
}

int cellAdecDecodeAu(u32 handle, mem_ptr_t<CellAdecAuInfo> auInfo)
{
	cellAdec->Log("cellAdecDecodeAu(handle=%d, auInfo_addr=0x%x)", handle, auInfo.GetAddr());

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	AdecTask task(adecDecodeAu);
	task.au.auInfo_addr = auInfo.GetAddr();
	task.au.addr = auInfo->startAddr;
	task.au.size = auInfo->size;
	task.au.pts = ((u64)auInfo->pts.upper << 32) | (u64)auInfo->pts.lower;
	task.au.userdata = auInfo->userData;

	adec->job.Push(task);
	return CELL_OK;
}

int cellAdecGetPcm(u32 handle, u32 outBuffer_addr)
{
	cellAdec->Log("cellAdecGetPcm(handle=%d, outBuffer_addr=0x%x)", handle, outBuffer_addr);

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	if (adec->frames.IsEmpty())
	{
		return CELL_ADEC_ERROR_EMPTY;
	}

	AdecFrame af;
	adec->frames.Pop(af);
	AVFrame* frame = af.data;

	if (!af.data) // fake: empty data
	{
		return CELL_OK;
	}

	// reverse byte order, extract data:
	float* in_f[2];
	in_f[0] = (float*)frame->extended_data[0];
	in_f[1] = (float*)frame->extended_data[1];
	be_t<float>* out_f = (be_t<float>*)Memory.GetMemFromAddr(outBuffer_addr);
	for (u32 i = 0; i < af.size / 8; i++)
	{
		out_f[i*2] = in_f[0][i];
		out_f[i*2+1] = in_f[1][i];
	}

	if (af.data)
	{
		av_frame_unref(af.data);
		av_frame_free(&af.data);
	}
	return CELL_OK;
}

int cellAdecGetPcmItem(u32 handle, mem32_t pcmItem_ptr)
{
	cellAdec->Log("cellAdecGetPcmItem(handle=%d, pcmItem_ptr_addr=0x%x)", handle, pcmItem_ptr.GetAddr());

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	if (adec->frames.IsEmpty())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
		return CELL_ADEC_ERROR_EMPTY;
	}

	AdecFrame& af = adec->frames.Peek();

	AVFrame* frame = af.data;

	mem_ptr_t<CellAdecPcmItem> pcm(adec->memAddr + adec->memBias);

	adec->memBias += 512;
	if (adec->memBias + 512 > adec->memSize)
	{
		adec->memBias = 0;
	}

	pcm->pcmHandle = 0; // ???
	pcm->pcmAttr.bsiInfo_addr = pcm.GetAddr() + sizeof(CellAdecPcmItem);
	pcm->startAddr = 0x00000312; // invalid address (no output)
	pcm->size = af.size;
	pcm->status = CELL_OK;
	pcm->auInfo.pts.lower = af.pts;
	pcm->auInfo.pts.upper = af.pts >> 32;
	pcm->auInfo.size = af.auSize;
	pcm->auInfo.startAddr = af.auAddr;
	pcm->auInfo.userData = af.userdata;

	mem_ptr_t<CellAdecAtracXInfo> atx(pcm.GetAddr() + sizeof(CellAdecPcmItem));
	atx->samplingFreq = frame->sample_rate; // ???
	atx->nbytes = frame->nb_samples * frame->channels * sizeof(float); // ???
	atx->channelConfigIndex = CELL_ADEC_CH_STEREO; // ???

	pcmItem_ptr = pcm.GetAddr();

	return CELL_OK;
}

void cellAdec_init()
{
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
