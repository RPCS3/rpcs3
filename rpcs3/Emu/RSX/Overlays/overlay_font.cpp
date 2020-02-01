#include "stdafx.h"
#include "overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		font::font(const char* ttf_name, f32 size)
		{
			// Init glyph
			std::vector<u8> bytes;
			std::vector<std::string> font_dirs;
			std::vector<std::string> fallback_fonts;
#ifdef _WIN32
			font_dirs.push_back("C:/Windows/Fonts/");
			fallback_fonts.push_back("C:/Windows/Fonts/Arial.ttf");
#else
			char* home = getenv("HOME");
			if (home == nullptr)
				home = getpwuid(getuid())->pw_dir;

			font_dirs.emplace_back(home);
			if (home[font_dirs[0].length() - 1] == '/')
				font_dirs[0] += ".fonts/";
			else
				font_dirs[0] += "/.fonts/";

			fallback_fonts.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"); //	ubuntu
			fallback_fonts.emplace_back("/usr/share/fonts/TTF/DejaVuSans.ttf");             //	arch
#endif
			// Search dev_flash for the font too
			font_dirs.push_back(g_cfg.vfs.get_dev_flash() + "data/font/");
			font_dirs.push_back(g_cfg.vfs.get_dev_flash() + "data/font/SONY-CC/");

			// Attempt to load a font from dev_flash as a last resort
			fallback_fonts.push_back(g_cfg.vfs.get_dev_flash() + "data/font/SCE-PS3-VR-R-LATIN.TTF");

			// Attemt to load requested font
			std::string file_path;
			bool font_found = false;
			for (auto& font_dir : font_dirs)
			{
				std::string requested_file = font_dir + ttf_name;

				// Append ".ttf" if not present
				std::string font_lower(requested_file);

				std::transform(requested_file.begin(), requested_file.end(), font_lower.begin(), ::tolower);
				if (font_lower.substr(font_lower.size() - 4) != ".ttf")
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
				for (auto& fallback_font : fallback_fonts)
				{
					if (fs::is_file(fallback_font))
					{
						file_path  = fallback_font;
						font_found = true;

						rsx_log.notice("Found font file '%s' as a replacement for '%s'", fallback_font.c_str(), ttf_name);
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
				rsx_log.error("Failed to initialize font '%s.ttf'", ttf_name);
				return;
			}

			glyph_data.resize(width * height);
			pack_info.resize(256);

			stbtt_pack_context context;
			if (!stbtt_PackBegin(&context, glyph_data.data(), width, height, 0, 1, nullptr))
			{
				rsx_log.error("Font packing failed");
				return;
			}

			stbtt_PackSetOversampling(&context, oversample, oversample);

			// Convert pt to px
			size_px = ceilf(size * 96.f / 72.f);
			size_pt = size;

			if (!stbtt_PackFontRange(&context, bytes.data(), 0, size_px, 0, 256, pack_info.data()))
			{
				rsx_log.error("Font packing failed");
				stbtt_PackEnd(&context);
				return;
			}

			stbtt_PackEnd(&context);

			font_name   = ttf_name;
			initialized = true;

			f32 unused;
			get_char('m', em_size, unused);
		}

		stbtt_aligned_quad font::get_char(char c, f32& x_advance, f32& y_advance)
		{
			if (!initialized)
				return {};

			stbtt_aligned_quad quad;
			stbtt_GetPackedQuad(pack_info.data(), width, height, u8(c), &x_advance, &y_advance, &quad, false);
			return quad;
		}

		void font::render_text_ex(std::vector<vertex>& result, f32& x_advance, f32& y_advance, const char* text, u32 char_limit, u16 max_width, bool wrap)
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
				if (char c = text[i++]; c && (i <= char_limit))
				{
					if (u8(c) >= char_count)
					{
						// Unsupported glyph, render null for now
						c = ' ';
					}

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

		std::vector<vertex> font::render_text(const char* text, u16 max_width, bool wrap)
		{
			std::vector<vertex> result;
			f32 unused_x, unused_y;

			render_text_ex(result, unused_x, unused_y, text, UINT32_MAX, max_width, wrap);
			return result;
		}

		std::pair<f32, f32> font::get_char_offset(const char* text, u16 max_length, u16 max_width, bool wrap)
		{
			std::vector<vertex> unused;
			f32 loc_x, loc_y;

			render_text_ex(unused, loc_x, loc_y, text, max_length, max_width, wrap);
			return {loc_x, loc_y};
		}
	} // namespace overlays
} // namespace rsx
