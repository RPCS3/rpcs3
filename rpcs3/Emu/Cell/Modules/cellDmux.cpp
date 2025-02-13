#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_sync.h"

#include "cellPamf.h"
#include "cellDmux.h"

#include "util/asm.hpp"

#include <thread>

LOG_CHANNEL(cellDmux);

template <>
void fmt_class_string<CellDmuxError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellDmuxError value)
	{
		switch (value)
		{
			STR_CASE(CELL_DMUX_ERROR_ARG);
			STR_CASE(CELL_DMUX_ERROR_SEQ);
			STR_CASE(CELL_DMUX_ERROR_BUSY);
			STR_CASE(CELL_DMUX_ERROR_EMPTY);
			STR_CASE(CELL_DMUX_ERROR_FATAL);
		}

		return unknown;
	});
}

/* Demuxer Thread Classes */

enum
{
	/* http://dvd.sourceforge.net/dvdinfo/mpeghdrs.html */

	PACKET_START_CODE_MASK   = 0xffffff00,
	PACKET_START_CODE_PREFIX = 0x00000100,

	PACK_START_CODE          = 0x000001ba,
	SYSTEM_HEADER_START_CODE = 0x000001bb,
	PRIVATE_STREAM_1         = 0x000001bd,
	PADDING_STREAM           = 0x000001be,
	PRIVATE_STREAM_2         = 0x000001bf,
};

struct DemuxerStream
{
	u32 addr;
	u32 size;
	u64 userdata;
	bool discontinuity;

	template<typename T>
	bool get(T& out)
	{
		if (sizeof(T) > size) return false;

		std::memcpy(&out, vm::base(addr), sizeof(T));
		addr += sizeof(T);
		size -= sizeof(T);

		return true;
	}

	template<typename T>
	bool peek(T& out, u32 shift = 0)
	{
		if (sizeof(T) + shift > size) return false;

		std::memcpy(&out, vm::base(addr + shift), sizeof(T));
		return true;
	}

	void skip(u32 count)
	{
		addr += count;
		size = size > count ? size - count : 0;
	}

	bool check(u32 count) const
	{
		return count <= size;
	}

	u64 get_ts(u8 c)
	{
		u8 v[4]; get(v);
		return
			((u64{c} & 0x0e) << 29) |
			((u64{v[0]}) << 21) |
			((u64{v[1]} & 0x7e) << 15) |
			((u64{v[2]}) << 7) | (u64{v[3]} >> 1);
	}
};

struct PesHeader
{
	u64 pts;
	u64 dts;
	u8 size;
	bool has_ts;
	bool is_ok;

	PesHeader(DemuxerStream& stream);
};

class ElementaryStream;
class Demuxer;

enum DemuxerJobType
{
	dmuxSetStream,
	dmuxResetStream,
	dmuxResetStreamAndWaitDone,
	dmuxEnableEs,
	dmuxDisableEs,
	dmuxResetEs,
	dmuxFlushEs,
	dmuxClose,
};

struct DemuxerTask
{
	DemuxerJobType type;

	union
	{
		DemuxerStream stream;

		struct
		{
			u32 es;
			u32 auInfo_ptr_addr;
			u32 auSpec_ptr_addr;
			ElementaryStream* es_ptr;
		} es;
	};

	DemuxerTask()
	{
	}

	DemuxerTask(DemuxerJobType type)
		: type(type)
	{
	}
};

class ElementaryStream
{
	std::mutex m_mutex;

	squeue_t<u32> entries; // AU starting addresses
	u32 put_count = 0; // number of AU written
	u32 got_count = 0; // number of AU obtained by GetAu(Ex)
	u32 released = 0; // number of AU released

	u32 put; // AU that is being written now

	bool is_full(u32 space);

public:
	static const u32 id_base = 1;
	static const u32 id_step = 1;
	static const u32 id_count = 1023;
	SAVESTATE_INIT_POS(34);

	ElementaryStream(Demuxer* dmux, u32 addr, u32 size, u32 fidMajor, u32 fidMinor, u32 sup1, u32 sup2, vm::ptr<CellDmuxCbEsMsg> cbFunc, u32 cbArg, u32 spec);

	Demuxer* dmux;
	const u32 id = idm::last_id();
	const u32 memAddr;
	const u32 memSize;
	const u32 fidMajor;
	const u32 fidMinor;
	const u32 sup1;
	const u32 sup2;
	const vm::ptr<CellDmuxCbEsMsg> cbFunc;
	const u32 cbArg;
	const u32 spec; //addr

	std::vector<u8> raw_data; // demultiplexed data stream (managed by demuxer thread)
	usz raw_pos = 0; // should be <= raw_data.size()
	u64 last_dts = CODEC_TS_INVALID;
	u64 last_pts = CODEC_TS_INVALID;

	void push(DemuxerStream& stream, u32 size); // called by demuxer thread (not multithread-safe)

	bool isfull(u32 space);

	void push_au(u32 size, u64 dts, u64 pts, u64 userdata, bool rap, u32 specific);

	bool release();

	bool peek(u32& out_data, bool no_ex, u32& out_spec, bool update_index);

	void reset();
};

class Demuxer : public ppu_thread
{
public:
	squeue_t<DemuxerTask, 32> job;
	const u32 memAddr;
	const u32 memSize;
	const vm::ptr<CellDmuxCbMsg> cbFunc;
	const u32 cbArg;
	volatile bool is_finished = false;
	volatile bool is_closed = false;
	atomic_t<bool> is_running = false;
	atomic_t<bool> is_working = false;

	Demuxer(u32 addr, u32 size, vm::ptr<CellDmuxCbMsg> func, u32 arg)
		: ppu_thread({}, "", 0)
		, memAddr(addr)
		, memSize(size)
		, cbFunc(func)
		, cbArg(arg)
	{
	}

	void non_task()
	{
		DemuxerTask task;
		DemuxerStream stream = {};
		ElementaryStream* esALL[96]{};
		ElementaryStream** esAVC = &esALL[0]; // AVC (max 16 minus M2V count)
		//ElementaryStream** esM2V = &esALL[16]; // M2V (max 16 minus AVC count)
		//ElementaryStream** esDATA = &esALL[32]; // user data (max 16)
		ElementaryStream** esATX = &esALL[48]; // ATRAC3+ (max 16)
		//ElementaryStream** esAC3 = &esALL[64]; // AC3 (max 16)
		//ElementaryStream** esPCM = &esALL[80]; // LPCM (max 16)

		u32 cb_add = 0;

		while (true)
		{
			if (Emu.IsStopped() || is_closed)
			{
				break;
			}

			if (!job.try_peek(task) && is_running && stream.addr)
			{
				// default task (demuxing) (if there is no other work)
				be_t<u32> code;
				be_t<u16> len;

				if (!stream.peek(code))
				{
					// demuxing finished
					is_running = false;

					// callback
					auto dmuxMsg = vm::ptr<CellDmuxMsg>::make(memAddr + (cb_add ^= 16));
					dmuxMsg->msgType = CELL_DMUX_MSG_TYPE_DEMUX_DONE;
					dmuxMsg->supplementalInfo = stream.userdata;
					cbFunc(*this, id, dmuxMsg, cbArg);
					lv2_obj::sleep(*this);

					is_working = false;

					stream = {};

					continue;
				}

				switch (code)
				{
				case PACK_START_CODE:
				{
					if (!stream.check(14))
					{
						fmt::throw_exception("End of stream (PACK_START_CODE)");
					}
					stream.skip(14);
					break;
				}

				case SYSTEM_HEADER_START_CODE:
				{
					if (!stream.check(18))
					{
						fmt::throw_exception("End of stream (SYSTEM_HEADER_START_CODE)");
					}
					stream.skip(18);
					break;
				}

				case PADDING_STREAM:
				{
					if (!stream.check(6))
					{
						fmt::throw_exception("End of stream (PADDING_STREAM)");
					}
					stream.skip(4);
					stream.get(len);

					if (!stream.check(len))
					{
						fmt::throw_exception("End of stream (PADDING_STREAM, len=%d)", len);
					}
					stream.skip(len);
					break;
				}

				case PRIVATE_STREAM_2:
				{
					if (!stream.check(6))
					{
						fmt::throw_exception("End of stream (PRIVATE_STREAM_2)");
					}
					stream.skip(4);
					stream.get(len);

					cellDmux.notice("PRIVATE_STREAM_2 (%d)", len);

					if (!stream.check(len))
					{
						fmt::throw_exception("End of stream (PRIVATE_STREAM_2, len=%d)", len);
					}
					stream.skip(len);
					break;
				}

				case PRIVATE_STREAM_1:
				{
					// audio and user data stream
					DemuxerStream backup = stream;

					if (!stream.check(6))
					{
						fmt::throw_exception("End of stream (PRIVATE_STREAM_1)");
					}
					stream.skip(4);
					stream.get(len);

					if (!stream.check(len))
					{
						fmt::throw_exception("End of stream (PRIVATE_STREAM_1, len=%d)", len);
					}

					const PesHeader pes(stream);
					if (!pes.is_ok)
					{
						fmt::throw_exception("PesHeader error (PRIVATE_STREAM_1, len=%d)", len);
					}

					if (len < pes.size + 4)
					{
						fmt::throw_exception("End of block (PRIVATE_STREAM_1, PesHeader + fid_minor, len=%d)", len);
					}
					len -= pes.size + 4;

					u8 fid_minor;
					if (!stream.get(fid_minor))
					{
						fmt::throw_exception("End of stream (PRIVATE_STREAM1, fid_minor)");
					}

					const u32 ch = fid_minor % 16;
					if ((fid_minor & -0x10) == 0 && esATX[ch])
					{
						ElementaryStream& es = *esATX[ch];
						if (es.raw_data.size() > 1024 * 1024)
						{
							stream = backup;
							std::this_thread::sleep_for(1ms); // hack
							continue;
						}

						if (len < 3 || !stream.check(3))
						{
							fmt::throw_exception("End of block (ATX, unknown header, len=%d)", len);
						}
						len -= 3;
						stream.skip(3);

						if (pes.has_ts)
						{
							es.last_dts = pes.dts;
							es.last_pts = pes.pts;
						}

						es.push(stream, len);

						while (true)
						{
							auto const size = es.raw_data.size() - es.raw_pos; // size of available new data
							auto const data = es.raw_data.data() + es.raw_pos; // pointer to available data

							if (size < 8) break; // skip if cannot read ATS header

							if (data[0] != 0x0f || data[1] != 0xd0)
							{
								fmt::throw_exception("ATX: 0x0fd0 header not found (ats=0x%llx)", *reinterpret_cast<be_t<u64>*>(data));
							}

							u32 frame_size = (((u32{data[2]} & 0x3) << 8) | u32{data[3]}) * 8 + 8;

							if (size < frame_size + 8) break; // skip non-complete AU

							if (es.isfull(frame_size + 8)) break; // skip if cannot push AU

							es.push_au(frame_size + 8, es.last_dts, es.last_pts, stream.userdata, false /* TODO: set correct value */, 0);

							//cellDmux.notice("ATX AU pushed (ats=0x%llx, frame_size=%d)", *(be_t<u64>*)data, frame_size);

							auto esMsg = vm::ptr<CellDmuxEsMsg>::make(memAddr + (cb_add ^= 16));
							esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
							esMsg->supplementalInfo = stream.userdata;
							es.cbFunc(*this, id, es.id, esMsg, es.cbArg);
							lv2_obj::sleep(*this);
						}
					}
					else
					{
						cellDmux.notice("PRIVATE_STREAM_1 (len=%d, fid_minor=0x%x)", len, fid_minor);
						stream.skip(len);
					}
					break;
				}

				case 0x1e0: case 0x1e1: case 0x1e2: case 0x1e3:
				case 0x1e4: case 0x1e5: case 0x1e6: case 0x1e7:
				case 0x1e8: case 0x1e9: case 0x1ea: case 0x1eb:
				case 0x1ec: case 0x1ed: case 0x1ee: case 0x1ef:
				{
					// video stream (AVC or M2V)
					DemuxerStream backup = stream;

					if (!stream.check(6))
					{
						fmt::throw_exception("End of stream (video, code=0x%x)", code);
					}
					stream.skip(4);
					stream.get(len);

					if (!stream.check(len))
					{
						fmt::throw_exception("End of stream (video, code=0x%x, len=%d)", code, len);
					}

					const PesHeader pes(stream);
					if (!pes.is_ok)
					{
						fmt::throw_exception("PesHeader error (video, code=0x%x, len=%d)", code, len);
					}

					if (len < pes.size + 3)
					{
						fmt::throw_exception("End of block (video, code=0x%x, PesHeader)", code);
					}
					len -= pes.size + 3;

					const u32 ch = code % 16;
					if (esAVC[ch])
					{
						ElementaryStream& es = *esAVC[ch];

						const u32 old_size = ::size32(es.raw_data);
						if (es.isfull(old_size))
						{
							stream = backup;
							std::this_thread::sleep_for(1ms); // hack
							continue;
						}

						if ((pes.has_ts && old_size) || old_size >= 0x69800)
						{
							// push AU if it becomes too big or the next packet contains PTS/DTS
							es.push_au(old_size, es.last_dts, es.last_pts, stream.userdata, false /* TODO: set correct value */, 0);

							// callback
							auto esMsg = vm::ptr<CellDmuxEsMsg>::make(memAddr + (cb_add ^= 16));
							esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
							esMsg->supplementalInfo = stream.userdata;
							es.cbFunc(*this, id, es.id, esMsg, es.cbArg);
							lv2_obj::sleep(*this);
						}

						if (pes.has_ts)
						{
							// preserve dts/pts for next AU
							es.last_dts = pes.dts;
							es.last_pts = pes.pts;
						}

						// reconstruction of MPEG2-PS stream for vdec module
						const u32 size = len + pes.size + 9;
						stream = backup;
						es.push(stream, size);
					}
					else
					{
						cellDmux.notice("Video stream (code=0x%x, len=%d)", code, len);
						stream.skip(len);
					}
					break;
				}

				default:
				{
					if ((code & PACKET_START_CODE_MASK) == PACKET_START_CODE_PREFIX)
					{
						fmt::throw_exception("Unknown code found (0x%x)", code);
					}

					// search
					stream.skip(1);
				}
				}

				continue;
			}

			// wait for task if no work
			if (!job.pop(task, &is_closed))
			{
				break; // Emu is stopped
			}

			switch (task.type)
			{
			case dmuxSetStream:
			{
				if (task.stream.discontinuity)
				{
					cellDmux.warning("dmuxSetStream (beginning)");
					for (u32 i = 0; i < std::size(esALL); i++)
					{
						if (esALL[i])
						{
							esALL[i]->reset();
						}
					}
				}

				stream = task.stream;
				//cellDmux.notice("*** stream updated(addr=0x%x, size=0x%x, discont=%d, userdata=0x%llx)",
					//stream.addr, stream.size, stream.discontinuity, stream.userdata);
				break;
			}

			case dmuxResetStream:
			case dmuxResetStreamAndWaitDone:
			{
				// demuxing stopped
				if (is_running.exchange(false))
				{
					// callback
					auto dmuxMsg = vm::ptr<CellDmuxMsg>::make(memAddr + (cb_add ^= 16));
					dmuxMsg->msgType = CELL_DMUX_MSG_TYPE_DEMUX_DONE;
					dmuxMsg->supplementalInfo = stream.userdata;
					cbFunc(*this, id, dmuxMsg, cbArg);
					lv2_obj::sleep(*this);

					stream = {};

					is_working = false;
				}

				break;
			}

			case dmuxEnableEs:
			{
				ElementaryStream& es = *task.es.es_ptr;

				// TODO: uncomment when ready to use
				//if ((es.fidMajor & -0x10) == 0xe0 && es.fidMinor == 0 && es.sup1 == 1 && !es.sup2)
				//{
				//	esAVC[es.fidMajor % 16] = task.es.es_ptr;
				//}
				//else if ((es.fidMajor & -0x10) == 0xe0 && es.fidMinor == 0 && !es.sup1 && !es.sup2)
				//{
				//	esM2V[es.fidMajor % 16] = task.es.es_ptr;
				//}
				//else if (es.fidMajor == 0xbd && (es.fidMinor & -0x10) == 0 && !es.sup1 && !es.sup2)
				//{
				//	esATX[es.fidMinor % 16] = task.es.es_ptr;
				//}
				//else if (es.fidMajor == 0xbd && (es.fidMinor & -0x10) == 0x20 && !es.sup1 && !es.sup2)
				//{
				//	esDATA[es.fidMinor % 16] = task.es.es_ptr;
				//}
				//else if (es.fidMajor == 0xbd && (es.fidMinor & -0x10) == 0x30 && !es.sup1 && !es.sup2)
				//{
				//	esAC3[es.fidMinor % 16] = task.es.es_ptr;
				//}
				//else if (es.fidMajor == 0xbd && (es.fidMinor & -0x10) == 0x40 && !es.sup1 && !es.sup2)
				//{
				//	esPCM[es.fidMinor % 16] = task.es.es_ptr;
				//}
				//else
				{
					fmt::throw_exception("dmuxEnableEs: unknown filter (0x%x, 0x%x, 0x%x, 0x%x)", es.fidMajor, es.fidMinor, es.sup1, es.sup2);
				}
				es.dmux = this;
				break;
			}

			case dmuxDisableEs:
			{
				ElementaryStream& es = *task.es.es_ptr;
				if (es.dmux != this)
				{
					fmt::throw_exception("dmuxDisableEs: invalid elementary stream");
				}

				for (u32 i = 0; i < std::size(esALL); i++)
				{
					if (esALL[i] == &es)
					{
						esALL[i] = nullptr;
					}
				}
				es.dmux = nullptr;
				idm::remove<ElementaryStream>(task.es.es);
				break;
			}

			case dmuxFlushEs:
			{
				ElementaryStream& es = *task.es.es_ptr;

				const u32 old_size = ::size32(es.raw_data);
				if (old_size && (es.fidMajor & -0x10) == 0xe0)
				{
					// TODO (it's only for AVC, some ATX data may be lost)
					while (es.isfull(old_size))
					{
						if (Emu.IsStopped() || is_closed) break;

						std::this_thread::sleep_for(1ms); // hack
					}

					es.push_au(old_size, es.last_dts, es.last_pts, stream.userdata, false, 0);

					// callback
					auto esMsg = vm::ptr<CellDmuxEsMsg>::make(memAddr + (cb_add ^= 16));
					esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
					esMsg->supplementalInfo = stream.userdata;
					es.cbFunc(*this, id, es.id, esMsg, es.cbArg);
					lv2_obj::sleep(*this);
				}

				if (!es.raw_data.empty())
				{
					cellDmux.error("dmuxFlushEs: 0x%x bytes lost (es_id=%d)", ::size32(es.raw_data), es.id);
				}

				// callback
				auto esMsg = vm::ptr<CellDmuxEsMsg>::make(memAddr + (cb_add ^= 16));
				esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_FLUSH_DONE;
				esMsg->supplementalInfo = stream.userdata;
				es.cbFunc(*this, id, es.id, esMsg, es.cbArg);
				lv2_obj::sleep(*this);
				break;
			}

			case dmuxResetEs:
			{
				task.es.es_ptr->reset();
				break;
			}

			case dmuxClose:
			{
				break;
			}

			default:
			{
				fmt::throw_exception("Demuxer thread error: unknown task (0x%x)", +task.type);
			}
			}
		}

		is_finished = true;
	}
};


PesHeader::PesHeader(DemuxerStream& stream)
	: pts(CODEC_TS_INVALID)
	, dts(CODEC_TS_INVALID)
	, size(0)
	, has_ts(false)
	, is_ok(false)
{
	u16 header;
	if (!stream.get(header))
	{
		fmt::throw_exception("End of stream (header)");
	}
	if (!stream.get(size))
	{
		fmt::throw_exception("End of stream (size)");
	}
	if (!stream.check(size))
	{
		fmt::throw_exception("End of stream (size=%d)", size);
	}

	u8 pos = 0;
	while (pos++ < size)
	{
		u8 v;
		if (!stream.get(v))
		{
			return; // should never occur
		}

		if (v == 0xff) // skip padding bytes
		{
			continue;
		}

		if ((v & 0xf0) == 0x20 && (size - pos) >= 4) // pts only
		{
			pos += 4;
			pts = stream.get_ts(v);
			has_ts = true;
		}
		else if ((v & 0xf0) == 0x30 && (size - pos) >= 9) // pts and dts
		{
			pos += 5;
			pts = stream.get_ts(v);
			stream.get(v);
			has_ts = true;

			if ((v & 0xf0) != 0x10)
			{
				cellDmux.error("PesHeader(): dts not found (v=0x%x, size=%d, pos=%d)", v, size, pos - 1);
				stream.skip(size - pos);
				return;
			}
			pos += 4;
			dts = stream.get_ts(v);
		}
		else
		{
			cellDmux.warning("PesHeader(): unknown code (v=0x%x, size=%d, pos=%d)", v, size, pos - 1);
			stream.skip(size - pos);
			pos = size;
			break;
		}
	}

	is_ok = true;
}

ElementaryStream::ElementaryStream(Demuxer* dmux, u32 addr, u32 size, u32 fidMajor, u32 fidMinor, u32 sup1, u32 sup2, vm::ptr<CellDmuxCbEsMsg> cbFunc, u32 cbArg, u32 spec)
	: put(utils::align(addr, 128))
	, dmux(dmux)
	, memAddr(utils::align(addr, 128))
	, memSize(size - (addr - memAddr))
	, fidMajor(fidMajor)
	, fidMinor(fidMinor)
	, sup1(sup1)
	, sup2(sup2)
	, cbFunc(cbFunc)
	, cbArg(cbArg)
	, spec(spec)
{
}

bool ElementaryStream::is_full(u32 space)
{
	if (released < put_count)
	{
		if (entries.is_full())
		{
			return true;
		}

		u32 first = 0;
		if (!entries.peek(first, 0, &dmux->is_closed) || !first)
		{
			fmt::throw_exception("entries.peek() failed");
		}
		else if (first >= put)
		{
			return first - put < space + 128;
		}
		else if (put + space + 128 > memAddr + memSize)
		{
			return first - memAddr < space + 128;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool ElementaryStream::isfull(u32 space)
{
	std::lock_guard lock(m_mutex);
	return is_full(space);
}

void ElementaryStream::push_au(u32 size, u64 dts, u64 pts, u64 userdata, bool rap, u32 specific)
{
	u32 addr;
	{
		std::lock_guard lock(m_mutex);
		ensure(!is_full(size));

		if (put + size + 128 > memAddr + memSize)
		{
			put = memAddr;
		}

		std::memcpy(vm::base(put + 128), raw_data.data(), size);
		raw_data.erase(raw_data.begin(), raw_data.begin() + size);

		auto info = vm::ptr<CellDmuxAuInfoEx>::make(put);
		info->auAddr = put + 128;
		info->auSize = size;
		info->dts.lower = static_cast<u32>(dts);
		info->dts.upper = static_cast<u32>(dts >> 32);
		info->pts.lower = static_cast<u32>(pts);
		info->pts.upper = static_cast<u32>(pts >> 32);
		info->isRap = rap;
		info->reserved = 0;
		info->userData = userdata;

		auto spec = vm::ptr<u32>::make(put + u32{sizeof(CellDmuxAuInfoEx)});
		*spec = specific;

		auto inf = vm::ptr<CellDmuxAuInfo>::make(put + 64);
		inf->auAddr = put + 128;
		inf->auSize = size;
		inf->dtsLower = static_cast<u32>(dts);
		inf->dtsUpper = static_cast<u32>(dts >> 32);
		inf->ptsLower = static_cast<u32>(pts);
		inf->ptsUpper = static_cast<u32>(pts >> 32);
		inf->auMaxSize = 0; // ?????
		inf->userData = userdata;

		addr = put;

		put = utils::align(put + 128 + size, 128);

		put_count++;
	}

	ensure(entries.push(addr, &dmux->is_closed));
}

void ElementaryStream::push(DemuxerStream& stream, u32 size)
{
	auto const old_size = raw_data.size();

	raw_data.resize(old_size + size);

	std::memcpy(raw_data.data() + old_size, vm::base(stream.addr), size); // append bytes

	stream.skip(size);
}

bool ElementaryStream::release()
{
	std::lock_guard lock(m_mutex);
	if (released >= put_count)
	{
		cellDmux.fatal("es::release() error: buffer is empty");
		return false;
	}
	if (released >= got_count)
	{
		cellDmux.fatal("es::release() error: buffer has not been seen yet");
		return false;
	}

	u32 addr = 0;
	if (!entries.pop(addr, &dmux->is_closed) || !addr)
	{
		cellDmux.fatal("es::release() error: entries.Pop() failed");
		return false;
	}

	released++;
	return true;
}

bool ElementaryStream::peek(u32& out_data, bool no_ex, u32& out_spec, bool update_index)
{
	std::lock_guard lock(m_mutex);
	if (got_count < released)
	{
		cellDmux.fatal("es::peek() error: got_count(%d) < released(%d) (put_count=%d)", got_count, released, put_count);
		return false;
	}
	if (got_count >= put_count)
	{
		return false;
	}

	u32 addr = 0;
	if (!entries.peek(addr, got_count - released, &dmux->is_closed) || !addr)
	{
		cellDmux.fatal("es::peek() error: entries.Peek() failed");
		return false;
	}

	out_data = no_ex ? addr + 64 : addr;
	out_spec = addr + sizeof(CellDmuxAuInfoEx);

	if (update_index)
	{
		got_count++;
	}
	return true;
}

void ElementaryStream::reset()
{
	std::lock_guard lock(m_mutex);
	put = memAddr;
	entries.clear();
	put_count = 0;
	got_count = 0;
	released = 0;
	raw_data.clear();
	raw_pos = 0;
}

void dmuxQueryAttr(u32 /* info_addr, may be 0 */, vm::ptr<CellDmuxAttr> attr)
{
	attr->demuxerVerLower = 0x280000; // TODO: check values
	attr->demuxerVerUpper = 0x260000;
	attr->memSize = 0x10000; // 0x3e8e6 from ps3
}

void dmuxQueryEsAttr(u32 /* info, may be 0 */, vm::cptr<CellCodecEsFilterId> esFilterId, u32 /*esSpecificInfo*/, vm::ptr<CellDmuxEsAttr> attr)
{
	if (esFilterId->filterIdMajor >= 0xe0)
	{
		attr->memSize = 0x500000; // 0x45fa49 from ps3
	}
	else
	{
		attr->memSize = 0x7000; // 0x73d9 from ps3
	}

	cellDmux.warning("*** filter(0x%x, 0x%x, 0x%x, 0x%x)", esFilterId->filterIdMajor, esFilterId->filterIdMinor, esFilterId->supplementalInfo1, esFilterId->supplementalInfo2);
}

error_code cellDmuxQueryAttr(vm::cptr<CellDmuxType> type, vm::ptr<CellDmuxAttr> attr)
{
	cellDmux.warning("cellDmuxQueryAttr(type=*0x%x, attr=*0x%x)", type, attr);

	if (type->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmuxQueryAttr(0, attr);
	return CELL_OK;
}

error_code cellDmuxQueryAttr2(vm::cptr<CellDmuxType2> type2, vm::ptr<CellDmuxAttr> attr)
{
	cellDmux.warning("cellDmuxQueryAttr2(demuxerType2=*0x%x, demuxerAttr=*0x%x)", type2, attr);

	if (type2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmuxQueryAttr(type2->streamSpecificInfo, attr);
	return CELL_OK;
}

error_code cellDmuxOpen(vm::cptr<CellDmuxType> type, vm::cptr<CellDmuxResource> res, vm::cptr<CellDmuxCb> cb, vm::ptr<u32> handle)
{
	cellDmux.warning("cellDmuxOpen(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	if (type->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerResource and demuxerCb arguments
	fmt::throw_exception("cellDmux disabled, use LLE.");
}

error_code cellDmuxOpenEx(vm::cptr<CellDmuxType> type, vm::cptr<CellDmuxResourceEx> resEx, vm::cptr<CellDmuxCb> cb, vm::ptr<u32> handle)
{
	cellDmux.warning("cellDmuxOpenEx(type=*0x%x, resEx=*0x%x, cb=*0x%x, handle=*0x%x)", type, resEx, cb, handle);

	if (type->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerResourceEx and demuxerCb arguments
	fmt::throw_exception("cellDmux disabled, use LLE.");
}

error_code cellDmuxOpenExt(vm::cptr<CellDmuxType> type, vm::cptr<CellDmuxResourceEx> resEx, vm::cptr<CellDmuxCb> cb, vm::ptr<u32> handle)
{
	cellDmux.warning("cellDmuxOpenExt(type=*0x%x, resEx=*0x%x, cb=*0x%x, handle=*0x%x)", type, resEx, cb, handle);

	return cellDmuxOpenEx(type, resEx, cb, handle);
}

error_code cellDmuxOpen2(vm::cptr<CellDmuxType2> type2, vm::cptr<CellDmuxResource2> res2, vm::cptr<CellDmuxCb> cb, vm::ptr<u32> handle)
{
	cellDmux.warning("cellDmuxOpen2(type2=*0x%x, res2=*0x%x, cb=*0x%x, handle=*0x%x)", type2, res2, cb, handle);

	if (type2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerType2, demuxerResource2 and demuxerCb arguments
	fmt::throw_exception("cellDmux disabled, use LLE.");
}

error_code cellDmuxClose(u32 handle)
{
	cellDmux.warning("cellDmuxClose(handle=0x%x)", handle);

	const auto dmux = idm::get_unlocked<Demuxer>(handle);

	if (!dmux)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->is_closed = true;
	dmux->job.try_push(DemuxerTask(dmuxClose));

	while (!dmux->is_finished)
	{
		if (Emu.IsStopped())
		{
			cellDmux.warning("cellDmuxClose(%d) aborted", handle);
			return CELL_OK;
		}

		std::this_thread::sleep_for(1ms); // hack
	}

	idm::remove<ppu_thread>(handle);
	return CELL_OK;
}

error_code cellDmuxSetStream(u32 handle, u32 streamAddress, u32 streamSize, b8 discontinuity, u64 userData)
{
	cellDmux.trace("cellDmuxSetStream(handle=0x%x, streamAddress=0x%x, streamSize=%d, discontinuity=%d, userData=0x%llx)", handle, streamAddress, streamSize, discontinuity, userData);

	const auto dmux = idm::get_unlocked<Demuxer>(handle);

	if (!dmux)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (dmux->is_running.exchange(true))
	{
		//std::this_thread::sleep_for(1ms); // hack
		return CELL_DMUX_ERROR_BUSY;
	}

	DemuxerTask task(dmuxSetStream);
	auto& info = task.stream;
	info.addr = streamAddress;
	info.size = streamSize;
	info.discontinuity = discontinuity;
	info.userdata = userData;

	dmux->job.push(task, &dmux->is_closed);
	return CELL_OK;
}

error_code cellDmuxResetStream(u32 handle)
{
	cellDmux.warning("cellDmuxResetStream(handle=0x%x)", handle);

	const auto dmux = idm::get_unlocked<Demuxer>(handle);

	if (!dmux)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.push(DemuxerTask(dmuxResetStream), &dmux->is_closed);
	return CELL_OK;
}

error_code cellDmuxResetStreamAndWaitDone(u32 handle)
{
	cellDmux.warning("cellDmuxResetStreamAndWaitDone(handle=0x%x)", handle);

	const auto dmux = idm::get_unlocked<Demuxer>(handle);

	if (!dmux)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!dmux->is_running)
	{
		return CELL_OK;
	}

	dmux->is_working = true;

	dmux->job.push(DemuxerTask(dmuxResetStreamAndWaitDone), &dmux->is_closed);

	while (dmux->is_running && dmux->is_working && !dmux->is_closed) // TODO: ensure that it is safe
	{
		if (Emu.IsStopped())
		{
			cellDmux.warning("cellDmuxResetStreamAndWaitDone(%d) aborted", handle);
			return CELL_OK;
		}
		std::this_thread::sleep_for(1ms); // hack
	}

	return CELL_OK;
}

error_code cellDmuxQueryEsAttr(vm::cptr<CellDmuxType> type, vm::cptr<CellCodecEsFilterId> esFilterId, u32 esSpecificInfo, vm::ptr<CellDmuxEsAttr> esAttr)
{
	cellDmux.warning("cellDmuxQueryEsAttr(demuxerType=*0x%x, esFilterId=*0x%x, esSpecificInfo=*0x%x, esAttr=*0x%x)", type, esFilterId, esSpecificInfo, esAttr);

	if (type->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check esFilterId and esSpecificInfo correctly
	dmuxQueryEsAttr(0, esFilterId, esSpecificInfo, esAttr);
	return CELL_OK;
}

error_code cellDmuxQueryEsAttr2(vm::cptr<CellDmuxType2> type2, vm::cptr<CellCodecEsFilterId> esFilterId, u32 esSpecificInfo, vm::ptr<CellDmuxEsAttr> esAttr)
{
	cellDmux.warning("cellDmuxQueryEsAttr2(type2=*0x%x, esFilterId=*0x%x, esSpecificInfo=*0x%x, esAttr=*0x%x)", type2, esFilterId, esSpecificInfo, esAttr);

	if (type2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerType2, esFilterId and esSpecificInfo correctly
	dmuxQueryEsAttr(type2->streamSpecificInfo, esFilterId, esSpecificInfo, esAttr);
	return CELL_OK;
}

error_code cellDmuxEnableEs(u32 handle, vm::cptr<CellCodecEsFilterId> esFilterId, vm::cptr<CellDmuxEsResource> esResourceInfo, vm::cptr<CellDmuxEsCb> esCb, u32 esSpecificInfo, vm::ptr<u32> esHandle)
{
	cellDmux.warning("cellDmuxEnableEs(handle=0x%x, esFilterId=*0x%x, esResourceInfo=*0x%x, esCb=*0x%x, esSpecificInfo=*0x%x, esHandle=*0x%x)", handle, esFilterId, esResourceInfo, esCb, esSpecificInfo, esHandle);

	const auto dmux = idm::get_unlocked<Demuxer>(handle);

	if (!dmux)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check esFilterId, esResourceInfo, esCb and esSpecificInfo correctly

	const auto es = idm::make_ptr<ElementaryStream>(dmux.get(), esResourceInfo->memAddr, esResourceInfo->memSize,
		esFilterId->filterIdMajor, esFilterId->filterIdMinor, esFilterId->supplementalInfo1, esFilterId->supplementalInfo2,
		esCb->cbFunc, esCb->cbArg, esSpecificInfo);

	*esHandle = es->id;

	cellDmux.warning("*** New ES(dmux=0x%x, addr=0x%x, size=0x%x, filter={0x%x, 0x%x, 0x%x, 0x%x}, cb=0x%x, arg=0x%x, spec=0x%x): id = 0x%x",
		handle, es->memAddr, es->memSize, es->fidMajor, es->fidMinor, es->sup1, es->sup2, es->cbFunc, es->cbArg, es->spec, es->id);

	DemuxerTask task(dmuxEnableEs);
	task.es.es = es->id;
	task.es.es_ptr = es.get();

	dmux->job.push(task, &dmux->is_closed);
	return CELL_OK;
}

error_code cellDmuxDisableEs(u32 esHandle)
{
	cellDmux.warning("cellDmuxDisableEs(esHandle=0x%x)", esHandle);

	const auto es = idm::get_unlocked<ElementaryStream>(esHandle);

	if (!es)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxDisableEs);
	task.es.es = esHandle;
	task.es.es_ptr = es.get();

	es->dmux->job.push(task, &es->dmux->is_closed);
	return CELL_OK;
}

error_code cellDmuxResetEs(u32 esHandle)
{
	cellDmux.trace("cellDmuxResetEs(esHandle=0x%x)", esHandle);

	const auto es = idm::get_unlocked<ElementaryStream>(esHandle);

	if (!es)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxResetEs);
	task.es.es = esHandle;
	task.es.es_ptr = es.get();

	es->dmux->job.push(task, &es->dmux->is_closed);
	return CELL_OK;
}

error_code cellDmuxGetAu(u32 esHandle, vm::ptr<u32> auInfo, vm::ptr<u32> auSpecificInfo)
{
	cellDmux.trace("cellDmuxGetAu(esHandle=0x%x, auInfo=**0x%x, auSpecificInfo=**0x%x)", esHandle, auInfo, auSpecificInfo);

	const auto es = idm::get_unlocked<ElementaryStream>(esHandle);

	if (!es)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, true, spec, true))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	*auInfo = info;
	*auSpecificInfo = spec;
	return CELL_OK;
}

error_code cellDmuxPeekAu(u32 esHandle, vm::ptr<u32> auInfo, vm::ptr<u32> auSpecificInfo)
{
	cellDmux.trace("cellDmuxPeekAu(esHandle=0x%x, auInfo=**0x%x, auSpecificInfo=**0x%x)", esHandle, auInfo, auSpecificInfo);

	const auto es = idm::get_unlocked<ElementaryStream>(esHandle);

	if (!es)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, true, spec, false))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	*auInfo = info;
	*auSpecificInfo = spec;
	return CELL_OK;
}

error_code cellDmuxGetAuEx(u32 esHandle, vm::ptr<u32> auInfoEx, vm::ptr<u32> auSpecificInfo)
{
	cellDmux.trace("cellDmuxGetAuEx(esHandle=0x%x, auInfoEx=**0x%x, auSpecificInfo=**0x%x)", esHandle, auInfoEx, auSpecificInfo);

	const auto es = idm::get_unlocked<ElementaryStream>(esHandle);

	if (!es)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, false, spec, true))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	*auInfoEx = info;
	*auSpecificInfo = spec;
	return CELL_OK;
}

error_code cellDmuxPeekAuEx(u32 esHandle, vm::ptr<u32> auInfoEx, vm::ptr<u32> auSpecificInfo)
{
	cellDmux.trace("cellDmuxPeekAuEx(esHandle=0x%x, auInfoEx=**0x%x, auSpecificInfo=**0x%x)", esHandle, auInfoEx, auSpecificInfo);

	const auto es = idm::get_unlocked<ElementaryStream>(esHandle);

	if (!es)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, false, spec, false))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	*auInfoEx = info;
	*auSpecificInfo = spec;
	return CELL_OK;
}

error_code cellDmuxReleaseAu(u32 esHandle)
{
	cellDmux.trace("cellDmuxReleaseAu(esHandle=0x%x)", esHandle);

	const auto es = idm::get_unlocked<ElementaryStream>(esHandle);

	if (!es)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!es->release())
	{
		return CELL_DMUX_ERROR_SEQ;
	}
	return CELL_OK;
}

error_code cellDmuxFlushEs(u32 esHandle)
{
	cellDmux.warning("cellDmuxFlushEs(esHandle=0x%x)", esHandle);

	const auto es = idm::get_unlocked<ElementaryStream>(esHandle);

	if (!es)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxFlushEs);
	task.es.es = esHandle;
	task.es.es_ptr = es.get();

	es->dmux->job.push(task, &es->dmux->is_closed);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellDmux)("cellDmux", []()
{
	REG_FUNC(cellDmux, cellDmuxQueryAttr);
	REG_FUNC(cellDmux, cellDmuxQueryAttr2);
	REG_FUNC(cellDmux, cellDmuxOpen);
	REG_FUNC(cellDmux, cellDmuxOpenEx);
	REG_FUNC(cellDmux, cellDmuxOpenExt); // 0xe075fabc
	REG_FUNC(cellDmux, cellDmuxOpen2);
	REG_FUNC(cellDmux, cellDmuxClose);
	REG_FUNC(cellDmux, cellDmuxSetStream);
	REG_FUNC(cellDmux, cellDmuxResetStream);
	REG_FUNC(cellDmux, cellDmuxResetStreamAndWaitDone);
	REG_FUNC(cellDmux, cellDmuxQueryEsAttr);
	REG_FUNC(cellDmux, cellDmuxQueryEsAttr2);
	REG_FUNC(cellDmux, cellDmuxEnableEs);
	REG_FUNC(cellDmux, cellDmuxDisableEs);
	REG_FUNC(cellDmux, cellDmuxResetEs);
	REG_FUNC(cellDmux, cellDmuxGetAu);
	REG_FUNC(cellDmux, cellDmuxPeekAu);
	REG_FUNC(cellDmux, cellDmuxGetAuEx);
	REG_FUNC(cellDmux, cellDmuxPeekAuEx);
	REG_FUNC(cellDmux, cellDmuxReleaseAu);
	REG_FUNC(cellDmux, cellDmuxFlushEs);
});
