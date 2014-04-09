#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/Audio/cellAudio.h"
#include "libmixer.h"

void libmixer_init();
Module libmixer("libmixer", libmixer_init);

CellSurMixerConfig surMixer;

#define SUR_PORT (7)
u32 surMixerCb = 0;
u32 surMixerCbArg = 0;
SMutex mixer_mutex;
float mixdata[8*256];
u64 mixcount = 0;

int cellAANAddData(u32 aan_handle, u32 aan_port, u32 offset, u32 addr, u32 samples)
{
	u32 type = aan_port >> 16;
	u32 port = aan_port & 0xffff;

	switch (type)
	{
	case CELL_SURMIXER_CHSTRIP_TYPE1A:
		if (port >= surMixer.chStrips1) type = 0; break;
	case CELL_SURMIXER_CHSTRIP_TYPE2A:
		if (port >= surMixer.chStrips2) type = 0; break;
	case CELL_SURMIXER_CHSTRIP_TYPE6A:
		if (port >= surMixer.chStrips6) type = 0; break;
	case CELL_SURMIXER_CHSTRIP_TYPE8A:
		if (port >= surMixer.chStrips8) type = 0; break;
	default:
		type = 0; break;
	}

	if (aan_handle == 0x11111111 && samples == 256 && type && offset == 0)
	{
		libmixer.Log("cellAANAddData(handle=0x%x, port=0x%x, offset=0x%x, addr=0x%x, samples=0x%x)",
			aan_handle, aan_port, offset, addr, samples);
	}
	else
	{
		libmixer.Error("cellAANAddData(handle=0x%x, port=0x%x, offset=0x%x, addr=0x%x, samples=0x%x)",
			aan_handle, aan_port, offset, addr, samples);
		Emu.Pause();
		return CELL_OK;
	}

	SMutexLocker lock(mixer_mutex);

	if (type == CELL_SURMIXER_CHSTRIP_TYPE1A)
	{
		// mono upmixing
		for (u32 i = 0; i < samples; i++)
		{
			const float center = *(be_t<float>*)&Memory[addr + i * sizeof(float)];
			mixdata[i * 8 + 0] += center;
			mixdata[i * 8 + 1] += center;
		}		
	}
	else if (type == CELL_SURMIXER_CHSTRIP_TYPE2A)
	{
		// stereo upmixing
		for (u32 i = 0; i < samples; i++)
		{
			const float left = *(be_t<float>*)&Memory[addr + i * 2 * sizeof(float)];
			const float right = *(be_t<float>*)&Memory[addr + (i * 2 + 1) * sizeof(float)];
			mixdata[i * 8 + 0] += left;
			mixdata[i * 8 + 1] += right;
		}
	}
	else if (type == CELL_SURMIXER_CHSTRIP_TYPE6A)
	{
		// 5.1 upmixing
		for (u32 i = 0; i < samples; i++)
		{
			const float left = *(be_t<float>*)&Memory[addr + i * 6 * sizeof(float)];
			const float right = *(be_t<float>*)&Memory[addr + (i * 6 + 1) * sizeof(float)];
			const float center = *(be_t<float>*)&Memory[addr + (i * 6 + 2) * sizeof(float)];
			const float low_freq = *(be_t<float>*)&Memory[addr + (i * 6 + 3) * sizeof(float)];
			const float rear_left = *(be_t<float>*)&Memory[addr + (i * 6 + 4) * sizeof(float)];
			const float rear_right = *(be_t<float>*)&Memory[addr + (i * 6 + 5) * sizeof(float)];
			mixdata[i * 8 + 0] += left;
			mixdata[i * 8 + 1] += right;
			mixdata[i * 8 + 2] += center;
			mixdata[i * 8 + 3] += low_freq;
			mixdata[i * 8 + 4] += rear_left;
			mixdata[i * 8 + 5] += rear_right;
		}
	}
	else if (type == CELL_SURMIXER_CHSTRIP_TYPE8A)
	{
		// 7.1
		for (u32 i = 0; i < samples * 8; i++)
		{
			mixdata[i] += *(be_t<float>*)&Memory[addr + i * sizeof(float)];
		}
	}

	return CELL_OK; 
}

int cellAANConnect(u32 receive, u32 receivePortNo, u32 source, u32 sourcePortNo)
{
	libmixer.Error("cellAANConnect(receive=0x%x, receivePortNo=0x%x, source=0x%x, sourcePortNo=0x%x)",
		receive, receivePortNo, source, sourcePortNo);
	return CELL_OK;
}

int cellAANDisconnect(u32 receive, u32 receivePortNo, u32 source, u32 sourcePortNo)
{
	libmixer.Error("cellAANDisconnect(receive=0x%x, receivePortNo=0x%x, source=0x%x, sourcePortNo=0x%x)",
		receive, receivePortNo, source, sourcePortNo);
	return CELL_OK;
}
 
int cellSSPlayerCreate(mem32_t handle, mem_ptr_t<CellSSPlayerConfig> config)
{
	libmixer.Error("cellSSPlayerCreate(handle_addr=0x%x, config_addr=0x%x)",
		handle.GetAddr(), config.GetAddr());
	return CELL_OK;
}

int cellSSPlayerRemove(u32 handle)
{
	libmixer.Error("cellSSPlayerRemove(handle=%d)", handle);
	return CELL_OK;
}

int cellSSPlayerSetWave(u32 handle, mem_ptr_t<CellSSPlayerWaveParam> waveInfo, mem_ptr_t<CellSSPlayerCommonParam> commonInfo)
{
	libmixer.Error("cellSSPlayerSetWave(handle=%d, waveInfo_addr=0x%x, commonInfo_addr=0x%x)",
		handle, waveInfo.GetAddr(), commonInfo.GetAddr());
	return CELL_OK;
}

int cellSSPlayerPlay(u32 handle, mem_ptr_t<CellSSPlayerRuntimeInfo> info)
{
	libmixer.Error("cellSSPlayerPlay(handle=%d, info_addr=0x%x)", handle, info.GetAddr());
	return CELL_OK;
}

int cellSSPlayerStop(u32 handle, u32 mode)
{
	libmixer.Error("cellSSPlayerStop(handle=%d, mode=0x%x)", handle, mode);
	return CELL_OK;
}

int cellSSPlayerSetParam(u32 handle, mem_ptr_t<CellSSPlayerRuntimeInfo> info)
{
	libmixer.Error("cellSSPlayerSetParam(handle=%d, info_addr=0x%x)", handle, info.GetAddr());
	return CELL_OK;
}
 
s32 cellSSPlayerGetState(u32 handle)
{
	libmixer.Error("cellSSPlayerGetState(handle=%d) -> OFF", handle);
	return CELL_SSPLAYER_STATE_OFF;
}

int cellSurMixerCreate(const mem_ptr_t<CellSurMixerConfig> config)
{
	libmixer.Warning("cellSurMixerCreate(config_addr=0x%x)", config.GetAddr());
	surMixer = *config;
	libmixer.Warning("*** surMixer created (ch1=%d, ch2=%d, ch6=%d, ch8=%d)",
		(u32)surMixer.chStrips1, (u32)surMixer.chStrips2, (u32)surMixer.chStrips6, (u32)surMixer.chStrips8);
	return CELL_OK;
}

int cellSurMixerGetAANHandle(mem32_t handle)
{
	libmixer.Warning("cellSurMixerGetAANHandle(handle_addr=0x%x) -> 0x11111111", handle.GetAddr());
	handle = 0x11111111;
	return CELL_OK;
}

int cellSurMixerChStripGetAANPortNo(mem32_t port, u32 type, u32 index)
{
	libmixer.Warning("cellSurMixerChStripGetAANPortNo(port_addr=0x%x, type=0x%x, index=0x%x) -> 0x%x", port.GetAddr(), type, index, (type << 16) | index);
	port = (type << 16) | index;
	return CELL_OK;
}

int cellSurMixerSetNotifyCallback(u32 func, u32 arg)
{
	libmixer.Warning("cellSurMixerSetNotifyCallback(func_addr=0x%x, arg=0x%x) (surMixerCb=0x%x)", func, arg, surMixerCb);
	surMixerCb = func;
	surMixerCbArg = arg;
	return CELL_OK;
}

int cellSurMixerRemoveNotifyCallback(u32 func)
{
	libmixer.Warning("cellSurMixerSetNotifyCallback(func_addr=0x%x) (surMixerCb=0x%x)", func, surMixerCb);
	surMixerCb = 0;
	surMixerCbArg = 0;
	return CELL_OK;
}

int cellSurMixerStart()
{
	libmixer.Warning("cellSurMixerStart()");

	AudioPortConfig& port = m_config.m_ports[SUR_PORT];

	if (port.m_is_audio_port_opened)
	{
		return CELL_LIBMIXER_ERROR_FULL;
	}
	
	port.channel = 8;
	port.block = 16;
	port.attr = 0;
	port.level = 1.0f;

	libmixer.Warning("*** audio port opened(default)");
			
	port.m_is_audio_port_opened = true;
	port.tag = 0;
	m_config.m_port_in_use++;
	port.m_is_audio_port_started = true;

	thread t("Surmixer Thread", []()
	{
		AudioPortConfig& port = m_config.m_ports[SUR_PORT];

		CPUThread* mixerCb = &Emu.GetCPU().AddThread(CPU_THREAD_PPU);

		mixerCb->SetName("Surmixer Callback");

		mixcount = 0;

		while (port.m_is_audio_port_started)
		{
			if (Emu.IsStopped())
			{
				ConLog.Warning("Surmixer aborted");
				return;
			}

			if (mixcount > (port.tag + 15)) // preemptive buffer filling (probably hack)
			{
				Sleep(1);
				continue;
			}

			u64 stamp0 = get_system_time();

			memset(mixdata, 0, sizeof(mixdata));
			mixerCb->ExecAsCallback(surMixerCb, true, surMixerCbArg, mixcount, 256);

			u64 stamp1 = get_system_time();

			auto buf = (be_t<float>*)&Memory[m_config.m_buffer + (128 * 1024 * SUR_PORT) + (mixcount % port.block) * port.channel * 256 * sizeof(float)];
			
			for (u32 i = 0; i < (sizeof(mixdata) / sizeof(float)); i++)
			{
				// reverse byte order
				buf[i] = mixdata[i];
			}

			u64 stamp2 = get_system_time();

			//ConLog.Write("Libmixer perf: start=%d (cb=%d, finalize=%d)",
				//stamp0 - m_config.start_time, stamp1-stamp0, stamp2-stamp1);

			mixcount++;
		}

		Emu.GetCPU().RemoveThread(mixerCb->GetId());
	});
	t.detach();

	return CELL_OK;
}

int cellSurMixerSetParameter(u32 param, float value)
{
	libmixer.Error("cellSurMixerSetParameter(param=0x%x, value=%f)", param, value);
	return CELL_OK;
}

int cellSurMixerFinalize()
{
	libmixer.Warning("cellSurMixerFinalize()");

	AudioPortConfig& port = m_config.m_ports[SUR_PORT];

	if (port.m_is_audio_port_opened)
	{
		port.m_is_audio_port_started = false;
		port.m_is_audio_port_opened = false;
		m_config.m_port_in_use--;
	}

	return CELL_OK;
}

int cellSurMixerSurBusAddData(u32 busNo, u32 offset, u32 addr, u32 samples)
{
	if (busNo < 8 && samples == 256 && offset == 0)
	{
		libmixer.Log("cellSurMixerSurBusAddData(busNo=%d, offset=0x%x, addr=0x%x, samples=%d)", busNo, offset, addr, samples);
	}
	else
	{
		libmixer.Error("cellSurMixerSurBusAddData(busNo=%d, offset=0x%x, addr=0x%x, samples=%d)", busNo, offset, addr, samples);
		Emu.Pause();
		return CELL_OK;
	}

	SMutexLocker lock(mixer_mutex);

	for (u32 i = 0; i < samples; i++)
	{
		// reverse byte order and mix
		u32 v = Memory.Read32(addr + i * sizeof(float));
		mixdata[i*8+busNo] += (float&)v;
	}

	return CELL_OK;
}

int cellSurMixerChStripSetParameter(u32 type, u32 index, mem_ptr_t<CellSurMixerChStripParam> param)
{
	libmixer.Error("cellSurMixerChStripSetParameter(type=%d, index=%d, param_addr=0x%x)", type, index, param.GetAddr());
	return CELL_OK;
}

int cellSurMixerPause(u32 type)
{
	libmixer.Error("cellSurMixerPause(type=%d)", type);
	return CELL_OK;
}

int cellSurMixerGetCurrentBlockTag(mem64_t tag)
{
	libmixer.Error("cellSurMixerGetCurrentBlockTag(tag_addr=0x%x)", tag.GetAddr());
	return CELL_OK;
}

int cellSurMixerGetTimestamp(u64 tag, mem64_t stamp)
{
	libmixer.Error("cellSurMixerGetTimestamp(tag=0x%llx, stamp_addr=0x%x)", tag, stamp.GetAddr());
	return CELL_OK;
}

void cellSurMixerBeep(u32 arg)
{
	libmixer.Error("cellSurMixerBeep(arg=%d)", arg);
	return;
}

void cellSurMixerUtilGetLevelFromDB(float dB)
{
	// not hooked, probably unnecessary
	libmixer.Error("cellSurMixerUtilGetLevelFromDB(dB=%f)", dB);
	declCPU();
	(float&)CPU.FPR[0] = 0.0f;
}

void cellSurMixerUtilGetLevelFromDBIndex(int index)
{
	// not hooked, probably unnecessary
	libmixer.Error("cellSurMixerUtilGetLevelFromDBIndex(index=%d)", index);
	declCPU();
	(float&)CPU.FPR[0] = 0.0f;
}

void cellSurMixerUtilNoteToRatio(u8 refNote, u8 note)
{
	// not hooked, probably unnecessary
	libmixer.Error("cellSurMixerUtilNoteToRatio(refNote=%d, note=%d)", refNote, note);
	declCPU();
	(float&)CPU.FPR[0] = 0.0f;
}

void libmixer_init()
{
	REG_SUB(libmixer, "surmxAAN", cellAANAddData,
		0xffffffff7c691b78,
		0xffffffff7c0802a6,
		0xfffffffff821ff91,
		0xfffffffff8010080,
		0xffffffff7c802378,
		0xffffffff7caa2b78,
		0xffffffff81690000,
		0xffffffff7c050378,
		0xffffffff7cc43378,
		0x78630020, // clrldi r3,r3,32
		0xffffffff7d465378,
		0xffffffff812b0030,
		0xffffffff80090000,
		0xfffffffff8410028,
		0xffffffff7c0903a6,
		0xffffffff80490004,
		0xffffffff4e800421,
		0xffffffffe8410028,
		0xffffffffe8010080,
		0xffffffff7c6307b4,
		0xffffffff7c0803a6,
		0xffffffff38210070,
		0xffffffff4e800020
	);

	REG_SUB(libmixer, "surmxAAN", cellAANConnect,
		0xfffffffff821ff71,
		0xffffffff7c0802a6,
		0xffffffff2f830000,
		0xfffffffff80100a0,
		0xffffffff3c008031,
		0xffffffff7c691b78,
		0xffffffff7c8a2378,
		0xffffffff60000003,
		0xffffff00409e0018, // bne
		0xffffffff7c0307b4,
		0xffffffffe80100a0,
		0xffffffff38210090,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0xffffffff2f850000,
		0xffffffff78630020,
		0xffffffff38810070,
		0xffffff00419effe0, // beq
		0xffffffff81690000,
		0xffffffff38000001,
		0xffffffff91210074,
		0xffffffff90a10070,
		0xffffffff90c10078,
		0xffffffff9141007c,
		0xffffffff812b0018, // difference
		0xffffffff90010080,
		0xffffffff80090000,
		0xfffffffff8410028,
		0xffffffff7c0903a6,
		0xffffffff80490004,
		0xffffffff4e800421,
		0xffffffffe8410028,
		0xffffffff7c601b78,
		0xffffffff7c0307b4,
		0xffffffffe80100a0,
		0xffffffff38210090,
		0xffffffff7c0803a6,
		0xffffffff4e800020
	);

	REG_SUB(libmixer, "surmxAAN", cellAANDisconnect,
		0xfffffffff821ff71,
		0xffffffff7c0802a6,
		0xffffffff2f830000,
		0xfffffffff80100a0,
		0xffffffff3c008031,
		0xffffffff7c691b78,
		0xffffffff7c8a2378,
		0xffffffff60000003,
		0xffffff00409e0018, // bne
		0xffffffff7c0307b4,
		0xffffffffe80100a0,
		0xffffffff38210090,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0xffffffff2f850000,
		0xffffffff78630020,
		0xffffffff38810070,
		0xffffff00419effe0, // beq
		0xffffffff81690000,
		0xffffffff38000001,
		0xffffffff91210074,
		0xffffffff90a10070,
		0xffffffff90c10078,
		0xffffffff9141007c,
		0xffffffff812b001c, // difference
		0xffffffff90010080,
		0xffffffff80090000,
		0xfffffffff8410028,
		0xffffffff7c0903a6,
		0xffffffff80490004,
		0xffffffff4e800421,
		0xffffffffe8410028,
		0xffffffff7c601b78,
		0xffffffff7c0307b4,
		0xffffffffe80100a0,
		0xffffffff38210090,
		0xffffffff7c0803a6,
		0xffffffff4e800020
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerCreate,
		0xffffffff2f830000,
		0xffffffff7c0802a6,
		0xfffffffff821ff51,
		0xfffffffffbc100a0,
		0xfffffffffb210078,
		0xfffffffffb410080,
		0xfffffffffb610088,
		0xfffffffffb810090,
		0xfffffffffba10098,
		0xfffffffffbe100a8,
		0xfffffffff80100c0,
		0xffffffff7c7e1b78,
		0xf000000040000000, // bne
		0xffffffff3fe08031,
		0xffffffff63ff0003,
		0xffffffffe80100c0,
		0xffffffff7fe307b4,
		0xffffffffeb210078,
		0xffffffffeb410080,
		0xffffffff7c0803a6,
		0xffffffffeb610088,
		0xffffffffeb810090,
		0xffffffffeba10098,
		0xffffffffebc100a0,
		0xffffffffebe100a8,
		0xffffffff382100b0
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerGetAANHandle,
		0xff00000081428250, // lwz
		0xffffffff3d607fce,
		0xffffffff616bfffe,
		0xffffffff812a0018,
		0xffffffff7d2afe70,
		0xffffffff91230000,
		0xffffffff7d404a78,
		0xffffffff7c005050,
		0xffffffff7c00fe70,
		0xffffffff7c035838,
		0xffffffff3c638031,
		0xffffffff38630002,
		0xffffffff7c6307b4,
		0xffffffff4e800020
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerChStripGetAANPortNo,
		0xff00000081228250, // lwz
		0xffffffff7c661b78,
		0xffffffff3c608031,
		0xffffffff78c60020,
		0xffffffff78840020,
		0xffffffff60630002,
		0xffffffff80090018,
		0xffffffff78a50020,
		0xffffffff2f800000,
		0xffffffff4d9e0020,
		0xffffffff78030020,
		0xf000000040000000 // b
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerSetNotifyCallback,
		0xff00000081428250, // lwz
		0xffffffff7c0802a6,
		0xfffffffff821ff81,
		0xfffffffff8010090,
		0xffffffff7c6b1b78,
		0xffffffff3c608031,
		0xffffffff812a0018,
		0xffffffff7c882378,
		0xffffffff60630003,
		0xffffffff2f890000,
		0xffffffff2f0b0000,
		0xffffff00409e0020, // bne
		0xffffffff3c608031,
		0xffffffff60630002,
		0xffffffffe8010090,
		0xffffffff7c6307b4,
		0xffffffff38210080,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0xffffff00419affec, // beq
		0xf0000000800a001c, // lwz
		0xffffffff79290020,
		0xffffffff38810070,
		0xffffffff2f800000,
		0xffffffff7d234b78
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerRemoveNotifyCallback,
		0xff00000081628250, // lwz
		0xffffffff7c0802a6,
		0xfffffffff821ff81,
		0xfffffffff8010090,
		0xffffffff7c6a1b78,
		0xffffffff3d208031,
		0xffffffff806b0018,
		0xffffffff61290002, // ori
		0xffffffff2f830000,
		0xffff0000409e0018, // bne
		0xffffffffe8010090,
		0xffffffff7d2307b4,
		0xffffffff38210080,
		0xffffffff7c0803a6,
		0xffffffff4e800020
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerStart,
		0xfffffffff821ff71,
		0xffffffff7c0802a6,
		0xfffffffffbc10080,
		0xf000000083c20000, // lwz
		0xfffffffff80100a0,
		0xfffffffffba10078,
		0xfffffffffbe10088,
		0xffffffff801e0018,
		0xffffffff2f800000,
		0xf0000000409e002c, // bne
		0xffffffff3fe08031,
		0xffffffff63ff0002,
		0xffffffffe80100a0,
		0xffffffff7fe307b4,
		0xffffffffeba10078,
		0xffffffffebc10080,
		0xffffffff7c0803a6,
		0xffffffffebe10088,
		0xffffffff38210090,
		0xffffffff4e800020
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerSetParameter,
		0xfffffffff821ff81,
		0xffffffff7c0802a6,
		0xfffffffffbc10070,
		0xfffffffffc000890,
		0xf000000083c28250, // lwz
		0xffffffff3d208031,
		0xfffffffff8010090,
		0xfffffffffbe10078,
		0xffffffff61290002,
		0xffffffff7c7f1b78,
		0xffffffff801e0018,
		0xffffffff2f800000,
		0xffff0000409e0020, // bne
		0xffffffffe8010090,
		0xffffffff7d2307b4,
		0xffffffffebc10070,
		0xffffffffebe10078,
		0xffffffff7c0803a6,
		0xffffffff38210080,
		0xffffffff4e800020,
		0xffffffff801e001c,
		0xffffffff2b03001f,
		0xffffffff2f800000,
		0xffff0000419cffd8, // blt
		0xffffffff2b83002b,
		0xffff000040990008, // ble
		0xffff0000409d0054 // ble
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerFinalize,
		0xfffffffff821ff91,
		0xffffffff7c0802a6,
		0xfffffffff8010080,
		0xffffff004bfffde1, // bl
		0xffffffffe8010080,
		0xffffffff38600000,
		0xffffffff38210070,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0xfffffffff821ff71,
		0xffffffff7c0802a6,
		0xfffffffffba10078,
		0xf000000083a28250, // lwz
		0xfffffffff80100a0,
		0xffffffff817d0018,
		0xffffffff7d635b78,
		0xffffffff812b0000,
		0xffffffff81490000,
		0xffffffff800a0000,
		0xfffffffff8410028,
		0xffffffff7c0903a6,
		0xffffffff804a0004,
		0xffffffff4e800421
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerSurBusAddData,
		0xff00000081428250, // lwz
		0xffffffff7c0802a6,
		0xfffffffff821ff91,
		0xfffffffff8010080,
		0xffffffff7c601b78,
		0xffffffff3d208031,
		0xffffffff806a0018,
		0xffffffff7c8b2378,
		0xffffffff7cc73378,
		0xffffffff2f830000,
		0xffffffff61290002,
		0xffff0000409e0018, // bne
		0xffffffffe8010080,
		0xffffffff7d2307b4,
		0xffffffff38210070,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0xffffffff78a40020,
		0xffffffff78050020,
		0xffffffff800a001c,
		0xffffffff78680020,
		0xffffffff2f800000,
		0xffffffff7d034378,
		0xffffffff79660020,
		0xffffffff78e70020,
		0xffff0000419cffcc // blt
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerChStripSetParameter,
		0xff00000081028250, // lwz
		0xffffffff7c6b1b78,
		0xffffffff3c608031,
		0xffffffff7c8a2378,
		0xffffffff7ca62b78,
		0xffffffff60630002,
		0xffffffff81280018,
		0xffffffff2f890000,
		0xffff00004d9e0020, // beqlr
		0xffffffff8008001c,
		0xffffffff79640020,
		0xffffffff79450020,
		0xffffffff2f800000,
		0xffffffff78c60020,
		0xffffffff4d9c0020,
		0xffffffff79230020,
		0xf000000048000000 // b
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerPause,
		0xff00000081428250, // lwz
		0xffffffff7c0802a6,
		0xfffffffff821ff81,
		0xfffffffff8010090,
		0xffffffff3d208031,
		0xfffffffffbe10078,
		0xffffffff800a0018,
		0xffffffff7c7f1b78,
		0xfffffffffbc10070,
		0xffffffff2f800000,
		0xffffffff61290002,
		0xffff0000409e0020, // bne
		0xffffffffe8010090,
		0xffffffff7d2307b4,
		0xffffffffebc10070,
		0xffffffffebe10078,
		0xffffffff7c0803a6,
		0xffffffff38210080,
		0xffffffff4e800020,
		0xffffffff800a001c,
		0xffffffff2b030002,
		0xffffffff2f800000
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerGetCurrentBlockTag,
		0xff00000081628250, // lwz
		0xffffffff3d208031,
		0xffffffff61290002,
		0xffffffff880b0020,
		0xffffffff2f800000,
		0xffff0000419e0010, // beq
		0xffffffffe80b0028,
		0xffffffff39200000,
		0xfffffffff8030000,
		0xffffffff7d2307b4,
		0xffffffff4e800020
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerGetTimestamp,
		0xff00000081628250, // lwz
		0xffffffff7c0802a6,
		0xfffffffff821ff91,
		0xfffffffff8010080,
		0xffffffff7c852378,
		0xffffffff3d208031,
		0xffffffff880b0020,
		0xffffffff7c641b78,
		0xffffffff78a50020,
		0xffffffff2f800000,
		0xffffffff61290002,
		0xffff000040de0018, // bne-
		0xffffffffe8010080,
		0xffffffff7d2307b4,
		0xffffffff38210070,
		0xffffffff7c0803a6,
		0xffffffff4e800020,
		0xffffffff806b04d8,
		0xf000000048000001 // bl
	);

	REG_SUB(libmixer, "surmixer", cellSurMixerBeep,
		0xff00000081228250, // lwz
		0xffffffff7c641b78,
		0xffffffff80690018,
		0xffffffff2f830000,
		0xffff00004d9e0020, // beqlr
		0xffffffff8009001c,
		0xffffffff78630020,
		0xffffffff78840020,
		0xffffffff2f800000,
		0xffffffff4d9c0020,
		0xf000000048000000 // b
	);

	REG_SUB(libmixer, "surmxSSP", cellSSPlayerCreate,
		0xfffffffff821ff51,
		0xffffffff7c0802a6,
		0xffffffff2f840000,
		0xfffffffff80100c0,
		0xffffffff3c008031,
		0xfffffffffb210078,
		0xfffffffffb410080,
		0xfffffffffb610088,
		0xfffffffffb810090,
		0xfffffffffba10098,
		0xfffffffffbc100a0,
		0xfffffffffbe100a8,
		0xffffffff7c9a2378,
		0xffffffff7c791b78,
		0xffffffff60000003,
		0xffff0000419e0068, // beq
		0xff00000083620000, // lwz
		0xffffffff3b800000,
		0xffffffff381b0064,
		0xffffffff901b0018,
		0xffffffff5780103a,
		0xffffffff38800010,
		0xffffffff7c0007b4,
		0xffffffff38a01c70,
		0xffffffff7fc0da14,
		0xffffffff38c00000,
		0xffffffff83be0024,
		0xffffffff2f9d0000,
		0xffffffff7ba30020,
		0xffff000041de00c0, // beq-
		0xf000000048000001 // bl
	);

	REG_SUB(libmixer, "surmxSSP", cellSSPlayerRemove,
		0xffffffff7c641b78,
		0xffffffff7c0802a6,
		0xffffffff3c608031,
		0xffffffff2f840000,
		0xfffffffff821ff51,
		0xfffffffffb010070,
		0xfffffffffb210078,
		0xfffffffffb410080,
		0xfffffffffb610088,
		0xfffffffffb810090,
		0xfffffffffba10098,
		0xfffffffffbc100a0,
		0xfffffffffbe100a8,
		0xfffffffff80100c0,
		0xffffffff60630003,
		0xffff0000419e0074, // beq
		0xffffffff81240000,
		0xffffffff78830020,
		0xffffffff83440004,
		0xffffffff83240008,
		0xffffffff7b5b0020,
		0xffffffff81690000,
		0xffffffff800b0000,
		0xfffffffff8410028,
		0xffffffff7c0903a6,
		0xffffffff804b0004,
		0xffffffff4e800421
	);

	REG_SUB(libmixer, "surmxSSP", cellSSPlayerSetWave,
		0xffffffff7c601b78,
		0xffffffff78840020,
		0xffffffff2f800000,
		0xffffffff3c608031,
		0xffffffff78a50020,
		0xffff0000419e000c, // beq
		0xffffffff78030020,
		0xf000000048000000, // b
		0xffffffff60630003,
		0xffffffff4e800020
	);

	REG_SUB(libmixer, "surmxSSP", cellSSPlayerPlay,
		0xffffffff7c601b78,
		0xffffffff3c608031,
		0xffffffff2f800000,
		0xffffffff60630003,
		0xffffffff78840020,
		0xffffffff4d9e0020,
		0xffffffff78030020,
		0xf000000048000000, // b
		0xfffffffff821ff81, // next func
		0xffffffff7c0802a6,
		0xfffffffffbe10078,
		0xffffffff7c7f1b78,
		0xff00000081620028, // lwz
		0xfffffffff8010090,
		0xffffffff39400000,
		0xffffffff38630010
	);

	REG_SUB(libmixer, "surmxSSP", cellSSPlayerStop,
		0xfffffffff821ff91,
		0xffffffff7c0802a6,
		0xffffffff2f830000,
		0xfffffffff8010080,
		0xffffffff3c008031,
		0xffffffff78630020,
		0xffffffff60000003,
		0xffff0000419e0010, // beq
		0xffffffff78840020,
		0xf000000048000001, // bl
		0xffffffff38000000,
		0xffffffff7c0307b4,
		0xffffffffe8010080,
		0xffffffff38210070,
		0xffffffff7c0803a6,
		0xffffffff4e800020
	);

	REG_SUB(libmixer, "surmxSSP", cellSSPlayerSetParam,
		0xffffffff7c601b78,
		0xffffffff3c608031,
		0xffffffff2f800000,
		0xffffffff60630003,
		0xffffffff78840020,
		0xffffffff4d9e0020,
		0xffffffff78030020,
		0xf000000048000000, // b
		0xfffffffff821ff71, // next func
		0xffffffff7c0802a6,
		0xffffffff3d608031,
		0xfffffffff80100a0,
		0xffffffff80030068,
		0xffffffff616b0002,
		0xfffffffffbc10080,
		0xffffffff2f800000
	);

	REG_SUB(libmixer, "surmxSSP", cellSSPlayerGetState,
		0xffffffff7c601b78,
		0xffffffff3c608031,
		0xffffffff2f800000,
		0xffffffff60630003,
		0xffffffff4d9e0020,
		0xffffffff78030020,
		0xf000000048000000 // b
	);

	REG_SUB_EMPTY(libmixer, "surmxUti", cellSurMixerUtilGetLevelFromDB);
	REG_SUB_EMPTY(libmixer, "surmxUti", cellSurMixerUtilGetLevelFromDBIndex);
	REG_SUB_EMPTY(libmixer, "surmxUti", cellSurMixerUtilNoteToRatio);
}