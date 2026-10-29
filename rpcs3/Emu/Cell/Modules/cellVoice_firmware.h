#pragma once

#include "util/types.hpp"

#include <memory>
#include <tuple>
#include <vector>

class ppu_thread;

namespace voice_firmware
{
	struct config
	{
		u32 bitrate = 0;
		u32 configuration = 0;
		u32 samples = 0;
		u32 bytes = 0;
	};

	const config* find_config(u32 bitrate);

	struct key
	{
		u16 port = 0;
		u64 generation = 0;
		bool encode = false;

		bool operator==(const key&) const = default;
		bool operator<(const key& other) const
		{
			return std::tie(port, generation, encode) < std::tie(other.port, other.generation, other.encode);
		}
	};

	struct request
	{
		key codec {};
		u32 bitrate = 0;
		std::vector<f32> pcm; // 8 kHz mono, s16 scale
		std::vector<u8> packet;
	};

	struct result
	{
		key codec {};
		u32 error = 0;
		std::vector<f32> pcm; // 8 kHz mono, s16 scale
		std::vector<u8> packet;
	};

	// Called by cellVoice's module registration, before any decoder is opened.
	void backend_register();

	// All calls run on the service's PPU, outside the voice manager lock. The
	// caller guards guest calls against savestate creation with hle_locks_t.
	class backend
	{
		struct impl;
		std::unique_ptr<impl> m_impl;

	public:
		backend();
		~backend(); // Host teardown only; normal shutdown must call stop().
		backend(const backend&) = delete;
		backend& operator=(const backend&) = delete;

		s32 start(ppu_thread& ppu);
		result process(ppu_thread& ppu, const request& input);
		s32 reconcile(ppu_thread& ppu, const std::vector<key>& live_keys);
		s32 stop(ppu_thread& ppu);
		void save(utils::serial& ar);
	};
}
