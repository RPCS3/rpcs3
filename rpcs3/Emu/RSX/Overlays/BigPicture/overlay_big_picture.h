#pragma once

#include "Emu/RSX/Overlays/overlays.h"
#include "overlay_big_picture_main_menu.h"

namespace rsx
{
	namespace overlays
	{
		struct big_picture_dialog : public user_interface
		{
		public:
			big_picture_dialog();

			void update(u64 timestamp_us) override;
			void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;

			compiled_resource get_compiled() override;

			void show();

		private:
			big_picture_main_menu m_main_menu;
			overlay_element m_dim_background{};
			label m_description{};

			animation_color_interpolate fade_animation{};
		};

		// Entry point run on the dedicated thread started by Emulator::BootBigPictureMode().
		// Blocks until Big Picture Mode's shell is torn down (either the user exited it, or picked a game to start).
		void open_big_picture_mode();
	}
}

// True while the currently running/booting game was launched from Big Picture Mode.
// Checked by the Qt front-end so exiting the game re-enters Big Picture Mode instead of the bare game list.
extern atomic_t<bool> g_big_picture_mode_active;
