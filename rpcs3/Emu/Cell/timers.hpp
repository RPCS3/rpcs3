#pragma once

#include "util/types.hpp"

u64 convert_to_timebased_time(u64 time);
u64 get_timebased_time();

// Returns some relative time in microseconds, don't change this fact
u64 get_system_time();

// As get_system_time but obeys Clocks scaling setting. Microseconds.
u64 get_guest_system_time(u64 time = umax);
