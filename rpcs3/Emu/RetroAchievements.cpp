#include "stdafx.h"
#include "RetroAchievements.h"

#ifdef RPCS3_RA_ENABLED

#include "rc_client.h"
#include "rc_consoles.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Emu/RSX/Overlays/overlay_controls.h"
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
	static std::string s_user_agent;
	static HWND s_main_hwnd = nullptr;
	static u32 read_memory(u32 address, u8* buffer, u32 num_bytes, rc_client_t* /*client*/)
	{
		if (vm::try_access(address, buffer, num_bytes, false))
			return num_bytes;

		if (address >= 0x10000000U && address < 0x40000000U)
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

		bool valid() const { return m_img && m_img->w > 0; }
	};

	static void event_handler(const rc_client_event_t* event, rc_client_t* /*client*/)
	{
		switch (event->type)
		{
		case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
		{
			ra_log.success("Achievement unlocked: %s", event->achievement->title);

			const std::string text = std::string("You have earned a trophy.\n") + event->achievement->title;
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
			// Install PS3 memory banks unconditionally — the 3-bank layout is hardware-defined
			// and must be set up for every PS3 game regardless of RA database recognition.
			// Without this, ResetMemory() leaves a single 0x30000000-byte default block.
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

					auto fn_install = reinterpret_cast<RA_InstallMemoryBank_t>(GetProcAddress(hDLL, "_RA_InstallMemoryBank"));
					auto fn_block   = reinterpret_cast<RA_InstallMemoryBankBlockReader_t>(GetProcAddress(hDLL, "_RA_InstallMemoryBankBlockReader"));
					auto fn_clear   = reinterpret_cast<void(*)()>(GetProcAddress(hDLL, "_RA_ClearMemoryBanks"));

					static auto read_bank0 = [](unsigned int offset) -> unsigned char {
						u8 byte = 0;
						vm::try_access(0x00000000 + offset, &byte, 1, false);
						return byte;
					};
					static auto write_bank0 = [](unsigned int offset, unsigned char value) {
						vm::try_access(0x00000000 + offset, &value, 1, true);
					};
					static auto block_bank0 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						return vm::try_access(0x00000000 + offset, buf, size, false) ? size : 0;
					};

					static auto read_bank1 = [](unsigned int offset) -> unsigned char {
						u8 byte = 0;
						vm::try_access(0x10000000 + offset, &byte, 1, false);
						return byte;
					};
					static auto write_bank1 = [](unsigned int offset, unsigned char value) {
						vm::try_access(0x10000000 + offset, &value, 1, true);
					};
					static auto block_bank1 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						if (vm::try_access(0x10000000 + offset, buf, size, false))
							return size;
						// Sparse region: read page-by-page so mapped pages within the chunk
						// are returned correctly instead of zeroing the whole chunk.
						std::memset(buf, 0, size);
						constexpr unsigned int PAGE = 4096U;
						for (unsigned int i = 0; i < size; i += PAGE)
							vm::try_access(0x10000000 + offset + i, buf + i, std::min(PAGE, size - i), false);
						return size;
					};

					static auto read_bank2 = [](unsigned int offset) -> unsigned char {
						u8 byte = 0;
						vm::try_access(0x30000000 + offset, &byte, 1, false);
						return byte;
					};
					static auto write_bank2 = [](unsigned int offset, unsigned char value) {
						vm::try_access(0x30000000 + offset, &value, 1, true);
					};
					static auto block_bank2 = [](unsigned int offset, unsigned char* buf, unsigned int size) -> unsigned int {
						if (vm::try_access(0x30000000 + offset, buf, size, false))
							return size;
						// Same sparse-region fallback as block_bank1.
						std::memset(buf, 0, size);
						constexpr unsigned int PAGE = 4096U;
						for (unsigned int i = 0; i < size; i += PAGE)
							vm::try_access(0x30000000 + offset + i, buf + i, std::min(PAGE, size - i), false);
						return size;
					};

					if (fn_install)
					{
						// ResetMemory() merges all PS3 regions into one 0x30000000 block because
						// the rcheevos map has no Unused gap between User64K and User1M.
						// AddMemoryBlock ignores calls where size != 0, so our sizes would be
						// silently discarded. Clear first so we own the layout entirely.
						if (fn_clear) fn_clear();
						fn_install(0, read_bank0, write_bank0, 0x10000000);
						fn_install(1, read_bank1, write_bank1, 0x20000000); // User64K + gap: keeps RA flat == PS3 VA
						fn_install(2, read_bank2, write_bank2, 0x10000000);
						ra_log.notice("RA: Installed 3 PS3 memory banks (game: %s)",
							result == RC_OK ? "recognized" : "unrecognized");
					}

					if (fn_block)
					{
						fn_block(0, block_bank0);
						fn_block(1, block_bank1);
						fn_block(2, block_bank2);
						ra_log.notice("RA: Installed block readers for 3 PS3 memory banks");
					}
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

			typedef void (*RA_DoAchievementsFrame_t)();
			HMODULE hDLL = GetModuleHandleW(L"RA_Integration-x64.dll");
			if (hDLL)
			{
				auto fn_do_frame = reinterpret_cast<RA_DoAchievementsFrame_t>(
					GetProcAddress(hDLL, "_RA_DoAchievementsFrame"));
				if (fn_do_frame)
					fn_do_frame();
			}
		});
#else
		if (!s_game_loaded)
			return;
		rc_client_do_frame(s_client);
#endif
	}

	void on_pause()
	{
		std::lock_guard lock(s_mutex);
		if (s_client)
			rc_client_idle(s_client);
	}

	void on_resume()
	{
	}

	bool is_active()
	{
		std::lock_guard lock(s_mutex);
		return s_client != nullptr && rc_client_get_user_info(s_client) != nullptr;
	}

	std::string get_token()
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return {};
		const rc_client_user_t* user = rc_client_get_user_info(s_client);
		return user && user->token ? user->token : std::string{};
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

#ifdef RC_CLIENT_SUPPORTS_RAINTEGRATION
	static void raintegration_event_handler(const rc_client_raintegration_event_t* event, rc_client_t* /*client*/)
	{
		switch (event->type)
		{
		case RC_CLIENT_RAINTEGRATION_EVENT_PAUSE:
			Emu.Pause();
			break;
		case RC_CLIENT_RAINTEGRATION_EVENT_HARDCORE_CHANGED:
			ra_log.notice("Hardcore mode changed: %s",
				rc_client_get_hardcore_enabled(s_client) ? "enabled" : "disabled");
			break;
		case RC_CLIENT_RAINTEGRATION_EVENT_MENU_CHANGED:
			// Menu update handled by Qt side
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
			rc_client_raintegration_set_write_memory_function(client, raintegration_write_memory);
			rc_client_raintegration_set_event_handler(client, raintegration_event_handler);
			rc_client_raintegration_set_get_game_name_function(client, raintegration_get_game_name);
			rc_client_raintegration_set_console_id(client, RC_CONSOLE_PLAYSTATION_3);
			// In RAIntegration mode, login is handled by the DLL via _RA_AttemptLogin,
			// not by rc_client_begin_login_with_token (which uses the offline handler and fails).
			{
				HMODULE hDLL = GetModuleHandleW(L"RA_Integration-x64.dll");
				if (hDLL)
				{
					typedef void (*RA_AttemptLogin_t)(int);
					auto fn_attempt_login = reinterpret_cast<RA_AttemptLogin_t>(GetProcAddress(hDLL, "_RA_AttemptLogin"));
					if (fn_attempt_login)
					{
						fn_attempt_login(1);
					}
					else
					{
						ra_log.warning("_RA_AttemptLogin not found in DLL, falling back to token login");
						if (g_cfg_ra.enabled && !g_cfg_ra.username.get().empty() && !g_cfg_ra.token.get().empty())
							login_with_token(g_cfg_ra.username.get(), g_cfg_ra.token.get());
					}

					typedef void (*RA_InstallSharedFunctions_t)(
						bool (*)(void),
						void (*)(void),
						void (*)(void),
						void (*)(void),
						void (*)(char*),
						void (*)(void),
						void (*)(const char*)
					);
					auto fn_shared = reinterpret_cast<RA_InstallSharedFunctions_t>(GetProcAddress(hDLL, "_RA_InstallSharedFunctions"));
					if (fn_shared)
					{
						fn_shared(
							ra_cb_is_active,
							ra_cb_cause_unpause,
							ra_cb_cause_pause,
							ra_cb_rebuild_menu,
							ra_cb_estimate_title,
							ra_cb_reset_emulator,
							ra_cb_load_rom
						);
						ra_log.notice("RA: Installed shared functions");
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
						ra_log.notice("RA: Installed breakpoint functions");
					}
#endif
				}
			}
			break;
		case RC_MISSING_VALUE:
			ra_log.notice("RAIntegration DLL not found - toolkit unavailable");
			if (g_cfg_ra.enabled && !g_cfg_ra.username.get().empty() && !g_cfg_ra.token.get().empty())
				login_with_token(g_cfg_ra.username.get(), g_cfg_ra.token.get());
			break;
		default:
			ra_log.error("RAIntegration DLL load failed: %s", error_message);
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

		// Write credentials to RAPrefs before DLL initialization so the DLL
		// reads them during rc_client_begin_load_raintegration.
		if (!g_cfg_ra.username.get().empty() && !g_cfg_ra.token.get().empty())
		{
			wchar_t prefs_path[MAX_PATH];
			GetModuleFileNameW(nullptr, prefs_path, MAX_PATH);
			PathRemoveFileSpecW(prefs_path);
			std::wstring prefs_file = std::wstring(prefs_path) + L"\\RAPrefs_RPCS3.cfg";

			std::string existing_json;
			{
				FILE* f = nullptr;
				_wfopen_s(&f, prefs_file.c_str(), L"rb");
				if (f)
				{
					fseek(f, 0, SEEK_END);
					long sz = ftell(f);
					fseek(f, 0, SEEK_SET);
					existing_json.resize(sz);
					fread(existing_json.data(), 1, sz, f);
					fclose(f);
				}
			}

			auto replace_json_field = [](std::string& json, const std::string& key, const std::string& value)
			{
				std::string search = "\"" + key + "\":\"";
				auto pos = json.find(search);
				if (pos != std::string::npos)
				{
					auto start = pos + search.size();
					auto end = json.find("\"", start);
					if (end != std::string::npos)
						json.replace(start, end - start, value);
				}
			};

			if (!existing_json.empty())
			{
				replace_json_field(existing_json, "Username", g_cfg_ra.username.get());
				replace_json_field(existing_json, "Token", g_cfg_ra.token.get());

				FILE* f = nullptr;
				_wfopen_s(&f, prefs_file.c_str(), L"wb");
				if (f)
				{
					fwrite(existing_json.data(), 1, existing_json.size(), f);
					fclose(f);
					ra_log.notice("Written credentials to RAPrefs_RPCS3.cfg");
				}
			}
		}

		wchar_t exe_path[MAX_PATH];
		GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
		PathRemoveFileSpecW(exe_path);

		rc_client_begin_load_raintegration(s_client, exe_path,
			static_cast<HWND>(hwnd),
			"RPCS3", rpcs3::get_version().to_string().c_str(),
			raintegration_load_callback, nullptr);
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
			result.push_back({ menu->items[i].id, menu->items[i].label ? menu->items[i].label : "", menu->items[i].checked != 0 });
		return result;
	}

	void activate_menu_item(uint32_t id)
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;
		rc_client_raintegration_activate_menu_item(s_client, id);
	}
#endif // RC_CLIENT_SUPPORTS_RAINTEGRATION

} // namespace rpcs3::ra

#endif // RPCS3_RA_ENABLED
