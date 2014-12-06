#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "cellPamf.h"
#include "cellDmux.h"

Module *cellDmux = nullptr;

PesHeader::PesHeader(DemuxerStream& stream)
	: pts(0xffffffffffffffffull)
	, dts(0xffffffffffffffffull)
	, size(0)
	, has_ts(false)
{
	u16 header;
	stream.get(header);
	stream.get(size);
	if (size)
	{
		u8 empty = 0;
		u8 v;
		while (true)
		{
			stream.get(v);
			if (v != 0xFF) break; // skip padding bytes
			empty++;
			if (empty == size) return;
		};

		if ((v & 0xF0) == 0x20 && (size - empty) >= 5) // pts only
		{
			has_ts = true;
			pts = stream.get_ts(v);
			stream.skip(size - empty - 5);
		}
		else
		{
			has_ts = true;
			if ((v & 0xF0) != 0x30 || (size - empty) < 10)
			{
				cellDmux->Error("PesHeader(): pts not found");
				Emu.Pause();
			}
			pts = stream.get_ts(v);
			stream.get(v);
			if ((v & 0xF0) != 0x10)
			{
				cellDmux->Error("PesHeader(): dts not found");
				Emu.Pause();
			}
			dts = stream.get_ts(v);
			stream.skip(size - empty - 10);
		}
	}
}

bool ElementaryStream::is_full(u32 space)
{
	if (released < put_count)
	{
		if (entries.IsFull())
		{
			return true;
		}

		u32 first; assert(entries.Peek(first, &sq_no_wait));
		if (first >= put)
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

		if (is_full(size))
		{
			cellDmux->Error("es::push_au(): buffer is full");
			Emu.Pause();
			return;
		}

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

		put = a128(put + 128 + size);

		put_count++;
	}
	assert(entries.Push(addr, &sq_no_wait));
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
		cellDmux->Error("es::release(): buffer is empty");
		Emu.Pause();
		return false;
	}
	if (released >= got_count)
	{
		cellDmux->Error("es::release(): buffer has not been seen yet");
		Emu.Pause();
		return false;
	}

	u32 addr; assert(entries.Pop(addr, &sq_no_wait));
	released++;
	return true;
}

bool ElementaryStream::peek(u32& out_data, bool no_ex, u32& out_spec, bool update_index)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (got_count < released)
	{
		cellDmux->Error("es::peek() error: got_count(%d) < released(%d) (put_count=%d)", got_count, released, put_count);
		Emu.Pause();
		return false;
	}
	if (got_count >= put_count)
	{
		return false;
	}

	u32 addr; assert(entries.Peek(addr, &sq_no_wait, got_count - released));
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
	entries.Clear();
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
		attr->memSize = 0x8000; // 0x73d9 from ps3

	cellDmux->Warning("*** filter(0x%x, 0x%x, 0x%x, 0x%x)", (u32)esFilterId->filterIdMajor, (u32)esFilterId->filterIdMinor,
		(u32)esFilterId->supplementalInfo1, (u32)esFilterId->supplementalInfo2);
}

u32 dmuxOpen(Demuxer* data)
{
	Demuxer& dmux = *data;

	u32 dmux_id = cellDmux->GetNewId(data);

	dmux.id = dmux_id;

	dmux.dmuxCb = (PPUThread*)&Emu.GetCPU().AddThread(CPU_THREAD_PPU);
	dmux.dmuxCb->SetName("Demuxer[" + std::to_string(dmux_id) + "] Callback");
	dmux.dmuxCb->SetEntry(0);
	dmux.dmuxCb->SetPrio(1001);
	dmux.dmuxCb->SetStackSize(0x10000);
	dmux.dmuxCb->InitStack();
	dmux.dmuxCb->InitRegs();
	dmux.dmuxCb->DoRun();

	thread t("Demuxer[" + std::to_string(dmux_id) + "] Thread", [&]()
	{
		cellDmux->Notice("Demuxer thread started (mem=0x%x, size=0x%x, cb=0x%x, arg=0x%x)", dmux.memAddr, dmux.memSize, dmux.cbFunc, dmux.cbArg);

		DemuxerTask task;
		DemuxerStream stream = {};
		ElementaryStream* esALL[192]; memset(esALL, 0, sizeof(esALL));
		ElementaryStream** esAVC = &esALL[0]; // AVC (max 16)
		ElementaryStream** esM2V = &esALL[16]; // MPEG-2 (max 16)
		ElementaryStream** esDATA = &esALL[32]; // user data (max 16)
		ElementaryStream** esATX = &esALL[48]; // ATRAC3+ (max 48)
		ElementaryStream** esAC3 = &esALL[96]; // AC3 (max 48)
		ElementaryStream** esPCM = &esALL[144]; // LPCM (max 48)

		u32 cb_add = 0;

		while (true)
		{
			if (Emu.IsStopped() || dmux.is_closed)
			{
				break;
			}
			
			if (!dmux.job.Peek(task, &sq_no_wait) && dmux.is_running && stream.addr)
			{
				// default task (demuxing) (if there is no other work)
				be_t<u32> code;
				be_t<u16> len;
				u8 ch;

				if (!stream.peek(code)) 
				{
					// demuxing finished
					auto dmuxMsg = vm::ptr<CellDmuxMsg>::make(a128(dmux.memAddr) + (cb_add ^= 16));
					dmuxMsg->msgType = CELL_DMUX_MSG_TYPE_DEMUX_DONE;
					dmuxMsg->supplementalInfo = stream.userdata;
					dmux.cbFunc.call(*dmux.dmuxCb, dmux.id, dmuxMsg, dmux.cbArg);

					dmux.is_running = false;
				}
				else switch (code.ToLE())
				{
				case PACK_START_CODE:
				{
					stream.skip(14);
				}
				break;

				case SYSTEM_HEADER_START_CODE:
				{
					stream.skip(18);
				}
				break;

				case PADDING_STREAM:
				{
					stream.skip(4);
					stream.get(len);
					stream.skip(len);
				}
				break;

				case PRIVATE_STREAM_2:
				{
					stream.skip(4);
					stream.get(len);
					stream.skip(len);
				}
				break;

				case PRIVATE_STREAM_1:
				{
					DemuxerStream backup = stream;

					// audio AT3+ (and probably LPCM or user data)
					stream.skip(4);
					stream.get(len);

					PesHeader pes(stream);

					// read additional header:
					stream.peek(ch); // ???
					//stream.skip(4);
					//pes.size += 4;

					if (esATX[ch])
					{
						ElementaryStream& es = *esATX[ch];
						if (es.raw_data.size() > 1024 * 1024)
						{
							stream = backup;
							std::this_thread::sleep_for(std::chrono::milliseconds(1));
							continue;
						}

						stream.skip(4);
						len -= 4;

						if (pes.has_ts)
						{
							es.last_dts = pes.dts;
							es.last_pts = pes.pts;
						}

						es.push(stream, len - pes.size - 3);

						while (true)
						{
							auto const size = es.raw_data.size() - es.raw_pos; // size of available new data
							auto const data = es.raw_data.data() + es.raw_pos; // pointer to available data

							if (size < 8) break; // skip if cannot read ATS header

							if (data[0] != 0x0f || data[1] != 0xd0)
							{
								cellDmux->Error("ATX: 0x0fd0 header not found (ats=0x%llx)", ((be_t<u64>*)data)->ToLE());
								Emu.Pause();
								return;
							}

							u32 frame_size = ((((u32)data[2] & 0x3) << 8) | (u32)data[3]) * 8 + 8;

							if (size < frame_size + 8) break; // skip non-complete AU

							if (es.isfull(frame_size + 8)) break; // skip if cannot push AU
							
							es.push_au(frame_size + 8, es.last_dts, es.last_pts, stream.userdata, false /* TODO: set correct value */, 0);

							auto esMsg = vm::ptr<CellDmuxEsMsg>::make(a128(dmux.memAddr) + (cb_add ^= 16));
							esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
							esMsg->supplementalInfo = stream.userdata;
							es.cbFunc.call(*dmux.dmuxCb, dmux.id, es.id, esMsg, es.cbArg);
						}
					}
					else
					{
						stream.skip(len - pes.size - 3);
					}
				}
				break;

				case 0x1e0: case 0x1e1: case 0x1e2: case 0x1e3:
				case 0x1e4: case 0x1e5: case 0x1e6: case 0x1e7:
				case 0x1e8: case 0x1e9: case 0x1ea: case 0x1eb:
				case 0x1ec: case 0x1ed: case 0x1ee: case 0x1ef:
				{
					// video AVC
					ch = code - 0x1e0;
					if (esAVC[ch])
					{
						ElementaryStream& es = *esAVC[ch];
						DemuxerStream backup = stream;

						stream.skip(4);
						stream.get(len);
						PesHeader pes(stream);

						const u32 old_size = (u32)es.raw_data.size();
						if (es.isfull(old_size))
						{
							stream = backup;
							std::this_thread::sleep_for(std::chrono::milliseconds(1));
							continue;
						}

						if ((pes.has_ts && old_size) || old_size >= 0x70000)
						{
							// push AU if it becomes too big or the next packet contains ts data
							es.push_au(old_size, es.last_dts, es.last_pts, stream.userdata, false /* TODO: set correct value */, 0);

							// callback
							auto esMsg = vm::ptr<CellDmuxEsMsg>::make(a128(dmux.memAddr) + (cb_add ^= 16));
							esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
							esMsg->supplementalInfo = stream.userdata;
							es.cbFunc.call(*dmux.dmuxCb, dmux.id, es.id, esMsg, es.cbArg);
						}
						
						if (pes.has_ts)
						{
							// preserve dts/pts for next AU
							es.last_dts = pes.dts;
							es.last_pts = pes.pts;
						}

						// reconstruction of MPEG2-PS stream for vdec module
						const u32 size = len + 6 /*- pes.size - 3*/;
						stream = backup;
						es.push(stream, size);
					}
					else
					{
						stream.skip(4);
						stream.get(len);
						stream.skip(len);
					}
				}
				break;

				case 0x1c0: case 0x1c1: case 0x1c2: case 0x1c3:
				case 0x1c4: case 0x1c5: case 0x1c6: case 0x1c7:
				case 0x1c8: case 0x1c9: case 0x1ca: case 0x1cb:
				case 0x1cc: case 0x1cd: case 0x1ce: case 0x1cf:
				case 0x1d0: case 0x1d1: case 0x1d2: case 0x1d3:
				case 0x1d4: case 0x1d5: case 0x1d6: case 0x1d7:
				case 0x1d8: case 0x1d9: case 0x1da: case 0x1db:
				case 0x1dc: case 0x1dd: case 0x1de: case 0x1df:
				{
					// unknown
					cellDmux->Warning("Unknown MPEG stream found");
					stream.skip(4);
					stream.get(len);
					stream.skip(len);
				}
				break;

				case USER_DATA_START_CODE:
				{
					cellDmux->Error("USER_DATA_START_CODE found");
					Emu.Pause();
					return;
				}

				default:
				{
					// search
					stream.skip(1);
				}
				}
				continue;
			}

			// wait for task with yielding (if no default work)
			if (!dmux.job.Pop(task, &dmux.is_closed))
			{
				break; // Emu is stopped
			}

			switch (task.type)
			{
			case dmuxSetStream:
			{
				if (task.stream.discontinuity)
				{
					cellDmux->Warning("dmuxSetStream (beginning)");
					for (u32 i = 0; i < 192; i++)
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
			}
			break;

			case dmuxResetStream:
			case dmuxResetStreamAndWaitDone:
			{
				auto dmuxMsg = vm::ptr<CellDmuxMsg>::make(a128(dmux.memAddr) + (cb_add ^= 16));
				dmuxMsg->msgType = CELL_DMUX_MSG_TYPE_DEMUX_DONE;
				dmuxMsg->supplementalInfo = stream.userdata;
				dmux.cbFunc.call(*dmux.dmuxCb, dmux.id, dmuxMsg, dmux.cbArg);

				stream = {};
				dmux.is_running = false;
				//if (task.type == dmuxResetStreamAndWaitDone)
				//{
				//}
			}
			break;

			case dmuxEnableEs:
			{
				ElementaryStream& es = *task.es.es_ptr;
				if (es.fidMajor >= 0xe0 &&
					es.fidMajor <= 0xef &&
					es.fidMinor == 0 &&
					es.sup1 == 1 &&
					es.sup2 == 0)
				{
					esAVC[es.fidMajor - 0xe0] = task.es.es_ptr;
				}
				else if (es.fidMajor == 0xbd &&
					es.fidMinor == 0 &&
					es.sup1 == 0 &&
					es.sup2 == 0)
				{
					esATX[0] = task.es.es_ptr;
				}
				else
				{
					cellDmux->Warning("dmuxEnableEs: (TODO) unsupported filter (0x%x, 0x%x, 0x%x, 0x%x)", es.fidMajor, es.fidMinor, es.sup1, es.sup2);
				}
				es.dmux = &dmux;
			}
			break;

			case dmuxDisableEs:
			{
				ElementaryStream& es = *task.es.es_ptr;
				if (es.dmux != &dmux)
				{
					cellDmux->Warning("dmuxDisableEs: invalid elementary stream");
					break;
				}
				for (u32 i = 0; i < 192; i++)
				{
					if (esALL[i] == &es)
					{
						esALL[i] = nullptr;
					}
				}
				es.dmux = nullptr;
				Emu.GetIdManager().RemoveID(task.es.es);
			}
			break;

			case dmuxFlushEs:
			{
				ElementaryStream& es = *task.es.es_ptr;

				const u32 old_size = (u32)es.raw_data.size();
				if (old_size && (es.fidMajor & 0xf0) == 0xe0)
				{
					// TODO (it's only for AVC, some ATX data may be lost)
					while (es.isfull(old_size))
					{
						if (Emu.IsStopped() || dmux.is_closed) break;

						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}

					es.push_au(old_size, es.last_dts, es.last_pts, stream.userdata, false, 0);

					// callback
					auto esMsg = vm::ptr<CellDmuxEsMsg>::make(a128(dmux.memAddr) + (cb_add ^= 16));
					esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
					esMsg->supplementalInfo = stream.userdata;
					es.cbFunc.call(*dmux.dmuxCb, dmux.id, es.id, esMsg, es.cbArg);
				}
				
				if (es.raw_data.size())
				{
					cellDmux->Error("dmuxFlushEs: 0x%x bytes lost (es_id=%d)", (u32)es.raw_data.size(), es.id);
				}

				// callback
				auto esMsg = vm::ptr<CellDmuxEsMsg>::make(a128(dmux.memAddr) + (cb_add ^= 16));
				esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_FLUSH_DONE;
				esMsg->supplementalInfo = stream.userdata;
				es.cbFunc.call(*dmux.dmuxCb, dmux.id, es.id, esMsg, es.cbArg);
			}
			break;

			case dmuxResetEs:
			{
				task.es.es_ptr->reset();
			}
			break;

			case dmuxClose: break;

			default:
			{
				cellDmux->Error("Demuxer thread error: unknown task(%d)", task.type);
				Emu.Pause();
				return;
			}	
			}
		}

		dmux.is_finished = true;
		if (Emu.IsStopped()) cellDmux->Warning("Demuxer thread aborted");
		if (dmux.is_closed) cellDmux->Notice("Demuxer thread ended");
	});

	t.detach();

	return dmux_id;
}

int cellDmuxQueryAttr(vm::ptr<const CellDmuxType> demuxerType, vm::ptr<CellDmuxAttr> demuxerAttr)
{
	cellDmux->Warning("cellDmuxQueryAttr(demuxerType_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType.addr(), demuxerAttr.addr());

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmuxQueryAttr(0, demuxerAttr);
	return CELL_OK;
}

int cellDmuxQueryAttr2(vm::ptr<const CellDmuxType2> demuxerType2, vm::ptr<CellDmuxAttr> demuxerAttr)
{
	cellDmux->Warning("cellDmuxQueryAttr2(demuxerType2_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType2.addr(), demuxerAttr.addr());

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
	cellDmux->Warning("cellDmuxOpen(demuxerType_addr=0x%x, demuxerResource_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.addr(), demuxerResource.addr(), demuxerCb.addr(), demuxerHandle.addr());

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerResource and demuxerCb arguments

	*demuxerHandle = dmuxOpen(new Demuxer(demuxerResource->memAddr, demuxerResource->memSize, vm::ptr<CellDmuxCbMsg>::make(demuxerCb->cbMsgFunc.addr()), demuxerCb->cbArg));

	return CELL_OK;
}

int cellDmuxOpenEx(vm::ptr<const CellDmuxType> demuxerType, vm::ptr<const CellDmuxResourceEx> demuxerResourceEx, 
	vm::ptr<const CellDmuxCb> demuxerCb, vm::ptr<u32> demuxerHandle)
{
	cellDmux->Warning("cellDmuxOpenEx(demuxerType_addr=0x%x, demuxerResourceEx_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.addr(), demuxerResourceEx.addr(), demuxerCb.addr(), demuxerHandle.addr());

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerResourceEx and demuxerCb arguments

	*demuxerHandle = dmuxOpen(new Demuxer(demuxerResourceEx->memAddr, demuxerResourceEx->memSize, vm::ptr<CellDmuxCbMsg>::make(demuxerCb->cbMsgFunc.addr()), demuxerCb->cbArg));

	return CELL_OK;
}

int cellDmuxOpen2(vm::ptr<const CellDmuxType2> demuxerType2, vm::ptr<const CellDmuxResource2> demuxerResource2, 
	vm::ptr<const CellDmuxCb> demuxerCb, vm::ptr<u32> demuxerHandle)
{
	cellDmux->Warning("cellDmuxOpen2(demuxerType2_addr=0x%x, demuxerResource2_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType2.addr(), demuxerResource2.addr(), demuxerCb.addr(), demuxerHandle.addr());

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerType2, demuxerResource2 and demuxerCb arguments

	*demuxerHandle = dmuxOpen(new Demuxer(demuxerResource2->memAddr, demuxerResource2->memSize, vm::ptr<CellDmuxCbMsg>::make(demuxerCb->cbMsgFunc.addr()), demuxerCb->cbArg));

	return CELL_OK;
}

int cellDmuxClose(u32 demuxerHandle)
{
	cellDmux->Warning("cellDmuxClose(demuxerHandle=%d)", demuxerHandle);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->is_closed = true;
	dmux->job.Push(DemuxerTask(dmuxClose), &sq_no_wait);

	while (!dmux->is_finished)
	{
		if (Emu.IsStopped())
		{
			cellDmux->Warning("cellDmuxClose(%d) aborted", demuxerHandle);
			return CELL_OK;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	if (dmux->dmuxCb) Emu.GetCPU().RemoveThread(dmux->dmuxCb->GetId());
	Emu.GetIdManager().RemoveID(demuxerHandle);
	return CELL_OK;
}

int cellDmuxSetStream(u32 demuxerHandle, const u32 streamAddress, u32 streamSize, bool discontinuity, u64 userData)
{
	cellDmux->Log("cellDmuxSetStream(demuxerHandle=%d, streamAddress=0x%x, streamSize=%d, discontinuity=%d, userData=0x%llx",
		demuxerHandle, streamAddress, streamSize, discontinuity, userData);

	Demuxer* dmux;
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

	dmux->job.Push(task, &dmux->is_closed);
	return CELL_OK;
}

int cellDmuxResetStream(u32 demuxerHandle)
{
	cellDmux->Warning("cellDmuxResetStream(demuxerHandle=%d)", demuxerHandle);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.Push(DemuxerTask(dmuxResetStream), &dmux->is_closed);
	return CELL_OK;
}

int cellDmuxResetStreamAndWaitDone(u32 demuxerHandle)
{
	cellDmux->Warning("cellDmuxResetStreamAndWaitDone(demuxerHandle=%d)", demuxerHandle);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.Push(DemuxerTask(dmuxResetStreamAndWaitDone), &dmux->is_closed);
	while (dmux->is_running && !dmux->is_closed) // TODO: ensure that it is safe
	{
		if (Emu.IsStopped())
		{
			cellDmux->Warning("cellDmuxResetStreamAndWaitDone(%d) aborted", demuxerHandle);
			return CELL_OK;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return CELL_OK;
}

int cellDmuxQueryEsAttr(vm::ptr<const CellDmuxType> demuxerType, vm::ptr<const CellCodecEsFilterId> esFilterId,
						const u32 esSpecificInfo_addr, vm::ptr<CellDmuxEsAttr> esAttr)
{
	cellDmux->Warning("cellDmuxQueryEsAttr(demuxerType_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
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
	cellDmux->Warning("cellDmuxQueryEsAttr2(demuxerType2_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
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
	cellDmux->Warning("cellDmuxEnableEs(demuxerHandle=%d, esFilterId_addr=0x%x, esResourceInfo_addr=0x%x, esCb_addr=0x%x, "
		"esSpecificInfo_addr=0x%x, esHandle_addr=0x%x)", demuxerHandle, esFilterId.addr(), esResourceInfo.addr(),
		esCb.addr(), esSpecificInfo_addr, esHandle.addr());

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check esFilterId, esResourceInfo, esCb and esSpecificInfo correctly

	ElementaryStream* es = new ElementaryStream(dmux, esResourceInfo->memAddr, esResourceInfo->memSize,
		esFilterId->filterIdMajor, esFilterId->filterIdMinor, esFilterId->supplementalInfo1, esFilterId->supplementalInfo2,
		vm::ptr<CellDmuxCbEsMsg>::make(esCb->cbEsMsgFunc.addr()), esCb->cbArg, esSpecificInfo_addr);

	u32 id = cellDmux->GetNewId(es);
	es->id = id;
	*esHandle = id;

	cellDmux->Warning("*** New ES(dmux=%d, addr=0x%x, size=0x%x, filter(0x%x, 0x%x, 0x%x, 0x%x), cb=0x%x(arg=0x%x), spec=0x%x): id = %d",
		demuxerHandle, es->memAddr, es->memSize, es->fidMajor, es->fidMinor, es->sup1, es->sup2, esCb->cbEsMsgFunc.addr(), es->cbArg, es->spec, id);

	DemuxerTask task(dmuxEnableEs);
	task.es.es = id;
	task.es.es_ptr = es;

	dmux->job.Push(task, &dmux->is_closed);
	return CELL_OK;
}

int cellDmuxDisableEs(u32 esHandle)
{
	cellDmux->Warning("cellDmuxDisableEs(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxDisableEs);
	task.es.es = esHandle;
	task.es.es_ptr = es;

	es->dmux->job.Push(task, &es->dmux->is_closed);
	return CELL_OK;
}

int cellDmuxResetEs(u32 esHandle)
{
	cellDmux->Log("cellDmuxResetEs(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxResetEs);
	task.es.es = esHandle;
	task.es.es_ptr = es;

	es->dmux->job.Push(task, &es->dmux->is_closed);
	return CELL_OK;
}

int cellDmuxGetAu(u32 esHandle, vm::ptr<u32> auInfo_ptr, vm::ptr<u32> auSpecificInfo_ptr)
{
	cellDmux->Log("cellDmuxGetAu(esHandle=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfo_ptr.addr(), auSpecificInfo_ptr.addr());

	ElementaryStream* es;
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
	cellDmux->Log("cellDmuxPeekAu(esHandle=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfo_ptr.addr(), auSpecificInfo_ptr.addr());

	ElementaryStream* es;
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
	cellDmux->Log("cellDmuxGetAuEx(esHandle=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfoEx_ptr.addr(), auSpecificInfo_ptr.addr());

	ElementaryStream* es;
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
	cellDmux->Log("cellDmuxPeekAuEx(esHandle=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfoEx_ptr.addr(), auSpecificInfo_ptr.addr());

	ElementaryStream* es;
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
	cellDmux->Log("cellDmuxReleaseAu(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
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
	cellDmux->Warning("cellDmuxFlushEs(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxFlushEs);
	task.es.es = esHandle;
	task.es.es_ptr = es;

	es->dmux->job.Push(task, &es->dmux->is_closed);
	return CELL_OK;
}

void cellDmux_init(Module *pxThis)
{
	cellDmux = pxThis;

	cellDmux->AddFunc(0xa2d4189b, cellDmuxQueryAttr);
	cellDmux->AddFunc(0x3f76e3cd, cellDmuxQueryAttr2);
	cellDmux->AddFunc(0x68492de9, cellDmuxOpen);
	cellDmux->AddFunc(0xf6c23560, cellDmuxOpenEx);
	cellDmux->AddFunc(0x11bc3a6c, cellDmuxOpen2);
	cellDmux->AddFunc(0x8c692521, cellDmuxClose);
	cellDmux->AddFunc(0x04e7499f, cellDmuxSetStream);
	cellDmux->AddFunc(0x5d345de9, cellDmuxResetStream);
	cellDmux->AddFunc(0xccff1284, cellDmuxResetStreamAndWaitDone);
	cellDmux->AddFunc(0x02170d1a, cellDmuxQueryEsAttr);
	cellDmux->AddFunc(0x52911bcf, cellDmuxQueryEsAttr2);
	cellDmux->AddFunc(0x7b56dc3f, cellDmuxEnableEs);
	cellDmux->AddFunc(0x05371c8d, cellDmuxDisableEs);
	cellDmux->AddFunc(0x21d424f0, cellDmuxResetEs);
	cellDmux->AddFunc(0x42c716b5, cellDmuxGetAu);
	cellDmux->AddFunc(0x2750c5e0, cellDmuxPeekAu);
	cellDmux->AddFunc(0x2c9a5857, cellDmuxGetAuEx);
	cellDmux->AddFunc(0x002e8da2, cellDmuxPeekAuEx);
	cellDmux->AddFunc(0x24ea6474, cellDmuxReleaseAu);
	cellDmux->AddFunc(0xebb3b2bd, cellDmuxFlushEs);
}
