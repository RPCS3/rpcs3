#include "stdafx.h"
#include "overlay_controls.h"
#include "Emu/system_config.h"

namespace rsx
{
	namespace overlays
	{
		void codepage::initialize_glyphs(u16 codepage_id, f32 font_size, const std::vector<u8>& ttf_data)
		{
			glyph_base = (codepage_id * 256);
			glyph_data.resize(bitmap_width * bitmap_height);
			pack_info.resize(256);

			stbtt_pack_context context;
			if (!stbtt_PackBegin(&context, glyph_data.data(), bitmap_width, bitmap_height, 0, 1, nullptr))
			{
				rsx_log.error("Font packing failed");
				return;
			}

			stbtt_PackSetOversampling(&context, oversample, oversample);

			if (!stbtt_PackFontRange(&context, ttf_data.data(), 0, font_size, (codepage_id * 256), 256, pack_info.data()))
			{
				rsx_log.error("Font packing failed");
				stbtt_PackEnd(&context);
				return;
			}

			stbtt_PackEnd(&context);
		}

		stbtt_aligned_quad codepage::get_char(wchar_t c, f32& x_advance, f32& y_advance)
		{
			stbtt_aligned_quad quad;
			stbtt_GetPackedQuad(pack_info.data(), bitmap_width, bitmap_height, (c - glyph_base), &x_advance, &y_advance, &quad, false);

			quad.t0 += sampler_z;
			quad.t1 += sampler_z;
			return quad;
		}

		font::font(const char* ttf_name, f32 size)
		{
			// Convert pt to px
			size_px = ceilf(size * 96.f / 72.f);
			size_pt = size;

			font_name = ttf_name;
			initialized = true;
		}

		language_class font::classify(u16 codepage_id)
		{
			if (codepage_id >= 0x2E && codepage_id <= 0x9F)
			{
				return language_class::cjk;
			}

			return language_class::default_;
		}

		glyph_load_setup font::get_glyph_files(language_class class_)
		{
			glyph_load_setup result;

#ifdef _WIN32
			result.lookup_font_dirs.push_back("C:/Windows/Fonts/");
#else
			char* home = getenv("HOME");
			if (home == nullptr)
				home = getpwuid(getuid())->pw_dir;

			result.lookup_font_dirs.emplace_back(home);
			if (home[result.lookup_font_dirs[0].length() - 1] == '/')
				result.lookup_font_dirs[0] += ".fonts/";
			else
				result.lookup_font_dirs[0] += "/.fonts/";
#endif
			// Search dev_flash for the font too
			result.lookup_font_dirs.push_back(g_cfg.vfs.get_dev_flash() + "data/font/");
			result.lookup_font_dirs.push_back(g_cfg.vfs.get_dev_flash() + "data/font/SONY-CC/");

			switch (class_)
			{
			case language_class::default_:
			{
				// Standard name
				result.font_name = font_name;
#ifdef _WIN32
				result.fallback_fonts.push_back("C:/Windows/Fonts/Arial.ttf");
#else
				fallback_fonts.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"); //	ubuntu
				fallback_fonts.emplace_back("/usr/share/fonts/TTF/DejaVuSans.ttf");             //	arch
#endif
				// Attempt to load a font from dev_flash as a last resort
				result.fallback_fonts.push_back(g_cfg.vfs.get_dev_flash() + "data/font/SCE-PS3-VR-R-LATIN.TTF");
				break;
			}
			case language_class::cjk:
			{
				// Skip loading font files directly
				result.lookup_font_dirs.clear();

				// Attempt to load a font from dev_flash before any other source
				result.fallback_fonts.push_back(g_cfg.vfs.get_dev_flash() + "data/font/SCE-PS3-SR-R-JPN.TTF");
#ifdef _WIN32
				result.fallback_fonts.push_back("C:/Windows/Fonts/Yu Gothic.ttf"); // Japanese only
#else
				// No standard CJK set on linux
#endif
				break;
			}
			}

			return result;
		}

		codepage* font::initialize_codepage(u16 codepage_id)
		{
			// Init glyph
			const auto class_ = classify(codepage_id);
			const auto fs_settings = get_glyph_files(class_);

			// Attemt to load requested font
			std::vector<u8> bytes;
			std::string file_path;
			bool font_found = false;

			for (auto& font_dir : fs_settings.lookup_font_dirs)
			{
				std::string requested_file = font_dir + fs_settings.font_name;

				// Append ".ttf" if not present
				std::string font_lower(requested_file);

				std::transform(requested_file.begin(), requested_file.end(), font_lower.begin(), ::tolower);
				if (!font_lower.ends_with(".ttf"))
					requested_file += ".ttf";

				file_path = requested_file;

				if (fs::is_file(requested_file))
				{
					font_found = true;
					break;
				}
			}

			// Attemt to load a fallback if request font wasn't found
			if (!font_found)
			{
				for (auto& fallback_font : fs_settings.fallback_fonts)
				{
					if (fs::is_file(fallback_font))
					{
						file_path = fallback_font;
						font_found = true;

						rsx_log.notice("Found font file '%s' as a replacement for '%s'", fallback_font, font_name);
						break;
					}
				}
			}

			// Read font
			if (font_found)
			{
				fs::file f(file_path);
				f.read(bytes, f.size());
			}
			else
			{
				rsx_log.error("Failed to initialize font '%s.ttf' on codepage %d", font_name, codepage_id);
				return nullptr;
			}

			codepage_cache.page = nullptr;
			auto page = std::make_unique<codepage>();
			page->initialize_glyphs(codepage_id, size_px, bytes);
			page->sampler_z = static_cast<f32>(m_glyph_map.size());

			auto ret = page.get();
			m_glyph_map.emplace_back(codepage_id, std::move(page));

			if (codepage_id == 0)
			{
				// Latin-1
				f32 unused;
				get_char('m', em_size, unused);
			}

			return ret;
		}

		stbtt_aligned_quad font::get_char(wchar_t c, f32& x_advance, f32& y_advance)
		{
			if (!initialized)
				return {};

			const auto page_id = (c >> 8);
			if (codepage_cache.codepage_id == page_id && codepage_cache.page) [[likely]]
			{
				return codepage_cache.page->get_char(c, x_advance, y_advance);
			}
			else
			{
				codepage_cache.codepage_id = page_id;
				codepage_cache.page = nullptr;

				for (const auto& e : m_glyph_map)
				{
					if (e.first == unsigned(page_id))
					{
						codepage_cache.page = e.second.get();
						break;
					}
				}

				if (!codepage_cache.page) [[unlikely]]
				{
					codepage_cache.page = initialize_codepage(page_id);
				}

				return codepage_cache.page->get_char(c, x_advance, y_advance);
			}
		}

		void font::render_text_ex(std::vector<vertex>& result, f32& x_advance, f32& y_advance, const wchar_t* text, u32 char_limit, u16 max_width, bool wrap)
		{
			x_advance = 0.f;
			y_advance = 0.f;
			result.clear();

			if (!initialized)
			{
				return;
			}

			u32 i                = 0u;
			bool skip_whitespace = false;

			while (true)
			{
				if (auto c = text[i++]; c && (i <= char_limit))
				{
					switch (c)
					{
					case '\n':
					{
						y_advance += size_px + 2.f;
						x_advance = 0.f;
						continue;
					}
					case '\r':
					{
						x_advance = 0.f;
						continue;
					}
					default:
					{
						stbtt_aligned_quad quad;
						if (skip_whitespace && text[i - 1] == ' ')
						{
							quad = {};
						}
						else
						{
							quad            = get_char(c, x_advance, y_advance);
							skip_whitespace = false;
						}

						if (quad.x1 > max_width)
						{
							bool wrapped              = false;
							bool non_whitespace_break = false;

							if (wrap)
							{
								// scan previous chars
								for (int j = i - 1, nb_chars = 0; j > 0; j--, nb_chars++)
								{
									if (text[j] == '\n')
										break;

									if (text[j] == ' ')
									{
										non_whitespace_break = true;
										continue;
									}

									if (non_whitespace_break)
									{
										if (nb_chars > 1)
										{
											nb_chars--;

											auto first_affected = result.size() - (nb_chars * 4);
											f32 base_x          = result[first_affected].values[0];

											for (size_t n = first_affected; n < result.size(); ++n)
											{
												auto char_index = n / 4;
												if (text[char_index] == ' ')
												{
													// Skip character
													result[n++].vec2(0.f, 0.f);
													result[n++].vec2(0.f, 0.f);
													result[n++].vec2(0.f, 0.f);
													result[n].vec2(0.f, 0.f);
													continue;
												}

												result[n].values[0] -= base_x;
												result[n].values[1] += size_px + 2.f;
											}

											x_advance = result.back().values[0];
										}
										else
										{
											x_advance = 0.f;
										}

										wrapped = true;
										y_advance += size_px + 2.f;

										if (text[i - 1] == ' ')
										{
											quad            = {};
											skip_whitespace = true;
										}
										else
										{
											quad = get_char(c, x_advance, y_advance);
										}

										break;
									}
								}
							}

							if (!wrapped)
							{
								// TODO: Ellipsize
								break;
							}
						}

						result.emplace_back(quad.x0, quad.y0, quad.s0, quad.t0);
						result.emplace_back(quad.x1, quad.y0, quad.s1, quad.t0);
						result.emplace_back(quad.x0, quad.y1, quad.s0, quad.t1);
						result.emplace_back(quad.x1, quad.y1, quad.s1, quad.t1);
						break;
					}
					} // switch
				}
				else
				{
					break;
				}
			}
		}

		std::vector<vertex> font::render_text(const wchar_t* text, u16 max_width, bool wrap)
		{
			std::vector<vertex> result;
			f32 unused_x, unused_y;

			render_text_ex(result, unused_x, unused_y, text, UINT32_MAX, max_width, wrap);
			return result;
		}

		std::pair<f32, f32> font::get_char_offset(const wchar_t* text, u16 max_length, u16 max_width, bool wrap)
		{
			std::vector<vertex> unused;
			f32 loc_x, loc_y;

			render_text_ex(unused, loc_x, loc_y, text, max_length, max_width, wrap);
			return {loc_x, loc_y};
		}

		void font::get_glyph_data(std::vector<u8>& bytes) const
		{
			const u32 page_size = codepage::bitmap_width * codepage::bitmap_height;
			const auto size = page_size * m_glyph_map.size();

			bytes.resize(size);
			u8* data = bytes.data();

			for (const auto& e : m_glyph_map)
			{
				std::memcpy(data, e.second->glyph_data.data(), page_size);
				data += page_size;
			}
		}
	} // namespace overlays
} // namespace rsx
