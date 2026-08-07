#include "stdafx.h"
#include "RetroAchievements.h"

#ifdef RPCS3_RA_ENABLED

#include "rc_client.h"
#include "rc_consoles.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "util/logs.hpp"
#include "rpcs3_version.h"
#include "ra_config.h"

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

	static u32 read_memory(u32 address, u8* buffer, u32 num_bytes, rc_client_t* /*client*/)
	{
		return vm::try_access(address, buffer, num_bytes, false) ? num_bytes : 0;
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

	static void event_handler(const rc_client_event_t* event, rc_client_t* /*client*/)
	{
		switch (event->type)
		{
		case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
			ra_log.success("Achievement unlocked: %s", event->achievement->title);
			break;
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
				s_game_loaded = true;
			}
			else
			{
				ra_log.warning("Game load failed: %s", error_message ? error_message : "Unknown error");
			}
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
		if (!s_game_loaded)
			return;
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;
		rc_client_do_frame(s_client);
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

	static void raintegration_load_callback(int result, const char* error_message, rc_client_t* client, void* /*userdata*/)
	{
		switch (result)
		{
		case RC_OK:
			ra_log.notice("RAIntegration DLL loaded successfully");
			rc_client_raintegration_set_event_handler(client, raintegration_event_handler);
			rc_client_raintegration_set_write_memory_function(client, raintegration_write_memory);
			rc_client_raintegration_set_get_game_name_function(client, raintegration_get_game_name);
			if (g_cfg_ra.enabled && !g_cfg_ra.username.get().empty() && !g_cfg_ra.token.get().empty())
				login_with_token(g_cfg_ra.username.get(), g_cfg_ra.token.get());
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

	void load_integration(void* hwnd)
	{
		std::lock_guard lock(s_mutex);
		if (!s_client)
			return;

		wchar_t exe_path[MAX_PATH];
		GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
		PathRemoveFileSpecW(exe_path);

		rc_client_begin_load_raintegration(s_client, exe_path,
			static_cast<HWND>(hwnd),
			"RPCS3", rpcs3::get_version().to_string().c_str(),
			raintegration_load_callback, nullptr);
	}
#endif // RC_CLIENT_SUPPORTS_RAINTEGRATION

} // namespace rpcs3::ra

#endif // RPCS3_RA_ENABLED
