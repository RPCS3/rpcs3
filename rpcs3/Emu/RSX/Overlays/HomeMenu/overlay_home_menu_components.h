#pragma once

#include "Emu/RSX/Overlays/overlays.h"
#include "Emu/RSX/Overlays/overlay_checkbox.h"
#include "Emu/RSX/Overlays/overlay_select.h"
#include "Emu/RSX/Overlays/overlay_slider.h"

#include "Emu/System.h"
#include "Utilities/Config.h"

#include "overlay_home_icons.h"

namespace rsx
{
	namespace overlays
	{
		static constexpr u16 menu_entry_height = 40;
		static constexpr u16 menu_entry_margin = 30;
		static constexpr u16 menu_checkbox_size = 24;
		static constexpr u16 element_height = 25;

		enum class page_navigation
		{
			stay,
			back,
			next,
			exit,
			exit_for_screenshot
		};

		struct home_menu_entry : horizontal_layout
		{
		public:
			home_menu_entry(home_menu::fa_icon icon, const std::string& text, u16 width, text_align alignment = text_align::center);
		};

		template <typename T, typename C>
		struct home_menu_setting : horizontal_layout
		{
		public:
			home_menu_setting(C* setting, const std::string& text)
				: m_setting(setting)
				, m_label_text(text)
			{
				update_value(true);
				auto_resize = false;
			}

			void set_reserved_width(u16 size)
			{
				m_reserved_width = size;
			}

			void set_size(u16 w, u16 h = menu_entry_height) override
			{
				clear_items();

				std::unique_ptr<overlay_element> layout = std::make_unique<vertical_layout>();
				std::unique_ptr<overlay_element> padding = std::make_unique<spacer>();
				std::unique_ptr<overlay_element> title = std::make_unique<label>(m_label_text);

				const u16 available_width = std::max(w, m_reserved_width) - (menu_entry_margin * 2) - m_reserved_width;
				layout->set_size(available_width, 0);

				padding->set_size(1, 1);
				title->set_size(available_width, menu_entry_height);
				title->set_font("Arial", 16);
				title->set_wrap_text(true);
				title->align_text(text_align::left);

				// Make back color transparent for text
				title->back_color.a = 0.f;

				static_cast<vertical_layout*>(layout.get())->auto_resize = true;
				static_cast<vertical_layout*>(layout.get())->pack_padding = 5;
				static_cast<vertical_layout*>(layout.get())->add_element(padding);
				static_cast<vertical_layout*>(layout.get())->add_element(title);

				horizontal_layout::set_size(w, std::max(h, layout->h));
				horizontal_layout::advance_pos = menu_entry_margin / 2;

				// Pack
				this->pack_padding = 15;
				add_element(layout);
			}

			void update_value(bool initializing = false)
			{
				if (m_setting)
				{
					if (const T new_value = m_setting->get(); new_value != m_last_value || initializing)
					{
						m_last_value = new_value;
						m_is_compiled = false;
					}
				}
			}

		protected:
			T m_last_value = {};
			C* m_setting = nullptr;

			u16 m_reserved_width;
			std::string m_label_text;
		};

		struct home_menu_checkbox : public home_menu_setting<bool, cfg::_bool>
		{
		public:
			home_menu_checkbox(cfg::_bool* setting, const std::string& text);

			void set_size(u16 w, u16 h = element_height) override;
			compiled_resource& get_compiled() override;

		private:
			checkbox* m_checkbox = nullptr;
		};

		template <typename T>
		struct home_menu_dropdown : public home_menu_setting<T, cfg::_enum<T>>
		{
		public:
			home_menu_dropdown(cfg::_enum<T>* setting, const std::string& text)
				: home_menu_setting<T, cfg::_enum<T>>(setting, text)
			{
				for (size_t index = 0; index < setting->size(); index++)
				{
					auto translated = Emu.GetCallbacks().get_localized_setting(home_menu_setting<T, cfg::_enum<T>>::m_setting, static_cast<u32>(index));
					m_options.emplace_back(std::move(translated));
				}
			}

			void sync_selection()
			{
				auto setting = home_menu_setting<T, cfg::_enum<T>>::m_setting;
				if (!setting || !m_dropdown)
				{
					return;
				}

				const auto current = fmt::format("%s", setting->get());
				const auto list = setting->to_list();
				for (s32 index = 0; index <= static_cast<s32>(list.size()); ++index)
				{
					if (list[index] != current)
					{
						continue;
					}

					if (index != m_dropdown->get_selected_index())
					{
						m_dropdown->select_item(index);
					}
					break;
				}
			}

			void set_size(u16 w, u16 h = element_height) override
			{
				auto dropdown = std::make_unique<overlays::select>(w / 2, element_height, m_options);
				dropdown->auto_resize();

				home_menu_setting<T, cfg::_enum<T>>::set_reserved_width(dropdown->w + menu_entry_margin);
				home_menu_setting<T, cfg::_enum<T>>::set_size(w, h);

				// Center horizontally
				dropdown->set_pos(0, this->compute_vertically_centered(dropdown.get()));
				m_dropdown = horizontal_layout::add_element(dropdown);

				// Select the correct item
				sync_selection();
			}

			bool is_popup_visible() const
			{
				return m_popup_visible;
			}

			s32 get_selected_index() const
			{
				return m_dropdown->get_selected_index();
			}

			void open_popup(const overlay_element* parent = nullptr)
			{
				m_previous_selection = m_dropdown->get_selected_index();
				m_popup_visible = true;

				if (!parent)
				{
					m_dropdown->get_popup()->set_pos(m_dropdown->x, m_dropdown->y + m_dropdown->h);
					return;
				}

				u16 dropdown_y = m_dropdown->y;

				// Check if the parent is a layout that can scroll
				if (auto container = dynamic_cast<const vertical_layout*>(parent))
				{
					// Apply the scroll (convert coordinate to view space from container space)
					dropdown_y = std::max<u16>(dropdown_y, container->scroll_offset_value) - container->scroll_offset_value;
				}

				const int space_above = static_cast<int>(dropdown_y) - std::min<int>(parent->y, dropdown_y);
				const int space_below = std::max<int>(parent->y + parent->h, dropdown_y) - dropdown_y;

				const int popup_height = m_dropdown->get_popup()->h;
				u16 popup_x = m_dropdown->x, popup_y = parent->y;
				if (space_below >= popup_height)
				{
					popup_y = dropdown_y + m_dropdown->h + 4;
				}
				else if (space_above >= popup_height)
				{
					popup_y = dropdown_y - popup_height - 4;
				}

				m_dropdown->get_popup()->set_pos(popup_x, popup_y);
			}

			void close_popup()
			{
				m_popup_visible = false;
			}

			page_navigation handle_input(pad_button button)
			{
				switch (button)
				{
				case pad_button::circle:
					m_dropdown->select_item(m_previous_selection);
					close_popup();
					return page_navigation::exit;

				case pad_button::cross:
					m_dropdown->select_item(m_dropdown->get_selected_index());
					close_popup();
					return page_navigation::exit;

				case pad_button::dpad_up:
				case pad_button::ls_up:
					m_dropdown->select_previous();
					break;

				case pad_button::dpad_down:
				case pad_button::ls_down:
					m_dropdown->select_next();
					break;

				default:
					break;
				}

				return page_navigation::stay;
			}

			compiled_resource& render_popup()
			{
				if (this->m_popup_visible)
				{
					return m_dropdown->get_popup()->get_compiled();
				}

				m_popup_compiled_resources.clear();
				return m_popup_compiled_resources;
			}

			compiled_resource& get_compiled() override
			{
				this->update_value();

				if (!this->is_compiled())
				{
					sync_selection();
					horizontal_layout::get_compiled();
					this->compiled_resources.add(m_dropdown->get_compiled());
				}

				return this->compiled_resources;
			}

		private:
			std::vector<std::string> m_options;
			select* m_dropdown = nullptr;

			int m_previous_selection = -1;
			bool m_popup_visible = false;
			compiled_resource m_popup_compiled_resources;
		};

		template <typename T, typename C>
		struct home_menu_slider : public home_menu_setting<T, C>
		{
		public:
			home_menu_slider(C* setting, const std::string& text, const std::string& suffix, std::map<T, std::string> special_labels = {}, T minimum = C::min, T maximum = C::max)
				: home_menu_setting<T, C>(setting, text)
				, m_suffix(suffix)
				, m_special_labels(std::move(special_labels))
				, m_minimum(minimum)
				, m_maximum(maximum)
			{}

			void set_size(u16 w, u16 h = element_height) override
			{
				home_menu_setting<T, C>::set_reserved_width(w / 2 + menu_entry_margin);
				home_menu_setting<T, C>::set_size(w, h);

				auto slider_ = std::make_unique<slider>(static_cast<f64>(m_minimum), static_cast<f64>(m_maximum), static_cast<f64>(1));
				slider_->set_size(w / 2, element_height);
				m_slider_component = this->add_element(slider_);
			}

			compiled_resource& get_compiled() override
			{
				this->update_value();

				if (this->is_compiled())
				{
					return this->compiled_resources;
				}

				this->compiled_resources.clear();

				if (!m_slider_component)
				{
					this->m_is_compiled = true;
					return this->compiled_resources;
				}

				auto formatter = [this](f64 value) -> std::string
				{
					const auto typed_value = static_cast<T>(value);
					if (const auto it = m_special_labels.find(typed_value); it != m_special_labels.cend())
					{
						return it->second;
					}

					if constexpr (std::is_floating_point_v<T>)
					{
						return fmt::format("%.2f%s", typed_value, m_suffix);
					}
					else
					{
						return fmt::format("%d%s", typed_value, m_suffix);
					}
				};

				m_slider_component->set_value_format(formatter);
				m_slider_component->set_value(static_cast<f64>(this->m_last_value));

				this->compiled_resources = horizontal_layout::get_compiled();
				this->compiled_resources.add(m_slider_component->get_compiled());

				this->m_is_compiled = true;
				return this->compiled_resources;
			}

		private:
			slider* m_slider_component = nullptr;

			std::string m_suffix;
			std::map<T, std::string> m_special_labels;
			T m_minimum{};
			T m_maximum{};
		};

		template <s64 Min, s64 Max>
		struct home_menu_signed_slider : public home_menu_slider<s64, cfg::_int<Min, Max>>
		{
			using home_menu_slider<s64, cfg::_int<Min, Max>>::home_menu_slider;
		};

		template <u64 Min, u64 Max>
		struct home_menu_unsigned_slider : public home_menu_slider<u64, cfg::uint<Min, Max>>
		{
			using home_menu_slider<u64, cfg::uint<Min, Max>>::home_menu_slider;
		};

		template <s32 Min, s32 Max>
		struct home_menu_float_slider : public home_menu_slider<f64, cfg::_float<Min, Max>>
		{
			using home_menu_slider<f64, cfg::_float<Min, Max>>::home_menu_slider;
		};
	}
}
