#pragma once

// PS5: the measurements of the trace's status lines, the latest five seconds'
// (ps5_frontend.cpp writes them), for the pause menu's Performance tab.

#include "util/types.hpp"

#include <string>
#include <utility>
#include <vector>

struct ps5_perf
{
	bool valid = false;
	u64 at_us = 0;          // when its five seconds ended (get_system_time)
	f32 game_fps = 0.f;     // the game's own frames a second (RSX's flips)
	f32 made_fps = 0.f;     // frames made between them a second (frame generation)
	f32 dropped_fps = 0.f;  // and made but dropped
	f32 gpu_wait_ms = 0.f;  // a second, the RSX thread waiting for the GPU to make them
	f32 rsx_load = 0.f;     // the RSX thread busy, its FIFO not waiting for the game's commands (%)
	f32 ppu_load = 0.f;     // the PPU threads running, as a share of the console's CPU threads (%)
	f32 spu_load = 0.f;     // and the SPU threads
	std::vector<std::pair<std::string, f32>> busiest; // the busiest threads, each one's share of the time it ran (%)
	std::string collision;  // the collision accuracy setting's state, if the game has collision code
	u32 heap_mib = 0;
	u32 free_mib = 0;       // direct memory left
};

// The latest
ps5_perf ps5_perf_latest();
