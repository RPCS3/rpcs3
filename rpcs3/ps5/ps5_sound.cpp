// PS5: a sound file played once, on an audio port of its own (the launcher's
// intro); and the menus' sounds (RPCS3's overlay sounds: the launcher's, the
// pause menu's, RPCS3's own dialogs'), mixed on one port that stays open while
// they play.
//
// libSceAudioOut as ps5_audio_backend.cpp drives it: a main port for the
// system user, 256-frame grains at 48 kHz in signed 16-bit stereo,
// sceAudioOutOutput blocking for one grain. The console mixes the port with
// RPCS3's own.

#include "stdafx.h"
#include "Utilities/File.h"

#include "Utilities/Thread.h"

#include <condition_variable>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <thread>

LOG_CHANNEL(ps5_sound, "PS5Sound");

extern "C"
{
	int sceAudioOutInit(void);
	int sceAudioOutOpen(s32 user, s32 type, s32 index, u32 grain, u32 rate, u32 format);
	int sceAudioOutOutput(s32 port, const void* samples);
	int sceAudioOutClose(s32 port);
}

namespace
{
	constexpr u32 c_grain = 256;
	constexpr u32 c_rate = 48000;
	constexpr s32 c_system_user = 0xff;
	constexpr s32 c_port_main = 0;
	constexpr u32 c_format_s16_stereo = 1;
	constexpr int c_already_initialised = static_cast<int>(0x8026000e);

	// Raised by ps5_stop_sounds; each playing sound fades out over a few grains
	atomic_t<u32> g_stop_generation = 0;

	// The samples of a PCM WAV (16, 24 or 32-bit integers, or 32-bit floats;
	// one channel or more, the first two kept; any rate, made 48 kHz by
	// straight lines between samples), as 48 kHz 16-bit stereo frames. Menu
	// sounds put in rpcs3/sounds/ come as they come
	bool read_wav(const std::string& path, std::vector<s16>& frames)
	{
		fs::file file(path);
		if (!file)
		{
			ps5_sound.error("%s: not found", path);
			return false;
		}
		const std::vector<u8> data = file.to_vector<u8>();
		if (data.size() < 12 || std::memcmp(data.data(), "RIFF", 4) || std::memcmp(data.data() + 8, "WAVE", 4))
		{
			ps5_sound.error("%s: not a WAV file", path);
			return false;
		}

		u16 format = 0, channels = 0, bits = 0;
		u32 rate = 0;
		for (usz at = 12; at + 8 <= data.size();)
		{
			u32 size;
			std::memcpy(&size, data.data() + at + 4, 4);
			const u8* body = data.data() + at + 8;
			if (!std::memcmp(data.data() + at, "fmt ", 4) && size >= 16 && at + 8 + size <= data.size())
			{
				std::memcpy(&format, body, 2);
				std::memcpy(&channels, body + 2, 2);
				std::memcpy(&rate, body + 4, 4);
				std::memcpy(&bits, body + 14, 2);
				// WAVE_FORMAT_EXTENSIBLE: the real format is the subformat's first two bytes
				if (format == 0xfffe && size >= 26)
				{
					std::memcpy(&format, body + 24, 2);
				}
			}
			else if (!std::memcmp(data.data() + at, "data", 4))
			{
				const bool integer = format == 1 && (bits == 16 || bits == 24 || bits == 32);
				const bool floats = format == 3 && bits == 32;
				if ((!integer && !floats) || !channels || rate < 4000 || rate > 384000)
				{
					ps5_sound.error("%s: wants PCM, 16, 24 or 32-bit, or 32-bit float (format %u, %u channels, %u Hz, %u bits)", path, format, channels, rate, bits);
					return false;
				}
				// A data chunk longer than the file (some writers leave its size unset) ends with the file
				const usz bytes = std::min<usz>(size, data.size() - (at + 8));
				const usz step = usz{channels} * (bits / 8);
				const usz count = bytes / step;
				const auto sample = [&](usz frame, u32 channel) -> f32
				{
					const u8* p = body + frame * step + std::min<u32>(channel, channels - 1) * (bits / 8);
					if (floats)
					{
						f32 v;
						std::memcpy(&v, p, 4);
						return v;
					}
					if (bits == 16)
					{
						s16 v;
						std::memcpy(&v, p, 2);
						return v / 32768.f;
					}
					if (bits == 24)
					{
						const s32 v = static_cast<s32>(static_cast<u32>(p[0]) << 8 | static_cast<u32>(p[1]) << 16 | static_cast<u32>(p[2]) << 24) >> 8;
						return v / 8388608.f;
					}
					s32 v;
					std::memcpy(&v, p, 4);
					return v / 2147483648.f;
				};
				const auto to_s16 = [](f32 v) { return static_cast<s16>(std::clamp(v * 32767.f, -32768.f, 32767.f)); };
				const usz out = count ? (count - 1) * c_rate / rate + 1 : 0;
				frames.resize(out * 2);
				for (usz i = 0; i < out; i++)
				{
					const f64 from = static_cast<f64>(i) * rate / c_rate;
					const usz a = std::min<usz>(static_cast<usz>(from), count - 1);
					const usz b = std::min<usz>(a + 1, count - 1);
					const f32 w = static_cast<f32>(from - a);
					for (u32 c = 0; c < 2; c++)
					{
						frames[i * 2 + c] = to_s16(sample(a, c) * (1.f - w) + sample(b, c) * w);
					}
				}
				return true;
			}
			at += 8 + size + (size & 1);
		}

		ps5_sound.error("%s: no PCM data", path);
		return false;
	}
}

void ps5_play_sound_file(const std::string& path)
{
	std::vector<s16> frames;
	if (!read_wav(path, frames))
	{
		return;
	}

	std::thread([frames = std::move(frames), path, generation = g_stop_generation.load()]()
	{
		if (const int init = sceAudioOutInit(); init < 0 && init != c_already_initialised)
		{
			ps5_sound.error("sceAudioOutInit failed: 0x%08x", static_cast<u32>(init));
			return;
		}
		const s32 port = sceAudioOutOpen(c_system_user, c_port_main, 0, c_grain, c_rate, c_format_s16_stereo);
		if (port < 0)
		{
			ps5_sound.error("%s: sceAudioOutOpen failed: 0x%08x", path, static_cast<u32>(port));
			return;
		}

		// Stopped: fade over these grains, then end
		constexpr u32 fade_grains = 8;
		u32 fading = 0;
		alignas(64) s16 grain[c_grain * 2];
		const usz total = frames.size() / 2;
		for (usz at = 0; at < total && fading <= fade_grains; at += c_grain)
		{
			if (!fading && g_stop_generation != generation)
			{
				fading = 1;
			}
			const usz count = std::min<usz>(c_grain, total - at);
			std::memcpy(grain, frames.data() + at * 2, count * 4);
			std::memset(grain + count * 2, 0, (c_grain - count) * 4);
			if (fading)
			{
				const f32 from = 1.f - static_cast<f32>(fading - 1) / fade_grains;
				const f32 to = 1.f - static_cast<f32>(fading) / fade_grains;
				for (u32 i = 0; i < c_grain; i++)
				{
					const f32 gain = from + (to - from) * i / c_grain;
					grain[i * 2] = static_cast<s16>(grain[i * 2] * gain);
					grain[i * 2 + 1] = static_cast<s16>(grain[i * 2 + 1] * gain);
				}
				fading++;
			}
			if (sceAudioOutOutput(port, grain) < 0)
			{
				ps5_sound.error("%s: sceAudioOutOutput failed", path);
				break;
			}
		}

		// The grain still queued
		sceAudioOutOutput(port, nullptr);
		sceAudioOutClose(port);
	}).detach();
}

void ps5_stop_sounds()
{
	g_stop_generation++;
}

// The menus' sounds. RPCS3 names each by a file in its sounds folder
// (rsx::overlays::get_sound_filepath: rpcs3/sounds/snd_cursor.wav and so on),
// which on a PC the player fills from their own PS3; here a file put there is
// played, and otherwise the title's own of the same name
// (PS5_RPCS3Title's ps5/tools/menu-sounds.py). Each is read once (again when
// its file changes), and one thread mixes them on one port: a cursor held
// down starts a sound every few frames, and a port and a thread each
// (ps5_play_sound_file) would have run out of ports
namespace
{
	struct menu_voice
	{
		std::shared_ptr<const std::vector<s16>> frames;
		usz at = 0;
		f32 gain = 1.f;
	};

	std::mutex g_menu_mutex;
	std::condition_variable g_menu_wake;
	std::vector<menu_voice> g_menu_pending;
	std::map<std::string, std::pair<s64, std::shared_ptr<const std::vector<s16>>>> g_menu_cache; // by path: its time, its frames
	bool g_menu_mixer = false;

	[[noreturn]] void menu_mixer()
	{
		// Above the emulator's threads, as the game's sound (ps5_audio_backend.cpp)
		thread_ctrl::set_native_priority(1);

		s32 port = -1;
		u32 idle = 0;
		std::vector<menu_voice> playing;
		alignas(64) s16 grain[c_grain * 2];
		f32 mix[c_grain * 2];
		for (;;)
		{
			{
				std::unique_lock lock(g_menu_mutex);
				if (playing.empty() && port < 0)
				{
					g_menu_wake.wait(lock, [] { return !g_menu_pending.empty(); });
				}
				for (auto& voice : g_menu_pending)
				{
					playing.push_back(std::move(voice));
				}
				g_menu_pending.clear();
			}
			// Eight at once at most: the oldest go
			if (playing.size() > 8)
			{
				playing.erase(playing.begin(), playing.end() - 8);
			}

			if (port < 0)
			{
				if (const int init = sceAudioOutInit(); init < 0 && init != c_already_initialised)
				{
					ps5_sound.error("Menu sounds: sceAudioOutInit failed: 0x%08x", static_cast<u32>(init));
				}
				port = sceAudioOutOpen(c_system_user, c_port_main, 0, c_grain, c_rate, c_format_s16_stereo);
				if (port < 0)
				{
					ps5_sound.error("Menu sounds: sceAudioOutOpen failed: 0x%08x", static_cast<u32>(port));
					playing.clear();
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					continue;
				}
				idle = 0;
			}

			std::fill(std::begin(mix), std::end(mix), 0.f);
			for (menu_voice& voice : playing)
			{
				const usz total = voice.frames->size() / 2;
				const usz count = std::min<usz>(c_grain, total - voice.at);
				const s16* from = voice.frames->data() + voice.at * 2;
				for (usz i = 0; i < count * 2; i++)
				{
					mix[i] += from[i] * voice.gain;
				}
				voice.at += count;
			}
			std::erase_if(playing, [](const menu_voice& voice) { return voice.at * 2 >= voice.frames->size(); });
			for (u32 i = 0; i < c_grain * 2; i++)
			{
				grain[i] = static_cast<s16>(std::clamp(mix[i], -32768.f, 32767.f));
			}
			if (sceAudioOutOutput(port, grain) < 0)
			{
				ps5_sound.error("Menu sounds: sceAudioOutOutput failed");
			}

			// A second with nothing to play: the port closes until the next sound
			idle = playing.empty() ? idle + 1 : 0;
			if (idle >= c_rate / c_grain)
			{
				sceAudioOutOutput(port, nullptr);
				sceAudioOutClose(port);
				port = -1;
			}
		}
	}
}

void ps5_play_menu_sound(const std::string& path, f32 volume)
{
	if (volume <= 0.f)
	{
		return;
	}

	// The player's own file, else the title's of the same name
	std::string chosen = path;
	fs::stat_t stat{};
	if (!fs::get_stat(chosen, stat) || stat.is_directory)
	{
		chosen = "/app0/assets/launcher/sounds/" + path.substr(path.find_last_of('/') + 1);
		if (!fs::get_stat(chosen, stat) || stat.is_directory)
		{
			// Said once for each: the title's sounds not installed, and none of the player's
			static std::set<std::string> s_missing;
			std::lock_guard lock(g_menu_mutex);
			if (s_missing.insert(chosen).second)
			{
				ps5_sound.error("Menu sound: no %s, nor %s", path, chosen);
			}
			return;
		}
	}

	std::shared_ptr<const std::vector<s16>> frames;
	{
		std::lock_guard lock(g_menu_mutex);
		if (const auto found = g_menu_cache.find(chosen); found != g_menu_cache.end() && found->second.first == stat.mtime)
		{
			frames = found->second.second;
		}
	}
	if (!frames)
	{
		// A file that cannot be read is said once (until it changes), and stays silent
		auto read = std::make_shared<std::vector<s16>>();
		if (read_wav(chosen, *read))
		{
			ps5_sound.notice("Menu sound %s: %.2f s", chosen, read->size() / 2.0 / c_rate);
		}
		frames = std::move(read);
		std::lock_guard lock(g_menu_mutex);
		g_menu_cache[chosen] = {stat.mtime, frames};
	}
	if (frames->empty())
	{
		return;
	}

	std::lock_guard lock(g_menu_mutex);
	if (!g_menu_mixer)
	{
		std::thread(menu_mixer).detach();
		g_menu_mixer = true;
	}
	g_menu_pending.push_back({frames, 0, volume});
	g_menu_wake.notify_one();
}
