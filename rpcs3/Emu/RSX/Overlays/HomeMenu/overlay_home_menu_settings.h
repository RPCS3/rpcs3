#pragma once

#include "overlay_home_menu_page.h"
#include "../overlay_tabs.h"

#include "Emu/System.h"
#include "Utilities/Config.h"

namespace rsx
{
	namespace overlays
	{
		struct home_menu_settings : public home_menu_page
		{
		public:
			home_menu_settings(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);

			page_navigation handle_button_press(pad_button button_press, bool is_auto_repeat, u64 auto_repeat_interval_ms) override;
			compiled_resource& get_compiled() override;

		private:
			void add_page(home_menu::fa_icon icon, std::shared_ptr<home_menu_page> page) override;

			std::unique_ptr<tabbed_container> m_tabs;
		};

		struct home_menu_settings_page : public home_menu_page
		{
			using home_menu_page::home_menu_page;

			bool show_reset_button() const override { return true; }

			void add_checkbox(cfg::_bool* setting, localized_string_id loc_id)
			{
				ensure(setting && setting->get_is_dynamic());

				const std::string localized_text = get_localized_string(loc_id);
				std::unique_ptr<overlay_element> elem = std::make_unique<home_menu_checkbox>(setting, localized_text);
				elem->set_size(this->w, menu_entry_height);

				add_item(elem, [this, setting](pad_button btn) -> page_navigation
				{
					if (setting)
					{
						bool value = setting->get();

						switch (btn)
						{
						case pad_button::cross:
							value = !value;
							break;
						case pad_button::select:
							value = setting->def;
							break;
						default:
							return page_navigation::stay;
						}

						if (value != setting->get())
						{
							rsx_log.notice("User toggled checkbox in '%s'. Setting '%s' to %d", title, setting->get_name(), value);
							setting->set(value);
							Emu.GetCallbacks().update_emu_settings();
							if (m_config_changed) *m_config_changed = true;
							refresh();
						}
					}

					return page_navigation::stay;
				});
			}

			template <typename T>
			void add_dropdown(cfg::_enum<T>* setting, localized_string_id loc_id)
			{
				ensure(setting && setting->get_is_dynamic());

				const std::string localized_text = get_localized_string(loc_id);
				std::unique_ptr<overlay_element> elem = std::make_unique<home_menu_dropdown<T>>(setting, localized_text);
				elem->set_size(this->w, menu_entry_height);

				add_item(elem, [this, setting, elem = elem.get()](pad_button btn) -> page_navigation
				{
					if (!setting)
					{
						// Nothing we can do with this
						return page_navigation::stay;
					}

					switch (btn)
					{
					case pad_button::cross:
						// We process only 'accept' inputs. The rest are handled by the popup hook.
						break;
					case pad_button::select:
						setting->set(setting->def);
						rsx_log.notice("User reset the value for setting '%s' to default", setting->get_name());
						return page_navigation::stay;
					default:
						return page_navigation::stay;
					}

					// If we're receiving this, we need to open the popup and install the input hook
					auto dropdown = ensure(dynamic_cast<home_menu_dropdown<T>*>(elem));
					dropdown->open_popup(static_cast<const list_view*>(this));

					auto render_fn = [dropdown]() -> compiled_resource&
					{
						return dropdown->render_popup();
					};

					auto input_fn = [this, dropdown, setting](pad_button button) -> page_navigation
					{
						const auto result = dropdown->handle_input(button);

						if (!setting || result != page_navigation::exit)
						{
							switch (button)
							{
							case pad_button::dpad_up:
							case pad_button::ls_up:
							case pad_button::dpad_down:
							case pad_button::ls_down:
								play_sound(sound_effect::cursor);
								break;
							default:
								break;
							}

							return result;
						}

						// We're closing. Check if we should play an accept or reject sound
						play_sound(
							(button == pad_button::cross)
							? sound_effect::accept
							: sound_effect::cancel);

						const auto previous = fmt::format("%s", setting->get());
						const std::vector<std::string> list = setting->to_list();
						const int selected_idx = dropdown->get_selected_index();

						if (const std::string& next_value = ::at32(list, selected_idx); setting->from_string(next_value))
						{
							rsx_log.notice("User toggled dropdown in '%s'. Setting '%s' to %s", title, setting->get_name(), next_value);

							if (next_value != previous)
							{
								Emu.GetCallbacks().update_emu_settings();
								if (m_config_changed)
								{
									*m_config_changed = true;
								}
								refresh();
							}
						}
						else
						{
							rsx_log.error("Can't toggle dropdown in '%s'. Setting '%s' to '%s' failed", title, setting->get_name(), next_value);
						}

						return result;
					};

					m_popup.get_compiled = render_fn;
					m_popup.input_hook = input_fn;

					return page_navigation::stay;
				});
			}

			template <s64 Min, s64 Max>
			void add_signed_slider(cfg::_int<Min, Max>* setting, localized_string_id loc_id, const std::string& suffix, s64 step_size, std::map<s64, std::string> special_labels = {}, s64 minimum = Min, s64 maximum = Max)
			{
				ensure(setting && setting->get_is_dynamic());

				const std::string localized_text = get_localized_string(loc_id);
				std::unique_ptr<overlay_element> elem = std::make_unique<home_menu_signed_slider<Min, Max>>(setting, localized_text, suffix, special_labels, minimum, maximum);
				elem->set_size(this->w, menu_entry_height);

				add_item(elem, [this, setting, step_size, minimum, maximum](pad_button btn) -> page_navigation
				{
					if (setting)
					{
						s64 value = setting->get();
						switch (btn)
						{
						case pad_button::dpad_left:
						case pad_button::ls_left:
							value = std::max(value - step_size, minimum);
							break;
						case pad_button::dpad_right:
						case pad_button::ls_right:
							value = std::min(value + step_size, maximum);
							break;
						case pad_button::select:
							value = setting->def;
							break;
						default:
							return page_navigation::stay;
						}

						if (value != setting->get())
						{
							rsx_log.notice("User toggled signed slider in '%s'. Setting '%s' to %d", title, setting->get_name(), value);
							setting->set(value);
							Emu.GetCallbacks().update_emu_settings();
							if (m_config_changed) *m_config_changed = true;
							refresh();
						}
					}

					return page_navigation::stay;
				});
			}

			template <u64 Min, u64 Max>
			void add_unsigned_slider(cfg::uint<Min, Max>* setting, localized_string_id loc_id, const std::string& suffix, u64 step_size, std::map<u64, std::string> special_labels = {}, const std::set<u64>& exceptions = {}, u64 minimum = Min, u64 maximum = Max)
			{
				ensure(setting && setting->get_is_dynamic());
				ensure(!exceptions.contains(minimum) && !exceptions.contains(maximum));

				const std::string localized_text = get_localized_string(loc_id);
				std::unique_ptr<overlay_element> elem = std::make_unique<home_menu_unsigned_slider<Min, Max>>(setting, localized_text, suffix, special_labels, minimum, maximum);
				elem->set_size(this->w, menu_entry_height);

				add_item(elem, [this, setting, step_size, minimum, maximum, exceptions](pad_button btn) -> page_navigation
				{
					if (setting)
					{
						u64 value = setting->get();
						switch (btn)
						{
						case pad_button::dpad_left:
						case pad_button::ls_left:
							do
							{
								value = step_size > value ? minimum : std::max(value - step_size, minimum);
							}
							while (exceptions.contains(value));
							break;
						case pad_button::dpad_right:
						case pad_button::ls_right:
							do
							{
								value = std::min(value + step_size, maximum);
							}
							while (exceptions.contains(value));
							break;
						case pad_button::select:
							value = setting->def;
							break;
						default:
							return page_navigation::stay;
						}

						if (value != setting->get())
						{
							rsx_log.notice("User toggled unsigned slider in '%s'. Setting '%s' to %d", title, setting->get_name(), value);
							setting->set(value);
							Emu.GetCallbacks().update_emu_settings();
							if (m_config_changed) *m_config_changed = true;
							refresh();
						}
					}

					return page_navigation::stay;
				});
			}

			template <s32 Min, s32 Max>
			void add_float_slider(cfg::_float<Min, Max>* setting, localized_string_id loc_id, const std::string& suffix, f32 step_size, std::map<f64, std::string> special_labels = {}, s32 minimum = Min, s32 maximum = Max)
			{
				ensure(setting && setting->get_is_dynamic());

				const std::string localized_text = get_localized_string(loc_id);
				std::unique_ptr<overlay_element> elem = std::make_unique<home_menu_float_slider<Min, Max>>(setting, localized_text, suffix, special_labels, minimum, maximum);
				elem->set_size(this->w, menu_entry_height);

				add_item(elem, [this, setting, step_size, minimum, maximum](pad_button btn) -> page_navigation
				{
					if (setting)
					{
						f64 value = setting->get();
						switch (btn)
						{
						case pad_button::dpad_left:
						case pad_button::ls_left:
							value = std::max(value - step_size, static_cast<f64>(minimum));
							break;
						case pad_button::dpad_right:
						case pad_button::ls_right:
							value = std::min(value + step_size, static_cast<f64>(maximum));
							break;
						case pad_button::select:
							value = setting->def;
							break;
						default:
							return page_navigation::stay;
						}

						if (value != setting->get())
						{
							rsx_log.notice("User toggled float slider in '%s'. Setting '%s' to %.2f", title, setting->get_name(), value);
							setting->set(value);
							Emu.GetCallbacks().update_emu_settings();
							if (m_config_changed) *m_config_changed = true;
							refresh();
						}
					}

					return page_navigation::stay;
				});
			}
		};

		struct home_menu_settings_audio : public home_menu_settings_page
		{
			home_menu_settings_audio(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);
		};

		struct home_menu_settings_video : public home_menu_settings_page
		{
			home_menu_settings_video(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);
		};

		struct home_menu_settings_advanced : public home_menu_settings_page
		{
			home_menu_settings_advanced(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);
		};

		struct home_menu_settings_input : public home_menu_settings_page
		{
			home_menu_settings_input(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);
		};

		struct home_menu_settings_overlays : public home_menu_settings_page
		{
			home_menu_settings_overlays(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);
		};

		struct home_menu_settings_performance_overlay : public home_menu_settings_page
		{
			home_menu_settings_performance_overlay(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);
		};

		struct home_menu_settings_debug : public home_menu_settings_page
		{
			home_menu_settings_debug(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent);
		};
	}
}
