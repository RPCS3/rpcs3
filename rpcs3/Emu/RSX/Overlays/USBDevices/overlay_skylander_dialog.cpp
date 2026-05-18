#include "stdafx.h"
#include "../overlay_manager.h"
#include "overlay_skylander_dialog.h"
#include "Emu/Io/Skylander.h"
#include "Emu/System.h"
#include "Emu/VFS.h"

namespace rsx
{
	namespace overlays
	{
		static constexpr u16 sky_list_y = 85;
		static constexpr u16 sky_list_h = 540;

		skylander_dialog::skylander_list_entry::skylander_list_entry(std::string name)
		{
			std::unique_ptr<overlay_element> label_text = std::make_unique<label>(name);
			std::unique_ptr<overlay_element> label_load = std::make_unique<label>("Load");
			std::unique_ptr<overlay_element> load_image = std::make_unique<image_view>(30, 30);
			std::unique_ptr<overlay_element> label_clear = std::make_unique<label>("Clear");
			std::unique_ptr<overlay_element> clear_image = std::make_unique<image_view>(30, 30);
			std::unique_ptr<overlay_element> label_create = std::make_unique<label>("Create");
			std::unique_ptr<overlay_element> create_image = std::make_unique<image_view>(30, 30);

			label_text->set_size(130, 30);
			label_text->set_font("Arial", 16);
			label_text->set_wrap_text(true);
			label_text->back_color.a = 0.f;

			label_load->set_size(100, 30);
			label_load->set_font("Arial", 16);
			label_load->set_wrap_text(true);
			label_load->back_color.a = 0.f;

			label_clear->set_size(100, 30);
			label_clear->set_font("Arial", 16);
			label_clear->set_wrap_text(true);
			label_clear->back_color.a = 0.f;

			label_create->set_size(100, 30);
			label_create->set_font("Arial", 16);
			label_create->set_wrap_text(true);
			label_create->back_color.a = 0.f;

			static_cast<image_view*>(load_image.get())->set_image_resource(resource_config::standard_image_resource::cross);
			static_cast<image_view*>(clear_image.get())->set_image_resource(resource_config::standard_image_resource::square);
			static_cast<image_view*>(create_image.get())->set_image_resource(resource_config::standard_image_resource::triangle);

			this->pack_padding = 15;
			add_element(label_text);
			add_element(load_image);
			add_element(label_load);
			add_element(clear_image);
			add_element(label_clear);
			add_element(create_image);
			add_element(label_create);
		}

		skylander_dialog::skylander_dialog()
		{
			m_allow_input_on_pause = true;

			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(virtual_width, virtual_height);
			m_dim_background->back_color.a = 0.9f;

			m_description = std::make_unique<label>();
			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text("Emulated Skylander Portal");
			m_description->auto_resize();
			m_description->back_color.a = 0.f;

			fade_animation.duration_sec = 0.15f;

			return_code = selection_code::canceled;
		}

		void skylander_dialog::update(u64 timestamp_us)
		{
			if (fade_animation.active)
			{
				fade_animation.update(timestamp_us);
			}
		}

		void skylander_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (fade_animation.active)
				return;

			bool close_dialog = false;

			switch (button_press)
			{
			case pad_button::circle:
				play_sound(sound_effect::cancel);
				close_dialog = true;
				break;
			case pad_button::cross:
				// load
				break;
			case pad_button::square:
				// clear
				clear_skylander(m_list->get_selected_index());
				break;
			case pad_button::triangle:
				// create
				break;
			case pad_button::dpad_up:
			case pad_button::ls_up:
				m_list->select_previous();
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
				m_list->select_next();
				break;
			default:
				rsx_log.trace("[ui] Button %d pressed", static_cast<u8>(button_press));
				break;
			}

			if (close_dialog)
			{
				fade_animation.current = color4f(1.f);
				fade_animation.end = color4f(0.f);
				fade_animation.active = true;

				fade_animation.on_finish = [this]
				{
					close(true, true);
				};
			}
			// Play a sound unless this is a fast auto repeat which would induce a nasty noise
			else if (!is_auto_repeat || m_auto_repeat_ms_interval >= m_auto_repeat_ms_interval_default)
			{
				play_sound(sound_effect::cursor);
			}
		}

		compiled_resource skylander_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background->get_compiled());
			if (m_list_dirty.exchange(false))
			{
				reload();
			}
			if (m_list)
			{
				result.add(m_list->get_compiled());
			}
			result.add(m_description->get_compiled());

			fade_animation.apply(result);

			return result;
		}

		void skylander_dialog::show()
		{
			visible = false;

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			visible = true;

			const auto notify = std::make_shared<atomic_t<u32>>(0);
			auto& overlayman = g_fxo->get<display_manager>();

			overlayman.attach_thread_input(
				uid, "Skylander dialog",
				[notify]()
				{
					*notify = true;
					notify->notify_one();
				});

			while (!Emu.IsStopped() && !*notify)
			{
				notify->wait(0, atomic_wait_timeout{1'000'000});
			}
		}

		void skylander_dialog::reload()
		{
			s32 selected_index = m_list ? m_list->get_selected_index() : 0;

			std::vector<std::unique_ptr<overlay_element>> entries;
			for (u8 sky_slot = 0; sky_slot < 8; ++sky_slot)
			{
				if (const auto& slot_infos = g_skyportal.get_skylander(sky_slot))
				{
					const auto& [portal_slot, sky_id, sky_var] = slot_infos.value();
					const auto found_sky = list_skylanders.find(std::make_pair(sky_id, sky_var));
					if (found_sky != list_skylanders.cend())
					{
						entries.emplace_back(std::make_unique<skylander_list_entry>(found_sky->second));
					}
					else
					{
						entries.emplace_back(std::make_unique<skylander_list_entry>(fmt::format("Unknown (Id:%d Var:%d)", sky_id, sky_var)));
					}
				}
				else
				{
					entries.emplace_back(std::make_unique<skylander_list_entry>("None"));
				}
			}

			// Recreate list
			if (m_list)
			{
				status_flags |= status_bits::invalidate_image_cache;
			}

			m_list = std::make_unique<list_view>(virtual_width - 2 * 20, sky_list_h);
			m_list->set_pos(20, sky_list_y);
			m_list->set_cancel_only(true);

			for (auto& entry : entries)
			{
				m_list->add_entry(entry);
			}

			if (!m_list->m_items.empty())
			{
				m_list->select_entry(selected_index);
			}

			m_description->set_text("Emulated Skylander Portal");
			m_description->auto_resize();
		}

		void skylander_dialog::clear_skylander(u8 sky_slot)
		{
			ensure(sky_slot < 8);

			if (const auto& slot_infos = g_skyportal.get_skylander(sky_slot))
			{
				const auto& [cur_slot, id, var] = slot_infos.value();
				if (g_skyportal.remove_skylander(cur_slot))
				{
					m_list_dirty = true;
				}
			}
		}
	} // namespace overlays
} // namespace rsx
