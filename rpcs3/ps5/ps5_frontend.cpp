// PS5: RPCS3's frontend on the console (ps5_frontend.h).
//
// The emulator's callbacks follow the headless frontend (headless_application.cpp)
// and main_application.cpp's, without Qt: the Vulkan renderer drawing to the
// display (ps5_gs_frame), the null audio, keyboard, mouse, camera and music
// handlers until the console's own are written, no dialogs. The main thread
// runs a queue of the calls RPCS3 makes "from the main thread".

#include "stdafx.h"
#include <map>
#include "ps5_frontend.h"

#include <sys/stat.h>

std::string ps5_open_files_report(); // ps5_fdtrack.cpp
#include "ps5_gs_frame.h"
#include "ps5_pad_handler.h"
#include "ps5_firmware.h"
#include "ps5_embedded_files.h"
#include "ps5_audio_backend.h"
#include "ps5_cpus.h"
#include "Input/pad_thread.h"

#include "util/logs.hpp"
#include "util/sysinfo.hpp"
#include "Utilities/Thread.h"
#include "Utilities/File.h"
#include "Utilities/bin_patch.h"
#include "Emu/emu_callbacks.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "ps5_memory_cache.h"
#include "Emu/system_utils.hpp"
#include "Emu/system_progress.hpp"
#include "Emu/vfs_config.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Io/pad_config.h"
#include "Emu/Io/KeyboardHandler.h"
#include "Emu/Io/MouseHandler.h"
#include "Emu/Io/Null/NullKeyboardHandler.h"
#include "Emu/Io/Null/NullMouseHandler.h"
#include "Emu/Io/Null/null_camera_handler.h"
#include "Emu/Io/Null/null_music_handler.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/Null/NullAudioBackend.h"
#include "Emu/Audio/Null/null_enumerator.h"
#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/RSX/VK/VKGSRender.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/Cell/Modules/cellOskDialog.h"
#include "Emu/Cell/Modules/cellSaveData.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/overlay_save_dialog.h"
#include "Emu/RSX/Overlays/overlay_trophy_notification.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "util/video_source.h"

#include <ps5platform/heap.h>
#include <ps5platform/kernel.h>

#include <condition_variable>
#include <sys/stat.h>
#include <csignal>
#include <ucontext.h>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <thread>

LOG_CHANNEL(sys_log, "SYS");
LOG_CHANNEL(ps5_log, "PS5");

// Utilities/Thread.cpp, on the PS5
bool ps5_load_code_copy(const char* path);

// ps5_terminate.cpp
void ps5_set_terminate_handler();
void ps5_play_menu_sound(const std::string& path, f32 volume); // ps5_sound.cpp: the menus' sounds

// ps5_localized.cpp (localized_strings.py): the English, or nullptr
const char* ps5_localized_string(localized_string_id id);

// Emu/RSX/Overlays/overlay_utils.cpp
std::u32string utf8_to_u32string(std::string_view utf8_string);

std::string spu_collision_tick(u32 seconds, bool running); // SPULLVMRecompiler.cpp: learnt collision accuracy, gathered and written; the trace's words
extern atomic_t<u64> g_ps5_getllar_waits[64][2]; // SPUThread.cpp: each SPU slot's GETLLAR polls answered by busy waiting, and by sleeping
extern atomic_t<u32> g_ps5_spu_cpus[64][16]; // SPUThread.cpp: each SPU slot's polls by the CPU it was on
u32 ps5_spu_slot(u32 id); // SPUThread.cpp: an SPU thread's slot in those counts
extern atomic_t<u64> g_ps5_frames_generated; // VKFrameGen.cpp: frames made between the game's
extern atomic_t<u64> g_ps5_frame_gen_us; // VKFrameGen.cpp: the RSX thread's time making and showing them,
extern atomic_t<u64> g_ps5_frame_gen_gpu_wait_us; // and in it waiting for the GPU; frames made but dropped;
extern atomic_t<u64> g_ps5_frames_dropped; // and the time waiting for an image for the game's own frames
extern atomic_t<u64> g_ps5_image_wait_us[2]; // and for those made
extern atomic_t<u64> g_ps5_frames_quick_moves; // and the game's frames whose camera moved too quickly for frames between
extern atomic_t<u64> g_ps5_frames_camera_field; // and those whose camera's field held

namespace
{
	// The title's trace (rpcs3_ps5_title::trace), set once by rpcs3_ps5_run
	void (*g_trace)(const char* line) = nullptr;

	// The title's build (rpcs3_ps5_title::build), set once by rpcs3_ps5_run
	std::string g_title_build;

	// Set once the boot has started the game
	atomic_t<bool> g_booted = false;

	template <usz N, typename... Args>
	void trace(const char (&format)[N], const Args&... args)
	{
		if (g_trace)
		{
			g_trace(fmt::format(format, args...).c_str());
		}
	}

	// The PS3's threads' loads, sampled. The console reports no CPU time per
	// thread or for the process: a thread's CPU clock runs with the wall clock
	// whether the thread works or waits (build 81's overlay read 18/16, 6/16
	// and 1/16 for 18 PPU, 6 SPU and 1 RSX threads, on my console), and times()
	// counts one thread. The status thread samples which PPU and SPU threads
	// are running rather than waiting, stopped or suspended, from the state
	// they keep themselves, 20 times a second while a game has run for ten
	// seconds. (A thread of its own sampling 500 times a second from boot on
	// froze the title as games booted: build 82.)
	struct sampled_load
	{
		std::string name;
		u32 running = 0;
		u32 samples = 0;
		u32 spu_index = umax;          // an SPU's slot, for its GETLLAR counts (ps5_spu_slot)
		std::map<u32, u32> spu_pcs;    // where an SPU was when sampled running, by local storage address
	};

	// The PS3's threads are read on the main thread, where a stop and the
	// next boot reset the object manager (g_fxo->reset, in Kill and Init)
	// without the lock idm::select takes: the status thread, listing them
	// itself, read a destroyed thread as the launcher's shell stopped and GTA
	// IV booted, and the app froze (build 88, on my console). The samples the
	// main thread takes, the status thread sums under this lock
	std::mutex g_loads_mutex;
	std::map<u64, sampled_load> g_loads;  // by group (PPU 0, SPU 1) and id, for the trace
	u32 g_group_running[2]{};             // the groups' running threads, summed over the samples
	u32 g_group_samples = 0;
	atomic_t<bool> g_sample_posted = false; // one sample waiting for the main thread at most
	atomic_t<f32> g_group_load[2]{};      // each group's running threads as a share of the hardware threads, over the last five seconds

	bool is_running(const cpu_thread& thread)
	{
		return !(thread.state & (cpu_flag::wait + cpu_flag::stop + cpu_flag::exit + cpu_flag::suspend + cpu_flag::dbg_global_pause + cpu_flag::dbg_pause));
	}

	// On the main thread
	void sample_thread_loads()
	{
		std::lock_guard lock(g_loads_mutex);
		const auto sample = [&](u32 group, u32 id, const cpu_thread& thread, auto&& name)
		{
			sampled_load& load = g_loads[u64{group} << 32 | id];
			if (load.name.empty())
			{
				load.name = name();
			}
			const bool busy = is_running(thread);
			load.running += busy;
			load.samples++;
			g_group_running[group] += busy;
		};
		idm::select<named_thread<ppu_thread>>([&](u32 id, named_thread<ppu_thread>& ppu)
		{
			sample(0, id, ppu, [&] { return "PPU " + ppu.get_name(); });
		});
		idm::select<named_thread<spu_thread>>([&](u32 id, named_thread<spu_thread>& spu)
		{
			sample(1, id, spu, [&] { return "SPU " + spu.get_name(); });
			sampled_load& load = g_loads[u64{1} << 32 | id];
			load.spu_index = ps5_spu_slot(spu.id); // its own ID, as SPUThread.cpp counts it: idm's ID for it differs, and put every SPU in one slot (build 103's trace)
			if (is_running(spu) && load.spu_pcs.size() < 256)
			{
				load.spu_pcs[spu.pc]++;
			}
		});
		g_group_samples++;
	}

	// The busiest since the last call, by the share of the time each ran, and
	// the groups' loads for the overlay; the counts begin again
	std::string take_thread_loads()
	{
		std::lock_guard lock(g_loads_mutex);
		const f32 hardware = static_cast<f32>(std::max<u32>(1, utils::get_thread_count()));
		for (u32 group = 0; group < 2; group++)
		{
			g_group_load[group].store(g_group_samples ? 100.f * g_group_running[group] / g_group_samples / hardware : 0.f);
			g_group_running[group] = 0;
		}
		g_group_samples = 0;

		// Each SPU slot's GETLLAR counts since the last call: an SPU polling for
		// work, answered by busy waiting, counts as running, so a busy SPU says
		// whether it worked or spun
		static u64 s_waits[64][2]{};
		static u64 s_last_us = 0;
		const u64 now_us = get_system_time();
		const f64 window = s_last_us ? (now_us - s_last_us) / 1e6 : 5.0;
		s_last_us = now_us;
		u64 waits[64][2]{};
		u32 spu_cpus[64][16]{};
		for (u32 slot = 0; slot < 64; slot++)
		{
			for (u32 kind = 0; kind < 2; kind++)
			{
				const u64 count = g_ps5_getllar_waits[slot][kind].load();
				waits[slot][kind] = count - s_waits[slot][kind];
				s_waits[slot][kind] = count;
			}
			for (u32 cpu = 0; cpu < 16; cpu++)
			{
				spu_cpus[slot][cpu] = g_ps5_spu_cpus[slot][cpu].exchange(0);
			}
		}

		std::vector<std::pair<f64, std::string>> loads;
		for (const auto& [key, load] : g_loads)
		{
			if (!load.samples)
			{
				continue;
			}
			const f64 share = 100.0 * load.running / load.samples;
			std::string name = load.name;
			if (load.spu_index != umax && share >= 50.0)
			{
				// Its three commonest places, and how its polls were answered
				std::vector<std::pair<u32, u32>> pcs(load.spu_pcs.begin(), load.spu_pcs.end());
				std::sort(pcs.begin(), pcs.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
				fmt::append(name, " (at");
				for (usz i = 0; i < pcs.size() && i < 3; i++)
				{
					fmt::append(name, " 0x%x %.0f%%", pcs[i].first, 100.0 * pcs[i].second / load.samples);
				}
				const u64* counts = waits[load.spu_index % 64];
				fmt::append(name, "; polls spun %.0f/s, slept %.0f/s", counts[0] / window, counts[1] / window);

				// And the CPUs it polled on, the two commonest
				std::pair<u32, u32> cpus[16];
				u64 polls = 0;
				for (u32 cpu = 0; cpu < 16; cpu++)
				{
					cpus[cpu] = {spu_cpus[load.spu_index % 64][cpu], cpu};
					polls += cpus[cpu].first;
				}
				if (polls)
				{
					std::sort(std::begin(cpus), std::end(cpus), [](const auto& a, const auto& b) { return a.first > b.first; });
					fmt::append(name, "; on CPU %u %.0f%%", cpus[0].second, 100.0 * cpus[0].first / polls);
					if (cpus[1].first)
					{
						fmt::append(name, ", %u %.0f%%", cpus[1].second, 100.0 * cpus[1].first / polls);
					}
				}
				name += ")";
			}
			loads.emplace_back(share, std::move(name));
		}
		g_loads.clear();
		std::sort(loads.begin(), loads.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
		std::string result;
		for (usz i = 0; i < loads.size() && i < 10 && loads[i].first >= 5.0; i++)
		{
			fmt::append(result, " [%s %.0f%%]", loads[i].second, loads[i].first);
		}
		return result;
	}

	// Every file and folder under path made readable and writable by all, and
	// each folder searchable: what the title writes is otherwise the title's
	// alone, and FTP could read config.yml but not replace or delete it
	// (fs_move_failed, on my console). Listed with fs::dir, as RPCS3 lists its
	// folders: a walk with lstat found nothing (214b6a5, on my console).
	// Returns how many it changed, and counts what it saw and what chmod refused
	usz open_to_ftp(const std::string& path, usz& seen, usz& refused)
	{
		usz changed = 0;
		fs::dir dir(path);
		if (!dir)
		{
			return 0;
		}

		for (const fs::dir_entry& entry : dir)
		{
			if (entry.name == "." || entry.name == "..")
			{
				continue;
			}

			seen++;
			const std::string child = path + "/" + entry.name;
			if (::chmod(child.c_str(), entry.is_directory ? 0777 : 0666) == 0)
			{
				changed++;
			}
			else
			{
				refused++;
			}

			if (entry.is_directory)
			{
				changed += open_to_ftp(child, seen, refused);
			}
		}

		return changed;
	}

	// RPCS3's overlay images, built into the title (ps5_embedded_files.h),
	// written to /app0/rpcs3/Icons/ui/ where its native dialogs look for them,
	// each one missing or of another size. FTP could not add them to the
	// title's own rpcs3/ folder (fs_move_failed, on my console). Returns how
	// many it wrote
	usz install_embedded_files(const std::string& dir)
	{
		usz written = 0;
		for (unsigned i = 0; i < ps5_embedded_file_count; i++)
		{
			const ps5_embedded_file& file = ps5_embedded_files[i];
			const std::string path = dir + file.name;
			fs::stat_t info{};
			if (fs::get_stat(path, info) && !info.is_directory && info.size == file.size)
			{
				continue;
			}

			if (fs::create_path(fs::get_parent_dir(path)) && fs::write_file(path, fs::rewrite, file.data, file.size))
			{
				written++;
			}
		}
		return written;
	}

	// The compiled modules under path (<name>.obj.gz) that have no IR log
	// beside them (<name>.obj.log), removed so they compile again and write
	// one. Returns how many
	usz remove_objects_without_logs(const std::string& path)
	{
		usz removed = 0;
		fs::dir dir(path);
		if (!dir)
		{
			return 0;
		}

		std::vector<std::string> objects;
		for (const fs::dir_entry& entry : dir)
		{
			if (entry.name == "." || entry.name == "..")
			{
				continue;
			}

			const std::string child = path + "/" + entry.name;
			if (entry.is_directory)
			{
				removed += remove_objects_without_logs(child);
			}
			else if (entry.name.ends_with(".obj.gz"))
			{
				// Cached compressed (JITLLVM.cpp's ObjectCache); the log is <name>.obj.log
				objects.push_back(child);
			}
		}

		for (const std::string& object : objects)
		{
			if (!fs::is_file(object.substr(0, object.size() - 3) + ".log") && fs::remove_file(object))
			{
				removed++;
			}
		}

		return removed;
	}

	// The PS3's save data list (choose a save, or make a new one), drawn by
	// RSX's native overlay as the desktop's save_data_dialog draws it, without
	// its Qt fallback. With none, every list operation answered cancel: the
	// Ratchet & Clank Collection's New Game and Load Game did nothing (its
	// cellSaveDataUserListSave and ListLoad returned 1, on my console)
	struct ps5_save_dialog final : SaveDialogBase
	{
		s32 ShowSaveDataList(const std::string& base_dir, std::vector<SaveDataEntry>& save_entries, s32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet, bool enable_overlay) override
		{
			const bool use_end = sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_BEGIN, 0) >= 0;
			s32 result = -2;

			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				result = manager->create<rsx::overlays::save_dialog>()->show(base_dir, save_entries, focused, op, listSet, enable_overlay);
				if (result == rsx::overlays::user_interface::selection_code::error)
				{
					sys_log.error("PS5: the save data dialog returned an error");
					result = -2;
				}
			}
			else
			{
				sys_log.error("PS5: no overlay manager for the save data dialog");
			}

			if (use_end)
			{
				sysutil_send_system_cmd(CELL_SYSUTIL_DRAWING_END, 0);
			}

			return result;
		}
	};

	// The native dialogs' video source (a save's animated icon, ICON1.PAM),
	// without a decoder: never active, so the still icon shows. With none,
	// video_view's ensure failed and took the game's save thread with it,
	// the game frozen on its save (Ratchet & Clank 1, its second save, on
	// my console)
	struct ps5_still_video_source final : video_source
	{
		void set_iso_path(const std::string&) override {}
		void set_video_path(const std::string&, bool) override {}
		void set_audio_path(const std::string&, bool) override {}
		void set_active(bool) override {}
		bool get_active() const override { return false; }
		bool has_new() const override { return false; }
		void get_image(std::vector<u8>& data, int& w, int& h, int& ch, int& bpp) override
		{
			data.clear();
			w = h = ch = bpp = 0;
		}
	};

	// Trophy pop-ups, by the same overlay, as the desktop's
	// trophy_notification_helper shows them
	struct ps5_trophy_notification final : TrophyNotificationBase
	{
		s32 ShowTrophyNotification(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer) override
		{
			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				// More than one at a time: the notification schedules them
				auto popup = std::make_shared<rsx::overlays::trophy_notification>();
				return manager->add(popup, false)->show(trophy, trophy_icon_buffer);
			}

			return 0;
		}
	};

	// The console's display, as swapchain_ps5.hpp chooses it
	constexpr int display_width = 3840;
	constexpr int display_height = 2160;
	constexpr f64 display_rate = 59.94;

	// RPCS3's log to klog: the title's standard error reaches klog, prefixed
	// with its name. Warnings and worse, and what RPCS3 always logs.
	struct klog_listener final : logs::listener
	{
		void log(u64 /*stamp*/, const logs::message& msg, std::string_view prefix, std::string_view text) override
		{
			const bool notice = msg > logs::level::warning;

			// And everything this frontend's own channel says (the frame rate...)
			if (!notice || std::string_view(msg->name) == "PS5")
			{
				std::fprintf(stderr, "%.*s%s: %.*s\n", static_cast<int>(prefix.size()), prefix.data(),
					msg->name, static_cast<int>(text.size()), text.data());
			}

			// The launcher's and the pause menu's own notices, always and outside
			// the caps: they are few, and they say what was asked between games
			// (a boot from the launcher after the first game said nothing). And
			// the audio output's, a line a port every ten seconds counting the
			// grains the emulator was late with: the sound's stutter, measured
			const std::string_view channel(msg->name);
			if (msg <= logs::level::notice && (channel == "Launcher" || channel == "PauseMenu" || channel == "PS5Audio"))
			{
				trace("%s%s: %s", prefix, msg->name, text.substr(0, std::min<usz>(text.find('\n'), 200)));
				return;
			}

			// Notices only until the game runs: its own (a file opened, a thread
			// made) filled the trace's 6000 within a second of the home menu's start
			if (msg > (g_booted ? logs::level::warning : logs::level::notice))
			{
				return;
			}

			// The trace takes notices too, each cut to its first line: RPCS3.log is
			// written behind, and a crash took its last lines (the PS3 home menu's
			// boot, PS5_RPCS3 21ab5ee). A running game can log thousands a second:
			// the trace keeps the first ones, so it never slows the emulation
			if (notice)
			{
				text = text.substr(0, std::min<usz>(text.find('\n'), 200));
			}

			// Errors and fatal ones beyond that, to a cap of their own: the
			// warnings' cap hid how the home menu's setup run ended
			static atomic_t<u32> s_traced = 0;
			static atomic_t<u32> s_errors = 0;
			if (msg <= logs::level::error)
			{
				if (msg <= logs::level::fatal || s_errors++ < 4000)
				{
					trace("%s%s: %s", prefix, msg->name, text);
				}
			}
			else if (const u32 n = s_traced++; n < 6000)
			{
				trace("%s%s: %s", prefix, msg->name, text);
			}
			else if (n == 6000)
			{
				trace("(the trace stops RPCS3's warnings here, errors go on: the rest is in /app0/rpcs3/cache/RPCS3.log)");
			}
		}
	};

	// The signals RPCS3 does not handle itself end the title without a word: the
	// trace records them first (in the handler, against the rules for one, as a
	// last act), then the default action ends the process as before
	void record_signal(int sig, siginfo_t* info, void* uct)
	{
		const auto* context = static_cast<const ucontext_t*>(uct);
		trace("fatal signal %d (code %d) at address %p, rip %p, thread %s", sig, info->si_code, info->si_addr,
			reinterpret_cast<void*>(context->uc_mcontext.mc_rip), thread_ctrl::get_name());
		::signal(sig, SIG_DFL);
		::raise(sig);
	}

	void record_signals()
	{
		struct ::sigaction sa{};
		sa.sa_flags = SA_SIGINFO;
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = record_signal;
		for (const int sig : {SIGABRT, SIGFPE, SIGSYS, SIGTRAP, SIGXCPU})
		{
			::sigaction(sig, &sa, nullptr);
		}
	}

	// The calls RPCS3 makes on the main thread
	struct main_queue
	{
		std::mutex mutex;
		std::condition_variable cv;
		std::deque<std::pair<std::function<void()>, atomic_t<u32>*>> calls;
		bool quit = false;

		void post(std::function<void()> func, atomic_t<u32>* wake_up)
		{
			{
				std::lock_guard lock(mutex);
				calls.emplace_back(std::move(func), wake_up);
			}
			cv.notify_one();
		}

		void request_quit()
		{
			{
				std::lock_guard lock(mutex);
				quit = true;
			}
			cv.notify_one();
		}

		// Runs the calls waiting now, without waiting for more
		void run_pending()
		{
			std::unique_lock lock(mutex);
			while (!calls.empty())
			{
				auto [func, wake_up] = std::move(calls.front());
				calls.pop_front();
				lock.unlock();
				func();
				if (wake_up)
				{
					*wake_up = true;
					wake_up->notify_one();
				}
				lock.lock();
			}
		}

		// A stop with nothing to boot after it (on_stop): the title ends once the
		// main thread's call that saw it is done, if the emulator is still
		// stopped then. Big Picture Mode hands off to a game by stopping its own
		// shell and booting the game in one call, and must not end the title
		atomic_t<bool> stop_pending = false;

		// Runs the calls as they come, until quit is asked for
		void run()
		{
			std::unique_lock lock(mutex);
			while (true)
			{
				cv.wait(lock, [this] { return quit || !calls.empty(); });
				while (!calls.empty())
				{
					auto [func, wake_up] = std::move(calls.front());
					calls.pop_front();
					lock.unlock();
					func();
					if (wake_up)
					{
						*wake_up = true;
						wake_up->notify_one();
					}
					if (stop_pending.exchange(false) && Emu.IsStopped())
					{
						quit = true;
					}
					lock.lock();
				}
				if (quit)
				{
					return;
				}
			}
		}
	};

	main_queue g_main;

	void create_callbacks()
	{
		// A blocking call made on the main thread itself runs at once: queued,
		// it waited on itself (BootGame installing the Ratchet & Clank
		// Collection disc's PKGDIR, BlockingCallFromMainThread, on my console)
		static const std::thread::id s_main_thread = std::this_thread::get_id();
		g_emu_callbacks.call_from_main_thread = [](std::function<void()> func, atomic_t<u32>* wake_up)
		{
			if (wake_up && std::this_thread::get_id() == s_main_thread)
			{
				func();
				*wake_up = true;
				wake_up->notify_one();
				return;
			}
			g_main.post(std::move(func), wake_up);
		};

		g_emu_callbacks.try_to_quit = [](bool force_quit, std::function<void()> on_exit) -> bool
		{
			if (!force_quit)
			{
				return false;
			}
			if (on_exit)
			{
				on_exit();
			}
			g_main.request_quit();
			return true;
		};

		g_emu_callbacks.update_emu_settings = []()
		{
			Emu.CallFromMainThread([]() { rpcs3::utils::configure_logs(Emu.IsStopped()); });
		};
		g_emu_callbacks.save_emu_settings = []()
		{
			Emu.BlockingCallFromMainThread([]() { Emulator::SaveSettings(g_cfg.to_string(), Emu.GetTitleID()); });
		};

		g_emu_callbacks.init_kb_handler = []()
		{
			ensure(g_fxo->init<KeyboardHandlerBase, NullKeyboardHandler>(Emu.DeserialManager()));
		};
		g_emu_callbacks.init_mouse_handler = []()
		{
			ensure(g_fxo->init<MouseHandlerBase, NullMouseHandler>(Emu.DeserialManager()));
		};
		g_emu_callbacks.init_pad_handler = [](std::string_view title_id)
		{
			// pad_thread gives players 1 to 4 the console's controllers (ps5_pad_handler)
			ensure(g_fxo->init<named_thread<pad_thread>>(nullptr, nullptr, title_id));
		};

		g_emu_callbacks.get_audio = []() -> std::shared_ptr<AudioBackend>
		{
			// The console's own output (libSceAudioOut), a port per backend
			// RPCS3 opens; /app0/rpcs3-mute.txt keeps the silent one
			if (fs::is_file("/app0/rpcs3-mute.txt"))
			{
				return std::make_shared<NullAudioBackend>();
			}
			return std::make_shared<ps5_audio_backend>();
		};
		g_emu_callbacks.get_audio_enumerator = [](u64) -> std::shared_ptr<audio_device_enumerator>
		{
			return std::make_shared<null_enumerator>();
		};

		g_emu_callbacks.init_gs_render = [](utils::serial* ar)
		{
			switch (const video_renderer type = g_cfg.video.renderer)
			{
			case video_renderer::null:
				g_fxo->init<rsx::thread, named_thread<NullGSRender>>(ar);
				break;
			case video_renderer::vulkan:
				g_fxo->init<rsx::thread, named_thread<VKGSRender>>(ar);
				break;
			default:
				fmt::throw_exception("The PS5 draws with Vulkan (or the null renderer), not %s", type);
			}
		};
		g_emu_callbacks.get_gs_frame = []() -> std::unique_ptr<GSFrameBase>
		{
			return std::make_unique<ps5_gs_frame>(display_width, display_height, display_rate);
		};
		g_emu_callbacks.close_gs_frame = []() {};

		g_emu_callbacks.get_camera_handler = []() -> std::shared_ptr<camera_handler_base> { return std::make_shared<null_camera_handler>(); };
		g_emu_callbacks.get_music_handler = []() -> std::shared_ptr<music_handler_base> { return std::make_shared<null_music_handler>(); };

		g_emu_callbacks.get_msg_dialog = []() -> std::shared_ptr<MsgDialogBase> { return {}; };
		g_emu_callbacks.get_osk_dialog = []() -> std::shared_ptr<OskDialogBase> { return {}; };
		g_emu_callbacks.get_save_dialog = []() -> std::unique_ptr<SaveDialogBase> { return std::make_unique<ps5_save_dialog>(); };
		g_emu_callbacks.get_sendmessage_dialog = []() -> std::shared_ptr<SendMessageDialogBase> { return {}; };
		g_emu_callbacks.get_recvmessage_dialog = []() -> std::shared_ptr<RecvMessageDialogBase> { return {}; };
		g_emu_callbacks.get_trophy_notification_dialog = []() -> std::unique_ptr<TrophyNotificationBase> { return std::make_unique<ps5_trophy_notification>(); };

		// The settings a game runs with, at each boot: the trace takes RPCS3's
		// notices only until the first game runs, so RPCS3.log's "Used
		// configuration" alone had them (God of War: Ascension's Accurate RSX
		// reservation access, build 108)
		g_emu_callbacks.on_run = [](bool)
		{
			if (Emu.GetTitleID().empty())
			{
				return;
			}
			const auto& core = g_cfg.core;
			const auto& video = g_cfg.video;
			const char* zcull = video.relaxed_zcull_sync ? "Relaxed" : video.precise_zpass_count ? "Precise" : "Approximate";
			trace("settings for %s: SPU block size %s, Max SPURS %s, SPU loop detection %s, accurate SPU reservations %s, accurate RSX reservation access %s, XFloat %s; "
				"frame limit %s, vblank %s Hz, resolution %s at %s%%, ZCULL %s, write color buffers %s, multithreaded RSX %s; late audio wait %s ms; frame generation %s (%s, quality %s, steps aside %s); own core %s, collision accuracy %s",
				Emu.GetTitleID(), core.spu_block_size.to_string(), core.max_spurs_threads.to_string(), core.spu_loop_detection.to_string(),
				core.spu_accurate_reservations.to_string(), core.rsx_accurate_res_access.to_string(), core.spu_xfloat_accuracy.to_string(),
				video.frame_limit.to_string(), video.vblank_rate.to_string(), video.resolution.to_string(), video.resolution_scale_percent.to_string(), zcull,
				video.write_color_buffers.to_string(), video.multithreaded_rsx.to_string(), g_cfg.audio.late_audio_wait.to_string(),
				video.frame_generation.to_string(), video.frame_generation_frames.to_string(), video.frame_generation_quality.to_string(),
				video.frame_generation_step_aside.to_string(), core.spu_own_core.to_string(), core.spu_collision_accuracy.to_string());
		};
		g_emu_callbacks.on_pause = []() {};
		g_emu_callbacks.on_resume = []() {};
		g_emu_callbacks.on_stop = []() {};
		g_emu_callbacks.on_ready = []() {};
		g_emu_callbacks.on_missing_fw = []()
		{
			ps5_log.error("No PS3 system software: install PS3UPDAT.PUP (from Sony) first");
		};
		g_emu_callbacks.on_emulation_stop_no_response = [](std::shared_ptr<atomic_t<bool>> closed_successfully, int)
		{
			if (!closed_successfully || !*closed_successfully)
			{
				ps5_log.fatal("Stopping the emulator took too long: a thread has probably deadlocked");
			}
		};
		g_emu_callbacks.on_save_state_progress = [](std::shared_ptr<atomic_t<bool>>, stx::shared_ptr<utils::serial>, stx::atomic_ptr<std::string>*, std::shared_ptr<void>) {};
		g_emu_callbacks.enable_disc_eject = [](bool) {};
		g_emu_callbacks.enable_disc_insert = [](bool) {};
		g_emu_callbacks.handle_taskbar_progress = [](s32, s32) {};

		// The English the desktop's native dialogs show (rpcs3qt/localized_emu.h),
		// its %0 given the argument: with none, the save data list's button
		// prompts had no words (on my console)
		g_emu_callbacks.get_localized_string = [](localized_string_id id, const char* args) -> std::string
		{
			const char* text = ps5_localized_string(id);
			std::string result = text ? text : "";
			if (const usz at = result.find("%0"); at != umax && args)
			{
				result.replace(at, 2, args);
			}
			return result;
		};
		g_emu_callbacks.get_localized_u32string = [](localized_string_id id, const char* args) -> std::u32string
		{
			return utf8_to_u32string(g_emu_callbacks.get_localized_string(id, args));
		};
		g_emu_callbacks.get_localized_setting = [](const cfg::_base*, u32) -> std::string { return {}; };
		g_emu_callbacks.get_photo_path = [](std::string_view title) -> std::string
		{
			return fs::get_config_dir() + "photos/" + std::string(title) + "/";
		};
		// The menus' sounds, at the launcher's Menu sounds volume
		g_emu_callbacks.play_sound = [](const std::string& path, std::optional<f32> volume)
		{
			ps5_play_menu_sound(path, volume.value_or(1.f) * static_cast<f32>(g_cfg.audio.menu_sound_volume.get()) / 100.f);
		};
		g_emu_callbacks.get_image_info = [](const std::string&, std::string&, s32&, s32&, s32&) { return false; };
		g_emu_callbacks.get_scaled_image = [](const std::string&, s32, s32, s32&, s32&, u8*, bool) { return false; };
		// The title's fonts (the launcher's Inter); the overlays' default is still
		// the PS3's own, from dev_flash
		fs::ps5_on_many_open = [](const std::string& report) { trace("open files: %s; %s", report, ps5_open_files_report()); };
		g_emu_callbacks.get_font_dirs = []() { return std::vector<std::string>{"/app0/assets/fonts/"}; };
		// A disc's packages (PKGDIR, INSDIR, PS3_EXTRA), installed to dev_hdd0 at
		// its first boot, as the desktop's headless frontend does
		g_emu_callbacks.on_install_pkgs = [](const std::vector<std::string>& pkgs, bool from_optical_drive)
		{
			for (const std::string& pkg : pkgs)
			{
				trace("frontend: installing %s", pkg);
				if (!rpcs3::utils::install_pkg(pkg, from_optical_drive))
				{
					sys_log.error("Failed to install %s", pkg);
					trace("frontend: installing %s failed", pkg);
					return false;
				}
			}
			trace("frontend: %u packages installed", pkgs.size());
			return true;
		};
		g_emu_callbacks.add_breakpoint = [](u32) {};
		g_emu_callbacks.display_sleep_control_supported = []() { return false; };
		g_emu_callbacks.enable_display_sleep = [](bool) {};
		g_emu_callbacks.check_microphone_permissions = []() {};
		g_emu_callbacks.make_video_source = []() -> std::unique_ptr<video_source> { return std::make_unique<ps5_still_video_source>(); };
		g_emu_callbacks.enable_gamemode = [](bool) {};
		g_emu_callbacks.get_database_config = [](const std::string&) -> std::string { return {}; };
	}
}

// What the Qt frontend defines for the emulator, without Qt

// The input configurations (rpcs3qt/pad_settings_dialog.cpp on the desktop)
cfg_input_configurations g_cfg_input_configs;

// The desktop's --input-config option (rpcs3.cpp): none on the console
std::string g_input_config_override;

// Repeats an operation until it succeeds, keeping the main thread's calls
// running in between when it is the main thread that waits
void qt_events_aware_op(int repeat_duration_ms, std::function<bool()> wrapped_op)
{
	while (!wrapped_op())
	{
		if (thread_ctrl::is_main())
		{
			g_main.run_pending();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(std::max(repeat_duration_ms, 1)));
	}
}

// The title's: asks the shell to close it, and never returns (ps5_frontend.h)
extern "C" void catchReturnFromMain(int status);

// A fatal error ends the title through the shell: a title never calls exit
[[noreturn]] void report_fatal_error(std::string_view text, bool /*is_html*/, bool /*include_help_text*/)
{
	std::fprintf(stderr, "RPCS3: fatal error: %.*s\n", static_cast<int>(text.size()), text.data());
	trace("fatal error: %s", text);
	if (!g_trace)
	{
		// Before the title's main (RPCS3's static initialisers reserve the guest
		// memory): straight to the title's trace file, which nothing has opened yet
		if (FILE* file = std::fopen("/app0/rpcs3-trace.txt", "a"))
		{
			std::fprintf(file, "before main: fatal error: %.*s\n", static_cast<int>(text.size()), text.data());
			std::fclose(file);
		}
	}
	logs::listener::sync_all();
	catchReturnFromMain(1);
	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

namespace
{
	int run(const char* boot_path);
}

int rpcs3_ps5_run(const char* boot_path, const rpcs3_ps5_title& title)
{
	g_trace = title.trace;
	g_title_build = title.build ? title.build : "";
	ps5_pad_handler::set_source(title.poll_pads);

	// RPCS3 is built without exceptions: its fatal errors (fmt::throw_exception)
	// end in report_fatal_error, which records them and ends the title
	return run(boot_path);
}

// The title's build, which the launcher shows (empty when the title has none)
const std::string& ps5_title_build()
{
	return g_title_build;
}

namespace
{
int run(const char* boot_path)
{
	trace("frontend: start, build %s", g_title_build.empty() ? "unknown" : g_title_build);
	record_signals();
	ps5_set_terminate_handler();

	// What RPCS3 writes from here on is open to FTP (files 0666, folders 0777),
	// and what earlier runs wrote is opened now
	::umask(0);
	usz seen = 0, refused = 0;
	// The folder itself too: FTP could not add a folder to it (fs_move_failed)
	const usz opened = (::chmod("/app0/rpcs3", 0777) == 0) + open_to_ftp("/app0/rpcs3", seen, refused);
	trace("frontend: /app0/rpcs3: %u files and folders, %u opened to FTP, %u refused (%s)", seen, opened, refused, refused ? fmt::format("errno %d", errno) : std::string("none"));
	trace("frontend: %u of %u overlay images written to /app0/rpcs3/Icons/ui/", install_embedded_files("/app0/rpcs3/Icons/ui/"), ps5_embedded_file_count);

	// RPCS3's patch files go here over FTP (the launcher's Patches page), and
	// the patch engine reads them when a game boots; PS3 packages and
	// licences go in packages/ (the launcher's Install tab)
	for (const std::string& folder : {patch_engine::get_patches_path(), fs::get_config_dir() + "packages/"})
	{
		fs::create_path(folder);
		std::vector<std::string> names;
		if (fs::dir dir{folder})
		{
			for (const fs::dir_entry& entry : dir)
			{
				if (!entry.is_directory)
				{
					names.push_back(fmt::format("%s (%u KiB)", entry.name, entry.size / 1024));
				}
			}
		}
		trace("frontend: %s: %s", folder, names.empty() ? std::string("empty") : fmt::merge(names, ", "));
	}

	// Which of the title's CPUs share a core, for "Own core for the busy SPU" (ps5_cpus.h)
	trace("frontend: CPUs: %s", ps5_measure_cpus());

	// The fault handler's readable copy of the title's code (Utilities/Thread.cpp)
	if (ps5_load_code_copy("/app0/rpcs3-code.bin"))
	{
		trace("frontend: code copy loaded");
	}
	else
	{
		trace("frontend: no /app0/rpcs3-code.bin: a fault in the title's code cannot be emulated");
	}

	// RPCS3's configuration, dev_hdd0 and log go to /app0/rpcs3/, its caches to
	// /app0/rpcs3/cache/ (fs::get_config_dir and get_cache_dir on PS5)

	if (!thread_ctrl::is_main())
	{
		std::fprintf(stderr, "rpcs3_ps5_run: not on the main thread\n");
		trace("not on the main thread");
		return 1;
	}

	// The thread pool's finalizer, on first use (as rpcs3.cpp)
	trace("frontend: thread pool");
	static_cast<void>(named_thread("", [](int) {}));
	trace("frontend: config %s, cache %s", fs::get_config_dir(), fs::get_cache_dir());

	// The last run's log is kept beside the new one: a run that ended in a
	// crash was followed by a launch, and the launch's new log replaced the
	// crash's (God of War: Ascension, build 109)
	if (fs::is_file(fs::get_cache_dir() + "RPCS3.log"))
	{
		fs::rename(fs::get_cache_dir() + "RPCS3.log", fs::get_cache_dir() + "RPCS3.old.log", true);
	}

	// Listeners stay in RPCS3's list for the life of the process
	static klog_listener klog;
	logs::listener::add(&klog);
	std::unique_ptr<logs::listener> log_file = logs::make_file_listener(fs::get_cache_dir() + "RPCS3.log", 256ull * 1024 * 1024);
	{
		logs::stored_message ver{sys_log.always()};
		ver.text = fmt::format("RPCS3 for the PS5, build %s, on %s", g_title_build.empty() ? "unknown" : g_title_build, utils::get_system_info());
		logs::set_init({std::move(ver)});
	}

	trace("frontend: logs open");
	create_callbacks();

	Emu.SetHasGui(false);
	Emu.SetHeadless(false);
	Emu.SetUsr("00000001");
	// What the desktop's main_application::InitializeEmulator says of the GPU:
	// Vulkan through RADV, the title's own, is there. Without it a boot set the
	// renderer back to Null ("not supported on this device"; PS5_RPCS3 b9ad001)
	Emu.SetSupportedRenderers({video_renderer::null, video_renderer::vulkan});
	Emu.SetDefaultRenderer(video_renderer::vulkan);

	// Emulator::Init insists on an adapter name with Vulkan the default (ddc00ec
	// stopped there on my console). VKGSRender takes the device of that name, or
	// the first when none matches: the console has one, RADV's
	Emu.SetDefaultGraphicsAdapter("PS5 GPU (RADV)");

	trace("frontend: Emu.Init");
	Emu.Init();
	trace("frontend: Emu.Init done; guest memory at %p, its mirror at %p, executable range at %p", vm::g_base_addr, vm::g_sudo_addr, vm::g_exec_addr);
	// An SPU polling for work waits in user mode with MWAITX (AMD) or TPAUSE
	// (Intel) when the CPU allows it, else in a loop of pauses
	trace("frontend: SPU busy waits use %s", utils::has_um_wait() ? (utils::has_waitpkg() ? "TPAUSE" : "MWAITX") : "a loop of pauses (no user-mode wait)");

	// Sony's PS3UPDAT.PUP in the title's folder installs the PS3 system software,
	// as the desktop's File > Install Firmware does (ps5_firmware.cpp)
	const std::string pup_path = "/app0/PS3UPDAT.PUP";
	if (fs::is_file(pup_path))
	{
		trace("firmware: installing from %s", pup_path);
		switch (ps5_install_firmware(pup_path))
		{
		case ps5_firmware_result::installed: trace("firmware: installed"); break;
		case ps5_firmware_result::already_installed: trace("firmware: that version is installed already"); break;
		case ps5_firmware_result::failed: trace("firmware: installation failed (see the lines above)"); break;
		case ps5_firmware_result::no_file: trace("firmware: %s could not be opened", pup_path); break;
		}
	}

	const std::string firmware = utils::get_firmware_version();
	sys_log.always()("PS3 system software: %s", firmware.empty() ? "missing" : firmware);
	trace("PS3 system software: %s", firmware.empty() ? "missing" : firmware);

	// The decoders: LLVM's recompilers where the build has LLVM, the
	// interpreters otherwise, or where /app0/rpcs3-interpreter.txt asks for them
	// (to tell a recompiler's fault from the emulator's). Saved in the global
	// configuration (config.yml), which a boot reads.
	// And no precompilation: booting the home menu from dev_flash analysed every
	// module of the system software first, 13 minutes behind a progress bar on
	// my console (5f51dfd); modules are compiled as they load, and cached
	// The file's first word chooses which: "ppu" or "spu" alone, anything else
	// both (to tell which recompiler a fault is in)
#ifdef LLVM_AVAILABLE
	std::string interpreter_choice;
	const bool interpreter_file = fs::is_file("/app0/rpcs3-interpreter.txt");
	if (interpreter_file)
	{
		interpreter_choice = fs::file("/app0/rpcs3-interpreter.txt").to_string();
		interpreter_choice = interpreter_choice.substr(0, interpreter_choice.find_first_of(" \r\n\t"));
	}
	const bool ppu_llvm = !interpreter_file || interpreter_choice == "spu";
	const bool spu_llvm = !interpreter_file || interpreter_choice == "ppu";
#else
	const bool ppu_llvm = false;
	const bool spu_llvm = false;
#endif
	const ppu_decoder_type ppu_decoder = ppu_llvm ? ppu_decoder_type::llvm : ppu_decoder_type::_static;
	const spu_decoder_type spu_decoder = spu_llvm ? spu_decoder_type::llvm : spu_decoder_type::_static;
	if (g_cfg.core.ppu_decoder != ppu_decoder || g_cfg.core.spu_decoder != spu_decoder || g_cfg.core.llvm_precompilation)
	{
		g_cfg.core.ppu_decoder.set(ppu_decoder);
		g_cfg.core.spu_decoder.set(spu_decoder);
		g_cfg.core.llvm_precompilation.set(false);
		Emulator::SaveSettings(g_cfg.to_string(), "");
	}
	trace("config: PPU %s, SPU %s, no precompilation", ppu_llvm ? "recompiler (LLVM)" : "interpreter", spu_llvm ? "recompiler (LLVM)" : "interpreter");

	// The CPU LLVM compiles for: the host's own (empty), or the first word of
	// /app0/rpcs3-llvm-cpu.txt ("x86-64", "x86-64-v2", "znver1"...). The PPU
	// recompiler's code computed wrong values on my console where the
	// interpreter's did not (the home menu read a float as a pointer), and a
	// plainer target tells a CPU feature's fault from LLVM's or ours. Compiled
	// code is cached per CPU name, so a change compiles anew
	std::string llvm_cpu;
	if (fs::is_file("/app0/rpcs3-llvm-cpu.txt"))
	{
		llvm_cpu = fs::file("/app0/rpcs3-llvm-cpu.txt").to_string();
		llvm_cpu = llvm_cpu.substr(0, llvm_cpu.find_first_of(" \r\n\t"));
	}
	// The recompilers emit SSSE3 at least: with "x86-64" the PPU compile
	// threads died (Cannot select: X86ISD::PSHUFB, on my console) and the boot
	// waited forever for their modules
	if (llvm_cpu == "x86-64" || llvm_cpu == "generic")
	{
		trace("config: LLVM CPU %s ignored: RPCS3's code needs SSSE3 at least (x86-64-v2 or later)", llvm_cpu);
		llvm_cpu.clear();
	}
	if (g_cfg.core.llvm_cpu.to_string() != llvm_cpu)
	{
		g_cfg.core.llvm_cpu.from_string(llvm_cpu);
		Emulator::SaveSettings(g_cfg.to_string(), "");
	}
	trace("config: LLVM CPU %s", llvm_cpu.empty() ? std::string("(the host's)") : llvm_cpu);

	// How many threads LLVM compiles on, from /app0/rpcs3-llvm-threads.txt
	// (a number; none or 0, as many as the CPU has). With 16, SPU cache
	// workers died in LLVM's post-RA scheduler on a memory operand pointer
	// whose upper half read 0x2 for the heap's 0x20 (0bf22b6, on my console):
	// one thread tells a race between the compile threads from a fault in one
	u32 llvm_threads = 0;
	if (fs::is_file("/app0/rpcs3-llvm-threads.txt"))
	{
		const std::string text = fs::file("/app0/rpcs3-llvm-threads.txt").to_string();
		llvm_threads = static_cast<u32>(std::min<unsigned long>(std::strtoul(text.c_str(), nullptr, 10), 1024));
	}
	if (static_cast<u32>(g_cfg.core.llvm_threads) != llvm_threads)
	{
		g_cfg.core.llvm_threads.set(llvm_threads);
		Emulator::SaveSettings(g_cfg.to_string(), "");
	}
	trace("config: LLVM compile threads %s", llvm_threads ? std::to_string(llvm_threads) : std::string("(as many as the CPU has)"));

	// LLVM's logs, where /app0/rpcs3-llvm-logs.txt asks for them: each PPU
	// module's IR beside its object (<name>.obj.log), to compile the same IR on
	// a PC and compare the code. Modules cached without one are removed, so
	// they compile again and write it
	const bool llvm_logs = fs::is_file("/app0/rpcs3-llvm-logs.txt");
	if (g_cfg.core.llvm_logs.get() != llvm_logs)
	{
		g_cfg.core.llvm_logs.set(llvm_logs);
		Emulator::SaveSettings(g_cfg.to_string(), "");
	}
	if (llvm_logs)
	{
		trace("config: LLVM logs on; %u cached modules without one removed", remove_objects_without_logs(fs::get_cache_dir() + "cache"));
	}

	// The renderer draws through VK_KHR_display (ps5_gs_frame); the configuration
	// the first runs saved chose Null, which drew nothing. And no GDB server:
	// nothing on the console attaches to it, and its socket failed to bind
	if (g_cfg.video.renderer != video_renderer::vulkan || !g_cfg.misc.gdb_server.to_string().empty())
	{
		g_cfg.video.renderer.set(video_renderer::vulkan);
		g_cfg.misc.gdb_server.from_string("");
		Emulator::SaveSettings(g_cfg.to_string(), "");
		trace("config: Vulkan renderer, no GDB server");
	}

	// No shader interpreter: the home menu's XMB on my console (688c9aa) froze
	// again and again under it and the title then ended with a system error
	// and memory to spare, as after a GPU hang. Shaders are compiled for each
	// program instead, off the render thread; objects wait for theirs
	if (g_cfg.video.shadermode == shader_mode::async_with_interpreter || g_cfg.video.shadermode == shader_mode::interpreter_only)
	{
		g_cfg.video.shadermode.set(shader_mode::async_recompiler);
		Emulator::SaveSettings(g_cfg.to_string(), "");
		trace("config: shaders compiled asynchronously, without the shader interpreter");
	}

	// Nothing named to boot: RPCS3's Big Picture Mode, the game library on the
	// display, over the games in /app0/rpcs3/games/; "vsh" named: the PS3's
	// own home menu, as the desktop's Boot VSH
	std::string vsh_path;
	bool big_picture = false;
	if (boot_path && (std::string_view(boot_path) == "vsh" || std::string_view(boot_path) == "xmb"))
	{
		vsh_path = g_cfg_vfs.get_dev_flash() + "vsh/module/vsh.self";
		boot_path = fs::is_file(vsh_path) ? vsh_path.c_str() : nullptr;
		trace("frontend: booting the PS3 home menu, %s", boot_path ? vsh_path : std::string("missing"));
	}
	else if (!boot_path || !*boot_path)
	{
		big_picture = true;
		trace("frontend: nothing named to boot: Big Picture Mode, games from %s", rpcs3::utils::get_games_dir());
	}
	rpcs3::utils::configure_logs(true);

	// A game's folder boots through its EBOOT.BIN, as the desktop's game list
	// does (GetElfPathFromDir: EBOOT.BIN, USRDIR/ or PS3_GAME/USRDIR/):
	// BootGame given the folder itself failed, "Failed to open executable"
	// (the Ratchet & Clank Collection's disc folder, on my console). Where
	// none is found, the folder's contents go to the trace
	std::string elf_path;
	if (boot_path && *boot_path && fs::is_dir(boot_path))
	{
		if (Emulator::GetElfPathFromDir(elf_path, boot_path) == game_boot_result::no_errors)
		{
			trace("frontend: %s boots through %s", boot_path, elf_path);
			boot_path = elf_path.c_str();
		}
		else
		{
			std::string listing;
			for (const fs::dir_entry& entry : fs::dir(boot_path))
			{
				if (entry.name != "." && entry.name != ".." && listing.size() < 600)
				{
					fmt::append(listing, " %s%s", entry.name, entry.is_directory ? "/" : "");
				}
			}
			trace("frontend: no EBOOT.BIN, USRDIR/EBOOT.BIN or PS3_GAME/USRDIR/EBOOT.BIN in %s; it holds:%s", boot_path, listing.empty() ? std::string(" nothing") : listing);
		}
	}

	int status = 0;
	bool booted = false;
	if (big_picture)
	{
		booted = Emu.BootBigPictureMode();
		trace("frontend: Big Picture Mode %s", booted ? "booted" : "failed to boot");
		status = booted ? 0 : 1;
	}
	else if (boot_path && *boot_path)
	{
		trace("frontend: Emu.BootGame %s", boot_path);
		if (const game_boot_result result = Emu.BootGame(boot_path, "", true); result != game_boot_result::no_errors)
		{
			sys_log.error("Booting %s failed: %s", boot_path, result);
			trace("frontend: booting failed: %s", result);
			status = 1;
		}
		else
		{
			booted = true;
		}
	}

	{
		if (booted)
		{
			trace("frontend: booted; running until the emulation stops");
			g_booted = true;

			// Every five seconds, in the trace: the emulation's state, the frames
			// RSX flipped, and where the PPU threads are, to tell a stall from slow
			named_thread status("PS5 Status", []()
			{
				u32 running_for = 0; // seconds a game has run without a stop
				for (u32 seconds = 0; thread_ctrl::state() != thread_state::aborting; seconds++)
				{
					const bool sampling = running_for >= 10;
					for (u32 tick = 0; tick < 20 && thread_ctrl::state() != thread_state::aborting; tick++)
					{
						thread_ctrl::wait_for(50'000);
						if (sampling && Emu.IsRunning() && !g_sample_posted.exchange(true))
						{
							g_main.post([]()
							{
								if (Emu.IsRunning())
								{
									sample_thread_loads();
								}
								g_sample_posted = false;
							}, nullptr);
						}
					}
					running_for = Emu.IsRunning() && !Emu.GetTitleID().empty() ? running_for + 1 : 0;
					if (!running_for)
					{
						std::lock_guard lock(g_loads_mutex);
						g_loads.clear();
					}
					if (seconds % 5 != 4)
					{
						continue;
					}
					// What GTA IV's watched collision functions did, learnt (with or
					// without the trace), and written down while the game stops too
					const std::string collision = spu_collision_tick(5, running_for > 0);
					if (Emu.IsStopped())
					{
						continue;
					}
					// The busiest of the PS3's threads over these five seconds
					if (const std::string busiest = take_thread_loads(); !busiest.empty())
					{
						trace("busiest threads (share of the time each was running):%s; %s; %s", busiest, collision.empty() ? std::string("no collision code") : collision, ps5_cpu_own_core_text());
					}

					// RSX's flips and where the PPU threads are, read on the main
					// thread (g_loads_mutex); not waited for long, so a main thread
					// that hangs still leaves the rest of this line
					struct emulator_view
					{
						atomic_t<u32> done = 0;
						u64 flips = 0;
						u32 count = 0;
						std::string ppus;
					};
					const auto view = std::make_shared<emulator_view>();
					g_main.post([view]()
					{
						if (!Emu.IsStopped())
						{
							if (const auto render = rsx::get_current_renderer())
							{
								view->flips = render->int_flip_index;
							}
							view->count = idm::select<named_thread<ppu_thread>>([&](u32, ppu_thread& ppu)
							{
								if (view->ppus.size() < 600)
								{
									fmt::append(view->ppus, " [%s: 0x%x %s]", ppu.get_name(), ppu.cia, ppu.current_function ? ppu.current_function : "");
								}
							});
						}
						view->done = 1;
					}, nullptr);
					for (u32 wait = 0; wait < 10 && !view->done && thread_ctrl::state() != thread_state::aborting; wait++)
					{
						thread_ctrl::wait_for(20'000);
					}
					const bool seen = view->done != 0;
					const u64 flips = seen ? view->flips : 0;
					// The game's own frames a second since the last status line (RSX's
					// flips; the frames made between are counted apart)
					static u64 s_last_flips = 0;
					static u64 s_last_flips_us = 0;
					const u64 flips_us = get_system_time();
					const f64 game_fps = s_last_flips_us && flips >= s_last_flips && flips_us > s_last_flips_us ? static_cast<f64>(flips - s_last_flips) * 1e6 / static_cast<f64>(flips_us - s_last_flips_us) : 0.0;
					s_last_flips = flips;
					s_last_flips_us = flips_us;
					const u32 count = seen ? view->count : 0;
					const std::string ppus = seen ? view->ppus : std::string(" (not read: the main thread is busy)");
					// And memory: the home menu's run ended at 205 s with no error of
					// RPCS3's or signal (71d0fa2), as the kernel ends a title out of
					// memory or after a GPU fault
					struct ps5_heap_stats heap{};
					ps5_heap_stats(&heap);
					size_t flexible = 0, direct = 0;
					int64_t direct_start = 0;
					sceKernelAvailableFlexibleMemorySize(&flexible);
					sceKernelAvailableDirectMemorySize(0, sceKernelGetDirectMemorySize(), 0x4000, &direct_start, &direct);
					// And the progress dialog's counters: the PPU thread waits for them
					// to be cleared before running the game (PPUThread.cpp,
					// ppu_cmd::initialize), and with both recompilers the home menu
					// stayed on its loading screen with no PPU code run (974d605)
					const std::string progress_text = g_progr_text;
					// And the open files: a title holds about 249 by path at once,
					// and GTA IV's boot ran out of them (bfb4830). Every descriptor
					// below 4096, by kind, and those RPCS3's fs opened
					u32 regular = 0, folders = 0, sockets = 0, others = 0;
					for (int fd = 0; fd < 4096; fd++)
					{
						struct ::stat info;
						if (::fstat(fd, &info) != 0) continue;
						if (S_ISREG(info.st_mode)) regular++;
						else if (S_ISDIR(info.st_mode)) folders++;
						else if (S_ISSOCK(info.st_mode)) sockets++;
						else others++;
					}
					// Frame generation's settings while it is on (VKFrameGen.h)
					std::string frame_gen;
					if (g_cfg.video.frame_generation)
					{
						const u32 most = static_cast<u32>(g_cfg.video.frame_generation_frames);
						const u32 quality = static_cast<u32>(g_cfg.video.frame_generation_quality);
						const u32 step = static_cast<u32>(g_cfg.video.frame_generation_step_aside);
						frame_gen = fmt::format(", up to %s a frame, %s, stepping aside %s", most ? std::to_string(most) : std::string("Auto"),
							quality == 0 ? "Fast" : quality == 2 ? "Quality" : "Balanced", step == 0 ? "never" : step == 2 ? "in most moves" : "in quick moves");
					}
					trace("status %ds: state %d, RSX flips %d (%.1f a second; %u made between, %u dropped, %u game frames moving too quickly for them, %u with the camera's field, in %u ms: %u ms waiting for the GPU; waiting for an image: %u ms for the game's frames, %u ms for those made%s); heap %d MiB (peak %d), system cache %u MiB in memory, free direct %d MiB, flexible %d MiB; progress '%s' modules %u/%u files %u/%u; open: %u files, %u folders, %u sockets, %u other (%u by fs); %d PPU threads:%s", seconds + 1,
						static_cast<u32>(Emu.GetStatus()), flips, game_fps, static_cast<u32>(g_ps5_frames_generated.load()), static_cast<u32>(g_ps5_frames_dropped.load()),
						static_cast<u32>(g_ps5_frames_quick_moves.load()), static_cast<u32>(g_ps5_frames_camera_field.load()), static_cast<u32>(g_ps5_frame_gen_us.load() / 1000), static_cast<u32>(g_ps5_frame_gen_gpu_wait_us.load() / 1000),
						static_cast<u32>(g_ps5_image_wait_us[0].load() / 1000), static_cast<u32>(g_ps5_image_wait_us[1].load() / 1000), frame_gen, heap.mapped_bytes >> 20, heap.peak_bytes >> 20, static_cast<u32>(ps5_syscache_bytes() >> 20), direct >> 20,
						flexible >> 20, progress_text, +g_progr_pdone, +g_progr_ptotal, +g_progr_fdone, +g_progr_ftotal, regular, folders, sockets, others, static_cast<u32>(fs::ps5_open_tracked()), count, ppus);
					// Near the limit, which ones (once per 40 more)
					static u32 s_reported = 0;
					if (const u32 open = regular + folders; open >= 150 && open >= s_reported + 40)
					{
						s_reported = open;
						trace("open files: %s; %s", fs::ps5_open_report(), ps5_open_files_report());
					}
				}
				// The title ends: what was learnt since the last write
				spu_collision_tick(0, false);
			});

			// Until the game stops and RPCS3 asks to quit
			// A reboot the game asks for (sys_sm_shutdown, the home menu's after
			// rebuilding its database) stops the emulator and then boots again
			// from after_kill_callback: only a stop without one ends the title
			// A game Big Picture Mode started returns to it the same way
			// (Emulator::Kill sets after_kill_callback to BootBigPictureMode);
			// and the stop that hands the library's shell over to a game is
			// followed by that game's boot in the same main-thread call, so a
			// stop ends the title only if nothing runs once that call is done
			g_emu_callbacks.on_stop = []()
			{
				if (!Emu.after_kill_callback)
				{
					g_main.stop_pending = true;
				}
				else
				{
					trace("frontend: stopped, booting again (a reboot the game asked for, or back to Big Picture Mode)");
				}
			};
			g_main.run();
		}
	}

	if (!Emu.IsStopped())
	{
		Emu.Kill(false);
	}

	trace("frontend: stopping, status %d", status);
	logs::listener::sync_all();
	logs::listener::shutdown_all();
	return status;
}
} // namespace

// The performance overlay's PPU (0) and SPU (1) loads on the console: their
// threads running, as a share of the hardware threads (sample_thread_loads)
f32 ps5_sampled_load(u32 group)
{
	return group < 2 ? g_group_load[group].load() : 0.f;
}
