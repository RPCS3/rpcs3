#include "stdafx.h"
#include "overlay_controls.h"

#include "util/types.hpp"
#include "util/logs.hpp"
#include "Utilities/geometry.h"
#include "Utilities/File.h"
#include "Emu/Cell/timers.hpp"

#ifndef _WIN32
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#if defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/sysctl.h>
#endif
#endif

// Definitions for common UI controls and their routines
namespace rsx
{
	namespace overlays
	{
		static std::vector<vertex> generate_unit_quadrant(int num_patch_points, const float offset[2], const float scale[2])
		{
			ensure(num_patch_points >= 3);
			std::vector<vertex> result(num_patch_points + 1);

			// Set root vertex
			result[0].vec2(offset[0], offset[1]);

			// Set the 0th and Nth outer vertices which lie flush with the axes
			result[1].vec2(offset[0] + scale[0], offset[1]);
			result[num_patch_points].vec2(offset[0], offset[1] + scale[1]);

			constexpr float degrees_to_radians = 0.0174533f;
			for (int i = 1; i < num_patch_points - 1; i++)
			{
				// If we keep a unit circle, 2 of the 4 components of the rotation matrix become 0
				// We end up with a simple vec2(cos_theta, sin_theta) as the output
				// The final scaling and translation can then be done with fmad
				const auto angle = degrees_to_radians * ((i * 90) / (num_patch_points - 1));
				result[i + 1].vec2(
					std::fmaf(std::cos(angle), scale[0], offset[0]),
					std::fmaf(std::sin(angle), scale[1], offset[1])
				);
			}

			return result;
		}

		image_info::image_info(const std::string& filename, bool grayscaled)
		{
			fs::file f(filename, fs::read + fs::isfile);

			if (!f)
			{
				rsx_log.error("Image resource file `%s' could not be opened (%s)", filename, fs::g_tls_error);
				return;
			}

			const std::vector<u8> bytes = f.to_vector<u8>();
			load_data(bytes, grayscaled);
		}

		image_info::image_info(const std::vector<u8>& bytes, bool grayscaled)
		{
			load_data(bytes, grayscaled);
		}

		image_info::~image_info()
		{
			if (data) stbi_image_free(data);
		}

		void image_info::load_data(const std::vector<u8>& bytes, bool grayscaled)
		{
			data = stbi_load_from_memory(bytes.data(), ::narrow<int>(bytes.size()), &w, &h, &bpp, grayscaled ? STBI_grey_alpha : STBI_rgb_alpha);
			channels = grayscaled ? 2 : 4;

			if (data && grayscaled)
			{
				data_grey.resize(4 * w * h);

				for (usz i = 0, n = 0; i < data_grey.size(); i += 4, n += 2)
				{
					const u8 grey = data[n];
					const u8 alpha = data[n + 1];

					data_grey[i + 0] = grey;
					data_grey[i + 1] = grey;
					data_grey[i + 2] = grey;
					data_grey[i + 3] = alpha;
				}
			}
			else
			{
				data_grey.clear();
			}
		}

		resource_config::resource_config()
		{
		}

		void resource_config::load_files()
		{
			const std::array<std::string, 15> texture_resource_files
			{
				"fade_top.png",
				"fade_bottom.png",
				"select.png",
				"start.png",
				"cross.png",
				"circle.png",
				"triangle.png",
				"square.png",
				"L1.png",
				"R1.png",
				"L2.png",
				"R2.png",
				"save.png",
				"new.png",
				"spinner-24.png"
			};
			for (const std::string& res : texture_resource_files)
			{
				// First check the global config dir
				const std::string image_path = fs::get_config_dir() + "Icons/ui/" + res;
				auto info = std::make_unique<image_info>(image_path);

#if !defined(_WIN32) && !defined(__APPLE__) && defined(DATADIR)
				// Check the DATADIR if defined
				if (info->get_data() == nullptr)
				{
					const std::string data_dir (DATADIR);
					const std::string image_data = data_dir + "/Icons/ui/" + res;
					info = std::make_unique<image_info>(image_data);
				}
#endif

				if (info->get_data() == nullptr)
				{
					// Resource was not found in the DATADIR or config dir, try and grab from relative path (linux)
					std::string src = "Icons/ui/" + res;
					info = std::make_unique<image_info>(src);
#ifndef _WIN32
					// Check for Icons in ../share/rpcs3 for AppImages,
					// in rpcs3.app/Contents/Resources for App Bundles, and /usr/bin.
					if (info->get_data() == nullptr)
					{
						char result[ PATH_MAX ];
#if defined(__APPLE__)
						u32 bufsize = PATH_MAX;
						const bool success = _NSGetExecutablePath( result, &bufsize ) == 0;
#elif defined(KERN_PROC_PATHNAME)
						usz bufsize = PATH_MAX;
						int mib[] = {
							CTL_KERN,
#if defined(__NetBSD__)
							KERN_PROC_ARGS,
							-1,
							KERN_PROC_PATHNAME,
#else
							KERN_PROC,
							KERN_PROC_PATHNAME,
							-1,
#endif
						};
						const bool success = sysctl(mib, sizeof(mib)/sizeof(mib[0]), result, &bufsize, NULL, 0) >= 0;
#elif defined(__linux__)
						const bool success = readlink( "/proc/self/exe", result, PATH_MAX ) >= 0;
#elif defined(__sun)
						const bool success = readlink( "/proc/self/path/a.out", result, PATH_MAX ) >= 0;
#else
						const bool success = readlink( "/proc/curproc/file", result, PATH_MAX ) >= 0;
#endif
						if (success)
						{
							std::string executablePath = dirname(result);
#ifdef __APPLE__
							src = executablePath + "/../Resources/Icons/ui/" + res;
#else
							src = executablePath + "/../share/rpcs3/Icons/ui/" + res;
#endif
							info = std::make_unique<image_info>(src);
							// Check if the icons are in the same directory as the executable (local builds)
							if (info->get_data() == nullptr)
							{
								src = executablePath + "/Icons/ui/" + res;
								info = std::make_unique<image_info>(src);
							}
						}
					}
#endif
					if (info->get_data())
					{
						// Install the image to config dir
						fs::create_path(fs::get_parent_dir(image_path));
						fs::copy_file(src, image_path, true);
					}
				}

				texture_raw_data.push_back(std::move(info));
			}
		}

		void resource_config::free_resources()
		{
			texture_raw_data.clear();
		}

		void compiled_resource::command_config::set_image_resource(u8 ref)
		{
			texture_ref = ref;
			font_ref = nullptr;
		}

		void compiled_resource::command_config::set_font(font *ref)
		{
			texture_ref = image_resource_id::font_file;
			font_ref = ref;
		}

		f32 compiled_resource::command_config::get_sinus_value() const
		{
			return (static_cast<f32>(get_system_time() / 1000) * pulse_speed_modifier) - pulse_sinus_offset;
		}

		void compiled_resource::add(const compiled_resource& other)
		{
			auto old_size = draw_commands.size();
			draw_commands.resize(old_size + other.draw_commands.size());
			std::copy(other.draw_commands.begin(), other.draw_commands.end(), draw_commands.begin() + old_size);
		}

		void compiled_resource::add(const compiled_resource& other, f32 x_offset, f32 y_offset)
		{
			auto old_size = draw_commands.size();
			draw_commands.resize(old_size + other.draw_commands.size());
			std::copy(other.draw_commands.begin(), other.draw_commands.end(), draw_commands.begin() + old_size);

			for (usz n = old_size; n < draw_commands.size(); ++n)
			{
				for (auto &v : draw_commands[n].verts)
				{
					v += vertex(x_offset, y_offset, 0.f, 0.f);
				}
			}
		}

		void compiled_resource::add(const compiled_resource& other, f32 x_offset, f32 y_offset, const areaf& clip_rect)
		{
			auto old_size = draw_commands.size();
			draw_commands.resize(old_size + other.draw_commands.size());
			std::copy(other.draw_commands.begin(), other.draw_commands.end(), draw_commands.begin() + old_size);

			for (usz n = old_size; n < draw_commands.size(); ++n)
			{
				for (auto &v : draw_commands[n].verts)
				{
					v += vertex(x_offset, y_offset, 0.f, 0.f);
				}

				draw_commands[n].config.clip_rect = clip_rect;
				draw_commands[n].config.clip_region = true;
			}
		}

		void compiled_resource::clear()
		{
			draw_commands.clear();
		}

		compiled_resource::command& compiled_resource::append(const command& new_command)
		{
			draw_commands.emplace_back(new_command);
			return draw_commands.back();
		}

		compiled_resource::command& compiled_resource::prepend(const command& new_command)
		{
			draw_commands.emplace(draw_commands.begin(), new_command);
			return draw_commands.front();
		}

		void overlay_element::set_sinus_offset(f32 sinus_modifier)
		{
			if (sinus_modifier >= 0)
			{
				static constexpr f32 PI = 3.14159265f;
				const f32 pulse_sinus_x = static_cast<f32>(get_system_time() / 1000) * pulse_speed_modifier;
				pulse_sinus_offset = fmod(pulse_sinus_x + sinus_modifier * PI, 2.0f * PI);
			}
		}

		void overlay_element::refresh()
		{
			// Just invalidate for draw when get_compiled() is called
			m_is_compiled = false;
		}

		void overlay_element::translate(s16 _x, s16 _y)
		{
			x += _x;
			y += _y;

			m_is_compiled = false;
		}

		void overlay_element::scale(f32 _x, f32 _y, bool origin_scaling)
		{
			if (origin_scaling)
			{
				x = static_cast<s16>(_x * x);
				y = static_cast<s16>(_y * y);
			}

			w = static_cast<u16>(_x * w);
			h = static_cast<u16>(_y * h);

			m_is_compiled = false;
		}

		void overlay_element::set_pos(s16 _x, s16 _y)
		{
			x = _x;
			y = _y;

			m_is_compiled = false;
		}

		void overlay_element::set_size(u16 _w, u16 _h)
		{
			w = _w;
			h = _h;

			m_is_compiled = false;
		}

		void overlay_element::set_padding(u16 left, u16 right, u16 top, u16 bottom)
		{
			padding_left = left;
			padding_right = right;
			padding_top = top;
			padding_bottom = bottom;

			m_is_compiled = false;
		}

		void overlay_element::set_padding(u16 padding)
		{
			padding_left = padding_right = padding_top = padding_bottom = padding;
			m_is_compiled = false;
		}

		// NOTE: Functions as a simple position offset. Top left corner is the anchor.
		void overlay_element::set_margin(u16 left, u16 top)
		{
			margin_left = left;
			margin_top = top;

			m_is_compiled = false;
		}

		void overlay_element::set_margin(u16 margin)
		{
			margin_left = margin_top = margin;
			m_is_compiled = false;
		}

		void overlay_element::set_text(const std::string& text)
		{
			std::u32string new_text = utf8_to_u32string(text);
			const bool is_dirty = this->text != new_text;
			this->text = std::move(new_text);

			if (is_dirty)
			{
				m_is_compiled = false;
			}
		}

		void overlay_element::set_unicode_text(const std::u32string& text)
		{
			const bool is_dirty = this->text != text;
			this->text = text;

			if (is_dirty)
			{
				m_is_compiled = false;
			}
		}

		void overlay_element::set_text(localized_string_id id)
		{
			set_unicode_text(get_localized_u32string(id));
		}

		void overlay_element::set_text(const localized_string& container)
		{
			set_text(container.str);
		}

		void overlay_element::set_font(const char* font_name, u16 font_size)
		{
			font_ref = fontmgr::get(font_name, font_size);
			m_is_compiled = false;
		}

		void overlay_element::align_text(text_align align)
		{
			alignment = align;
			m_is_compiled = false;
		}

		void overlay_element::set_wrap_text(bool state)
		{
			wrap_text = state;
			m_is_compiled = false;
		}

		font* overlay_element::get_font() const
		{
			return font_ref ? font_ref : fontmgr::get("Arial", 12);
		}

		std::vector<vertex> overlay_element::render_text(const char32_t* string, f32 x, f32 y)
		{
			auto renderer = get_font();

			const u16 clip_width = clip_text ? w : umax;
			std::vector<vertex> result = renderer->render_text(string, clip_width, wrap_text);

			if (!result.empty())
			{
				const auto apply_transform = [&]()
				{
					const f32 size_px = renderer->get_size_px();

					for (vertex& v : result)
					{
						// Apply transform.
						// (0, 0) has text sitting one line off the top left corner (text is outside the rect) hence the offset by text height
						v.x() += x + padding_left;
						v.y() += y + padding_top + size_px;
					}
				};

				if (alignment == text_align::left)
				{
					apply_transform();
				}
				else
				{
					// Scan for lines and measure them
					// Reposition them to the center or right depending on the alignment
					std::vector<std::tuple<u32, u32, f32>> lines;
					u32 line_begin = 0;
					u32 line_end = 0;
					u32 word_end = 0;
					u32 ctr = 0;
					f32 text_extents_w = w;

					for (const auto& c : text)
					{
						switch (c)
						{
						case '\r':
						{
							continue;
						}
						case '\n':
						{
							lines.emplace_back(line_begin, std::min(word_end, line_end), text_extents_w);
							word_end = line_end = line_begin = ctr;
							text_extents_w = w;
							continue;
						}
						default:
						{
							ctr += 4;

							if (c == ' ')
							{
								if (line_end == line_begin)
								{
									// Ignore leading whitespace
									word_end = line_end = line_begin = ctr;
								}
								else
								{
									line_end = ctr;
								}
							}
							else
							{
								word_end = line_end = ctr;

								// Check for real text region extent
								text_extents_w = std::max(result[ctr - 1].x(), text_extents_w);
							}
							continue;
						}
						}
					}

					// Add final line
					lines.emplace_back(line_begin, std::min(word_end, line_end), std::max<f32>(text_extents_w, w));

					const f32 offset_extent = (alignment == text_align::center ? 0.5f : 1.0f);
					const f32 size_px = renderer->get_size_px() * 0.5f;

					// Apply padding
					apply_transform();

					// Moves all glyphs of a line by the correct amount to get a nice alignment.
					const auto move_line = [&result, &offset_extent](u32 begin, u32 end, f32 max_region_w)
					{
						const f32 line_length = result[end - 1].x() - result[begin].x();

						if (line_length < max_region_w)
						{
							const f32 offset = (max_region_w - line_length) * offset_extent;
							for (auto n = begin; n < end; ++n)
							{
								result[n].x() += offset;
							}
						}
					};

					// Properly place all lines individually
					for (const auto& [begin, end, max_region_w] : lines)
					{
						if (begin >= end)
							continue;

						// Check if there's any wrapped text
						if (std::fabs(result[end - 1].y() - result[begin + 3].y()) < size_px)
						{
							// No wrapping involved. We can just move the entire line.
							move_line(begin, end, max_region_w);
							continue;
						}

						// Wrapping involved. We have to search for the line breaks and move each line seperately.
						for (u32 i_begin = begin, i_next = begin + 4;; i_next += 4)
						{
							// Check if this is the last glyph in the line of text.
							const bool is_last_glyph = i_next >= end;

							// The line may be wrapped, so we need to check if the next glyph's position is below the current position.
							if (is_last_glyph || (result[i_next - 1].y() - result[i_begin + 3].y() >= size_px))
							{
								// Whenever we reached the end of a visual line we need to move its glyphs accordingly.
								const u32 i_end = i_next - (is_last_glyph ? 0 : 4);

								move_line(i_begin, i_end, max_region_w);

								i_begin = i_end;

								if (is_last_glyph)
								{
									break;
								}
							}
						}
					}
				}
			}

			return result;
		}

		compiled_resource& overlay_element::get_compiled()
		{
			if (!is_compiled())
			{
				compiled_resources.clear();

				compiled_resource compiled_resources_temp = {};
				auto& cmd_bg = compiled_resources_temp.append({});
				auto& config = cmd_bg.config;

				config.color = back_color;
				config.pulse_glow = pulse_effect_enabled;
				config.pulse_sinus_offset = pulse_sinus_offset;
				config.pulse_speed_modifier = pulse_speed_modifier;

				auto& verts = compiled_resources_temp.draw_commands.front().verts;
				verts.resize(4);

				verts[0].vec4(x, y, 0.f, 0.f);
				verts[1].vec4(f32(x + w), y, 1.f, 0.f);
				verts[2].vec4(x, f32(y + h), 0.f, 1.f);
				verts[3].vec4(f32(x + w), f32(y + h), 1.f, 1.f);

				compiled_resources.add(std::move(compiled_resources_temp), margin_left, margin_top);

				if (!text.empty())
				{
					compiled_resources_temp.clear();
					auto& cmd_text = compiled_resources_temp.append({});

					cmd_text.config.set_font(get_font());
					cmd_text.config.color = fore_color;
					cmd_text.verts = render_text(text.c_str(), static_cast<f32>(x), static_cast<f32>(y));

					if (!cmd_text.verts.empty())
						compiled_resources.add(std::move(compiled_resources_temp), margin_left - horizontal_scroll_offset, margin_top - vertical_scroll_offset);
				}

				m_is_compiled = true;
			}

			return compiled_resources;
		}

		void overlay_element::measure_text(u16& width, u16& height, bool ignore_word_wrap) const
		{
			if (text.empty())
			{
				width = height = 0;
				return;
			}

			auto renderer = get_font();

			f32 text_width = 0.f;
			f32 unused = 0.f;
			f32 max_w = 0.f;
			f32 last_word = 0.f;
			height = static_cast<u16>(renderer->get_size_px());

			for (auto c : text)
			{
				if (c == '\n')
				{
					height += static_cast<u16>(renderer->get_size_px() + 2);
					max_w = std::max(max_w, text_width);
					text_width = 0.f;
					last_word = 0.f;
					continue;
				}

				if (c == ' ')
				{
					last_word = text_width;
				}

				renderer->get_char(c, text_width, unused);

				if (!ignore_word_wrap && wrap_text && text_width >= w)
				{
					if ((text_width - last_word) < w)
					{
						max_w = std::max(max_w, last_word);
						text_width -= (last_word + renderer->get_em_size());
						height += static_cast<u16>(renderer->get_size_px() + 2);
					}
				}
			}

			max_w = std::max(max_w, text_width);
			width = static_cast<u16>(ceilf(max_w));
		}

		layout_container::layout_container()
		{
			// Transparent by default
			back_color.a = 0.f;
		}

		void layout_container::translate(s16 _x, s16 _y)
		{
			overlay_element::translate(_x, _y);

			for (auto& itm : m_items)
				itm->translate(_x, _y);
		}

		void layout_container::set_pos(s16 _x, s16 _y)
		{
			const s16 dx = _x - x;
			const s16 dy = _y - y;
			translate(dx, dy);
		}

		bool layout_container::is_compiled()
		{
			if (m_is_compiled && std::any_of(m_items.cbegin(), m_items.cend(), [](const auto& item){ return item && !item->is_compiled(); }))
			{
				m_is_compiled = false;
			}

			return m_is_compiled;
		}

		compiled_resource& layout_container::get_compiled()
		{
			if (!is_compiled())
			{
				compiled_resource result = overlay_element::get_compiled();

				for (auto& itm : m_items)
					result.add(itm->get_compiled());

				compiled_resources = result;
			}

			return compiled_resources;
		}

		void layout_container::add_spacer()
		{
			std::unique_ptr<overlay_element> spacer_element = std::make_unique<spacer>();
			add_element(spacer_element);
		}

		overlay_element* vertical_layout::add_element(std::unique_ptr<overlay_element>& item, int offset)
		{
			if (auto_resize)
			{
				item->set_pos(item->x + x, h + pack_padding + y);
				h += item->h + pack_padding;
				w = std::max(w, item->w);
			}
			else
			{
				item->set_pos(item->x + x, advance_pos + pack_padding + y);
				advance_pos += item->h + pack_padding;
			}

			if (offset < 0)
			{
				m_items.push_back(std::move(item));
				return m_items.back().get();
			}

			overlay_element* result = item.get();
			m_items.insert(m_items.begin() + offset, std::move(item));
			return result;
		}

		compiled_resource& vertical_layout::get_compiled()
		{
			if (scroll_offset_value == 0 && auto_resize)
				return layout_container::get_compiled();

			if (!is_compiled())
			{
				compiled_resource result = overlay_element::get_compiled();
				const f32 global_y_offset = static_cast<f32>(-scroll_offset_value);

				for (auto& item : m_items)
				{
					if (!item)
					{
						rsx_log.error("Found null item in overlay_controls::vertical_layout");
						continue;
					}

					const s32 item_y_limit = s32{item->y} + item->h - scroll_offset_value - y;
					const s32 item_y_base = s32{item->y} - scroll_offset_value - y;

					if (item_y_base > h)
					{
						// Out of bounds. The following items will be too.
						break;
					}

					if (item_y_limit < 0)
					{
						// Out of bounds
						continue;
					}

					if (item_y_limit > h || item_y_base < 0)
					{
						// Partial render
						areaf clip_rect = static_cast<areaf>(areai{x, y, (x + w), (y + h)});
						result.add(item->get_compiled(), 0.f, global_y_offset, clip_rect);
					}
					else
					{
						// Normal
						result.add(item->get_compiled(), 0.f, global_y_offset);
					}
				}

				compiled_resources = result;
			}

			return compiled_resources;
		}

		u16 vertical_layout::get_scroll_offset_px()
		{
			return scroll_offset_value;
		}

		overlay_element* horizontal_layout::add_element(std::unique_ptr<overlay_element>& item, int offset)
		{
			if (auto_resize)
			{
				item->set_pos(w + pack_padding + x, item->y + y);
				w += item->w + pack_padding;
				h = std::max(h, item->h);
			}
			else
			{
				item->set_pos(advance_pos + pack_padding + x, item->y + y);
				advance_pos += item->w + pack_padding;
			}

			if (offset < 0)
			{
				m_items.push_back(std::move(item));
				return m_items.back().get();
			}

			auto result = item.get();
			m_items.insert(m_items.begin() + offset, std::move(item));
			return result;
		}

		compiled_resource& horizontal_layout::get_compiled()
		{
			if (scroll_offset_value == 0 && auto_resize)
				return layout_container::get_compiled();

			if (!is_compiled())
			{
				compiled_resource result = overlay_element::get_compiled();
				const f32 global_x_offset = static_cast<f32>(-scroll_offset_value);

				for (auto &item : m_items)
				{
					if (!item)
					{
						rsx_log.error("Found null item in overlay_controls::horizontal_layout");
						continue;
					}

					const s32 item_x_limit = s32{item->x} + item->w - scroll_offset_value - w;
					const s32 item_x_base = s32{item->x} - scroll_offset_value - w;

					if (item_x_base > w)
					{
						// Out of bounds. The following items will be too.
						break;
					}

					if (item_x_limit < 0)
					{
						// Out of bounds
						continue;
					}

					if (item_x_limit > w || item_x_base < 0)
					{
						// Partial render
						areaf clip_rect = static_cast<areaf>(areai{x, y, (x + w), (y + h)});
						result.add(item->get_compiled(), global_x_offset, 0.f, clip_rect);
					}
					else
					{
						// Normal
						result.add(item->get_compiled(), global_x_offset, 0.f);
					}
				}

				compiled_resources = result;
			}

			return compiled_resources;
		}

		u16 horizontal_layout::get_scroll_offset_px()
		{
			return scroll_offset_value;
		}

		compiled_resource& image_view::get_compiled()
		{
			if (!is_compiled())
			{
				auto& result  = overlay_element::get_compiled();
				auto& cmd_img = result.draw_commands.front();

				cmd_img.config.set_image_resource(image_resource_ref);
				cmd_img.config.color = fore_color;
				cmd_img.config.external_data_ref = external_ref;
				cmd_img.config.blur_strength = blur_strength;

				// Make padding work for images (treat them as the content instead of the 'background')
				auto& verts = cmd_img.verts;

				verts[0] += vertex(padding_left, padding_bottom, 0, 0);
				verts[1] += vertex(-padding_right, padding_bottom, 0, 0);
				verts[2] += vertex(padding_left, -padding_top, 0, 0);
				verts[3] += vertex(-padding_right, -padding_top, 0, 0);

				m_is_compiled = true;
			}

			return compiled_resources;
		}

		void image_view::set_image_resource(u8 resource_id)
		{
			image_resource_ref = resource_id;
			external_ref = nullptr;
		}

		void image_view::set_raw_image(image_info_base* raw_image)
		{
			image_resource_ref = image_resource_id::raw_image;
			external_ref = raw_image;
		}

		void image_view::clear_image()
		{
			image_resource_ref = image_resource_id::none;
			external_ref = nullptr;
		}

		void image_view::set_blur_strength(u8 strength)
		{
			blur_strength = strength;
		}

		image_button::image_button()
		{
			// Do not clip text to region extents
			// TODO: Define custom clipping region or use two controls to emulate
			clip_text = false;
		}

		image_button::image_button(u16 _w, u16 _h)
		{
			clip_text = false;
			set_size(_w, _h);
		}

		void image_button::set_text_vertical_adjust(s16 offset)
		{
			m_text_offset_y = offset;
		}

		void image_button::set_size(u16 /*w*/, u16 h)
		{
			image_view::set_size(h, h);
			m_text_offset_x = (h / 2) + text_horizontal_offset; // By default text is at the horizontal center
		}

		compiled_resource& image_button::get_compiled()
		{
			if (!is_compiled())
			{
				auto& compiled = image_view::get_compiled();
				for (auto& cmd : compiled.draw_commands)
				{
					if (cmd.config.texture_ref == image_resource_id::font_file)
					{
						// Text, translate geometry to the right
						for (auto &v : cmd.verts)
						{
							v.values[0] += m_text_offset_x;
							v.values[1] += m_text_offset_y;
						}
					}
				}
			}

			return compiled_resources;
		}

		label::label(const std::string& text)
		{
			set_text(text);
		}

		bool label::auto_resize(bool grow_only, u16 limit_w, u16 limit_h)
		{
			u16 new_width, new_height;
			u16 old_width = w, old_height = h;
			measure_text(new_width, new_height, true);

			new_width += padding_left + padding_right;
			new_height += padding_top + padding_bottom;

			if (new_width > limit_w && wrap_text)
				measure_text(new_width, new_height, false);

			if (grow_only)
			{
				new_width = std::max(w, new_width);
				new_height = std::max(h, new_height);
			}

			w = std::min(new_width, limit_w);
			h = std::min(new_height, limit_h);

			bool size_changed = old_width != new_width || old_height != new_height;
			return size_changed;
		}

		compiled_resource& rounded_rect::get_compiled()
		{
			if (!is_compiled())
			{
				compiled_resources.clear();

#ifdef __APPLE__
				if (true)
#else
				if (radius == 0 || radius > (w / 2))
#endif
				{
					// Invalid radius
					compiled_resources = overlay_element::get_compiled();
				}
				else
				{
					compiled_resource compiled_resources_temp = {};
					compiled_resources_temp.append({}); // Bg horizontal mid
					compiled_resources_temp.append({}); // Bg horizontal top
					compiled_resources_temp.append({}); // Bg horizontal bottom
					compiled_resources_temp.append({}); // Bg upper-left
					compiled_resources_temp.append({}); // Bg lower-left
					compiled_resources_temp.append({}); // Bg upper-right
					compiled_resources_temp.append({}); // Bg lower-right

					for (auto& draw_cmd : compiled_resources_temp.draw_commands)
					{
						auto& config = draw_cmd.config;
						config.color = back_color;
						config.disable_vertex_snap = true;
						config.pulse_glow = pulse_effect_enabled;
						config.pulse_sinus_offset = pulse_sinus_offset;
						config.pulse_speed_modifier = pulse_speed_modifier;
					}

					auto& bg0 = compiled_resources_temp.draw_commands[0];
					auto& bg1 = compiled_resources_temp.draw_commands[1];
					auto& bg2 = compiled_resources_temp.draw_commands[2];

					bg0.verts.emplace_back(f32(x), f32(y + radius), 0.f, 0.f);
					bg0.verts.emplace_back(f32(x + w), f32(y + radius), 0.f, 0.f);
					bg0.verts.emplace_back(f32(x), f32(y + h) - radius, 0.f, 0.f);
					bg0.verts.emplace_back(f32(x + w), f32(y + h) - radius, 0.f, 0.f);

					bg1.verts.emplace_back(f32(x + radius), f32(y), 0.f, 0.f);
					bg1.verts.emplace_back(f32(x + w) - radius, f32(y), 0.f, 0.f);
					bg1.verts.emplace_back(f32(x + radius), f32(y + radius), 0.f, 0.f);
					bg1.verts.emplace_back(f32(x + w) - radius, f32(y + radius), 0.f, 0.f);

					bg2.verts.emplace_back(f32(x + radius), f32(y + h) - radius, 0.f, 0.f);
					bg2.verts.emplace_back(f32(x + w) - radius, f32(y + h) - radius, 0.f, 0.f);
					bg2.verts.emplace_back(f32(x + radius), f32(y + h), 0.f, 0.f);
					bg2.verts.emplace_back(f32(x + w) - radius, f32(y + h), 0.f, 0.f);

					// Generate the quadrants
					const f32 corners[4][2] =
					{
						{ f32(x + radius), f32(y + radius) },
						{ f32(x + radius), f32(y + h) - radius },
						{ f32(x + w) - radius, f32(y + radius) },
						{ f32(x + w) - radius, f32(y + h) - radius }
					};

					const f32 radius_f = static_cast<f32>(radius);
					const f32 scale[4][2] =
					{
						{ -radius_f, -radius_f },
						{ -radius_f, +radius_f },
						{ +radius_f, -radius_f },
						{ +radius_f, +radius_f }
					};

					for (int i = 0; i < 4; ++i)
					{
						auto& command = compiled_resources_temp.draw_commands[i + 3];
						command.config.primitives = rsx::overlays::primitive_type::triangle_fan;
						command.verts = generate_unit_quadrant(num_control_points, corners[i], scale[i]);
					}

					compiled_resources.add(std::move(compiled_resources_temp), margin_left, margin_top);
				}

				m_is_compiled = true;
			}

			return compiled_resources;
		}
	}
}
