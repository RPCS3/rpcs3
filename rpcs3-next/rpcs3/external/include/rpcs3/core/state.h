#pragma once
#include "config.h"
#include <string>
#include <atomic>
#include <memory>
#include <mutex>
#include <common/exception.h>

namespace rpcs3
{
	struct state_t
	{
		std::atomic<bool> run;
		std::atomic<bool> closing;
		std::mutex core_mutex;
		config_t config;

		std::string path_to_elf;
		std::string virtual_path_to_elf;
	};

	extern state_t state;

	using lv2_lock_t = std::unique_lock<std::mutex>;

	inline bool check_lv2_lock(lv2_lock_t& lv2_lock)
	{
		return lv2_lock.owns_lock() && lv2_lock.mutex() == &state.core_mutex;
	}

	struct closing_exception
	{
	};
}

#define LV2_LOCK ::rpcs3::lv2_lock_t lv2_lock(::rpcs3::state.core_mutex)
#define LV2_DEFER_LOCK ::rpcs3::lv2_lock_t lv2_lock
#define CHECK_LV2_LOCK(x) if (!check_lv2_lock(x)) throw EXCEPTION("lv2_lock is invalid or not locked")
#define CHECK_EMU_STATUS if (::rpcs3::state.closing) throw ::rpcs3::closing_exception{}
