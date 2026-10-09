#pragma once

// PS5: the PS3's system cache (dev_hdd1, cellSysCache.cpp) kept in memory.
//
// A game copies what it reads from its disc into its system cache, to read it
// back faster than a PS3's Blu-ray drive could: God of War: Ascension's FIOS
// wrote /dev_hdd1/fios/cache.dat about 90 times a second on my console (build
// 108). A title's writes run at full speed for about 1.3 GiB and then at about
// 2 MiB/s (PS5_PayloadSDK's platform/docs/PROBE.md), and RPCS3 empties the
// cache whenever the game stops, so each session spent the burst within two
// minutes, FIOS's writes then slowed to about 9 a second, and the game waited
// on them: 0 fps for minutes, its FIOS thread in sys_fs_write at every sample.
// Here the disc is a folder on the same storage as the cache, so the cache
// saves no reading time; kept in memory, it costs no writes. It goes when the
// title ends, as RPCS3's own goes when the game stops.

#include "util/types.hpp"

#include <string>

// The folder cellSysCache keeps its caches in: a device in memory, made on first use
const std::string& ps5_syscache_root();

// What the caches hold now, in bytes, and the most they may hold (as the PS3's
// own system cache partition: 2 GiB)
u64 ps5_syscache_bytes();
u64 ps5_syscache_limit();
