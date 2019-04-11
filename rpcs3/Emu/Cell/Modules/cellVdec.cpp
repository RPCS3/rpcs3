#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_ppu_thread.h"
#include "sysPrxForUser.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

#include "cellPamf.h"
#include "cellVdec.h"

#include <mutex>
#include <queue>
#include <cmath>
#include "Utilities/lockless.h"
#include <variant>

std::mutex g_mutex_avcodec_open2;

LOG_CHANNEL(cellVdec);

vm::gvar<s32> _cell_vdec_prx_ver; // ???

constexpr struct vdec_start_seq_t{} vdec_start_seq{};
constexpr struct vdec_close_t{} vdec_close{};

struct vdec_cmd
{
	s32 mode;
	CellVdecAuInfo au;
};

struct vdec_frame
{
	struct frame_dtor
	{
		void operator()(AVFrame* data) const
		{
			av_frame_unref(data);
			av_frame_free(&data);
		}
	};

	std::unique_ptr<AVFrame, frame_dtor> avf;
	u64 dts;
	u64 pts;
	u64 userdata;
	u32 frc;
	bool PicItemRecieved = false;

	AVFrame* operator ->() const
	{
		return avf.get();
	}
};

struct vdec_context final
{
	static constexpr u32 id_base = 0xf0000000;
	static constexpr u32 id_step = 0x00000100;
	static constexpr u32 id_count = 1024;

	AVCodec* codec{};
	AVCodecContext* ctx{};
	SwsContext* sws{};

	shared_mutex mutex; // Used for 'out' queue (TODO)

	const u32 type;
	const u32 mem_addr;
	const u32 mem_size;
	const vm::ptr<CellVdecCbMsg> cb_func;
	const u32 cb_arg;
	u32 mem_bias{};

	u32 frc_set{}; // Frame Rate Override
	u64 next_pts{};
	u64 next_dts{};
	u64 ppu_tid{};

	std::deque<vdec_frame> out;
	atomic_t<u32> out_max = 60;

	atomic_t<u32> au_count{0};

	cond_one in_cv;
	lf_queue<std::variant<vdec_start_seq_t, vdec_close_t, vdec_cmd, CellVdecFrameRate>> in_cmd;

	vdec_context(s32 type, u32 profile, u32 addr, u32 size, vm::ptr<CellVdecCbMsg> func, u32 arg)
		: type(type)
		, mem_addr(addr)
		, mem_size(size)
		, cb_func(func)
		, cb_arg(arg)
	{
		avcodec_register_all();

		switch (type)
		{
		case CELL_VDEC_CODEC_TYPE_MPEG2:
		{
			codec = avcodec_find_decoder(AV_CODEC_ID_MPEG2VIDEO);
			break;
		}
		case CELL_VDEC_CODEC_TYPE_AVC:
		{
			codec = avcodec_find_decoder(AV_CODEC_ID_H264);
			break;
		}
		case CELL_VDEC_CODEC_TYPE_DIVX:
		{
			codec = avcodec_find_decoder(AV_CODEC_ID_MPEG4);
			break;
		}
		default:
		{
			fmt::throw_exception("Unknown video decoder type (0x%x)" HERE, type);
		}
		}

		if (!codec)
		{
			fmt::throw_exception("avcodec_find_decoder() failed (type=0x%x)" HERE, type);
		}

		ctx = avcodec_alloc_context3(codec);

		if (!ctx)
		{
			fmt::throw_exception("avcodec_alloc_context3() failed (type=0x%x)" HERE, type);
		}

		AVDictionary* opts{};
		av_dict_set(&opts, "refcounted_frames", "1", 0);

		std::lock_guard lock(g_mutex_avcodec_open2);

		int err = avcodec_open2(ctx, codec, &opts);
		if (err || opts)
		{
			avcodec_free_context(&ctx);
			fmt::throw_exception("avcodec_open2() failed (err=0x%x, opts=%d)" HERE, err, opts ? 1 : 0);
		}
	}

	~vdec_context()
	{
		avcodec_close(ctx);
		avcodec_free_context(&ctx);
		sws_freeContext(sws);
	}

	void exec(ppu_thread& ppu, u32 vid)
	{
		ppu.state -= cpu_flag::signal;
		lv2_obj::sleep(ppu);

		ppu_tid = ppu.id;

		std::unique_lock cv_lock(in_cv);

		for (auto cmds = in_cmd.pop_all(); !Emu.IsStopped(); cmds ? cmds.pop_front() : cmds = in_cmd.pop_all())
		{
			if (!cmds)
			{
				in_cv.wait(cv_lock, 1000);
				continue;
			}

			if (std::get_if<vdec_start_seq_t>(&*cmds))
			{
				avcodec_flush_buffers(ctx);

				frc_set = 0; // TODO: ???
				next_pts = 0;
				next_dts = 0;
				cellVdec.trace("Start sequence...");
			}
			else if (auto* cmd = std::get_if<vdec_cmd>(&*cmds))
			{
				AVPacket packet{};
				packet.pos = -1;

				u64 au_usrd{};

				if (cmd->mode != -1)
				{
					const u32 au_mode = cmd->mode;
					const u32 au_addr = cmd->au.startAddr;
					const u32 au_size = cmd->au.size;
					const u64 au_pts = u64{cmd->au.pts.upper} << 32 | cmd->au.pts.lower;
					const u64 au_dts = u64{cmd->au.dts.upper} << 32 | cmd->au.dts.lower;
					au_usrd = cmd->au.userData;

					packet.data = vm::_ptr<u8>(au_addr);
					packet.size = au_size;
					packet.pts = au_pts != -1 ? au_pts : AV_NOPTS_VALUE;
					packet.dts = au_dts != -1 ? au_dts : AV_NOPTS_VALUE;

					if (next_pts == 0 && au_pts != -1)
					{
						next_pts = au_pts;
					}

					if (next_dts == 0 && au_dts != -1)
					{
						next_dts = au_dts;
					}

					ctx->skip_frame =
						au_mode == CELL_VDEC_DEC_MODE_NORMAL ? AVDISCARD_DEFAULT :
						au_mode == CELL_VDEC_DEC_MODE_B_SKIP ? AVDISCARD_NONREF : AVDISCARD_NONINTRA;

					cellVdec.trace("AU decoding: size=0x%x, pts=0x%llx, dts=0x%llx, userdata=0x%llx", au_size, au_pts, au_dts, au_usrd);
				}
				else
				{
					packet.pts = AV_NOPTS_VALUE;
					packet.dts = AV_NOPTS_VALUE;
					cellVdec.trace("End sequence...");
				}

				while (out_max)
				{
					if (cmd->mode == -1)
					{
						break;
					}

					vdec_frame frame;
					frame.avf.reset(av_frame_alloc());

					if (!frame.avf)
					{
						fmt::throw_exception("av_frame_alloc() failed" HERE);
					}

					int got_picture = 0;

					int decode = avcodec_decode_video2(ctx, frame.avf.get(), &got_picture, &packet);

					if (decode < 0)
					{
						char av_error[AV_ERROR_MAX_STRING_SIZE];
						av_make_error_string(av_error, AV_ERROR_MAX_STRING_SIZE, decode);
						fmt::throw_exception("AU decoding error(0x%x): %s" HERE, decode, av_error);
					}

					if (got_picture == 0)
					{
						break;
					}

					if (decode != packet.size)
					{
						cellVdec.error("Incorrect AU size (0x%x, decoded 0x%x)", packet.size, decode);
					}

					if (got_picture)
					{
						if (frame->interlaced_frame)
						{
							// NPEB01838, NPUB31260
							cellVdec.todo("Interlaced frames not supported (0x%x)", frame->interlaced_frame);
						}

						if (frame->repeat_pict)
						{
							fmt::throw_exception("Repeated frames not supported (0x%x)", frame->repeat_pict);
						}

						if (frame->pkt_pts != AV_NOPTS_VALUE)
						{
							next_pts = frame->pkt_pts;
						}

						if (frame->pkt_dts != AV_NOPTS_VALUE)
						{
							next_dts = frame->pkt_dts;
						}

						frame.pts = next_pts;
						frame.dts = next_dts;
						frame.userdata = au_usrd;

						if (frc_set)
						{
							u64 amend = 0;

							switch (frc_set)
							{
							case CELL_VDEC_FRC_24000DIV1001: amend = 1001 * 90000 / 24000; break;
							case CELL_VDEC_FRC_24: amend = 90000 / 24; break;
							case CELL_VDEC_FRC_25: amend = 90000 / 25; break;
							case CELL_VDEC_FRC_30000DIV1001: amend = 1001 * 90000 / 30000; break;
							case CELL_VDEC_FRC_30: amend = 90000 / 30; break;
							case CELL_VDEC_FRC_50: amend = 90000 / 50; break;
							case CELL_VDEC_FRC_60000DIV1001: amend = 1001 * 90000 / 60000; break;
							case CELL_VDEC_FRC_60: amend = 90000 / 60; break;
							default:
							{
								fmt::throw_exception("Invalid frame rate code set (0x%x)" HERE, frc_set);
							}
							}

							next_pts += amend;
							next_dts += amend;
							frame.frc = frc_set;
						}
						else if (ctx->time_base.num == 0)
						{
							// Hack
							const u64 amend = u64{90000} / 30;
							frame.frc = CELL_VDEC_FRC_30;
							next_pts += amend;
							next_dts += amend;
						}
						else
						{
							u64 amend = u64{90000} * ctx->time_base.num * ctx->ticks_per_frame / ctx->time_base.den;
							const auto freq = 1. * ctx->time_base.den / ctx->time_base.num / ctx->ticks_per_frame;

							if (std::abs(freq - 23.976) < 0.002)
								frame.frc = CELL_VDEC_FRC_24000DIV1001;
							else if (std::abs(freq - 24.000) < 0.001)
								frame.frc = CELL_VDEC_FRC_24;
							else if (std::abs(freq - 25.000) < 0.001)
								frame.frc = CELL_VDEC_FRC_25;
							else if (std::abs(freq - 29.970) < 0.002)
								frame.frc = CELL_VDEC_FRC_30000DIV1001;
							else if (std::abs(freq - 30.000) < 0.001)
								frame.frc = CELL_VDEC_FRC_30;
							else if (std::abs(freq - 50.000) < 0.001)
								frame.frc = CELL_VDEC_FRC_50;
							else if (std::abs(freq - 59.940) < 0.002)
								frame.frc = CELL_VDEC_FRC_60000DIV1001;
							else if (std::abs(freq - 60.000) < 0.001)
								frame.frc = CELL_VDEC_FRC_60;
							else
							{
								// Hack
								cellVdec.error("Unsupported time_base.num (%d/%d, tpf=%d)", ctx->time_base.den, ctx->time_base.num, ctx->ticks_per_frame);
								amend = u64{90000} / 30;
								frame.frc = CELL_VDEC_FRC_30;
							}

							next_pts += amend;
							next_dts += amend;
						}

						cellVdec.trace("Got picture (pts=0x%llx[0x%llx], dts=0x%llx[0x%llx])", frame.pts, frame->pkt_pts, frame.dts, frame->pkt_dts);

						std::lock_guard{mutex}, out.push_back(std::move(frame));

						cb_func(ppu, vid, CELL_VDEC_MSG_TYPE_PICOUT, CELL_OK, cb_arg);
						lv2_obj::sleep(ppu);
					}

					if (cmd->mode != -1)
					{
						break;
					}
				}

				if (out_max)
				{
					cb_func(ppu, vid, cmd->mode != -1 ? CELL_VDEC_MSG_TYPE_AUDONE : CELL_VDEC_MSG_TYPE_SEQDONE, CELL_OK, cb_arg);
					lv2_obj::sleep(ppu);
				}

				if (cmd->mode != -1)
				{
					au_count--;
				}

				while (!Emu.IsStopped() && out_max && (std::lock_guard{mutex}, out.size() > out_max))
				{
					thread_ctrl::wait_for(1000);
				}
			}
			else if (auto* frc = std::get_if<CellVdecFrameRate>(&*cmds))
			{
				frc_set = *frc;
			}
			else
			{
				break;
			}
		}
	}
};

static void vdecEntry(ppu_thread& ppu, u32 vid)
{
	idm::get<vdec_context>(vid)->exec(ppu, vid);

	_sys_ppu_thread_exit(ppu, 0);
}

static u32 vdecQueryAttr(s32 type, u32 profile, u32 spec_addr /* may be 0 */, vm::ptr<CellVdecAttr> attr)
{
	switch (type) // TODO: check profile levels
	{
	case CELL_VDEC_CODEC_TYPE_AVC: cellVdec.warning("cellVdecQueryAttr: AVC (profile=%d)", profile); break;
	case CELL_VDEC_CODEC_TYPE_MPEG2: cellVdec.warning("cellVdecQueryAttr: MPEG2 (profile=%d)", profile); break;
	case CELL_VDEC_CODEC_TYPE_DIVX: cellVdec.warning("cellVdecQueryAttr: DivX (profile=%d)", profile); break;
	default: return CELL_VDEC_ERROR_ARG;
	}

	// TODO: check values
	attr->decoderVerLower = 0x280000; // from dmux
	attr->decoderVerUpper = 0x260000;
	attr->memSize = 4 * 1024 * 1024; // 4 MB
	attr->cmdDepth = 4;
	return CELL_OK;
}

s32 cellVdecQueryAttr(vm::cptr<CellVdecType> type, vm::ptr<CellVdecAttr> attr)
{
	cellVdec.warning("cellVdecQueryAttr(type=*0x%x, attr=*0x%x)", type, attr);

	return vdecQueryAttr(type->codecType, type->profileLevel, 0, attr);
}

s32 cellVdecQueryAttrEx(vm::cptr<CellVdecTypeEx> type, vm::ptr<CellVdecAttr> attr)
{
	cellVdec.warning("cellVdecQueryAttrEx(type=*0x%x, attr=*0x%x)", type, attr);

	return vdecQueryAttr(type->codecType, type->profileLevel, type->codecSpecificInfo_addr, attr);
}

template <typename T, typename U>
static s32 vdecOpen(ppu_thread& ppu, T type, U res, vm::cptr<CellVdecCb> cb, vm::ptr<u32> handle)
{
	if (!type || !res || !cb || !handle)
	{
		return CELL_VDEC_ERROR_ARG;
	}

	if (u32(res->ppuThreadPriority) > 3071 || u32(res->spuThreadPriority) > 255 || res->ppuThreadStackSize < 4096
		|| u32(type->codecType) > 0xd)
	{
		return CELL_VDEC_ERROR_ARG;
	}

	// Create decoder context
	const u32 vid = idm::make<vdec_context>(type->codecType, type->profileLevel, res->memAddr, res->memSize, cb->cbFunc, cb->cbArg);

	// Run thread
	vm::var<u64> _tid(0);
	vm::var<char[]> _name = vm::make_str("HLE Video Decoder");
	ppu_execute<&sys_ppu_thread_create>(ppu, +_tid, ppu_function_manager::addr + 8 * FIND_FUNC(vdecEntry), vid, +res->ppuThreadPriority, +res->ppuThreadStackSize, SYS_PPU_THREAD_CREATE_JOINABLE, +_name);

	if (ppu.test_stopped())
	{
		return 0;
	}

	verify(HERE), *_tid;
	*handle = vid;
	return CELL_OK;
}

s32 cellVdecOpen(ppu_thread& ppu, vm::cptr<CellVdecType> type, vm::cptr<CellVdecResource> res, vm::cptr<CellVdecCb> cb, vm::ptr<u32> handle)
{
	cellVdec.warning("cellVdecOpen(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	return vdecOpen(ppu, type, res, cb, handle);
}

s32 cellVdecOpenEx(ppu_thread& ppu, vm::cptr<CellVdecTypeEx> type, vm::cptr<CellVdecResourceEx> res, vm::cptr<CellVdecCb> cb, vm::ptr<u32> handle)
{
	cellVdec.warning("cellVdecOpenEx(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	return vdecOpen(ppu, type, res, cb, handle);
}

s32 cellVdecClose(ppu_thread& ppu, u32 handle)
{
	cellVdec.warning("cellVdecClose(handle=0x%x)", handle);

	const auto vdec = idm::get<vdec_context>(handle);

	if (!vdec)
	{
		return CELL_VDEC_ERROR_ARG;
	}

	vdec->out_max = 0;
	vdec->in_cmd.push(vdec_close);
	vdec->in_cv.notify();

	while (!atomic_storage<u64>::load(vdec->ppu_tid))
	{
		thread_ctrl::wait_for(1000);
	}

	vm::var<u64> ret;
	sys_ppu_thread_join(ppu, vdec->ppu_tid, +ret);

	if (!idm::remove<vdec_context>(handle))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	return CELL_OK;
}

s32 cellVdecStartSeq(u32 handle)
{
	cellVdec.trace("cellVdecStartSeq(handle=0x%x)", handle);

	const auto vdec = idm::get<vdec_context>(handle);

	if (!vdec)
	{
		return CELL_VDEC_ERROR_ARG;
	}

	vdec->in_cmd.push(vdec_start_seq);
	vdec->in_cv.notify();
	return CELL_OK;
}

s32 cellVdecEndSeq(u32 handle)
{
	cellVdec.warning("cellVdecEndSeq(handle=0x%x)", handle);

	const auto vdec = idm::get<vdec_context>(handle);

	if (!vdec)
	{
		return CELL_VDEC_ERROR_ARG;
	}

	vdec->in_cmd.push(vdec_cmd{-1});
	vdec->in_cv.notify();
	return CELL_OK;
}

s32 cellVdecDecodeAu(u32 handle, CellVdecDecodeMode mode, vm::cptr<CellVdecAuInfo> auInfo)
{
	cellVdec.trace("cellVdecDecodeAu(handle=0x%x, mode=%d, auInfo=*0x%x)", handle, (s32)mode, auInfo);

	const auto vdec = idm::get<vdec_context>(handle);

	if (mode < 0 || mode > CELL_VDEC_DEC_MODE_PB_SKIP || !vdec)
	{
		return CELL_VDEC_ERROR_ARG;
	}

	if (!vdec->au_count.try_inc(4))
	{
		return CELL_VDEC_ERROR_BUSY;
	}

	// TODO: check info
	vdec->in_cmd.push(vdec_cmd{mode, *auInfo});
	vdec->in_cv.notify();
	return CELL_OK;
}

s32 cellVdecDecodeAuEx2()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

s32 cellVdecGetPicture(u32 handle, vm::cptr<CellVdecPicFormat> format, vm::ptr<u8> outBuff)
{
	cellVdec.trace("cellVdecGetPicture(handle=0x%x, format=*0x%x, outBuff=*0x%x)", handle, format, outBuff);

	const auto vdec = idm::get<vdec_context>(handle);

	if (!format || !vdec || format->formatType > 4 || (format->formatType <= CELL_VDEC_PICFMT_RGBA32_ILV && format->colorMatrixType > CELL_VDEC_COLOR_MATRIX_TYPE_BT709))
	{
		return CELL_VDEC_ERROR_ARG;
	}

	vdec_frame frame;
	bool notify = false;
	{
		std::lock_guard lock(vdec->mutex);

		if (vdec->out.empty())
		{
			return CELL_VDEC_ERROR_EMPTY;
		}

		frame = std::move(vdec->out.front());

		vdec->out.pop_front();
		if (vdec->out.size() + 1 == vdec->out_max)
			notify = true;
	}

	if (notify)
	{
		auto vdec_ppu = idm::get<named_thread<ppu_thread>>(vdec->ppu_tid);
		thread_ctrl::notify(*vdec_ppu);
	}

	if (outBuff)
	{
		const int w = frame->width;
		const int h = frame->height;

		AVPixelFormat out_f = AV_PIX_FMT_YUV420P;

		std::unique_ptr<u8[]> alpha_plane;

		switch (const u32 type = format->formatType)
		{
		case CELL_VDEC_PICFMT_ARGB32_ILV: out_f = AV_PIX_FMT_ARGB; alpha_plane.reset(new u8[w * h]); break;
		case CELL_VDEC_PICFMT_RGBA32_ILV: out_f = AV_PIX_FMT_RGBA; alpha_plane.reset(new u8[w * h]); break;
		case CELL_VDEC_PICFMT_UYVY422_ILV: out_f = AV_PIX_FMT_UYVY422; break;
		case CELL_VDEC_PICFMT_YUV420_PLANAR: out_f = AV_PIX_FMT_YUV420P; break;

		default:
		{
			fmt::throw_exception("Unknown formatType (%d)" HERE, type);
		}
		}

		// TODO: color matrix

		if (alpha_plane)
		{
			std::memset(alpha_plane.get(), format->alpha, w * h);
		}

		AVPixelFormat in_f = AV_PIX_FMT_YUV420P;

		switch (frame->format)
		{
		case AV_PIX_FMT_YUV420P: in_f = alpha_plane ? AV_PIX_FMT_YUVA420P : AV_PIX_FMT_YUV420P; break;

		default:
		{
			fmt::throw_exception("Unknown format (%d)" HERE, frame->format);
		}
		}

		vdec->sws = sws_getCachedContext(vdec->sws, w, h, in_f, w, h, out_f, SWS_POINT, NULL, NULL, NULL);

		u8* in_data[4] = { frame->data[0], frame->data[1], frame->data[2], alpha_plane.get() };
		int in_line[4] = { frame->linesize[0], frame->linesize[1], frame->linesize[2], w * 1 };
		u8* out_data[4] = { outBuff.get_ptr() };
		int out_line[4] = { w * 4 };

		if (!alpha_plane)
		{
			out_data[1] = out_data[0] + w * h;
			out_data[2] = out_data[0] + w * h * 5 / 4;
			out_line[0] = w;
			out_line[1] = w / 2;
			out_line[2] = w / 2;
		}

		sws_scale(vdec->sws, in_data, in_line, 0, h, out_data, out_line);

		//const u32 buf_size = align(av_image_get_buffer_size(vdec->ctx->pix_fmt, vdec->ctx->width, vdec->ctx->height, 1), 128);

		//// TODO: zero padding bytes

		//int err = av_image_copy_to_buffer(outBuff.get_ptr(), buf_size, frame->data, frame->linesize, vdec->ctx->pix_fmt, frame->width, frame->height, 1);
		//if (err < 0)
		//{
		//	cellVdec.Fatal("cellVdecGetPicture: av_image_copy_to_buffer failed (err=0x%x)", err);
		//}
	}

	return CELL_OK;
}

s32 cellVdecGetPictureExt(u32 handle, vm::cptr<CellVdecPicFormat2> format2, vm::ptr<u8> outBuff, u32 arg4)
{
	cellVdec.warning("cellVdecGetPictureExt(handle=0x%x, format2=*0x%x, outBuff=*0x%x, arg4=*0x%x)", handle, format2, outBuff, arg4);

	if (arg4 || format2->unk0 || format2->unk1)
	{
		fmt::throw_exception("Unknown arguments (arg4=*0x%x, unk0=0x%x, unk1=0x%x)" HERE, arg4, format2->unk0, format2->unk1);
	}

	vm::var<CellVdecPicFormat> format;
	format->formatType = format2->formatType;
	format->colorMatrixType = format2->colorMatrixType;
	format->alpha = format2->alpha;

	return cellVdecGetPicture(handle, format, outBuff);
}

s32 cellVdecGetPicItem(u32 handle, vm::pptr<CellVdecPicItem> picItem)
{
	cellVdec.trace("cellVdecGetPicItem(handle=0x%x, picItem=**0x%x)", handle, picItem);

	const auto vdec = idm::get<vdec_context>(handle);

	if (!vdec)
	{
		return CELL_VDEC_ERROR_ARG;
	}

	AVFrame* frame{};
	u64 pts;
	u64 dts;
	u64 usrd;
	u32 frc;
	{
		std::lock_guard lock(vdec->mutex);

		for (auto& picture : vdec->out)
		{
			if (!picture.PicItemRecieved)
			{
				picture.PicItemRecieved = true;
				frame = picture.avf.get();
				pts = picture.pts;
				dts = picture.dts;
				usrd = picture.userdata;
				frc = picture.frc;
				break;
			}
		}
	}

	if (!frame)
	{
		// If frame is empty info was not found
		return CELL_VDEC_ERROR_EMPTY;
	}

	const vm::ptr<CellVdecPicItem> info = vm::cast(vdec->mem_addr + vdec->mem_bias);

	vdec->mem_bias += 512;
	if (vdec->mem_bias + 512 > vdec->mem_size)
	{
		vdec->mem_bias = 0;
	}

	info->codecType = vdec->type;
	info->startAddr = 0x00000123; // invalid value (no address for picture)
	info->size = align(av_image_get_buffer_size(vdec->ctx->pix_fmt, vdec->ctx->width, vdec->ctx->height, 1), 128);
	info->auNum = 1;
	info->auPts[0].lower = (u32)(pts);
	info->auPts[0].upper = (u32)(pts >> 32);
	info->auPts[1].lower = (u32)CODEC_TS_INVALID;
	info->auPts[1].upper = (u32)CODEC_TS_INVALID;
	info->auDts[0].lower = (u32)(dts);
	info->auDts[0].upper = (u32)(dts >> 32);
	info->auDts[1].lower = (u32)CODEC_TS_INVALID;
	info->auDts[1].upper = (u32)CODEC_TS_INVALID;
	info->auUserData[0] = usrd;
	info->auUserData[1] = 0;
	info->status = CELL_OK;
	info->attr = CELL_VDEC_PICITEM_ATTR_NORMAL;
	info->picInfo_addr = info.addr() + u32{sizeof(CellVdecPicItem)};

	if (vdec->type == CELL_VDEC_CODEC_TYPE_AVC)
	{
		const vm::ptr<CellVdecAvcInfo> avc = vm::cast(info.addr() + u32{sizeof(CellVdecPicItem)});

		avc->horizontalSize = frame->width;
		avc->verticalSize = frame->height;

		switch (s32 pct = frame->pict_type)
		{
		case AV_PICTURE_TYPE_I: avc->pictureType[0] = CELL_VDEC_AVC_PCT_I; break;
		case AV_PICTURE_TYPE_P: avc->pictureType[0] = CELL_VDEC_AVC_PCT_P; break;
		case AV_PICTURE_TYPE_B: avc->pictureType[0] = CELL_VDEC_AVC_PCT_B; break;
		default: cellVdec.error("cellVdecGetPicItem(AVC): unknown pict_type value (0x%x)", pct);
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

		switch (frc)
		{
		case CELL_VDEC_FRC_24000DIV1001: avc->frameRateCode = CELL_VDEC_AVC_FRC_24000DIV1001; break;
		case CELL_VDEC_FRC_24: avc->frameRateCode = CELL_VDEC_AVC_FRC_24; break;
		case CELL_VDEC_FRC_25: avc->frameRateCode = CELL_VDEC_AVC_FRC_25; break;
		case CELL_VDEC_FRC_30000DIV1001: avc->frameRateCode = CELL_VDEC_AVC_FRC_30000DIV1001; break;
		case CELL_VDEC_FRC_30: avc->frameRateCode = CELL_VDEC_AVC_FRC_30; break;
		case CELL_VDEC_FRC_50: avc->frameRateCode = CELL_VDEC_AVC_FRC_50; break;
		case CELL_VDEC_FRC_60000DIV1001: avc->frameRateCode = CELL_VDEC_AVC_FRC_60000DIV1001; break;
		case CELL_VDEC_FRC_60: avc->frameRateCode = CELL_VDEC_AVC_FRC_60; break;
		default: cellVdec.error("cellVdecGetPicItem(AVC): unknown frc value (0x%x)", frc);
		}

		avc->fixed_frame_rate_flag = true;
		avc->low_delay_hrd_flag = true; // ???
		avc->entropy_coding_mode_flag = true; // ???
		avc->nalUnitPresentFlags = 0; // ???
		avc->ccDataLength[0] = 0;
		avc->ccDataLength[1] = 0;
		avc->reserved[0] = 0;
		avc->reserved[1] = 0;
	}
	else if (vdec->type == CELL_VDEC_CODEC_TYPE_DIVX)
	{
		const vm::ptr<CellVdecDivxInfo> dvx = vm::cast(info.addr() + u32{sizeof(CellVdecPicItem)});

		switch (s32 pct = frame->pict_type)
		{
		case AV_PICTURE_TYPE_I: dvx->pictureType = CELL_VDEC_DIVX_VCT_I; break;
		case AV_PICTURE_TYPE_P: dvx->pictureType = CELL_VDEC_DIVX_VCT_P; break;
		case AV_PICTURE_TYPE_B: dvx->pictureType = CELL_VDEC_DIVX_VCT_B; break;
		default: cellVdec.error("cellVdecGetPicItem(DivX): unknown pict_type value (0x%x)", pct);
		}

		dvx->horizontalSize = frame->width;
		dvx->verticalSize = frame->height;
		dvx->pixelAspectRatio = CELL_VDEC_DIVX_ARI_PAR_1_1; // ???
		dvx->parHeight = 0;
		dvx->parWidth = 0;
		dvx->colourDescription = false; // ???
		dvx->colourPrimaries = CELL_VDEC_DIVX_CP_ITU_R_BT_709; // ???
		dvx->transferCharacteristics = CELL_VDEC_DIVX_TC_ITU_R_BT_709; // ???
		dvx->matrixCoefficients = CELL_VDEC_DIVX_MXC_ITU_R_BT_709; // ???
		dvx->pictureStruct = CELL_VDEC_DIVX_PSTR_FRAME; // ???

		switch (frc)
		{
		case CELL_VDEC_FRC_24000DIV1001: dvx->frameRateCode = CELL_VDEC_DIVX_FRC_24000DIV1001; break;
		case CELL_VDEC_FRC_24: dvx->frameRateCode = CELL_VDEC_DIVX_FRC_24; break;
		case CELL_VDEC_FRC_25: dvx->frameRateCode = CELL_VDEC_DIVX_FRC_25; break;
		case CELL_VDEC_FRC_30000DIV1001: dvx->frameRateCode = CELL_VDEC_DIVX_FRC_30000DIV1001; break;
		case CELL_VDEC_FRC_30: dvx->frameRateCode = CELL_VDEC_DIVX_FRC_30; break;
		case CELL_VDEC_FRC_50: dvx->frameRateCode = CELL_VDEC_DIVX_FRC_50; break;
		case CELL_VDEC_FRC_60000DIV1001: dvx->frameRateCode = CELL_VDEC_DIVX_FRC_60000DIV1001; break;
		case CELL_VDEC_FRC_60: dvx->frameRateCode = CELL_VDEC_DIVX_FRC_60; break;
		default: cellVdec.error("cellVdecGetPicItem(DivX): unknown frc value (0x%x)", frc);
		}
	}
	else if (vdec->type == CELL_VDEC_CODEC_TYPE_MPEG2)
	{
		const vm::ptr<CellVdecMpeg2Info> mp2 = vm::cast(info.addr() + u32{sizeof(CellVdecPicItem)});

		std::memset(mp2.get_ptr(), 0, sizeof(CellVdecMpeg2Info));
		mp2->horizontal_size = frame->width;
		mp2->vertical_size = frame->height;
		mp2->aspect_ratio_information = CELL_VDEC_MPEG2_ARI_SAR_1_1; // ???

		switch (frc)
		{
		case CELL_VDEC_FRC_24000DIV1001: mp2->frame_rate_code = CELL_VDEC_MPEG2_FRC_24000DIV1001; break;
		case CELL_VDEC_FRC_24: mp2->frame_rate_code = CELL_VDEC_MPEG2_FRC_24; break;
		case CELL_VDEC_FRC_25: mp2->frame_rate_code = CELL_VDEC_MPEG2_FRC_25; break;
		case CELL_VDEC_FRC_30000DIV1001: mp2->frame_rate_code = CELL_VDEC_MPEG2_FRC_30000DIV1001; break;
		case CELL_VDEC_FRC_30: mp2->frame_rate_code = CELL_VDEC_MPEG2_FRC_30; break;
		case CELL_VDEC_FRC_50: mp2->frame_rate_code = CELL_VDEC_MPEG2_FRC_50; break;
		case CELL_VDEC_FRC_60000DIV1001: mp2->frame_rate_code = CELL_VDEC_MPEG2_FRC_60000DIV1001; break;
		case CELL_VDEC_FRC_60: mp2->frame_rate_code = CELL_VDEC_MPEG2_FRC_60; break;
		default: cellVdec.error("cellVdecGetPicItem(MPEG2): unknown frc value (0x%x)", frc);
		}

		mp2->progressive_sequence = true; // ???
		mp2->low_delay = true; // ???
		mp2->video_format = CELL_VDEC_MPEG2_VF_UNSPECIFIED; // ???
		mp2->colour_description = false; // ???

		switch (s32 pct = frame->pict_type)
		{
		case AV_PICTURE_TYPE_I: mp2->picture_coding_type[0] = CELL_VDEC_MPEG2_PCT_I; break;
		case AV_PICTURE_TYPE_P: mp2->picture_coding_type[0] = CELL_VDEC_MPEG2_PCT_P; break;
		case AV_PICTURE_TYPE_B: mp2->picture_coding_type[0] = CELL_VDEC_MPEG2_PCT_B; break;
		default: cellVdec.error("cellVdecGetPicItem(MPEG2): unknown pict_type value (0x%x)", pct);
		}

		mp2->picture_coding_type[1] = CELL_VDEC_MPEG2_PCT_FORBIDDEN; // ???
		mp2->picture_structure[0] = CELL_VDEC_MPEG2_PSTR_FRAME;
		mp2->picture_structure[1] = CELL_VDEC_MPEG2_PSTR_FRAME;

		// ...
	}

	*picItem = info;
	return CELL_OK;
}

s32 cellVdecSetFrameRate(u32 handle, CellVdecFrameRate frc)
{
	cellVdec.trace("cellVdecSetFrameRate(handle=0x%x, frc=0x%x)", handle, (s32)frc);

	const auto vdec = idm::get<vdec_context>(handle);

	if (!vdec)
	{
		return CELL_VDEC_ERROR_ARG;
	}

	// TODO: check frc value
	vdec->in_cmd.push(frc);
	vdec->in_cv.notify();
	return CELL_OK;
}

s32 cellVdecOpenExt()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

s32 cellVdecStartSeqExt()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

s32 cellVdecGetPicItemEx()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

s32 cellVdecGetPicItemExt()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

s32 cellVdecSetFrameRateExt()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

s32 cellVdecSetPts()
{
	UNIMPLEMENTED_FUNC(cellVdec);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellVdec)("libvdec", []()
{
	static ppu_static_module libavcdec("libavcdec");
	static ppu_static_module libdivx311dec("libdivx311dec");
	static ppu_static_module libdivxdec("libdivxdec");
	static ppu_static_module libmvcdec("libmvcdec");
	static ppu_static_module libsjvtd("libsjvtd");
	static ppu_static_module libsmvd2("libsmvd2");
	static ppu_static_module libsmvd4("libsmvd4");
	static ppu_static_module libsvc1d("libsvc1d");

	REG_VAR(libvdec, _cell_vdec_prx_ver); // 0x085a7ecb

	REG_FUNC(libvdec, cellVdecQueryAttr);
	REG_FUNC(libvdec, cellVdecQueryAttrEx);
	REG_FUNC(libvdec, cellVdecOpen);
	REG_FUNC(libvdec, cellVdecOpenEx);
	REG_FUNC(libvdec, cellVdecOpenExt); // 0xef4d8ad7
	REG_FUNC(libvdec, cellVdecClose);
	REG_FUNC(libvdec, cellVdecStartSeq);
	REG_FUNC(libvdec, cellVdecStartSeqExt); // 0xebb8e70a
	REG_FUNC(libvdec, cellVdecEndSeq);
	REG_FUNC(libvdec, cellVdecDecodeAu);
	REG_FUNC(libvdec, cellVdecDecodeAuEx2);
	REG_FUNC(libvdec, cellVdecGetPicture);
	REG_FUNC(libvdec, cellVdecGetPictureExt); // 0xa21aa896
	REG_FUNC(libvdec, cellVdecGetPicItem);
	REG_FUNC(libvdec, cellVdecGetPicItemEx);
	REG_FUNC(libvdec, cellVdecGetPicItemExt); // 0x2cbd9806
	REG_FUNC(libvdec, cellVdecSetFrameRate);
	REG_FUNC(libvdec, cellVdecSetFrameRateExt); // 0xcffc42a5
	REG_FUNC(libvdec, cellVdecSetPts); // 0x3ce2e4f8

	REG_FUNC(libvdec, vdecEntry).flag(MFF_HIDDEN);
});
