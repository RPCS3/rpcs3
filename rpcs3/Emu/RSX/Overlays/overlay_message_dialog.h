#pragma once

#include "overlays.h"
#include "overlay_progress_bar.hpp"
#include "overlay_gauge_button.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"

namespace rsx
{
	namespace overlays
	{
		class message_dialog : public user_interface
		{
			msg_dialog_source m_source = msg_dialog_source::_cellMsgDialog;

			label text_display;
			image_button btn_ok;
			image_button btn_cancel;

			overlay_element bottom_bar;
			overlay_element background;
			image_view background_poster;
			image_view background_overlay_poster;
			std::array<progress_bar, 2> progress_bars{};
			u8 num_progress_bars = 0;
			s32 taskbar_index = 0;
			s32 taskbar_limit = 0;

			bool interactive = false;
			bool ok_only = false;
			bool cancel_only = false;

			bool custom_background_allowed = false;
			u32 background_blur_strength = 0;
			u32 background_darkening_strength = 0;
			std::unique_ptr<image_info> background_image;
			std::unique_ptr<image_info> background_overlay_image;

			animation_color_interpolate fade_animation;

			text_guard_t text_guard{};
			std::array<text_guard_t, 2> bar_text_guard{};

			// Hold-to-skip gauge button
			static constexpr f32 gauge_fill_duration_us = 1'500'000.f;
			static constexpr f32 gauge_drain_duration_us = 1'000'000.f;

			gauge_button btn_hold_to_skip;
			u64 m_gauge_timestamp_us = 0;
			atomic_t<bool> m_gauge_enabled = false;
			atomic_t<bool> m_gauge_held = false;
			std::mutex m_gauge_mutex;
			std::function<void()> m_gauge_callback;

			void update_gauge(u64 timestamp_us);

		public:
			message_dialog(bool allow_custom_background = false);

			compiled_resource get_compiled() override;

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;
			void on_button_released(pad_button button_release) override;
			void close(bool use_callback, bool stop_pad_interception) override;

			error_code show(bool is_blocking, const std::string& text, const MsgDialogType& type, msg_dialog_source source, std::function<void(s32 status)> on_close);

			// Show or hide the hold-to-skip gauge. Holding the confirm button fills it and invokes the callback.
			void show_gauge_button(bool state, std::function<void()> callback = {});

			void set_text(std::string text);
			void update_custom_background();

			u32 progress_bar_count() const;
			void progress_bar_set_taskbar_index(s32 index);
			error_code progress_bar_set_message(u32 index, std::string msg);
			error_code progress_bar_increment(u32 index, f32 value);
			error_code progress_bar_set_value(u32 index, f32 value);
			error_code progress_bar_reset(u32 index);
			error_code progress_bar_set_limit(u32 index, u32 limit);

			msg_dialog_source source() const { return m_source; }
		};
	}
}
