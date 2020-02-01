#pragma once
#include "Utilities/types.h"
#include "Utilities/geometry.h"
#include "Utilities/File.h"
#include "overlay_utils.h"
#include "Emu/System.h"

#include <string>
#include <vector>
#include <memory>
#include <locale>

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <libgen.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#if defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/sysctl.h>
#endif

// STB_IMAGE_IMPLEMENTATION and STB_TRUETYPE_IMPLEMENTATION defined externally
#include <stb_image.h>
#include <stb_truetype.h>

// Definitions for common UI controls and their routines
namespace rsx
{
	namespace overlays
	{
		enum image_resource_id : u8
		{
			// NOTE: 1 - 252 are user defined
			none = 0,         // No image
			raw_image = 252,  // Raw image data passed via image_info struct
			font_file = 253,  // Font file
			game_icon = 254,  // Use game icon
			backbuffer = 255  // Use current backbuffer contents
		};

		enum class primitive_type : u8
		{
			quad_list = 0,
			triangle_strip = 1,
			line_list = 2,
			line_strip = 3
		};

		struct font
		{
			const u32 width = 1024;
			const u32 height = 1024;
			const u32 oversample = 2;
			const u32 char_count = 256; // 16x16 grid at max 48pt

			f32 size_pt = 12.f;
			f32 size_px = 16.f; // Default font 12pt size
			f32 em_size = 0.f;
			std::string font_name;
			std::vector<stbtt_packedchar> pack_info;
			std::vector<u8> glyph_data;
			bool initialized = false;

			font(const char* ttf_name, f32 size);

			stbtt_aligned_quad get_char(char c, f32& x_advance, f32& y_advance);

			void render_text_ex(std::vector<vertex>& result, f32& x_advance, f32& y_advance, const char* text, u32 char_limit, u16 max_width, bool wrap);

			std::vector<vertex> render_text(const char* text, u16 max_width = UINT16_MAX, bool wrap = false);

			std::pair<f32, f32> get_char_offset(const char* text, u16 max_length, u16 max_width = UINT16_MAX, bool wrap = false);
		};

		// TODO: Singletons are cancer
		class fontmgr
		{
		private:
			std::vector<std::unique_ptr<font>> fonts;
			static fontmgr *m_instance;

			font* find(const char *name, int size)
			{
				for (auto &f : fonts)
				{
					if (f->font_name == name &&
						f->size_pt == size)
						return f.get();
				}

				fonts.push_back(std::make_unique<font>(name, static_cast<f32>(size)));
				return fonts.back().get();
			}

		public:

			fontmgr() = default;
			~fontmgr()
			{
				if (m_instance)
				{
					delete m_instance;
					m_instance = nullptr;
				}
			}

			static font* get(const char *name, int size)
			{
				if (m_instance == nullptr)
					m_instance = new fontmgr;

				return m_instance->find(name, size);
			}
		};

		struct image_info
		{
			int w = 0, h = 0;
			int bpp = 0;
			u8* data = nullptr;

			image_info(image_info&) = delete;

			image_info(const char* filename)
			{
				if (!fs::is_file(filename))
				{
					rsx_log.error("Image resource file `%s' not found", filename);
					return;
				}

				std::vector<u8> bytes;
				fs::file f(filename);
				f.read(bytes, f.size());
				data = stbi_load_from_memory(bytes.data(), ::narrow<int>(f.size()), &w, &h, &bpp, STBI_rgb_alpha);
			}

			image_info(const std::vector<u8>& bytes)
			{
				data = stbi_load_from_memory(bytes.data(), ::narrow<int>(bytes.size()), &w, &h, &bpp, STBI_rgb_alpha);
			}

			~image_info()
			{
				stbi_image_free(data);
				data = nullptr;
				w = h = bpp = 0;
			}
		};

		struct resource_config
		{
			enum standard_image_resource : u8
			{
				fade_top = 1,
				fade_bottom,
				select,
				start,
				cross,
				circle,
				triangle,
				square,
				L1,
				R1,
				L2,
				R2,
				save,
				new_entry
			};

			// Define resources
			std::vector<std::string> texture_resource_files;
			std::vector<std::unique_ptr<image_info>> texture_raw_data;

			resource_config()
			{
				texture_resource_files.emplace_back("fade_top.png");
				texture_resource_files.emplace_back("fade_bottom.png");
				texture_resource_files.emplace_back("select.png");
				texture_resource_files.emplace_back("start.png");
				texture_resource_files.emplace_back("cross.png");
				texture_resource_files.emplace_back("circle.png");
				texture_resource_files.emplace_back("triangle.png");
				texture_resource_files.emplace_back("square.png");
				texture_resource_files.emplace_back("L1.png");
				texture_resource_files.emplace_back("R1.png");
				texture_resource_files.emplace_back("L2.png");
				texture_resource_files.emplace_back("R2.png");
				texture_resource_files.emplace_back("save.png");
				texture_resource_files.emplace_back("new.png");
			}

			void load_files()
			{
				for (const auto &res : texture_resource_files)
				{
					// First check the global config dir
					auto info = std::make_unique<image_info>((fs::get_config_dir() + "Icons/ui/" + res).c_str());

					if (info->data == nullptr)
					{
						// Resource was not found in config dir, try and grab from relative path (linux)
						auto src = "Icons/ui/" + res;
						info = std::make_unique<image_info>(src.c_str());
#ifndef _WIN32
						// Check for Icons in ../share/rpcs3 for AppImages,
						// in rpcs3.app/Contents/Resources for App Bundles, and /usr/bin.
						if (info->data == nullptr)
						{
							char result[ PATH_MAX ];
#if defined(__APPLE__)
							uint32_t bufsize = PATH_MAX;
							bool success = _NSGetExecutablePath( result, &bufsize ) == 0;
#elif defined(KERN_PROC_PATHNAME)
							size_t bufsize = PATH_MAX;
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
							bool success = sysctl(mib, sizeof(mib)/sizeof(mib[0]), result, &bufsize, NULL, 0) >= 0;
#elif defined(__linux__)
							bool success = readlink( "/proc/self/exe", result, PATH_MAX ) >= 0;
#elif defined(__sun)
							bool success = readlink( "/proc/self/path/a.out", result, PATH_MAX ) >= 0;
#else
							bool success = readlink( "/proc/curproc/file", result, PATH_MAX ) >= 0;
#endif
							if (success)
							{
								std::string executablePath = dirname(result);
#ifdef __APPLE__
								src = executablePath + "/../Resources/Icons/ui/" + res;
#else
								src = executablePath + "/../share/rpcs3/Icons/ui/" + res;
#endif
								info = std::make_unique<image_info>(src.c_str());
								// Check if the icons are in the same directory as the executable (local builds)
								if (info->data == nullptr)
								{
									src = executablePath + "/Icons/ui/" + res;
									info = std::make_unique<image_info>(src.c_str());
								}
							}
						}
#endif
						if (info->data != nullptr)
						{
							// Install the image to config dir
							auto dst_dir = fs::get_config_dir() + "Icons/ui/";
							auto dst = dst_dir + res;

							if (!fs::is_dir(dst_dir))
							{
								auto root_folder = fs::get_config_dir() + "Icons/";
								if (!fs::is_dir(root_folder))
									fs::create_dir(root_folder);

								fs::create_dir(dst_dir);
							}

							fs::copy_file(src, dst, true);
						}
					}

					texture_raw_data.push_back(std::move(info));
				}
			}

			void free_resources()
			{
				texture_raw_data.clear();
			}
		};

		struct compiled_resource
		{
			struct command_config
			{
				primitive_type primitives = primitive_type::quad_list;

				color4f color = { 1.f, 1.f, 1.f, 1.f };
				bool pulse_glow = false;

				areaf clip_rect = {};
				bool clip_region = false;

				u8 texture_ref = image_resource_id::none;
				font *font_ref = nullptr;
				void *external_data_ref = nullptr;

				u8 blur_strength = 0;

				command_config() = default;

				void set_image_resource(u8 ref)
				{
					texture_ref = ref;
					font_ref = nullptr;
				}

				void set_font(font *ref)
				{
					texture_ref = image_resource_id::font_file;
					font_ref = ref;
				}
			};

			struct command
			{
				command_config config;
				std::vector<vertex> verts;
			};

			std::vector<command> draw_commands;

			void add(const compiled_resource& other)
			{
				auto old_size = draw_commands.size();
				draw_commands.resize(old_size + other.draw_commands.size());
				std::copy(other.draw_commands.begin(), other.draw_commands.end(), draw_commands.begin() + old_size);
			}

			void add(const compiled_resource& other, f32 x_offset, f32 y_offset)
			{
				auto old_size = draw_commands.size();
				draw_commands.resize(old_size + other.draw_commands.size());
				std::copy(other.draw_commands.begin(), other.draw_commands.end(), draw_commands.begin() + old_size);

				for (size_t n = old_size; n < draw_commands.size(); ++n)
				{
					for (auto &v : draw_commands[n].verts)
					{
						v += vertex(x_offset, y_offset, 0.f, 0.f);
					}
				}
			}

			void add(const compiled_resource& other, f32 x_offset, f32 y_offset, const areaf& clip_rect)
			{
				auto old_size = draw_commands.size();
				draw_commands.resize(old_size + other.draw_commands.size());
				std::copy(other.draw_commands.begin(), other.draw_commands.end(), draw_commands.begin() + old_size);

				for (size_t n = old_size; n < draw_commands.size(); ++n)
				{
					for (auto &v : draw_commands[n].verts)
					{
						v += vertex(x_offset, y_offset, 0.f, 0.f);
					}

					draw_commands[n].config.clip_rect = clip_rect;
					draw_commands[n].config.clip_region = true;
				}
			}

			//! Clear commands list
			void clear()
			{
				draw_commands.clear();
			}

			//! Append the command to the back of the commands list and return it
			command& append(const command& new_command)
			{
				draw_commands.emplace_back(new_command);
				return draw_commands.back();
			}

			//! Prepend the command to the front of the commands list and return it
			command& prepend(const command& new_command)
			{
				draw_commands.emplace(draw_commands.begin(), new_command);
				return draw_commands.front();
			}
		};

		struct overlay_element
		{
			enum text_align
			{
				left = 0,
				center
			};

			u16 x = 0;
			u16 y = 0;
			u16 w = 0;
			u16 h = 0;

			std::string text;
			font* font_ref = nullptr;
			text_align alignment = left;
			bool wrap_text = false;
			bool clip_text = true;

			color4f back_color = { 0.f, 0.f, 0.f, 1.f };
			color4f fore_color = { 1.f, 1.f, 1.f, 1.f };
			bool pulse_effect_enabled = false;

			compiled_resource compiled_resources;
			bool is_compiled = false;

			u16 padding_left = 0;
			u16 padding_right = 0;
			u16 padding_top = 0;
			u16 padding_bottom = 0;

			u16 margin_left = 0;
			u16 margin_top = 0;

			overlay_element() = default;
			overlay_element(u16 _w, u16 _h) : w(_w), h(_h) {}
			virtual ~overlay_element() = default;

			virtual void refresh()
			{
				// Just invalidate for draw when get_compiled() is called
				is_compiled = false;
			}

			virtual void translate(s16 _x, s16 _y)
			{
				x = static_cast<u16>(x + _x);
				y = static_cast<u16>(y + _y);

				is_compiled = false;
			}

			virtual void scale(f32 _x, f32 _y, bool origin_scaling)
			{
				if (origin_scaling)
				{
					x = static_cast<u16>(_x * x);
					y = static_cast<u16>(_y * y);
				}

				w = static_cast<u16>(_x * w);
				h = static_cast<u16>(_y * h);

				is_compiled = false;
			}

			virtual void set_pos(u16 _x, u16 _y)
			{
				x = _x;
				y = _y;

				is_compiled = false;
			}

			virtual void set_size(u16 _w, u16 _h)
			{
				w = _w;
				h = _h;

				is_compiled = false;
			}

			virtual void set_padding(u16 left, u16 right, u16 top, u16 bottom)
			{
				padding_left = left;
				padding_right = right;
				padding_top = top;
				padding_bottom = bottom;

				is_compiled = false;
			}

			virtual void set_padding(u16 padding)
			{
				padding_left = padding_right = padding_top = padding_bottom = padding;
				is_compiled = false;
			}

			// NOTE: Functions as a simple position offset. Top left corner is the anchor.
			virtual void set_margin(u16 left, u16 top)
			{
				margin_left = left;
				margin_top = top;

				is_compiled = false;
			}

			virtual void set_margin(u16 margin)
			{
				margin_left = margin_top = margin;
				is_compiled = false;
			}

			virtual void set_text(const std::string& text)
			{
				this->text = text;
				is_compiled = false;
			}

			virtual void set_text(const char* text)
			{
				this->text = text;
				is_compiled = false;
			}

			virtual void set_font(const char* font_name, u16 font_size)
			{
				font_ref = fontmgr::get(font_name, font_size);
				is_compiled = false;
			}

			virtual void align_text(text_align align)
			{
				alignment = align;
				is_compiled = false;
			}

			virtual void set_wrap_text(bool state)
			{
				wrap_text = state;
				is_compiled = false;
			}

			virtual font* get_font() const
			{
				return font_ref ? font_ref : fontmgr::get("Arial", 12);
			}

			virtual std::vector<vertex> render_text(const char *string, f32 x, f32 y)
			{
				auto renderer = get_font();

				f32 text_extents_w = 0.f;
				u16 clip_width = clip_text ? w : UINT16_MAX;
				std::vector<vertex> result = renderer->render_text(string, clip_width, wrap_text);

				if (!result.empty())
				{
					for (auto &v : result)
					{
						// Check for real text region extent
						// TODO: Ellipsis
						text_extents_w = std::max(v.values[0], text_extents_w);

						// Apply transform.
						// (0, 0) has text sitting one line off the top left corner (text is outside the rect) hence the offset by text height
						v.values[0] += x + padding_left;
						v.values[1] += y + padding_top + static_cast<f32>(renderer->size_px);
					}

					if (alignment == center)
					{
						// Scan for lines and measure them
						// Reposition them to the center
						std::vector<std::pair<u32, u32>> lines;
						u32 line_begin = 0;
						u32 ctr = 0;

						for (auto c : text)
						{
							switch (c)
							{
							case '\r':
								continue;
							case '\n':
								lines.emplace_back(line_begin, ctr);
								line_begin = ctr;
								continue;
							default:
								ctr += 4;
							}
						}

						lines.emplace_back(line_begin, ctr);
						const auto max_region_w = std::max<f32>(text_extents_w, w);

						for (auto p : lines)
						{
							if (p.first >= p.second)
								continue;

							const f32 line_length = result[p.second - 1].values[0] - result[p.first].values[0];
							const bool wrapped = std::fabs(result[p.second - 1].values[1] - result[p.first + 3].values[1]) >= (renderer->size_px * 0.5f);

							if (wrapped)
								continue;

							if (line_length < max_region_w)
							{
								f32 offset = (max_region_w - line_length) * 0.5f;
								for (auto n = p.first; n < p.second; ++n)
								{
									result[n].values[0] += offset;
								}
							}
						}
					}
				}

				return result;
			}

			virtual compiled_resource& get_compiled()
			{
				if (!is_compiled)
				{
					compiled_resources.clear();

					compiled_resource compiled_resources_temp = {};
					auto& cmd_bg = compiled_resources_temp.append({});
					auto& config = cmd_bg.config;

					config.color = back_color;
					config.pulse_glow = pulse_effect_enabled;

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

						cmd_text.config.set_font(font_ref ? font_ref : fontmgr::get("Arial", 12));
						cmd_text.config.color = fore_color;
						cmd_text.verts = render_text(text.c_str(), static_cast<f32>(x), static_cast<f32>(y));

						if (!cmd_text.verts.empty())
							compiled_resources.add(std::move(compiled_resources_temp), margin_left, margin_top);
					}

					is_compiled = true;
				}

				return compiled_resources;
			}

			void measure_text(u16& width, u16& height, bool ignore_word_wrap = false) const
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
				height = static_cast<u16>(renderer->size_px);

				for (auto c : text)
				{
					if (c == '\n')
					{
						height += static_cast<u16>(renderer->size_px + 2);
						max_w = std::max(max_w, text_width);
						text_width = 0.f;
						last_word = 0.f;
						continue;
					}

					if (c == ' ')
					{
						last_word = text_width;
					}

					if (static_cast<u8>(c) > renderer->char_count)
					{
						// Non-existent glyph
						text_width += renderer->em_size;
					}
					else
					{
						renderer->get_char(c, text_width, unused);
					}

					if (!ignore_word_wrap && wrap_text && text_width >= w)
					{
						if ((text_width - last_word) < w)
						{
							max_w = std::max(max_w, last_word);
							text_width -= (last_word + renderer->em_size);
							height += static_cast<u16>(renderer->size_px + 2);
						}
					}
				}

				max_w = std::max(max_w, text_width);
				width = static_cast<u16>(ceilf(max_w));
			}
		};

		struct layout_container : public overlay_element
		{
			std::vector<std::unique_ptr<overlay_element>> m_items;
			u16 advance_pos = 0;
			u16 pack_padding = 0;
			u16 scroll_offset_value = 0;
			bool auto_resize = true;

			virtual overlay_element* add_element(std::unique_ptr<overlay_element>&, int = -1) = 0;

			layout_container()
			{
				// Transparent by default
				back_color.a = 0.f;
			}

			void translate(s16 _x, s16 _y) override
			{
				overlay_element::translate(_x, _y);

				for (auto &itm : m_items)
					itm->translate(_x, _y);
			}

			void set_pos(u16 _x, u16 _y) override
			{
				s16 dx = static_cast<s16>(_x - x);
				s16 dy = static_cast<s16>(_y - y);
				translate(dx, dy);
			}

			compiled_resource& get_compiled() override
			{
				if (!is_compiled)
				{
					compiled_resource result = overlay_element::get_compiled();

					for (auto &itm : m_items)
						result.add(itm->get_compiled());

					compiled_resources = result;
				}

				return compiled_resources;
			}

			virtual u16 get_scroll_offset_px() = 0;
		};

		struct vertical_layout : public layout_container
		{
			overlay_element* add_element(std::unique_ptr<overlay_element>& item, int offset = -1) override
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
				else
				{
					auto result = item.get();
					m_items.insert(m_items.begin() + offset, std::move(item));
					return result;
				}
			}

			compiled_resource& get_compiled() override
			{
				if (scroll_offset_value == 0 && auto_resize)
					return layout_container::get_compiled();

				if (!is_compiled)
				{
					compiled_resource result = overlay_element::get_compiled();
					const f32 global_y_offset = static_cast<f32>(-scroll_offset_value);

					for (auto &item : m_items)
					{
						const s32 item_y_limit = s32{item->y} + item->h - scroll_offset_value - y;
						const s32 item_y_base = s32{item->y} - scroll_offset_value - y;

						if (item_y_limit < 0 || item_y_base > h)
						{
							// Out of bounds
							continue;
						}
						else if (item_y_limit > h || item_y_base < 0)
						{
							// Partial render
							areaf clip_rect = areai{ x, y, (x + w), (y + h) };
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

			u16 get_scroll_offset_px() override
			{
				return scroll_offset_value;
			}
		};

		struct horizontal_layout : public layout_container
		{
			overlay_element* add_element(std::unique_ptr<overlay_element>& item, int offset = -1) override
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
				else
				{
					auto result = item.get();
					m_items.insert(m_items.begin() + offset, std::move(item));
					return result;
				}
			}

			compiled_resource& get_compiled() override
			{
				if (scroll_offset_value == 0 && auto_resize)
					return layout_container::get_compiled();

				if (!is_compiled)
				{
					compiled_resource result = overlay_element::get_compiled();
					const f32 global_x_offset = static_cast<f32>(-scroll_offset_value);

					for (auto &item : m_items)
					{
						const s32 item_x_limit = s32{item->x} + item->w - scroll_offset_value - w;
						const s32 item_x_base = s32{item->x} - scroll_offset_value - w;

						if (item_x_limit < 0 || item_x_base > h)
						{
							// Out of bounds
							continue;
						}
						else if (item_x_limit > h || item_x_base < 0)
						{
							// Partial render
							areaf clip_rect = areai{ x, y, (x + w), (y + h) };
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

			u16 get_scroll_offset_px() override
			{
				return scroll_offset_value;
			}
		};

		// Controls
		struct spacer : public overlay_element
		{
			using overlay_element::overlay_element;
			compiled_resource& get_compiled() override
			{
				// No draw
				return compiled_resources;
			}
		};

		struct image_view : public overlay_element
		{
		private:
			u8 image_resource_ref = image_resource_id::none;
			void *external_ref = nullptr;

			// Strength of blur effect
			u8 blur_strength = 0;

		public:
			using overlay_element::overlay_element;

			compiled_resource& get_compiled() override
			{
				if (!is_compiled)
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

					is_compiled = true;
				}

				return compiled_resources;
			}

			void set_image_resource(u8 resource_id)
			{
				image_resource_ref = resource_id;
				external_ref = nullptr;
			}

			void set_raw_image(image_info *raw_image)
			{
				image_resource_ref = image_resource_id::raw_image;
				external_ref = raw_image;
			}

			void set_blur_strength(u8 strength)
			{
				blur_strength = strength;
			}
		};

		struct image_button : public image_view
		{
			const u16 text_horizontal_offset = 25;
			u16 m_text_offset_x = 0;
			s16 m_text_offset_y = 0;

			image_button()
			{
				// Do not clip text to region extents
				// TODO: Define custom clipping region or use two controls to emulate
				clip_text = false;
			}

			image_button(u16 _w, u16 _h)
			{
				clip_text = false;
				set_size(_w, _h);
			}

			void set_text_vertical_adjust(s16 offset)
			{
				m_text_offset_y = offset;
			}

			void set_size(u16 /*w*/, u16 h) override
			{
				image_view::set_size(h, h);
				m_text_offset_x = (h / 2) + text_horizontal_offset; // By default text is at the horizontal center
			}

			compiled_resource& get_compiled() override
			{
				if (!is_compiled)
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
		};

		struct label : public overlay_element
		{
			label() = default;

			label(const std::string& text)
			{
				this->text = text;
			}

			bool auto_resize(bool grow_only = false, u16 limit_w = UINT16_MAX, u16 limit_h = UINT16_MAX)
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
		};

		struct progress_bar : public overlay_element
		{
		private:
			overlay_element indicator;
			label text_view;

			f32 m_limit = 100.f;
			f32 m_value = 0.f;

		public:
			progress_bar();
			void inc(f32 value);
			void dec(f32 value);
			void set_limit(f32 limit);
			void set_value(f32 value);
			void set_pos(u16 _x, u16 _y) override;
			void set_size(u16 _w, u16 _h) override;
			void translate(s16 dx, s16 dy) override;
			void set_text(const char* str) override;
			void set_text(const std::string& str) override;

			compiled_resource& get_compiled() override;
		};

		struct list_view : public vertical_layout
		{
		private:
			std::unique_ptr<image_view> m_scroll_indicator_top;
			std::unique_ptr<image_view> m_scroll_indicator_bottom;
			std::unique_ptr<image_button> m_cancel_btn;
			std::unique_ptr<image_button> m_accept_btn;
			std::unique_ptr<overlay_element> m_highlight_box;

			u16 m_elements_height = 0;
			s32 m_selected_entry = -1;
			u16 m_elements_count = 0;

			bool m_cancel_only = false;

		public:
			list_view(u16 width, u16 height);

			void update_selection();

			void select_entry(s32 entry);
			void select_next(u16 count = 1);
			void select_previous(u16 count = 1);

			void add_entry(std::unique_ptr<overlay_element>& entry);

			int get_selected_index();

			std::string get_selected_item();

			void set_cancel_only(bool cancel_only);
			void translate(s16 _x, s16 _y) override;

			compiled_resource& get_compiled() override;
		};

		struct edit_text : public label
		{
			enum direction
			{
				up,
				down,
				left,
				right
			};

			u16 caret_position = 0;
			u16 vertical_scroll_offset = 0;

			using label::label;

			void move_caret(direction dir);
			void insert_text(const std::string& str);
			void erase();

			compiled_resource& get_compiled() override;
		};

		struct graph : public overlay_element
		{
		private:
			std::string m_title;
			std::vector<f32> m_datapoints;
			u32 m_datapoint_count{};
			color4f m_color;
			f32 m_min{};
			f32 m_max{};
			f32 m_guide_interval{};
			label m_label{};

		public:
			graph();
			void set_pos(u16 _x, u16 _y) override;
			void set_size(u16 _w, u16 _h) override;
			void set_title(const char* title);
			void set_font(const char* font_name, u16 font_size) override;
			void set_font_size(u16 font_size);
			void set_count(u32 datapoint_count);
			void set_color(color4f color);
			void set_guide_interval(f32 guide_interval);
			u16 get_height() const;
			void record_datapoint(f32 datapoint);
			void update();
			compiled_resource& get_compiled() override;
		};
	}
}
