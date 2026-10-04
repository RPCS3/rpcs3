#include "stdafx.h"
#include "RetroAchievements.h"

#ifdef RPCS3_RA_ENABLED

#include "rc_client.h"
#include "rc_consoles.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Emu/RSX/Overlays/overlay_controls.h"
#include "Emu/RSX/Overlays/overlay_manager.h"
#include "util/logs.hpp"
#include "rpcs3_version.h"
#include "ra_config.h"
#include "Utilities/File.h"

#include <curl/curl.h>
#include <mutex>
#include <string>
#include <thread>

#ifdef RC_CLIENT_SUPPORTS_RAINTEGRATION
#include "rc_client_raintegration.h"
#include <shlwapi.h>
#endif

LOG_CHANNEL(ra_log, "RA");

namespace rpcs3::ra
{
	static rc_client_t* s_client = nullptr;
	static std::mutex s_mutex;
	static atomic_t<bool> s_game_loaded = false;
	static atomic_t<bool> s_integration_loaded = false;
	static atomic_t<bool> s_pending_hc_restart = false;
static std::string s_user_agent;
	static HWND s_main_hwnd = nullptr;

	// PS3 memory bank layout — identity-mapped to PS3 VA space.
	// Bank0 base/size are verified from vm::get(vm::main) at game load; banks 1–3 are fixed.
	static atomic_t<u32> s_bank0_base{0x00000000};
	static atomic_t<u32> s_bank0_size{0x10000000};
	static atomic_t<u32> s_bank1_base{0x10000000};
	static atomic_t<u32> s_bank1_size{0x20000000};
	static atomic_t<u32> s_bank2_base{0x30000000};
	static atomic_t<u32> s_bank2_size{0x10000000};
	static atomic_t<u32> s_bank3_base{0x40000000};
	static atomic_t<u32> s_bank3_size{0x10000000};

	static u32 read_memory(u32 address, u8* buffer, u32 num_bytes, rc_client_t* /*client*/)
	{
		if (vm::try_access(address, buffer, num_bytes, false))
			return num_bytes;

		// Sparse user/rsx regions: return zeros for unmapped pages within allocated banks
		// so rcheevos sees a valid (zero) read rather than a failure.
		const u32 b1 = s_bank1_base, b2 = s_bank2_base, b3 = s_bank3_base;
		const u32 e1 = b1 + s_bank1_size, e2 = b2 + s_bank2_size, e3 = b3 + s_bank3_size;
		if ((address >= b1 && address < e1) ||
		    (address >= b2 && address < e2) ||
		    (address >= b3 && address < e3))
		{
			memset(buffer, 0, num_bytes);
			return num_bytes;
		}

		return 0;
	}

	static size_t curl_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
	{
		auto* buf = static_cast<std::string*>(userdata);
		buf->append(ptr, size * nmemb);
		return size * nmemb;
	}

	static void server_call(const rc_api_request_t* request,
		rc_client_server_callback_t callback, void* callback_data, rc_client_t* /*client*/)
	{
		struct request_state
		{
			std::string url;
			std::string post_data;
			rc_client_server_callback_t callback;
			void* callback_data;
		};

		auto* state = new request_state{
			request->url,
			request->post_data ? request->post_data : "",
			callback,
			callback_data
		};

		std::thread([state]()
		{
			std::string response_body;
			long http_status = 0;

			CURL* curl = curl_easy_init();
			if (!curl)
			{
				rc_api_server_response_t response{};
				response.http_status_code = RC_API_SERVER_RESPONSE_CLIENT_ERROR;
				state->callback(&response, state->callback_data);
				delete state;
				return;
			}

#ifdef _WIN32
			curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

			CURLcode err;
			err = curl_easy_setopt(curl, CURLOPT_URL, state->url.c_str());
			if (err != CURLE_OK) ra_log.error("curl_easy_setopt(CURLOPT_URL): %s", curl_easy_strerror(err));
			err = curl_easy_setopt(curl, CURLOPT_USERAGENT, s_user_agent.c_str());
			if (err != CURLE_OK) ra_log.error("curl_easy_setopt(CURLOPT_USERAGENT): %s", curl_easy_strerror(err));
			err = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
			if (err != CURLE_OK) ra_log.error("curl_easy_setopt(CURLOPT_WRITEFUNCTION): %s", curl_easy_strerror(err));
			err = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
			if (err != CURLE_OK) ra_log.error("curl_easy_setopt(CURLOPT_WRITEDATA): %s", curl_easy_strerror(err));
			err = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
			if (err != CURLE_OK) ra_log.error("curl_easy_setopt(CURLOPT_SSL_VERIFYPEER): %s", curl_easy_strerror(err));
			err = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
			if (err != CURLE_OK) ra_log.error("curl_easy_setopt(CURLOPT_TIMEOUT): %s", curl_easy_strerror(err));

			if (!state->post_data.empty())
			{
				err = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, state->post_data.c_str());
				if (err != CURLE_OK) ra_log.error("curl_easy_setopt(CURLOPT_POSTFIELDS): %s", curl_easy_strerror(err));
			}

			const CURLcode res = curl_easy_perform(curl);
			if (res == CURLE_OK)
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
			else
				ra_log.error("HTTP request failed: %s (url='%s')", curl_easy_strerror(res), state->url);

			curl_easy_cleanup(curl);

			rc_api_server_response_t response{};
			if (res == CURLE_OK)
			{
				response.body = response_body.c_str();
				response.body_length = response_body.size();
				response.http_status_code = static_cast<int>(http_status);
			}
			else
			{
				response.body = curl_easy_strerror(res);
				response.body_length = std::strlen(response.body);
				response.http_status_code = RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR;
			}

			state->callback(&response, state->callback_data);
			delete state;
		}).detach();
	}

	static void log_message(const char* message, const rc_client_t* /*client*/)
	{
		ra_log.notice("%s", message);
	}

	struct badge_icon final : public rsx::overlays::image_view
	{
		std::unique_ptr<rsx::overlays::image_info> m_img;

		explicit badge_icon(const std::string& path)
			: m_img(std::make_unique<rsx::overlays::image_info>(path))
		{
			if (m_img->w > 0 && m_img->h > 0)
			{
				set_raw_image(m_img.get());
				set_size(64, 64);
			}
		}

		bool valid() const { return m_img && m_img->w > 0 && m_img->h > 0; }
	};

	static void event_handler(const rc_client_event_t* event, rc_client_t* /*client*/)
	{
		switch (event->type)
		{
		case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
		{
			ra_log.success("Achievement unlocked: %s", event->achievement->title);
			Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/ra_unlock.wav", std::nullopt);

			std::string text = "You have earned a trophy.\n";
			text += event->achievement->title;
			if (event->achievement->description && *event->achievement->description)
			{
				text += '\n';
				text += event->achievement->description;
			}

			const bool hardcore = g_cfg_ra.hardcore.get();
			const color4f bg = hardcore
				? color4f(0.f, 0.f, 0.f, 0.85f)
				: color4f(0.25f, 0.25f, 0.25f, 0.85f);

			std::shared_ptr<rsx::overlays::overlay_element> icon;
			if (event->achievement->badge_name && *event->achievement->badge_name)
			{
				const std::string badge_path = fs::get_executable_dir() + "RACache/Badge/" + event->achievement->badge_name + ".png";
				if (fs::exists(badge_path))
				{
					auto b = std::make_shared<badge_icon>(badge_path);
					if (b->valid())
						icon = std::move(b);
				}
			}

			rsx::overlays::queue_message(text, 8'000'000, {}, rsx::overlays::message_pin_location::top_left, std::move(icon), false, false, bg);
			break;
		}
		case RC_CLIENT_EVENT_GAME_COMPLETED:
			ra_log.success("Game completed/mastered");
			break;
		case RC_CLIENT_EVENT_RESET:
			ra_log.warning("rcheevos requested emulator reset");
			if (!Emu.IsStopped())
				s_pending_hc_restart = true;
			else
				Emu.Restart(false);
			break;
		case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_SHOW:
		case RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE:
			if (g_cfg_ra.challenge_indicators.get())
				ra_log.notice("Challenge indicator: %d", event->type);
			break;
		case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_SHOW:
		case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_HIDE:
		case RC_CLIENT_EVENT_ACHIEVEMENT_PROGRESS_INDICATOR_UPDATE:
			if (g_cfg_ra.progress_notifications.get())
				ra_log.notice("Progress indicator: %d", event->type);
			break;
		case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_SHOW:
		case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE:
		case RC_CLIENT_EVENT_LEADERBOARD_TRACKER_UPDATE:
			if (g_cfg_ra.leaderboard_trackers.get())
				ra_log.notice("Leaderboard tracker: %d", event->type);
			break;
		case RC_CLIENT_EVENT_LEADERBOARD_SUBMITTED:
			ra_log.success("Leaderboard submitted: %s", event->leaderboard->title);
			Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/ra_lb.wav", std::nullopt);
			break;
		case RC_CLIENT_EVENT_LEADERBOARD_SCOREBOARD:
			ra_log.notice("Leaderboard scoreboard: %s", event->leaderboard->title);
			break;
		default:
			ra_log.notice("rcheevos event: %d", event->type);
			break;
		}
	}

	void initialize()
	{
		g_cfg_ra.load();

		{
			std::lock_guard lock(s_mutex);
			if (s_client)
				return;

			s_user_agent = "RPCS3/" + std::string(rpcs3::get_version_and_branch()) + " rcheevos";
			s_client = rc_client_create(read_memory, server_call);
			if (!s_client)
			{
				ra_log.error("Failed to create rc_client");
				return;
			}

			rc_client_enable_logging(s_client, RC_CLIENT_LOG_LEVEL_VERBOSE, log_message);
			rc_client_set_event_handler(s_client, event_handler);
			rc_client_set_hardcore_enabled(s_client, g_cfg_ra.hardcore ? 1 : 0);
			rc_client_set_unpromoted_enabled(s_client, g_cfg_ra.unofficial ? 1 : 0);
			rc_client_set_encore_mode_enabled(s_client, g_cfg_ra.encore ? 1 : 0);
			rc_client_set_spectator_mode_enabled(s_client, g_cfg_ra.spectator ? 1 : 0);
			ra_log.notice("RetroAchievements initialized");
		}

	}

	void shutdown()
	{
		std::lock_guard lock(s_mutex);
		if (s_client)
		{
			rc_client_destroy(s_client);
			s_client = nullptr;
		}
		s_game_loaded = false;
	}

	void on_game_start()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client || s_game_loaded)
			return;

		const auto load_callback = [](int result, const char* error_message, rc_client_t* client, void* /*userdata*/)
		{
			if (result == RC_OK)
			{
				const rc_client_game_t* game = rc_client_get_game_info(client);
				ra_log.success("Game loaded: %s", game ? game->title : "Unknown");
			}
			else
			{
				ra_log.warning("Game load failed: %s", error_message ? error_message : "Unknown error");
			}

#ifdef RC_CLIENT_SUPPORTS_RAINTEGRATION
			// ResetMemory() in the DLL callback clears banks; reinstall unconditionally.
			s_game_loaded = true;
			{
				HMODULE hDLL = GetModuleHandleW(L"RA_Integration-x64.dll");
				if (hDLL)
				{
					typedef unsigned char (*RA_ReadMemoryFunc_t)(unsigned int);
					typedef void (*RA_WriteMemoryFunc_t)(unsigned int, unsigned char);
					typedef unsigned int (*RA_ReadMemoryBlockFunc_t)(unsigned int, unsigned char*, unsigned int);
					typedef void (*RA_InstallMemoryBank_t)(int, RA_ReadMemoryFunc_t, RA_WriteMemoryFunc_t, int);
					typedef void (*RA_InstallMemoryBankBlockReader_t)(int, RA_ReadMemoryBlockFunc_t);
					typedef void (*RA_InstallSearchMemoryBankReader_t)(int, RA_ReadMemoryBlockFunc_t);

					auto fn_install = reinterpret_cast<RA_InstallMemoryBank_t>(GetProcAddress(hDLL, "_RA_InstallMemoryBank"));
					auto fn_block   = reinterpret_cast<RA_InstallMemoryBankBlockReader_t>(GetProcAddress(hDLL, "_RA_InstallMemoryBankBlockReader"));
					auto fn_search  = reinterpret_cast<RA_InstallSearchMemoryBankReader_t>(GetProcAddress(hDLL, "_RA_InstallSearchMemoryBankReader"));
					auto fn_clear   = reinterpret_cast<void(*)()>(GetProcAddress(hDLL, "_RA_ClearMemoryBanks"));

					// Bank 1 intentionally spans rsx_context + user64k[0] as one 512MB block.
					if (auto blk = vm::get(vm::main))
						{ s_bank0_base = blk->addr; s_bank0_size = blk->size; }

					static auto read_bank0 = [](unsigned int offset) -> unsigned char {
						u8 byte = 0;
						vm::try_access(s_bank0_base + offset, &byte, 1, false);
						return byte;
					};
					static auto write_bank0 = [](unsigned int offset, unsigned char value) {
						vm::try_access(s_bank0_base + offset, &value, 1, true);
					};
					static auto block_bank0 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						return vm::try_access(s_bank0_base + offset, buf, size, false) ? size : 0;
					};

					static auto read_bank1 = [](unsigned int offset) -> unsigned char {
						u8 byte = 0;
						vm::try_access(s_bank1_base + offset, &byte, 1, false);
						return byte;
					};
					static auto write_bank1 = [](unsigned int offset, unsigned char value) {
						vm::try_access(s_bank1_base + offset, &value, 1, true);
					};
					static auto block_bank1 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						if (vm::try_access(s_bank1_base + offset, buf, size, false))
							return size;
						std::memset(buf, 0, size);
						constexpr unsigned int PAGE = 4096U;
						for (unsigned int i = 0; i < size; i += PAGE)
							vm::try_access(s_bank1_base + offset + i, buf + i, std::min(PAGE, size - i), false);
						return size;
					};
					static auto search_bank1 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						if (vm::try_access(s_bank1_base + offset, buf, size, false))
							return size;
						constexpr unsigned int PAGE = 4096U;
						unsigned int alloc_pages = 0;
						for (unsigned int i = 0; i < size; i += PAGE)
						{
							if (vm::check_addr(s_bank1_base + offset + i, vm::page_readable, PAGE))
								alloc_pages++;
						}
						if (alloc_pages == 0)
							return 0;
						std::memset(buf, 0, size);
						for (unsigned int i = 0; i < size; i += PAGE)
							vm::try_access(s_bank1_base + offset + i, buf + i, std::min(PAGE, size - i), false);
						return size;
					};

					static auto read_bank2 = [](unsigned int offset) -> unsigned char {
						u8 byte = 0;
						vm::try_access(s_bank2_base + offset, &byte, 1, false);
						return byte;
					};
					static auto write_bank2 = [](unsigned int offset, unsigned char value) {
						vm::try_access(s_bank2_base + offset, &value, 1, true);
					};
					static auto block_bank2 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						if (vm::try_access(s_bank2_base + offset, buf, size, false))
							return size;
						std::memset(buf, 0, size);
						constexpr unsigned int PAGE = 4096U;
						for (unsigned int i = 0; i < size; i += PAGE)
							vm::try_access(s_bank2_base + offset + i, buf + i, std::min(PAGE, size - i), false);
						return size;
					};
					static auto search_bank2 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						if (vm::try_access(s_bank2_base + offset, buf, size, false))
							return size;
						constexpr unsigned int PAGE = 4096U;
						unsigned int alloc_pages = 0;
						for (unsigned int i = 0; i < size; i += PAGE)
						{
							if (vm::check_addr(s_bank2_base + offset + i, vm::page_readable, PAGE))
								alloc_pages++;
						}
						if (alloc_pages == 0)
							return 0;
						std::memset(buf, 0, size);
						for (unsigned int i = 0; i < size; i += PAGE)
							vm::try_access(s_bank2_base + offset + i, buf + i, std::min(PAGE, size - i), false);
						return size;
					};

					static auto read_bank3 = [](unsigned int offset) -> unsigned char {
						u8 byte = 0;
						vm::try_access(s_bank3_base + offset, &byte, 1, false);
						return byte;
					};
					static auto write_bank3 = [](unsigned int offset, unsigned char value) {
						vm::try_access(s_bank3_base + offset, &value, 1, true);
					};
					static auto block_bank3 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						if (vm::try_access(s_bank3_base + offset, buf, size, false))
							return size;
						std::memset(buf, 0, size);
						constexpr unsigned int PAGE = 4096U;
						for (unsigned int i = 0; i < size; i += PAGE)
							vm::try_access(s_bank3_base + offset + i, buf + i, std::min(PAGE, size - i), false);
						return size;
					};
					static auto search_bank3 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						if (vm::try_access(s_bank3_base + offset, buf, size, false))
							return size;
						constexpr unsigned int PAGE = 4096U;
						unsigned int alloc_pages = 0;
						for (unsigned int i = 0; i < size; i += PAGE)
						{
							if (vm::check_addr(s_bank3_base + offset + i, vm::page_readable, PAGE))
								alloc_pages++;
						}
						if (alloc_pages == 0)
							return 0;
						std::memset(buf, 0, size);
						for (unsigned int i = 0; i < size; i += PAGE)
							vm::try_access(s_bank3_base + offset + i, buf + i, std::min(PAGE, size - i), false);
						return size;
					};

					// DLL's m_vMemoryBlocks has no mutex; install banks on same thread as do_frame.
					Emu.CallFromMainThread([fn_install, fn_clear, fn_block, fn_search]()
					{
						if (fn_install)
						{
							if (fn_clear) fn_clear();
							fn_install(0, read_bank0, write_bank0, static_cast<int>(s_bank0_size));
							fn_install(1, read_bank1, write_bank1, static_cast<int>(s_bank1_size));
							fn_install(2, read_bank2, write_bank2, static_cast<int>(s_bank2_size));
							fn_install(3, read_bank3, write_bank3, static_cast<int>(s_bank3_size));
						}
						if (fn_block)
						{
							fn_block(0, block_bank0);
							fn_block(1, block_bank1);
							fn_block(2, block_bank2);
							fn_block(3, block_bank3);
						}
						if (fn_search)
						{
							fn_search(1, search_bank1);
							fn_search(2, search_bank2);
							fn_search(3, search_bank3);
						}
					});
				}
			}
#endif
		};

		const std::string& disc_path = Emu.GetLastBoot();
		rc_client_begin_identify_and_load_game(s_client, RC_CONSOLE_PLAYSTATION_3,
			disc_path.c_str(), nullptr, 0,
			load_callback, nullptr);
	}

	void on_game_stop()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;

		rc_client_unload_game(s_client);
		s_game_loaded = false;
		ra_log.notice("RetroAchievements game session ended");
	}

	void on_frame_end()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;
#ifdef RC_CLIENT_SUPPORTS_RAINTEGRATION
		Emu.CallFromMainThread([client = s_client]() {
			if (client)
			{
				rc_client_do_frame(client);
				rc_client_raintegration_update_main_window_handle(client, s_main_hwnd);
			}
		});
#else
		if (!s_game_loaded)
			return;
		rc_client_do_frame(s_client);
#endif
	}

#ifdef RC_CLIENT_SUPPORTS_RAINTEGRATION
	static void call_set_paused(int paused)
	{
		static auto fn = []() -> void(*)(int) {
			HMODULE hDLL = GetModuleHandleW(L"RA_Integration-x64.dll");
			return hDLL ? reinterpret_cast<void(*)(int)>(GetProcAddress(hDLL, "_RA_SetPaused")) : nullptr;
		}();
		if (fn) fn(paused);
	}
#endif

	std::vector<ra_achievement_item> get_achievement_list()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return {};

		rc_client_achievement_list_t* list = rc_client_create_achievement_list(
			s_client,
			RC_CLIENT_ACHIEVEMENT_CATEGORY_PROMOTED_AND_UNPROMOTED,
			RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_LOCK_STATE);

		if (!list)
			return {};

		std::vector<ra_achievement_item> result;
		for (uint32_t b = 0; b < list->num_buckets; ++b)
		{
			const rc_client_achievement_bucket_t& bucket = list->buckets[b];
			for (uint32_t i = 0; i < bucket.num_achievements; ++i)
			{
				const rc_client_achievement_t* ach = bucket.achievements[i];
				ra_achievement_item item{};
				item.id                = ach->id;
				item.title             = ach->title       ? ach->title       : "";
				item.description       = ach->description ? ach->description : "";
				item.badge_name        = ach->badge_name;
				item.measured_progress = ach->measured_progress;
				item.measured_percent  = ach->measured_percent;
				item.points            = ach->points;
				item.rarity            = ach->rarity;
				item.rarity_hardcore   = ach->rarity_hardcore;
				item.unlock_time       = static_cast<time_t>(ach->unlock_time);
				item.state             = ach->state;
				item.bucket            = ach->bucket;
				item.unlocked          = ach->unlocked;
				result.push_back(std::move(item));
			}
		}

		rc_client_destroy_achievement_list(list);
		return result;
	}

	// ── Simple JSON helpers (for RA Connect API responses) ───────────────────

	// Extract next {...} object starting at/after pos; advances pos past it.
	static std::string json_next_object(const std::string& json, size_t& pos)
	{
		while (pos < json.size() && json[pos] != '{') ++pos;
		if (pos >= json.size()) return {};
		int depth = 0;
		bool in_str = false, escaped = false;
		auto start = pos;
		for (; pos < json.size(); ++pos)
		{
			char c = json[pos];
			if (escaped) { escaped = false; continue; }
			if (c == '\\' && in_str) { escaped = true; continue; }
			if (c == '"') { in_str = !in_str; continue; }
			if (in_str) continue;
			if (c == '{') ++depth;
			else if (c == '}') { if (--depth == 0) { ++pos; return json.substr(start, pos - start); } }
		}
		return {};
	}

	static std::string json_str(const std::string& obj, const std::string& key)
	{
		const std::string search = "\"" + key + "\":\"";
		auto pos = obj.find(search);
		if (pos == std::string::npos) return {};
		pos += search.size();
		std::string result;
		bool escaped = false;
		for (; pos < obj.size(); ++pos)
		{
			char c = obj[pos];
			if (escaped) { escaped = false; result += c; continue; }
			if (c == '\\') { escaped = true; continue; }
			if (c == '"') break;
			result += c;
		}
		return result;
	}

	static int json_int(const std::string& obj, const std::string& key)
	{
		const std::string search = "\"" + key + "\":";
		auto pos = obj.find(search);
		if (pos == std::string::npos) return 0;
		pos += search.size();
		while (pos < obj.size() && obj[pos] == ' ') ++pos;
		if (pos >= obj.size() || obj.substr(pos, 4) == "null") return 0;
		if (obj[pos] == '"') ++pos;
		try { return std::stoi(obj.substr(pos)); } catch (...) { return 0; }
	}

	static bool json_non_null(const std::string& obj, const std::string& key)
	{
		const std::string search = "\"" + key + "\":";
		auto pos = obj.find(search);
		if (pos == std::string::npos) return false;
		pos += search.size();
		while (pos < obj.size() && obj[pos] == ' ') ++pos;
		return pos < obj.size() && obj.substr(pos, 4) != "null";
	}

	// Async HTTP GET to RA Connect API using the user's Bearer token.
	// callback(body, success) fires on the calling thread (background).
	static void ra_api_get(const std::string& url, const std::string& token,
	                       std::function<void(std::string, bool)> callback)
	{
		std::thread([url, token, callback = std::move(callback)]()
		{
			std::string body;
			CURL* curl = curl_easy_init();
			if (!curl) { callback({}, false); return; }

#ifdef _WIN32
			curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_USERAGENT, s_user_agent.c_str());
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
				+[](char* ptr, size_t, size_t nmemb, void* ud) -> size_t
				{ static_cast<std::string*>(ud)->append(ptr, nmemb); return nmemb; });
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

			curl_slist* headers = nullptr;
			const std::string auth = "Authorization: Bearer " + token;
			headers = curl_slist_append(headers, auth.c_str());
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

			long status = 0;
			const bool ok = curl_easy_perform(curl) == CURLE_OK
			                && (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status), status == 200);
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);

			callback(body, ok);
		}).detach();
	}

	// ── Public helpers ────────────────────────────────────────────────────────

	std::string get_username()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client) return {};
		const rc_client_user_t* user = rc_client_get_user_info(s_client);
		return user && user->username ? user->username : "";
	}

	ra_game_item get_current_game_item()
	{
		std::lock_guard lock(s_mutex);
		ra_game_item item{};
		item.is_current_game = true;
		if (!s_client) return item;

		const rc_client_game_t* game = rc_client_get_game_info(s_client);
		if (game)
		{
			item.id    = game->id;
			item.title = game->title ? game->title : "";
		}

		rc_client_user_game_summary_t summary{};
		rc_client_get_user_game_summary(s_client, &summary);
		item.num_achievements       = summary.num_promoted_achievements;
		item.num_unlocked           = summary.num_unlocked_achievements;
		item.num_unlocked_hardcore  = 0; // summary doesn't expose hardcore separately
		return item;
	}

	void fetch_recently_played_games(std::function<void(std::vector<ra_game_item>, bool)> callback)
	{
		std::string username, token;
		{
			std::lock_guard lock(s_mutex);
			if (!s_client) { callback({}, false); return; }
			const rc_client_user_t* user = rc_client_get_user_info(s_client);
			if (!user) { callback({}, false); return; }
			username = user->username ? user->username : "";
			token    = user->token    ? user->token    : "";
		}

		const std::string url = fmt::format(
			"https://retroachievements.org/API/API_GetUserRecentlyPlayedGames.php?u=%s&c=25", username);

		ra_api_get(url, token, [username, callback = std::move(callback)](std::string body, bool ok)
		{
			if (!ok) { callback({}, false); return; }

			std::vector<ra_game_item> games;
			size_t pos = 0;
			while (pos < body.size())
			{
				std::string obj = json_next_object(body, pos);
				if (obj.empty()) break;
				const int id = json_int(obj, "GameID");
				if (id <= 0) continue;
				ra_game_item g{};
				g.id                   = static_cast<uint32_t>(id);
				g.title                = json_str(obj, "Title");
				g.num_achievements     = static_cast<uint32_t>(json_int(obj, "NumAchievements"));
				g.num_unlocked         = static_cast<uint32_t>(json_int(obj, "NumAchievementsWon"));
				g.num_unlocked_hardcore= 0;
				g.is_current_game      = false;
				games.push_back(std::move(g));
			}
			callback(std::move(games), true);
		});
	}

	void fetch_game_achievements_for_id(uint32_t game_id,
	                                    std::function<void(std::vector<ra_achievement_item>, std::string, bool)> callback)
	{
		std::string username, token;
		{
			std::lock_guard lock(s_mutex);
			if (!s_client) { callback({}, {}, false); return; }
			const rc_client_user_t* user = rc_client_get_user_info(s_client);
			if (!user) { callback({}, {}, false); return; }
			username = user->username ? user->username : "";
			token    = user->token    ? user->token    : "";
		}

		const std::string url = fmt::format(
			"https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?g=%u&u=%s",
			game_id, username);

		ra_api_get(url, token, [callback = std::move(callback)](std::string body, bool ok)
		{
			if (!ok) { callback({}, {}, false); return; }

			const std::string game_title = json_str(body, "Title");

			// Find the "Achievements" object and iterate its values
			const std::string ach_key = "\"Achievements\":";
			auto ach_pos = body.find(ach_key);
			if (ach_pos == std::string::npos) { callback({}, game_title, true); return; }
			ach_pos += ach_key.size();

			// Skip whitespace to the opening '{'
			while (ach_pos < body.size() && body[ach_pos] != '{') ++ach_pos;

			// Iterate through each achievement object inside the map
			std::vector<ra_achievement_item> result;
			size_t pos = ach_pos + 1; // step past outer '{'
			while (pos < body.size() && body[pos] != '}')
			{
				// Skip to next '{' (the achievement object value)
				std::string obj = json_next_object(body, pos);
				if (obj.empty()) break;

				const int id = json_int(obj, "ID");
				if (id <= 0) continue;

				ra_achievement_item item{};
				item.id               = static_cast<uint32_t>(id);
				item.title            = json_str(obj, "Title");
				item.description      = json_str(obj, "Description");
				item.badge_name       = json_str(obj, "BadgeName");
				item.points           = static_cast<uint32_t>(json_int(obj, "Points"));
				item.measured_percent = 0.f;

				const bool earned    = json_non_null(obj, "DateEarned");
				const bool earned_hc = json_non_null(obj, "DateEarnedHardcore");
				item.state   = earned ? RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED : RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
				item.unlocked= earned ? (earned_hc ? 2 : 1) : 0;
				item.bucket  = earned ? RC_CLIENT_ACHIEVEMENT_BUCKET_UNLOCKED : RC_CLIENT_ACHIEVEMENT_BUCKET_LOCKED;
				// unlock_time not parsed for simplicity
				result.push_back(std::move(item));
			}

			callback(std::move(result), game_title, true);
		});
	}

	bool is_active()
	{
		std::lock_guard lock(s_mutex);
		return s_client != nullptr && rc_client_get_user_info(s_client) != nullptr;
	}

	bool is_integration_loaded()
	{
		return s_integration_loaded.load();
	}

	void set_hardcore(bool enabled)
	{
		std::lock_guard lock(s_mutex);
		if (s_client)
			rc_client_set_hardcore_enabled(s_client, enabled ? 1 : 0);
	}

	void set_unofficial(bool enabled)
	{
		std::lock_guard lock(s_mutex);
		if (s_client)
			rc_client_set_unpromoted_enabled(s_client, enabled ? 1 : 0);
	}

	void set_encore(bool enabled)
	{
		std::lock_guard lock(s_mutex);
		if (s_client)
			rc_client_set_encore_mode_enabled(s_client, enabled ? 1 : 0);
	}

	void set_spectator(bool enabled)
	{
		std::lock_guard lock(s_mutex);
		if (s_client)
			rc_client_set_spectator_mode_enabled(s_client, enabled ? 1 : 0);
	}

	std::string get_rich_presence_message()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return {};
		char buf[256] = {};
		rc_client_get_rich_presence_message(s_client, buf, sizeof(buf));
		return buf;
	}

	std::string get_discord_state()
	{
		if (!g_cfg_ra.discord.get() || !s_game_loaded.load())
			return {};
		std::string msg = get_rich_presence_message();
		return msg.empty() ? "RetroAchievements" : msg;
	}

	std::string get_token()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return {};
		const rc_client_user_t* user = rc_client_get_user_info(s_client);
		return user && user->token ? user->token : std::string{};
	}

	uint32_t get_user_score()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return 0;
		const rc_client_user_t* user = rc_client_get_user_info(s_client);
		return user ? user->score : 0;
	}

	void login(const std::string& username, const std::string& password,
	           std::function<void(bool)> callback)
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
		{
			if (callback) callback(false);
			return;
		}

		auto* cb_ptr = callback ? new std::function<void(bool)>(std::move(callback)) : nullptr;
		rc_client_begin_login_with_password(s_client, username.c_str(), password.c_str(),
			[](int result, const char* error_message, rc_client_t* client, void* userdata)
			{
				auto* cb = static_cast<std::function<void(bool)>*>(userdata);
				if (result == RC_OK)
				{
					const rc_client_user_t* user = rc_client_get_user_info(client);
					ra_log.success("Logged in as %s", user ? user->display_name : "Unknown");
				}
				else
				{
					ra_log.error("Login failed: %s", error_message ? error_message : "Unknown error");
				}
				if (cb) { (*cb)(result == RC_OK); delete cb; }
			},
			cb_ptr);
	}

	void login_with_token(const std::string& username, const std::string& token)
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;

		rc_client_begin_login_with_token(s_client, username.c_str(), token.c_str(),
			[](int result, const char* error_message, rc_client_t* client, void* /*userdata*/)
			{
				if (result == RC_OK)
				{
					const rc_client_user_t* user = rc_client_get_user_info(client);
					ra_log.success("Logged in as %s", user ? user->display_name : "Unknown");
				}
				else
				{
					ra_log.error("Login failed: %s", error_message ? error_message : "Unknown error");
				}
			},
			nullptr);
	}

	void logout()
	{
		std::lock_guard lock(s_mutex);
		if (s_client)
		{
			rc_client_logout(s_client);
			ra_log.notice("Logged out from RetroAchievements");
		}
	}

	bool get_hardcore_mode()
	{
		return g_cfg_ra.hardcore.get();
	}

	bool consume_pending_hc_restart()
	{
		return s_pending_hc_restart.exchange(false);
	}

#ifdef RC_CLIENT_SUPPORTS_RAINTEGRATION
	static void raintegration_event_handler(const rc_client_raintegration_event_t* event, rc_client_t* /*client*/)
	{
		switch (event->type)
		{
		case RC_CLIENT_RAINTEGRATION_EVENT_PAUSE:
			Emu.Pause();
			break;
		case RC_CLIENT_RAINTEGRATION_EVENT_HARDCORE_CHANGED:
		{
			const bool enabled = rc_client_get_hardcore_enabled(s_client) != 0;
			ra_log.notice("Hardcore mode changed: %s", enabled ? "enabled" : "disabled");
			g_cfg_ra.hardcore.set(enabled);
			g_cfg_ra.save();
			break;
		}
		case RC_CLIENT_RAINTEGRATION_EVENT_MENU_CHANGED:
			// Qt menu is rebuilt lazily via QMenu::aboutToShow — no action needed here
			break;
		default:
			ra_log.warning("Unhandled RAIntegration event: %u", event->type);
			break;
		}
	}

	static void raintegration_write_memory(u32 address, u8* buffer, u32 num_bytes, rc_client_t* /*client*/)
	{
		vm::try_access(address, buffer, num_bytes, true);
	}

	static void raintegration_get_game_name(char* buffer, u32 buffer_size, rc_client_t* /*client*/)
	{
		const std::string& path = Emu.GetLastBoot();
		std::string filename = path.substr(path.find_last_of("/\\") + 1);
		const auto dot = filename.find_last_of('.');
		if (dot != std::string::npos)
			filename = filename.substr(0, dot);
		std::snprintf(buffer, buffer_size, "%s", filename.c_str());
	}

	static bool ra_cb_is_active()
	{
		return s_game_loaded.load();
	}

	static void ra_cb_cause_unpause()
	{
		Emu.Resume();
	}

	static void ra_cb_cause_pause()
	{
		Emu.Pause();
	}

	static void ra_cb_rebuild_menu()
	{
		// Qt menu rebuilds lazily via aboutToShow; DLL rebuild notification is a no-op.
	}

	static void ra_cb_estimate_title(char* buf)
	{
		if (!buf)
			return;
		const rc_client_game_t* game = s_client ? rc_client_get_game_info(s_client) : nullptr;
		if (game && game->title)
			std::snprintf(buf, 64, "%s", game->title);
		else
			buf[0] = '\0';
	}

	static void ra_cb_reset_emulator()
	{
		(void)Emu.Restart();
	}

	static void ra_cb_load_rom(const char* path)
	{
		if (path)
			(void)Emu.BootGame(std::string(path));
	}

	static void raintegration_load_callback(int result, const char* error_message, rc_client_t* client, void* /*userdata*/)
	{
		switch (result)
		{
		case RC_OK:
			ra_log.notice("RAIntegration DLL loaded successfully");
			s_integration_loaded = true;
			rc_client_raintegration_set_write_memory_function(client, raintegration_write_memory);
			rc_client_raintegration_set_event_handler(client, raintegration_event_handler);
			rc_client_raintegration_set_get_game_name_function(client, raintegration_get_game_name);
			rc_client_raintegration_set_console_id(client, RC_CONSOLE_PLAYSTATION_3);
			// DLL handles login via _RA_AttemptLogin; rc_client_begin_login_with_token uses the offline handler and fails.
			{
				HMODULE hDLL = GetModuleHandleW(L"RA_Integration-x64.dll");
				if (hDLL)
				{
					typedef void (*RA_AttemptLogin_t)(int);
					auto fn_attempt_login = reinterpret_cast<RA_AttemptLogin_t>(GetProcAddress(hDLL, "_RA_AttemptLogin"));
					if (fn_attempt_login)
					{
						if (!g_cfg_ra.username.get().empty() && !g_cfg_ra.token.get().empty())
							fn_attempt_login(0);
					}
					else
					{
						ra_log.warning("_RA_AttemptLogin not found in DLL, falling back to token login");
						if (!g_cfg_ra.username.get().empty() && !g_cfg_ra.token.get().empty())
							login_with_token(g_cfg_ra.username.get(), g_cfg_ra.token.get());
					}

#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
					typedef void (*RA_InstallBreakpointFunctions_t)(
						void (*)(unsigned int),
						void (*)(unsigned int),
						void (*)(void (*)(unsigned int, unsigned int, unsigned int))
					);
					auto fn_bp = reinterpret_cast<RA_InstallBreakpointFunctions_t>(
						GetProcAddress(hDLL, "_RA_InstallBreakpointFunctions"));
					if (fn_bp)
					{
						fn_bp(
							[](unsigned int nAddress) {
								g_breakpoint_handler.AddBreakpoint(nAddress, breakpoint_types::bp_write);
							},
							[](unsigned int nAddress) {
								g_breakpoint_handler.RemoveBreakpoint(nAddress);
							},
							[](void (*callback)(unsigned int, unsigned int, unsigned int)) {
								ra_set_write_bp_callback(callback);
							}
						);
						ra_log.notice("Installed breakpoint functions");
					}
#endif
				}
			}
			break;
		case RC_MISSING_VALUE:
			ra_log.notice("RAIntegration DLL not found - toolkit unavailable");
			if (!g_cfg_ra.username.get().empty() && !g_cfg_ra.token.get().empty())
				login_with_token(g_cfg_ra.username.get(), g_cfg_ra.token.get());
			break;
		default:
			ra_log.error("RAIntegration DLL load failed: %s", error_message);
			rc_client_set_host(client, "");
			if (!g_cfg_ra.username.get().empty() && !g_cfg_ra.token.get().empty())
				login_with_token(g_cfg_ra.username.get(), g_cfg_ra.token.get());
			break;
		}
	}

	void set_main_window(void* hwnd)
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;
		s_main_hwnd = static_cast<HWND>(hwnd);
		rc_client_raintegration_update_main_window_handle(s_client, s_main_hwnd);
	}

	void load_integration(void* hwnd)
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;
		s_main_hwnd = static_cast<HWND>(hwnd);

		wchar_t dll_dir[MAX_PATH];
		GetModuleFileNameW(nullptr, dll_dir, MAX_PATH);
		PathRemoveFileSpecW(dll_dir);

		rc_client_begin_load_raintegration(s_client,
			dll_dir,
			s_main_hwnd,
			"RPCS3",
			rpcs3::get_version().to_string().c_str(),
			raintegration_load_callback,
			nullptr);
	}

	std::vector<RAIntegrationMenuItem> get_menu_items()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return {};
		const rc_client_raintegration_menu_t* menu = rc_client_raintegration_get_menu(s_client);
		if (!menu)
			return {};
		std::vector<RAIntegrationMenuItem> result;
		result.reserve(menu->num_items);
		for (uint32_t i = 0; i < menu->num_items; i++)
			result.push_back({ menu->items[i].id, menu->items[i].label ? menu->items[i].label : "", menu->items[i].checked != 0, menu->items[i].enabled != 0 });
		return result;
	}

	void activate_menu_item(uint32_t id)
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;
		rc_client_raintegration_activate_menu_item(s_client, id);
	}

	void cancel_hc_enable()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;
		rc_client_set_hardcore_enabled(s_client, 0);
		g_cfg_ra.hardcore.set(false);
		g_cfg_ra.save();
	}

	bool query_client_hardcore_state()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return false;
		return rc_client_get_hardcore_enabled(s_client) != 0;
	}
#endif // RC_CLIENT_SUPPORTS_RAINTEGRATION

} // namespace rpcs3::ra

#endif // RPCS3_RA_ENABLED
