#pragma once
#include "overlay_animation.h"
#include "overlay_controls.h"

#include "Emu/Io/pad_types.h"

#include "Utilities/Timer.h"

#include "../Common/bitfield.hpp"

#include <mutex>
#include <set>


// Definition of user interface implementations
namespace rsx
{
	namespace overlays
	{
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

			virtual void update(u64 /*timestamp_us*/) {}
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
				ok = 0,
				new_save = -1,
				canceled = -2,
				error = -3,
				interrupted = -4,
				no = -5
			};

			static constexpr u64 m_auto_repeat_ms_interval_default = 200;

		protected:
			Timer m_input_timer;
			u64 m_auto_repeat_ms_interval = m_auto_repeat_ms_interval_default;
			std::set<pad_button> m_auto_repeat_buttons = {
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
			atomic_t<bool> m_input_thread_detached = false;
			atomic_t<u32> thread_bits = 0;
			bool m_keyboard_input_enabled = false; // Allow keyboard input
			bool m_keyboard_pad_handler_active = true; // Initialized as true to prevent keyboard input until proven otherwise.
			bool m_allow_input_on_pause = false;

			static thread_local u32 g_thread_bit;

			u32 alloc_thread_bit();

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
				u32 m_thread_bit;
			};
		public:
			s32 return_code = 0; // CELL_OK

			bool is_detached() const { return m_input_thread_detached; }
			void detach_input() { m_input_thread_detached.store(true); }

			compiled_resource get_compiled() override = 0;

			virtual void on_button_pressed(pad_button /*button_press*/, bool /*is_auto_repeat*/) {}
			virtual void on_key_pressed(u32 /*led*/, u32 /*mkey*/, u32 /*key_code*/, u32 /*out_key_code*/, bool /*pressed*/, std::u32string /*key*/) {}

			virtual void close(bool use_callback, bool stop_pad_interception);

			s32 run_input_loop(std::function<bool()> check_state = nullptr);
		};

		struct text_guard_t
		{
			std::mutex mutex;
			std::string text;
			bool dirty{false};

			void set_text(std::string t)
			{
				std::lock_guard lock(mutex);
				text = std::move(t);
				dirty = true;
			}

			std::pair<bool, std::string> get_text()
			{
				if (dirty)
				{
					std::lock_guard lock(mutex);
					dirty = false;
					return { true, std::move(text) };
				}

				return { false, {} };
			}
		};
	}
}
