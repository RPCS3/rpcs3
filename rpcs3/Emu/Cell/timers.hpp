#pragma once

#include "util/types.hpp"

u64 get_timebased_time();
void initalize_timebased_time();
u64 get_system_time();
u64 get_guest_system_time(u64 time = umax);
