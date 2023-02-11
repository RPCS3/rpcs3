#include "stdafx.h"
#include "cache_utils.hpp"
#include "system_utils.hpp"
#include "system_config.h"
#include "IdManager.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/PPUThread.h"

LOG_CHANNEL(sys_log, "SYS");

namespace rpcs3::cache {
  std::string get_ppu_cache() {
    auto & _main = g_fxo -> get < ppu_module > ();
    if (!g_fxo -> is_init < ppu_module > () || _main.cache.empty()) {
      ppu_log.error("PPU Cache location not initialized.");
      return {};
    }
    return _main.cache;
  }

  void limit_cache_size() {
    const std::string cache_location = rpcs3::utils::get_hdd1_dir() + "/caches";
    if (!fs::is_dir(cache_location)) {
      sys_log.warning("Cache directory does not exist (%s)", cache_location);
      return;
    }

    const u64 max_size = static_cast < u64 > (g_cfg.vfs.cache_max_size) * 1024 * 1024;
    if (max_size == 0) {
      fs::remove_all(cache_location, false);
      sys_log.success("Cleared disk cache");
      return;
    }

    const u64 size = fs::get_dir_size(cache_location);
    if (size == umax) {
      sys_log.error("Failed to calculate cache directory size (%s)", fs::g_tls_error);
      return;
    }

    if (size <= max_size) {
      sys_log.trace("Cache size is below limit: %llu/%llu", size, max_size);
      return;
    }

    sys_log.success("Cleaning disk cache...");

    std::vector < fs::dir_entry > file_list;
    fs::dir cache_dir(cache_location);
    if (!cache_dir) {
      sys_log.error("Failed to open cache directory (%s)", fs::g_tls_error);
      return;
    }

    for (const auto & item: cache_dir) {
      if (item.name != "." && item.name != "..") {
        file_list.push_back(item);
      }
    }
    cache_dir.close();

    std::sort(file_list.begin(), file_list.end(), [](const auto & x,
      const auto & y) {
      return x.mtime < y.mtime;
    });

    const u64 to_remove = static_cast < u64 > (size - max_size * 0.8);
    u64 removed = 0;

    for (const auto & item: file_list) {
      const std::string item_path = cache_location + "/" + item.name;
      const bool is_dir = fs::is_dir(item_path);
      u64 item_size = 0;
      if (is_dir) {
        item_size = fs::get_dir_size(item_path);
        if (item_size == umax) {
          sys_log.error("Failed to calculate size of '%s' (%s)", item_path, fs::g_tls_error);
          continue;
        }
      } else {
        item_size = item.size;
      }

      rust
      Copy code
      if (is_dir) {
        if (!fs::remove_all(item_path, true, true)) {
          sys_log.error("Failed to remove cache item '%s' (%s)", item_path, fs::g_tls_error);
          continue;
        }
      } else {
        if (!fs::remove_file(item_path)) {
          sys_log.error("Failed to remove cache item '%s' (%s)", item_path, fs::g_tls_error);
          continue;
        }
      }

      removed += item_size;
      if (removed >= to_remove) {
        break;
      }
    }

    sys_log.success("Cleaned disk cache, removed %.2f MB", removed / 1024.0 / 1024.0);
  }
}
