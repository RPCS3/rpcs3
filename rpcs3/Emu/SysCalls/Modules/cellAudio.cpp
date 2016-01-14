#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/SysCalls/Callback.h"

#include "Emu/SysCalls/lv2/sys_sync.h"
#include "Emu/SysCalls/lv2/sys_event.h"
#include "Emu/Event.h"
#include "Emu/Audio/AudioManager.h"
#include "Emu/Audio/AudioDumper.h"

#include "cellAudio.h"

extern Module<> cellAudio;

extern u64 get_system_time();

AudioConfig g_audio;

std::shared_ptr<thread_ctrl> g_audio_thread;

s32 cellAudioInit()
{
	cellAudio.warning("cellAudioInit()");

	if (!g_audio.state.compare_and_swap_test(AUDIO_STATE_NOT_INITIALIZED, AUDIO_STATE_INITIALIZED))
	{
		return CELL_AUDIO_ERROR_ALREADY_INIT;
	}

	// clear ports
	for (auto& port : g_audio.ports)
	{
		port.state = AUDIO_PORT_STATE_CLOSED;
	}

	// reset variables
	g_audio.counter = 0;
	g_audio.keys.clear();
	g_audio.start_time = get_system_time();

	// alloc memory (only once until the emulator is stopped)
	g_audio.buffer = g_audio.buffer ? g_audio.buffer : vm::alloc(AUDIO_PORT_OFFSET * AUDIO_PORT_COUNT, vm::main);
	g_audio.indexes = g_audio.indexes ? g_audio.indexes : vm::alloc(SIZE_32(u64) * AUDIO_PORT_COUNT, vm::main);

	// clear memory
	std::memset(vm::base(g_audio.buffer), 0, AUDIO_PORT_OFFSET * AUDIO_PORT_COUNT);
	std::memset(vm::base(g_audio.indexes), 0, SIZE_32(u64) * AUDIO_PORT_COUNT);

	// start audio thread
	g_audio_thread = thread_ctrl::spawn(PURE_EXPR("Audio Thread"s), []()
	{
		const bool do_dump = rpcs3::config.audio.dump_to_file.value();

		AudioDumper m_dump;
		if (do_dump && !m_dump.Init(2)) // Init AudioDumper for 2 channels
		{
			throw EXCEPTION("AudioDumper::Init() failed");
		}

		float buf2ch[2 * BUFFER_SIZE]; // intermediate buffer for 2 channels
		float buf8ch[8 * BUFFER_SIZE]; // intermediate buffer for 8 channels

		static const size_t out_buffer_size = 8 * BUFFER_SIZE; // output buffer for 8 channels

		std::unique_ptr<float[]> out_buffer[BUFFER_NUM];

		for (u32 i = 0; i < BUFFER_NUM; i++)
		{
			out_buffer[i].reset(new float[out_buffer_size] {});
		}

		squeue_t<float*, BUFFER_NUM - 1> out_queue;

		scope_thread_t iat(PURE_EXPR("Internal Audio Thread"s), [&out_queue]()
		{
			const bool use_u16 = rpcs3::config.audio.convert_to_u16.value();

			Emu.GetAudioManager().GetAudioOut().Init();

			bool opened = false;
			float* buffer;

			while (out_queue.pop(buffer, [](){ return g_audio.state != AUDIO_STATE_INITIALIZED; }))
			{
				if (use_u16)
				{
					// convert the data from float to u16 with clipping:
					// 2x MULPS
					// 2x MAXPS (optional)
					// 2x MINPS (optional)
					// 2x CVTPS2DQ (converts float to s32)
					// PACKSSDW (converts s32 to s16 with signed saturation)

					u16 buf_u16[out_buffer_size];
					for (size_t i = 0; i < out_buffer_size; i += 8)
					{
						const auto scale = _mm_set1_ps(0x8000);
						(__m128i&)(buf_u16[i]) = _mm_packs_epi32(
							_mm_cvtps_epi32(_mm_mul_ps(_mm_load_ps(buffer + i), scale)),
							_mm_cvtps_epi32(_mm_mul_ps(_mm_load_ps(buffer + i + 4), scale)));
					}

					if (!opened)
					{
						Emu.GetAudioManager().GetAudioOut().Open(buf_u16, out_buffer_size * sizeof(u16));
						opened = true;
					}
					else
					{
						Emu.GetAudioManager().GetAudioOut().AddData(buf_u16, out_buffer_size * sizeof(u16));
					}
				}
				else
				{
					if (!opened)
					{
						Emu.GetAudioManager().GetAudioOut().Open(buffer, out_buffer_size * sizeof(float));
						opened = true;
					}
					else
					{
						Emu.GetAudioManager().GetAudioOut().AddData(buffer, out_buffer_size * sizeof(float));
					}
				}
			}

			Emu.GetAudioManager().GetAudioOut().Quit();
		});

		while (g_audio.state == AUDIO_STATE_INITIALIZED && !Emu.IsStopped())
		{
			if (Emu.IsPaused())
			{
				std::this_thread::sleep_for(1ms); // hack
				continue;
			}

			const u64 stamp0 = get_system_time();

			const u64 time_pos = stamp0 - g_audio.start_time - Emu.GetPauseTime();

			// TODO: send beforemix event (in ~2,6 ms before mixing)

			// precise time of sleeping: 5,(3) ms (or 256/48000 sec)
			const u64 expected_time = g_audio.counter * AUDIO_SAMPLES * 1000000 / 48000;
			if (expected_time >= time_pos)
			{
				std::this_thread::sleep_for(1ms); // hack
				continue;
			}
			
			//// crutch to hide giant lags caused by debugger
			//const u64 missed_time = time_pos - expected_time;
			//if (missed_time > AUDIO_SAMPLES * MHZ / 48000)
			//{
			//	cellAudio.notice("%f ms adjusted", (float)missed_time / 1000);
			//	g_audio.start_time += missed_time;
			//}

			g_audio.counter++;

			const u32 out_pos = g_audio.counter % BUFFER_NUM;

			bool first_mix = true;

			// mixing:
			for (auto& port : g_audio.ports)
			{
				if (port.state != AUDIO_PORT_STATE_STARTED) continue;

				const u32 block_size = port.channel * AUDIO_SAMPLES;
				const u32 position = port.tag % port.block; // old value
				const u32 buf_addr = port.addr + position * block_size * sizeof(float);

				auto buf = vm::_ptr<f32>(buf_addr);

				static const float k = 1.0f; // may be 1.0f
				const float& m = port.level;

				auto step_volume = [](AudioPortConfig& port) // part of cellAudioSetPortLevel functionality
				{
					const auto param = port.level_set.load();

					if (param.inc != 0.0f)
					{
						port.level += param.inc;
						const bool dec = param.inc < 0.0f;

						if ((!dec && param.value - port.level <= 0.0f) || (dec && param.value - port.level >= 0.0f))
						{
							port.level = param.value;
							port.level_set.compare_and_swap(param, { param.value, 0.0f });
						}
					}
				};

				if (port.channel == 2)
				{
					if (first_mix)
					{
						for (u32 i = 0; i < (sizeof(buf2ch) / sizeof(float)); i += 2)
						{
							step_volume(port);

							// reverse byte order
							const float left = buf[i + 0] * m;
							const float right = buf[i + 1] * m;

							buf2ch[i + 0] = left;
							buf2ch[i + 1] = right;

							buf8ch[i * 4 + 0] = left;
							buf8ch[i * 4 + 1] = right;
							buf8ch[i * 4 + 2] = 0.0f;
							buf8ch[i * 4 + 3] = 0.0f;
							buf8ch[i * 4 + 4] = 0.0f;
							buf8ch[i * 4 + 5] = 0.0f;
							buf8ch[i * 4 + 6] = 0.0f;
							buf8ch[i * 4 + 7] = 0.0f;
						}
						first_mix = false;
					}
					else
					{
						for (u32 i = 0; i < (sizeof(buf2ch) / sizeof(float)); i += 2)
						{
							step_volume(port);

							const float left = buf[i + 0] * m;
							const float right = buf[i + 1] * m;

							buf2ch[i + 0] += left;
							buf2ch[i + 1] += right;

							buf8ch[i * 4 + 0] += left;
							buf8ch[i * 4 + 1] += right;
						}
					}
				}
				else if (port.channel == 8)
				{
					if (first_mix)
					{
						for (u32 i = 0; i < (sizeof(buf2ch) / sizeof(float)); i += 2)
						{
							step_volume(port);

							const float left = buf[i * 4 + 0] * m;
							const float right = buf[i * 4 + 1] * m;
							const float center = buf[i * 4 + 2] * m;
							const float low_freq = buf[i * 4 + 3] * m;
							const float rear_left = buf[i * 4 + 4] * m;
							const float rear_right = buf[i * 4 + 5] * m;
							const float side_left = buf[i * 4 + 6] * m;
							const float side_right = buf[i * 4 + 7] * m;

							const float mid = (center + low_freq) * 0.708f;
							buf2ch[i + 0] = (left + rear_left + side_left + mid) * k;
							buf2ch[i + 1] = (right + rear_right + side_right + mid) * k;

							buf8ch[i * 4 + 0] = left;
							buf8ch[i * 4 + 1] = right;
							buf8ch[i * 4 + 2] = center;
							buf8ch[i * 4 + 3] = low_freq;
							buf8ch[i * 4 + 4] = rear_left;
							buf8ch[i * 4 + 5] = rear_right;
							buf8ch[i * 4 + 6] = side_left;
							buf8ch[i * 4 + 7] = side_right;
						}
						first_mix = false;
					}
					else
					{
						for (u32 i = 0; i < (sizeof(buf2ch) / sizeof(float)); i += 2)
						{
							step_volume(port);

							const float left = buf[i * 4 + 0] * m;
							const float right = buf[i * 4 + 1] * m;
							const float center = buf[i * 4 + 2] * m;
							const float low_freq = buf[i * 4 + 3] * m;
							const float rear_left = buf[i * 4 + 4] * m;
							const float rear_right = buf[i * 4 + 5] * m;
							const float side_left = buf[i * 4 + 6] * m;
							const float side_right = buf[i * 4 + 7] * m;

							const float mid = (center + low_freq) * 0.708f;
							buf2ch[i + 0] += (left + rear_left + side_left + mid) * k;
							buf2ch[i + 1] += (right + rear_right + side_right + mid) * k;

							buf8ch[i * 4 + 0] += left;
							buf8ch[i * 4 + 1] += right;
							buf8ch[i * 4 + 2] += center;
							buf8ch[i * 4 + 3] += low_freq;
							buf8ch[i * 4 + 4] += rear_left;
							buf8ch[i * 4 + 5] += rear_right;
							buf8ch[i * 4 + 6] += side_left;
							buf8ch[i * 4 + 7] += side_right;
						}
					}
				}
				else
				{
					throw EXCEPTION("Unknown channel count (port=%lld, channel=%d)", &port - g_audio.ports, port.channel);
				}

				memset(buf, 0, block_size * sizeof(float));
			}


			if (!first_mix)
			{
				// copy output data (2 ch)
				//for (u32 i = 0; i < (sizeof(buf2ch) / sizeof(float)); i++)
				//{
				//	out_buffer[out_pos][i] = buf2ch[i];
				//}

				// copy output data (8 ch)
				for (u32 i = 0; i < (sizeof(buf8ch) / sizeof(float)); i++)
				{
					out_buffer[out_pos][i] = buf8ch[i];
				}
			}

			//const u64 stamp1 = get_system_time();

			if (first_mix)
			{
				memset(out_buffer[out_pos].get(), 0, out_buffer_size * sizeof(float));
			}

			if (!out_queue.push(out_buffer[out_pos].get(), [](){ return g_audio.state != AUDIO_STATE_INITIALIZED; }))
			{
				break;
			}

			//const u64 stamp2 = get_system_time();

			{
				// update indices:

				auto indexes = vm::ptr<u64>::make(g_audio.indexes);

				for (u32 i = 0; i < AUDIO_PORT_COUNT; i++)
				{
					AudioPortConfig& port = g_audio.ports[i];

					if (port.state != AUDIO_PORT_STATE_STARTED) continue;

					u32 position = port.tag % port.block; // old value
					port.counter = g_audio.counter;
					port.tag++; // absolute index of block that will be read
					indexes[i] = (position + 1) % port.block; // write new value
				}

				// send aftermix event (normal audio event)

				LV2_LOCK;

				std::lock_guard<std::mutex> lock(g_audio.mutex);

				for (auto key : g_audio.keys)
				{
					if (const auto queue = Emu.GetEventManager().GetEventQueue(key))
					{
						queue->push(lv2_lock, 0, 0, 0, 0); // TODO: check arguments
					}
				}
			}
			

			//const u64 stamp3 = get_system_time();

			if (do_dump && !first_mix)
			{
				if (m_dump.GetCh() == 8)
				{
					if (m_dump.WriteData(&buf8ch, sizeof(buf8ch)) != sizeof(buf8ch)) // write file data (8 ch)
					{
						throw EXCEPTION("AudioDumper::WriteData() failed (8 ch)");
					}
				}
				else if (m_dump.GetCh() == 2)
				{
					if (m_dump.WriteData(&buf2ch, sizeof(buf2ch)) != sizeof(buf2ch)) // write file data (2 ch)
					{
						throw EXCEPTION("AudioDumper::WriteData() failed (2 ch)");
					}
				}
				else
				{
					throw EXCEPTION("AudioDumper::GetCh() returned unknown value (%d)", m_dump.GetCh());
				}
			}

			//LOG_NOTICE(HLE, "Audio perf: start=%d (access=%d, AddData=%d, events=%d, dump=%d)",
			//time_pos, stamp1 - stamp0, stamp2 - stamp1, stamp3 - stamp2, get_system_time() - stamp3);
		}
	});

	return CELL_OK;
}

s32 cellAudioQuit()
{
	cellAudio.warning("cellAudioQuit()");

	if (!g_audio.state.compare_and_swap_test(AUDIO_STATE_INITIALIZED, AUDIO_STATE_FINALIZED))
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	g_audio_thread->join();
	g_audio_thread.reset();
	g_audio.state.exchange(AUDIO_STATE_NOT_INITIALIZED);

	return CELL_OK;
}

s32 cellAudioPortOpen(vm::ptr<CellAudioPortParam> audioParam, vm::ptr<u32> portNum)
{
	cellAudio.warning("cellAudioPortOpen(audioParam=*0x%x, portNum=*0x%x)", audioParam, portNum);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (!audioParam || !portNum)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const u64 channel = audioParam->nChannel;
	const u64 block = audioParam->nBlock;
	const u64 attr = audioParam->attr;

	// check attributes
	if (channel != CELL_AUDIO_PORT_2CH &&
		channel != CELL_AUDIO_PORT_8CH &&
		channel)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (block != CELL_AUDIO_BLOCK_8 &&
		block != CELL_AUDIO_BLOCK_16 &&
		block != 2 &&
		block != 4 &&
		block != 32)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	// list unsupported flags
	if (attr & CELL_AUDIO_PORTATTR_BGM)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_BGM");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_SECONDARY)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_SECONDARY");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_PERSONAL_0)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_PERSONAL_0");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_PERSONAL_1)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_PERSONAL_1");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_PERSONAL_2)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_PERSONAL_2");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_PERSONAL_3)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_PERSONAL_3");
	}
	if (attr & CELL_AUDIO_PORTATTR_OUT_NO_ROUTE)
	{
		cellAudio.todo("cellAudioPortOpen(): CELL_AUDIO_PORTATTR_OUT_NO_ROUTE");
	}
	if (attr & 0xFFFFFFFFF0EFEFEEULL)
	{
		cellAudio.todo("cellAudioPortOpen(): unknown attributes (0x%llx)", attr);
	}

	// open audio port
	const u32 port_index = g_audio.open_port();

	if (!~port_index)
	{
		return CELL_AUDIO_ERROR_PORT_FULL;
	}

	AudioPortConfig& port = g_audio.ports[port_index];

	port.channel = (u32)channel;
	port.block = (u32)block;
	port.attr = attr;
	port.addr = g_audio.buffer + AUDIO_PORT_OFFSET * port_index;
	port.read_index_addr = g_audio.indexes + sizeof(u64) * port_index;
	port.size = port.channel * port.block * AUDIO_SAMPLES * sizeof(float);
	port.tag = 0;

	if (port.attr & CELL_AUDIO_PORTATTR_INITLEVEL)
	{
		port.level = audioParam->level;
	}
	else
	{
		port.level = 1.0f;
	}

	port.level_set.store({ port.level, 0.0f });

	*portNum = port_index;
	cellAudio.warning("*** audio port opened(nChannel=%d, nBlock=%d, attr=0x%llx, level=%f): port = %d", channel, block, attr, port.level, port_index);

	return CELL_OK;
}

s32 cellAudioGetPortConfig(u32 portNum, vm::ptr<CellAudioPortConfig> portConfig)
{
	cellAudio.warning("cellAudioGetPortConfig(portNum=%d, portConfig=*0x%x)", portNum, portConfig);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (!portConfig || portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	AudioPortConfig& port = g_audio.ports[portNum];

	portConfig->readIndexAddr = port.read_index_addr;

	switch (auto state = port.state.load())
	{
	case AUDIO_PORT_STATE_CLOSED: portConfig->status = CELL_AUDIO_STATUS_CLOSE; break;
	case AUDIO_PORT_STATE_OPENED: portConfig->status = CELL_AUDIO_STATUS_READY; break;
	case AUDIO_PORT_STATE_STARTED: portConfig->status = CELL_AUDIO_STATUS_RUN; break;
	default: throw EXCEPTION("Invalid port state (%d: %d)", portNum, state);
	}

	portConfig->nChannel = port.channel;
	portConfig->nBlock = port.block;
	portConfig->portSize = port.size;
	portConfig->portAddr = port.addr;
	return CELL_OK;
}

s32 cellAudioPortStart(u32 portNum)
{
	cellAudio.warning("cellAudioPortStart(portNum=%d)", portNum);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	switch (auto state = g_audio.ports[portNum].state.compare_and_swap(AUDIO_PORT_STATE_OPENED, AUDIO_PORT_STATE_STARTED))
	{
	case AUDIO_PORT_STATE_CLOSED: return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	case AUDIO_PORT_STATE_STARTED: return CELL_AUDIO_ERROR_PORT_ALREADY_RUN;
	case AUDIO_PORT_STATE_OPENED: return CELL_OK;
	default: throw EXCEPTION("Invalid port state (%d: %d)", portNum, state);
	}
}

s32 cellAudioPortClose(u32 portNum)
{
	cellAudio.warning("cellAudioPortClose(portNum=%d)", portNum);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	switch (auto state = g_audio.ports[portNum].state.exchange(AUDIO_PORT_STATE_CLOSED))
	{
	case AUDIO_PORT_STATE_CLOSED: return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	case AUDIO_PORT_STATE_STARTED: return CELL_OK;
	case AUDIO_PORT_STATE_OPENED: return CELL_OK;
	default: throw EXCEPTION("Invalid port state (%d: %d)", portNum, state);
	}
}

s32 cellAudioPortStop(u32 portNum)
{
	cellAudio.warning("cellAudioPortStop(portNum=%d)", portNum);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	switch (auto state = g_audio.ports[portNum].state.compare_and_swap(AUDIO_PORT_STATE_STARTED, AUDIO_PORT_STATE_OPENED))
	{
	case AUDIO_PORT_STATE_CLOSED: return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	case AUDIO_PORT_STATE_STARTED: return CELL_OK;
	case AUDIO_PORT_STATE_OPENED: return CELL_AUDIO_ERROR_PORT_NOT_RUN;
	default: throw EXCEPTION("Invalid port state (%d: %d)", portNum, state);
	}
}

s32 cellAudioGetPortTimestamp(u32 portNum, u64 tag, vm::ptr<u64> stamp)
{
	cellAudio.trace("cellAudioGetPortTimestamp(portNum=%d, tag=0x%llx, stamp=*0x%x)", portNum, tag, stamp);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	AudioPortConfig& port = g_audio.ports[portNum];

	if (port.state == AUDIO_PORT_STATE_CLOSED)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	// TODO: check tag (CELL_AUDIO_ERROR_TAG_NOT_FOUND error)

	*stamp = g_audio.start_time + Emu.GetPauseTime() + (port.counter + (tag - port.tag)) * 256000000 / 48000;

	return CELL_OK;
}

s32 cellAudioGetPortBlockTag(u32 portNum, u64 blockNo, vm::ptr<u64> tag)
{
	cellAudio.trace("cellAudioGetPortBlockTag(portNum=%d, blockNo=0x%llx, tag=*0x%x)", portNum, blockNo, tag);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	AudioPortConfig& port = g_audio.ports[portNum];

	if (port.state == AUDIO_PORT_STATE_CLOSED)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (blockNo >= port.block)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	u64 tag_base = port.tag;
	if (tag_base % port.block > blockNo)
	{
		tag_base &= ~(port.block - 1);
		tag_base += port.block;
	}
	else
	{
		tag_base &= ~(port.block - 1);
	}
	*tag = tag_base + blockNo;

	return CELL_OK;
}

s32 cellAudioSetPortLevel(u32 portNum, float level)
{
	cellAudio.trace("cellAudioSetPortLevel(portNum=%d, level=%f)", portNum, level);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	AudioPortConfig& port = g_audio.ports[portNum];

	if (port.state == AUDIO_PORT_STATE_CLOSED)
	{
		return CELL_AUDIO_ERROR_PORT_NOT_OPEN;
	}

	if (level >= 0.0f)
	{
		port.level_set.exchange({ level, (port.level - level) / 624.0f });
	}
	else
	{
		cellAudio.todo("cellAudioSetPortLevel(%d): negative level value (%f)", portNum, level);
	}

	return CELL_OK;
}

s32 cellAudioCreateNotifyEventQueue(vm::ptr<u32> id, vm::ptr<u64> key)
{
	cellAudio.warning("cellAudioCreateNotifyEventQueue(id=*0x%x, key=*0x%x)", id, key);

	for (u64 k = 0; k < 100; k++)
	{
		const u64 key_value = 0x80004d494f323221ull + k;

		// register key if not used yet
		if (const auto queue = Emu.GetEventManager().MakeEventQueue(key_value, SYS_SYNC_FIFO, SYS_PPU_QUEUE, 0, key_value, 32))
		{
			*id = queue->id;
			*key = key_value;

			return CELL_OK;
		}
	}

	return CELL_AUDIO_ERROR_EVENT_QUEUE;
}

s32 cellAudioCreateNotifyEventQueueEx(vm::ptr<u32> id, vm::ptr<u64> key, u32 iFlags)
{
	cellAudio.todo("cellAudioCreateNotifyEventQueueEx(id=*0x%x, key=*0x%x, iFlags=0x%x)", id, key, iFlags);

	if (iFlags & ~CELL_AUDIO_CREATEEVENTFLAG_SPU)
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	// TODO

	return CELL_AUDIO_ERROR_EVENT_QUEUE;
}

s32 cellAudioSetNotifyEventQueue(u64 key)
{
	cellAudio.warning("cellAudioSetNotifyEventQueue(key=0x%llx)", key);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(g_audio.mutex);

	for (auto k : g_audio.keys) // check for duplicates
	{
		if (k == key)
		{
			return CELL_AUDIO_ERROR_TRANS_EVENT;
		}
	}

	g_audio.keys.emplace_back(key);

	return CELL_OK;
}

s32 cellAudioSetNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.todo("cellAudioSetNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);

	// TODO

	return CELL_OK;
}

s32 cellAudioRemoveNotifyEventQueue(u64 key)
{
	cellAudio.warning("cellAudioRemoveNotifyEventQueue(key=0x%llx)", key);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	std::lock_guard<std::mutex> lock(g_audio.mutex);

	for (auto i = g_audio.keys.begin(); i != g_audio.keys.end(); i++)
	{
		if (*i == key)
		{
			g_audio.keys.erase(i);
			
			return CELL_OK;
		}
	}

	return CELL_AUDIO_ERROR_TRANS_EVENT;
}

s32 cellAudioRemoveNotifyEventQueueEx(u64 key, u32 iFlags)
{
	cellAudio.todo("cellAudioRemoveNotifyEventQueueEx(key=0x%llx, iFlags=0x%x)", key, iFlags);

	// TODO

	return CELL_OK;
}

s32 cellAudioAddData(u32 portNum, vm::ptr<float> src, u32 samples, float volume)
{
	cellAudio.trace("cellAudioAddData(portNum=%d, src=*0x%x, samples=%d, volume=%f)", portNum, src, samples, volume);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (samples != 256)
	{
		// despite the docs, seems that only fixed value is supported
		cellAudio.error("cellAudioAddData(): invalid samples value (%d)", samples);
		return CELL_AUDIO_ERROR_PARAM;
	}

	const AudioPortConfig& port = g_audio.ports[portNum];

	const auto dst = vm::ptr<float>::make(port.addr + u32(port.tag % port.block) * port.channel * 256 * SIZE_32(float));

	for (u32 i = 0; i < samples * port.channel; i++)
	{
		dst[i] += src[i] * volume; // mix all channels
	}

	return CELL_OK;
}

s32 cellAudioAdd2chData(u32 portNum, vm::ptr<float> src, u32 samples, float volume)
{
	cellAudio.trace("cellAudioAdd2chData(portNum=%d, src=*0x%x, samples=%d, volume=%f)", portNum, src, samples, volume);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	if (samples != 256)
	{
		// despite the docs, seems that only fixed value is supported
		cellAudio.error("cellAudioAdd2chData(): invalid samples value (%d)", samples);
		return CELL_AUDIO_ERROR_PARAM;
	}

	const AudioPortConfig& port = g_audio.ports[portNum];

	const auto dst = vm::ptr<float>::make(port.addr + s32(port.tag % port.block) * port.channel * 256 * SIZE_32(float));

	if (port.channel == 2)
	{
		cellAudio.error("cellAudioAdd2chData(portNum=%d): port.channel = 2", portNum);
	}
	else if (port.channel == 6)
	{
		for (u32 i = 0; i < samples; i++)
		{
			dst[i * 6 + 0] += src[i * 2 + 0] * volume; // mix L ch
			dst[i * 6 + 1] += src[i * 2 + 1] * volume; // mix R ch
			//dst[i * 6 + 2] += 0.0f; // center
			//dst[i * 6 + 3] += 0.0f; // LFE
			//dst[i * 6 + 4] += 0.0f; // rear L
			//dst[i * 6 + 5] += 0.0f; // rear R
		}
	}
	else if (port.channel == 8)
	{
		for (u32 i = 0; i < samples; i++)
		{
			dst[i * 8 + 0] += src[i * 2 + 0] * volume; // mix L ch
			dst[i * 8 + 1] += src[i * 2 + 1] * volume; // mix R ch
			//dst[i * 8 + 2] += 0.0f; // center
			//dst[i * 8 + 3] += 0.0f; // LFE
			//dst[i * 8 + 4] += 0.0f; // rear L
			//dst[i * 8 + 5] += 0.0f; // rear R
			//dst[i * 8 + 6] += 0.0f; // side L
			//dst[i * 8 + 7] += 0.0f; // side R
		}
	}
	else
	{
		cellAudio.error("cellAudioAdd2chData(portNum=%d): invalid port.channel value (%d)", portNum, port.channel);
	}

	return CELL_OK;
}

s32 cellAudioAdd6chData(u32 portNum, vm::ptr<float> src, float volume)
{
	cellAudio.trace("cellAudioAdd6chData(portNum=%d, src=*0x%x, volume=%f)", portNum, src, volume);

	if (g_audio.state != AUDIO_STATE_INITIALIZED)
	{
		return CELL_AUDIO_ERROR_NOT_INIT;
	}

	if (portNum >= AUDIO_PORT_COUNT || !src || !src.aligned())
	{
		return CELL_AUDIO_ERROR_PARAM;
	}

	const AudioPortConfig& port = g_audio.ports[portNum];

	const auto dst = vm::ptr<float>::make(port.addr + s32(port.tag % port.block) * port.channel * 256 * SIZE_32(float));

	if (port.channel == 2 || port.channel == 6)
	{
		cellAudio.error("cellAudioAdd2chData(portNum=%d): port.channel = %d", portNum, port.channel);
	}
	else if (port.channel == 8)
	{
		for (u32 i = 0; i < 256; i++)
		{
			dst[i * 8 + 0] += src[i * 6 + 0] * volume; // mix L ch
			dst[i * 8 + 1] += src[i * 6 + 1] * volume; // mix R ch
			dst[i * 8 + 2] += src[i * 6 + 2] * volume; // mix center
			dst[i * 8 + 3] += src[i * 6 + 3] * volume; // mix LFE
			dst[i * 8 + 4] += src[i * 6 + 4] * volume; // mix rear L
			dst[i * 8 + 5] += src[i * 6 + 5] * volume; // mix rear R
			//dst[i * 8 + 6] += 0.0f; // side L
			//dst[i * 8 + 7] += 0.0f; // side R
		}
	}
	else
	{
		cellAudio.error("cellAudioAdd6chData(portNum=%d): invalid port.channel value (%d)", portNum, port.channel);
	}

	return CELL_OK;
}

s32 cellAudioMiscSetAccessoryVolume(u32 devNum, float volume)
{
	cellAudio.todo("cellAudioMiscSetAccessoryVolume(devNum=%d, volume=%f)", devNum, volume);
	return CELL_OK;
}

s32 cellAudioSendAck(u64 data3)
{
	cellAudio.todo("cellAudioSendAck(data3=0x%llx)", data3);
	return CELL_OK;
}

s32 cellAudioSetPersonalDevice(s32 iPersonalStream, s32 iDevice)
{
	cellAudio.todo("cellAudioSetPersonalDevice(iPersonalStream=%d, iDevice=%d)", iPersonalStream, iDevice);
	return CELL_OK;
}

s32 cellAudioUnsetPersonalDevice(s32 iPersonalStream)
{
	cellAudio.todo("cellAudioUnsetPersonalDevice(iPersonalStream=%d)", iPersonalStream);
	return CELL_OK;
}

Module<> cellAudio("cellAudio", []()
{
	g_audio.state = AUDIO_STATE_NOT_INITIALIZED;
	g_audio.buffer = 0;
	g_audio.indexes = 0;

	REG_FUNC(cellAudio, cellAudioInit);
	REG_FUNC(cellAudio, cellAudioPortClose);
	REG_FUNC(cellAudio, cellAudioPortStop);
	REG_FUNC(cellAudio, cellAudioGetPortConfig);
	REG_FUNC(cellAudio, cellAudioPortStart);
	REG_FUNC(cellAudio, cellAudioQuit);
	REG_FUNC(cellAudio, cellAudioPortOpen);
	REG_FUNC(cellAudio, cellAudioSetPortLevel);
	REG_FUNC(cellAudio, cellAudioCreateNotifyEventQueue);
	REG_FUNC(cellAudio, cellAudioCreateNotifyEventQueueEx);
	REG_FUNC(cellAudio, cellAudioMiscSetAccessoryVolume);
	REG_FUNC(cellAudio, cellAudioSetNotifyEventQueue);
	REG_FUNC(cellAudio, cellAudioSetNotifyEventQueueEx);
	REG_FUNC(cellAudio, cellAudioGetPortTimestamp);
	REG_FUNC(cellAudio, cellAudioAdd2chData);
	REG_FUNC(cellAudio, cellAudioAdd6chData);
	REG_FUNC(cellAudio, cellAudioAddData);
	REG_FUNC(cellAudio, cellAudioGetPortBlockTag);
	REG_FUNC(cellAudio, cellAudioRemoveNotifyEventQueue);
	REG_FUNC(cellAudio, cellAudioRemoveNotifyEventQueueEx);
	REG_FUNC(cellAudio, cellAudioSendAck);
	REG_FUNC(cellAudio, cellAudioSetPersonalDevice);
	REG_FUNC(cellAudio, cellAudioUnsetPersonalDevice);
});
