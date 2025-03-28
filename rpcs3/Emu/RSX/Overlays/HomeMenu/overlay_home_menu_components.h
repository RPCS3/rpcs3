#pragma once

#include "Emu/RSX/Overlays/overlays.h"
#include "Emu/System.h"
#include "Utilities/Config.h"

namespace rsx
{
	namespace overlays
	{
		static constexpr u16 menu_entry_height = 40;
		static constexpr u16 menu_entry_margin = 20;
		static constexpr u16 available_side_width = (overlay::virtual_width - 6 * menu_entry_margin) / 2;
		static constexpr u16 element_height = 25;

		enum class page_navigation
		{
			stay,
			back,
			next,
			exit
		};

		struct home_menu_entry : horizontal_layout
		{
		public:
			home_menu_entry(const std::string& text);
		};

		template <typename T, typename C>
		struct home_menu_setting : horizontal_layout
		{
		public:
			home_menu_setting(C* setting, const std::string& text) : m_setting(setting)
			{
				std::unique_ptr<overlay_element> layout  = std::make_unique<vertical_layout>();
				std::unique_ptr<overlay_element> padding = std::make_unique<spacer>();
				std::unique_ptr<overlay_element> title   = std::make_unique<label>(text);

				padding->set_size(1, 1);
				title->set_size(available_side_width, menu_entry_height);
				title->set_font("Arial", 16);
				title->set_wrap_text(true);
				title->align_text(text_align::right);

				// Make back color transparent for text
				title->back_color.a = 0.f;

				static_cast<vertical_layout*>(layout.get())->pack_padding = 5;
				static_cast<vertical_layout*>(layout.get())->add_element(padding);
				static_cast<vertical_layout*>(layout.get())->add_element(title);

				// Pack
				this->pack_padding = 15;
				add_element(layout);

				update_value(true);
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
		};

		struct home_menu_checkbox : public home_menu_setting<bool, cfg::_bool>
		{
		public:
			home_menu_checkbox(cfg::_bool* setting, const std::string& text);
			compiled_resource& get_compiled() override;
		private:
			overlay_element m_background;
			overlay_element m_checkbox;
		};

		template <typename T>
		struct home_menu_dropdown : public home_menu_setting<T, cfg::_enum<T>>
		{
		public:
			home_menu_dropdown(cfg::_enum<T>* setting, const std::string& text) : home_menu_setting<T, cfg::_enum<T>>(setting, text)
			{
				m_dropdown.set_size(available_side_width / 2, element_height);
				m_dropdown.set_pos(overlay::virtual_width / 2 + menu_entry_margin, 0);
				m_dropdown.set_font("Arial", 14);
				m_dropdown.align_text(home_menu_dropdown<T>::text_align::center);
				m_dropdown.back_color = { 0.3f, 0.3f, 0.3f, 1.0f };
			}

			compiled_resource& get_compiled() override
			{
				this->update_value();

				if (!this->is_compiled())
				{
					const std::string value_text = Emu.GetCallbacks().get_localized_setting(home_menu_setting<T, cfg::_enum<T>>::m_setting, static_cast<u32>(this->m_last_value));
					m_dropdown.set_text(value_text);
					m_dropdown.set_pos(m_dropdown.x, this->y + (this->h - m_dropdown.h) / 2);

					this->compiled_resources = horizontal_layout::get_compiled();
					this->compiled_resources.add(m_dropdown.get_compiled());
				}

				return this->compiled_resources;
			}

		private:
			label m_dropdown;
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
			{
				m_slider.set_size(available_side_width / 2, element_height);
				m_slider.set_pos(overlay::virtual_width / 2 + menu_entry_margin, 0);
				m_slider.back_color = { 0.3f, 0.3f, 0.3f, 1.0f };

				m_handle.set_size(element_height / 2, element_height);
				m_handle.set_pos(m_slider.x, 0);
				m_handle.back_color = { 1.0f, 1.0f, 1.0f, 1.0f };

				m_value_label.back_color = m_slider.back_color;
				m_value_label.set_font("Arial", 14);
			}

			compiled_resource& get_compiled() override
			{
				this->update_value();

				if (!this->is_compiled())
				{
					const f64 percentage = std::clamp((this->m_last_value - static_cast<T>(m_minimum)) / std::fabs(m_maximum - m_minimum), 0.0, 1.0);
					m_slider.set_pos(m_slider.x, this->y + (this->h - m_slider.h) / 2);
					m_handle.set_pos(m_slider.x + static_cast<s16>(percentage * (m_slider.w - m_handle.w)), this->y + (this->h - m_handle.h) / 2);

					const auto set_label_text = [this]() -> void
					{
						if (const auto it = m_special_labels.find(this->m_last_value); it != m_special_labels.cend())
						{
							m_value_label.set_text(it->second);
							return;
						}

						if constexpr (std::is_floating_point_v<T>)
						{
							m_value_label.set_text(fmt::format("%.2f%s", this->m_last_value, m_suffix));
						}
						else
						{
							m_value_label.set_text(fmt::format("%d%s", this->m_last_value, m_suffix));
						}
					};

					set_label_text();
					m_value_label.auto_resize();

					constexpr u16 handle_margin = 10;

					if ((m_handle.x - m_slider.x) > (m_slider.w - (m_handle.w + 2 * handle_margin + m_value_label.w)))
					{
						m_value_label.set_pos(m_handle.x - (handle_margin + m_value_label.w), m_handle.y);
					}
					else
					{
						m_value_label.set_pos(m_handle.x + m_handle.w + handle_margin, m_handle.y);
					}

					this->compiled_resources = horizontal_layout::get_compiled();
					this->compiled_resources.add(m_slider.get_compiled());
					this->compiled_resources.add(m_handle.get_compiled());
					this->compiled_resources.add(m_value_label.get_compiled());
				}

				return this->compiled_resources;
			}

		private:
			overlay_element m_slider;
			overlay_element m_handle;
			label m_value_label;
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
