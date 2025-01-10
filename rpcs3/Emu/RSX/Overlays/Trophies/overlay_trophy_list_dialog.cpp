#include "stdafx.h"
#include "../overlay_manager.h"
#include "overlay_trophy_list_dialog.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"
#include "Emu/System.h"
#include "Emu/VFS.h"

namespace rsx
{
	namespace overlays
	{
		trophy_list_dialog::trophy_list_entry::trophy_list_entry(const std::string& name, const std::string& description, const std::string& trophy_type, const std::string& icon_path, bool hidden, bool locked, bool platinum_relevant)
		{
			std::unique_ptr<overlay_element> image = std::make_unique<image_view>();
			image->set_size(160, 110);
			image->set_padding(36, 36, 11, 11); // Square image, 88x88

			if (fs::exists(icon_path))
			{
				icon_data = std::make_unique<image_info>(icon_path.c_str(), hidden || locked);
				static_cast<image_view*>(image.get())->set_raw_image(icon_data.get());
			}
			else
			{
				// Fallback
				// TODO: use proper icon
				static_cast<image_view*>(image.get())->set_image_resource(resource_config::standard_image_resource::square);
			}

			std::unique_ptr<overlay_element> text_stack  = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding     = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> header_text = std::make_unique<label>(fmt::format("%s (%s%s)", (locked && !hidden) ? get_localized_string(localized_string_id::HOME_MENU_TROPHY_LOCKED_TITLE, name.c_str()) : name, trophy_type, platinum_relevant ? " - " + get_localized_string(localized_string_id::HOME_MENU_TROPHY_PLATINUM_RELEVANT) : ""));
			std::unique_ptr<overlay_element> subtext     = std::make_unique<label>(description);

			padding->set_size(1, 1);
			header_text->set_size(800, 40);
			header_text->set_font("Arial", 16);
			header_text->set_wrap_text(true);

			subtext->set_size(800, 0);
			subtext->set_font("Arial", 14);
			subtext->set_wrap_text(true);
			static_cast<label*>(subtext.get())->auto_resize(true);

			// Make back color transparent for text
			header_text->back_color.a = 0.f;
			subtext->back_color.a     = 0.f;

			static_cast<vertical_layout*>(text_stack.get())->pack_padding = 5;
			static_cast<vertical_layout*>(text_stack.get())->add_element(padding);
			static_cast<vertical_layout*>(text_stack.get())->add_element(header_text);
			static_cast<vertical_layout*>(text_stack.get())->add_element(subtext);

			if (text_stack->h > image->h)
			{
				std::unique_ptr<overlay_element> padding2 = std::make_unique<spacer>();
				padding2->set_size(1, 5);
				static_cast<vertical_layout*>(text_stack.get())->add_element(padding2);
			}

			// Pack
			this->pack_padding = 15;
			add_element(image);
			add_element(text_stack);
		}

		trophy_list_dialog::trophy_list_dialog()
		{
			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(virtual_width, virtual_height);
			m_dim_background->back_color.a = 0.9f;

			m_list = std::make_unique<list_view>(virtual_width - 2 * 20, 540);
			m_list->set_pos(20, 85);
			m_list->set_cancel_only(true);

			m_description = std::make_unique<label>();
			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text("Select trophy"); // Fallback. I don't think this will ever be used, so I won't localize it.
			m_description->auto_resize();
			m_description->back_color.a = 0.f;

			fade_animation.duration_sec = 0.15f;

			return_code = selection_code::canceled;
		}

		void trophy_list_dialog::update(u64 timestamp_us)
		{
			if (fade_animation.active)
			{
				fade_animation.update(timestamp_us);
			}
		}

		void trophy_list_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (fade_animation.active) return;

			bool close_dialog = false;

			switch (button_press)
			{
			case pad_button::circle:
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cancel.wav");
				close_dialog = true;
				break;
			case pad_button::dpad_up:
			case pad_button::ls_up:
				m_list->select_previous();
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
				m_list->select_next();
				break;
			case pad_button::L1:
				m_list->select_previous(10);
				break;
			case pad_button::R1:
				m_list->select_next(10);
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
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cursor.wav");
			}
		}

		compiled_resource trophy_list_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background->get_compiled());
			result.add(m_list->get_compiled());
			result.add(m_description->get_compiled());

			fade_animation.apply(result);

			return result;
		}

		void trophy_list_dialog::show(const std::string& trop_name)
		{
			visible = false;

			std::unique_ptr<trophy_data> data = load_trophies(trop_name);
			ensure(data && data->trop_usr);

			rsx_log.trace("Populating Trophy List Overlay with %s %s", data->game_name, data->path);

			std::vector<std::unique_ptr<overlay_element>> entries;

			const int all_trophies = data->trop_usr->GetTrophiesCount();
			const int unlocked_trophies = data->trop_usr->GetUnlockedTrophiesCount();
			const int percentage = (all_trophies > 0) ? (100 * unlocked_trophies / all_trophies) : 0;

			std::shared_ptr<rXmlNode> trophy_base = data->trop_config.GetRoot();
			if (!trophy_base)
			{
				rsx_log.error("Populating Trophy List Overlay failed (root is null): %s %s", data->game_name, data->path);
			}

			const std::string hidden_title = get_localized_string(localized_string_id::HOME_MENU_TROPHY_HIDDEN_TITLE);
			const std::string hidden_description = get_localized_string(localized_string_id::HOME_MENU_TROPHY_HIDDEN_DESCRIPTION);

			for (std::shared_ptr<rXmlNode> n = trophy_base ? trophy_base->GetChildren() : nullptr; n; n = n->GetNext())
			{
				// Only show trophies.
				if (n->GetName() != "trophy")
				{
					continue;
				}

				// Get data (stolen graciously from sceNpTrophy.cpp)
				SceNpTrophyDetails details{};

				// Get trophy id
				const s32 trophy_id = atoi(n->GetAttribute("id").c_str());
				details.trophyId = trophy_id;

				// Get platinum link id (we assume there only exists one platinum trophy per game for now)
				const s32 platinum_link_id = atoi(n->GetAttribute("pid").c_str());
				const bool platinum_relevant = platinum_link_id >= 0;

				// Get trophy type
				std::string trophy_type;

				switch (n->GetAttribute("ttype")[0])
				{
				case 'B': details.trophyGrade = SCE_NP_TROPHY_GRADE_BRONZE;   trophy_type = get_localized_string(localized_string_id::HOME_MENU_TROPHY_GRADE_BRONZE); break;
				case 'S': details.trophyGrade = SCE_NP_TROPHY_GRADE_SILVER;   trophy_type = get_localized_string(localized_string_id::HOME_MENU_TROPHY_GRADE_SILVER); break;
				case 'G': details.trophyGrade = SCE_NP_TROPHY_GRADE_GOLD;     trophy_type = get_localized_string(localized_string_id::HOME_MENU_TROPHY_GRADE_GOLD); break;
				case 'P': details.trophyGrade = SCE_NP_TROPHY_GRADE_PLATINUM; trophy_type = get_localized_string(localized_string_id::HOME_MENU_TROPHY_GRADE_PLATINUM); break;
				default: rsx_log.warning("Unknown trophy grade %s", n->GetAttribute("ttype")); break;
				}

				// Get hidden state
				const bool hidden = n->GetAttribute("hidden")[0] == 'y';
				details.hidden = hidden;

				// Get name and detail
				if (hidden)
				{
					strcpy_trunc(details.name, hidden_title);
					strcpy_trunc(details.description, hidden_description);
				}
				else
				{
					for (std::shared_ptr<rXmlNode> n2 = n->GetChildren(); n2; n2 = n2->GetNext())
					{
						const std::string name = n2->GetName();
						if (name == "name")
						{
							strcpy_trunc(details.name, n2->GetNodeContent());
						}
						else if (name == "detail")
						{
							strcpy_trunc(details.description, n2->GetNodeContent());
						}
					}
				}

				const bool unlocked = data->trop_usr->GetTrophyUnlockState(trophy_id);
				const auto icon_path_it = data->trophy_image_paths.find(trophy_id);

				std::unique_ptr<overlay_element> entry = std::make_unique<trophy_list_entry>(details.name, details.description, trophy_type, icon_path_it != data->trophy_image_paths.cend() ? icon_path_it->second : "", hidden, !unlocked, platinum_relevant);
				entries.emplace_back(std::move(entry));
			}

			for (auto& entry : entries)
			{
				m_list->add_entry(entry);
			}

			if (!m_list->m_items.empty())
			{
				m_list->select_entry(0);
			}

			m_description->set_text(get_localized_string(localized_string_id::HOME_MENU_TROPHY_LIST_TITLE, fmt::format("%d%% (%d/%d)", percentage, unlocked_trophies, all_trophies).c_str()));
			m_description->auto_resize();

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			visible = true;

			const auto notify = std::make_shared<atomic_t<u32>>(0);
			auto& overlayman = g_fxo->get<display_manager>();

			overlayman.attach_thread_input(
				uid, "Trophy list dialog",
				[notify]() { *notify = true; notify->notify_one(); }
			);

			while (!Emu.IsStopped() && !*notify)
			{
				notify->wait(0, atomic_wait_timeout{1'000'000});
			}
		}

		std::unique_ptr<trophy_data> trophy_list_dialog::load_trophies(const std::string& trop_name) const
		{
			// Populate GameTrophiesData
			std::unique_ptr<trophy_data> game_trophy_data = std::make_unique<trophy_data>();
			game_trophy_data->trop_usr = std::make_unique<TROPUSRLoader>();

			const std::string trophy_path = "/dev_hdd0/home/" + Emu.GetUsr() + "/trophy/" + trop_name;
			const std::string vfs_path = vfs::get(trophy_path + "/");

			if (vfs_path.empty())
			{
				rsx_log.error("Failed to load trophy database for %s. Path empty!", trop_name);
				return game_trophy_data;
			}

			game_trophy_data->path = vfs_path;
			const std::string tropusr_path = trophy_path + "/TROPUSR.DAT";
			const std::string tropconf_path = trophy_path + "/TROPCONF.SFM";
			const bool success = game_trophy_data->trop_usr->Load(tropusr_path, tropconf_path).success;

			fs::file config(vfs::get(tropconf_path));

			if (!success || !config)
			{
				rsx_log.error("Failed to load trophy database for %s", trop_name);
				return game_trophy_data;
			}

			const u32 trophy_count = game_trophy_data->trop_usr->GetTrophiesCount();

			if (trophy_count == 0)
			{
				rsx_log.error("Warning game %s in trophy folder %s usr file reports zero trophies. Cannot load in trophy manager.", game_trophy_data->game_name, game_trophy_data->path);
				return game_trophy_data;
			}

			for (u32 trophy_id = 0; trophy_id < trophy_count; ++trophy_id)
			{
				// A trophy icon has 3 digits from 000 to 999, for example TROP001.PNG
				game_trophy_data->trophy_image_paths[trophy_id] = fmt::format("%sTROP%03d.PNG", game_trophy_data->path, trophy_id);
			}

			// Get game name
			pugi::xml_parse_result res = game_trophy_data->trop_config.Read(config.to_string());
			if (!res)
			{
				rsx_log.error("Failed to read trophy xml: %s", tropconf_path);
				return game_trophy_data;
			}

			std::shared_ptr<rXmlNode> trophy_base = game_trophy_data->trop_config.GetRoot();
			if (!trophy_base)
			{
				rsx_log.error("Failed to read trophy xml (root is null): %s", tropconf_path);
				return game_trophy_data;
			}

			for (std::shared_ptr<rXmlNode> n = trophy_base->GetChildren(); n; n = n->GetNext())
			{
				if (n->GetName() == "title-name")
				{
					game_trophy_data->game_name = n->GetNodeContent();
					break;
				}
			}

			config.release();

			return game_trophy_data;
		}
	} // namespace overlays
} // namespace RSX
