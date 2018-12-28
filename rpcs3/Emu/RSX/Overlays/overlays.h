#pragma once
#include "overlay_controls.h"

#include "../../../Utilities/date_time.h"
#include "../../../Utilities/Thread.h"
#include "../../Io/PadHandler.h"
#include "Emu/Memory/vm.h"
#include "Emu/IdManager.h"
#include "pad_thread.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/Modules/cellSaveData.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"
#include "Utilities/CPUStats.h"
#include "Utilities/Timer.h"

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
		struct user_interface : overlay
		{
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
				triangle,
				circle,
				square,
				cross
			};

			Timer input_timer;
			bool  exit = false;

			s32 return_code = CELL_OK;
			std::function<void(s32 status)> on_close;

			virtual void update() override {}
			virtual compiled_resource get_compiled() override = 0;

			virtual void on_button_pressed(pad_button /*button_press*/)
			{
				close();
			}

			void close();

			s32 run_input_loop()
			{
				std::array<std::chrono::steady_clock::time_point, CELL_PAD_MAX_PORT_NUM> timestamp;
				timestamp.fill(std::chrono::steady_clock::now());

				std::array<std::array<bool, 8>, CELL_PAD_MAX_PORT_NUM> button_state;
				for (auto& state : button_state)
				{
					state.fill(true);
				}

				input_timer.Start();

				while (!exit)
				{
					if (Emu.IsStopped())
						return selection_code::canceled;

					std::this_thread::sleep_for(1ms);

					std::lock_guard lock(pad::g_pad_mutex);

					const auto handler = pad::get_current_handler();

					const PadInfo& rinfo = handler->GetInfo();

					if (Emu.IsPaused() || !rinfo.now_connect)
					{
						continue;
					}

					int pad_index = -1;
					for (const auto &pad : handler->GetPads())
					{
						if (++pad_index >= CELL_PAD_MAX_PORT_NUM)
						{
							LOG_FATAL(RSX, "The native overlay cannot handle more than 7 pads! Current number of pads: %d", pad_index + 1);
							continue;
						}

						for (auto &button : pad->m_buttons)
						{
							u8 button_id = 255;
							if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL1)
							{
								switch (button.m_outKeyCode)
								{
								case CELL_PAD_CTRL_LEFT:
									button_id = pad_button::dpad_left;
									break;
								case CELL_PAD_CTRL_RIGHT:
									button_id = pad_button::dpad_right;
									break;
								case CELL_PAD_CTRL_DOWN:
									button_id = pad_button::dpad_down;
									break;
								case CELL_PAD_CTRL_UP:
									button_id = pad_button::dpad_up;
									break;
								}
							}
							else if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
							{
								switch (button.m_outKeyCode)
								{
								case CELL_PAD_CTRL_TRIANGLE:
									button_id = pad_button::triangle;
									break;
								case CELL_PAD_CTRL_CIRCLE:
									button_id = g_cfg.sys.enter_button_assignment == enter_button_assign::circle ? pad_button::cross : pad_button::circle;
									break;
								case CELL_PAD_CTRL_SQUARE:
									button_id = pad_button::square;
									break;
								case CELL_PAD_CTRL_CROSS:
									button_id = g_cfg.sys.enter_button_assignment == enter_button_assign::circle ? pad_button::circle : pad_button::cross;
									break;
								}
							}

							if (button_id < 255)
							{
								if (button.m_pressed)
								{
									if (button_id < 4) // d-pad button
									{
										if (!button_state[pad_index][button_id] || input_timer.GetMsSince(timestamp[pad_index]) > 400)
										{
											// d-pad button was not pressed, or was pressed more than 400ms ago
											timestamp[pad_index] = std::chrono::steady_clock::now();
											on_button_pressed(static_cast<pad_button>(button_id));
										}
									}
									else if (!button_state[pad_index][button_id])
									{
										// button was not pressed
										on_button_pressed(static_cast<pad_button>(button_id));
									}
								}

								button_state[pad_index][button_id] = button.m_pressed;
							}

							if (button.m_flush)
							{
								button.m_pressed = false;
								button.m_flush = false;
								button.m_value = 0;
							}

							if (exit)
								return 0;
						}
					}

					refresh();
				}

				// Unreachable
				return 0;
			}
		};

		class display_manager
		{
		private:
			atomic_t<u32> m_uid_ctr { 0u };
			std::vector<std::unique_ptr<overlay>> m_iface_list;
			std::vector<std::unique_ptr<overlay>> m_dirty_list;

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
			display_manager() {}
			~display_manager() {}

			// Adds an object to the internal list. Optionally removes other objects of the same type.
			// Original handle loses ownership but a usable pointer is returned
			template <typename T>
			T* add(std::unique_ptr<T>& entry, bool remove_existing = true)
			{
				std::lock_guard lock(m_list_mutex);

				T* e = entry.get();
				e->uid = m_uid_ctr.fetch_add(1);
				e->type_index = id_manager::typeinfo::get_index<T>();

				if (remove_existing)
				{
					for (auto It = m_iface_list.begin(); It != m_iface_list.end(); It++)
					{
						if (It->get()->type_index == e->type_index)
						{
							// Replace
							m_dirty_list.push_back(std::move(*It));
							It->reset(e);
							entry.reset();
							return e;
						}
					}
				}

				m_iface_list.push_back(std::move(entry));
				return e;
			}

			// Allocates object and adds to internal list. Returns pointer to created object
			template <typename T, typename ...Args>
			T* create(Args&&... args)
			{
				std::unique_ptr<T> object = std::make_unique<T>(std::forward<Args>(args)...);
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
			const std::vector<std::unique_ptr<overlay>>& get_views() const
			{
				return m_iface_list;
			}

			// Returns current list of removed objects not yet deallocated for reading.
			// Caller must ensure synchronization by first locking the list
			const std::vector<std::unique_ptr<overlay>>& get_dirty() const
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
					std::remove_if(m_dirty_list.begin(), m_dirty_list.end(), [&uids](std::unique_ptr<overlay>& e)
					{
						return std::find(uids.begin(), uids.end(), e->uid) != uids.end();
					}),
					m_dirty_list.end()
				);
			}

			// Returns pointer to the object matching the given uid
			overlay* get(u32 uid)
			{
				reader_lock lock(m_list_mutex);

				for (const auto& iface : m_iface_list)
				{
					if (iface->uid == uid)
						return iface.get();
				}

				return nullptr;
			}

			// Returns pointer to the first object matching the given type
			template <typename T>
			T* get()
			{
				reader_lock lock(m_list_mutex);

				const auto type_id = id_manager::typeinfo::get_index<T>();
				for (const auto& iface : m_iface_list)
				{
					if (iface->type_index == type_id)
					{
						return static_cast<T*>(iface.get());
					}
				}

				return nullptr;
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

		struct perf_metrics_overlay : overlay
		{
		private:
			/*
			   minimal - fps
			   low - fps, total cpu usage
			   medium - fps, detailed cpu usage
			   high - fps, frametime, detailed cpu usage, thread number, rsx load
			 */
			detail_level m_detail;

			screen_quadrant m_quadrant;
			positioni m_position;

			label m_body;
			label m_titles;

			CPUStats m_cpu_stats;
			Timer m_update_timer;
			u32 m_update_interval; // in ms
			u32 m_frames{ 0 };
			std::string m_font;
			u32 m_font_size;
			u32 m_margin_x; // horizontal distance to the screen border relative to the screen_quadrant in px
			u32 m_margin_y; // vertical distance to the screen border relative to the screen_quadrant in px
			f32 m_opacity;  // 0..1

			bool m_force_update;
			bool m_is_initialised{ false };

			const std::string title1_medium{"CPU Utilization:"};
			const std::string title1_high{"Host Utilization (CPU):"};
			const std::string title2{"Guest Utilization (PS3):"};

			void reset_transform(label& elm) const;
			void reset_transforms();
			void reset_body();
			void reset_titles();
			void reset_text();

		public:
			void init();

			void set_detail_level(detail_level level);
			void set_position(screen_quadrant pos);
			void set_update_interval(u32 update_interval);
			void set_font(std::string font);
			void set_font_size(u32 font_size);
			void set_margins(u32 margin_x, u32 margin_y);
			void set_opacity(f32 opacity);
			void force_next_update();

			void update() override;

			compiled_resource get_compiled() override
			{
				auto result = m_body.get_compiled();
				result.add(m_titles.get_compiled());
				return result;
			}
		};

		struct save_dialog : public user_interface
		{
		private:
			struct save_dialog_entry : horizontal_layout
			{
			private:
				std::unique_ptr<image_info> icon_data;

			public:
				save_dialog_entry(const char* text1, const char* text2, u8 resource_id, const std::vector<u8>& icon_buf)
				{
					std::unique_ptr<overlay_element> image = std::make_unique<image_view>();
					image->set_size(160, 110);
					image->set_padding(36, 36, 11, 11); // Square image, 88x88

					if (resource_id != image_resource_id::raw_image)
					{
						static_cast<image_view*>(image.get())->set_image_resource(resource_id);
					}
					else if (icon_buf.size())
					{
						image->set_padding(0, 0, 11, 11); // Half sized icon, 320x176->160x88
						icon_data = std::make_unique<image_info>(icon_buf);
						static_cast<image_view*>(image.get())->set_raw_image(icon_data.get());
					}
					else
					{
						// Fallback
						static_cast<image_view*>(image.get())->set_image_resource(resource_config::standard_image_resource::save);
					}

					std::unique_ptr<overlay_element> text_stack = std::make_unique<vertical_layout>();
					std::unique_ptr<overlay_element> padding = std::make_unique<spacer>();
					std::unique_ptr<overlay_element> header_text = std::make_unique<label>(text1);
					std::unique_ptr<overlay_element> subtext = std::make_unique<label>(text2);

					padding->set_size(1, 1);
					header_text->set_size(800, 40);
					header_text->set_text(text1);
					header_text->set_font("Arial", 16);
					header_text->set_wrap_text(true);
					subtext->set_size(800, 40);
					subtext->set_text(text2);
					subtext->set_font("Arial", 14);
					subtext->set_wrap_text(true);

					// Auto-resize save details label
					static_cast<label*>(subtext.get())->auto_resize(true);

					// Make back color transparent for text
					header_text->back_color.a = 0.f;
					subtext->back_color.a = 0.f;

					static_cast<vertical_layout*>(text_stack.get())->pack_padding = 5;
					static_cast<vertical_layout*>(text_stack.get())->add_element(padding);
					static_cast<vertical_layout*>(text_stack.get())->add_element(header_text);
					static_cast<vertical_layout*>(text_stack.get())->add_element(subtext);

					// Pack
					this->pack_padding = 15;
					add_element(image);
					add_element(text_stack);
				}
			};

			std::unique_ptr<overlay_element> m_dim_background;
			std::unique_ptr<list_view> m_list;
			std::unique_ptr<label> m_description;
			std::unique_ptr<label> m_time_thingy;
			std::unique_ptr<label> m_no_saves_text;

			bool m_no_saves = false;

		public:
			save_dialog()
			{
				m_dim_background = std::make_unique<overlay_element>();
				m_dim_background->set_size(1280, 720);

				m_list = std::make_unique<list_view>(1240, 540);
				m_description = std::make_unique<label>();
				m_time_thingy = std::make_unique<label>();

				m_list->set_pos(20, 85);

				m_description->set_font("Arial", 20);
				m_description->set_pos(20, 37);
				m_description->set_text("Save Dialog");

				m_time_thingy->set_font("Arial", 14);
				m_time_thingy->set_pos(1000, 30);
				m_time_thingy->set_text(date_time::current_time());

				static_cast<label*>(m_description.get())->auto_resize();
				static_cast<label*>(m_time_thingy.get())->auto_resize();

				m_dim_background->back_color.a = 0.8f;
				m_description->back_color.a = 0.f;
				m_time_thingy->back_color.a = 0.f;

				return_code = selection_code::canceled;
			}

			void on_button_pressed(pad_button button_press) override
			{
				switch (button_press)
				{
				case pad_button::cross:
					if (m_no_saves)
						break;
					return_code = m_list->get_selected_index();
					// Fall through
				case pad_button::circle:
					close();
					break;
				case pad_button::dpad_up:
					m_list->select_previous();
					break;
				case pad_button::dpad_down:
					m_list->select_next();
					break;
				default:
					LOG_TRACE(RSX, "[ui] Button %d pressed", (u8)button_press);
				}
			}

			compiled_resource get_compiled() override
			{
				compiled_resource result;
				result.add(m_dim_background->get_compiled());
				result.add(m_list->get_compiled());
				result.add(m_description->get_compiled());
				result.add(m_time_thingy->get_compiled());

				if (m_no_saves)
					result.add(m_no_saves_text->get_compiled());

				return result;
			}

			s32 show(std::vector<SaveDataEntry>& save_entries, u32 op, vm::ptr<CellSaveDataListSet> listSet)
			{
				std::vector<u8> icon;
				std::vector<std::unique_ptr<overlay_element>> entries;

				for (auto& entry : save_entries)
				{
					std::unique_ptr<overlay_element> e;
					e = std::make_unique<save_dialog_entry>(entry.title.c_str(), (entry.subtitle + " - " + entry.details).c_str(), image_resource_id::raw_image, entry.iconBuf);
					entries.emplace_back(std::move(e));
				}

				if (op >= 8)
				{
					m_description->set_text("Delete Save");
				}
				else if (op & 1)
				{
					m_description->set_text("Load Save");
				}
				else
				{
					m_description->set_text("Save");
				}

				const bool newpos_head = listSet->newData && listSet->newData->iconPosition == CELL_SAVEDATA_ICONPOS_HEAD;

				if (!newpos_head)
				{
					for (auto& entry : entries)
					{
						m_list->add_entry(entry);
					}
				}

				if (listSet->newData)
				{
					std::unique_ptr<overlay_element> new_stub;

					const char* title = "Create New";

					int id = resource_config::standard_image_resource::new_entry;

					if (auto picon = +listSet->newData->icon)
					{
						if (picon->title)
							title = picon->title.get_ptr();

						if (picon->iconBuf && picon->iconBufSize && picon->iconBufSize <= 225280)
						{
							const auto iconBuf = static_cast<u8*>(picon->iconBuf.get_ptr());
							const auto iconEnd = iconBuf + picon->iconBufSize;
							icon.assign(iconBuf, iconEnd);
						}
					}

					if (!icon.empty())
					{
						id = image_resource_id::raw_image;
					}

					new_stub = std::make_unique<save_dialog_entry>(title, "Select to create a new entry", id, icon);

					m_list->add_entry(new_stub);
				}

				if (newpos_head)
				{
					for (auto& entry : entries)
					{
						m_list->add_entry(entry);
					}
				}

				if (!m_list->m_items.size())
				{
					m_no_saves_text = std::make_unique<label>();
					m_no_saves_text->set_font("Arial", 20);
					m_no_saves_text->align_text(overlay_element::text_align::center);
					m_no_saves_text->set_pos(m_list->x, m_list->y + m_list->h / 2);
					m_no_saves_text->set_size(m_list->w, 30);
					m_no_saves_text->set_text("There is no saved data.");
					m_no_saves_text->back_color.a = 0;

					m_no_saves = true;
					m_list->set_cancel_only(true);
				}

				static_cast<label*>(m_description.get())->auto_resize();

				if (auto err = run_input_loop())
					return err;

				if (return_code == entries.size() && !newpos_head)
					return selection_code::new_save;
				if (return_code >= 0 && newpos_head)
					return return_code - 1;

				return return_code;
			}

			void update() override
			{
				m_time_thingy->set_text(date_time::current_time());
				static_cast<label*>(m_time_thingy.get())->auto_resize();
			}
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
			message_dialog(bool use_custom_background = false)
			{
				background.set_size(1280, 720);
				background.back_color.a = 0.85f;

				text_display.set_size(1100, 40);
				text_display.set_pos(90, 364);
				text_display.set_font("Arial", 16);
				text_display.align_text(overlay_element::text_align::center);
				text_display.set_wrap_text(true);
				text_display.back_color.a = 0.f;

				bottom_bar.back_color = color4f(1.f, 1.f, 1.f, 1.f);
				bottom_bar.set_size(1200, 2);
				bottom_bar.set_pos(40, 400);

				progress_1.set_size(800, 4);
				progress_2.set_size(800, 4);
				progress_1.back_color = color4f(0.25f, 0.f, 0.f, 0.85f);
				progress_2.back_color = color4f(0.25f, 0.f, 0.f, 0.85f);

				btn_ok.set_text("Yes");
				btn_ok.set_size(140, 30);
				btn_ok.set_pos(545, 420);
				btn_ok.set_font("Arial", 16);

				btn_cancel.set_text("No");
				btn_cancel.set_size(140, 30);
				btn_cancel.set_pos(685, 420);
				btn_cancel.set_font("Arial", 16);

				if (g_cfg.sys.enter_button_assignment == enter_button_assign::circle)
				{
					btn_ok.set_image_resource(resource_config::standard_image_resource::circle);
					btn_cancel.set_image_resource(resource_config::standard_image_resource::cross);
				}
				else
				{
					btn_ok.set_image_resource(resource_config::standard_image_resource::cross);
					btn_cancel.set_image_resource(resource_config::standard_image_resource::circle);
				}

				if (use_custom_background)
				{
					auto icon_path = Emu.GetSfoDir() + "/PIC1.PNG";
					if (!fs::exists(icon_path))
					{
						// Fallback path
						icon_path = Emu.GetSfoDir() + "/ICON0.PNG";
					}

					if (fs::exists(icon_path))
					{
						background_image = std::make_unique<image_info>(icon_path.c_str());
						if (background_image->data)
						{
							f32 color = (100 - g_cfg.video.shader_preloading_dialog.darkening_strength) / 100.f;
							background_poster.fore_color = color4f(color, color, color, 1.);
							background.back_color.a = 0.f;

							background_poster.set_size(1280, 720);
							background_poster.set_raw_image(background_image.get());
							background_poster.set_blur_strength((u8)g_cfg.video.shader_preloading_dialog.blur_strength);
						}
					}
				}

				return_code = CELL_MSGDIALOG_BUTTON_NONE;
			}

			compiled_resource get_compiled() override
			{
				compiled_resource result;

				if (background_image && background_image->data)
				{
					result.add(background_poster.get_compiled());
				}

				result.add(background.get_compiled());
				result.add(text_display.get_compiled());

				if (num_progress_bars > 0)
				{
					result.add(progress_1.get_compiled());
				}

				if (num_progress_bars > 1)
				{
					result.add(progress_2.get_compiled());
				}

				if (interactive)
				{
					if (!num_progress_bars)
						result.add(bottom_bar.get_compiled());

					if (!cancel_only)
						result.add(btn_ok.get_compiled());

					if (!ok_only)
						result.add(btn_cancel.get_compiled());
				}

				return result;
			}

			void on_button_pressed(pad_button button_press) override
			{
				switch (button_press)
				{
				case pad_button::cross:
				{
					if (ok_only)
					{
						return_code = CELL_MSGDIALOG_BUTTON_OK;
					}
					else if (cancel_only)
					{
						// Do not accept for cancel-only dialogs
						return;
					}
					else
					{
						return_code = CELL_MSGDIALOG_BUTTON_YES;
					}

					break;
				}
				case pad_button::circle:
				{
					if (ok_only)
					{
						// Ignore cancel operation for Ok-only
						return;
					}
					else if (cancel_only)
					{
						return_code = CELL_MSGDIALOG_BUTTON_ESCAPE;
					}
					else
					{
						return_code = CELL_MSGDIALOG_BUTTON_NO;
					}

					break;
				}
				default:
					return;
				}

				close();
			}

			s32 show(std::string text, const MsgDialogType &type, std::function<void(s32 status)> on_close)
			{
				num_progress_bars = type.progress_bar_count;
				if (num_progress_bars)
				{
					u16 offset = 58;
					progress_1.set_pos(240, 412);

					if (num_progress_bars > 1)
					{
						progress_2.set_pos(240, 462);
						offset = 98;
					}

					// Push the other stuff down
					bottom_bar.translate(0, offset);
					btn_ok.translate(0, offset);
					btn_cancel.translate(0, offset);
				}

				text_display.set_text(text.c_str());

				u16 text_w, text_h;
				text_display.measure_text(text_w, text_h);
				text_display.translate(0, -(text_h - 16));

				switch (type.button_type.unshifted())
				{
				case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE:
					interactive = !type.disable_cancel;
					if (interactive)
					{
						btn_cancel.set_pos(585, btn_cancel.y);
						btn_cancel.set_text("Cancel");
						cancel_only = true;
					}
					break;
				case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK:
					btn_ok.set_pos(600, btn_ok.y);
					btn_ok.set_text("OK");
					interactive = true;
					ok_only = true;
					break;
				case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO:
					interactive = true;
					break;
				}

				this->on_close = on_close;
				if (interactive)
				{
					thread_ctrl::spawn("dialog input thread", [&]
					{
						if (auto error = run_input_loop())
						{
							LOG_ERROR(RSX, "Dialog input loop exited with error code=%d", error);
						}
					});
				}

				return CELL_OK;
			}

			u32 progress_bar_count()
			{
				return num_progress_bars;
			}

			void progress_bar_set_taskbar_index(s32 index)
			{
				taskbar_index = index;
			}

			s32 progress_bar_set_message(u32 index, const std::string& msg)
			{
				if (index >= num_progress_bars)
					return CELL_MSGDIALOG_ERROR_PARAM;

				if (index == 0)
					progress_1.set_text(msg);
				else
					progress_2.set_text(msg);

				return CELL_OK;
			}

			s32 progress_bar_increment(u32 index, f32 value)
			{
				if (index >= num_progress_bars)
					return CELL_MSGDIALOG_ERROR_PARAM;

				if (index == 0)
					progress_1.inc(value);
				else
					progress_2.inc(value);

				if (index == taskbar_index || taskbar_index == -1)
					Emu.GetCallbacks().handle_taskbar_progress(1, (s32)value);

				return CELL_OK;
			}

			s32 progress_bar_reset(u32 index)
			{
				if (index >= num_progress_bars)
					return CELL_MSGDIALOG_ERROR_PARAM;

				if (index == 0)
					progress_1.set_value(0.f);
				else
					progress_2.set_value(0.f);

				Emu.GetCallbacks().handle_taskbar_progress(0, 0);

				return CELL_OK;
			}

			s32 progress_bar_set_limit(u32 index, u32 limit)
			{
				if (index >= num_progress_bars)
					return CELL_MSGDIALOG_ERROR_PARAM;

				if (index == 0)
					progress_1.set_limit((float)limit);
				else
					progress_2.set_limit((float)limit);

				if (index == taskbar_index)
				{
					taskbar_limit = limit;
					Emu.GetCallbacks().handle_taskbar_progress(2, taskbar_limit);
				}
				else if (taskbar_index == -1)
				{
					taskbar_limit += limit;
					Emu.GetCallbacks().handle_taskbar_progress(2, taskbar_limit);
				}

				return CELL_OK;
			}
		};

		struct trophy_notification : public user_interface
		{
		private:
			overlay_element frame;
			image_view image;
			label text_view;

			u64 creation_time = 0;
			std::unique_ptr<image_info> icon_info;

		public:
			trophy_notification()
			{
				frame.set_pos(0, 0);
				frame.set_size(260, 80);
				frame.back_color.a = 0.85f;

				image.set_pos(8, 8);
				image.set_size(64, 64);
				image.back_color.a = 0.f;

				text_view.set_pos(85, 0);
				text_view.set_padding(0, 0, 24, 0);
				text_view.set_font("Arial", 8);
				text_view.align_text(overlay_element::text_align::center);
				text_view.back_color.a = 0.f;
			}

			void update() override
			{
				u64 t = get_system_time();
				if (((t - creation_time) / 1000) > 7500)
					close();
			}

			compiled_resource get_compiled() override
			{
				auto result = frame.get_compiled();
				result.add(image.get_compiled());
				result.add(text_view.get_compiled());

				return result;
			}

			s32 show(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer)
			{
				if (trophy_icon_buffer.size())
				{
					icon_info = std::make_unique<image_info>(trophy_icon_buffer);
					image.set_raw_image(icon_info.get());
				}

				std::string trophy_message;
				switch (trophy.trophyGrade)
				{
				case SCE_NP_TROPHY_GRADE_BRONZE: trophy_message = "bronze"; break;
				case SCE_NP_TROPHY_GRADE_SILVER: trophy_message = "silver"; break;
				case SCE_NP_TROPHY_GRADE_GOLD: trophy_message = "gold"; break;
				case SCE_NP_TROPHY_GRADE_PLATINUM: trophy_message = "platinum"; break;
				default: break;
				}

				trophy_message = "You have earned the " + trophy_message + " trophy\n" + trophy.name;
				text_view.set_text(trophy_message);
				text_view.auto_resize();

				//Resize background to cover the text
				u16 margin_sz = text_view.x - image.w - image.x;
				frame.w = text_view.x + text_view.w + margin_sz;

				creation_time = get_system_time();
				return CELL_OK;
			}
		};

		struct shader_compile_notification : user_interface
		{
			label m_text;

			overlay_element dots[3];
			u8 current_dot = 255;

			u64 creation_time = 0;
			u64 expire_time = 0; // Time to end the prompt
			u64 urgency_ctr = 0; // How critical it is to show to the user

			shader_compile_notification()
			{
				const u16 pos_x = g_cfg.video.shader_compilation_hint.pos_x;
				const u16 pos_y = g_cfg.video.shader_compilation_hint.pos_y;

				m_text.set_font("Arial", 16);
				m_text.set_text("Compiling shaders");
				m_text.auto_resize();
				m_text.set_pos(pos_x, pos_y);

				m_text.back_color.a = 0.f;

				for (int n = 0; n < 3; ++n)
				{
					dots[n].set_size(2, 2);
					dots[n].back_color = color4f(1.f, 1.f, 1.f, 1.f);
					dots[n].set_pos(m_text.w + pos_x + 5 + (6 * n), pos_y + 20);
				}

				creation_time = get_system_time();
				expire_time = creation_time + 1000000;

				// Disable forced refresh unless fps dips below 4
				min_refresh_duration_us = 250000;
			}

			void update_animation(u64 t)
			{
				// Update rate is twice per second
				auto elapsed = t - creation_time;
				elapsed /= 500000;

				auto old_dot = current_dot;
				current_dot = elapsed % 3;

				if (old_dot != current_dot)
				{
					if (old_dot != 255)
					{
						dots[old_dot].set_size(2, 2);
						dots[old_dot].translate(0, 1);
					}

					dots[current_dot].translate(0, -1);
					dots[current_dot].set_size(3, 3);
				}
			}

			// Extends visible time by half a second. Also updates the screen
			void touch()
			{
				if (urgency_ctr == 0 || urgency_ctr > 8)
				{
					refresh();
					urgency_ctr = 0;
				}

				expire_time = get_system_time() + 500000;
				urgency_ctr++;
			}

			void update() override
			{
				auto current_time = get_system_time();
				if (current_time > expire_time)
					close();

				update_animation(current_time);

				// Usually this method is called during a draw-to-screen operation. Reset urgency ctr
				urgency_ctr = 1;
			}

			compiled_resource get_compiled() override
			{
				auto compiled = m_text.get_compiled();
				compiled.add(dots[0].get_compiled());
				compiled.add(dots[1].get_compiled());
				compiled.add(dots[2].get_compiled());

				return compiled;
			}
		};
	}
}
