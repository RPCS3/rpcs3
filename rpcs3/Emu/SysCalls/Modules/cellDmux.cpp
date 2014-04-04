#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellPamf.h"
#include "cellDmux.h"

void cellDmux_init();
Module cellDmux(0x0007, cellDmux_init);

void dmuxQueryAttr(u32 info_addr /* may be 0 */, mem_ptr_t<CellDmuxAttr> attr)
{
	attr->demuxerVerLower = 0x280000; // TODO: check values
	attr->demuxerVerUpper = 0x260000;
	attr->memSize = 0x10000; // 0x3e8e6 from ps3
}

void dmuxQueryEsAttr(u32 info_addr /* may be 0 */, const mem_ptr_t<CellCodecEsFilterId> esFilterId,
					 const u32 esSpecificInfo_addr, mem_ptr_t<CellDmuxEsAttr> attr)
{
	if (esFilterId->filterIdMajor >= 0xe0)
		attr->memSize = 0x500000; // 0x45fa49 from ps3
	else
		attr->memSize = 0x8000; // 0x73d9 from ps3

	cellDmux.Warning("*** filter(0x%x, 0x%x, 0x%x, 0x%x)", (u32)esFilterId->filterIdMajor, (u32)esFilterId->filterIdMinor,
		(u32)esFilterId->supplementalInfo1, (u32)esFilterId->supplementalInfo2);
}

u32 dmuxOpen(Demuxer* data)
{
	Demuxer& dmux = *data;

	dmux.dmuxCb = &Emu.GetCPU().AddThread(CPU_THREAD_PPU);

	u32 dmux_id = cellDmux.GetNewId(data);

	dmux.id = dmux_id;

	dmux.dmuxCb->SetName("Demuxer[" + std::to_string(dmux_id) + "] Callback");

	thread t("Demuxer[" + std::to_string(dmux_id) + "] Thread", [&]()
	{
		ConLog.Write("Demuxer enter (mem=0x%x, size=0x%x, cb=0x%x, arg=0x%x)", dmux.memAddr, dmux.memSize, dmux.cbFunc, dmux.cbArg);

		DemuxerTask task;
		DemuxerStream stream;
		ElementaryStream* esALL[192]; memset(esALL, 0, sizeof(esALL));
		ElementaryStream** esAVC = &esALL[0]; // AVC (max 16)
		ElementaryStream** esM2V = &esALL[16]; // MPEG-2 (max 16)
		ElementaryStream** esDATA = &esALL[32]; // user data (max 16)
		ElementaryStream** esATX = &esALL[48]; // ATRAC3+ (max 48)
		ElementaryStream** esAC3 = &esALL[96]; // AC3 (max 48)
		ElementaryStream** esPCM = &esALL[144]; // LPCM (max 48)

		u32 cb_add = 0;

		u32 updates_count = 0;
		u32 updates_signaled = 0;

		while (true)
		{
			if (Emu.IsStopped())
			{
				break;
			}
			
			if (dmux.job.IsEmpty() && dmux.is_running)
			{
				// default task (demuxing) (if there is no other work)
				be_t<u32> code;
				be_t<u16> len;
				u8 ch;

				if (!stream.peek(code)) 
				{
					dmux.is_running = false;
					// demuxing finished
					mem_ptr_t<CellDmuxMsg> dmuxMsg(a128(dmux.memAddr) + (cb_add ^= 16));
					dmuxMsg->msgType = CELL_DMUX_MSG_TYPE_DEMUX_DONE;
					dmuxMsg->supplementalInfo = stream.userdata;
					/*Callback cb;
					cb.SetAddr(dmux.cbFunc);
					cb.Handle(dmux.id, dmuxMsg.GetAddr(), dmux.cbArg);
					cb.Branch(task.type == dmuxResetStreamAndWaitDone);*/
					dmux.dmuxCb->ExecAsCallback(dmux.cbFunc, true, dmux.id, dmuxMsg.GetAddr(), dmux.cbArg);
					updates_signaled++;
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

						if (!pes.new_au) // temporarily
						{
							ConLog.Error("No pts info found");
						}

						// read additional header:
						stream.peek(ch); // ???
						//stream.skip(4);
						//pes.size += 4;

						if (esATX[ch])
						{
							ElementaryStream& es = *esATX[ch];
							while (es.isfull())
							{
								if (Emu.IsStopped())
								{
									ConLog.Warning("esATX[%d] was full, waiting aborted", ch);
									return;
								}
								Sleep(1);
							}

							if (es.hasunseen()) // hack, probably useless
							{
								stream = backup;
								continue;
							}

							stream.skip(4);
							len -= 4;

							es.push(stream, len - pes.size - 3, pes);
							es.finish(stream);
							//ConLog.Write("*** AT3+ AU sent (len=0x%x, pts=0x%llx)", len - pes.size - 3, pes.pts);
							
							mem_ptr_t<CellDmuxEsMsg> esMsg(a128(dmux.memAddr) + (cb_add ^= 16));
							esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
							esMsg->supplementalInfo = stream.userdata;
							/*Callback cb;
							cb.SetAddr(es.cbFunc);
							cb.Handle(dmux.id, es.id, esMsg.GetAddr(), es.cbArg);
							cb.Branch(false);*/
							dmux.dmuxCb->ExecAsCallback(es.cbFunc, false, dmux.id, es.id, esMsg.GetAddr(), es.cbArg);
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
							while (es.isfull())
							{
								if (Emu.IsStopped())
								{
									ConLog.Warning("esAVC[%d] was full, waiting aborted", ch);
									return;
								}
								Sleep(1);
							}

							DemuxerStream backup = stream;

							stream.skip(4);
							stream.get(len);
							PesHeader pes(stream);

							if (es.freespace() < (u32)(len + 6))
							{
								pes.new_au = true;
							}

							if (pes.new_au && es.hasdata()) // new AU detected
							{
								if (es.hasunseen()) // hack, probably useless
								{
									stream = backup;
									continue;
								}
								es.finish(stream);
								// callback
								mem_ptr_t<CellDmuxEsMsg> esMsg(a128(dmux.memAddr) + (cb_add ^= 16));
								esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
								esMsg->supplementalInfo = stream.userdata;
								/*Callback cb;
								cb.SetAddr(es.cbFunc);
								cb.Handle(dmux.id, es.id, esMsg.GetAddr(), es.cbArg);
								cb.Branch(false);*/
								dmux.dmuxCb->ExecAsCallback(es.cbFunc, false, dmux.id, es.id, esMsg.GetAddr(), es.cbArg);
							}

							if (pes.new_au)
							{
								//ConLog.Write("*** AVC AU detected (pts=0x%llx, dts=0x%llx)", pes.pts, pes.dts);
							}

							if (es.isfull())
							{
								stream = backup;
								continue;
							}

							//reconstruction of MPEG2-PS stream for vdec module
							stream = backup;
							es.push(stream, len + 6 /*- pes.size - 3*/, pes);
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
						ConLog.Warning("Unknown MPEG stream found");
						stream.skip(4);
						stream.get(len);
						stream.skip(len);
					}
					break;

				case USER_DATA_START_CODE:
					{
						ConLog.Error("USER_DATA_START_CODE found");
						return;
					}

				default:
					{
						// search
						stream.skip(1);
					}
					break;

				}
				continue;
			}

			// wait for task with yielding (if no default work)
			if (!dmux.job.Pop(task))
			{
				break; // Emu is stopped
			}

			switch (task.type)
			{
			case dmuxSetStream:
				{
					if (task.stream.discontinuity)
					{
						ConLog.Warning("dmuxSetStream (beginning)");
						for (u32 i = 0; i < 192; i++)
						{
							if (esALL[i])
							{
								esALL[i]->reset();
							}
						}
						updates_count = 0;
						updates_signaled = 0;
					}

					if (updates_count != updates_signaled)
					{
						ConLog.Error("dmuxSetStream: stream update inconsistency (input=%d, signaled=%d)", updates_count, updates_signaled);
						return;
					}

					updates_count++;
					stream = task.stream;
					//ConLog.Write("*** stream updated(addr=0x%x, size=0x%x, discont=%d, userdata=0x%llx)",
						//stream.addr, stream.size, stream.discontinuity, stream.userdata);

					dmux.is_running = true;
					dmux.fbSetStream.Push(task.stream.addr); // feedback
				}
				break;

			case dmuxResetStream:
			case dmuxResetStreamAndWaitDone:
				{
					mem_ptr_t<CellDmuxMsg> dmuxMsg(a128(dmux.memAddr) + (cb_add ^= 16));
					dmuxMsg->msgType = CELL_DMUX_MSG_TYPE_DEMUX_DONE;
					dmuxMsg->supplementalInfo = stream.userdata;
					/*Callback cb;
					cb.SetAddr(dmux.cbFunc);
					cb.Handle(dmux.id, dmuxMsg.GetAddr(), dmux.cbArg);
					cb.Branch(task.type == dmuxResetStreamAndWaitDone);*/
					dmux.dmuxCb->ExecAsCallback(dmux.cbFunc, task.type == dmuxResetStreamAndWaitDone,
						dmux.id, dmuxMsg.GetAddr(), dmux.cbArg);

					updates_signaled++;
					dmux.is_running = false;
					if (task.type == dmuxResetStreamAndWaitDone)
					{
						dmux.fbSetStream.Push(0);
					}
				}
				break;

			case dmuxClose:
				{
					dmux.is_finished = true;
					ConLog.Write("Demuxer exit");
					return;
				}

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
						ConLog.Warning("dmuxEnableEs: (TODO) unsupported filter (0x%x, 0x%x, 0x%x, 0x%x)", es.fidMajor, es.fidMinor, es.sup1, es.sup2);
					}
					es.dmux = &dmux;
				}
				break;

			case dmuxDisableEs:
				{
					ElementaryStream& es = *task.es.es_ptr;
					if (es.dmux != &dmux)
					{
						ConLog.Warning("dmuxDisableEs: invalid elementary stream");
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

			/*case dmuxReleaseAu:
				{
					task.es.es_ptr->release();
				}
				break;*/

			case dmuxFlushEs:
				{
					ElementaryStream& es = *task.es.es_ptr;

					if (es.hasdata())
					{
						es.finish(stream);
						// callback
						mem_ptr_t<CellDmuxEsMsg> esMsg(a128(dmux.memAddr) + (cb_add ^= 16));
						esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_AU_FOUND;
						esMsg->supplementalInfo = stream.userdata;
						/*Callback cb;
						cb.SetAddr(es.cbFunc);
						cb.Handle(dmux.id, es.id, esMsg.GetAddr(), es.cbArg);
						cb.Branch(false);*/
						dmux.dmuxCb->ExecAsCallback(es.cbFunc, false, dmux.id, es.id, esMsg.GetAddr(), es.cbArg);
					}

					// callback
					mem_ptr_t<CellDmuxEsMsg> esMsg(a128(dmux.memAddr) + (cb_add ^= 16));
					esMsg->msgType = CELL_DMUX_ES_MSG_TYPE_FLUSH_DONE;
					esMsg->supplementalInfo = stream.userdata;
					/*Callback cb;
					cb.SetAddr(es.cbFunc);
					cb.Handle(dmux.id, es.id, esMsg.GetAddr(), es.cbArg);
					cb.Branch(false);*/
					dmux.dmuxCb->ExecAsCallback(es.cbFunc, false, dmux.id, es.id, esMsg.GetAddr(), es.cbArg);
				}
				break;

			case dmuxResetEs:
				{
					task.es.es_ptr->reset();
				}
				break;

			default:
				ConLog.Error("Demuxer error: unknown task(%d)", task.type);
				return;
			}
		}
		ConLog.Warning("Demuxer aborted");
	});

	t.detach();

	return dmux_id;
}

int cellDmuxQueryAttr(const mem_ptr_t<CellDmuxType> demuxerType, mem_ptr_t<CellDmuxAttr> demuxerAttr)
{
	cellDmux.Warning("cellDmuxQueryAttr(demuxerType_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType.GetAddr(), demuxerAttr.GetAddr());

	if (!demuxerType.IsGood() || !demuxerAttr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmuxQueryAttr(0, demuxerAttr);
	return CELL_OK;
}

int cellDmuxQueryAttr2(const mem_ptr_t<CellDmuxType2> demuxerType2, mem_ptr_t<CellDmuxAttr> demuxerAttr)
{
	cellDmux.Warning("cellDmuxQueryAttr2(demuxerType2_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType2.GetAddr(), demuxerAttr.GetAddr());

	if (!demuxerType2.IsGood() || !demuxerAttr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmuxQueryAttr(demuxerType2->streamSpecificInfo_addr, demuxerAttr);
	return CELL_OK;
}

int cellDmuxOpen(const mem_ptr_t<CellDmuxType> demuxerType, const mem_ptr_t<CellDmuxResource> demuxerResource, 
				 const mem_ptr_t<CellDmuxCb> demuxerCb, mem32_t demuxerHandle)
{
	cellDmux.Warning("cellDmuxOpen(demuxerType_addr=0x%x, demuxerResource_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.GetAddr(), demuxerResource.GetAddr(), demuxerCb.GetAddr(), demuxerHandle.GetAddr());

	if (!demuxerType.IsGood() || !demuxerResource.IsGood() || !demuxerCb.IsGood() || !demuxerHandle.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!Memory.IsGoodAddr(demuxerResource->memAddr, demuxerResource->memSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	// TODO: check demuxerResource and demuxerCb arguments

	demuxerHandle = dmuxOpen(new Demuxer(demuxerResource->memAddr, demuxerResource->memSize, demuxerCb->cbMsgFunc, demuxerCb->cbArg_addr));

	return CELL_OK;
}

int cellDmuxOpenEx(const mem_ptr_t<CellDmuxType> demuxerType, const mem_ptr_t<CellDmuxResourceEx> demuxerResourceEx, 
				   const mem_ptr_t<CellDmuxCb> demuxerCb, mem32_t demuxerHandle)
{
	cellDmux.Warning("cellDmuxOpenEx(demuxerType_addr=0x%x, demuxerResourceEx_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.GetAddr(), demuxerResourceEx.GetAddr(), demuxerCb.GetAddr(), demuxerHandle.GetAddr());

	if (!demuxerType.IsGood() || !demuxerResourceEx.IsGood() || !demuxerCb.IsGood() || !demuxerHandle.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!Memory.IsGoodAddr(demuxerResourceEx->memAddr, demuxerResourceEx->memSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	// TODO: check demuxerResourceEx and demuxerCb arguments

	demuxerHandle = dmuxOpen(new Demuxer(demuxerResourceEx->memAddr, demuxerResourceEx->memSize, demuxerCb->cbMsgFunc, demuxerCb->cbArg_addr));

	return CELL_OK;
}

int cellDmuxOpen2(const mem_ptr_t<CellDmuxType2> demuxerType2, const mem_ptr_t<CellDmuxResource2> demuxerResource2, 
				  const mem_ptr_t<CellDmuxCb> demuxerCb, mem32_t demuxerHandle)
{
	cellDmux.Warning("cellDmuxOpen2(demuxerType2_addr=0x%x, demuxerResource2_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType2.GetAddr(), demuxerResource2.GetAddr(), demuxerCb.GetAddr(), demuxerHandle.GetAddr());

	if (!demuxerType2.IsGood() || !demuxerResource2.IsGood() || !demuxerCb.IsGood() || !demuxerHandle.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!Memory.IsGoodAddr(demuxerResource2->memAddr, demuxerResource2->memSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	// TODO: check demuxerType2, demuxerResource2 and demuxerCb arguments

	demuxerHandle = dmuxOpen(new Demuxer(demuxerResource2->memAddr, demuxerResource2->memSize, demuxerCb->cbMsgFunc, demuxerCb->cbArg_addr));

	return CELL_OK;
}

int cellDmuxClose(u32 demuxerHandle)
{
	cellDmux.Warning("cellDmuxClose(demuxerHandle=%d)", demuxerHandle);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.Push(DemuxerTask(dmuxClose));

	while (!dmux->is_finished)
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("cellDmuxClose(%d) aborted", demuxerHandle);
			return CELL_OK;
		}

		Sleep(1);
	}

	if (dmux->dmuxCb) Emu.GetCPU().RemoveThread(dmux->dmuxCb->GetId());
	Emu.GetIdManager().RemoveID(demuxerHandle);
	return CELL_OK;
}

int cellDmuxSetStream(u32 demuxerHandle, const u32 streamAddress, u32 streamSize, bool discontinuity, u64 userData)
{
	cellDmux.Log("cellDmuxSetStream(demuxerHandle=%d, streamAddress=0x%x, streamSize=%d, discontinuity=%d, userData=0x%llx",
		demuxerHandle, streamAddress, streamSize, discontinuity, userData);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!Memory.IsGoodAddr(streamAddress, streamSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (dmux->is_running)
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("cellDmuxSetStream(%d) aborted (waiting)", demuxerHandle);
			return CELL_OK;
		}
		Sleep(1);
		return CELL_DMUX_ERROR_BUSY;
	}

	DemuxerTask task(dmuxSetStream);
	auto& info = task.stream;
	info.addr = streamAddress;
	info.size = streamSize;
	info.discontinuity = discontinuity;
	info.userdata = userData;

	dmux->job.Push(task);

	u32 addr;
	if (!dmux->fbSetStream.Pop(addr))
	{
		ConLog.Warning("cellDmuxSetStream(%d) aborted (fbSetStream.Pop())", demuxerHandle);
		return CELL_OK;
	}
	if (addr != info.addr)
	{
		ConLog.Error("cellDmuxSetStream(%d): wrong stream queued (right=0x%x, queued=0x%x)", demuxerHandle, info.addr, addr);
		Emu.Pause();
	}
	return CELL_OK;
}

int cellDmuxResetStream(u32 demuxerHandle)
{
	cellDmux.Warning("cellDmuxResetStream(demuxerHandle=%d)", demuxerHandle);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.Push(DemuxerTask(dmuxResetStream));

	return CELL_OK;
}

int cellDmuxResetStreamAndWaitDone(u32 demuxerHandle)
{
	cellDmux.Warning("cellDmuxResetStreamAndWaitDone(demuxerHandle=%d)", demuxerHandle);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.Push(DemuxerTask(dmuxResetStreamAndWaitDone));

	u32 addr;
	if (!dmux->fbSetStream.Pop(addr))
	{
		ConLog.Warning("cellDmuxResetStreamAndWaitDone(%d) aborted (fbSetStream.Pop())", demuxerHandle);
		return CELL_OK;
	}
	if (addr != 0)
	{
		ConLog.Error("cellDmuxResetStreamAndWaitDone(%d): wrong stream queued (0x%x)", demuxerHandle, addr);
		Emu.Pause();
	}
	return CELL_OK;
}

int cellDmuxQueryEsAttr(const mem_ptr_t<CellDmuxType> demuxerType, const mem_ptr_t<CellCodecEsFilterId> esFilterId,
						const u32 esSpecificInfo_addr, mem_ptr_t<CellDmuxEsAttr> esAttr)
{
	cellDmux.Warning("cellDmuxQueryEsAttr(demuxerType_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
		demuxerType.GetAddr(), esFilterId.GetAddr(), esSpecificInfo_addr, esAttr.GetAddr());

	if (!demuxerType.IsGood() || !esFilterId.IsGood() || !esAttr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(esSpecificInfo_addr, 12))
	{
		cellDmux.Error("cellDmuxQueryEsAttr: invalid specific info addr (0x%x)", esSpecificInfo_addr);
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check esFilterId and esSpecificInfo correctly

	dmuxQueryEsAttr(0, esFilterId, esSpecificInfo_addr, esAttr);
	return CELL_OK;
}

int cellDmuxQueryEsAttr2(const mem_ptr_t<CellDmuxType2> demuxerType2, const mem_ptr_t<CellCodecEsFilterId> esFilterId,
						 const u32 esSpecificInfo_addr, mem_ptr_t<CellDmuxEsAttr> esAttr)
{
	cellDmux.Warning("cellDmuxQueryEsAttr2(demuxerType2_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
		demuxerType2.GetAddr(), esFilterId.GetAddr(), esSpecificInfo_addr, esAttr.GetAddr());

	if (!demuxerType2.IsGood() || !esFilterId.IsGood() || !esAttr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(esSpecificInfo_addr, 12))
	{
		cellDmux.Error("cellDmuxQueryEsAttr2: invalid specific info addr (0x%x)", esSpecificInfo_addr);
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerType2, esFilterId and esSpecificInfo correctly

	dmuxQueryEsAttr(demuxerType2->streamSpecificInfo_addr, esFilterId, esSpecificInfo_addr, esAttr);
	return CELL_OK;
}

int cellDmuxEnableEs(u32 demuxerHandle, const mem_ptr_t<CellCodecEsFilterId> esFilterId, 
					 const mem_ptr_t<CellDmuxEsResource> esResourceInfo, const mem_ptr_t<CellDmuxEsCb> esCb,
					 const u32 esSpecificInfo_addr, mem32_t esHandle)
{
	cellDmux.Warning("cellDmuxEnableEs(demuxerHandle=%d, esFilterId_addr=0x%x, esResourceInfo_addr=0x%x, esCb_addr=0x%x, "
		"esSpecificInfo_addr=0x%x, esHandle_addr=0x%x)", demuxerHandle, esFilterId.GetAddr(), esResourceInfo.GetAddr(),
		esCb.GetAddr(), esSpecificInfo_addr, esHandle.GetAddr());

	if (!esFilterId.IsGood() || !esResourceInfo.IsGood() || !esCb.IsGood() || !esHandle.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(esSpecificInfo_addr, 12))
	{
		cellDmux.Error("cellDmuxEnableEs: invalid specific info addr (0x%x)", esSpecificInfo_addr);
		return CELL_DMUX_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(esResourceInfo->memAddr, esResourceInfo->memSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check esFilterId, esResourceInfo, esCb and esSpecificInfo correctly

	ElementaryStream* es = new ElementaryStream(dmux, esResourceInfo->memAddr, esResourceInfo->memSize,
		esFilterId->filterIdMajor, esFilterId->filterIdMinor, esFilterId->supplementalInfo1, esFilterId->supplementalInfo2,
		esCb->cbEsMsgFunc, esCb->cbArg_addr, esSpecificInfo_addr);

	u32 id = cellDmux.GetNewId(es);
	es->id = id;
	esHandle = id;

	cellDmux.Warning("*** New ES(dmux=%d, addr=0x%x, size=0x%x, filter(0x%x, 0x%x, 0x%x, 0x%x), cb=0x%x(arg=0x%x), spec=0x%x): id = %d",
		demuxerHandle, es->memAddr, es->memSize, es->fidMajor, es->fidMinor, es->sup1, es->sup2, (u32)esCb->cbEsMsgFunc, es->cbArg, es->spec, id);

	DemuxerTask task(dmuxEnableEs);
	task.es.es = id;
	task.es.es_ptr = es;

	dmux->job.Push(task);
	return CELL_OK;
}

int cellDmuxDisableEs(u32 esHandle)
{
	cellDmux.Warning("cellDmuxDisableEs(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxDisableEs);
	task.es.es = esHandle;
	task.es.es_ptr = es;

	es->dmux->job.Push(task);
	return CELL_OK;
}

int cellDmuxResetEs(u32 esHandle)
{
	cellDmux.Log("cellDmuxResetEs(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxResetEs);
	task.es.es = esHandle;
	task.es.es_ptr = es;

	es->dmux->job.Push(task);
	return CELL_OK;
}

int cellDmuxGetAu(u32 esHandle, mem32_t auInfo_ptr, mem32_t auSpecificInfo_ptr)
{
	cellDmux.Log("cellDmuxGetAu(esHandle=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfo_ptr.GetAddr(), auSpecificInfo_ptr.GetAddr());

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!auInfo_ptr.IsGood() || !auSpecificInfo_ptr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, true, spec, true))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	auInfo_ptr = info;
	auSpecificInfo_ptr = spec;
	return CELL_OK;
}

int cellDmuxPeekAu(u32 esHandle, mem32_t auInfo_ptr, mem32_t auSpecificInfo_ptr)
{
	cellDmux.Log("cellDmuxPeekAu(esHandle=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfo_ptr.GetAddr(), auSpecificInfo_ptr.GetAddr());

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!auInfo_ptr.IsGood() || !auSpecificInfo_ptr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, true, spec, false))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	auInfo_ptr = info;
	auSpecificInfo_ptr = spec;
	return CELL_OK;
}

int cellDmuxGetAuEx(u32 esHandle, mem32_t auInfoEx_ptr, mem32_t auSpecificInfo_ptr)
{
	cellDmux.Log("cellDmuxGetAuEx(esHandle=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfoEx_ptr.GetAddr(), auSpecificInfo_ptr.GetAddr());

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!auInfoEx_ptr.IsGood() || !auSpecificInfo_ptr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, false, spec, true))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	auInfoEx_ptr = info;
	auSpecificInfo_ptr = spec;
	return CELL_OK;
}

int cellDmuxPeekAuEx(u32 esHandle, mem32_t auInfoEx_ptr, mem32_t auSpecificInfo_ptr)
{
	cellDmux.Log("cellDmuxPeekAuEx(esHandle=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfoEx_ptr.GetAddr(), auSpecificInfo_ptr.GetAddr());

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!auInfoEx_ptr.IsGood() || !auSpecificInfo_ptr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	u32 info;
	u32 spec;
	if (!es->peek(info, false, spec, false))
	{
		return CELL_DMUX_ERROR_EMPTY;
	}

	auInfoEx_ptr = info;
	auSpecificInfo_ptr = spec;
	return CELL_OK;
}

int cellDmuxReleaseAu(u32 esHandle)
{
	cellDmux.Log("cellDmuxReleaseAu(esHandle=0x%x)", esHandle);

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
	cellDmux.Warning("cellDmuxFlushEs(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxFlushEs);
	task.es.es = esHandle;
	task.es.es_ptr = es;

	es->dmux->job.Push(task);
	return CELL_OK;
}

void cellDmux_init()
{
	cellDmux.AddFunc(0xa2d4189b, cellDmuxQueryAttr);
	cellDmux.AddFunc(0x3f76e3cd, cellDmuxQueryAttr2);
	cellDmux.AddFunc(0x68492de9, cellDmuxOpen);
	cellDmux.AddFunc(0xf6c23560, cellDmuxOpenEx);
	cellDmux.AddFunc(0x11bc3a6c, cellDmuxOpen2);
	cellDmux.AddFunc(0x8c692521, cellDmuxClose);
	cellDmux.AddFunc(0x04e7499f, cellDmuxSetStream);
	cellDmux.AddFunc(0x5d345de9, cellDmuxResetStream);
	cellDmux.AddFunc(0xccff1284, cellDmuxResetStreamAndWaitDone);
	cellDmux.AddFunc(0x02170d1a, cellDmuxQueryEsAttr);
	cellDmux.AddFunc(0x52911bcf, cellDmuxQueryEsAttr2);
	cellDmux.AddFunc(0x7b56dc3f, cellDmuxEnableEs);
	cellDmux.AddFunc(0x05371c8d, cellDmuxDisableEs);
	cellDmux.AddFunc(0x21d424f0, cellDmuxResetEs);
	cellDmux.AddFunc(0x42c716b5, cellDmuxGetAu);
	cellDmux.AddFunc(0x2750c5e0, cellDmuxPeekAu);
	cellDmux.AddFunc(0x2c9a5857, cellDmuxGetAuEx);
	cellDmux.AddFunc(0x002e8da2, cellDmuxPeekAuEx);
	cellDmux.AddFunc(0x24ea6474, cellDmuxReleaseAu);
	cellDmux.AddFunc(0xebb3b2bd, cellDmuxFlushEs);
}