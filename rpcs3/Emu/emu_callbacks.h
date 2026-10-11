#pragma once

#include "util/atomic.hpp"
#include "util/shared_ptr.hpp"

#include <functional>

enum class localized_string_id;

namespace cfg
{
	class _base;
}

struct emu_callbacks
{
	std::function<void(std::function<void()>, atomic_t<u32>*)> call_from_main_thread;
	std::function<void(bool)> on_run; // (start_playtime) continuing or going ingame, so start the clock
	std::function<void()> on_pause;
	std::function<void()> on_resume;
	std::function<void()> on_stop;
	std::function<void()> on_ready;
	std::function<void()> on_missing_fw;
	std::function<void(std::shared_ptr<atomic_t<bool>>, int)> on_emulation_stop_no_response;
	std::function<void(std::shared_ptr<atomic_t<bool>>, stx::shared_ptr<utils::serial>, stx::atomic_ptr<std::string>*, std::shared_ptr<void>)> on_save_state_progress;
	std::function<void(bool enabled)> enable_disc_eject;
	std::function<void(bool enabled)> enable_disc_insert;
	std::function<bool(bool, std::function<void()>)> try_to_quit; // (force_quit, on_exit) Try to close RPCS3
	std::function<void(s32, s32)> handle_taskbar_progress; // (type, value) type: 0 for reset, 1 for increment, 2 for set_limit, 3 for set_value
	std::function<void()> init_kb_handler;
	std::function<void()> init_mouse_handler;
	std::function<void(std::string_view title_id)> init_pad_handler;
	std::function<void()> update_emu_settings;
	std::function<void()> save_emu_settings;
	std::function<void()> close_gs_frame;
	std::function<std::unique_ptr<class GSFrameBase>()> get_gs_frame;
	std::function<std::shared_ptr<class camera_handler_base>()> get_camera_handler;
	std::function<std::shared_ptr<class music_handler_base>()> get_music_handler;
	std::function<void(utils::serial*)> init_gs_render;
	std::function<std::shared_ptr<class AudioBackend>()> get_audio;
	std::function<std::shared_ptr<class audio_device_enumerator>(u64)> get_audio_enumerator; // (audio_renderer)
	std::function<std::shared_ptr<class MsgDialogBase>()> get_msg_dialog;
	std::function<std::shared_ptr<class OskDialogBase>()> get_osk_dialog;
	std::function<std::unique_ptr<class SaveDialogBase>()> get_save_dialog;
	std::function<std::shared_ptr<class SendMessageDialogBase>()> get_sendmessage_dialog;
	std::function<std::shared_ptr<class RecvMessageDialogBase>()> get_recvmessage_dialog;
	std::function<std::unique_ptr<class TrophyNotificationBase>()> get_trophy_notification_dialog;
	std::function<std::string(localized_string_id, const char*)> get_localized_string;
	std::function<std::u32string(localized_string_id, const char*)> get_localized_u32string;
	std::function<std::string(const cfg::_base*, u32)> get_localized_setting;
	std::function<std::string(std::string_view)> get_photo_path;
	std::function<void(const std::string&, std::optional<f32>)> play_sound;
	std::function<bool(const std::string&, std::string&, s32&, s32&, s32&)> get_image_info; // (filename, sub_type, width, height, CellSearchOrientation)
	std::function<bool(const std::string&, s32, s32, s32&, s32&, u8*, bool)> get_scaled_image; // (filename, target_width, target_height, width, height, dst, force_fit)
	std::function<std::string(std::string_view)> resolve_path = [](std::string_view arg){ return std::string{arg}; }; // Resolve path using Qt (returns empty string if the file doesn't exist)
	std::function<std::string(std::string_view)> resolve_path_may_not_exist = [](std::string_view arg){ return std::string{arg}; }; // Resolve path using Qt
	std::function<std::vector<std::string>()> get_font_dirs;
	std::function<bool(const std::vector<std::string>&, bool)> on_install_pkgs;
	std::function<void(u32)> add_breakpoint;
	std::function<bool()> display_sleep_control_supported;
	std::function<void(bool)> enable_display_sleep;
	std::function<void()> check_microphone_permissions;
	std::function<std::unique_ptr<class video_source>()> make_video_source;
	std::function<void(bool)> enable_gamemode;
	std::function<std::string(const std::string&)> get_database_config;
};

extern emu_callbacks g_emu_callbacks;
