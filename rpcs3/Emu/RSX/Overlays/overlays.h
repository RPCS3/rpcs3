#pragma once
#include "overlay_animation.h"
#include "overlay_controls.h"

#include "../../../Utilities/date_time.h"
#include "../../../Utilities/Thread.h"
#include "../../Io/PadHandler.h"
#include "Emu/Memory/vm.h"
#include "Emu/IdManager.h"
#include "Input/pad_thread.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/Modules/cellSaveData.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/Cell/Modules/cellOskDialog.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"
#include "Utilities/CPUStats.h"
#include "Utilities/Timer.h"

#include <list>

// Utils
std::string utf8_to_ascii8(const std::string& utf8_string);
std::string utf16_to_ascii8(const std::u16string& utf16_string);
std::u16string ascii8_to_utf16(const std::string& ascii_string);
extern u64 get_system_time();

// Definition of user interface implementations
namespace rsx
{
	namespace overlays
	{
		// Non-interactable UI element
		struct overlay
		{
			u32 uid = UINT32_MAX;
			u32 type_index = UINT32_MAX;

			u16 virtual_width = 1280;
			u16 virtual_height = 720;

			u32 min_refresh_duration_us = 16600;

			virtual ~overlay() = default;

			virtual void update() {}
			virtual compiled_resource get_compiled() = 0;

			void refresh();
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

			enum pad_button : u8
			{
				dpad_up = 0,
				dpad_down,
				dpad_left,
				dpad_right,
				select,
				start,
				triangle,
				circle,
				square,
				cross,
				L1,
				R1,

				pad_button_max_enum
			};

		protected:
			Timer input_timer;
			atomic_t<bool> exit{ false };

			std::function<void(s32 status)> on_close;

			shared_mutex m_threadpool_mutex;
			std::list<std::thread> m_workers;

		public:
			s32 return_code = CELL_OK;

		public:
			void update() override {}

			compiled_resource get_compiled() override = 0;

			virtual void on_button_pressed(pad_button /*button_press*/) {}

			void close(bool use_callback = true);

			s32 run_input_loop();
		};

		class display_manager
		{
		private:
			atomic_t<u32> m_uid_ctr = 0;
			std::vector<std::shared_ptr<overlay>> m_iface_list;
			std::vector<std::shared_ptr<overlay>> m_dirty_list;

			shared_mutex m_list_mutex;
			std::vector<u32> m_uids_to_remove;
			std::vector<u32> m_type_ids_to_remove;

			bool remove_type(u32 type_id)
			{
				bool found = false;
				for (auto It = m_iface_list.begin(); It != m_iface_list.end();)
				{
					if (It->get()->type_index == type_id)
					{
						m_dirty_list.push_back(std::move(*It));
						It = m_iface_list.erase(It);
						found = true;
					}
					else
					{
						++It;
					}
				}

				return found;
			}

			bool remove_uid(u32 uid)
			{
				for (auto It = m_iface_list.begin(); It != m_iface_list.end(); It++)
				{
					const auto e = It->get();
					if (e->uid == uid)
					{
						m_dirty_list.push_back(std::move(*It));
						m_iface_list.erase(It);
						return true;
					}
				}

				return false;
			}

			void cleanup_internal()
			{
				for (const auto &uid : m_uids_to_remove)
				{
					remove_uid(uid);
				}

				for (const auto &type_id : m_type_ids_to_remove)
				{
					remove_type(type_id);
				}

				m_uids_to_remove.clear();
				m_type_ids_to_remove.clear();
			}

		public:
			// Disable default construction to make it conditionally available in g_fxo
			explicit display_manager(int) noexcept
			{
			}

			// Adds an object to the internal list. Optionally removes other objects of the same type.
			// Original handle loses ownership but a usable pointer is returned
			template <typename T>
			std::shared_ptr<T> add(std::shared_ptr<T>& entry, bool remove_existing = true)
			{
				std::lock_guard lock(m_list_mutex);

				entry->uid = m_uid_ctr.fetch_add(1);
				entry->type_index = id_manager::typeinfo::get_index<T>();

				if (remove_existing)
				{
					for (auto It = m_iface_list.begin(); It != m_iface_list.end(); It++)
					{
						if (It->get()->type_index == entry->type_index)
						{
							// Replace
							m_dirty_list.push_back(std::move(*It));
							*It = std::move(entry);
							return std::static_pointer_cast<T>(*It);
						}
					}
				}

				m_iface_list.push_back(std::move(entry));
				return std::static_pointer_cast<T>(m_iface_list.back());
			}

			// Allocates object and adds to internal list. Returns pointer to created object
			template <typename T, typename ...Args>
			std::shared_ptr<T> create(Args&&... args)
			{
				auto object = std::make_shared<T>(std::forward<Args>(args)...);
				return add(object);
			}

			// Removes item from list if it matches the uid
			void remove(u32 uid)
			{
				if (m_list_mutex.try_lock())
				{
					remove_uid(uid);
					m_list_mutex.unlock();
				}
				else
				{
					m_uids_to_remove.push_back(uid);
				}
			}

			// Removes all objects of this type from the list
			template <typename T>
			void remove()
			{
				const auto type_id = id_manager::typeinfo::get_index<T>();
				if (m_list_mutex.try_lock())
				{
					remove_type(type_id);
					m_list_mutex.unlock();
				}
				else
				{
					m_type_ids_to_remove.push_back(type_id);
				}
			}

			// True if any visible elements to draw exist
			bool has_visible() const
			{
				return !m_iface_list.empty();
			}

			// True if any elements have been deleted but their resources may not have been cleaned up
			bool has_dirty() const
			{
				return !m_dirty_list.empty();
			}

			// Returns current list for reading. Caller must ensure synchronization by first locking the list
			const std::vector<std::shared_ptr<overlay>>& get_views() const
			{
				return m_iface_list;
			}

			// Returns current list of removed objects not yet deallocated for reading.
			// Caller must ensure synchronization by first locking the list
			const std::vector<std::shared_ptr<overlay>>& get_dirty() const
			{
				return m_dirty_list;
			}

			// Deallocate object. Object must first be removed via the remove() functions
			void dispose(const std::vector<u32>& uids)
			{
				std::lock_guard lock(m_list_mutex);

				if (!m_uids_to_remove.empty() || !m_type_ids_to_remove.empty())
				{
					cleanup_internal();
				}

				m_dirty_list.erase
				(
					std::remove_if(m_dirty_list.begin(), m_dirty_list.end(), [&uids](std::shared_ptr<overlay>& e)
					{
						return std::find(uids.begin(), uids.end(), e->uid) != uids.end();
					}),
					m_dirty_list.end()
				);
			}

			// Returns pointer to the object matching the given uid
			std::shared_ptr<overlay> get(u32 uid)
			{
				reader_lock lock(m_list_mutex);

				for (const auto& iface : m_iface_list)
				{
					if (iface->uid == uid)
						return iface;
				}

				return {};
			}

			// Returns pointer to the first object matching the given type
			template <typename T>
			std::shared_ptr<T> get()
			{
				reader_lock lock(m_list_mutex);

				const auto type_id = id_manager::typeinfo::get_index<T>();
				for (const auto& iface : m_iface_list)
				{
					if (iface->type_index == type_id)
					{
						return std::static_pointer_cast<T>(iface);
					}
				}

				return {};
			}

			// Lock for read-only access (BasicLockable)
			void lock()
			{
				m_list_mutex.lock_shared();
			}

			// Release read-only lock (BasicLockable). May perform internal cleanup before returning
			void unlock()
			{
				m_list_mutex.unlock_shared();

				if (!m_uids_to_remove.empty() || !m_type_ids_to_remove.empty())
				{
					std::lock_guard lock(m_list_mutex);
					cleanup_internal();
				}
			}
		};

		struct osk_dialog : public user_interface, public OskDialogBase
		{
			using callback_t = std::function<void(const std::string&)>;

			enum border_flags
			{
				top = 1,
				bottom = 2,
				left = 4,
				right = 8,

				start_cell = top | bottom | left,
				end_cell = top | bottom | right,
				middle_cell = top | bottom,
				default_cell = top | bottom | left | right
			};

			enum button_flags
			{
				_default = 0,
				_return = 1,
				_space = 2
			};

			struct cell
			{
				position2u pos;
				color4f backcolor{};
				border_flags flags = default_cell;
				bool selected = false;
				bool enabled = false;

				std::vector<std::string> outputs;
				callback_t callback;
			};

			struct grid_entry_ctor
			{
				std::vector<std::string> outputs;
				color4f color;
				u32 num_cell_hz;
				button_flags type_flags;
				callback_t callback;
			};

			// Base UI
			overlay_element m_frame;
			overlay_element m_background;
			label m_title;
			edit_text m_preview;
			image_button m_btn_accept;
			image_button m_btn_cancel;
			image_button m_btn_shift;
			image_button m_btn_space;
			image_button m_btn_delete;

			// Grid
			u32 cell_size_x = 0;
			u32 cell_size_y = 0;
			u32 num_columns = 0;
			u32 num_rows = 0;
			u32 num_layers = 0;
			u32 selected_x = 0;
			u32 selected_y = 0;
			u32 selected_z = 0;

			std::vector<cell> m_grid;

			// Fade in/out
			animation_color_interpolate fade_animation;

			bool m_visible = false;
			bool m_update = true;
			compiled_resource m_cached_resource;

			u32 flags = 0;
			u32 char_limit = UINT32_MAX;

			osk_dialog() = default;
			~osk_dialog() override = default;

			void Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 options) override = 0;
			void Close(bool ok) override;

			void initialize_layout(const std::vector<grid_entry_ctor>& layout, const std::string& title, const std::string& initial_text);
			void update() override;

			void on_button_pressed(pad_button button_press) override;
			void on_text_changed();

			void on_default_callback(const std::string&);
			void on_shift(const std::string&);
			void on_space(const std::string&);
			void on_backspace(const std::string&);
			void on_enter(const std::string&);

			compiled_resource get_compiled() override;
		};

		struct osk_latin : osk_dialog
		{
			using osk_dialog::osk_dialog;

			void Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 options) override;
		};

		struct save_dialog : public user_interface
		{
		private:
			struct save_dialog_entry : horizontal_layout
			{
			private:
				std::unique_ptr<image_info> icon_data;

			public:
				save_dialog_entry(const std::string& text1, const std::string& text2, const std::string& text3, u8 resource_id, const std::vector<u8>& icon_buf);
			};

			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			std::unique_ptr<label> m_time_thingy;
			std::unique_ptr<label> m_no_saves_text;

			bool m_no_saves = false;

		public:
			save_dialog();

			void update() override;
			void on_button_pressed(pad_button button_press) override;

			compiled_resource get_compiled() override;

			s32 show(std::vector<SaveDataEntry>& save_entries, u32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet);
		};

		struct message_dialog : public user_interface
		{
		private:
			label text_display;
			image_button btn_ok;
			image_button btn_cancel;

			overlay_element bottom_bar, background;
			image_view background_poster;
			progress_bar progress_1, progress_2;
			u8 num_progress_bars = 0;
			s32 taskbar_index = 0;
			s32 taskbar_limit = 0;

			bool interactive = false;
			bool ok_only = false;
			bool cancel_only = false;

			std::unique_ptr<image_info> background_image;

		public:
			message_dialog(bool use_custom_background = false);

			compiled_resource get_compiled() override;

			void on_button_pressed(pad_button button_press) override;

			error_code show(bool is_blocking, const std::string& text, const MsgDialogType& type, std::function<void(s32 status)> on_close);

			u32 progress_bar_count();
			void progress_bar_set_taskbar_index(s32 index);
			error_code progress_bar_set_message(u32 index, const std::string& msg);
			error_code progress_bar_increment(u32 index, f32 value);
			error_code progress_bar_reset(u32 index);
			error_code progress_bar_set_limit(u32 index, u32 limit);
		};

		struct trophy_notification : public user_interface
		{
		private:
			overlay_element frame;
			image_view image;
			label text_view;

			u64 display_sched_id = 0;
			u64 creation_time = 0;
			std::unique_ptr<image_info> icon_info;

			animation_translate sliding_animation;

		public:
			trophy_notification();

			void update() override;

			compiled_resource get_compiled() override;

			s32 show(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer);
		};

		struct shader_compile_notification : user_interface
		{
			label m_text;

			overlay_element dots[3];
			u8 current_dot = 255;

			u64 creation_time = 0;
			u64 expire_time = 0; // Time to end the prompt
			u64 urgency_ctr = 0; // How critical it is to show to the user

			shader_compile_notification();

			void update_animation(u64 t);
			void touch();
			void update() override;

			compiled_resource get_compiled() override;
		};
	}
}
