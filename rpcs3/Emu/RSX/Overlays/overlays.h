#pragma once
#include "overlay_animation.h"
#include "overlay_controls.h"

#include "Emu/IdManager.h"

#include "Utilities/mutex.h"
#include "Utilities/Timer.h"

#include "../Common/bitfield.hpp"

#include <mutex>
#include <set>


// Definition of user interface implementations
namespace rsx
{
	namespace overlays
	{
		enum pad_button : u8
		{
			dpad_up = 0,
			dpad_down,
			dpad_left,
			dpad_right,
			select,
			start,
			ps,
			triangle,
			circle,
			square,
			cross,
			L1,
			R1,
			L2,
			R2,
			L3,
			R3,

			ls_up,
			ls_down,
			ls_left,
			ls_right,
			rs_up,
			rs_down,
			rs_left,
			rs_right,

			pad_button_max_enum
		};

		// Bitfield of UI signals to overlay manager
		enum status_bits : u32
		{
			invalidate_image_cache = 0x0001, // Flush the address-based image cache
		};

		// Non-interactable UI element
		struct overlay
		{
			u32 uid = umax;
			u32 type_index = umax;

			static constexpr u16 virtual_width = 1280;
			static constexpr u16 virtual_height = 720;

			u32 min_refresh_duration_us = 16600;
			atomic_t<bool> visible = false;
			atomic_bitmask_t<status_bits> status_flags = {};

			virtual ~overlay() = default;

			virtual void update() {}
			virtual compiled_resource get_compiled() = 0;

			void refresh() const;
		};

		// Interactable UI element
		class user_interface : public overlay
		{
		public:
			// Move this somewhere to avoid duplication
			enum selection_code
			{
				new_save = -1,
				canceled = -2,
				error = -3
			};

		protected:
			Timer m_input_timer;
			static constexpr u64 m_auto_repeat_ms_interval_default = 200;
			u64 m_auto_repeat_ms_interval = m_auto_repeat_ms_interval_default;
			std::set<u8> m_auto_repeat_buttons = {
				pad_button::dpad_up,
				pad_button::dpad_down,
				pad_button::dpad_left,
				pad_button::dpad_right,
				pad_button::rs_up,
				pad_button::rs_down,
				pad_button::rs_left,
				pad_button::rs_right,
				pad_button::ls_up,
				pad_button::ls_down,
				pad_button::ls_left,
				pad_button::ls_right
			};

			atomic_t<bool> m_stop_input_loop = false;
			atomic_t<bool> m_interactive = false;
			bool m_start_pad_interception = true;
			atomic_t<bool> m_stop_pad_interception = false;
			atomic_t<u64> thread_bits = 0;
			bool m_keyboard_input_enabled = false; // Allow keyboard input
			bool m_keyboard_pad_handler_active = true; // Initialized as true to prevent keyboard input until proven otherwise.
			bool m_allow_input_on_pause = false;

			static thread_local u64 g_thread_bit;

			u64 alloc_thread_bit();

			std::function<void(s32 status)> on_close = nullptr;

			class thread_bits_allocator
			{
			public:
				thread_bits_allocator(user_interface* parent)
					: m_parent(parent)
				{
					m_thread_bit = m_parent->alloc_thread_bit();
					g_thread_bit = m_thread_bit;
				}

				~thread_bits_allocator()
				{
					m_parent->thread_bits &= ~m_thread_bit;
					m_parent->thread_bits.notify_all();
				}

			private:
				user_interface* m_parent;
				u64 m_thread_bit;
			};
		public:
			s32 return_code = 0; // CELL_OK

			void update() override {}

			compiled_resource get_compiled() override = 0;

			virtual void on_button_pressed(pad_button /*button_press*/) {}
			virtual void on_key_pressed(u32 /*led*/, u32 /*mkey*/, u32 /*key_code*/, u32 /*out_key_code*/, bool /*pressed*/, std::u32string /*key*/) {}

			virtual void close(bool use_callback, bool stop_pad_interception);

			s32 run_input_loop();
		};
	}
}
