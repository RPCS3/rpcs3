#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/emu_callbacks.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/voice_input.h"
#include "Emu/Cell/timers.hpp"
#include "Utilities/StrUtil.h"

#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_ppu_thread.h"
#include "Emu/Cell/lv2/sys_timer.h"
#include "sysPrxForUser.h"
#include "cellMic.h"
#include "cellVoice.h"
#include "cellVoice_firmware.h"

#include <cmath>
#include <set>

LOG_CHANNEL(cellVoice);

template<>
void fmt_class_string<CellVoiceError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_VOICE_ERROR_ADDRESS_INVALID);
			STR_CASE(CELL_VOICE_ERROR_ARGUMENT_INVALID);
			STR_CASE(CELL_VOICE_ERROR_CONTAINER_INVALID);
			STR_CASE(CELL_VOICE_ERROR_DEVICE_NOT_PRESENT);
			STR_CASE(CELL_VOICE_ERROR_EVENT_DISPATCH);
			STR_CASE(CELL_VOICE_ERROR_EVENT_QUEUE);
			STR_CASE(CELL_VOICE_ERROR_GENERAL);
			STR_CASE(CELL_VOICE_ERROR_LIBVOICE_INITIALIZED);
			STR_CASE(CELL_VOICE_ERROR_LIBVOICE_NOT_INIT);
			STR_CASE(CELL_VOICE_ERROR_NOT_IMPLEMENTED);
			STR_CASE(CELL_VOICE_ERROR_PORT_INVALID);
			STR_CASE(CELL_VOICE_ERROR_RESOURCE_INSUFFICIENT);
			STR_CASE(CELL_VOICE_ERROR_SERVICE_ATTACHED);
			STR_CASE(CELL_VOICE_ERROR_SERVICE_DETACHED);
			STR_CASE(CELL_VOICE_ERROR_SERVICE_HANDLE);
			STR_CASE(CELL_VOICE_ERROR_SERVICE_NOT_FOUND);
			STR_CASE(CELL_VOICE_ERROR_SHAREDMEMORY);
			STR_CASE(CELL_VOICE_ERROR_TOPOLOGY);
		}

		return unknown;
	});
}

namespace
{
	constexpr u32 voice_rate = voice_service::rate;
	constexpr u32 tick_samples = voice_service::tick_samples;
	constexpr u64 tick_usec = voice_service::tick_usec;

	constexpr u32 max_in_voice_frames = 25;     // IN_VOICE: what it keeps at most (latency)
	constexpr u32 in_voice_prebuffer_ticks = 3; // IN_VOICE: how long it waits for more after silence (jitter)
	constexpr u32 max_in_voice_bytes = 8192;    // IN_VOICE: what a write may queue
	constexpr u32 max_out_voice_frames = 50;    // OUT_VOICE: what it keeps for the game at most
	constexpr u32 vad_hangover_ticks = 15;      // OUT_VOICE: frames still sent after the voice drops (300 ms)
	constexpr usz max_mic_samples = voice_rate * 3 / 10;
	constexpr usz mic_prebuffer_samples = tick_samples * 2;
	constexpr usz max_headset_samples = voice_rate * 4 / 10;
	constexpr usz headset_prebuffer_samples = tick_samples * 2;

	bool is_input_port(u32 type)
	{
		return type <= CELLVOICE_PORTTYPE_IN_VOICE;
	}

	bool is_output_port(u32 type)
	{
		return type >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO && type <= CELLVOICE_PORTTYPE_OUT_SECONDARY;
	}

	// 2:1 between 16 and 8 kHz: a 31-tap windowed-sinc low-pass at 3.8 kHz (Blackman window, unity gain)
	struct halfband_coefs
	{
		std::array<f32, 31> h{};

		halfband_coefs()
		{
			constexpr f64 fc = 3800.0 / voice_rate;
			f64 sum = 0.0;

			for (s32 i = 0; i < 31; i++)
			{
				const f64 n = i - 15.0;
				const f64 sinc = i == 15 ? 2 * fc : std::sin(2 * std::numbers::pi * fc * n) / (std::numbers::pi * n);
				const f64 w = 0.42 - 0.5 * std::cos(2 * std::numbers::pi * i / 30) + 0.08 * std::cos(4 * std::numbers::pi * i / 30);
				h[i] = static_cast<f32>(sinc * w);
				sum += h[i];
			}

			for (f32& c : h)
			{
				c = static_cast<f32>(c / sum);
			}
		}
	};

	const halfband_coefs s_halfband;

	u32 pcm_sample_size(u32 type)
	{
		return type == CELLVOICE_PCM_SHORT || type == CELLVOICE_PCM_SHORT_LITTLE_ENDIAN ? 2 : 4;
	}

	// A PCM port's sample on the 16-bit scale
	f32 pcm_read(const u8* p, u32 type)
	{
		switch (type)
		{
		case CELLVOICE_PCM_FLOAT: { be_t<f32> v; std::memcpy(&v, p, 4); return static_cast<f32>(v) * 32768.0f; }
		case CELLVOICE_PCM_FLOAT_LITTLE_ENDIAN: { f32 v; std::memcpy(&v, p, 4); return v * 32768.0f; }
		case CELLVOICE_PCM_SHORT: { be_t<s16> v; std::memcpy(&v, p, 2); return static_cast<s16>(v); }
		case CELLVOICE_PCM_SHORT_LITTLE_ENDIAN: { s16 v; std::memcpy(&v, p, 2); return v; }
		case CELLVOICE_PCM_INTEGER: { be_t<s32> v; std::memcpy(&v, p, 4); return static_cast<s32>(v) / 65536.0f; }
		default: { s32 v; std::memcpy(&v, p, 4); return v / 65536.0f; }
		}
	}

	void pcm_write(u8* p, u32 type, f32 v)
	{
		v = std::clamp(v, -32768.0f, 32767.0f);

		switch (type)
		{
		case CELLVOICE_PCM_FLOAT: { const be_t<f32> x = v / 32768.0f; std::memcpy(p, &x, 4); break; }
		case CELLVOICE_PCM_FLOAT_LITTLE_ENDIAN: { const f32 x = v / 32768.0f; std::memcpy(p, &x, 4); break; }
		case CELLVOICE_PCM_SHORT: { const be_t<s16> x = static_cast<s16>(std::lround(v)); std::memcpy(p, &x, 2); break; }
		case CELLVOICE_PCM_SHORT_LITTLE_ENDIAN: { const s16 x = static_cast<s16>(std::lround(v)); std::memcpy(p, &x, 2); break; }
		case CELLVOICE_PCM_INTEGER: { const be_t<s32> x = static_cast<s32>(std::lround(v)) * 65536; std::memcpy(p, &x, 4); break; }
		default: { const s32 x = static_cast<s32>(std::lround(v)) * 65536; std::memcpy(p, &x, 4); break; }
		}
	}

	// The CELP configuration of a voice port's bit rate (the narrowband ones; the 16 kHz RPE rates are not done)
	const voice_firmware::config* voice_config(u32 bitrate)
	{
		return voice_firmware::find_config(bitrate);
	}

	// A voice port's bytes per frame (sys_voice's table, the RPE rates included)
	u32 voice_frame_bytes(u32 bitrate)
	{
		if (const voice_firmware::config* c = voice_config(bitrate))
		{
			return c->bytes;
		}

		switch (bitrate)
		{
		case CELLVOICE_BITRATE_14400: return 27;
		case CELLVOICE_BITRATE_16000: return 20;
		case CELLVOICE_BITRATE_22533: return 43;
		default: return 1;
		}
	}

}

void voice_service::halfband_filter::down(const f32* in, u32 count, std::vector<f32>& out)
{
	for (u32 i = 0; i < count; i++)
	{
		const f32 y = push(in[i]);
		if (i & 1) out.push_back(y);
	}
}

f32 voice_service::halfband_filter::push(f32 v)
{
	std::copy_backward(x.begin(), x.end() - 1, x.end());
	x[0] = v;

	f32 s = 0.0f;
	for (usz i = 0; i < x.size(); i++) s += s_halfband.h[i] * x[i];
	return s;
}

void voice_service::halfband_filter::up(const f32* in, u32 count, std::vector<f32>& out)
{
	for (u32 i = 0; i < count; i++)
	{
		out.push_back(2.0f * push(in[i]));
		out.push_back(2.0f * push(0.0f));
	}
}

voice_service::voice_service(voice_manager&, bool)
{
	firmware = std::make_unique<voice_firmware::backend>();
}

voice_service::~voice_service()
{
	close_headset();
	close_mic();
}

void voiceWorkerEntry(ppu_thread& ppu, u64 generation)
{
	auto& manager = g_fxo->get<voice_manager>();
	u64 next = get_system_time();

	while (!ppu.test_stopped())
	{
		// A bounded tick may call firmware through fast_call. Save only between ticks,
		// when all inputs and ownership records are back in persistent service state.
		std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};
		if (!savestate_lock.owns_lock())
		{
			ppu.state += cpu_flag::again;
			break;
		}

		std::vector<voice_firmware::request> requests;
		std::vector<voice_firmware::key> keys;
		voice_service* service = nullptr;
		bool stopping = false;
		{
			std::lock_guard lock(manager.mtx);
			if (manager.service_generation != generation || !manager.m_service)
				break;
			service = manager.m_service.get();
			stopping = manager.run_state == voice_run_state::stopping || manager.run_state == voice_run_state::failed;
			if (manager.run_state == voice_run_state::running)
			{
				service->tick(manager);
				requests.swap(service->requests);
				keys = service->codec_keys();
			}
		}

		if (stopping)
		{
			const s32 rc = service->firmware->stop(ppu);
			if (ppu.test_stopped())
				break;
			std::lock_guard lock(manager.mtx);
			manager.last_error = rc;
			break;
		}

		const s32 reconcile_error = service->firmware->reconcile(ppu, keys);
		if (ppu.test_stopped())
			break;
		if (reconcile_error)
		{
			std::lock_guard lock(manager.mtx);
			manager.last_error = reconcile_error;
			manager.run_state = voice_run_state::failed;
			manager.voice_service_started = false;
			service->stop(manager);
			manager.notify(CELLVOICE_EVENT_SERVICE_DETACHED);
			continue;
		}
		for (const auto& request : requests)
		{
			const auto result = service->firmware->process(ppu, request);
			if (ppu.test_stopped())
				break;
			std::lock_guard lock(manager.mtx);
			if (manager.service_generation == generation && manager.run_state == voice_run_state::running)
				service->commit(manager, result);
		}
		if (ppu.test_stopped())
			break;

		savestate_lock.unlock();
		const u64 now = get_system_time();
		next += tick_usec;
		if (next + tick_usec * 5 < now)
			next = now + tick_usec;
		sys_timer_usleep(ppu, next > now ? next - now : 1);
	}
	if (ppu.state & cpu_flag::again)
	{
		ppu.syscall_args[0] = generation;
		return;
	}
	if (!ppu.test_stopped())
		ppu_execute<&sys_ppu_thread_exit>(ppu, CELL_OK);
}

void voice_service::tick(voice_manager& manager)
{
	if (!manager.is_init || !manager.voice_service_started)
	{
		if (mic || headset)
		{
			stop(manager);
		}

		return;
	}

	bool want_mic = false, want_headset = false;

	for (const auto& [id, port] : manager.ports)
	{
		if (port.info.portType == CELLVOICE_PORTTYPE_IN_MIC)
		{
			want_mic = true;
		}
		else if (port.info.portType == CELLVOICE_PORTTYPE_OUT_SECONDARY)
		{
			want_headset |= std::any_of(edges.begin(), edges.end(), [id](const auto& e) { return e.second == id; });
		}
	}

	if (want_mic) open_mic();
	else close_mic();
	const bool transmit_mic = update_mic_gate(manager);

	if (want_headset) open_headset();
	else close_headset();

	pull_mic();
	if (!transmit_mic)
	{
		// Drain the device while closed, so pressing the key never sends old capture.
		mic_fifo.clear();
		mic_primed = false;
	}

	// A game learns of the headset microphone from PORT_ATTACHED for its IN_MIC port (data2 = the port)
	for (const auto& [id, port] : manager.ports)
	{
		if (port.info.portType != CELLVOICE_PORTTYPE_IN_MIC)
		{
			continue;
		}

		port_data& d = data[id];

		if (d.attached != mic_present())
		{
			d.attached = mic_present();
			cellVoice.notice("IN_MIC port 0x%x: %s", id, d.attached ? "CELLVOICE_EVENT_PORT_ATTACHED" : "CELLVOICE_EVENT_PORT_DETACHED");
			manager.notify(d.attached ? CELLVOICE_EVENT_PORT_ATTACHED : CELLVOICE_EVENT_PORT_DETACHED, id);
		}
	}

	std::array<f32, tick_samples> mic_signal{};
	take_mic(mic_signal.data());

	// The inputs' samples this period
	for (const auto& [id, port] : manager.ports)
	{
		if (!is_input_port(port.info.portType))
		{
			continue;
		}

		port_data& d = data[id];
		d.signal.fill(0.0f);

		switch (port.info.portType)
		{
		case CELLVOICE_PORTTYPE_IN_MIC: d.signal = mic_signal; break;
		case CELLVOICE_PORTTYPE_IN_PCMAUDIO: read_pcm(port, d); break;
		case CELLVOICE_PORTTYPE_IN_VOICE: decode_voice(port, d, id); break;
		default: break;
		}

		measure_port(d, d.signal.data(), port.info.portType == CELLVOICE_PORTTYPE_IN_MIC);
		if (port.info.portType == CELLVOICE_PORTTYPE_IN_MIC && d.vad_activity <= d.vad_threshold)
			d.signal.fill(0.0f);

		const f32 gain = static_cast<u16>(port.info.bMute) || mute_all || d.paused ? 0.0f : static_cast<f32>(port.info.volume);

		if (gain != 1.0f)
		{
			for (f32& s : d.signal) s *= gain;
		}
	}

	std::array<f32, tick_samples> headset_mix{};

	// Each output: the mix of its connected inputs
	for (const auto& [id, port] : manager.ports)
	{
		if (!is_output_port(port.info.portType))
		{
			continue;
		}

		port_data& d = data[id];
		std::array<f32, tick_samples> mix{};
		bool has_mic = false, has_other_input = false;

		for (const auto& [in, out] : edges)
		{
			if (out != id)
			{
				continue;
			}

			if (const auto it = data.find(in); it != data.end() && manager.ports.contains(in))
			{
				if (manager.ports.at(in).info.portType == CELLVOICE_PORTTYPE_IN_MIC) has_mic = true;
				else has_other_input = true;
				for (u32 i = 0; i < tick_samples; i++) mix[i] += it->second.signal[i];
			}
		}

		const f32 gain = static_cast<u16>(port.info.bMute) || d.paused ? 0.0f : static_cast<f32>(port.info.volume);

		if (gain != 1.0f)
		{
			for (f32& s : mix) s *= gain;
		}

		measure_port(d, mix.data(), false);

		switch (port.info.portType)
		{
		case CELLVOICE_PORTTYPE_OUT_PCMAUDIO: write_pcm(port, d, mix.data()); break;
		case CELLVOICE_PORTTYPE_OUT_VOICE:
			if (!has_mic || has_other_input || transmit_mic) encode_voice(port, d, id, mix.data());
			break;
		case CELLVOICE_PORTTYPE_OUT_SECONDARY:
			for (u32 i = 0; i < tick_samples; i++) headset_mix[i] += mix[i];
			break;
		default: break;
		}
	}

	// All secondary ports share one host stream and one playback period.
	if (want_headset) play_headset(headset_mix.data());
}

void voice_service::stop(voice_manager& manager)
{
	close_headset();
	close_mic();

	for (auto& [id, d] : data)
	{
		if (d.attached)
		{
			d.attached = false;
			manager.notify(CELLVOICE_EVENT_PORT_DETACHED, id);
		}

		d.bytes.clear();
		d.pcm.clear();
		d.playing = false;
		d.waited = 0;
		d.hangover = 0;
		d.noise_db = 30.0f;
		d.mic_noise_db = 30.0f;
		d.vad_activity = 0;
		d.filter = {};
		d.signal.fill(0.0f);
		d.logged = false;
		d.read_logged = false;
		d.codec_generation = 0;
		d.codec_pending = 0;
		d.codec_failed = false;
	}
	requests.clear();

	mic_failed = false;
	headset_failed = false;
}

void voice_service::remove_port(u16 id)
{
	data.erase(id);
	std::erase_if(edges, [id](const auto& e) { return e.first == id || e.second == id; });
}

void voice_service::clear()
{
	data.clear();
	edges.clear();
	requests.clear();
	mute_all = false;
}

u32 voice_service::set_port_attr(const voice_manager::port_t& port, u16 id, u32 attr, u32 value)
{
	switch (attr)
	{
	case CELLVOICE_ATTR_ENERGY_LEVEL:
		// The setter takes a 32-bit integer enable flag; the getter returns a float measurement.
		data[id].energy_enabled = value != 0;
		return CELL_OK;
	case CELLVOICE_ATTR_VAD:
		if (port.info.portType != CELLVOICE_PORTTYPE_IN_MIC)
			return CELL_VOICE_ERROR_ARGUMENT_INVALID;
		data[id].vad_threshold = value;
		return CELL_OK;
	default:
		return port.info.portType == CELLVOICE_PORTTYPE_IN_MIC ? CELL_VOICE_ERROR_DEVICE_NOT_PRESENT : CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}
}

u32 voice_service::get_port_attr(const voice_manager::port_t& port, u16 id, u32 attr, u32& value) const
{
	const auto it = data.find(id);
	switch (attr)
	{
	case CELLVOICE_ATTR_ENERGY_LEVEL:
		value = std::bit_cast<u32>(it == data.end() ? 0.0f : it->second.energy_level);
		return CELL_OK;
	case CELLVOICE_ATTR_VAD:
		if (port.info.portType != CELLVOICE_PORTTYPE_IN_MIC)
			return CELL_VOICE_ERROR_ARGUMENT_INVALID;
		value = it == data.end() ? 0 : it->second.vad_activity;
		return CELL_OK;
	default:
		return port.info.portType == CELLVOICE_PORTTYPE_IN_MIC ? CELL_VOICE_ERROR_DEVICE_NOT_PRESENT : CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}
}

void voice_service::measure_port(port_data& d, const f32* samples, bool microphone)
{
	if (!d.energy_enabled && !microphone)
		return;

	f64 sum = 0.0;
	for (u32 i = 0; i < tick_samples; i++)
		sum += static_cast<f64>(samples[i]) * samples[i];

	if (d.energy_enabled)
	{
		// Use the same host energy approximation as cellMic (40..90 dB).
		const f64 rms = std::sqrt(sum / tick_samples) / 32768.0;
		d.energy_level = static_cast<f32>(std::clamp(90.0 + 10.0 * std::log10(std::max(rms, 0.00001)), 40.0, 90.0));
	}

	if (microphone)
	{
		if (d.vad_threshold < 1 || d.vad_threshold > 10)
			d.vad_threshold = 8;

		// Approximate the microphone DSP's 0..10 local-talk score using our existing
		// noise-floor detector. A detected voice scores 9, as in cellMic; silence scores 0.
		const f32 level = static_cast<f32>(10.0 * std::log10(sum / tick_samples + 1.0));
		const bool active = level > std::max(d.mic_noise_db + 9.0f, 40.0f);
		d.vad_activity = active ? 9 : 0;
		if (!active)
			d.mic_noise_db = std::max(level < d.mic_noise_db ? level : d.mic_noise_db + 0.05f, 20.0f);
	}
}

bool voice_service::ensure_codec(port_data& d, u32 bitrate, bool encode, u16 id)
{
	if (d.codec_bitrate == bitrate && d.codec_generation && d.codec_encode == encode)
	{
		return !d.codec_failed;
	}
	if (d.codec_generation && d.codec_bitrate != bitrate)
	{
		// Bytes queued under the old framing cannot be read as packets of the new rate.
		d.bytes.clear();
		d.filter = {};
		d.playing = false;
		d.waited = 0;
		d.hangover = 0;
		d.logged = false;
		d.read_logged = false;
	}

	d.pcm.clear();
	d.codec_bitrate = bitrate;
	d.codec_generation = next_codec_generation++;
	d.codec_encode = encode;
	d.codec_pending = 0;
	d.codec_failed = false;

	const auto* cfg = voice_config(bitrate);

	if (!cfg)
	{
		if (!d.logged)
		{
			cellVoice.todo("Voice port 0x%x: bit rate %d is not implemented", id, bitrate);
			d.logged = true;
		}

		d.codec_failed = true;
		return false;
	}

	return true;
}

void voice_service::decode_voice(const voice_manager::port_t& port, port_data& d, u16 id)
{
	if (!ensure_codec(d, port.info.voice.bitrate, false, id))
	{
		d.bytes.clear();
		return;
	}

	const auto& cfg = *voice_config(d.codec_bitrate);
	const u32 frame_bytes = cfg.bytes;

	// Latency: keep the newest frames
	if (d.bytes.size() > usz{max_in_voice_frames} * frame_bytes)
	{
		const usz drop = (d.bytes.size() - usz{max_in_voice_frames} * frame_bytes + frame_bytes - 1) / frame_bytes * frame_bytes;
		d.bytes.erase(d.bytes.begin(), d.bytes.begin() + std::min(drop, d.bytes.size()));
	}

	if (!d.playing)
	{
		// After silence, wait a little for the rest of the talker's burst (games send voice every 200 ms or so)
		if (d.bytes.size() < frame_bytes)
		{
			d.waited = 0;
			return;
		}

		if (++d.waited < in_voice_prebuffer_ticks)
		{
			return;
		}

		d.playing = true;
	}

	const usz n = std::min<usz>(tick_samples, d.pcm.size());
	std::copy_n(d.pcm.begin(), n, d.signal.begin());
	d.pcm.erase(d.pcm.begin(), d.pcm.begin() + n);

	// Refill after consuming this tick. Testing before consumption inserts a silent
	// tick between every pair of 20 ms packets and makes the input backlog grow.
	if (!d.codec_pending)
	{
		// RPE frames span 10 or 15 ms. Refill a full service period, including
		// any samples carried from the preceding frame, before the next tick.
		const u32 frame_samples = cfg.samples * 16000 / cfg.sample_rate;
		for (usz queued = d.pcm.size(); queued < tick_samples && d.bytes.size() >= frame_bytes; queued += frame_samples)
		{
			voice_firmware::request request;
			request.codec = {id, d.codec_generation, false};
			request.bitrate = d.codec_bitrate;
			request.packet.assign(d.bytes.begin(), d.bytes.begin() + frame_bytes);
			d.bytes.erase(d.bytes.begin(), d.bytes.begin() + frame_bytes);
			d.codec_pending++;
			requests.push_back(std::move(request));
		}
	}

	if (!d.codec_pending && d.pcm.empty() && d.bytes.size() < frame_bytes)
	{
		d.playing = false;
		d.waited = 0;
	}
}

void voice_service::encode_voice(const voice_manager::port_t& port, port_data& d, u16 id, const f32* mix)
{
	if (port.info.bMute || d.paused || mute_all)
	{
		d.pcm.clear();
		d.hangover = 0;
		return;
	}
	if (!ensure_codec(d, port.info.voice.bitrate, true, id))
	{
		return;
	}

	// Voice activity (cellVoice sends no frames in silence): the level against the input's noise floor
	f64 energy = 0.0;
	for (u32 i = 0; i < tick_samples; i++) energy += static_cast<f64>(mix[i]) * mix[i];
	const f32 level = static_cast<f32>(10.0 * std::log10(energy / tick_samples + 1.0));

	const bool active = level > std::max(d.noise_db + 9.0f, 40.0f);
	// Learn the background only outside speech. Raising the floor while someone
	// talks eventually classifies a sustained sentence as noise and cuts it off.
	if (!active)
		d.noise_db = std::max(level < d.noise_db ? level : d.noise_db + 0.05f, 20.0f);

	if (active)
	{
		d.hangover = vad_hangover_ticks;
	}
	else if (d.hangover)
	{
		d.hangover--;
	}
	else
	{
		d.pcm.clear();
		return;
	}

	const auto& cfg = *voice_config(d.codec_bitrate);
	if (cfg.sample_rate == 16000)
		d.pcm.insert(d.pcm.end(), mix, mix + tick_samples);
	else
		d.filter.down(mix, tick_samples, d.pcm);

	if (!d.codec_pending)
	{
		// Bound the batch to one tick's worth of whole frames. Firmware calls
		// remain sequential, outside the manager lock, even for the same stream.
		const u32 max_frames = (tick_samples * cfg.sample_rate / 16000 + cfg.samples - 1) / cfg.samples;
		while (d.codec_pending < max_frames && d.pcm.size() >= cfg.samples)
		{
			voice_firmware::request request;
			request.codec = {id, d.codec_generation, true};
			request.bitrate = d.codec_bitrate;
			request.pcm.assign(d.pcm.begin(), d.pcm.begin() + cfg.samples);
			d.pcm.erase(d.pcm.begin(), d.pcm.begin() + cfg.samples);
			d.codec_pending++;
			requests.push_back(std::move(request));
		}
	}
}

void voice_service::commit(voice_manager& manager, const voice_firmware::result& result)
{
	update_mic_gate(manager);
	const auto found = data.find(result.codec.port);
	const auto port = manager.access_port(result.codec.port);
	if (!port || found == data.end())
		return;
	auto& d = found->second;
	if (d.codec_generation != result.codec.generation || d.codec_encode != result.codec.encode)
		return;
	if (d.codec_pending) d.codec_pending--;
	if (d.codec_failed) return;
	if (d.codec_bitrate != static_cast<u32>(port->info.voice.bitrate))
		return;
	if (result.error)
	{
		d.codec_failed = true;
		d.bytes.clear();
		d.pcm.clear();
		cellVoice.error("Voice port 0x%x: firmware codec failed (0x%x)", result.codec.port, result.error);
		manager.notify(CELLVOICE_EVENT_DATA_ERROR, result.codec.port);
		return;
	}
	if (result.codec.encode)
	{
		if (port->info.bMute || d.paused || mute_all)
		{
			d.pcm.clear();
			d.hangover = 0;
			return;
		}
		d.bytes.insert(d.bytes.end(), result.packet.begin(), result.packet.end());
		const u32 frame_bytes = voice_frame_bytes(d.codec_bitrate);
		if (d.bytes.size() > usz{max_out_voice_frames} * frame_bytes)
			d.bytes.erase(d.bytes.begin(), d.bytes.end() - usz{max_out_voice_frames} * frame_bytes);
		if (!result.packet.empty() && !d.logged)
		{
			cellVoice.success("OUT_VOICE port 0x%x: firmware encoding at %d bps (%d-byte frames)", result.codec.port, d.codec_bitrate, frame_bytes);
			d.logged = true;
		}
	}
	else
	{
		if (voice_config(d.codec_bitrate)->sample_rate == 16000)
			d.pcm.insert(d.pcm.end(), result.pcm.begin(), result.pcm.end());
		else
			d.filter.up(result.pcm.data(), static_cast<u32>(result.pcm.size()), d.pcm);
	}
}

std::vector<voice_firmware::key> voice_service::codec_keys() const
{
	std::vector<voice_firmware::key> keys;
	for (const auto& [id, d] : data)
	{
		if (d.codec_generation && !d.codec_failed)
			keys.push_back({id, d.codec_generation, d.codec_encode});
	}
	return keys;
}

void voice_service::read_pcm(const voice_manager::port_t& port, port_data& d)
{
	const CellVoicePCMFormat& format = port.info.pcmaudio.format;
	const u32 channels = std::max<u32>(format.numChannels, 1);
	const u32 type = format.dataType;
	const u32 sample_size = pcm_sample_size(type);
	const usz frames = std::min<usz>(tick_samples, d.bytes.size() / (sample_size * channels));

	for (usz i = 0; i < frames; i++)
	{
		f32 s = 0.0f;
		for (u32 c = 0; c < channels; c++) s += pcm_read(d.bytes.data() + (i * channels + c) * sample_size, type);
		d.signal[i] = s / channels;
	}

	d.bytes.erase(d.bytes.begin(), d.bytes.begin() + frames * sample_size * channels);
}

void voice_service::write_pcm(const voice_manager::port_t& port, port_data& d, const f32* mix)
{
	const CellVoicePCMFormat& format = port.info.pcmaudio.format;
	const u32 channels = std::max<u32>(format.numChannels, 1);
	const u32 type = format.dataType;
	const u32 frame_size = pcm_sample_size(type) * channels;
	const usz at = d.bytes.size();
	d.bytes.resize(at + usz{tick_samples} * frame_size);

	for (u32 i = 0; i < tick_samples; i++)
	{
		for (u32 c = 0; c < channels; c++) pcm_write(d.bytes.data() + at + i * frame_size + c * pcm_sample_size(type), type, mix[i]);
	}

	// Its buffer (bufSize) holds the newest samples
	const usz capacity = std::max<usz>(static_cast<u32>(port.info.pcmaudio.bufSize) / frame_size * frame_size, usz{tick_samples} * frame_size);

	if (d.bytes.size() > capacity)
	{
		d.bytes.erase(d.bytes.begin(), d.bytes.end() - capacity);
	}
}

u32 voice_service::write(const voice_manager::port_t& port, u16 id, const u8* src, u32 size)
{
	u32 accepted = 0;

	switch (port.info.portType)
	{
	case CELLVOICE_PORTTYPE_IN_VOICE:
	case CELLVOICE_PORTTYPE_IN_PCMAUDIO:
	{
		port_data& d = data[id];
		const usz capacity = port.info.portType == CELLVOICE_PORTTYPE_IN_VOICE ? max_in_voice_bytes : std::max<u32>(port.info.pcmaudio.bufSize, tick_samples * 8);
		accepted = static_cast<u32>(std::min<usz>(size, capacity - std::min(capacity, d.bytes.size())));

		if (accepted)
		{
			d.bytes.insert(d.bytes.end(), src, src + accepted);

			if (!d.logged && port.info.portType == CELLVOICE_PORTTYPE_IN_VOICE)
			{
				cellVoice.success("IN_VOICE port 0x%x: voice received (%d bytes, %d bps)", id, accepted, static_cast<u32>(port.info.voice.bitrate));
				d.logged = true;
			}
		}

		break;
	}
	default:
		break;
	}

	return accepted;
}

u32 voice_service::read(voice_manager& manager, const voice_manager::port_t& port, u16 id, u8* dst, u32 capacity)
{
	update_mic_gate(manager);
	const auto it = data.find(id);

	if (it == data.end())
	{
		return 0;
	}

	port_data& d = it->second;
	u32 count = 0;

	switch (port.info.portType)
	{
	case CELLVOICE_PORTTYPE_OUT_VOICE:
	{
		// Whole frames (Black Ops II asks for 255 bytes: 13 frames of 19 bytes, 247, come)
		const u32 frame_bytes = voice_frame_bytes(port.info.voice.bitrate);
		count = static_cast<u32>(std::min<usz>(capacity / frame_bytes, d.bytes.size() / frame_bytes) * frame_bytes);
		break;
	}
	case CELLVOICE_PORTTYPE_OUT_PCMAUDIO:
	{
		const CellVoicePCMFormat& format = port.info.pcmaudio.format;
		const u32 frame_size = pcm_sample_size(format.dataType) * std::max<u32>(format.numChannels, 1);
		count = static_cast<u32>(std::min<usz>(capacity, d.bytes.size()) / frame_size * frame_size);
		break;
	}
	default:
		break;
	}

	if (count)
	{
		std::memcpy(dst, d.bytes.data(), count);
		d.bytes.erase(d.bytes.begin(), d.bytes.begin() + count);

		if (!d.read_logged && port.info.portType == CELLVOICE_PORTTYPE_OUT_VOICE)
		{
			cellVoice.success("OUT_VOICE port 0x%x: the game reads voice (%d bytes)", id, count);
			d.read_logged = true;
		}
	}

	return count;
}

u32 voice_service::bytes_queued(u16 id) const
{
	const auto it = data.find(id);
	return it == data.end() ? 0 : static_cast<u32>(it->second.bytes.size());
}

bool voice_service::mic_present() const
{
	return !!mic;
}

void voice_service::discard_mic_outputs(voice_manager& manager)
{
	std::set<u16> outputs;
	for (const auto& [input, output] : edges)
	{
		const auto port = manager.access_port(input);
		if (port && port->info.portType == CELLVOICE_PORTTYPE_IN_MIC) outputs.insert(output);
	}
	for (const u16 id : outputs)
	{
		if (const auto it = data.find(id); it != data.end())
		{
			auto& d = it->second;
			d.bytes.clear();
			d.pcm.clear();
			d.filter = {};
			d.hangover = 0;
			d.noise_db = 30.0f;
			// A firmware encoder can retain one frame internally. Recreate it as well
			// as dropping queued packets, so the next press cannot expose that frame.
			d.codec_generation = 0;
			d.codec_pending = 0;
			d.codec_failed = false;
		}
		if (const auto port = manager.access_port(id); port && port->info.portType == CELLVOICE_PORTTYPE_OUT_SECONDARY)
		{
			std::lock_guard lock(headset_mtx);
			headset_fifo.clear();
			headset_primed = false;
			headset_prev = headset_cur = 0.0f;
		}
	}
	std::erase_if(requests, [&](const auto& request) { return outputs.contains(request.codec.port); });
	mic_fifo.clear();
	mic_primed = false;
}

bool voice_service::update_mic_gate(voice_manager& manager)
{
	const auto talk = voice_input::talk.load();
	const auto devices = fmt::split(g_cfg.audio.microphone_devices.get(), {"@@@"});
	const bool source_changed = mic_source_changed || mic_config_type != static_cast<u32>(g_cfg.audio.microphone_type.get()) ||
		mic_config_device != (devices.empty() ? "" : devices.front());
	const bool allowed = mic_present() && !source_changed && talk->transmitting;
	if (source_changed || mic_transmitting != allowed || mic_talk_generation != talk->generation)
	{
		discard_mic_outputs(manager);
	}
	mic_source_changed = false;
	mic_transmitting = allowed;
	mic_talk_generation = talk->generation;
	return allowed;
}

void voice_service::open_mic()
{
	const microphone_handler type = g_cfg.audio.microphone_type.get();
	const auto devices = fmt::split(g_cfg.audio.microphone_devices.get(), {"@@@"});
	const std::string device_name = devices.empty() ? "" : devices.front();
	if (mic_config_type != static_cast<u32>(type) || mic_config_device != device_name)
	{
		close_mic();
		mic_failed = false;
		mic_config_type = static_cast<u32>(type);
		mic_config_device = device_name;
		mic_source_changed = true;
	}

	if (mic || mic_failed)
	{
		return;
	}

	// The microphone configured for cellMic (its first device)
	if (type == microphone_handler::null || device_name.empty())
	{
		cellVoice.notice("No microphone configured: the voice chat's IN_MIC ports stay detached");
		mic_failed = true;
		return;
	}

	const u8 channels = type == microphone_handler::singstar || type == microphone_handler::real_singstar ? 2 : 1;
	auto device = std::make_unique<microphone_device>(type);
	device->add_device(devices[0]);
	device->set_registered(true);

	if (device->open_microphone(CELLMIC_SIGTYPE_RAW, voice_rate, voice_rate, channels) + 0u)
	{
		cellVoice.error("Failed to open the microphone '%s' for the voice chat", devices[0]);
		mic_failed = true;
		return;
	}

	if (device->start_microphone() + 0u)
	{
		cellVoice.error("Failed to start the microphone '%s' for the voice chat", devices[0]);
		device->close_microphone();
		mic_failed = true;
		return;
	}

	mic = std::move(device);
	mic_fifo.clear();
	mic_primed = false;
	mic_resample_pos = 0.0;
	mic_last = 0.0f;
	cellVoice.success("Microphone '%s' for the voice chat (%d Hz, %d channel(s))", devices[0], mic->get_raw_samplingrate(), mic->get_num_channels());
}

void voice_service::close_mic()
{
	if (mic)
	{
		mic->close_microphone();
		mic.reset();
	}

	mic_fifo.clear();
	mic_primed = false;
	mic_resample_pos = 0.0;
	mic_last = 0.0f;
}

void voice_service::pull_mic()
{
	if (mic)
	{
		mic->update_audio();

		const u32 channels = std::max<u32>(mic->get_num_channels(), 1);
		const bool little_endian = mic->get_datatype() == 0;
		const f64 step = static_cast<f64>(mic->get_raw_samplingrate()) / voice_rate;
		mic_raw.resize(channels * 2 * 1024);

		while (const u32 n = mic->read_raw(mic_raw.data(), ::size32(mic_raw)))
		{
			for (u32 i = 0; i < n / (2 * channels); i++)
			{
				f32 x = 0.0f;

				for (u32 c = 0; c < channels; c++)
				{
					const u8* p = mic_raw.data() + (i * channels + c) * 2;
					x += static_cast<s16>(little_endian ? p[0] | p[1] << 8 : p[0] << 8 | p[1]);
				}

				x /= channels;

				// To 16 kHz if the device would not open at it (linear interpolation)
				while (mic_resample_pos <= 1.0)
				{
					mic_fifo.push_back(mic_last + (x - mic_last) * static_cast<f32>(mic_resample_pos));
					mic_resample_pos += step;
				}

				mic_resample_pos -= 1.0;
				mic_last = x;
			}
		}
	}

	if (mic_fifo.size() > max_mic_samples)
	{
		mic_fifo.erase(mic_fifo.begin(), mic_fifo.end() - max_mic_samples);
	}
}

void voice_service::take_mic(f32* out)
{
	std::fill_n(out, tick_samples, 0.0f);

	if (!mic_primed)
	{
		if (mic_fifo.size() < (mic ? mic_prebuffer_samples : tick_samples))
		{
			return;
		}

		mic_primed = true;
	}

	const usz n = std::min<usz>(tick_samples, mic_fifo.size());
	std::copy_n(mic_fifo.begin(), n, out);
	mic_fifo.erase(mic_fifo.begin(), mic_fifo.begin() + n);

	if (n < tick_samples)
	{
		mic_primed = false;
	}
}

void voice_service::open_headset()
{
	std::string device = g_cfg.audio.voice_output_device.to_string();
	if (device.empty()) device = g_cfg.audio.audio_device.to_string();
	const u32 renderer = static_cast<u32>(g_cfg.audio.renderer.get());
	const bool convert_to_s16 = g_cfg.audio.convert_to_s16.get();

	// This runs on the voice service thread. Close waits for the audio callback,
	// so never hold headset_mtx while replacing the backend.
	if (device != headset_config_device || renderer != headset_config_renderer || convert_to_s16 != headset_config_s16)
	{
		close_headset();
		headset_failed = false;
		headset_config_device = device;
		headset_config_renderer = renderer;
		headset_config_s16 = convert_to_s16;
	}

	if (headset)
	{
		const bool device_changed = headset_device_changed.exchange(false);
		if (headset->Operational() && (!device_changed || !headset->DefaultDeviceChanged()))
		{
			return;
		}

		close_headset();
	}

	// Retry unavailable devices once per second of service time, not every tick.
	if (headset_retry_ticks && --headset_retry_ticks)
	{
		return;
	}

	std::shared_ptr<AudioBackend> backend = g_emu_callbacks.get_audio();

	if (!backend)
	{
		headset_failed = true;
		headset_retry_ticks = 1'000'000 / tick_usec;
		return;
	}

	backend->SetStateCallback([this](AudioStateEvent event)
	{
		if (event == AudioStateEvent::DEFAULT_DEVICE_MAYBE_CHANGED)
			headset_device_changed = true;
	});
	backend->SetWriteCallback([this](u32 bytes, void* buffer) { return headset_callback(bytes, buffer); });

	const AudioSampleSize sample_size = convert_to_s16 ? AudioSampleSize::S16 : AudioSampleSize::FLOAT;

	if (!backend->Open(device, AudioFreq::FREQ_48K, sample_size, AudioChannelCnt::STEREO, audio_channel_layout::stereo))
	{
		if (!headset_failed)
			cellVoice.error("Failed to open the audio backend for the voice chat (OUT_SECONDARY)");
		headset_failed = true;
		headset_retry_ticks = 1'000'000 / tick_usec;
		backend->SetStateCallback({});
		backend->Close();
		return;
	}

	{
		std::lock_guard lock(headset_mtx);
		headset_channels = std::max<u32>(backend->get_channels(), 1);
		headset_s16 = backend->get_sample_size() == sizeof(s16);
		headset_step = static_cast<f64>(voice_rate) / backend->get_sampling_rate();
		headset_fifo.clear();
		headset_primed = false;
		headset_pos = 1.0;
		headset_prev = headset_cur = 0.0f;
	}

	backend->Play();
	headset = std::move(backend);
	headset_failed = false;
	cellVoice.notice("Voice chat output (OUT_SECONDARY): %s, device '%s', %d Hz, %d channel(s)", headset->GetName(), device, headset->get_sampling_rate(), headset->get_channels());
}

void voice_service::close_headset()
{
	if (headset)
	{
		headset->SetStateCallback({});
		headset->Close();
		headset.reset();
	}

	headset_device_changed = false;
	headset_retry_ticks = 0;
	std::lock_guard lock(headset_mtx);
	headset_fifo.clear();
	headset_primed = false;
	headset_pos = 1.0;
	headset_prev = headset_cur = 0.0f;
}

void voice_service::play_headset(const f32* mix)
{
	if (!headset)
	{
		return;
	}

	std::lock_guard lock(headset_mtx);
	headset_volume = static_cast<f32>(g_cfg.audio.volume.get()) / 100.0f;
	headset_fifo.insert(headset_fifo.end(), mix, mix + tick_samples);

	if (headset_fifo.size() > max_headset_samples)
	{
		headset_fifo.erase(headset_fifo.begin(), headset_fifo.end() - headset_prebuffer_samples);
	}
}

u32 voice_service::headset_callback(u32 bytes, void* buffer)
{
	std::lock_guard lock(headset_mtx);

	const u32 sample_size = headset_s16 ? 2 : 4;
	const u32 frames = bytes / (sample_size * headset_channels);

	if (!headset_primed && headset_fifo.size() >= headset_prebuffer_samples)
	{
		headset_primed = true;
	}

	for (u32 i = 0; i < frames; i++)
	{
		// From 16 kHz (linear interpolation)
		while (headset_pos >= 1.0)
		{
			headset_prev = headset_cur;
			headset_cur = 0.0f;

			if (headset_primed && !headset_fifo.empty())
			{
				headset_cur = headset_fifo.front();
				headset_fifo.pop_front();
			}
			else
			{
				headset_primed = false;
			}

			headset_pos -= 1.0;
		}

		const f32 v = std::clamp((headset_prev + (headset_cur - headset_prev) * static_cast<f32>(headset_pos)) / 32768.0f * headset_volume, -1.0f, 1.0f);
		headset_pos += headset_step;

		for (u32 c = 0; c < headset_channels; c++)
		{
			if (headset_s16) static_cast<s16*>(buffer)[i * headset_channels + c] = static_cast<s16>(v * 32767.0f);
			else static_cast<f32*>(buffer)[i * headset_channels + c] = v;
		}
	}

	std::memset(static_cast<u8*>(buffer) + frames * sample_size * headset_channels, 0, bytes - frames * sample_size * headset_channels);
	return bytes;
}

voice_manager::voice_manager() = default;

voice_manager::voice_manager(utils::serial& ar)
{
	save(ar);
}

voice_manager::~voice_manager() = default;

voice_service& voice_manager::service()
{
	if (!m_service)
	{
		m_service = std::make_unique<voice_service>(*this);
	}

	return *m_service;
}

void voice_manager::notify(u64 event, u64 data2)
{
	for (auto& key_pair : queue_keys)
	{
		if (auto queue = lv2_event_queue::find(key_pair.first))
		{
			for (const auto& source : key_pair.second)
			{
				queue->send(source, event, data2, 0);
			}
		}
	}
}

void voice_manager::reset()
{
	id_ctr = 0;
	port_source = 0;
	ports.clear();
	queue_keys.clear();

	if (m_service)
	{
		m_service->clear();
	}
}

void voice_manager::save(utils::serial& ar)
{
	const s32 version = GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellVoice);
	ar(id_ctr, port_source, ports, queue_keys, voice_service_started);
	if (ar.is_writing() || version >= 2)
	{
		bool initialized = is_init;
		bool has_service = !!m_service;
		ar(initialized, run_state, service_generation, worker_id, stop_caller, last_error, has_service);
		if (!ar.is_writing())
		{
			is_init = initialized;
			if (has_service)
				m_service = std::make_unique<voice_service>(*this, false);
		}
		if (has_service)
			m_service->save(ar);
	}
}

void voice_service::save(utils::serial& ar)
{
	// Saves occur between bounded firmware operations. Guest memory, LV2 resources,
	// and the PPU worker are saved by their owners; keep the addresses that own them.
	ensure(requests.empty());
	std::vector<u16> ids;
	std::vector<u32> connections;
	if (ar.is_writing())
	{
		for (const auto& [id, d] : data)
			ids.push_back(id);
		std::sort(ids.begin(), ids.end());
		for (const auto& [input, output] : edges)
			connections.push_back((u32{input} << 16) | output);
	}
	ar(mute_all, next_codec_generation, ids, connections);
	ensure(ids.size() <= CELLVOICE_MAX_PORT && connections.size() <= CELLVOICE_MAX_PORT * CELLVOICE_MAX_PORT);
	if (!ar.is_writing())
	{
		data.clear();
		edges.clear();
		for (const u32 connection : connections)
			edges.emplace(static_cast<u16>(connection >> 16), static_cast<u16>(connection));
	}
	for (const u16 id : ids)
	{
		auto& d = data[id];
		// Saves occur after the whole batch completes. Preserve the version-2
		// boolean field while the live counter tracks multiple pending frames.
		bool pending = d.codec_pending != 0;
		ar(d.bytes, d.pcm, d.signal, d.codec_bitrate, d.codec_generation, d.codec_encode,
			pending, d.codec_failed, d.filter.x, d.playing, d.waited, d.hangover,
			d.noise_db, d.attached, d.paused, d.logged, d.read_logged);
		if (GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellVoice) >= 4)
			ar(d.energy_enabled, d.energy_level, d.vad_threshold, d.vad_activity, d.mic_noise_db);
		ensure(!pending);
		d.codec_pending = 0;
	}
	if (!ar.is_writing() && GET_OR_USE_SERIALIZATION_VERSION(false, cellVoice) < 5)
	{
		// Discard the optional synthetic source stored by older development builds.
		bool loaded = false;
		std::vector<f32> samples;
		usz position = 0;
		ar(loaded, samples, position);
	}
	ar(mic_fifo, mic_primed, mic_resample_pos, mic_last);
	{
		std::lock_guard lock(headset_mtx);
		ar(headset_fifo, headset_primed, headset_pos, headset_step, headset_prev, headset_cur, headset_volume);
	}
	firmware->save(ar, GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellVoice));
}

error_code cellVoiceConnectIPortToOPort(u32 ips, u32 ops)
{
	cellVoice.notice("cellVoiceConnectIPortToOPort(ips=0x%x, ops=0x%x)", ips, ops);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return CELL_VOICE_ERROR_TOPOLOGY;

	auto oport = manager.access_port(ops);

	if (!oport || oport->info.portType <= CELLVOICE_PORTTYPE_IN_VOICE)
		return CELL_VOICE_ERROR_TOPOLOGY;

	manager.service().edges.emplace(static_cast<u16>(ips), static_cast<u16>(ops));
	return CELL_OK;
}

error_code cellVoiceCreateNotifyEventQueue(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<u64> key)
{
	cellVoice.warning("cellVoiceCreateNotifyEventQueue(id=*0x%x, key=*0x%x)", id, key);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	vm::var<sys_event_queue_attribute_t> attr;
	attr->protocol = SYS_SYNC_FIFO;
	attr->type = SYS_PPU_QUEUE;
	attr->name_u64 = 0;

	for (u64 i = 0; i < 10; i++)
	{
		// Create an event queue "bruteforcing" an available key
		const u64 key_value = 0x80004d494f323285ull + i;
		if (CellError res{sys_event_queue_create(ppu, id, attr, key_value, 0x40) + 0u})
		{
			if (res != CELL_EEXIST)
			{
				return res;
			}
		}
		else
		{
			*key = key_value;
			return CELL_OK;
		}
	}

	return CELL_VOICE_ERROR_EVENT_QUEUE;
}

error_code cellVoiceCreatePort(vm::ptr<u32> portId, vm::cptr<CellVoicePortParam> pArg)
{
	cellVoice.warning("cellVoiceCreatePort(portId=*0x%x, pArg=*0x%x)", portId, pArg);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	switch (pArg->portType)
	{
	case CELLVOICE_PORTTYPE_IN_PCMAUDIO:
	case CELLVOICE_PORTTYPE_OUT_PCMAUDIO:
	{
		if (pArg->pcmaudio.format.dataType > CELLVOICE_PCM_INTEGER_LITTLE_ENDIAN)
			return CELL_VOICE_ERROR_ARGUMENT_INVALID;

		break;
	}
	case CELLVOICE_PORTTYPE_IN_VOICE:
	case CELLVOICE_PORTTYPE_OUT_VOICE:
	{
		// Must be an exact value
		switch (pArg->voice.bitrate)
		{
		case CELLVOICE_BITRATE_3850:
		case CELLVOICE_BITRATE_4650:
		case CELLVOICE_BITRATE_5700:
		case CELLVOICE_BITRATE_7300:
		case CELLVOICE_BITRATE_14400:
		case CELLVOICE_BITRATE_16000:
		case CELLVOICE_BITRATE_22533:
			break;
		default:
		{
			return CELL_VOICE_ERROR_ARGUMENT_INVALID;
		}
		}
		break;
	}
	case CELLVOICE_PORTTYPE_IN_MIC:
	case CELLVOICE_PORTTYPE_OUT_SECONDARY:
	{
		break;
	}
	default:
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	if (manager.ports.size() >= CELLVOICE_MAX_PORT)
		return CELL_VOICE_ERROR_RESOURCE_INSUFFICIENT;

	// Id: bits [8,15] seem to contain a "random" value
	// bits [0,7] are based on creation counter modulo 0xa0
	// The rest are set to zero and ignored.
	manager.id_ctr++; manager.id_ctr %= 0xa0;

	// It isn't known whether bits[8,15] are guaranteed to be non-zero
	constexpr u32 min_value = 1;

	for (u32 ctr2 = min_value; ctr2 < CELLVOICE_MAX_PORT + min_value; ctr2++)
	{
		const auto [port, success] = manager.ports.try_emplace(static_cast<u16>((ctr2 << 8) | manager.id_ctr));

		if (success)
		{
			port->second.info = *pArg;
			*portId = port->first;

			if (manager.m_service)
			{
				manager.m_service->remove_port(port->first);
			}

			const u32 type = pArg->portType;
			cellVoice.notice("cellVoiceCreatePort(): port 0x%x, type %d, bit rate %d", port->first, type,
				type == CELLVOICE_PORTTYPE_IN_VOICE || type == CELLVOICE_PORTTYPE_OUT_VOICE ? static_cast<u32>(pArg->voice.bitrate) : 0u);
			return CELL_OK;
		}
	}

	fmt::throw_exception("Unreachable");
}

error_code cellVoiceDeletePort(u32 portId)
{
	cellVoice.warning("cellVoiceDeletePort(portId=0x%x)", portId);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (manager.ports.erase(static_cast<u16>(portId)) == 0)
		return CELL_VOICE_ERROR_TOPOLOGY;

	if (manager.m_service)
	{
		manager.m_service->remove_port(static_cast<u16>(portId));
	}

	return CELL_OK;
}

error_code cellVoiceDisconnectIPortFromOPort(u32 ips, u32 ops)
{
	cellVoice.notice("cellVoiceDisconnectIPortFromOPort(ips=0x%x, ops=0x%x)", ips, ops);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return CELL_VOICE_ERROR_TOPOLOGY;

	auto oport = manager.access_port(ops);

	if (!oport || oport->info.portType <= CELLVOICE_PORTTYPE_IN_VOICE)
		return CELL_VOICE_ERROR_TOPOLOGY;

	manager.service().edges.erase({static_cast<u16>(ips), static_cast<u16>(ops)});
	return CELL_OK;
}

static error_code VoiceStop(ppu_thread& ppu, bool ending);

error_code cellVoiceEnd(ppu_thread& ppu)
{
	cellVoice.warning("cellVoiceEnd()");
	return VoiceStop(ppu, true);
}

error_code cellVoiceGetBitRate(u32 portId, vm::ptr<u32> bitrate)
{
	cellVoice.warning("cellVoiceGetBitRate(portId=0x%x, bitrate=*0x%x)", portId, bitrate);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	// No nullptr check!

	// Constant value for errors (meaning unknown)
	*bitrate = 0x4f323285;

	auto port = manager.access_port(portId);

	if (!port || (port->info.portType != CELLVOICE_PORTTYPE_IN_VOICE && port->info.portType != CELLVOICE_PORTTYPE_OUT_VOICE))
		return CELL_VOICE_ERROR_TOPOLOGY;

	*bitrate = port->info.voice.bitrate;
	return CELL_OK;
}

error_code cellVoiceGetMuteFlag(u32 portId, vm::ptr<u16> bMuted)
{
	cellVoice.warning("cellVoiceGetMuteFlag(portId=0x%x, bMuted=*0x%x)", portId, bMuted);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	*bMuted = port->info.bMute;
	return CELL_OK;
}

error_code cellVoiceGetPortAttr(u32 portId, u32 attr, vm::ptr<void> attrValue)
{
	cellVoice.trace("cellVoiceGetPortAttr(portId=0x%x, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	auto& manager = g_fxo->get<voice_manager>();
	std::scoped_lock lock(manager.mtx);
	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	const auto port = manager.access_port(portId);
	if (!port)
		return CELL_VOICE_ERROR_PORT_INVALID;
	if (!attrValue)
		return CELL_VOICE_ERROR_ADDRESS_INVALID;

	if (attr != CELLVOICE_ATTR_ENERGY_LEVEL && attr != CELLVOICE_ATTR_VAD)
		cellVoice.todo("cellVoiceGetPortAttr: attribute %d is not implemented", attr);

	u32 value = 0;
	const u32 error = manager.service().get_port_attr(*port, static_cast<u16>(portId), attr, value);
	if (error)
		return error;
	*vm::static_ptr_cast<u32>(attrValue) = value;
	return CELL_OK;
}

error_code cellVoiceGetPortInfo(u32 portId, vm::ptr<CellVoiceBasePortInfo> pInfo)
{
	cellVoice.trace("cellVoiceGetPortInfo(portId=0x%x, pInfo=*0x%x)", portId, pInfo);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	if (!manager.voice_service_started)
		return CELL_VOICE_ERROR_SERVICE_DETACHED;

	// No nullptr check!
	pInfo->portType = port->info.portType;
	pInfo->state = CELLVOICE_PORTSTATE_RUNNING;
	pInfo->numByte = manager.m_service ? manager.m_service->bytes_queued(static_cast<u16>(portId)) : 0;

	if (port->info.portType == CELLVOICE_PORTTYPE_IN_VOICE || port->info.portType == CELLVOICE_PORTTYPE_OUT_VOICE)
	{
		pInfo->frameSize = voice_frame_bytes(port->info.voice.bitrate);
	}
	else if (port->info.portType == CELLVOICE_PORTTYPE_IN_PCMAUDIO || port->info.portType == CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
	{
		pInfo->frameSize = pcm_sample_size(port->info.pcmaudio.format.dataType) * std::max<u32>(port->info.pcmaudio.format.numChannels, 1);
	}

	// Temporary diagnostics: sample each port separately so outgoing polling cannot hide incoming queries.
	static std::array<atomic_t<u64>, 256> query_counts{};
	const u64 query = ++query_counts[portId % query_counts.size()];
	if (query <= 8 || query % 256 == 0)
	{
		const u32 type = port->info.portType;
		const u32 frame_size = type == CELLVOICE_PORTTYPE_IN_MIC || type == CELLVOICE_PORTTYPE_OUT_SECONDARY ? 0 : pInfo->frameSize;
		cellVoice.notice("VOICE_RX cellVoiceGetPortInfo query=%u port=0x%x type=%u numByte=%u frameSize=%u state=%u result=0x0",
			query, portId, type, static_cast<u32>(pInfo->numByte), frame_size, static_cast<u32>(pInfo->state));
	}

	// TODO: edges

	return CELL_OK;
}

error_code cellVoiceGetSignalState(u32 portId, u32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceGetSignalState(portId=0x%x, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	// Report detached microphone
	return not_an_error(CELL_VOICE_ERROR_DEVICE_NOT_PRESENT);
}

error_code cellVoiceGetVolume(u32 portId, vm::ptr<f32> volume)
{
	cellVoice.warning("cellVoiceGetVolume(portId=0x%x, volume=*0x%x)", portId, volume);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	// No nullptr check!

	// Constant value for errors (meaning unknown)
	*volume = std::bit_cast<f32, s32>(0x4f323285);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	*volume = port->info.volume;
	return CELL_OK;
}

error_code cellVoiceInit(vm::ptr<CellVoiceInitParam> pArg)
{
	cellVoice.warning("cellVoiceInit(pArg=*0x%x)", pArg);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_INITIALIZED;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	manager.service();
	manager.is_init = true;

	return CELL_OK;
}

error_code cellVoiceInitEx(vm::ptr<CellVoiceInitParam> pArg)
{
	cellVoice.warning("cellVoiceInitEx(pArg=*0x%x)", pArg);

	auto& manager = g_fxo->get<voice_manager>();

	std::lock_guard lock(manager.mtx);

	if (manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_INITIALIZED;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	manager.service();
	manager.is_init = true;

	return CELL_OK;
}

error_code cellVoicePausePort(u32 portId)
{
	cellVoice.notice("cellVoicePausePort(portId=0x%x)", portId);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	manager.service().data[static_cast<u16>(portId)].paused = true;
	return CELL_OK;
}

error_code cellVoicePausePortAll()
{
	cellVoice.notice("cellVoicePausePortAll()");

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	for (const auto& [id, port] : manager.ports)
	{
		manager.service().data[id].paused = true;
	}

	return CELL_OK;
}

error_code cellVoiceRemoveNotifyEventQueue(u64 key)
{
	cellVoice.warning("cellVoiceRemoveNotifyEventQueue(key=0x%llx)", key);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (manager.queue_keys.erase(key) == 0)
		return CELL_VOICE_ERROR_EVENT_QUEUE;

	return CELL_OK;
}

error_code cellVoiceResetPort(u32 portId)
{
	cellVoice.notice("cellVoiceResetPort(portId=0x%x)", portId);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	// Its data and codec state
	auto& data = manager.service().data;

	if (const auto it = data.find(static_cast<u16>(portId)); it != data.end())
	{
		const bool paused = it->second.paused;
		const bool attached = it->second.attached;
		const bool energy_enabled = it->second.energy_enabled;
		const f32 energy_level = it->second.energy_level;
		const u32 vad_threshold = it->second.vad_threshold;
		it->second = {};
		it->second.energy_enabled = energy_enabled;
		it->second.energy_level = energy_level;
		it->second.vad_threshold = vad_threshold;
		it->second.paused = paused;
		it->second.attached = attached;
	}

	return CELL_OK;
}

error_code cellVoiceResumePort(u32 portId)
{
	cellVoice.notice("cellVoiceResumePort(portId=0x%x)", portId);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	manager.service().data[static_cast<u16>(portId)].paused = false;
	return CELL_OK;
}

error_code cellVoiceResumePortAll()
{
	cellVoice.notice("cellVoiceResumePortAll()");

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	for (auto& [id, d] : manager.service().data)
	{
		d.paused = false;
	}

	return CELL_OK;
}

error_code cellVoiceSetBitRate(u32 portId, CellVoiceBitRate bitrate)
{
	cellVoice.warning("cellVoiceSetBitRate(portId=0x%x, bitrate=%d)", portId, +bitrate);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port || (port->info.portType != CELLVOICE_PORTTYPE_IN_VOICE && port->info.portType != CELLVOICE_PORTTYPE_OUT_VOICE))
		return CELL_VOICE_ERROR_TOPOLOGY;

	// TODO: Check ordering of checks.
	switch (bitrate)
	{
	case CELLVOICE_BITRATE_3850:
	case CELLVOICE_BITRATE_4650:
	case CELLVOICE_BITRATE_5700:
	case CELLVOICE_BITRATE_7300:
	case CELLVOICE_BITRATE_14400:
	case CELLVOICE_BITRATE_16000:
	case CELLVOICE_BITRATE_22533:
		break;
	default:
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}
	}

	port->info.voice.bitrate = bitrate;
	return CELL_OK;
}

error_code cellVoiceSetMuteFlag(u32 portId, u16 bMuted)
{
	cellVoice.warning("cellVoiceSetMuteFlag(portId=0x%x, bMuted=%d)", portId, bMuted);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	port->info.bMute = bMuted;
	return CELL_OK;
}

error_code cellVoiceSetMuteFlagAll(u16 bMuted)
{
	cellVoice.warning("cellVoiceSetMuteFlagAll(bMuted=%d)", bMuted);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	// Doesn't change port->bMute value
	manager.service().mute_all = bMuted != 0;
	return CELL_OK;
}

error_code cellVoiceSetNotifyEventQueue(u64 key, u64 source)
{
	cellVoice.warning("cellVoiceSetNotifyEventQueue(key=0x%llx, source=0x%llx)", key, source);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	// Note: it is allowed to enqueue the key twice (another source is enqueued with FIFO ordering)
	// It is not allowed to enqueue an invalid key

	if (!lv2_event_queue::find(key))
		return CELL_VOICE_ERROR_EVENT_QUEUE;

	if (!source)
	{
		// same thing as sys_event_port_send with port.name == 0
		// Try to give different port id everytime
		source = ((process_getpid() + 1ull) << 32) | (lv2_event_port::id_base + manager.port_source * lv2_event_port::id_step);
		manager.port_source = (manager.port_source + 1) % lv2_event_port::id_count;
	}

	manager.queue_keys[key].push_back(source);
	return CELL_OK;
}

error_code cellVoiceSetPortAttr(u32 portId, u32 attr, vm::ptr<void> attrValue)
{
	cellVoice.trace("cellVoiceSetPortAttr(portId=0x%x, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	auto& manager = g_fxo->get<voice_manager>();
	std::scoped_lock lock(manager.mtx);
	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	const auto port = manager.access_port(portId);
	if (!port)
		return CELL_VOICE_ERROR_PORT_INVALID;
	if (!attrValue)
		return CELL_VOICE_ERROR_ADDRESS_INVALID;

	if (attr != CELLVOICE_ATTR_ENERGY_LEVEL && attr != CELLVOICE_ATTR_VAD)
		cellVoice.todo("cellVoiceSetPortAttr: attribute %d is not implemented", attr);

	return manager.service().set_port_attr(*port, static_cast<u16>(portId), attr, *vm::static_ptr_cast<u32>(attrValue));
}

error_code cellVoiceSetVolume(u32 portId, f32 volume)
{
	cellVoice.warning("cellVoiceSetVolume(portId=0x%x, volume=%f)", portId, volume);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	port->info.volume = volume;
	return CELL_OK;
}

static error_code VoiceStart(ppu_thread& ppu)
{
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};
	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return CELL_OK;
	}
	auto& manager = g_fxo->get<voice_manager>();
	voice_service* service;
	u64 generation;
	{
		std::lock_guard lock(manager.mtx);
		if (!manager.is_init)
			return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
		if (manager.run_state == voice_run_state::running)
			return CELL_OK;
		if (manager.run_state != voice_run_state::stopped)
			return CELL_VOICE_ERROR_SERVICE_DETACHED;
		manager.run_state = voice_run_state::starting;
		manager.last_error = 0;
		generation = ++manager.service_generation;
		service = &manager.service();
	}

	s32 rc = service->firmware->start(ppu);
	if (ppu.test_stopped())
		return CELL_OK;
	ppu.state += cpu_flag::wait;
	vm::var<u64> thread_id;
	if (!rc)
	{
		const auto name = vm::make_str("cellVoice firmware worker");
		const auto entry = g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(voiceWorkerEntry));
		rc = ppu_execute<&sys_ppu_thread_create>(ppu, +thread_id, entry, generation, 1000, 0x10000, SYS_PPU_THREAD_CREATE_JOINABLE, +name);
	}
	if (ppu.test_stopped())
		return CELL_OK;
	const s32 cleanup_rc = rc ? service->firmware->stop(ppu) : CELL_OK;
	std::lock_guard lock(manager.mtx);
	if (rc)
	{
		manager.last_error = rc;
		manager.run_state = cleanup_rc ? voice_run_state::failed : voice_run_state::stopped;
		cellVoice.error("Firmware voice service could not start: 0x%x", rc);
		return CELL_VOICE_ERROR_SERVICE_NOT_FOUND;
	}
	manager.worker_id = static_cast<u32>(+*thread_id);
	manager.run_state = voice_run_state::running;
	manager.voice_service_started = true;
	manager.notify(CELLVOICE_EVENT_SERVICE_ATTACHED);
	return CELL_OK;
}

error_code cellVoiceStart(ppu_thread& ppu)
{
	cellVoice.warning("cellVoiceStart()");
	cellVoice.notice("VOICE_RX diagnostics enabled: WriteToIPort/Ex/Ex2 entry and return, first 8 and every 256 calls; extra is numFrameLost for Ex, frameGaps for Ex2");
	return VoiceStart(ppu);
}

error_code cellVoiceStartEx(ppu_thread& ppu, vm::ptr<CellVoiceStartParam> pArg)
{
	cellVoice.todo("cellVoiceStartEx(pArg=*0x%x)", pArg);

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	// TODO: Check provided memory container

	return VoiceStart(ppu);
}

static error_code VoiceStop(ppu_thread& ppu, bool ending)
{
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};
	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return CELL_OK;
	}
	auto& manager = g_fxo->get<voice_manager>();
	u32 worker_id;
	{
		std::lock_guard lock(manager.mtx);
		if (!manager.is_init)
			return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
		if (manager.run_state == voice_run_state::starting || manager.run_state == voice_run_state::stopping)
			return CELL_VOICE_ERROR_SERVICE_ATTACHED;
		manager.run_state = voice_run_state::stopping;
		manager.stop_caller = ppu.id;
		worker_id = manager.worker_id;
		if (manager.m_service)
			manager.m_service->stop(manager);
		if (std::exchange(manager.voice_service_started, false))
			manager.notify(CELLVOICE_EVENT_SERVICE_DETACHED);
	}
	if (worker_id)
	{
		ppu.state += cpu_flag::wait;
		vm::var<u64> status;
		const s32 rc = sys_ppu_thread_join(ppu, worker_id, +status);
		if (ppu.test_stopped())
			return CELL_OK;
		if (rc)
		{
			std::lock_guard lock(manager.mtx);
			manager.last_error = rc;
			manager.run_state = voice_run_state::failed;
			return CELL_VOICE_ERROR_GENERAL;
		}
	}
	else if (manager.m_service)
	{
		// Retry cleanup after a failed start while its ownership records remain intact.
		const s32 rc = manager.m_service->firmware->stop(ppu);
		if (ppu.test_stopped())
			return CELL_OK;
		std::lock_guard lock(manager.mtx);
		manager.last_error = rc;
	}
	std::lock_guard lock(manager.mtx);
	manager.worker_id = 0;
	manager.stop_caller = 0;
	manager.run_state = manager.last_error ? voice_run_state::failed : voice_run_state::stopped;
	if (manager.last_error)
		return CELL_VOICE_ERROR_GENERAL;
	if (ending)
	{
		manager.reset();
		manager.is_init = false;
	}
	return CELL_OK;
}

error_code cellVoiceStop(ppu_thread& ppu)
{
	cellVoice.warning("cellVoiceStop()");
	return VoiceStop(ppu, false);
}

error_code cellVoiceUpdatePort(u32 portId, vm::cptr<CellVoicePortParam> pArg)
{
	cellVoice.warning("cellVoiceUpdatePort(portId=0x%x, pArg=*0x%x)", portId, pArg);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	// Not all info is updated
	port->info.bMute = pArg->bMute;
	port->info.volume = pArg->volume;
	port->info.threshold = pArg->threshold;

	if (port->info.portType == CELLVOICE_PORTTYPE_IN_VOICE || port->info.portType == CELLVOICE_PORTTYPE_OUT_VOICE)
	{
		port->info.voice.bitrate = pArg->voice.bitrate;
	}

	return CELL_OK;
}

error_code cellVoiceWriteToIPort(u32 ips, vm::cptr<void> data, vm::ptr<u32> size)
{
	// Spammy
	cellVoice.trace("cellVoiceWriteToIPort(ips=0x%x, data=*0x%x, size=*0x%x)", ips, data, size);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	// Temporary receive diagnostics; counters are protected by manager.mtx.
	static u64 calls = 0, failures = 0;
	const u64 call = ++calls;
	const bool sampled = call <= 8 || call % 256 == 0;
	u32 requested = 0, accepted = 0;
	bool size_read = false;
	if (sampled)
		cellVoice.notice("VOICE_RX cellVoiceWriteToIPort call=%u ENTER port=0x%x data=0x%x size_ptr=0x%x extra=%d", call, ips, data, size, 0);
	const auto report = [&](error_code result) -> error_code
	{
		const bool failed = static_cast<s32>(result) != 0;
		if (failed) failures++;
		if (sampled || (failed && (failures <= 8 || failures % 256 == 0)))
			cellVoice.notice("VOICE_RX cellVoiceWriteToIPort call=%u RETURN port=0x%x data=0x%x size_ptr=0x%x extra=%d size_read=%d requested=%u accepted=%u result=0x%x",
				call, ips, data, size, 0, size_read, requested, accepted, static_cast<u32>(static_cast<s32>(result)));
		return result;
	};

	if (!manager.is_init)
		return report(CELL_VOICE_ERROR_LIBVOICE_NOT_INIT);

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return report(CELL_VOICE_ERROR_TOPOLOGY);

	if (!size)
		return report(CELL_VOICE_ERROR_ADDRESS_INVALID);

	requested = *size;
	size_read = true;
	if (requested && !data)
		return report(CELL_VOICE_ERROR_ADDRESS_INVALID);

	accepted = manager.service().write(*iport, static_cast<u16>(ips), static_cast<const u8*>(data.get_ptr()), requested);
	*size = accepted;
	return report(CELL_OK);
}

error_code cellVoiceWriteToIPortEx(u32 ips, vm::cptr<void> data, vm::ptr<u32> size, u32 numFrameLost)
{
	cellVoice.trace("cellVoiceWriteToIPortEx(ips=0x%x, data=*0x%x, size=*0x%x, numFrameLost=%d)", ips, data, size, numFrameLost);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	// Temporary receive diagnostics; counters are protected by manager.mtx.
	static u64 calls = 0, failures = 0;
	const u64 call = ++calls;
	const bool sampled = call <= 8 || call % 256 == 0;
	u32 requested = 0, accepted = 0;
	bool size_read = false;
	if (sampled)
		cellVoice.notice("VOICE_RX cellVoiceWriteToIPortEx call=%u ENTER port=0x%x data=0x%x size_ptr=0x%x extra=%d", call, ips, data, size, numFrameLost);
	const auto report = [&](error_code result) -> error_code
	{
		const bool failed = static_cast<s32>(result) != 0;
		if (failed) failures++;
		if (sampled || (failed && (failures <= 8 || failures % 256 == 0)))
			cellVoice.notice("VOICE_RX cellVoiceWriteToIPortEx call=%u RETURN port=0x%x data=0x%x size_ptr=0x%x extra=%d size_read=%d requested=%u accepted=%u result=0x%x",
				call, ips, data, size, numFrameLost, size_read, requested, accepted, static_cast<u32>(static_cast<s32>(result)));
		return result;
	};

	if (!manager.is_init)
		return report(CELL_VOICE_ERROR_LIBVOICE_NOT_INIT);

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return report(CELL_VOICE_ERROR_TOPOLOGY);

	if (!size)
		return report(CELL_VOICE_ERROR_ADDRESS_INVALID);

	requested = *size;
	size_read = true;
	if (requested && !data)
		return report(CELL_VOICE_ERROR_ADDRESS_INVALID);

	// TODO: conceal the lost frames
	accepted = manager.service().write(*iport, static_cast<u16>(ips), static_cast<const u8*>(data.get_ptr()), requested);
	*size = accepted;
	return report(CELL_OK);
}

error_code cellVoiceWriteToIPortEx2(u32 ips, vm::cptr<void> data, vm::ptr<u32> size, s16 frameGaps)
{
	cellVoice.trace("cellVoiceWriteToIPortEx2(ips=0x%x, data=*0x%x, size=*0x%x, frameGaps=%d)", ips, data, size, frameGaps);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	// Temporary receive diagnostics; counters are protected by manager.mtx.
	static u64 calls = 0, failures = 0;
	const u64 call = ++calls;
	const bool sampled = call <= 8 || call % 256 == 0;
	u32 requested = 0, accepted = 0;
	bool size_read = false;
	if (sampled)
		cellVoice.notice("VOICE_RX cellVoiceWriteToIPortEx2 call=%u ENTER port=0x%x data=0x%x size_ptr=0x%x extra=%d", call, ips, data, size, frameGaps);
	const auto report = [&](error_code result) -> error_code
	{
		const bool failed = static_cast<s32>(result) != 0;
		if (failed) failures++;
		if (sampled || (failed && (failures <= 8 || failures % 256 == 0)))
			cellVoice.notice("VOICE_RX cellVoiceWriteToIPortEx2 call=%u RETURN port=0x%x data=0x%x size_ptr=0x%x extra=%d size_read=%d requested=%u accepted=%u result=0x%x",
				call, ips, data, size, frameGaps, size_read, requested, accepted, static_cast<u32>(static_cast<s32>(result)));
		return result;
	};

	if (!manager.is_init)
		return report(CELL_VOICE_ERROR_LIBVOICE_NOT_INIT);

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return report(CELL_VOICE_ERROR_TOPOLOGY);

	if (!size)
		return report(CELL_VOICE_ERROR_ADDRESS_INVALID);

	requested = *size;
	size_read = true;
	if (requested && !data)
		return report(CELL_VOICE_ERROR_ADDRESS_INVALID);

	// TODO: frame gaps
	accepted = manager.service().write(*iport, static_cast<u16>(ips), static_cast<const u8*>(data.get_ptr()), requested);
	*size = accepted;
	return report(CELL_OK);
}

error_code cellVoiceReadFromOPort(u32 ops, vm::ptr<void> data, vm::ptr<u32> size)
{
	// Spammy
	cellVoice.trace("cellVoiceReadFromOPort(ops=0x%x, data=*0x%x, size=*0x%x)", ops, data, size);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto oport = manager.access_port(ops);

	if (!oport || oport->info.portType <= CELLVOICE_PORTTYPE_IN_VOICE)
		return CELL_VOICE_ERROR_TOPOLOGY;

	if (!size)
		return CELL_OK;

	if (*size && !data)
		return CELL_VOICE_ERROR_ADDRESS_INVALID;

	*size = manager.service().read(manager, *oport, static_cast<u16>(ops), static_cast<u8*>(data.get_ptr()), *size);
	return CELL_OK;
}

error_code cellVoiceDebugTopology()
{
	UNIMPLEMENTED_FUNC(cellVoice);

	if (!g_fxo->get<voice_manager>().is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_VOICE_ERROR_NOT_IMPLEMENTED;
}

DECLARE(ppu_module_manager::cellVoice)("cellVoice", []()
{
	voice_firmware::backend_register();
	REG_HIDDEN_FUNC(voiceWorkerEntry);
	REG_FUNC(cellVoice, cellVoiceConnectIPortToOPort);
	REG_FUNC(cellVoice, cellVoiceCreateNotifyEventQueue);
	REG_FUNC(cellVoice, cellVoiceCreatePort);
	REG_FUNC(cellVoice, cellVoiceDeletePort);
	REG_FUNC(cellVoice, cellVoiceDisconnectIPortFromOPort);
	REG_FUNC(cellVoice, cellVoiceEnd);
	REG_FUNC(cellVoice, cellVoiceGetBitRate);
	REG_FUNC(cellVoice, cellVoiceGetMuteFlag);
	REG_FUNC(cellVoice, cellVoiceGetPortAttr);
	REG_FUNC(cellVoice, cellVoiceGetPortInfo);
	REG_FUNC(cellVoice, cellVoiceGetSignalState);
	REG_FUNC(cellVoice, cellVoiceGetVolume);
	REG_FUNC(cellVoice, cellVoiceInit);
	REG_FUNC(cellVoice, cellVoiceInitEx);
	REG_FUNC(cellVoice, cellVoicePausePort);
	REG_FUNC(cellVoice, cellVoicePausePortAll);
	REG_FUNC(cellVoice, cellVoiceRemoveNotifyEventQueue);
	REG_FUNC(cellVoice, cellVoiceResetPort);
	REG_FUNC(cellVoice, cellVoiceResumePort);
	REG_FUNC(cellVoice, cellVoiceResumePortAll);
	REG_FUNC(cellVoice, cellVoiceSetBitRate);
	REG_FUNC(cellVoice, cellVoiceSetMuteFlag);
	REG_FUNC(cellVoice, cellVoiceSetMuteFlagAll);
	REG_FUNC(cellVoice, cellVoiceSetNotifyEventQueue);
	REG_FUNC(cellVoice, cellVoiceSetPortAttr);
	REG_FUNC(cellVoice, cellVoiceSetVolume);
	REG_FUNC(cellVoice, cellVoiceStart);
	REG_FUNC(cellVoice, cellVoiceStartEx);
	REG_FUNC(cellVoice, cellVoiceStop);
	REG_FUNC(cellVoice, cellVoiceUpdatePort);
	REG_FUNC(cellVoice, cellVoiceWriteToIPort);
	REG_FUNC(cellVoice, cellVoiceWriteToIPortEx);
	REG_FUNC(cellVoice, cellVoiceWriteToIPortEx2);
	REG_FUNC(cellVoice, cellVoiceReadFromOPort);
	REG_FUNC(cellVoice, cellVoiceDebugTopology);
});
