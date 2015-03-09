#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "cellPamf.h"
#include "cellDmux.h"

extern Module cellDmux;

#define DMUX_ERROR(...) { cellDmux.Error(__VA_ARGS__); Emu.Pause(); return; } // only for demuxer thread

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
		DMUX_ERROR("PesHeader(): end of stream (header)");
	}
	if (!stream.get(size))
	{
		DMUX_ERROR("PesHeader(): end of stream (size)");
	}
	if (!stream.check(size))
	{
		DMUX_ERROR("PesHeader(): end of stream (size=%d)", size);
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
				cellDmux.Error("PesHeader(): dts not found (v=0x%x, size=%d, pos=%d)", v, size, pos - 1);
				stream.skip(size - pos);
				return;
			}
			pos += 4;
			dts = stream.get_ts(v);
		}
		else
		{
			cellDmux.Warning("PesHeader(): unknown code (v=0x%x, size=%d, pos=%d)", v, size, pos - 1);
			stream.skip(size - pos);
			pos = size;
			break;
		}
	}

	is_ok = true;
}

ElementaryStream::ElementaryStream(Demuxer* dmux, u32 addr, u32 size, u32 fidMajor, u32 fidMinor, u32 sup1, u32 sup2, vm::ptr<CellDmuxCbEsMsg> cbFunc, u32 cbArg, u32 spec)
	: dmux(dmux)
	, memAddr(align(addr, 128))
	, memSize(size - (addr - memAddr))
	, fidMajor(fidMajor)
	, fidMinor(fidMinor)
	, sup1(sup1)
	, sup2(sup2)
	, cbFunc(cbFunc)
	, cbArg(cbArg)
	, spec(spec)
	, put(align(addr, 128))
	, put_count(0)
	, got_count(0)
	, released(0)
	, raw_pos(0)
	, last_dts(CODEC_TS_INVALID)
	, last_pts(CODEC_TS_INVALID)
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
			assert(!"es::is_full() error: entries.Peek() failed");
			return false;
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
	std::lock_guard<std::mutex> lock(m_mutex);
	return is_full(space);
}

void ElementaryStream::push_au(u32 size, u64 dts, u64 pts, u64 userdata, bool rap, u32 specific)
{
	u32 addr;
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		assert(!is_full(size));

		if (put + size + 128 > memAddr + memSize)
		{
			put = memAddr;
		}

		memcpy(vm::get_ptr<void>(put + 128), raw_data.data(), size);
		raw_data.erase(raw_data.begin(), raw_data.begin() + size);

		auto info = vm::ptr<CellDmuxAuInfoEx>::make(put);
		info->auAddr = put + 128;
		info->auSize = size;
		info->dts.lower = (u32)(dts);
		info->dts.upper = (u32)(dts >> 32);
		info->pts.lower = (u32)(pts);
		info->pts.upper = (u32)(pts >> 32);
		info->isRap = rap;
		info->reserved = 0;
		info->userData = userdata;

		auto spec = vm::ptr<u32>::make(put + sizeof(CellDmuxAuInfoEx));
		*spec = specific;

		auto inf = vm::ptr<CellDmuxAuInfo>::make(put + 64);
		inf->auAddr = put + 128;
		inf->auSize = size;
		inf->dtsLower = (u32)(dts);
		inf->dtsUpper = (u32)(dts >> 32);
		inf->ptsLower = (u32)(pts);
		inf->ptsUpper = (u32)(pts >> 32);
		inf->auMaxSize = 0; // ?????
		inf->userData = userdata;

		addr = put;

		put = align(put + 128 + size, 128);

		put_count++;
	}
	if (!entries.push(addr, &dmux->is_closed))
	{
		assert(!"es::push_au() error: entries.Push() failed");
	}
}

void ElementaryStream::push(DemuxerStream& stream, u32 size)
{
	auto const old_size = raw_data.size();

	raw_data.resize(old_size + size);

	memcpy(raw_data.data() + old_size, vm::get_ptr<void>(stream.addr), size); // append bytes

	stream.skip(size);
}

bool ElementaryStream::release()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (released >= put_count)
	{
		cellDmux.Error("es::release() error: buffer is empty");
		Emu.Pause();
		return false;
	}
	if (released >= got_count)
	{
		cellDmux.Error("es::release() error: buffer has not been seen yet");
		Emu.Pause();
		return false;
	}

	u32 addr = 0;
	if (!entries.pop(addr, &dmux->is_closed) || !addr)
	{
		cellDmux.Error("es::release() error: entries.Pop() failed");
		Emu.Pause();
		return false;
	}

	released++;
	return true;
}

bool ElementaryStream::peek(u32& out_data, bool no_ex, u32& out_spec, bool update_index)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (got_count < released)
	{
		cellDmux.Error("es::peek() error: got_count(%d) < released(%d) (put_count=%d)", got_count, released, put_count);
		Emu.Pause();
		return false;
	}
	if (got_count >= put_count)
	{
		return false;
	}

	u32 addr = 0;
	if (!entries.peek(addr, got_count - released, &dmux->is_closed) || !addr)
	{
		cellDmux.Error("es::peek() error: entries.Peek() failed");
		Emu.Pause();
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
	std::lock_guard<std::mutex> lock(m_mutex);
	put = memAddr;
	entries.clear();
	put_count = 0;
	got_count = 0;
	released = 0;
	raw_data.clear();
	raw_pos = 0;
}

void dmuxQueryAttr(u32 info_addr /* may be 0 */, vm::ptr<CellDmuxAttr> attr)
{
	attr->demuxerVerLower = 0x280000; // TODO: check values
	attr->demuxerVerUpper = 0x260000;
	attr->memSize = 0x10000; // 0x3e8e6 from ps3
}

void dmuxQueryEsAttr(u32 info_addr /* may be 0 */, vm::ptr<const CellCodecEsFilterId> esFilterId,
					 const u32 esSpecificInfo_addr, vm::ptr<CellDmuxEsAttr> attr)
{
	if (esFilterId->filterIdMajor >= 0xe0)
		attr->memSize = 0x500000; // 0x45fa49 from ps3
	else
		attr->memSize = 0x7000; // 0x73d9 from ps3

	cellDmux.Warning("*** filter(0x%x, 0x%x, 0x%x, 0x%x)", (u32)esFilterId->filterIdMajor, (u32)esFilterId->filterIdMinor,
		(u32)esFilterId->supplementalInfo1, (u32)esFilterId->supplementalInfo2);
}

u32 dmuxOpen(Demuxer* dmux_ptr)
{
	std::shared_ptr<Demuxer> sptr(dmux_ptr);
	Demuxer& dmux = *dmux_ptr;

	u32 dmux_id = Emu.GetIdManager().GetNewID(sptr);

	dmux.id = dmux_id;

	dmux.dmuxCb = static_cast<PPUThread*>(Emu.GetCPU().AddThread(CPU_THREAD_PPU).get());
	dmux.dmuxCb->SetName(fmt::format("Demuxer[%d] Callback", dmux_id));
	dmux.dmuxCb->SetEntry(0);
	dmux.dmuxCb->SetPrio(1001);
	dmux.dmuxCb->SetStackSize(0x10000);
	dmux.dmuxCb->InitStack();
	dmux.dmuxCb->InitRegs();
	dmux.dmuxCb->DoRun();

	thread_t t(fmt::format("Demuxer[%d] Thread", dmux_id), [dmux_ptr, sptr]()
	{
		Demuxer& dmux = *dmux_ptr;

		DemuxerTask task;
		DemuxerStream stream = {};
		ElementaryStream* esALL[96]; memset(esALL, 0, sizeof(esALL));
		ElementaryStream** esAVC = &esALL[0]; // AVC (max 16 minus M2V count)
		ElementaryStream** esM2V = &esALL[16]; // M2V (max 16 minus AVC count)
		ElementaryStream** esDATA = &esALL[32]; // user data (max 16)
		ElementaryStream** esATX = &esALL[48]; // ATRAC3+ (max 16)
		ElementaryStream** esAC3 = &esALL[64]; // AC3 (max 16)
		ElementaryStream** esPCM = &esALL[80]; // LPCM (max 16)

		u32 cb_add = 0;

		while (true)
		{
			if (Emu.IsStopped() || dmux.is_closed)
			{
				break;
			}
			
			if (!dmux.job.try_peek(task) && dmux.is_running && stream.addr)
			{
				// default task (demuxing) (if there is no other work)
				be_t<u32> code;
				be_t<u16> len;

				if (!stream.peek(code)) 
				{
					// demuxing finished
					dmux.is_running = false;

					// callback
					auto dmuxMsg = vm::ptr<CellDmuxMsg>::make(dmux.memAddr + (cb_add ^= 16));
					dmuxMsg->msgType = CELL_DMUX_MSG_TYPE_DEMUX_DONE;
					dmuxMsg->supplementalInfo = stream.userdata;
					dmux.cbFunc(*dmux.dmuxCb, dmux.id, dmuxMsg, dmux.cbArg);

					dmux.is_working = false;

					stream = {};
					
					continue;
				}
				
				switch (code.value())
				{
				case PACK_START_CODE:
				{
					if (!stream.check(14))
					{
						DMUX_ERROR("End of stream (PACK_START_CODE)");
					}
					stream.skip(14);
					break;
				}

				case SYSTEM_HEADER_START_CODE:
				{
					if (!stream.check(18))
					{
						DMUX_ERROR("End of stream (SYSTEM_HEADER_START_CODE)");
					}
					stream.skip(18);
					break;
				}

				case PADDING_STREAM:
				{
					if (!stream.check(6))
					{
						DMUX_ERROR("End of stream (PADDING_STREAM)");
					}
					stream.skip(4);
					stream.get(len);

					if (!stream.check(len))
					{
						DMUX_ERROR("End of stream (PADDING_STREAM, len=%d)", len);
					}
					stream.skip(len);
					break;
				}

				case PRIVATE_STREAM_2:
				{
					if (!stream.check(6))
					{
						DMUX_ERROR("End of stream (PRIVATE_STREAM_2)");
					}
					stream.skip(4);
					stream.get(len);

					cellDmux.Notice("PRIVATE_STREAM_2 (%d)", len);

					if (!stream.check(len))
					{
						DMUX_ERROR("End of stream (PRIVATE_STREAM_2, len=%d)", len);
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
						DMUX_ERROR("End of stream (PRIVATE_STREAM_1)");
					}
					stream.skip(4);
					stream.get(len);

					if (!stream.check(len))
					{
						DMUX_ERROR("End of stream (PRIVATE_STREAM_1, len=%d)", len);
					}

					const PesHeader pes(stream);
					if (!pes.is_ok)
					{
						DMUX_ERROR("PesHeader error (PRIVATE_STREAM_1, len=%d)", len);
					}

					if (len < pes.size + 4)
					{
						DMUX_ERROR("End of block (PRIVATE_STREAM_1, PesHeader + fid_minor, len=%d)", len);
					}
					len -= pes.size + 4;
					
					u8 fid_minor;
					if (!stream.get(fid_minor))
					{
						DMUX_ERROR("End of stream (PRIVATE_STREAM1, fid_minor)");
					}

					const u32 ch = fid_minor % 16;
					if ((fid_minor & -0x10) == 0 && esATX[ch])
					{
						ElementaryStream& es = *esATX[ch];
						if (es.raw_data.size() > 1024 * 1024)
						{
							stream = backup;
							std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
							continue;
						}

						if (len < 3 || !stream.check(3))
						{
							DMUX_ERROR("End of block (ATX, unknown header, len=%d)", len);
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
								DMUX_ERROR("ATX: 0x0fd0 header not found (ats=0x%llx)", *(be_t<u64>*)data);
							}

							u32 frame_size = ((((u32)data[2] & 0x3) << 8) | (u32)data[3]) * 8 + 8;

							if (size < frame_size + 8) break; // skip non-complete AU

							if (es.isfull(frame_size + 8)) break; // skip if cannot push AU
							
							es.push_au(frame_size + 8, es.last_dts, es.last_pts, stream.userdata, false /* TODO: set correct value */, 0);

							//cellDmux.Notice("ATX AU pushed (ats=0x%llx, frame_size=%d)", *(be_t<u64>*)data, frame_size);

							auto esMsg = vm::ptr<CellDmuxEsMsg>::make(dmux.memAddr + (cb_add ^= 16));
							esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
							esMsg->supplementalInfo = stream.userdata;
							es.cbFunc(*dmux.dmuxCb, dmux.id, es.id, esMsg, es.cbArg);
						}
					}
					else
					{
						cellDmux.Notice("PRIVATE_STREAM_1 (len=%d, fid_minor=0x%x)", len, fid_minor);
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
						DMUX_ERROR("End of stream (video, code=0x%x)", code);
					}
					stream.skip(4);
					stream.get(len);

					if (!stream.check(len))
					{
						DMUX_ERROR("End of stream (video, code=0x%x, len=%d)", code, len);
					}

					const PesHeader pes(stream);
					if (!pes.is_ok)
					{
						DMUX_ERROR("PesHeader error (video, code=0x%x, len=%d)", code, len);
					}

					if (len < pes.size + 3)
					{
						DMUX_ERROR("End of block (video, code=0x%x, PesHeader)", code);
					}
					len -= pes.size + 3;

					const u32 ch = code % 16;
					if (esAVC[ch])
					{
						ElementaryStream& es = *esAVC[ch];

						const u32 old_size = (u32)es.raw_data.size();
						if (es.isfull(old_size))
						{
							stream = backup;
							std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
							continue;
						}

						if ((pes.has_ts && old_size) || old_size >= 0x69800)
						{
							// push AU if it becomes too big or the next packet contains PTS/DTS
							es.push_au(old_size, es.last_dts, es.last_pts, stream.userdata, false /* TODO: set correct value */, 0);

							// callback
							auto esMsg = vm::ptr<CellDmuxEsMsg>::make(dmux.memAddr + (cb_add ^= 16));
							esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
							esMsg->supplementalInfo = stream.userdata;
							es.cbFunc(*dmux.dmuxCb, dmux.id, es.id, esMsg, es.cbArg);
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
						cellDmux.Notice("Video stream (code=0x%x, len=%d)", code, len);
						stream.skip(len);
					}
					break;
				}

				default:
				{
					if ((code & PACKET_START_CODE_MASK) == PACKET_START_CODE_PREFIX)
					{
						DMUX_ERROR("Unknown code found (0x%x)", code);
					}

					// search
					stream.skip(1);
				}
				}

				continue;
			}

			// wait for task if no work
			if (!dmux.job.pop(task, &dmux.is_closed))
			{
				break; // Emu is stopped
			}

			switch (task.type)
			{
			case dmuxSetStream:
			{
				if (task.stream.discontinuity)
				{
					cellDmux.Warning("dmuxSetStream (beginning)");
					for (u32 i = 0; i < sizeof(esALL) / sizeof(esALL[0]); i++)
					{
						if (esALL[i])
						{
							esALL[i]->reset();
						}
					}
				}

				stream = task.stream;
				//LOG_NOTICE(HLE, "*** stream updated(addr=0x%x, size=0x%x, discont=%d, userdata=0x%llx)",
					//stream.addr, stream.size, stream.discontinuity, stream.userdata);
				break;
			}

			case dmuxResetStream:
			case dmuxResetStreamAndWaitDone:
			{
				// demuxing stopped
				if (dmux.is_running.exchange(false))
				{
					// callback
					auto dmuxMsg = vm::ptr<CellDmuxMsg>::make(dmux.memAddr + (cb_add ^= 16));
					dmuxMsg->msgType = CELL_DMUX_MSG_TYPE_DEMUX_DONE;
					dmuxMsg->supplementalInfo = stream.userdata;
					dmux.cbFunc(*dmux.dmuxCb, dmux.id, dmuxMsg, dmux.cbArg);

					stream = {};

					dmux.is_working = false;
				}

				break;
			}

			case dmuxEnableEs:
			{
				ElementaryStream& es = *task.es.es_ptr;

				// TODO: uncomment when ready to use
				if ((es.fidMajor & -0x10) == 0xe0 && es.fidMinor == 0 && es.sup1 == 1 && !es.sup2)
				{
					esAVC[es.fidMajor % 16] = task.es.es_ptr;
				}
				//else if ((es.fidMajor & -0x10) == 0xe0 && es.fidMinor == 0 && !es.sup1 && !es.sup2)
				//{
				//	esM2V[es.fidMajor % 16] = task.es.es_ptr;
				//}
				else if (es.fidMajor == 0xbd && (es.fidMinor & -0x10) == 0 && !es.sup1 && !es.sup2)
				{
					esATX[es.fidMinor % 16] = task.es.es_ptr;
				}
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
				else
				{
					DMUX_ERROR("dmuxEnableEs: unknown filter (0x%x, 0x%x, 0x%x, 0x%x)", es.fidMajor, es.fidMinor, es.sup1, es.sup2);
				}
				es.dmux = &dmux;
				break;
			}

			case dmuxDisableEs:
			{
				ElementaryStream& es = *task.es.es_ptr;
				if (es.dmux != &dmux)
				{
					DMUX_ERROR("dmuxDisableEs: invalid elementary stream");
				}

				for (u32 i = 0; i < sizeof(esALL) / sizeof(esALL[0]); i++)
				{
					if (esALL[i] == &es)
					{
						esALL[i] = nullptr;
					}
				}
				es.dmux = nullptr;
				Emu.GetIdManager().RemoveID(task.es.es);
				break;
			}

			case dmuxFlushEs:
			{
				ElementaryStream& es = *task.es.es_ptr;

				const u32 old_size = (u32)es.raw_data.size();
				if (old_size && (es.fidMajor & -0x10) == 0xe0)
				{
					// TODO (it's only for AVC, some ATX data may be lost)
					while (es.isfull(old_size))
					{
						if (Emu.IsStopped() || dmux.is_closed) break;

						std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
					}

					es.push_au(old_size, es.last_dts, es.last_pts, stream.userdata, false, 0);

					// callback
					auto esMsg = vm::ptr<CellDmuxEsMsg>::make(dmux.memAddr + (cb_add ^= 16));
					esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
					esMsg->supplementalInfo = stream.userdata;
					es.cbFunc(*dmux.dmuxCb, dmux.id, es.id, esMsg, es.cbArg);
				}
				
				if (es.raw_data.size())
				{
					cellDmux.Error("dmuxFlushEs: 0x%x bytes lost (es_id=%d)", (u32)es.raw_data.size(), es.id);
				}

				// callback
				auto esMsg = vm::ptr<CellDmuxEsMsg>::make(dmux.memAddr + (cb_add ^= 16));
				esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_FLUSH_DONE;
				esMsg->supplementalInfo = stream.userdata;
				es.cbFunc(*dmux.dmuxCb, dmux.id, es.id, esMsg, es.cbArg);
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
				DMUX_ERROR("Demuxer thread error: unknown task (0x%x)", task.type);
			}	
			}
		}

		dmux.is_finished = true;
	});

	return dmux_id;
}

int cellDmuxQueryAttr(vm::ptr<const CellDmuxType> demuxerType, vm::ptr<CellDmuxAttr> demuxerAttr)
{
	cellDmux.Warning("cellDmuxQueryAttr(demuxerType_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType.addr(), demuxerAttr.addr());

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmuxQueryAttr(0, demuxerAttr);
	return CELL_OK;
}

int cellDmuxQueryAttr2(vm::ptr<const CellDmuxType2> demuxerType2, vm::ptr<CellDmuxAttr> demuxerAttr)
{
	cellDmux.Warning("cellDmuxQueryAttr2(demuxerType2_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType2.addr(), demuxerAttr.addr());

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmuxQueryAttr(demuxerType2->streamSpecificInfo_addr, demuxerAttr);
	return CELL_OK;
}

int cellDmuxOpen(vm::ptr<const CellDmuxType> demuxerType, vm::ptr<const CellDmuxResource> demuxerResource, 
	vm::ptr<const CellDmuxCb> demuxerCb, vm::ptr<u32> demuxerHandle)
{
	cellDmux.Warning("cellDmuxOpen(demuxerType_addr=0x%x, demuxerResource_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.addr(), demuxerResource.addr(), demuxerCb.addr(), demuxerHandle.addr());

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerResource and demuxerCb arguments

	*demuxerHandle = dmuxOpen(new Demuxer(demuxerResource->memAddr, demuxerResource->memSize, demuxerCb->cbMsgFunc, demuxerCb->cbArg));

	return CELL_OK;
}

int cellDmuxOpenEx(vm::ptr<const CellDmuxType> demuxerType, vm::ptr<const CellDmuxResourceEx> demuxerResourceEx, 
	vm::ptr<const CellDmuxCb> demuxerCb, vm::ptr<u32> demuxerHandle)
{
	cellDmux.Warning("cellDmuxOpenEx(demuxerType_addr=0x%x, demuxerResourceEx_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.addr(), demuxerResourceEx.addr(), demuxerCb.addr(), demuxerHandle.addr());

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerResourceEx and demuxerCb arguments

	*demuxerHandle = dmuxOpen(new Demuxer(demuxerResourceEx->memAddr, demuxerResourceEx->memSize, demuxerCb->cbMsgFunc, demuxerCb->cbArg));

	return CELL_OK;
}

int cellDmuxOpen2(vm::ptr<const CellDmuxType2> demuxerType2, vm::ptr<const CellDmuxResource2> demuxerResource2, 
	vm::ptr<const CellDmuxCb> demuxerCb, vm::ptr<u32> demuxerHandle)
{
	cellDmux.Warning("cellDmuxOpen2(demuxerType2_addr=0x%x, demuxerResource2_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType2.addr(), demuxerResource2.addr(), demuxerCb.addr(), demuxerHandle.addr());

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerType2, demuxerResource2 and demuxerCb arguments

	*demuxerHandle = dmuxOpen(new Demuxer(demuxerResource2->memAddr, demuxerResource2->memSize, demuxerCb->cbMsgFunc, demuxerCb->cbArg));

	return CELL_OK;
}

int cellDmuxClose(u32 demuxerHandle)
{
	cellDmux.Warning("cellDmuxClose(demuxerHandle=%d)", demuxerHandle);

	std::shared_ptr<Demuxer> dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->is_closed = true;
	dmux->job.try_push(DemuxerTask(dmuxClose));

	while (!dmux->is_finished)
	{
		if (Emu.IsStopped())
		{
			cellDmux.Warning("cellDmuxClose(%d) aborted", demuxerHandle);
			return CELL_OK;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	}

	if (dmux->dmuxCb) Emu.GetCPU().RemoveThread(dmux->dmuxCb->GetId());
	Emu.GetIdManager().RemoveID(demuxerHandle);
	return CELL_OK;
}

int cellDmuxSetStream(u32 demuxerHandle, const u32 streamAddress, u32 streamSize, bool discontinuity, u64 userData)
{
	cellDmux.Log("cellDmuxSetStream(demuxerHandle=%d, streamAddress=0x%x, streamSize=%d, discontinuity=%d, userData=0x%llx",
		demuxerHandle, streamAddress, streamSize, discontinuity, userData);

	std::shared_ptr<Demuxer> dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (dmux->is_running.exchange(true))
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
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

int cellDmuxResetStream(u32 demuxerHandle)
{
	cellDmux.Warning("cellDmuxResetStream(demuxerHandle=%d)", demuxerHandle);

	std::shared_ptr<Demuxer> dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.push(DemuxerTask(dmuxResetStream), &dmux->is_closed);
	return CELL_OK;
}

int cellDmuxResetStreamAndWaitDone(u32 demuxerHandle)
{
	cellDmux.Warning("cellDmuxResetStreamAndWaitDone(demuxerHandle=%d)", demuxerHandle);

	std::shared_ptr<Demuxer> dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
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
			cellDmux.Warning("cellDmuxResetStreamAndWaitDone(%d) aborted", demuxerHandle);
			return CELL_OK;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
	}

	return CELL_OK;
}

int cellDmuxQueryEsAttr(vm::ptr<const CellDmuxType> demuxerType, vm::ptr<const CellCodecEsFilterId> esFilterId,
						const u32 esSpecificInfo_addr, vm::ptr<CellDmuxEsAttr> esAttr)
{
	cellDmux.Warning("cellDmuxQueryEsAttr(demuxerType_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
		demuxerType.addr(), esFilterId.addr(), esSpecificInfo_addr, esAttr.addr());

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check esFilterId and esSpecificInfo correctly
	dmuxQueryEsAttr(0, esFilterId, esSpecificInfo_addr, esAttr);
	return CELL_OK;
}

int cellDmuxQueryEsAttr2(vm::ptr<const CellDmuxType2> demuxerType2, vm::ptr<const CellCodecEsFilterId> esFilterId,
						 const u32 esSpecificInfo_addr, vm::ptr<CellDmuxEsAttr> esAttr)
{
	cellDmux.Warning("cellDmuxQueryEsAttr2(demuxerType2_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
		demuxerType2.addr(), esFilterId.addr(), esSpecificInfo_addr, esAttr.addr());

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerType2, esFilterId and esSpecificInfo correctly
	dmuxQueryEsAttr(demuxerType2->streamSpecificInfo_addr, esFilterId, esSpecificInfo_addr, esAttr);
	return CELL_OK;
}

int cellDmuxEnableEs(u32 demuxerHandle, vm::ptr<const CellCodecEsFilterId> esFilterId, 
					 vm::ptr<const CellDmuxEsResource> esResourceInfo, vm::ptr<const CellDmuxEsCb> esCb,
					 const u32 esSpecificInfo_addr, vm::ptr<u32> esHandle)
{
	cellDmux.Warning("cellDmuxEnableEs(demuxerHandle=%d, esFilterId_addr=0x%x, esResourceInfo_addr=0x%x, esCb_addr=0x%x, "
		"esSpecificInfo_addr=0x%x, esHandle_addr=0x%x)", demuxerHandle, esFilterId.addr(), esResourceInfo.addr(),
		esCb.addr(), esSpecificInfo_addr, esHandle.addr());

	std::shared_ptr<Demuxer> dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check esFilterId, esResourceInfo, esCb and esSpecificInfo correctly

	std::shared_ptr<ElementaryStream> es(new ElementaryStream(dmux.get(), esResourceInfo->memAddr, esResourceInfo->memSize,
		esFilterId->filterIdMajor, esFilterId->filterIdMinor, esFilterId->supplementalInfo1, esFilterId->supplementalInfo2,
		esCb->cbEsMsgFunc, esCb->cbArg, esSpecificInfo_addr));

	u32 id = Emu.GetIdManager().GetNewID(es);
	es->id = id;
	*esHandle = id;

	cellDmux.Warning("*** New ES(dmux=%d, addr=0x%x, size=0x%x, filter(0x%x, 0x%x, 0x%x, 0x%x), cb=0x%x(arg=0x%x), spec=0x%x): id = %d",
		demuxerHandle, es->memAddr, es->memSize, es->fidMajor, es->fidMinor, es->sup1, es->sup2, es->cbFunc, es->cbArg, es->spec, id);

	DemuxerTask task(dmuxEnableEs);
	task.es.es = id;
	task.es.es_ptr = es.get();

	dmux->job.push(task, &dmux->is_closed);
	return CELL_OK;
}

int cellDmuxDisableEs(u32 esHandle)
{
	cellDmux.Warning("cellDmuxDisableEs(esHandle=0x%x)", esHandle);

	std::shared_ptr<ElementaryStream> es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxDisableEs);
	task.es.es = esHandle;
	task.es.es_ptr = es.get();

	es->dmux->job.push(task, &es->dmux->is_closed);
	return CELL_OK;
}

int cellDmuxResetEs(u32 esHandle)
{
	cellDmux.Log("cellDmuxResetEs(esHandle=0x%x)", esHandle);

	std::shared_ptr<ElementaryStream> es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxResetEs);
	task.es.es = esHandle;
	task.es.es_ptr = es.get();

	es->dmux->job.push(task, &es->dmux->is_closed);
	return CELL_OK;
}

int cellDmuxGetAu(u32 esHandle, vm::ptr<u32> auInfo_ptr, vm::ptr<u32> auSpecificInfo_ptr)
{
	cellDmux.Log("cellDmuxGetAu(esHandle=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfo_ptr.addr(), auSpecificInfo_ptr.addr());

	std::shared_ptr<ElementaryStream> es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, true, spec, true))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	*auInfo_ptr = info;
	*auSpecificInfo_ptr = spec;
	return CELL_OK;
}

int cellDmuxPeekAu(u32 esHandle, vm::ptr<u32> auInfo_ptr, vm::ptr<u32> auSpecificInfo_ptr)
{
	cellDmux.Log("cellDmuxPeekAu(esHandle=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfo_ptr.addr(), auSpecificInfo_ptr.addr());

	std::shared_ptr<ElementaryStream> es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, true, spec, false))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	*auInfo_ptr = info;
	*auSpecificInfo_ptr = spec;
	return CELL_OK;
}

int cellDmuxGetAuEx(u32 esHandle, vm::ptr<u32> auInfoEx_ptr, vm::ptr<u32> auSpecificInfo_ptr)
{
	cellDmux.Log("cellDmuxGetAuEx(esHandle=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfoEx_ptr.addr(), auSpecificInfo_ptr.addr());

	std::shared_ptr<ElementaryStream> es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, false, spec, true))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	*auInfoEx_ptr = info;
	*auSpecificInfo_ptr = spec;
	return CELL_OK;
}

int cellDmuxPeekAuEx(u32 esHandle, vm::ptr<u32> auInfoEx_ptr, vm::ptr<u32> auSpecificInfo_ptr)
{
	cellDmux.Log("cellDmuxPeekAuEx(esHandle=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfoEx_ptr.addr(), auSpecificInfo_ptr.addr());

	std::shared_ptr<ElementaryStream> es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, false, spec, false))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	*auInfoEx_ptr = info;
	*auSpecificInfo_ptr = spec;
	return CELL_OK;
}

int cellDmuxReleaseAu(u32 esHandle)
{
	cellDmux.Log("cellDmuxReleaseAu(esHandle=0x%x)", esHandle);

	std::shared_ptr<ElementaryStream> es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!es->release())
	{
		return CELL_DMUX_ERROR_SEQ;
	}
	return CELL_OK;
}

int cellDmuxFlushEs(u32 esHandle)
{
	cellDmux.Warning("cellDmuxFlushEs(esHandle=0x%x)", esHandle);

	std::shared_ptr<ElementaryStream> es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxFlushEs);
	task.es.es = esHandle;
	task.es.es_ptr = es.get();

	es->dmux->job.push(task, &es->dmux->is_closed);
	return CELL_OK;
}

Module cellDmux("cellDmux", []()
{
	REG_FUNC(cellDmux, cellDmuxQueryAttr);
	REG_FUNC(cellDmux, cellDmuxQueryAttr2);
	REG_FUNC(cellDmux, cellDmuxOpen);
	REG_FUNC(cellDmux, cellDmuxOpenEx);
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
