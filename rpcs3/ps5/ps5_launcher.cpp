// PS5: the title's game launcher (ps5_launcher.h).
//
// The layout is in RSX's virtual 1280x720 space; the art is the games' own,
// read from their folders (PS3_GAME/ICON0.PNG, PS3_GAME/PIC1.PNG), and the
// game list is Big Picture Mode's (game_enumeration over the games folder and
// dev_hdd0/game). A game boots as Big Picture Mode boots one: its shell stops
// and the game boots in one main-thread call, and the game's stop returns here.

#include "stdafx.h"
#include "ps5_launcher.h"
#include "ps5_ui.h"
#include "ps5_pad_handler.h"
#include "ps5_patches.h"

#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/BigPicture/overlay_big_picture.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Utilities/Config.h"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cmath>

LOG_CHANNEL(launcher_log, "Launcher");

void ps5_play_sound_file(const std::string& path); // ps5_sound.cpp
void ps5_stop_sounds();
const std::string& ps5_title_build(); // ps5_frontend.cpp

namespace rsx::overlays
{
	using namespace ps5ui;

	namespace
	{
		// The overlays' 1280x720 space: margins, the top bar's middle line, the hero's left edge
		constexpr s16 c_margin = 40;
		constexpr s16 c_right = 1240;
		constexpr s16 c_bar_y = 36;
		constexpr s16 c_hero_x = 64;

		// The games row: five tiles across the margins. ICON0.PNG is 320x176
		constexpr s16 c_row_y = 500;
		constexpr u16 c_tile_w = 232;
		constexpr u16 c_tile_h = 128;
		constexpr u16 c_tile_gap = 10;
		constexpr u16 c_tile_radius = 8;
		constexpr s32 c_visible_tiles = 5;

		// The hints' line, bottom right
		constexpr s16 c_hints_y = 690;


		// A new game's art fades in over the last one's
		constexpr u64 c_background_fade_us = 220'000;

		void boot_game(std::string path, std::string title_id)
		{
			launcher_log.notice("Booting '%s' (%s)", title_id, path);

			// As Big Picture Mode's own boot_game_from_big_picture_mode: the shell
			// stops and the game boots in one main-thread call, and only then is
			// the session marked as launched from here, so the shell's own stop
			// does not count as the game's
			Emu.CallFromMainThread([path, title_id]()
			{
				launcher_log.notice("Booting '%s': the launcher's shell stops", title_id);
				Emu.SetContinuousMode(true);
				Emu.GracefulShutdown(false);
				launcher_log.notice("Booting '%s': the shell stopped, the game boots", title_id);
				g_big_picture_mode_active = true;

				if (const game_boot_result result = Emu.BootGame(path, title_id); is_error(result))
				{
					launcher_log.error("Booting '%s' failed: %s", path, result);
					g_big_picture_mode_active = false;
					// Back to the launcher rather than an empty screen
					Emu.BootBigPictureMode();
				}
			});
		}

		std::string read_user_name()
		{
			const std::string path = rpcs3::utils::get_hdd0_dir() + "home/" + Emu.GetUsr() + "/localusername";
			if (fs::file file{path})
			{
				std::string name = file.to_string();
				name = name.substr(0, name.find_first_of(std::string_view("\r\n\0", 3)));
				if (!name.empty())
				{
					return name;
				}
			}
			return "User " + Emu.GetUsr();
		}

		// A white ramp, its alpha falling as an eased curve from `from` to 0 along
		// its length; tinted by the view's colour and stretched, it is a fade
		// without the bands of stacked rectangles
		std::unique_ptr<memory_image_info> make_ramp(std::vector<u8>& pixels, u16 length, bool horizontal, f32 from)
		{
			pixels.resize(usz{length} * 4);
			for (u16 i = 0; i < length; i++)
			{
				const f32 t = static_cast<f32>(i) / (length - 1);
				const f32 eased = (1.f - t) * (1.f - t) * (3.f - 2.f * (1.f - t)) * 0.5f + (1.f - t) * (1.f - t) * 0.5f;
				pixels[i * 4 + 0] = 255;
				pixels[i * 4 + 1] = 255;
				pixels[i * 4 + 2] = 255;
				pixels[i * 4 + 3] = static_cast<u8>(std::lround(255.f * from * eased));
			}
			auto image = std::make_unique<memory_image_info>(horizontal ? length : u16{1}, horizontal ? u16{1} : length, u8{4}, pixels.data());
			image->dirty = true;
			return image;
		}

		// Covers: a PS3 case's shape (about 1 wide to 1.16 high)
		constexpr u16 c_cover_w = 360;
		constexpr u16 c_cover_h = 416;

		// A pixel of an RGBA image, bilinear, at (u, v) in 0..1
		void sample(const image_info_base& image, f32 u, f32 v, f32 out[4])
		{
			const u8* data = image.get_data();
			const f32 x = std::clamp(u * image.w - 0.5f, 0.f, image.w - 1.f);
			const f32 y = std::clamp(v * image.h - 0.5f, 0.f, image.h - 1.f);
			const int x0 = static_cast<int>(x), y0 = static_cast<int>(y);
			const int x1 = std::min(x0 + 1, image.w - 1), y1 = std::min(y0 + 1, image.h - 1);
			const f32 fx = x - x0, fy = y - y0;
			for (int c = 0; c < 4; c++)
			{
				const f32 a = data[(y0 * image.w + x0) * 4 + c] * (1 - fx) + data[(y0 * image.w + x1) * 4 + c] * fx;
				const f32 b = data[(y1 * image.w + x0) * 4 + c] * (1 - fx) + data[(y1 * image.w + x1) * 4 + c] * fx;
				out[c] = a * (1 - fy) + b * fy;
			}
		}

		// A cover drawn from the game's own art: its PIC1 (or its icon) filling
		// the case, darkened toward the foot, its icon across the middle, and a
		// band at the top in the app's colours
		void draw_cover(ps5_launcher_game& game)
		{
			const image_info_base* fill = game.background && game.background->get_data() ? game.background.get() : (game.icon && game.icon->get_data() ? game.icon.get() : nullptr);
			const image_info_base* icon = game.icon && game.icon->get_data() ? game.icon.get() : nullptr;

			std::vector<u8>& px = game.cover_pixels;
			px.resize(usz{c_cover_w} * c_cover_h * 4);

			constexpr u16 band = 34;
			f32 rgba[4];

			// The fill's art boiled down to 6x7 averaged pixels: stretched over
			// the case, a soft wash of its colours with nothing busy in it
			constexpr u16 soft_w = 6, soft_h = 7;
			std::vector<u8> soft_pixels(usz{soft_w} * soft_h * 4, 0);
			if (fill)
			{
				const u8* data = fill->get_data();
				for (u16 sy = 0; sy < soft_h; sy++)
				{
					for (u16 sx = 0; sx < soft_w; sx++)
					{
						const int x0 = sx * fill->w / soft_w, x1 = std::max(x0 + 1, (sx + 1) * fill->w / soft_w);
						const int y0 = sy * fill->h / soft_h, y1 = std::max(y0 + 1, (sy + 1) * fill->h / soft_h);
						u64 sum[4]{};
						u64 n = 0;
						for (int yy = y0; yy < y1; yy += 2)
						{
							for (int xx = x0; xx < x1; xx += 2)
							{
								for (int c = 0; c < 4; c++) sum[c] += data[(yy * fill->w + xx) * 4 + c];
								n++;
							}
						}
						for (int c = 0; c < 4; c++) soft_pixels[(sy * soft_w + sx) * 4 + c] = static_cast<u8>(sum[c] / std::max<u64>(n, 1));
					}
				}
			}
			memory_image_info soft(soft_w, soft_h, u8{4}, soft_pixels.data());
			for (u16 y = 0; y < c_cover_h; y++)
			{
				for (u16 x = 0; x < c_cover_w; x++)
				{
					f32 r = 10, g = 14, b = 30;
					if (fill)
					{
						// The art's colours only: its tiny version, stretched
						const f32 u = (x + 0.5f) / c_cover_w;
						const f32 v = (y + 0.5f) / c_cover_h;
						sample(soft, u, v, rgba);
						const f32 shade = 0.7f - 0.35f * (static_cast<f32>(y) / c_cover_h);
						r = rgba[0] * shade, g = rgba[1] * shade, b = rgba[2] * shade;
					}
					if (y < band)
					{
						r = 6, g = 8, b = 18;
					}
					else if (y < band + 3)
					{
						r = 102, g = 222, b = 242;
					}
					u8* out = &px[(usz{y} * c_cover_w + x) * 4];
					out[0] = static_cast<u8>(std::clamp(r, 0.f, 255.f));
					out[1] = static_cast<u8>(std::clamp(g, 0.f, 255.f));
					out[2] = static_cast<u8>(std::clamp(b, 0.f, 255.f));
					out[3] = 255;
				}
			}

			if (icon)
			{
				// The icon at 86% of the width, a little below the middle
				const u16 iw = static_cast<u16>(c_cover_w * 0.86f);
				const u16 ih = static_cast<u16>(iw * static_cast<f32>(icon->h) / icon->w);
				const u16 ix = static_cast<u16>((c_cover_w - iw) / 2);
				const u16 iy = static_cast<u16>(std::min<int>(c_cover_h - ih - 16, static_cast<int>(c_cover_h * 0.56f - ih / 2.f)));
				for (u16 y = 0; y < ih; y++)
				{
					for (u16 x = 0; x < iw; x++)
					{
						sample(*icon, (x + 0.5f) / iw, (y + 0.5f) / ih, rgba);
						u8* out = &px[(static_cast<usz>(iy + y) * c_cover_w + ix + x) * 4];
						const f32 a = rgba[3] / 255.f;
						for (int c = 0; c < 3; c++)
						{
							out[c] = static_cast<u8>(std::clamp(rgba[c] * a + out[c] * (1.f - a), 0.f, 255.f));
						}
					}
				}
			}

			game.cover_drawn = std::make_unique<memory_image_info>(c_cover_w, c_cover_h, u8{4}, px.data());
			game.cover_drawn->dirty = true;
		}

		// Draws `image` into an RGBA canvas at (x, y, w, h), scaled to fill the
		// box and cropped evenly (or, with `fit`, scaled to fit inside it)
		void paint(std::vector<u8>& canvas, u16 canvas_w, const image_info_base& image, int x, int y, int w, int h, bool fit)
		{
			const f32 box = static_cast<f32>(w) / h;
			const f32 art = static_cast<f32>(image.w) / image.h;
			f32 su0 = 0.f, su1 = 1.f, sv0 = 0.f, sv1 = 1.f;
			if (fit)
			{
				if (art > box) { const int nh = static_cast<int>(w / art); y += (h - nh) / 2; h = nh; }
				else { const int nw = static_cast<int>(h * art); x += (w - nw) / 2; w = nw; }
			}
			else if (art > box) { const f32 keep = box / art; su0 = 0.5f - keep / 2; su1 = 0.5f + keep / 2; }
			else { const f32 keep = art / box; sv0 = 0.5f - keep / 2; sv1 = 0.5f + keep / 2; }

			f32 rgba[4];
			for (int py = 0; py < h; py++)
			{
				for (int px = 0; px < w; px++)
				{
					sample(image, su0 + (su1 - su0) * (px + 0.5f) / w, sv0 + (sv1 - sv0) * (py + 0.5f) / h, rgba);
					u8* out = &canvas[(static_cast<usz>(y + py) * canvas_w + x + px) * 4];
					const f32 a = rgba[3] / 255.f;
					for (int c = 0; c < 3; c++)
					{
						out[c] = static_cast<u8>(std::clamp(rgba[c] * a + out[c] * (1.f - a), 0.f, 255.f));
					}
				}
			}
		}

		// A back drawn for the case: the game's icon at the top, its PIC1 below
		// as a screenshot would be, over a deep blue, with a cyan rule and a
		// dark foot like a real back's ratings band
		void draw_back(ps5_launcher_game& game)
		{
			std::vector<u8>& px = game.back_pixels;
			px.assign(usz{c_cover_w} * c_cover_h * 4, 0);
			for (u16 y = 0; y < c_cover_h; y++)
			{
				for (u16 x = 0; x < c_cover_w; x++)
				{
					u8* out = &px[(usz{y} * c_cover_w + x) * 4];
					const f32 t = static_cast<f32>(y) / c_cover_h;
					const bool foot = y >= c_cover_h - 44;
					out[0] = foot ? 8 : static_cast<u8>(10 + 8 * (1 - t));
					out[1] = foot ? 10 : static_cast<u8>(16 + 14 * (1 - t));
					out[2] = foot ? 22 : static_cast<u8>(60 + 40 * (1 - t));
					out[3] = 255;
					if (y >= c_cover_h - 47 && y < c_cover_h - 44 && x >= 20 && x < c_cover_w - 20)
					{
						out[0] = 102, out[1] = 222, out[2] = 242;
					}
				}
			}

			if (game.icon && game.icon->get_data())
			{
				paint(px, c_cover_w, *game.icon, 30, 28, c_cover_w - 60, 150, true);
			}
			if (game.background && game.background->get_data())
			{
				paint(px, c_cover_w, *game.background, 30, 196, c_cover_w - 60, 156, false);
			}

			game.back_drawn = std::make_unique<memory_image_info>(c_cover_w, c_cover_h, u8{4}, px.data());
			game.back_drawn->dirty = true;
		}

		// A spine drawn for the case: a black band with "PS3" at the top, then
		// the game's name down it in the cover's own colour (its left edge's),
		// as a PS3 spine is laid out. Text by stb_truetype, with the title's
		// Inter Bold
		void draw_spine(ps5_launcher_game& game, const std::vector<u8>& font_data)
		{
			constexpr u16 sw = 52, sh = 560;
			constexpr u16 band = 70;
			std::vector<u8>& px = game.spine_pixels;
			px.assign(usz{sw} * sh * 4, 255);

			// The colour of the cover's left edge
			f32 edge[3] = {40.f, 60.f, 120.f};
			if (const image_info_base* cover = game.cover(); cover && cover->get_data())
			{
				f64 sum[3]{};
				int n = 0;
				const int x0 = static_cast<int>(game.cover_crop[0] * cover->w);
				const int x1 = std::max(x0 + 1, static_cast<int>((game.cover_crop[0] + 0.05f * (game.cover_crop[1] - game.cover_crop[0])) * cover->w));
				const int y0 = static_cast<int>((game.cover_crop[2] + 0.15f * (game.cover_crop[3] - game.cover_crop[2])) * cover->h);
				const int y1 = static_cast<int>(game.cover_crop[3] * cover->h);
				for (int y = y0; y < y1; y += 3)
				{
					for (int x = x0; x < x1; x++)
					{
						const u8* p = cover->get_data() + (static_cast<usz>(y) * cover->w + x) * 4;
						for (int c = 0; c < 3; c++) sum[c] += p[c];
						n++;
					}
				}
				if (n)
				{
					for (int c = 0; c < 3; c++) edge[c] = static_cast<f32>(sum[c] / n);
				}
			}
			const bool light = 0.299f * edge[0] + 0.587f * edge[1] + 0.114f * edge[2] > 150.f;

			for (u16 y = 0; y < sh; y++)
			{
				for (u16 x = 0; x < sw; x++)
				{
					u8* out = &px[(usz{y} * sw + x) * 4];
					if (y < band) { out[0] = 10; out[1] = 10; out[2] = 12; }
					else if (y < band + 3) { out[0] = 200; out[1] = 36; out[2] = 44; }
					else
					{
						// A little shading across, as print on a curved spine
						const f32 shade = 0.9f + 0.1f * std::sin(3.14159f * (x + 0.5f) / sw);
						for (int c = 0; c < 3; c++) out[c] = static_cast<u8>(std::clamp(edge[c] * shade, 0.f, 255.f));
					}
					out[3] = 255;
				}
			}

			// Text, turned to read down the spine with its letters' tops toward
			// the front
			const auto write = [&](std::string_view text, f32 pixel_height, int v0, int v_max, const u8 colour[3])
			{
				stbtt_fontinfo font;
				if (font_data.empty() || !stbtt_InitFont(&font, font_data.data(), stbtt_GetFontOffsetForIndex(font_data.data(), 0)))
				{
					return;
				}
				const std::u32string chars = utf8_to_u32string(text);
				f32 scale = stbtt_ScaleForPixelHeight(&font, pixel_height);
				int ascent, descent, gap;
				stbtt_GetFontVMetrics(&font, &ascent, &descent, &gap);

				const auto measure = [&](f32 s)
				{
					f32 w = 0.f;
					for (usz i = 0; i < chars.size(); i++)
					{
						int advance, bearing;
						stbtt_GetCodepointHMetrics(&font, chars[i], &advance, &bearing);
						w += advance * s;
						if (i + 1 < chars.size()) w += stbtt_GetCodepointKernAdvance(&font, chars[i], chars[i + 1]) * s;
					}
					return w;
				};
				// Shrunk to fit the spine's length
				if (const f32 w = measure(scale); w > v_max - v0)
				{
					scale *= (v_max - v0) / w;
				}
				const int th = static_cast<int>(std::ceil((ascent - descent) * scale));
				const int tw = static_cast<int>(std::ceil(measure(scale))) + 2;
				std::vector<u8> mask(static_cast<usz>(tw) * th, 0);
				f32 pen = 0.f;
				const int baseline = static_cast<int>(ascent * scale);
				for (usz i = 0; i < chars.size(); i++)
				{
					int x0, y0, x1, y1;
					stbtt_GetCodepointBitmapBox(&font, chars[i], scale, scale, &x0, &y0, &x1, &y1);
					const int gx = static_cast<int>(pen) + x0, gy = baseline + y0;
					if (x1 > x0 && y1 > y0 && gx >= 0 && gy >= 0 && gx + (x1 - x0) <= tw && gy + (y1 - y0) <= th)
					{
						stbtt_MakeCodepointBitmap(&font, &mask[static_cast<usz>(gy) * tw + gx], x1 - x0, y1 - y0, tw, scale, scale, chars[i]);
					}
					int advance, bearing;
					stbtt_GetCodepointHMetrics(&font, chars[i], &advance, &bearing);
					pen += advance * scale;
					if (i + 1 < chars.size()) pen += stbtt_GetCodepointKernAdvance(&font, chars[i], chars[i + 1]) * scale;
				}

				// Turned a quarter clockwise, centred across the spine
				const int u0 = (sw - th) / 2;
				for (int ty = 0; ty < th; ty++)
				{
					for (int tx = 0; tx < tw; tx++)
					{
						const u8 a = mask[static_cast<usz>(ty) * tw + tx];
						const int u = u0 + (th - 1 - ty), v = v0 + tx;
						if (!a || u < 0 || u >= sw || v < 0 || v >= sh) continue;
						u8* out = &px[(static_cast<usz>(v) * sw + u) * 4];
						for (int c = 0; c < 3; c++) out[c] = static_cast<u8>((colour[c] * a + out[c] * (255 - a)) / 255);
					}
				}
			};

			const u8 white[3] = {245, 245, 245};
			const u8 dark[3] = {16, 18, 26};
			write("PS3", 30.f, 12, band - 8, white);
			write(game.info.name.empty() ? game.info.serial : game.info.name, 26.f, band + 20, sh - 16, light ? dark : white);

			game.spine_drawn = std::make_unique<memory_image_info>(sw, sh, u8{4}, px.data());
			game.spine_drawn->dirty = true;
		}

		// Where an image is not transparent, as (u0, u1, v0, v1)
		void opaque_bounds(const image_info_base& image, f32 crop[4])
		{
			const u8* data = image.get_data();
			int x0 = image.w, x1 = -1, y0 = image.h, y1 = -1;
			for (int y = 0; y < image.h; y++)
			{
				for (int x = 0; x < image.w; x++)
				{
					if (data[(y * image.w + x) * 4 + 3] > 24)
					{
						x0 = std::min(x0, x); x1 = std::max(x1, x);
						y0 = std::min(y0, y); y1 = std::max(y1, y);
					}
				}
			}
			if (x1 < x0 || y1 < y0)
			{
				return;
			}
			crop[0] = static_cast<f32>(x0) / image.w;
			crop[1] = static_cast<f32>(x1 + 1) / image.w;
			crop[2] = static_cast<f32>(y0) / image.h;
			crop[3] = static_cast<f32>(y1 + 1) / image.h;
		}

		std::unique_ptr<image_info> load_any(const std::string& base)
		{
			for (const char* extension : {".png", ".jpg", ".jpeg", ".PNG", ".JPG", ".JPEG"})
			{
				if (auto image = load_image(base + extension))
				{
					return image;
				}
			}
			return nullptr;
		}

		// The player's own cover and back if there are, else ones drawn
		void load_cover(ps5_launcher_game& game)
		{
			if (!game.info.serial.empty())
			{
				const std::string base = fs::get_config_dir(true) + "covers/" + game.info.serial;
				if ((game.cover_file = load_any(base)))
				{
					opaque_bounds(*game.cover_file, game.cover_crop);
				}
				for (const char* suffix : {"-back", "_back", " back"})
				{
					if ((game.back_file = load_any(base + suffix)))
					{
						opaque_bounds(*game.back_file, game.back_crop);
						break;
					}
				}
				for (const char* suffix : {"-spine", "_spine", " spine"})
				{
					if ((game.spine_file = load_any(base + suffix)))
					{
						opaque_bounds(*game.spine_file, game.spine_crop);
						break;
					}
				}
			}
			if (!game.cover_file)
			{
				draw_cover(game);
			}
			if (!game.back_file)
			{
				draw_back(game);
			}
			if (!game.spine_file)
			{
				static const std::vector<u8> font = []
				{
					std::vector<u8> bytes;
					if (fs::file file{"/app0/assets/fonts/Inter-Bold.ttf"})
					{
						file.read(bytes, file.size());
					}
					return bytes;
				}();
				draw_spine(game, font);
			}
		}

		// The region a serial's third letter names
		std::string region_of(std::string_view serial)
		{
			if (serial.size() < 3) return {};
			switch (serial[2])
			{
			case 'U': return "USA";
			case 'E': return "Europe";
			case 'J': case 'P': return "Japan";
			case 'A': case 'H': return "Asia";
			case 'K': return "Korea";
			default: return {};
			}
		}

		// Its timeline, in seconds from the launcher's first frame: two screens,
		// as a PS1 started. On white, shards fly in and lock into a diamond
		// around the mark, and the maker's name comes up under it; on black,
		// the logo and its fine print, then the logo glides into the top bar.
		// The sound (PS5_RPCS3Title's ps5/tools/intro-sound.py) keeps the same
		// moments: move both together
		constexpr f32 c_intro_white_in = 0.45f;    // the screen turns white over this
		constexpr f32 c_intro_shards = 0.15f;      // the shards set off
		constexpr f32 c_intro_lock = 1.5f;         // they lock into the diamond
		constexpr f32 c_intro_name = 1.85f;        // the maker's name comes up
		constexpr f32 c_intro_white_out = 3.5f;    // the white screen fades to black
		constexpr f32 c_intro_logo_in = 4.2f;      // the logo comes up on black
		constexpr f32 c_intro_print = 4.6f;        // its fine print
		constexpr f32 c_intro_move = 6.0f;         // the logo glides into the top bar
		constexpr f32 c_intro_move_length = 0.6f;
		constexpr f32 c_intro_reveal = 6.05f;      // the splash's backdrop fades away
		constexpr f32 c_intro_bar = 6.35f;         // the tabs and the user come down
		constexpr f32 c_intro_content = 6.4f;      // the hero and the row may start
		constexpr f32 c_intro_end = 7.15f;

		// The white screen's diamond: its centre and half its diagonal
		constexpr f32 c_diamond_x = 640.f; // the middle of the overlays' 1280-pixel width
		constexpr f32 c_diamond_y = 300.f;
		constexpr f32 c_diamond_r = 170.f;
		constexpr u16 c_mark_size = 150;

		// Each opening's own, from when the list is read (and the splash allows)
		constexpr f32 c_content_end = 1.3f;

		// Played once per run of the app
		bool s_intro_played = false;
	}

	ps5_launcher_dialog::ps5_launcher_dialog()
	{
		m_allow_input_on_pause = true;
		m_fade_animation.duration_sec = 0.2f;
		m_play_intro = !std::exchange(s_intro_played, true);
		return_code = selection_code::canceled;

		build_static();
		fs::create_path(fs::get_config_dir(true) + "covers/");


		start_reload();
	}

	ps5_launcher_dialog::~ps5_launcher_dialog()
	{
		m_delete_thread.reset();
		m_pt_thread.reset();
		m_tr_thread.reset();
		m_installer.reset();

		if (m_enumeration_thread)
		{
			*m_enumeration_thread = thread_state::aborting;
			(*m_enumeration_thread)();
			m_enumeration_thread.reset();
		}
	}

	void ps5_launcher_dialog::build_static()
	{
		// Background: the art, a light wash, and the fades that seat the text on it
		m_backdrop.set_size(virtual_width, virtual_height);
		m_backdrop.back_color = c_backdrop;
		for (image_view* view : {&m_background, &m_background_prev})
		{
			view->set_size(virtual_width, virtual_height);
			view->back_color.a = 0.f;
		}

		m_wash.set_size(virtual_width, virtual_height);
		m_wash.back_color = color4f(0.f, 0.f, 0.f, 0.12f);

		m_fade_left_image = make_ramp(m_fade_left_pixels, 256, true, 0.94f);
		m_fade_left.set_raw_image(m_fade_left_image.get());
		m_fade_left.set_size(820, virtual_height);

		m_fade_top_image = make_ramp(m_fade_top_pixels, 128, false, 0.6f);
		m_fade_top.set_raw_image(m_fade_top_image.get());
		m_fade_top.set_size(virtual_width, 130);

		// The bottom fade rises from the screen's foot: its ramp runs upward
		m_fade_bottom_image = make_ramp(m_fade_bottom_pixels, 256, false, 0.96f);
		std::reverse(reinterpret_cast<u32*>(m_fade_bottom_pixels.data()), reinterpret_cast<u32*>(m_fade_bottom_pixels.data()) + 256);
		m_fade_bottom.set_raw_image(m_fade_bottom_image.get());
		m_fade_bottom.set_size(virtual_width, 360);
		m_fade_bottom.set_pos(0, virtual_height - 360);

		// The Library's glow: a soft disc, tinted and stretched behind the covers
		m_glow_image = make_glow(m_glow_pixels);
		// The cases' sheen: a soft diagonal band of light, slid across the front
		// as the case turns
		{
			constexpr u16 n = 128;
			m_sheen_pixels.resize(usz{n} * n * 4);
			for (u16 yy = 0; yy < n; yy++)
			{
				for (u16 xx = 0; xx < n; xx++)
				{
					const f32 d = (xx + 0.45f * yy) / n - 0.72f;
					const f32 a = std::exp(-d * d / 0.006f) * 0.9f + std::exp(-d * d / 0.05f) * 0.25f;
					u8* px = &m_sheen_pixels[(usz{yy} * n + xx) * 4];
					px[0] = px[1] = px[2] = 255;
					px[3] = static_cast<u8>(std::clamp(a, 0.f, 1.f) * 255.f);
				}
			}
			m_sheen_image = std::make_unique<memory_image_info>(n, n, u8{4}, m_sheen_pixels.data());
			m_sheen_image->dirty = true;
		}
		for (auto [data, path] : {std::pair{&m_flip_icon_data, "home/32/rotate-left-solid.png"}, std::pair{&m_back_icon_data, "home/32/circle-left-solid.png"}})
		{
			*data = resource_config::load_icon(path);
			if (*data)
			{
				(*data)->dirty = true;
			}
		}
		m_library_art.set_size(virtual_width, virtual_height);
		m_library_art.back_color.a = 0.f;
		m_library_art.set_blur_strength(60);

		for (image_view* fade : {&m_fade_left, &m_fade_top, &m_fade_bottom})
		{
			fade->fore_color = c_backdrop;
			fade->back_color.a = 0.f;
		}

		// Top bar: the logo, a divider, the tabs, the user
		m_logo_data = load_image("/app0/assets/launcher/rpcs3-logo.png");
		if (m_logo_data)
		{
			// Drawn at three times the virtual space
			m_logo.set_raw_image(m_logo_data.get());
			m_logo.set_size(static_cast<u16>(m_logo_data->w / 3), static_cast<u16>(m_logo_data->h / 3));
			m_logo.back_color.a = 0.f;
			m_logo.set_pos(c_margin, static_cast<s16>(c_bar_y - m_logo.h / 2));
		}
		else
		{
			style_label(m_logo_text, "RPCS3", 15, f_bold, c_text);
			place(m_logo_text, c_margin, c_bar_y);
		}
		const s16 logo_right = m_logo_data ? static_cast<s16>(m_logo.x + m_logo.w) : static_cast<s16>(m_logo_text.x + m_logo_text.w);

		// The intro's white screen and its words
		m_mark_data = load_image("/app0/assets/launcher/mark.png");
		style_label(m_intro_name, spaced("KONGATIME"), 20, f_semibold, color4f(0.05f, 0.1f, 0.3f, 1.f));
		m_intro_name.set_pos(static_cast<s16>((virtual_width - m_intro_name.w) / 2), 0);
		place(m_intro_name, m_intro_name.x, c_diamond_y + c_diamond_r + 52.f);
		style_label(m_intro_print, "RPCS3 is free software, under the GNU General Public License, version 2.", 10, f_regular, color4f(0.6f, 0.63f, 0.72f, 1.f));
		m_intro_print.set_pos(static_cast<s16>((virtual_width - m_intro_print.w) / 2), 0);

		m_bar_divider.set_pos(static_cast<s16>(logo_right + 22), c_bar_y - 12);
		m_bar_divider.set_size(1, 24);
		m_bar_divider.back_color = color4f(1.f, 1.f, 1.f, 0.28f);

		for (const char* name : {"Home", "Library", "Trophies", "Settings", "Install"})
		{
			m_tab_labels.push_back(make_label(name, 13, f_medium, c_text_dim));
		}
		m_tab_underline.border_radius = 2;
		m_tab_underline.back_color = c_accent;

		const std::string user = read_user_name();
		style_label(m_user_name, user, 12, f_semibold, c_text);
		place(m_user_name, static_cast<s16>(c_right - m_user_name.w), c_bar_y);

		m_avatar.set_size(30, 30);
		m_avatar.set_pos(static_cast<s16>(m_user_name.x - 12 - 30), c_bar_y - 15);
		m_avatar.back_color = c_accent;

		// Which build runs, so a test is never read against the wrong one
		if (!ps5_title_build().empty())
		{
			style_label(m_build, "Build " + ps5_title_build(), 11, f_medium, c_text_dim);
			place(m_build, static_cast<s16>(m_avatar.x - 24 - m_build.w), c_bar_y);
		}

		// The initial, centred on the circle by its drawn shape, not its advance
		const std::u32string initial = utf8_to_u32string(user).substr(0, 1);
		std::u32string upper = initial;
		if (!upper.empty() && upper[0] < 0x80)
		{
			upper[0] = static_cast<char32_t>(std::toupper(static_cast<int>(upper[0])));
		}
		m_avatar_letter.set_font(13, f_bold);
		m_avatar_letter.set_unicode_text(upper);
		m_avatar_letter.fore_color = c_button_text;
		m_avatar_letter.back_color.a = 0.f;
		m_avatar_letter.set_padding(0);
		m_avatar_letter.auto_resize();
		{
			font* renderer = m_avatar_letter.get_font();
			const ink shape = measure_ink(renderer, upper);
			const f32 cx = m_avatar.x + m_avatar.w / 2.f;
			const f32 cy = m_avatar.y + m_avatar.h / 2.f;
			m_avatar_letter.set_pos(static_cast<s16>(std::lround(cx - (shape.left + shape.right) / 2.f)),
				static_cast<s16>(std::lround(cy - renderer->get_size_px() - (shape.top + shape.bottom) / 2.f)));
		}

		// Hero
		style_label(m_welcome, spaced("WELCOME BACK"), 10, f_semibold, c_text_dim);
		place(m_welcome, c_hero_x, 146);

		m_title.set_font(40, f_bold);
		m_title.fore_color = c_text;
		m_title.back_color.a = 0.f;
		m_title.set_padding(0);
		m_title.set_wrap_text(true);
		m_title.set_pos(c_hero_x - 2, 166);
		m_title.set_size(600, 120);

		m_play_button.set_size(176, 44);
		m_play_button.border_radius = 22;
		m_play_button.back_color = c_button;

		m_play_icon_data = resource_config::load_icon("home/32/play-button-arrowhead.png");
		m_play_icon.set_size(16, 16);
		m_play_icon.back_color.a = 0.f;
		if (m_play_icon_data)
		{
			m_play_icon_data->dirty = true;
			m_play_icon.set_raw_image(m_play_icon_data.get());
			m_play_icon.fore_color = c_button_text;
		}
		style_label(m_play_label, "Play now", 14, f_semibold, c_button_text);

		const auto round_button = [](ellipse& button, image_view& icon, std::unique_ptr<image_info>& data, const std::string& path, label& text, std::string_view caption)
		{
			button.set_size(44, 44);
			button.back_color = c_glass;
			button.border_size = 1;
			button.border_color = c_glass_border;
			data = path.starts_with("/") ? load_image(path) : resource_config::load_icon(path);
			icon.set_size(20, 20);
			icon.back_color.a = 0.f;
			if (data)
			{
				data->dirty = true;
				icon.set_raw_image(data.get());
			}
			style_label(text, caption, 12, f_medium, c_text);
		};
		round_button(m_settings_button, m_settings_icon, m_settings_icon_data, "home/32/settings.png", m_settings_label, "Game settings");
		round_button(m_patches_button, m_patches_icon, m_patches_icon_data, "/app0/assets/launcher/patch.png", m_patches_label, "Patches");
		round_button(m_delete_button, m_delete_icon, m_delete_icon_data, "/app0/assets/launcher/trash.png", m_delete_label, "Delete");

		// The delete confirmation, centred over a dimmed screen
		m_confirm_dim.set_size(virtual_width, virtual_height);
		m_confirm_dim.back_color = color4f(0.f, 0.f, 0.f, 0.62f);
		m_confirm_panel.set_size(600, 220);
		m_confirm_panel.set_pos((virtual_width - 600) / 2, (virtual_height - 220) / 2);
		m_confirm_panel.border_radius = 20;
		m_confirm_panel.back_color = color4f(0.06f, 0.07f, 0.12f, 0.98f);
		m_confirm_panel.border_size = 1;
		m_confirm_panel.border_color = color4f(1.f, 1.f, 1.f, 0.16f);
		style_label(m_confirm_title, "", 17, f_semibold, c_text);
		m_confirm_title.set_wrap_text(true);
		style_label(m_confirm_body, "", 12, f_regular, c_text_dim);
		m_confirm_body.set_wrap_text(true);

		const auto make_hint = [](hint& target, u8 image, std::string_view text)
		{
			target.icon.set_image_resource(image);
			target.icon.set_size(18, 18);
			target.icon.back_color.a = 0.f;
			style_label(target.text, text, 11, f_medium, c_text);
		};
		make_hint(m_confirm_yes, resource_config::confirm_button_resource(), "Delete");
		make_hint(m_confirm_no, resource_config::cancel_button_resource(), "Cancel");

		// The games row
		style_label(m_row_title, "Your games", 12, f_semibold, c_text);
		place(m_row_title, c_margin, c_row_y - 24);
		m_row_rule.set_pos(static_cast<s16>(c_margin + m_row_title.w + 18), c_row_y - 24);
		m_row_rule.set_size(static_cast<u16>(c_right - m_row_rule.x), 1);
		m_row_rule.back_color = color4f(1.f, 1.f, 1.f, 0.2f);

		// A rim: filled, behind the tile, which covers all but its edge. (An
		// outline-only rounded_rect leaks a hairline along its diagonal, where
		// its two triangles meet, across whatever it is drawn over)
		m_highlight.border_radius = c_tile_radius + 3;
		m_highlight.back_color = c_accent;
		m_highlight.set_size(c_tile_w + 6, c_tile_h + 6);

		style_label(m_placeholder, "Looking for games...", 13, f_regular, c_text_dim);
		place(m_placeholder, c_margin, c_row_y + 40);

		build_trophies_static();

		layout_tabs();
		layout_home();
	}

	void ps5_launcher_dialog::layout_tabs()
	{
		s16 x = static_cast<s16>(m_bar_divider.x + 30);
		for (usz i = 0; i < m_tab_labels.size(); i++)
		{
			label& tab_label = *m_tab_labels[i];
			const bool active = i == static_cast<usz>(m_tab);
			tab_label.set_font(13, active ? f_semibold : f_medium);
			tab_label.fore_color = active ? c_text : c_text_dim;
			tab_label.auto_resize();
			place(tab_label, x, c_bar_y);
			tab_label.refresh();

			if (active)
			{
				m_tab_underline.set_pos(static_cast<s16>(x - 10), c_bar_y + 16);
				m_tab_underline.set_size(static_cast<u16>(tab_label.w + 20), 3);
				m_tab_underline.refresh();
			}

			x = static_cast<s16>(x + tab_label.w + 34);
		}

		layout_hints();
	}

	void ps5_launcher_dialog::layout_hints()
	{
		// Right-aligned on one line: what the buttons do here
		std::vector<std::pair<u8, std::string_view>> hints;
		if (m_gs_open)
		{
			hints.emplace_back(resource_config::confirm_button_resource(), "Change");
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::square), "Use global");
			hints.emplace_back(resource_config::cancel_button_resource(), "Save and close");
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::L1), "");
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::R1), "Sections");
		}
		else if (m_pt_open)
		{
			const patch_row* row = m_pt && m_pt_selected >= 0 && static_cast<usz>(m_pt_selected) < m_pt_rows.size() ? &m_pt_rows[m_pt_selected] : nullptr;
			if (row && row->value < 0)
			{
				hints.emplace_back(resource_config::confirm_button_resource(), m_pt->patches[row->patch].enabled ? "Turn off" : "Turn on");
			}
			else if (row)
			{
				hints.emplace_back(resource_config::confirm_button_resource(), "Change");
				hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::square), "Default");
			}
			hints.emplace_back(resource_config::cancel_button_resource(), "Save and close");
			if (std::count_if(m_pt_rows.begin(), m_pt_rows.end(), [](const patch_row& r) { return !r.heading.empty(); }) > 1)
			{
				hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::L1), "");
				hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::R1), "Sections");
			}
		}
		else if (m_tab == tab::install && m_installer && !m_installer->done())
		{
			hints.emplace_back(resource_config::cancel_button_resource(), m_installer->cancelled() ? "Stopping..." : "Stop");
		}
		else if (m_tab == tab::install && m_installer)
		{
			hints.emplace_back(resource_config::confirm_button_resource(), "Done");
		}
		else if (m_tab == tab::install)
		{
			const s32 files = static_cast<s32>(m_in_files.size());
			const bool on_file = m_in_row >= 3 && m_in_row - 3 < files;
			hints.emplace_back(resource_config::confirm_button_resource(), m_in_row == 0 ? "Install" : m_in_row == 1 ? "Change" : m_in_row == 2 ? "Look again" : on_file && m_in_files[m_in_row - 3].chosen ? "Leave out" : "Choose");
			hints.emplace_back(resource_config::cancel_button_resource(), "Home");
		}
		else if (m_tab == tab::trophies && m_tr_open)
		{
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::square), "Sort");
			hints.emplace_back(resource_config::cancel_button_resource(), "Close");
			if (m_tr_games.size() > 1)
			{
				hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::L1), "");
				hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::R1), "Games");
			}
		}
		else if (m_tab == tab::trophies)
		{
			if (!m_tr_games.empty() && !m_tr_loading)
			{
				hints.emplace_back(resource_config::confirm_button_resource(), "Show trophies");
			}
			hints.emplace_back(resource_config::cancel_button_resource(), "Home");
		}
		else if (m_tab == tab::library && m_detail)
		{
			hints.emplace_back(resource_config::confirm_button_resource(), "Select");
			hints.emplace_back(resource_config::cancel_button_resource(), "Back");
		}
		else if (m_tab == tab::library && !m_games.empty())
		{
			hints.emplace_back(resource_config::confirm_button_resource(), "Open");
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::triangle), "Settings");
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::square), "Delete");
		}
		else if (m_tab != tab::settings && !m_games.empty())
		{
			hints.emplace_back(resource_config::confirm_button_resource(), m_focus == focus::tiles ? "Play" : "Select");
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::triangle), "Settings");
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::square), "Delete");
		}
		else if (m_tab == tab::settings && m_set_focus_list)
		{
			hints.emplace_back(resource_config::confirm_button_resource(), "Change");
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::square), "Default");
			hints.emplace_back(resource_config::cancel_button_resource(), "Back");
		}
		else if (m_tab == tab::settings)
		{
			hints.emplace_back(resource_config::confirm_button_resource(), "Open");
			hints.emplace_back(resource_config::cancel_button_resource(), "Home");
		}
		if (!m_gs_open && !m_pt_open && !m_tr_open && !installing())
		{
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::L1), "");
			hints.emplace_back(static_cast<u8>(resource_config::standard_image_resource::R1), "Tabs");
		}

		m_hints.clear();
		for (const auto& [image, text] : hints)
		{
			auto entry = std::make_unique<hint>();
			entry->icon.set_image_resource(image);
			entry->icon.set_size(20, 20);
			entry->icon.back_color.a = 0.f;
			style_label(entry->text, text, 11, f_medium, c_text_dim);
			m_hints.push_back(std::move(entry));
		}

		// A glyph with no words (L1) sits close to the next one (R1, "Tabs")
		s16 x = c_right;
		for (usz i = m_hints.size(); i-- > 0;)
		{
			hint& entry = *m_hints[i];
			if (!entry.text.text.empty())
			{
				x = static_cast<s16>(x - entry.text.w);
				place(entry.text, x, c_hints_y);
				x = static_cast<s16>(x - 7);
			}
			x = static_cast<s16>(x - 20);
			entry.icon.set_pos(x, c_hints_y - 10);
			x = static_cast<s16>(x - ((i > 0 && m_hints[i - 1]->text.text.empty()) ? 4 : 24));
		}
	}

	void ps5_launcher_dialog::layout_confirm()
	{
		const s16 left = static_cast<s16>(m_confirm_panel.x + 36);
		m_confirm_title.set_pos(left, static_cast<s16>(m_confirm_panel.y + 32));
		m_confirm_title.set_size(528, 60);
		m_confirm_title.auto_resize(false, 528, 60);
		m_confirm_body.set_pos(left, static_cast<s16>(m_confirm_title.y + m_confirm_title.h + 14));
		m_confirm_body.set_size(528, 80);
		m_confirm_body.auto_resize(false, 528, 80);

		const f32 hints_y = m_confirm_panel.y + m_confirm_panel.h - 36.f;
		s16 x = left;
		for (hint* entry : {&m_confirm_yes, &m_confirm_no})
		{
			entry->icon.set_pos(x, static_cast<s16>(hints_y - 9));
			entry->text.auto_resize();
			place(entry->text, static_cast<s16>(x + 26), hints_y);
			x = static_cast<s16>(x + 26 + entry->text.w + 28);
		}
	}

	void ps5_launcher_dialog::layout_home()
	{
		// The title's lines decide where the chips and the buttons go
		const ps5_launcher_game* game = (m_selected >= 0 && static_cast<usz>(m_selected) < m_games.size()) ? &m_games[m_selected] : nullptr;

		// Anchored at the buttons' line, as the design: the title grows upward
		// from the chips, and the greeting sits over its first line
		constexpr s16 buttons_y = 404;
		constexpr s16 chips_y = 350;

		m_title.set_text(game ? (game->info.name.empty() ? game->info.serial : game->info.name) : std::string(m_loading ? "" : "No games yet"));
		m_title.set_size(600, 120);
		m_title.auto_resize(false, 600, 120);
		{
			font* renderer = m_title.get_font();
			const ink cap = measure_ink(renderer, U"H");
			// The last line's baseline sits 20 above the chips
			const f32 last_baseline = (game ? chips_y : buttons_y) - 20.f;
			const f32 extra_lines = std::max(0.f, static_cast<f32>(m_title.h) - renderer->get_size_px());
			m_title.set_pos(c_hero_x - 2, static_cast<s16>(std::lround(last_baseline - extra_lines - renderer->get_size_px())));
			place(m_welcome, c_hero_x, m_title.y + renderer->get_size_px() + cap.top - 24.f);
			m_welcome.refresh();
		}
		s16 y = chips_y;

		// Chips: what the game's PARAM.SFO says of it
		m_chips.clear();
		m_chip_labels.clear();
		if (game)
		{
			std::vector<std::string> chips;
			if (game->info.category == "DG") chips.push_back("Disc");
			else if (game->info.category == "HG") chips.push_back("Digital");
			if (!game->info.serial.empty()) chips.push_back(game->info.serial);
			if (!game->info.app_ver.empty() && game->info.app_ver != "Unknown") chips.push_back("Version " + game->info.app_ver);

			s16 x = c_hero_x;
			for (const std::string& text : chips)
			{
				auto chip_label = make_label(text, 10, f_medium, c_text);
				auto chip = std::make_unique<rounded_rect>();
				chip->border_radius = 12;
				chip->back_color = c_glass;
				chip->border_size = 1;
				chip->border_color = color4f(1.f, 1.f, 1.f, 0.14f);
				chip->set_pos(x, y);
				chip->set_size(static_cast<u16>(chip_label->w + 30), 24);
				place(*chip_label, static_cast<s16>(x + 15), y + 12.f);
				x = static_cast<s16>(x + chip->w + 8);
				m_chips.push_back(std::move(chip));
				m_chip_labels.push_back(std::move(chip_label));
			}
		}
		y = buttons_y;

		const f32 mid = y + 22.f;
		m_play_button.set_pos(c_hero_x, y);
		m_play_icon.set_pos(static_cast<s16>(c_hero_x + 30), static_cast<s16>(mid - 8));
		place(m_play_label, static_cast<s16>(c_hero_x + 58), mid);

		m_settings_button.set_pos(static_cast<s16>(c_hero_x + m_play_button.w + 22), y);
		m_settings_icon.set_pos(static_cast<s16>(m_settings_button.x + 12), static_cast<s16>(y + 12));
		place(m_settings_label, static_cast<s16>(m_settings_button.x + 44 + 14), mid);

		m_patches_button.set_pos(static_cast<s16>(m_settings_label.x + m_settings_label.w + 30), y);
		m_patches_icon.set_pos(static_cast<s16>(m_patches_button.x + 12), static_cast<s16>(y + 12));
		place(m_patches_label, static_cast<s16>(m_patches_button.x + 44 + 14), mid);

		m_delete_button.set_pos(static_cast<s16>(m_patches_label.x + m_patches_label.w + 30), y);
		m_delete_icon.set_pos(static_cast<s16>(m_delete_button.x + 12), static_cast<s16>(y + 12));
		place(m_delete_label, static_cast<s16>(m_delete_button.x + 44 + 14), mid);

		for (overlay_element* element : std::initializer_list<overlay_element*>{&m_title, &m_play_button, &m_play_icon, &m_play_label, &m_settings_button,
				&m_settings_icon, &m_settings_label, &m_patches_button, &m_patches_icon, &m_patches_label, &m_delete_button, &m_delete_icon, &m_delete_label})
		{
			element->refresh();
		}

		// The background: the game's art, or its icon blurred, or none; the last
		// one stays beneath while the new one fades in
		const image_info_base* next = game ? (game->background ? game->background.get() : game->icon.get()) : nullptr;
		if (next != m_background_image)
		{
			m_background_fading = m_background_image && next;
			if (m_background_fading)
			{
				m_background_prev.set_raw_image(m_background_image);
				m_background_prev.set_blur_strength(m_background_blur);
				m_background_prev.fore_color = color4f(1.f);
				m_background_prev.refresh();
				m_background_fade_start = 0; // set by the next update
			}

			m_background_image = next;
			m_background_blur = (game && !game->background) ? 80 : 0;
			if (next)
			{
				m_background.set_blur_strength(m_background_blur);
				m_background.set_raw_image(next);
				m_background.fore_color = color4f(1.f, 1.f, 1.f, m_background_fading ? 0.f : 1.f);
				m_background.refresh();
			}
		}

		// The visible tiles, with the selected one kept in view
		if (m_selected < m_first_visible)
		{
			m_first_visible = m_selected;
		}
		else if (m_selected >= m_first_visible + c_visible_tiles)
		{
			m_first_visible = m_selected - c_visible_tiles + 1;
		}

		m_tiles.clear();
		m_tile_labels.clear();
		for (s32 i = 0; i < c_visible_tiles && static_cast<usz>(m_first_visible + i) < m_games.size(); i++)
		{
			const ps5_launcher_game& entry = m_games[m_first_visible + i];
			const s16 x = static_cast<s16>(c_margin + i * (c_tile_w + c_tile_gap));
			const bool selected = m_first_visible + i == m_selected;

			auto tile = std::make_unique<rounded_image>();
			tile->border_radius = c_tile_radius;
			tile->set_pos(x, c_row_y);
			tile->set_size(c_tile_w, c_tile_h);
			tile->back_color = color4f(1.f, 1.f, 1.f, 0.08f);
			if (entry.icon)
			{
				tile->set_raw_image(entry.icon.get());
			}
			else
			{
				tile->set_image_resource(resource_config::standard_image_resource::new_entry);
			}

			auto name = make_label("", 12, selected ? f_semibold : f_medium, selected ? c_text : c_text_dim);
			fit_text(*name, entry.info.name.empty() ? entry.info.serial : entry.info.name, c_tile_w - 4);
			place(*name, static_cast<s16>(x + 2), c_row_y + c_tile_h + 22.f);

			if (selected)
			{
				m_highlight.set_pos(static_cast<s16>(x - 3), static_cast<s16>(c_row_y - 3));
				m_highlight.refresh();
			}

			m_tiles.push_back(std::move(tile));
			m_tile_labels.push_back(std::move(name));
		}

		if (!m_loading)
		{
			style_label(m_placeholder, "No games found. Put each game's folder in /data/homebrew/PPSA99200/rpcs3/games/", 13, f_regular, c_text_dim);
			place(m_placeholder, c_margin, c_row_y + 40);
		}

		layout_focus();
	}

	void ps5_launcher_dialog::layout_focus()
	{
		// The focused button gets the accent's ring; the tile ring is bright
		// while the row has the focus and faint while a button has it
		const auto outline = [](overlay_element& button, bool focused, u8 idle_border, const color4f& idle_color)
		{
			button.border_size = focused ? 3 : idle_border;
			button.border_color = focused ? c_accent : idle_color;
			button.refresh();
		};
		outline(m_play_button, m_focus == focus::play, 0, c_glass_border);
		outline(m_settings_button, m_focus == focus::settings, 1, c_glass_border);
		outline(m_patches_button, m_focus == focus::patches, 1, c_glass_border);
		outline(m_delete_button, m_focus == focus::remove, 1, c_glass_border);

		m_highlight.back_color = m_focus == focus::tiles ? c_accent : color4f(c_accent.r, c_accent.g, c_accent.b, 0.3f);
		m_highlight.refresh();

		layout_hints();
	}

	void ps5_launcher_dialog::set_focus(focus next)
	{
		if (next == m_focus)
		{
			return;
		}

		m_focus = next;
		play_sound(sound_effect::cursor);
		layout_focus();
	}

	void ps5_launcher_dialog::ask_delete()
	{
		if (m_selected < 0 || static_cast<usz>(m_selected) >= m_games.size())
		{
			return;
		}

		const big_picture_game_info& info = m_games[m_selected].info;
		m_confirm_title.set_text("Delete " + (info.name.empty() ? info.serial : info.name) + "?");
		m_confirm_body.set_text("This removes the game's files, its compiled code and its place in the library from the console. Saves are kept. It can't be undone.");
		m_confirm_yes.text.set_text("Delete");
		m_confirm_no.icon.set_visible(true);
		m_confirm_no.text.set_visible(true);
		layout_confirm();
		m_confirm_delete = true;
		m_delete_result.clear();
		play_sound(sound_effect::dialog_ok);
	}

	void ps5_launcher_dialog::delete_selected()
	{
		if (m_selected < 0 || static_cast<usz>(m_selected) >= m_games.size() || m_deleting)
		{
			return;
		}

		const big_picture_game_info info = m_games[m_selected].info;

		// Only a folder directly in the games folder or dev_hdd0/game is ever
		// removed: the one the game's path starts in
		std::string root;
		for (std::string base : {rpcs3::utils::get_games_dir(), rpcs3::utils::get_hdd0_game_dir()})
		{
			if (!base.ends_with('/'))
			{
				base += '/';
			}
			if (info.path.starts_with(base) && info.path.size() > base.size())
			{
				const std::string rest = info.path.substr(base.size());
				const std::string first = rest.substr(0, rest.find('/'));
				if (!first.empty() && first != "." && first != ".." && !first.starts_with("$") && !first.starts_with(".") && !first.starts_with("\xef"))
				{
					root = base + first;
				}
			}
		}

		m_confirm_delete = false;
		m_detail = false;
		m_deleting = true;
		m_confirm_title.set_text("Deleting " + (info.name.empty() ? info.serial : info.name) + "...");
		m_confirm_body.set_text("This can take a while for a large game.");
		layout_confirm();

		m_delete_thread = std::make_unique<named_thread<std::function<void()>>>("Launcher Delete", [this, info, root]()
		{
			std::string result;

			if (root.empty())
			{
				launcher_log.error("Not deleting '%s': its path (%s) is not in the games folder or dev_hdd0/game", info.serial, info.path);
				result = "This game isn't in the app's games folder, so its files were left alone. It was removed from the library.";
			}
			else if (fs::is_dir(root) && !fs::remove_all(root))
			{
				launcher_log.error("Deleting %s failed: %s", root, fs::g_tls_error);
				result = fmt::format("Some of the files couldn't be deleted (%s). Delete the folder over FTP instead:\n%s", fs::g_tls_error, root.substr(root.find("/rpcs3/") == umax ? 0 : root.find("/rpcs3/") + 1));
			}
			else
			{
				launcher_log.notice("Deleted %s", root);
			}

			// Its compiled code and its entry in games.yml
			if (!info.serial.empty())
			{
				const std::string cache = rpcs3::utils::get_cache_dir_by_serial(info.serial);
				if (!cache.empty() && fs::is_dir(cache))
				{
					fs::remove_all(cache);
				}
				if (const std::string config = rpcs3::utils::get_custom_config_path(info.serial); fs::is_file(config))
				{
					fs::remove_file(config);
				}
				Emu.RemoveGames({info.serial});
			}

			{
				std::lock_guard lock(m_mutex);
				m_delete_result = result;
				if (!result.empty())
				{
					m_confirm_title.set_text("Couldn't delete everything");
					m_confirm_body.set_text(result);
					m_confirm_yes.text.set_text("OK");
					m_confirm_no.icon.set_visible(false);
					m_confirm_no.text.set_visible(false);
					layout_confirm();
				}
			}

			m_deleting = false;
			m_reload_requested = true;
		});
	}

	void ps5_launcher_dialog::start_reload()
	{
		m_loading = true;

		m_enumeration.initialize_paths();
		m_enumeration.set_localization(g_cfg.sys.language, "Unknown", [](const std::string&) { return std::string(); });
		m_enumeration.set_show_custom_icons(true);
		m_enumeration.set_prefer_game_data_icons(true);
		m_enumeration.set_play_hover_movies(false);
		m_enumeration.set_play_hover_music(false);
		m_enumeration.set_canceled_callback([]() { return thread_ctrl::state() == thread_state::aborting; });

		m_enumeration_thread = std::make_unique<named_thread<std::function<void()>>>("Launcher Reload", [this]()
		{
			m_enumeration.parse_directories();
			m_enumeration.add_vfs_entry();
			m_enumeration.remove_duplicates();

			for (const auto& entry : m_enumeration.path_entries())
			{
				m_enumeration.parse_entry(entry);
			}
			m_enumeration.apply_patches();

			std::vector<ps5_launcher_game> games;
			for (big_picture_game_info& info : m_enumeration.take_games())
			{
				if (!info.bootable)
				{
					continue;
				}

				ps5_launcher_game game;
				game.icon = info.load_icon();
				if (game.icon)
				{
					game.icon->dirty = true;
				}

				// PIC1.PNG sits beside ICON0.PNG in the game's PS3_GAME folder
				if (!info.icon_in_archive && !info.icon_path.empty())
				{
					game.background = load_image(fs::get_parent_dir(info.icon_path) + "/PIC1.PNG");
				}

				game.info = std::move(info);
				load_cover(game);
				games.push_back(std::move(game));
			}
			m_enumeration.clear(true);

			std::sort(games.begin(), games.end(), [](const ps5_launcher_game& a, const ps5_launcher_game& b)
			{
				return a.info.name < b.info.name;
			});

			launcher_log.notice("%u games found", games.size());

			std::lock_guard lock(m_mutex);
			if (thread_ctrl::state() == thread_state::aborting)
			{
				return;
			}
			// The art shown belongs to the list being replaced
			m_background_image = nullptr;
			m_background_fading = false;
			m_games = std::move(games);
			m_selected = 0;
			m_first_visible = 0;
			m_loading = false;
			layout_home();
		});
	}

	void ps5_launcher_dialog::select_game(s32 index)
	{
		if (m_games.empty())
		{
			return;
		}

		index = std::clamp(index, 0, static_cast<s32>(m_games.size()) - 1);
		if (index == m_selected)
		{
			return;
		}

		m_selected = index;
		m_flipped = false;
		m_flip_angle = 0.f;
		m_spin = 0.f;
		play_sound(sound_effect::cursor);
		layout_home();
	}

	void ps5_launcher_dialog::set_tab(tab next)
	{
		if (next == m_tab)
		{
			return;
		}

		m_tab = next;
		play_sound(sound_effect::cursor);
		layout_tabs();

		m_detail = false;
		m_detail_t = 0.f;
		if (m_tab == tab::library)
		{
			// The row starts where the selection is
			m_flow_pos = static_cast<f32>(m_selected);
		}
		else if (m_tab == tab::settings)
		{
			open_settings_tab();
		}
		else if (m_tab == tab::trophies)
		{
			open_trophies_tab();
		}
		else if (m_tab == tab::install)
		{
			open_install_tab();
		}
	}

	void ps5_launcher_dialog::boot_selected()
	{
		if (m_selected < 0 || static_cast<usz>(m_selected) >= m_games.size())
		{
			return;
		}

		const big_picture_game_info& info = m_games[m_selected].info;
		play_sound(sound_effect::accept);
		boot_game(info.path, info.serial);
	}


	void ps5_launcher_dialog::open_game_settings()
	{
		if (m_selected < 0 || static_cast<usz>(m_selected) >= m_games.size())
		{
			return;
		}

		const big_picture_game_info& info = m_games[m_selected].info;
		if (info.serial.empty())
		{
			return;
		}

		m_gs_serial = info.serial;
		m_gs_name = info.name.empty() ? info.serial : info.name;

		m_gs_rows = read_game_settings(m_gs_serial, true);

		m_gs_selected = 1;
		m_gs_scroll = 0;
		m_gs_open = true;
		m_gs_open_us = m_now_us;
		play_sound(sound_effect::accept);
		layout_game_settings();
		layout_hints();
	}

	void ps5_launcher_dialog::close_game_settings()
	{
		write_game_settings(m_gs_serial, m_gs_rows);

		m_gs_open = false;
		m_gs_items.clear();
		play_sound(sound_effect::cancel);
		layout_hints();
	}

	void ps5_launcher_dialog::handle_game_settings(pad_button button_press)
	{
		const bool up = button_press == pad_button::dpad_up || button_press == pad_button::ls_up;
		const bool down = button_press == pad_button::dpad_down || button_press == pad_button::ls_down;
		const bool left = button_press == pad_button::dpad_left || button_press == pad_button::ls_left;
		const bool right = button_press == pad_button::dpad_right || button_press == pad_button::ls_right || button_press == pad_button::cross;

		if (button_press == pad_button::circle || button_press == pad_button::triangle)
		{
			close_game_settings();
			return;
		}

		if (button_press == pad_button::L1 || button_press == pad_button::R1)
		{
			// The first setting of the section before or after this one
			const s32 count = static_cast<s32>(m_gs_rows.size());
			s32 heading = m_gs_selected;
			while (heading > 0 && !m_gs_rows[heading].heading) heading--;
			s32 next = heading;
			if (button_press == pad_button::L1)
			{
				do next--; while (next > 0 && !m_gs_rows[next].heading);
			}
			else
			{
				do next++; while (next < count && !m_gs_rows[next].heading);
			}
			if (next >= 0 && next + 1 < count && m_gs_rows[next].heading)
			{
				m_gs_selected = next + 1;
				play_sound(sound_effect::cursor);
				layout_game_settings();
			}
			return;
		}

		if (up || down)
		{
			// The next row that is a setting, not a heading
			s32 next = m_gs_selected;
			do
			{
				next += up ? -1 : 1;
			}
			while (next >= 0 && next < static_cast<s32>(m_gs_rows.size()) && m_gs_rows[next].heading);

			if (next >= 0 && next < static_cast<s32>(m_gs_rows.size()))
			{
				m_gs_selected = next;
				play_sound(sound_effect::cursor);
				layout_game_settings();
			}
			return;
		}

		if (m_gs_selected < 0 || static_cast<usz>(m_gs_selected) >= m_gs_rows.size())
		{
			return;
		}
		game_setting& row = m_gs_rows[m_gs_selected];

		if (left || right)
		{
			const auto it = std::find(row.options.begin(), row.options.end(), row.value);
			const s32 at = static_cast<s32>(it - row.options.begin());
			const s32 count = static_cast<s32>(row.options.size());
			row.value = row.options[(at + (left ? count - 1 : 1)) % count];
			play_sound(sound_effect::cursor);
			layout_game_settings();
		}
		else if (button_press == pad_button::square && row.value != row.global)
		{
			row.value = row.global;
			play_sound(sound_effect::cancel);
			layout_game_settings();
		}
	}

	void ps5_launcher_dialog::layout_game_settings()
	{
		m_gs_items.clear();
		const auto add = [this](std::unique_ptr<overlay_element> item) -> overlay_element&
		{
			m_gs_items.push_back(std::move(item));
			return *m_gs_items.back();
		};

		// A veil over the art, deeper on the left where the list is
		auto veil = std::make_unique<overlay_element>();
		veil->set_size(virtual_width, virtual_height);
		veil->back_color = color4f(c_backdrop.r, c_backdrop.g, c_backdrop.b, 0.55f);
		add(std::move(veil));

		// Heading
		auto kicker = make_label(spaced("GAME SETTINGS"), 10, f_semibold, c_text_dim);
		place(*kicker, c_hero_x, 104);
		add(std::move(kicker));

		auto title = make_label("", 24, f_bold, c_text);
		fit_text(*title, m_gs_name, 760);
		place(*title, c_hero_x, 136);
		add(std::move(title));

		auto note = make_label("Saved for this game only, and used the next time it starts. Marked settings differ from the global ones.", 11, f_regular, c_text_dim);
		place(*note, c_hero_x, 166);
		add(std::move(note));

		// The list: rows from y 196 to 646, scrolled to keep the selection in view
		constexpr s16 list_x = c_hero_x - 12;
		constexpr u16 list_w = 760;
		constexpr s16 list_top = 192;
		constexpr s16 list_bottom = 650;
		constexpr s16 row_h = 38;
		constexpr s16 heading_h = 34;

		std::vector<s16> tops;
		s16 y = 0;
		for (const game_setting& row : m_gs_rows)
		{
			tops.push_back(y);
			y = static_cast<s16>(y + (row.heading ? heading_h : row_h));
		}
		if (m_gs_selected >= 0 && static_cast<usz>(m_gs_selected) < tops.size())
		{
			const s16 top = tops[m_gs_selected];
			const s16 visible = list_bottom - list_top;
			// Keep the heading above the first setting in view
			const s16 want_top = m_gs_selected > 0 && m_gs_rows[m_gs_selected - 1].heading ? tops[m_gs_selected - 1] : top;
			if (want_top < m_gs_scroll) m_gs_scroll = want_top;
			if (top + row_h > m_gs_scroll + visible) m_gs_scroll = top + row_h - visible;
		}

		for (usz i = 0; i < m_gs_rows.size(); i++)
		{
			const game_setting& row = m_gs_rows[i];
			const s16 top = static_cast<s16>(list_top + tops[i] - m_gs_scroll);
			const s16 h = row.heading ? heading_h : row_h;
			if (top < list_top || top + h > list_bottom)
			{
				continue;
			}

			if (row.heading)
			{
				auto heading = make_label(spaced(row.label), 9, f_semibold, c_accent);
				place(*heading, static_cast<s16>(list_x + 12), top + 22.f);
				add(std::move(heading));
				continue;
			}

			const bool selected = static_cast<s32>(i) == m_gs_selected;
			const bool own = row.value != row.global;
			const f32 mid = top + h / 2.f;

			if (selected)
			{
				auto bar = std::make_unique<rounded_rect>();
				bar->set_pos(list_x, static_cast<s16>(top + 2));
				bar->set_size(list_w, static_cast<u16>(h - 4));
				bar->border_radius = 10;
				bar->back_color = color4f(1.f, 1.f, 1.f, 0.1f);
				bar->border_size = 2;
				bar->border_color = c_accent;
				add(std::move(bar));
			}
			else
			{
				auto rule = std::make_unique<overlay_element>();
				rule->set_pos(static_cast<s16>(list_x + 12), static_cast<s16>(top + h - 1));
				rule->set_size(static_cast<u16>(list_w - 24), 1);
				rule->back_color = color4f(1.f, 1.f, 1.f, 0.07f);
				add(std::move(rule));
			}

			if (own)
			{
				auto dot = std::make_unique<ellipse>();
				dot->set_size(6, 6);
				dot->set_pos(static_cast<s16>(list_x + 12), static_cast<s16>(mid - 3));
				dot->back_color = c_accent;
				add(std::move(dot));
			}

			auto name = make_label(row.label, 13, selected ? f_semibold : f_medium, selected || own ? c_text : c_text_dim);
			place(*name, static_cast<s16>(list_x + 26), mid);
			add(std::move(name));

			// The value, right-aligned, with arrows on the selected row
			auto value = make_label(setting_text(row.key, row.value), 13, own ? f_semibold : f_medium, own ? c_accent : (selected ? c_text : c_text_dim));
			const s16 value_right = static_cast<s16>(list_x + list_w - (selected ? 34 : 18));
			place(*value, static_cast<s16>(value_right - value->w), mid);
			const s16 value_x = value->x;
			add(std::move(value));

			if (selected)
			{
				auto less = make_label("‹", 15, f_semibold, c_text);
				place(*less, static_cast<s16>(value_x - 16), mid);
				add(std::move(less));
				auto more = make_label("›", 15, f_semibold, c_text);
				place(*more, static_cast<s16>(value_right + 10), mid);
				add(std::move(more));
			}
		}

		// The selected setting, explained, on the right
		if (m_gs_selected >= 0 && static_cast<usz>(m_gs_selected) < m_gs_rows.size() && !m_gs_rows[m_gs_selected].heading)
		{
			const game_setting& row = m_gs_rows[m_gs_selected];
			constexpr s16 panel_x = 852;
			constexpr u16 panel_w = 388;

			auto panel = std::make_unique<rounded_rect>();
			panel->set_pos(panel_x, list_top);
			panel->border_radius = 16;
			panel->back_color = color4f(0.03f, 0.04f, 0.09f, 0.72f);
			panel->border_size = 1;
			panel->border_color = c_glass_border;
			overlay_element& panel_ref = add(std::move(panel));

			auto name = make_label(row.label, 15, f_semibold, c_text);
			place(*name, static_cast<s16>(panel_x + 24), list_top + 34.f);
			add(std::move(name));

			auto help = make_label("", 11, f_regular, c_text_dim);
			help->set_wrap_text(true);
			help->set_text(row.help.empty() ? "RPCS3 has no description of this setting." : row.help);
			help->set_pos(static_cast<s16>(panel_x + 24), static_cast<s16>(list_top + 54));
			help->set_size(panel_w - 48, 300);
			help->auto_resize(false, panel_w - 48, 300);
			const s16 help_bottom = static_cast<s16>(help->y + help->h);
			add(std::move(help));

			auto rule = std::make_unique<overlay_element>();
			rule->set_pos(static_cast<s16>(panel_x + 24), static_cast<s16>(help_bottom + 18));
			rule->set_size(panel_w - 48, 1);
			rule->back_color = color4f(1.f, 1.f, 1.f, 0.12f);
			add(std::move(rule));

			const f32 line1 = help_bottom + 42.f;
			const f32 line2 = line1 + 26.f;
			for (const auto& [caption, text, f32_y, accent] : {std::tuple{"Global", setting_text(row.key, row.global), line1, false},
					std::tuple{"This game", setting_text(row.key, row.value), line2, row.value != row.global}})
			{
				auto left_label = make_label(caption, 12, f_medium, c_text_dim);
				place(*left_label, static_cast<s16>(panel_x + 24), f32_y);
				add(std::move(left_label));
				auto right_label = make_label(text, 12, f_semibold, accent ? c_accent : c_text);
				place(*right_label, static_cast<s16>(panel_x + panel_w - 24 - right_label->w), f32_y);
				add(std::move(right_label));
			}

			panel_ref.set_size(panel_w, static_cast<u16>(line2 + 26 - list_top));
			panel_ref.refresh();
		}
	}

	void ps5_launcher_dialog::open_patches()
	{
		if (m_selected < 0 || static_cast<usz>(m_selected) >= m_games.size())
		{
			return;
		}

		const big_picture_game_info& info = m_games[m_selected].info;
		if (info.serial.empty())
		{
			return;
		}

		m_pt_name = info.name.empty() ? info.serial : info.name;
		m_pt.reset();
		m_pt_rows.clear();
		m_pt_selected = 0;
		m_pt_scroll = 0;
		m_pt_open = true;
		m_pt_open_us = m_now_us;
		play_sound(sound_effect::accept);

		// RPCS3's patch.yml is a few megabytes of YAML: read on a thread, and
		// taken up by update(). A thread still reading for an earlier opening
		// finishes first (it never takes m_mutex)
		m_pt_thread.reset();
		{
			std::lock_guard lock(m_pt_result_mutex);
			m_pt_result.reset();
		}
		const u32 generation = ++m_pt_generation;
		m_pt_thread = std::make_unique<named_thread<std::function<void()>>>("Launcher Patches", [this, generation, serial = info.serial, version = info.app_ver]()
		{
			auto patches = std::make_unique<ps5_patches::game>(ps5_patches::read(serial, version));
			std::lock_guard lock(m_pt_result_mutex);
			m_pt_result = std::move(patches);
			m_pt_result_generation = generation;
		});

		layout_patches();
		layout_hints();
	}

	void ps5_launcher_dialog::close_patches()
	{
		if (m_pt && m_pt->changed)
		{
			std::vector<std::string> on;
			for (const ps5_patches::patch& p : m_pt->patches)
			{
				if (p.enabled)
				{
					on.push_back(p.description);
				}
			}
			ps5_patches::write(*m_pt);
			launcher_log.notice("Patches for %s saved: %s", m_pt->serial, on.empty() ? std::string("none on") : fmt::merge(on, "; "));
		}

		m_pt_open = false;
		m_pt.reset();
		m_pt_rows.clear();
		m_pt_items.clear();
		play_sound(sound_effect::cancel);
		layout_hints();
	}

	void ps5_launcher_dialog::build_patch_rows()
	{
		m_pt_rows.clear();
		m_pt_selected = 0;
		m_pt_scroll = 0;
		if (!m_pt)
		{
			return;
		}

		bool game_heading = false;
		bool every_heading = false;
		for (usz i = 0; i < m_pt->patches.size(); i++)
		{
			const ps5_patches::patch& p = m_pt->patches[i];
			if (!p.every_game && !std::exchange(game_heading, true))
			{
				m_pt_rows.push_back({"For this game"});
			}
			if (p.every_game && !std::exchange(every_heading, true))
			{
				m_pt_rows.push_back({"For every game"});
			}
			m_pt_rows.push_back({"", static_cast<s32>(i), -1});
			for (usz v = 0; v < p.values.size(); v++)
			{
				m_pt_rows.push_back({"", static_cast<s32>(i), static_cast<s32>(v)});
			}
		}

		while (static_cast<usz>(m_pt_selected) < m_pt_rows.size() && !m_pt_rows[m_pt_selected].heading.empty())
		{
			m_pt_selected++;
		}
	}

	void ps5_launcher_dialog::handle_patches(pad_button button_press)
	{
		const bool up = button_press == pad_button::dpad_up || button_press == pad_button::ls_up;
		const bool down = button_press == pad_button::dpad_down || button_press == pad_button::ls_down;
		const bool left = button_press == pad_button::dpad_left || button_press == pad_button::ls_left;
		const bool right = button_press == pad_button::dpad_right || button_press == pad_button::ls_right;

		if (button_press == pad_button::circle || button_press == pad_button::triangle)
		{
			close_patches();
			return;
		}

		const s32 count = static_cast<s32>(m_pt_rows.size());
		if (!m_pt || m_pt_selected < 0 || m_pt_selected >= count)
		{
			return;
		}

		if (button_press == pad_button::L1 || button_press == pad_button::R1)
		{
			// The first row of the section before or after this one
			s32 heading = m_pt_selected;
			while (heading > 0 && m_pt_rows[heading].heading.empty()) heading--;
			s32 next = heading;
			if (button_press == pad_button::L1)
			{
				do next--; while (next > 0 && m_pt_rows[next].heading.empty());
			}
			else
			{
				do next++; while (next < count && m_pt_rows[next].heading.empty());
			}
			if (next >= 0 && next + 1 < count && !m_pt_rows[next].heading.empty())
			{
				m_pt_selected = next + 1;
				play_sound(sound_effect::cursor);
				layout_patches();
				layout_hints();
			}
			return;
		}

		if (up || down)
		{
			s32 next = m_pt_selected;
			do
			{
				next += up ? -1 : 1;
			}
			while (next >= 0 && next < count && !m_pt_rows[next].heading.empty());

			if (next >= 0 && next < count)
			{
				m_pt_selected = next;
				play_sound(sound_effect::cursor);
				layout_patches();
				layout_hints();
			}
			return;
		}

		const patch_row& row = m_pt_rows[m_pt_selected];
		ps5_patches::patch& p = m_pt->patches[row.patch];
		bool changed = false;

		if (row.value < 0)
		{
			// The patch itself: on and off
			if (button_press == pad_button::cross || left || right)
			{
				const bool enable = button_press == pad_button::cross ? !p.enabled : right;
				if (enable != p.enabled)
				{
					ps5_patches::set_enabled(*m_pt, row.patch, enable);
					play_sound(enable ? sound_effect::accept : sound_effect::cancel);
					changed = true;
				}
			}
		}
		else if (left || right || button_press == pad_button::cross || button_press == pad_button::L2 || button_press == pad_button::R2)
		{
			// One of its values: a step, or ten with L2 / R2
			const s32 direction = (left || button_press == pad_button::L2) ? -1 : 1;
			const bool big = button_press == pad_button::L2 || button_press == pad_button::R2;
			const f64 before = p.values[row.value].current.value;
			ps5_patches::step_value(*m_pt, row.patch, row.value, direction, big);
			if (p.values[row.value].current.value != before)
			{
				play_sound(sound_effect::cursor);
				changed = true;
			}
		}
		else if (button_press == pad_button::square)
		{
			const f64 before = p.values[row.value].current.value;
			ps5_patches::reset_value(*m_pt, row.patch, row.value);
			if (p.values[row.value].current.value != before)
			{
				play_sound(sound_effect::cancel);
				changed = true;
			}
		}

		if (changed)
		{
			layout_patches();
			layout_hints();
		}
	}

	void ps5_launcher_dialog::layout_patches()
	{
		m_pt_items.clear();
		const auto add = [this](std::unique_ptr<overlay_element> item) -> overlay_element&
		{
			m_pt_items.push_back(std::move(item));
			return *m_pt_items.back();
		};
		const auto wrapped = [&](std::string_view text, u16 size, std::string_view font, const color4f& color, s16 x, s16 y, u16 w, u16 max_h) -> overlay_element&
		{
			auto words = make_label("", size, font, color);
			words->set_wrap_text(true);
			words->set_text(text);
			words->set_pos(x, y);
			words->set_size(w, max_h);
			words->auto_resize(false, w, max_h);
			return add(std::move(words));
		};

		auto veil = std::make_unique<overlay_element>();
		veil->set_size(virtual_width, virtual_height);
		veil->back_color = color4f(c_backdrop.r, c_backdrop.g, c_backdrop.b, 0.55f);
		add(std::move(veil));

		// Heading
		auto kicker = make_label(spaced("PATCHES"), 10, f_semibold, c_text_dim);
		place(*kicker, c_hero_x, 104);
		add(std::move(kicker));

		auto title = make_label("", 24, f_bold, c_text);
		fit_text(*title, m_pt_name, 760);
		place(*title, c_hero_x, 136);
		add(std::move(title));

		constexpr s16 list_x = c_hero_x - 12;
		constexpr u16 list_w = 760;
		constexpr s16 list_top = 192;
		constexpr s16 list_bottom = 650;
		constexpr s16 row_h = 38;
		constexpr s16 heading_h = 34;

		const ps5_patches::game* game = m_pt.get();
		std::string note;
		if (!game)
		{
			note = "Reading RPCS3's patch files...";
		}
		else if (!game->unpacked.empty())
		{
			note = game->unpacked + ".";
		}
		else
		{
			const std::string version = game->app_version.empty() || game->app_version == "Unknown" ? "" : " v" + game->app_version;
			note = fmt::format("%s%s. A patch turned on here is applied the next time the game starts.", game->serial, version);
		}
		auto note_label = make_label("", 11, f_regular, game && !game->unpacked.empty() ? c_accent : c_text_dim);
		fit_text(*note_label, note, 760);
		place(*note_label, c_hero_x, 166);
		add(std::move(note_label));

		// Nothing to list: why, and what to do
		if (!game || m_pt_rows.empty())
		{
			std::string heading;
			std::string body;
			if (!game)
			{
				heading = "Reading the patches...";
			}
			else if (game->files == 0)
			{
				heading = "No patch files yet";
				body = fmt::format("Put RPCS3's patch.yml in %s over FTP, then open this page again.\n\n"
					"From RPCS3 on a PC: Manage > Game Patches > Download latest patches, then copy patch.yml from its patches folder.\n\n"
					"Or open https://rpcs3.net/compatibility?patch&api=v1&v=%s in a browser, save the page, and put the saved file in the same folder: this page unpacks it into patch.yml.",
					ps5_patches::ftp_folder(), patch_engine_version);
			}
			else
			{
				const std::string version = game->app_version.empty() || game->app_version == "Unknown" ? "" : " version " + game->app_version;
				heading = "No patches for this game";
				body = fmt::format("The %u patches in %u patch file%s have none for %s%s.", game->total, game->files, game->files == 1 ? "" : "s", game->serial, version);
				if (!game->other_versions.empty())
				{
					body += fmt::format("\n\nThey have some for version %s of it. A patch is made for one version of a game's code: install the game's update to that version to use them.",
						fmt::merge(game->other_versions, ", "));
				}
				body += fmt::format("\n\nA newer patch.yml may have some: put it in %s over FTP.", ps5_patches::ftp_folder());
			}
			if (game && !game->broken.empty())
			{
				body += fmt::format("\n\nRPCS3 found errors in %s and skipped what it could not read.", fmt::merge(game->broken, ", "));
			}

			auto big = make_label(heading, 17, f_semibold, c_text);
			place(*big, c_hero_x, list_top + 28.f);
			add(std::move(big));
			if (!body.empty())
			{
				wrapped(body, 12, f_regular, c_text_dim, c_hero_x, list_top + 50, 700, 360);
			}
			return;
		}

		// The list, scrolled to keep the selection in view
		std::vector<s16> tops;
		s16 y = 0;
		for (const patch_row& row : m_pt_rows)
		{
			tops.push_back(y);
			y = static_cast<s16>(y + (row.heading.empty() ? row_h : heading_h));
		}
		if (m_pt_selected >= 0 && static_cast<usz>(m_pt_selected) < tops.size())
		{
			const s16 top = tops[m_pt_selected];
			const s16 visible = list_bottom - list_top;
			const s16 want_top = m_pt_selected > 0 && !m_pt_rows[m_pt_selected - 1].heading.empty() ? tops[m_pt_selected - 1] : top;
			if (want_top < m_pt_scroll) m_pt_scroll = want_top;
			if (top + row_h > m_pt_scroll + visible) m_pt_scroll = top + row_h - visible;
		}

		for (usz i = 0; i < m_pt_rows.size(); i++)
		{
			const patch_row& row = m_pt_rows[i];
			const s16 top = static_cast<s16>(list_top + tops[i] - m_pt_scroll);
			const s16 h = row.heading.empty() ? row_h : heading_h;
			if (top < list_top || top + h > list_bottom)
			{
				continue;
			}

			if (!row.heading.empty())
			{
				auto heading = make_label(spaced(row.heading), 9, f_semibold, c_accent);
				place(*heading, static_cast<s16>(list_x + 12), top + 22.f);
				add(std::move(heading));
				continue;
			}

			const ps5_patches::patch& p = game->patches[row.patch];
			const bool selected = static_cast<s32>(i) == m_pt_selected;
			const f32 mid = top + h / 2.f;

			if (selected)
			{
				auto bar = std::make_unique<rounded_rect>();
				bar->set_pos(list_x, static_cast<s16>(top + 2));
				bar->set_size(list_w, static_cast<u16>(h - 4));
				bar->border_radius = 10;
				bar->back_color = color4f(1.f, 1.f, 1.f, 0.1f);
				bar->border_size = 2;
				bar->border_color = c_accent;
				add(std::move(bar));
			}
			else
			{
				auto rule = std::make_unique<overlay_element>();
				rule->set_pos(static_cast<s16>(list_x + 12), static_cast<s16>(top + h - 1));
				rule->set_size(static_cast<u16>(list_w - 24), 1);
				rule->back_color = color4f(1.f, 1.f, 1.f, 0.07f);
				add(std::move(rule));
			}

			if (row.value < 0)
			{
				// The patch's name, and its switch on the right
				auto name = make_label("", 13, selected || p.enabled ? f_semibold : f_medium, selected || p.enabled ? c_text : c_text_dim);
				fit_text(*name, p.description, list_w - 110);
				place(*name, static_cast<s16>(list_x + 18), mid);
				add(std::move(name));

				constexpr u16 switch_w = 40;
				constexpr u16 switch_h = 22;
				const s16 switch_x = static_cast<s16>(list_x + list_w - 18 - switch_w);
				auto track = std::make_unique<rounded_rect>();
				track->set_pos(switch_x, static_cast<s16>(mid - switch_h / 2.f));
				track->set_size(switch_w, switch_h);
				track->border_radius = switch_h / 2;
				track->back_color = p.enabled ? c_accent : color4f(1.f, 1.f, 1.f, 0.16f);
				add(std::move(track));
				auto knob = std::make_unique<ellipse>();
				knob->set_size(16, 16);
				knob->set_pos(static_cast<s16>(p.enabled ? switch_x + switch_w - 19 : switch_x + 3), static_cast<s16>(mid - 8));
				knob->back_color = p.enabled ? c_button_text : color4f(1.f, 1.f, 1.f, 0.85f);
				add(std::move(knob));

				auto state = make_label(p.enabled ? "On" : "Off", 11, f_semibold, p.enabled ? c_accent : c_text_dim);
				place(*state, static_cast<s16>(switch_x - 10 - state->w), mid);
				add(std::move(state));
				continue;
			}

			// One of its values, under it
			const ps5_patches::value& v = p.values[row.value];
			const bool own = v.current.value != v.default_value;
			const color4f dim = p.enabled ? c_text_dim : color4f(1.f, 1.f, 1.f, 0.4f);

			auto branch = make_label("→", 11, f_medium, dim);
			place(*branch, static_cast<s16>(list_x + 30), mid);
			add(std::move(branch));

			auto name = make_label("", 12, selected ? f_semibold : f_medium, selected ? c_text : dim);
			fit_text(*name, v.key, list_w - 300);
			place(*name, static_cast<s16>(list_x + 50), mid);
			add(std::move(name));

			auto value = make_label(ps5_patches::value_text(v), 12, own ? f_semibold : f_medium, own && p.enabled ? c_accent : (selected ? c_text : dim));
			const s16 value_right = static_cast<s16>(list_x + list_w - (selected ? 34 : 18));
			place(*value, static_cast<s16>(value_right - value->w), mid);
			const s16 value_x = value->x;
			add(std::move(value));

			if (selected)
			{
				auto less = make_label("‹", 15, f_semibold, c_text);
				place(*less, static_cast<s16>(value_x - 16), mid);
				add(std::move(less));
				auto more = make_label("›", 15, f_semibold, c_text);
				place(*more, static_cast<s16>(value_right + 10), mid);
				add(std::move(more));
			}
		}

		// The selected patch, or value, explained on the right
		if (m_pt_selected < 0 || static_cast<usz>(m_pt_selected) >= m_pt_rows.size() || !m_pt_rows[m_pt_selected].heading.empty())
		{
			return;
		}
		const patch_row& row = m_pt_rows[m_pt_selected];
		const ps5_patches::patch& p = game->patches[row.patch];
		constexpr s16 panel_x = 852;
		constexpr u16 panel_w = 388;
		constexpr s16 inner_x = panel_x + 24;
		constexpr u16 inner_w = panel_w - 48;

		auto panel = std::make_unique<rounded_rect>();
		panel->set_pos(panel_x, list_top);
		panel->border_radius = 16;
		panel->back_color = color4f(0.03f, 0.04f, 0.09f, 0.72f);
		panel->border_size = 1;
		panel->border_color = c_glass_border;
		overlay_element& panel_ref = add(std::move(panel));

		std::vector<std::pair<std::string, std::string>> lines;
		std::string name;
		std::string help;
		if (row.value < 0)
		{
			name = p.description;
			help = p.notes.empty() ? "Its author left no notes." : p.notes;
			lines.emplace_back("Turned", p.enabled ? "On" : "Off");
			if (!p.author.empty()) lines.emplace_back("By", p.author);
			if (!p.patch_version.empty()) lines.emplace_back("Patch version", p.patch_version);
			lines.emplace_back("For", p.every_game ? "Every game" : p.every_version ? "Every version" : "Version " + game->app_version);
			if (!p.group.empty()) lines.emplace_back("Group", p.group);
			if (!p.values.empty()) lines.emplace_back("Values", fmt::format("%u, under it", p.values.size()));
		}
		else
		{
			const ps5_patches::value& v = p.values[row.value];
			name = v.key;
			const bool choices = v.current.type == patch_configurable_type::double_enum || v.current.type == patch_configurable_type::long_enum;
			help = choices ? fmt::format("One of %u choices, for the patch \"%s\".", v.current.allowed_values.size(), p.description)
				: fmt::format("From %s to %s, for the patch \"%s\". L2 and R2 change it ten steps at a time.",
					ps5_patches::value_text(v, v.current.min), ps5_patches::value_text(v, v.current.max), p.description);
			if (!p.enabled)
			{
				help += " It is used once the patch is turned on.";
			}
			lines.emplace_back("Default", ps5_patches::value_text(v, v.default_value));
			lines.emplace_back("Set to", ps5_patches::value_text(v));
		}

		overlay_element& title_ref = wrapped(name, 15, f_semibold, c_text, inner_x, list_top + 22, inner_w, 60);
		const s16 help_top = static_cast<s16>(title_ref.y + title_ref.h + 8);
		overlay_element& help_ref = wrapped(help, 11, f_regular, c_text_dim, inner_x, help_top, inner_w, 220);
		const s16 help_bottom = static_cast<s16>(help_ref.y + help_ref.h);

		auto rule = std::make_unique<overlay_element>();
		rule->set_pos(inner_x, static_cast<s16>(help_bottom + 16));
		rule->set_size(inner_w, 1);
		rule->back_color = color4f(1.f, 1.f, 1.f, 0.12f);
		add(std::move(rule));

		f32 line_y = help_bottom + 38.f;
		for (const auto& [caption, text] : lines)
		{
			auto left_label = make_label(caption, 12, f_medium, c_text_dim);
			place(*left_label, inner_x, line_y);
			const s16 caption_w = left_label->w;
			add(std::move(left_label));
			const bool accent = caption == "Turned" ? p.enabled : caption == "Set to";
			auto right_label = make_label("", 12, f_semibold, accent ? c_accent : c_text);
			fit_text(*right_label, text, static_cast<u16>(inner_w - caption_w - 16));
			place(*right_label, static_cast<s16>(inner_x + inner_w - right_label->w), line_y);
			add(std::move(right_label));
			line_y += 26.f;
		}

		panel_ref.set_size(panel_w, static_cast<u16>(std::min<f32>(line_y + 2 - list_top, list_bottom - list_top)));
		panel_ref.refresh();
	}

	bool ps5_launcher_dialog::installing() const
	{
		return m_installer && !m_installer->done();
	}

	void ps5_launcher_dialog::open_install_tab()
	{
		m_in_files = ps5_packages::scan();
		m_in_row = std::clamp(m_in_row, 0, static_cast<s32>(m_in_files.size()) + 2);
		m_in_scroll = 0;
		launcher_log.notice("Install: %u files in %s", m_in_files.size(), ps5_packages::folder());
		layout_hints();
	}

	void ps5_launcher_dialog::handle_install(pad_button button_press)
	{
		const bool up = button_press == pad_button::dpad_up || button_press == pad_button::ls_up;
		const bool down = button_press == pad_button::dpad_down || button_press == pad_button::ls_down;
		const bool left = button_press == pad_button::dpad_left || button_press == pad_button::ls_left;
		const bool right = button_press == pad_button::dpad_right || button_press == pad_button::ls_right;

		// An installation: its files can be looked through; it stops, or once
		// it has ended goes away
		if (m_installer)
		{
			const s32 count = static_cast<s32>(m_installer->files().size());
			if ((up && m_in_job_row > 0) || (down && m_in_job_row + 1 < count))
			{
				m_in_job_row += up ? -1 : 1;
				play_sound(sound_effect::cursor);
			}
			else if (!m_installer->done())
			{
				if (button_press == pad_button::circle && !m_installer->cancelled())
				{
					launcher_log.notice("Install: stopped by the player");
					m_installer->cancel();
					play_sound(sound_effect::cancel);
					layout_hints();
				}
			}
			else if (button_press == pad_button::cross || button_press == pad_button::circle)
			{
				m_installer.reset();
				play_sound(sound_effect::accept);
				open_install_tab();
			}
			return;
		}

		const s32 count = static_cast<s32>(m_in_files.size()) + 3;
		if (up || down)
		{
			const s32 next = m_in_row + (up ? -1 : 1);
			if (next >= 0 && next < count)
			{
				m_in_row = next;
				play_sound(sound_effect::cursor);
				layout_hints();
			}
			return;
		}
		if (button_press == pad_button::circle)
		{
			set_tab(tab::home);
			return;
		}

		const bool act = button_press == pad_button::cross;
		if (m_in_row == 0 && act)
		{
			std::vector<ps5_packages::file> chosen;
			for (const ps5_packages::file& f : m_in_files)
			{
				if (f.chosen)
				{
					chosen.push_back(f);
				}
			}
			if (chosen.empty())
			{
				return;
			}
			launcher_log.notice("Install: %u files, %s", chosen.size(), m_in_delete ? "each deleted once installed" : "kept");
			m_installer = std::make_unique<ps5_packages::installer>(std::move(chosen), m_in_delete);
			m_in_reported = false;
			m_in_job_row = 0;
			play_sound(sound_effect::accept);
			layout_hints();
		}
		else if (m_in_row == 1 && (act || left || right))
		{
			m_in_delete = !m_in_delete;
			play_sound(sound_effect::cursor);
		}
		else if (m_in_row == 2 && act)
		{
			play_sound(sound_effect::accept);
			open_install_tab();
		}
		else if (m_in_row >= 3 && m_in_row < count && (act || left || right))
		{
			ps5_packages::file& f = m_in_files[m_in_row - 3];
			const bool choose = act ? !f.chosen : right;
			if (f.type != ps5_packages::kind::unreadable && choose != f.chosen)
			{
				f.chosen = choose;
				play_sound(sound_effect::cursor);
				layout_hints();
			}
		}
	}

	void ps5_launcher_dialog::compile_install(compiled_resource& result)
	{
		// The Settings tab's deep blue, glow and cards
		{
			overlay_element base;
			base.set_size(virtual_width, virtual_height);
			base.back_color = c_library_blue;
			result.add(base.get_compiled());
		}
		const auto glow = [&](s16 cx, s16 cy, u16 w, u16 h, const color4f& color)
		{
			image_view view;
			view.set_raw_image(m_glow_image.get());
			view.back_color.a = 0.f;
			view.fore_color = color;
			view.set_pos(static_cast<s16>(cx - w / 2), static_cast<s16>(cy - h / 2));
			view.set_size(w, h);
			result.add(view.get_compiled());
		};
		glow(640, 340, 1600, 1000, color4f(0.1f, 0.2f, 0.65f, 0.34f));
		glow(620, 360, 900, 700, color4f(c_accent.r, c_accent.g, c_accent.b, 0.06f));

		const auto card = [&](s16 x, s16 y, u16 w, u16 h)
		{
			rounded_rect panel;
			panel.set_pos(x, y);
			panel.set_size(w, h);
			panel.border_radius = 24;
			panel.back_color = color4f(0.02f, 0.04f, 0.16f, 0.78f);
			result.add(panel.get_compiled());
			rounded_rect edge;
			edge.set_pos(static_cast<s16>(x + 24), y);
			edge.set_size(static_cast<u16>(w - 48), 3);
			edge.border_radius = 2;
			edge.back_color = c_accent;
			result.add(edge.get_compiled());
		};
		const auto text = [&](std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 x, f32 mid, u16 max_w = 0) -> std::unique_ptr<label>
		{
			auto l = make_label("", size, font_name, colour);
			if (max_w)
			{
				fit_text(*l, s, max_w);
			}
			else
			{
				l->set_text(s);
				l->auto_resize();
			}
			place(*l, x, mid);
			result.add(l->get_compiled());
			return l;
		};
		const auto wrapped = [&](std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 x, s16 y, u16 w, u16 max_h) -> s16
		{
			label words;
			words.set_font(size, font_name);
			words.fore_color = colour;
			words.back_color.a = 0.f;
			words.set_padding(0);
			words.set_wrap_text(true);
			words.set_text(s);
			words.set_pos(x, y);
			words.set_size(w, max_h);
			words.auto_resize(false, w, max_h);
			result.add(words.get_compiled());
			return static_cast<s16>(words.y + words.h);
		};
		const auto bar = [&](s16 x, s16 y, u16 w, u16 h, f32 fraction)
		{
			rounded_rect track;
			track.set_pos(x, y);
			track.set_size(w, h);
			track.border_radius = h / 2;
			track.back_color = color4f(1.f, 1.f, 1.f, 0.12f);
			result.add(track.get_compiled());
			if (const u16 filled = static_cast<u16>(std::lround(w * std::clamp(fraction, 0.f, 1.f))); filled >= h)
			{
				rounded_rect fill;
				fill.set_pos(x, y);
				fill.set_size(filled, h);
				fill.border_radius = h / 2;
				fill.back_color = c_accent;
				result.add(fill.get_compiled());
			}
		};
		const color4f c_fail(1.f, 0.48f, 0.42f, 1.f);

		constexpr s16 top = 96;
		constexpr u16 height = 552;

		// What goes where, on the left
		{
			card(40, top, 250, height);
			text(spaced("INSTALL"), 9, f_semibold, c_accent, 66, top + 38.f);
			text("Packages", 22, f_bold, c_text, 64, top + 68.f);
			s16 y = wrapped("PS3 packages (.pkg) and licences (.rap, .edat), put over FTP in", 11, f_regular, c_text_dim, 64, top + 90, 204, 60);
			const std::string where = ps5_packages::ftp_folder();
			const usz split = where.find("/rpcs3/");
			text(where.substr(0, split + 1), 10, f_semibold, c_text, 64, y + 14.f);
			text(split == umax ? "" : where.substr(split + 1), 10, f_semibold, c_text, 64, y + 30.f);
			y = wrapped("A game installs into the Library. An update or an add-on goes into its game's folder: install a game's updates oldest first, each on the one before it.", 11, f_regular, c_text_dim, 64, static_cast<s16>(y + 50), 204, 120);
			wrapped("The console's storage writes slowly after about a gigabyte, so a big game takes a long time here: RPCS3 on a PC installs it faster, and its dev_hdd0/game folder can be copied over.", 10, f_regular, c_text_dim, 64, static_cast<s16>(y + 14), 204, 140);
		}

		// The list, in the middle
		constexpr s16 list_x = 310;
		constexpr u16 list_w = 600;
		card(list_x, top, list_w, height);
		constexpr s16 row_left = list_x + 22;
		constexpr u16 row_w = list_w - 44;
		constexpr f32 row_h = 44.f;

		// A file's two lines: its title, and what it is
		const auto file_lines = [&](const ps5_packages::file& f, f32 mid, bool selected, bool lit, u16 room)
		{
			const color4f ink = lit ? c_button_text : (selected ? c_text : c_text_dim);
			text(f.title, 13, selected ? f_semibold : f_medium, f.type == ps5_packages::kind::unreadable && !lit ? color4f(1.f, 1.f, 1.f, 0.45f) : ink, row_left + 16, mid - 8.f, room);
			std::vector<std::string> parts;
			if (!f.title_id.empty()) parts.push_back(f.title_id);
			parts.push_back(ps5_packages::kind_text(f));
			parts.push_back(ps5_packages::size_text(f.size));
			text(fmt::merge(parts, "  ·  "), 10, f_medium, lit ? color4f(c_button_text.r, c_button_text.g, c_button_text.b, 0.7f) : color4f(1.f, 1.f, 1.f, 0.5f), row_left + 16, mid + 10.f, room);
		};

		const ps5_packages::file* shown = nullptr; // the selected file, for the right
		std::string shown_message;
		std::string action_help;

		if (m_installer)
		{
			// An installation: everything's progress, then each file's
			const auto& files = m_installer->files();
			const s32 count = static_cast<s32>(files.size());
			usz finished = 0;
			for (usz i = 0; i < files.size(); i++)
			{
				const auto state = m_installer->state_of(i);
				finished += state != ps5_packages::installer::state::waiting && state != ps5_packages::installer::state::installing;
			}
			const bool done = m_installer->done();
			const f32 overall = m_installer->progress();
			text(done ? (m_installer->cancelled() ? "Stopped" : "Finished") : (m_installer->cancelled() ? "Stopping..." : "Installing"), 22, f_bold, c_text, list_x + 32, top + 44.f);
			text(done ? fmt::format("%u of %u files done. The Library has been read again.", finished, count) : fmt::format("%u of %u files done  ·  %d%%", finished, count, static_cast<int>(overall * 100.f)), 11, f_regular, c_text_dim, list_x + 32, top + 72.f);
			bar(list_x + 32, top + 86, list_w - 64, 6, overall);

			constexpr s32 visible = 9;
			if (m_in_job_row < m_in_scroll) m_in_scroll = m_in_job_row;
			if (m_in_job_row >= m_in_scroll + visible) m_in_scroll = m_in_job_row - visible + 1;
			for (s32 r = m_in_scroll; r < count && r < m_in_scroll + visible; r++)
			{
				const ps5_packages::file& f = files[r];
				const f32 y = top + 104.f + (r - m_in_scroll) * row_h;
				const f32 mid = y + row_h / 2.f;
				const bool selected = r == m_in_job_row;
				if (selected)
				{
					rounded_rect pill;
					pill.set_pos(row_left, static_cast<s16>(y + 2));
					pill.set_size(row_w, static_cast<u16>(row_h - 4));
					pill.border_radius = 12;
					pill.back_color = color4f(1.f, 1.f, 1.f, 0.1f);
					pill.border_size = 2;
					pill.border_color = c_accent;
					result.add(pill.get_compiled());
				}
				file_lines(f, mid, selected, false, row_w - 190);

				const auto state = m_installer->state_of(r);
				const s16 right = static_cast<s16>(row_left + row_w - 16);
				switch (state)
				{
				case ps5_packages::installer::state::waiting:
				{
					auto l = make_label(m_installer->cancelled() ? "Not installed" : "Waiting", 11, f_medium, c_text_dim);
					place(*l, static_cast<s16>(right - l->w), mid);
					result.add(l->get_compiled());
					break;
				}
				case ps5_packages::installer::state::installing:
				{
					const f32 fraction = m_installer->progress_of(r);
					auto l = make_label(fmt::format("%d%%", static_cast<int>(fraction * 100.f)), 11, f_semibold, c_accent);
					place(*l, static_cast<s16>(right - l->w), mid);
					result.add(l->get_compiled());
					bar(static_cast<s16>(right - 150), static_cast<s16>(mid - 3), 100, 6, fraction);
					break;
				}
				case ps5_packages::installer::state::installed:
				case ps5_packages::installer::state::failed:
				case ps5_packages::installer::state::cancelled:
				{
					const bool ok = state == ps5_packages::installer::state::installed;
					const bool failed = state == ps5_packages::installer::state::failed;
					auto l = make_label(ok ? "Installed" : failed ? "Failed" : "Not installed", 11, f_semibold, ok ? c_accent : failed ? c_fail : c_text_dim);
					place(*l, static_cast<s16>(right - l->w), mid);
					result.add(l->get_compiled());
					break;
				}
				}
			}

			if (m_in_job_row >= 0 && m_in_job_row < count)
			{
				shown = &files[m_in_job_row];
				shown_message = m_installer->message_of(m_in_job_row);
			}
		}
		else
		{
			// The folder: three actions, then its files
			usz chosen = 0, licences = 0;
			u64 bytes = 0;
			for (const ps5_packages::file& f : m_in_files)
			{
				chosen += f.chosen;
				licences += f.type == ps5_packages::kind::licence;
				bytes += f.size;
			}
			const usz packages = m_in_files.size() - licences;
			text("In the folder", 22, f_bold, c_text, list_x + 32, top + 44.f);
			text(m_in_files.empty() ? std::string("Nothing yet.") : fmt::format("%u package%s, %u licence%s  ·  %s", packages, packages == 1 ? "" : "s", licences, licences == 1 ? "" : "s", ps5_packages::size_text(bytes)),
				11, f_regular, c_text_dim, list_x + 32, top + 72.f);

			const s32 count = static_cast<s32>(m_in_files.size()) + 3;
			constexpr s32 visible = 10;
			if (m_in_row < m_in_scroll) m_in_scroll = m_in_row;
			if (m_in_row >= m_in_scroll + visible) m_in_scroll = m_in_row - visible + 1;
			for (s32 r = m_in_scroll; r < count && r < m_in_scroll + visible; r++)
			{
				const f32 y = top + 96.f + (r - m_in_scroll) * row_h;
				const f32 mid = y + row_h / 2.f;
				const bool selected = r == m_in_row;
				const bool action = r < 3;
				if (selected)
				{
					rounded_rect pill;
					pill.set_pos(row_left, static_cast<s16>(y + 2));
					pill.set_size(row_w, static_cast<u16>(row_h - 4));
					pill.border_radius = action ? static_cast<u16>((row_h - 4) / 2) : 12;
					pill.back_color = action ? c_accent : color4f(1.f, 1.f, 1.f, 0.1f);
					pill.border_size = action ? 0 : 2;
					pill.border_color = c_accent;
					result.add(pill.get_compiled());
				}
				if (r == 3)
				{
					overlay_element rule;
					rule.set_pos(row_left + 16, static_cast<s16>(y - 1));
					rule.set_size(row_w - 32, 1);
					rule.back_color = color4f(1.f, 1.f, 1.f, 0.14f);
					result.add(rule.get_compiled());
				}

				if (action)
				{
					const color4f ink = selected ? c_button_text : c_text;
					const char* const names[] = {"Install the chosen files", "Delete each file once installed", "Look in the folder again"};
					text(names[r], 14, selected ? f_bold : f_medium, ink, row_left + 16, mid);
					std::string value;
					if (r == 0) value = chosen ? fmt::format("%u chosen", chosen) : "Nothing chosen";
					if (r == 1) value = m_in_delete ? "On" : "Off";
					if (!value.empty())
					{
						auto l = make_label(value, 13, f_semibold, selected ? c_button_text : (r == 1 && m_in_delete ? c_accent : c_text_dim));
						place(*l, static_cast<s16>(row_left + row_w - 18 - l->w), mid);
						result.add(l->get_compiled());
					}
					continue;
				}

				const ps5_packages::file& f = m_in_files[r - 3];
				file_lines(f, mid, selected, false, row_w - 110);
				if (f.type == ps5_packages::kind::unreadable)
				{
					continue;
				}

				// Chosen or not: the Patches page's switch
				constexpr u16 switch_w = 40;
				constexpr u16 switch_h = 22;
				const s16 switch_x = static_cast<s16>(row_left + row_w - 16 - switch_w);
				rounded_rect track;
				track.set_pos(switch_x, static_cast<s16>(mid - switch_h / 2.f));
				track.set_size(switch_w, switch_h);
				track.border_radius = switch_h / 2;
				track.back_color = f.chosen ? c_accent : color4f(1.f, 1.f, 1.f, 0.16f);
				result.add(track.get_compiled());
				ellipse knob;
				knob.set_size(16, 16);
				knob.set_pos(static_cast<s16>(f.chosen ? switch_x + switch_w - 19 : switch_x + 3), static_cast<s16>(mid - 8));
				knob.back_color = f.chosen ? c_button_text : color4f(1.f, 1.f, 1.f, 0.85f);
				result.add(knob.get_compiled());
			}
			if (m_in_files.empty())
			{
				wrapped("No packages here yet. Put .pkg files in the folder over FTP, then choose Look in the folder again.", 12, f_regular, c_text_dim, row_left + 16, static_cast<s16>(top + 96 + 3 * row_h + 18), row_w - 32, 80);
			}

			if (m_in_row >= 3 && m_in_row - 3 < static_cast<s32>(m_in_files.size()))
			{
				shown = &m_in_files[m_in_row - 3];
			}
			else if (m_in_row == 0)
			{
				action_help = "Installs the chosen files in the order listed: licences, then games, then each game's updates and add-ons, oldest first. A file that fails does not stop the ones after it.";
			}
			else if (m_in_row == 1)
			{
				action_help = "Deletes each file from the packages folder once it has installed, to give its space back. A file that fails to install is kept.";
			}
			else
			{
				action_help = "Reads the packages folder again, after files were added or removed over FTP.";
			}
		}

		// The selected file, or action, on the right
		constexpr s16 panel_x = 930;
		constexpr u16 panel_w = 310;
		constexpr s16 inner_x = panel_x + 24;
		constexpr u16 inner_w = panel_w - 48;
		card(panel_x, top, panel_w, height);
		if (!shown)
		{
			text(spaced("ABOUT"), 9, f_semibold, c_accent, inner_x, top + 38.f);
			wrapped(action_help, 12, f_regular, c_text_dim, inner_x, top + 60, inner_w, 300);
			return;
		}

		text(spaced(ps5_packages::kind_text(*shown)), 9, f_semibold, shown->type == ps5_packages::kind::unreadable ? c_fail : c_accent, inner_x, top + 38.f, inner_w);
		s16 y = wrapped(shown->title, 15, f_semibold, c_text, inner_x, top + 54, inner_w, 64);

		std::vector<std::pair<std::string, std::string>> lines;
		if (!shown->title_id.empty()) lines.emplace_back("ID", shown->title_id);
		if (!shown->version.empty()) lines.emplace_back("Version", shown->version);
		if (!shown->target_version.empty()) lines.emplace_back("Goes on version", shown->target_version);
		if (shown->type == ps5_packages::kind::update || shown->type == ps5_packages::kind::addon)
		{
			lines.emplace_back("Installed now", shown->installed_version.empty() ? "No update yet" : "Version " + shown->installed_version);
		}
		lines.emplace_back("Size", ps5_packages::size_text(shown->size));
		lines.emplace_back("File", shown->name);

		f32 line_y = y + 30.f;
		for (const auto& [caption, value] : lines)
		{
			auto left_label = text(caption, 11, f_medium, c_text_dim, inner_x, line_y);
			auto right_label = make_label("", 11, f_semibold, c_text);
			fit_text(*right_label, value, static_cast<u16>(inner_w - left_label->w - 14));
			place(*right_label, static_cast<s16>(inner_x + inner_w - right_label->w), line_y);
			result.add(right_label->get_compiled());
			line_y += 24.f;
		}

		std::string note = shown_message;
		if (note.empty() && !m_installer)
		{
			if (shown->type == ps5_packages::kind::unreadable)
			{
				note = "RPCS3 could not read this file as a PS3 package. It may be cut short (copy it again), or be another console's.";
			}
			else if (shown->type == ps5_packages::kind::update && !shown->target_version.empty() && shown->installed_version != shown->target_version)
			{
				note = fmt::format("This update goes on version %s. Install the updates up to that version first: chosen together, they install in order.", shown->target_version);
			}
		}
		if (!note.empty())
		{
			overlay_element rule;
			rule.set_pos(inner_x, static_cast<s16>(line_y - 4));
			rule.set_size(inner_w, 1);
			rule.back_color = color4f(1.f, 1.f, 1.f, 0.12f);
			result.add(rule.get_compiled());
			wrapped(note, 11, f_regular, c_text_dim, inner_x, static_cast<s16>(line_y + 8), inner_w, static_cast<u16>(std::max(40.f, top + height - 20.f - (line_y + 8))));
		}
	}

	void ps5_launcher_dialog::open_settings_tab()
	{
		m_set_cfg = load_global_config();
		m_set_focus_list = false;
		load_settings_category();
	}

	void ps5_launcher_dialog::load_settings_category()
	{
		m_set_rows.clear();
		m_set_row = 0;
		m_set_scroll = 0;
		if (!m_set_cfg)
		{
			return;
		}

		const std::string_view tab = c_settings_tabs[m_set_category].name;
		for (const settings_entry& entry : settings_table())
		{
			if (entry.tab != tab)
			{
				continue;
			}
			const auto [section, key] = setting_path(entry.type);
			cfg::_base* setting = find_setting(*m_set_cfg, section, key);
			if (!setting)
			{
				launcher_log.error("Settings: no setting %s/%s", section, key);
				continue;
			}
			game_setting row;
			row.section = section;
			row.key = key;
			row.label = *entry.label ? entry.label : key;
			row.help = entry.help;
			row.options = setting_options(*setting, key);
			row.value = setting->to_string();
			if (std::find(row.options.begin(), row.options.end(), row.value) == row.options.end())
			{
				row.options.insert(row.options.begin(), row.value);
			}
			row.global = usable_default(*setting, row.options); // here: the default
			m_set_rows.push_back(std::move(row));
		}
	}

	void ps5_launcher_dialog::save_setting(const game_setting& row)
	{
		cfg::_base* setting = m_set_cfg ? find_setting(*m_set_cfg, row.section, row.key) : nullptr;
		if (!setting || !setting->from_string(row.value))
		{
			launcher_log.error("Settings: could not set %s/%s to %s", row.section, row.key, row.value);
			return;
		}
		// config.yml, written whole as the emulator writes it; the next game
		// boots with it (and the running shell takes it, where it can)
		Emulator::SaveSettings(m_set_cfg->to_string(), "");
		launcher_log.notice("Settings: %s/%s = %s", row.section, row.key, row.value);
	}

	void ps5_launcher_dialog::handle_settings(pad_button button_press)
	{
		const bool up = button_press == pad_button::dpad_up || button_press == pad_button::ls_up;
		const bool down = button_press == pad_button::dpad_down || button_press == pad_button::ls_down;
		const bool left = button_press == pad_button::dpad_left || button_press == pad_button::ls_left;
		const bool right = button_press == pad_button::dpad_right || button_press == pad_button::ls_right;
		const s32 categories = static_cast<s32>(std::size(c_settings_tabs));

		if (!m_set_focus_list)
		{
			// The categories
			if ((up && m_set_category > 0) || (down && m_set_category + 1 < categories))
			{
				m_set_category += up ? -1 : 1;
				play_sound(sound_effect::cursor);
				load_settings_category();
			}
			else if ((button_press == pad_button::cross || right) && !m_set_rows.empty())
			{
				m_set_focus_list = true;
				play_sound(sound_effect::accept);
				layout_hints();
			}
			else if (button_press == pad_button::circle)
			{
				set_tab(tab::home);
			}
			return;
		}

		// The settings of the category
		if (button_press == pad_button::circle)
		{
			m_set_focus_list = false;
			play_sound(sound_effect::cancel);
			layout_hints();
			return;
		}
		if (up || down)
		{
			const s32 next = m_set_row + (up ? -1 : 1);
			if (next >= 0 && next < static_cast<s32>(m_set_rows.size()))
			{
				m_set_row = next;
				play_sound(sound_effect::cursor);
			}
			return;
		}
		if (m_set_row < 0 || static_cast<usz>(m_set_row) >= m_set_rows.size())
		{
			return;
		}
		game_setting& row = m_set_rows[m_set_row];
		if (left || right || button_press == pad_button::cross)
		{
			const auto it = std::find(row.options.begin(), row.options.end(), row.value);
			const s32 at = static_cast<s32>(it - row.options.begin());
			const s32 count = static_cast<s32>(row.options.size());
			row.value = row.options[(at + (left ? count - 1 : 1)) % count];
			play_sound(sound_effect::cursor);
			save_setting(row);
		}
		else if (button_press == pad_button::square && row.value != row.global)
		{
			row.value = row.global;
			play_sound(sound_effect::cancel);
			save_setting(row);
		}
	}

	// The intro's white screen: eight shards that fly in turning and lock into
	// a diamond around the mark, a flash and a gold rim as they do, and the
	// maker's name under it
	void ps5_launcher_dialog::compile_intro_white(compiled_resource& result, f32 intro)
	{
		const f32 out = 1.f - ease_in_out(progress(intro, c_intro_white_out, 0.5f));
		const f32 locked = intro >= c_intro_lock ? 1.f : 0.f;

		overlay_element paper;
		paper.set_size(virtual_width, virtual_height);
		paper.back_color = color4f(1.f, 1.f, 1.f, 1.f);
		add_animated(result, paper.get_compiled(), ease_out(progress(intro, 0.f, c_intro_white_in)) * out);

		const auto shape = [&](std::initializer_list<std::pair<f32, f32>> points, const color4f& color)
		{
			compiled_resource::command cmd;
			cmd.config.color = color;
			cmd.config.primitives = primitive_type::triangle_strip;
			cmd.config.disable_vertex_snap = true;
			for (const auto& [x, y] : points)
			{
				vertex v;
				v.vec4(x, y, 0.f, 0.f);
				cmd.verts.push_back(v);
			}
			compiled_resource part;
			part.append(cmd);
			result.add(part);
		};

		// Flying in, slowing as they arrive; then a small pop as they lock
		const f32 fly = ease_out(progress(intro, c_intro_shards, c_intro_lock - c_intro_shards));
		const f32 pop = 1.f + 0.06f * locked * (1.f - ease_out(progress(intro, c_intro_lock, 0.4f)));
		const f32 shards_alpha = progress(intro, c_intro_shards, 0.3f) * out;
		const f32 flash = 0.75f * locked * (1.f - ease_out(progress(intro, c_intro_lock, 0.45f))) * out;

		// The facets: from the centre to the corners and the middles of the
		// sides, shaded as a cut stone lit from the top left
		constexpr f32 r = c_diamond_r;
		static constexpr std::pair<f32, f32> rim[8]{{0.f, -r}, {r / 2, -r / 2}, {r, 0.f}, {r / 2, r / 2}, {0.f, r}, {-r / 2, r / 2}, {-r, 0.f}, {-r / 2, -r / 2}};
		const color4f deep(0.03f, 0.09f, 0.36f, 1.f);
		const color4f bright(0.36f, 0.68f, 1.f, 1.f);
		for (int i = 0; i < 8; i++)
		{
			const auto [ax, ay] = rim[i];
			const auto [bx, by] = rim[(i + 1) % 8];
			const f32 mx = (ax + bx) / 3.f, my = (ay + by) / 3.f; // its centroid
			const f32 angle = std::atan2(my, mx);
			const f32 light = 0.5f + 0.5f * std::cos(angle + 2.356f);

			// Out along its own direction, and turned about its centroid
			const f32 length = std::hypot(mx, my);
			const f32 away = (1.f - fly) * 760.f;
			const f32 dx = mx / length * away, dy = my / length * away;
			const f32 turn = (1.f - fly) * (i % 2 ? 2.2f : -2.2f);
			const f32 cs = std::cos(turn), sn = std::sin(turn);
			const auto at = [&](f32 x, f32 y) -> std::pair<f32, f32>
			{
				const f32 lx = x - mx, ly = y - my;
				return {c_diamond_x + (mx + lx * cs - ly * sn + dx) * pop, c_diamond_y + (my + lx * sn + ly * cs + dy) * pop};
			};

			const color4f color(deep.r + (bright.r - deep.r) * light, deep.g + (bright.g - deep.g) * light, deep.b + (bright.b - deep.b) * light, shards_alpha);
			shape({at(0.f, 0.f), at(ax, ay), at(bx, by)}, color);
			if (flash > 0.f)
			{
				shape({at(0.f, 0.f), at(ax, ay), at(bx, by)}, color4f(1.f, 1.f, 1.f, flash));
			}
		}

		// The gold rim, drawn on as they lock
		if (const f32 rim_alpha = ease_out(progress(intro, c_intro_lock, 0.3f)) * out; rim_alpha > 0.f)
		{
			const color4f gold(0.98f, 0.76f, 0.24f, rim_alpha);
			constexpr f32 width = 3.f;
			for (int i = 0; i < 8; i += 2)
			{
				const auto [ax, ay] = rim[i];
				const auto [bx, by] = rim[(i + 2) % 8];
				const f32 x0 = c_diamond_x + ax * pop, y0 = c_diamond_y + ay * pop;
				const f32 x1 = c_diamond_x + bx * pop, y1 = c_diamond_y + by * pop;
				const f32 length = std::hypot(x1 - x0, y1 - y0);
				const f32 ux = (x1 - x0) / length, uy = (y1 - y0) / length;
				const f32 nx = -uy * width / 2.f, ny = ux * width / 2.f;
				// Each side a little long, so the corners close
				const f32 sx = x0 - ux * width / 2.f, sy = y0 - uy * width / 2.f;
				const f32 ex = x1 + ux * width / 2.f, ey = y1 + uy * width / 2.f;
				shape({{sx + nx, sy + ny}, {sx - nx, sy - ny}, {ex + nx, ey + ny}, {ex - nx, ey - ny}}, gold);
			}
		}

		// The mark, landing in the middle as the shards lock
		if (m_mark_data)
		{
			const f32 land = ease_out(progress(intro, c_intro_lock - 0.05f, 0.4f));
			const f32 size = c_mark_size * (0.82f + 0.18f * land) * pop;
			image_view mark;
			mark.set_raw_image(m_mark_data.get());
			mark.back_color.a = 0.f;
			mark.set_size(static_cast<u16>(std::lround(size)), static_cast<u16>(std::lround(size)));
			mark.set_pos(static_cast<s16>(std::lround(c_diamond_x - size / 2.f)), static_cast<s16>(std::lround(c_diamond_y - size / 2.f)));
			add_animated(result, mark.get_compiled(), ease_out(progress(intro, c_intro_lock - 0.05f, 0.3f)) * out);
		}

		// The maker's name
		const f32 name = ease_out(progress(intro, c_intro_name, 0.5f));
		add_animated(result, m_intro_name.get_compiled(), name * out, 0.f, 8.f * (1.f - name));
	}

	void ps5_launcher_dialog::compile_settings(compiled_resource& result)
	{
		// The Library's deep blue and glow
		{
			overlay_element base;
			base.set_size(virtual_width, virtual_height);
			base.back_color = c_library_blue;
			result.add(base.get_compiled());
		}
		const auto glow = [&](s16 cx, s16 cy, u16 w, u16 h, const color4f& color)
		{
			image_view view;
			view.set_raw_image(m_glow_image.get());
			view.back_color.a = 0.f;
			view.fore_color = color;
			view.set_pos(static_cast<s16>(cx - w / 2), static_cast<s16>(cy - h / 2));
			view.set_size(w, h);
			result.add(view.get_compiled());
		};
		glow(640, 340, 1600, 1000, color4f(0.1f, 0.2f, 0.65f, 0.34f));
		glow(620, 360, 900, 700, color4f(c_accent.r, c_accent.g, c_accent.b, 0.06f));

		const auto card = [&](s16 x, s16 y, u16 w, u16 h)
		{
			rounded_rect panel;
			panel.set_pos(x, y);
			panel.set_size(w, h);
			panel.border_radius = 24;
			panel.back_color = color4f(0.02f, 0.04f, 0.16f, 0.78f);
			result.add(panel.get_compiled());
			rounded_rect edge;
			edge.set_pos(static_cast<s16>(x + 24), y);
			edge.set_size(static_cast<u16>(w - 48), 3);
			edge.border_radius = 2;
			edge.back_color = c_accent;
			result.add(edge.get_compiled());
		};
		const auto text = [&](std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 x, f32 mid) -> std::unique_ptr<label>
		{
			auto l = make_label(s, size, font_name, colour);
			place(*l, x, mid);
			result.add(l->get_compiled());
			return l;
		};

		// The categories, on the left
		constexpr s16 top = 96;
		constexpr u16 height = 552;
		card(40, top, 250, height);
		text(spaced("SETTINGS"), 9, f_semibold, c_accent, 66, top + 38.f);
		const auto& categories = c_settings_tabs;
		for (usz c = 0; c < std::size(categories); c++)
		{
			const bool selected = static_cast<s32>(c) == m_set_category;
			const bool lit = selected && !m_set_focus_list;
			const f32 y = top + 58.f + c * 44.f; // nine, as the desktop dialog has them
			if (selected)
			{
				rounded_rect pill;
				pill.set_pos(54, static_cast<s16>(y));
				pill.set_size(222, 40);
				pill.border_radius = 20;
				pill.back_color = lit ? c_accent : color4f(1.f, 1.f, 1.f, 0.12f);
				result.add(pill.get_compiled());
			}
			// Icons load once and are kept, by path
			if (!m_set_icons.contains(categories[c].icon))
			{
				auto& kept = m_set_icons[categories[c].icon];
				if ((kept = resource_config::load_icon(categories[c].icon)))
				{
					kept->dirty = true;
				}
			}
			if (const auto it = m_set_icons.find(categories[c].icon); it != m_set_icons.end() && it->second)
			{
				image_view icon;
				icon.set_raw_image(it->second.get());
				icon.set_size(18, 18);
				icon.set_pos(72, static_cast<s16>(y + 11));
				icon.back_color.a = 0.f;
				icon.fore_color = lit ? c_button_text : (selected ? c_text : c_text_dim);
				result.add(icon.get_compiled());
			}
			text(categories[c].name, 14, selected ? f_bold : f_medium, lit ? c_button_text : (selected ? c_text : c_text_dim), 102, y + 20.f);
		}
		{
			label note;
			note.set_font(10, f_regular);
			note.fore_color = c_text_dim;
			note.back_color.a = 0.f;
			note.set_padding(0);
			note.set_wrap_text(true);
			note.set_text("Saved at once, used from the next game started. Games with settings of their own keep those.");
			note.set_pos(64, static_cast<s16>(top + height - 80));
			note.set_size(204, 60);
			note.auto_resize(false, 204, 60);
			result.add(note.get_compiled());
		}

		// The category's settings, in the middle
		const settings_tab& category = categories[m_set_category];
		constexpr s16 list_x = 310;
		constexpr u16 list_w = 600;
		card(list_x, top, list_w, height);
		text(category.name, 22, f_bold, c_text, list_x + 32, top + 44.f);
		text(category.note, 11, f_regular, c_text_dim, list_x + 32, top + 72.f);

		constexpr f32 row_h = 44.f;
		constexpr s32 visible = 10;
		if (m_set_row < m_set_scroll) m_set_scroll = m_set_row;
		if (m_set_row >= m_set_scroll + visible) m_set_scroll = m_set_row - visible + 1;
		for (s32 r = m_set_scroll; r < static_cast<s32>(m_set_rows.size()) && r < m_set_scroll + visible; r++)
		{
			const game_setting& row = m_set_rows[r];
			const f32 y = top + 96.f + (r - m_set_scroll) * row_h;
			const bool selected = m_set_focus_list && r == m_set_row;
			const bool changed = row.value != row.global;
			const f32 mid = y + row_h / 2.f;
			const color4f ink = selected ? c_button_text : (changed ? c_text : c_text_dim);

			if (selected)
			{
				rounded_rect pill;
				pill.set_pos(list_x + 16, static_cast<s16>(y + 3));
				pill.set_size(list_w - 32, static_cast<u16>(row_h - 6));
				pill.border_radius = static_cast<u16>((row_h - 6) / 2);
				pill.back_color = c_accent;
				result.add(pill.get_compiled());
			}
			else
			{
				overlay_element rule;
				rule.set_pos(list_x + 32, static_cast<s16>(y + row_h - 1));
				rule.set_size(list_w - 64, 1);
				rule.back_color = color4f(1.f, 1.f, 1.f, 0.07f);
				result.add(rule.get_compiled());
			}
			if (changed)
			{
				ellipse dot;
				dot.set_size(6, 6);
				dot.set_pos(list_x + 32, static_cast<s16>(mid - 3));
				dot.back_color = selected ? c_button_text : c_accent;
				result.add(dot.get_compiled());
			}
			auto value = make_label("", 13, f_semibold, selected ? c_button_text : (changed ? c_accent : c_text_dim));
			fit_text(*value, setting_text(row.key, row.value), 240);
			const s16 right = static_cast<s16>(list_x + list_w - (selected ? 52 : 34));
			place(*value, static_cast<s16>(right - value->w), mid);
			result.add(value->get_compiled());

			// The name, in what the value leaves
			auto name = make_label("", 13, selected ? f_bold : f_medium, ink);
			fit_text(*name, row.label, static_cast<u16>(std::max(80, value->x - (selected ? 24 : 16) - (list_x + 46))));
			place(*name, list_x + 46, mid);
			result.add(name->get_compiled());
			if (selected)
			{
				text("‹", 16, f_bold, c_button_text, static_cast<s16>(value->x - 18), mid);
				text("›", 16, f_bold, c_button_text, static_cast<s16>(right + 10), mid);
			}
		}
		if (static_cast<s32>(m_set_rows.size()) > visible)
		{
			// Where in the list
			const f32 track = visible * row_h;
			const f32 thumb = track * visible / m_set_rows.size();
			rounded_rect bar;
			bar.set_pos(list_x + list_w - 14, static_cast<s16>(top + 96 + (track - thumb) * m_set_scroll / (m_set_rows.size() - visible)));
			bar.set_size(4, static_cast<u16>(thumb));
			bar.border_radius = 2;
			bar.back_color = color4f(1.f, 1.f, 1.f, 0.25f);
			result.add(bar.get_compiled());
		}

		// The setting, explained, on the right
		constexpr s16 info_x = 930;
		constexpr u16 info_w = 310;
		card(info_x, top, info_w, height);
		if (!m_set_rows.empty())
		{
			const game_setting& row = m_set_rows[std::clamp<s32>(m_set_row, 0, static_cast<s32>(m_set_rows.size()) - 1)];
			text(spaced(m_set_focus_list ? "SETTING" : "IN THIS CATEGORY"), 9, f_semibold, c_accent, info_x + 26, top + 38.f);
			if (m_set_focus_list)
			{
				label name;
				name.set_font(17, f_bold);
				name.fore_color = c_text;
				name.back_color.a = 0.f;
				name.set_padding(0);
				name.set_wrap_text(true);
				name.set_text(row.label);
				name.set_pos(info_x + 26, top + 54);
				name.set_size(info_w - 52, 60);
				name.auto_resize(false, info_w - 52, 60);
				result.add(name.get_compiled());

				label help;
				// RPCS3's own tooltip: some run to a dozen lines
				help.set_font(11, f_regular);
				help.fore_color = c_text_dim;
				help.back_color.a = 0.f;
				help.set_padding(0);
				help.set_wrap_text(true);
				help.set_text(row.help.empty() ? "RPCS3 has no description of this setting." : row.help);
				help.set_pos(info_x + 26, static_cast<s16>(name.y + name.h + 14));
				help.set_size(info_w - 52, 320);
				help.auto_resize(false, info_w - 52, 320);
				result.add(help.get_compiled());

				const f32 y = help.y + help.h + 30.f;
				overlay_element rule;
				rule.set_pos(info_x + 26, static_cast<s16>(y - 14));
				rule.set_size(info_w - 52, 1);
				rule.back_color = color4f(1.f, 1.f, 1.f, 0.12f);
				result.add(rule.get_compiled());
				text("Default", 12, f_medium, c_text_dim, info_x + 26, y + 6.f);
				auto def = make_label(setting_text(row.key, row.global), 12, f_semibold, c_text);
				place(*def, static_cast<s16>(info_x + info_w - 26 - def->w), y + 6.f);
				result.add(def->get_compiled());
				text("Now", 12, f_medium, c_text_dim, info_x + 26, y + 32.f);
				auto now = make_label(setting_text(row.key, row.value), 12, f_semibold, row.value != row.global ? c_accent : c_text);
				place(*now, static_cast<s16>(info_x + info_w - 26 - now->w), y + 32.f);
				result.add(now->get_compiled());
			}
			else
			{
				// The category's settings at a glance
				f32 y = top + 70.f;
				for (const game_setting& each : m_set_rows)
				{
					if (y > top + height - 30) break;
					auto name = make_label("", 11, f_medium, c_text_dim);
					fit_text(*name, each.label, 160);
					place(*name, info_x + 26, y);
					result.add(name->get_compiled());
					auto value = make_label("", 11, f_semibold, each.value != each.global ? c_accent : c_text);
					fit_text(*value, setting_text(each.key, each.value), 110);
					place(*value, static_cast<s16>(info_x + info_w - 26 - value->w), y);
					result.add(value->get_compiled());
					y += 26.f;
				}
			}
		}
	}

	f32 ps5_launcher_dialog::intro_seconds() const
	{
		if (!m_play_intro)
		{
			return 100.f;
		}
		return m_intro_start_us ? (m_now_us - m_intro_start_us) / 1'000'000.f : 0.f;
	}

	f32 ps5_launcher_dialog::content_seconds() const
	{
		// From the later of the list being read and the splash making way
		const f32 since_read = m_content_start_us ? (m_now_us - m_content_start_us) / 1'000'000.f : -1.f;
		return std::min(since_read, intro_seconds() - c_intro_content);
	}

	bool ps5_launcher_dialog::intro_running() const
	{
		return intro_seconds() < c_intro_end || (m_content_start_us && content_seconds() < c_content_end);
	}

	void ps5_launcher_dialog::skip_intro()
	{
		// Straight to the end: both clocks run back past their last step, and
		// the intro's sound fades
		ps5_stop_sounds();
		constexpr u64 far = 30'000'000;
		if (m_intro_start_us && m_now_us > far)
		{
			m_intro_start_us = m_now_us - far;
		}
		if (m_content_start_us && m_now_us > far)
		{
			m_content_start_us = m_now_us - far;
		}
	}


	namespace
	{
		// The Library's 3D: covers are PS3 cases, turned and placed in a space
		// whose origin is the row's middle, seen through a pinhole at `c_focal`
		// in front of the screen. A case is 135 x 170 x 14 mm with rounded
		// corners; its cover sits under the clear front 3 mm in from the sides
		// and foot and 14 mm down from the top, where the plastic is clear
		constexpr f32 c_focal = 1100.f;
		constexpr f32 c_row_mid_x = 640.f;
		constexpr f32 c_row_mid_y = 322.f;
		constexpr f32 c_case_w = 268.f;
		constexpr f32 c_case_h = c_case_w * 170.f / 135.f;
		constexpr f32 c_case_d = c_case_w * 14.f / 135.f;
		constexpr f32 c_case_r = c_case_w * 4.5f / 135.f;     // corner radius
		constexpr f32 c_insert_side = c_case_w * 3.f / 135.f;
		constexpr f32 c_insert_top = c_case_w * 14.f / 135.f;
		constexpr f32 c_insert_foot = c_case_w * 3.f / 135.f;
		constexpr f32 c_deg = 3.14159265f / 180.f;

		struct vec3
		{
			f32 x, y, z;
		};

		vec3 operator+(vec3 a, vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
		vec3 operator*(vec3 a, f32 s) { return {a.x * s, a.y * s, a.z * s}; }
		f32 dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

		// A case's pose: where its middle is, how it is turned, how big
		struct case_pose
		{
			vec3 at{};
			f32 yaw = 0.f;   // about the vertical axis
			f32 pitch = 0.f; // about the horizontal axis
			f32 scale = 1.f;
			f32 alpha = 1.f;
			bool mirrored = false; // its reflection in the floor
			f32 floor_y = 0.f;
		};

		vec3 turn(const case_pose& pose, vec3 p)
		{
			// Yaw, then pitch
			const f32 cy = std::cos(pose.yaw), sy = std::sin(pose.yaw);
			const f32 cp = std::cos(pose.pitch), sp = std::sin(pose.pitch);
			const vec3 a{p.x * cy + p.z * sy, p.y, -p.x * sy + p.z * cy};
			return {a.x, a.y * cp - a.z * sp, a.y * sp + a.z * cp};
		}

		vec3 to_world(const case_pose& pose, vec3 local)
		{
			vec3 p = turn(pose, local * pose.scale) + pose.at;
			if (pose.mirrored)
			{
				p.y = 2.f * pose.floor_y - p.y;
			}
			return p;
		}

		void project(vec3 p, f32& x, f32& y)
		{
			const f32 k = c_focal / (c_focal + p.z);
			x = c_row_mid_x + p.x * k;
			y = c_row_mid_y + p.y * k;
		}

		// Half the case's width at height y: narrower in the rounded corners
		f32 case_half_width(f32 y)
		{
			const f32 into = std::max(0.f, std::abs(y) - (c_case_h / 2 - c_case_r));
			return c_case_w / 2 - (c_case_r - std::sqrt(std::max(0.f, c_case_r * c_case_r - into * into)));
		}

		// The case's outline, clockwise from the top of its left side, as
		// points with their outward normals; each corner in `steps` pieces
		struct outline_point
		{
			f32 x, y, nx, ny;
		};

		std::vector<outline_point> case_outline(int steps)
		{
			std::vector<outline_point> points;
			const f32 ix = c_case_w / 2 - c_case_r, iy = c_case_h / 2 - c_case_r;
			// Corner centres, and the angle each corner's arc starts at
			const struct { f32 cx, cy, start; } corners[] =
			{
				{-ix, -iy, 180.f}, // top left: from the left side round to the top
				{ix, -iy, 270.f},  // top right
				{ix, iy, 0.f},     // bottom right
				{-ix, iy, 90.f},   // bottom left
			};
			for (const auto& corner : corners)
			{
				for (int s = 0; s <= steps; s++)
				{
					const f32 angle = (corner.start + 90.f * s / steps) * c_deg;
					const f32 nx = std::cos(angle), ny = std::sin(angle);
					points.push_back({corner.cx + nx * c_case_r, corner.cy + ny * c_case_r, nx, ny});
				}
			}
			return points;
		}

		// Light from the front, above and a little left
		const vec3 c_light = [] { const vec3 l{-0.3f, -0.55f, -1.f}; return l * (1.f / std::sqrt(dot(l, l))); }();
	}

	void ps5_launcher_dialog::handle_library(pad_button button_press)
	{
		if (m_games.empty())
		{
			return;
		}

		if (m_detail)
		{
			// The game's own menu
			switch (button_press)
			{
			case pad_button::dpad_up:
			case pad_button::ls_up:
				if (m_detail_option > 0)
				{
					m_detail_option--;
					play_sound(sound_effect::cursor);
				}
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
				if (m_detail_option < 5)
				{
					m_detail_option++;
					play_sound(sound_effect::cursor);
				}
				break;
			case pad_button::cross:
				switch (m_detail_option)
				{
				case 0: boot_selected(); break;
				case 1: open_game_settings(); break;
				case 2: open_patches(); break;
				case 3: m_flipped = !m_flipped; play_sound(sound_effect::cursor); break;
				case 4: ask_delete(); break;
				default: m_detail = false; play_sound(sound_effect::cancel); layout_hints(); break;
				}
				break;
			case pad_button::R3:
				m_flipped = !m_flipped;
				play_sound(sound_effect::cursor);
				break;
			case pad_button::circle:
				m_detail = false;
				play_sound(sound_effect::cancel);
				layout_hints();
				break;
			default:
				break;
			}
			return;
		}

		switch (button_press)
		{
		case pad_button::dpad_left:
		case pad_button::ls_left:
			select_game(m_selected - 1);
			break;
		case pad_button::dpad_right:
		case pad_button::ls_right:
			select_game(m_selected + 1);
			break;
		case pad_button::L2:
			select_game(m_selected - 5);
			break;
		case pad_button::R2:
			select_game(m_selected + 5);
			break;
		case pad_button::cross:
			// Into the game's own menu: its case comes forward, spinning once
			m_detail = true;
			m_detail_option = 0;
			m_spin += 2.f * 3.14159265f;
			play_sound(sound_effect::accept);
			layout_hints();
			break;
		case pad_button::triangle:
			open_game_settings();
			break;
		case pad_button::square:
			ask_delete();
			break;
		case pad_button::circle:
			set_tab(tab::home);
			break;
		case pad_button::R3:
			// Turn the case round to its back, and back again
			m_flipped = !m_flipped;
			play_sound(sound_effect::cursor);
			break;
		default:
			break;
		}
	}

	void ps5_launcher_dialog::compile_library(compiled_resource& result)
	{
		// The deep blue, lighter toward the middle, and a cyan pool on the floor
		// under the middle case
		{
			overlay_element base;
			base.set_size(virtual_width, virtual_height);
			base.back_color = c_library_blue;
			result.add(base.get_compiled());
		}
		const auto glow = [&](s16 cx, s16 cy, u16 w, u16 h, const color4f& color)
		{
			image_view view;
			view.set_raw_image(m_glow_image.get());
			view.back_color.a = 0.f;
			view.fore_color = color;
			view.set_pos(static_cast<s16>(cx - w / 2), static_cast<s16>(cy - h / 2));
			view.set_size(w, h);
			result.add(view.get_compiled());
		};
		glow(640, 320, 1600, 1000, color4f(0.1f, 0.2f, 0.65f, 0.34f));

		const f32 detail = m_detail_t * m_detail_t * (3.f - 2.f * m_detail_t);
		const f32 floor_y = c_case_h / 2.f + 6.f;

		glow(static_cast<s16>(640 - 330 * detail), static_cast<s16>(c_row_mid_y + floor_y + 10), 640, 150, color4f(c_accent.r, c_accent.g, c_accent.b, 0.22f));

		if (m_games.empty())
		{
			result.add(m_placeholder.get_compiled());
			return;
		}

		// Where each case goes: the selected one faces the screen, turned by the
		// right stick and swaying a little by itself; the others turn away to
		// their side, pushed back and stacked. In the game's menu the selected
		// one comes forward on the left and the others leave
		const auto pose_of = [&](s32 index) -> case_pose
		{
			const f32 d = index - m_flow_pos;
			const f32 side = d < 0.f ? -1.f : 1.f;
			const f32 a = std::abs(d);
			const f32 t = std::min(a, 1.f);
			const f32 e = t * t * (3.f - 2.f * t);

			case_pose pose;
			pose.at = {side * (330.f * e + std::max(0.f, a - 1.f) * 120.f), 0.f, 150.f * e + std::max(0.f, a - 1.f) * 30.f};
			pose.yaw = -side * 58.f * c_deg * e;
			pose.alpha = std::clamp(4.6f - a, 0.f, 1.f);

			// The selected one's own turn fades in as it reaches the middle
			const f32 own = 1.f - e;
			pose.yaw += own * m_cover_yaw;
			pose.pitch += own * m_cover_pitch;

			if (index == m_selected)
			{
				pose.yaw += m_flip_angle + m_spin;
				pose.at = pose.at + vec3{-330.f, 6.f, -170.f} * detail;
			}
			else
			{
				pose.at.x += side * 500.f * detail;
				pose.alpha *= 1.f - detail;
			}
			pose.floor_y = floor_y;
			return pose;
		};

		// A patch of a case's surface as one triangle strip over a grid (rows
		// joined by repeated vertices), fine enough that the texture follows the
		// perspective. `at(a, b)` gives the local point at grid position (a, b)
		// in 0..1; the texture spans uv over the patch
		const auto add_patch = [&](const image_info_base* image, const f32 uv[4], const case_pose& pose, int cols, int rows, f32 b0, f32 b1, const color4f& color, const auto& at)
		{
			compiled_resource::command cmd;
			if (image)
			{
				cmd.config.set_image_resource(image_resource_id::raw_image);
				cmd.config.external_data_ref = image;
			}
			cmd.config.color = color;
			cmd.config.primitives = primitive_type::triangle_strip;
			cmd.config.disable_vertex_snap = true;

			const auto point = [&](f32 a, f32 b)
			{
				f32 x, y;
				project(to_world(pose, at(a, b)), x, y);
				vertex v;
				v.vec4(x, y, uv[0] + (uv[1] - uv[0]) * a, uv[2] + (uv[3] - uv[2]) * b);
				return v;
			};

			for (int r = 0; r < rows; r++)
			{
				const f32 bt = b0 + (b1 - b0) * r / rows;
				const f32 bb = b0 + (b1 - b0) * (r + 1) / rows;
				if (r > 0)
				{
					cmd.verts.push_back(cmd.verts.back());
					cmd.verts.push_back(point(0.f, bt));
				}
				for (int c = 0; c <= cols; c++)
				{
					const f32 a = static_cast<f32>(c) / cols;
					cmd.verts.push_back(point(a, bt));
					cmd.verts.push_back(point(a, bb));
				}
			}

			compiled_resource part;
			part.append(cmd);
			result.add(part);
		};

		const auto facing = [&](const case_pose& pose, vec3 normal, vec3 local)
		{
			// Seen when it faces the eye (at -focal on z)
			vec3 n = turn(pose, normal);
			if (pose.mirrored) n.y = -n.y;
			const vec3 p = to_world(pose, local);
			return dot(n, vec3{p.x, p.y, p.z + c_focal}) < 0.f;
		};

		const auto lit = [&](const case_pose& pose, vec3 normal) -> f32
		{
			return 0.42f + 0.58f * std::max(0.f, dot(turn(pose, normal), c_light));
		};

		// The case's plastic: a pale grey, lit; the cover's ink: lit, a little
		const auto plastic = [](f32 shade, f32 alpha) { return color4f(0.5f + 0.36f * shade, 0.53f + 0.36f * shade, 0.6f + 0.36f * shade, alpha); };
		const auto ink = [](f32 shade, f32 alpha) { const f32 s = 0.35f + 0.65f * shade; return color4f(s, s, s, alpha); };

		// The front or back of a case: its rounded plastic, the cover under it,
		// and the sheen over both. `side` is -1 for the front, +1 for the back
		// (seen from behind, so its left is the case's right)
		const auto add_side = [&](const ps5_launcher_game& game, const case_pose& pose, f32 side, int detail_level, f32 alpha, f32 b0, f32 b1)
		{
			const vec3 normal{0.f, 0.f, side};
			const f32 z = side * c_case_d / 2.f;
			const f32 shade = lit(pose, normal);
			const f32 flip = -side; // +1: a runs left to right across the case

			// The plastic: rows spaced finer toward the rounded ends
			const auto tray = [&](f32 a, f32 b)
			{
				const f32 t = (1.f - std::cos(3.14159265f * b)) / 2.f;
				const f32 y = -c_case_h / 2 + c_case_h * t;
				const f32 hw = case_half_width(y);
				return vec3{flip * (-hw + 2.f * hw * a), y, z};
			};
			static constexpr f32 whole[4] = {0.f, 1.f, 0.f, 1.f};
			add_patch(nullptr, whole, pose, 1, detail_level, b0, b1, plastic(shade, alpha), tray);

			// The cover, under the clear front
			const image_info_base* image = side < 0.f ? game.cover() : game.back();
			const f32* crop = side < 0.f ? game.cover_crop : game.back_crop;
			const f32 top = -c_case_h / 2 + c_insert_top;
			const f32 foot = c_case_h / 2 - c_insert_foot;
			const auto insert = [&](f32 a, f32 b)
			{
				const f32 half = c_case_w / 2 - c_insert_side;
				return vec3{flip * (-half + 2.f * half * a), top + (foot - top) * b, z + side * 0.4f};
			};
			// The cover's rows that fall in [b0, b1] of the case's height
			const f32 cb0 = std::clamp((std::lerp(-c_case_h / 2, c_case_h / 2, b0) - top) / (foot - top), 0.f, 1.f);
			const f32 cb1 = std::clamp((std::lerp(-c_case_h / 2, c_case_h / 2, b1) - top) / (foot - top), 0.f, 1.f);
			if (cb1 > cb0)
			{
				const f32 uv[4] = {crop[0], crop[1], crop[2] + (crop[3] - crop[2]) * cb0, crop[2] + (crop[3] - crop[2]) * cb1};
				const auto part = [&](f32 a, f32 b) { return insert(a, cb0 + (cb1 - cb0) * b); };
				add_patch(image, uv, pose, detail_level, std::max(1, static_cast<int>(detail_level * (cb1 - cb0))), 0.f, 1.f, ink(shade, alpha), part);
			}

			// The sheen, sliding across as the case turns
			if (m_sheen_image && !pose.mirrored)
			{
				const f32 slide = std::clamp(-pose.yaw * 0.55f + pose.pitch * 0.3f, -0.6f, 0.6f) + (side > 0.f ? 0.25f : 0.f);
				const f32 sheen_uv[4] = {slide, slide + 1.f, 0.f, 1.f};
				const auto gloss = [&](f32 a, f32 b) { vec3 p = tray(a, b); p.z += side * 0.8f; return p; };
				add_patch(m_sheen_image.get(), sheen_uv, pose, detail_level, detail_level, 0.f, 1.f, color4f(1.f, 1.f, 1.f, 0.2f * alpha), gloss);
			}
		};

		// The case's rim: its rounded sides, plastic, but the spine (the left
		// side) carries the spine's print between the top band and the foot
		const auto add_rim = [&](const ps5_launcher_game& game, const case_pose& pose, int steps, f32 alpha)
		{
			const std::vector<outline_point> outline = case_outline(steps);
			const usz count = outline.size();
			static constexpr f32 whole[4] = {0.f, 1.f, 0.f, 1.f};
			const f32 half_d = c_case_d / 2.f;

			for (usz i = 0; i < count; i++)
			{
				const outline_point& p0 = outline[i];
				const outline_point& p1 = outline[(i + 1) % count];
				const f32 nx = (p0.nx + p1.nx) / 2.f, ny = (p0.ny + p1.ny) / 2.f;
				const f32 length = std::sqrt(nx * nx + ny * ny);
				if (length < 1e-4f) continue;
				const vec3 normal{nx / length, ny / length, 0.f};
				const vec3 middle{(p0.x + p1.x) / 2.f, (p0.y + p1.y) / 2.f, 0.f};
				if (!facing(pose, normal, middle) || (pose.mirrored && normal.y != 0.f && std::abs(normal.y) > 0.7f))
				{
					continue;
				}
				const f32 shade = lit(pose, normal);

				// Along the segment (a), back to front (b)
				const auto wall = [&](f32 y0, f32 y1)
				{
					return [&, y0, y1](f32 a, f32 b)
					{
						const f32 x = p0.x + (p1.x - p0.x) * a;
						const f32 y = y0 + (y1 - y0) * a;
						return vec3{x, y, half_d - 2.f * half_d * b};
					};
				};

				const bool spine = i + 1 == count; // from the bottom-left corner's end up to the top-left's start
				if (!spine)
				{
					add_patch(nullptr, whole, pose, 1, 1, 0.f, 1.f, plastic(shade, alpha), wall(p0.y, p1.y));
					continue;
				}

				// The spine runs up from the foot (p0) to the top (p1): plastic,
				// then the print from the foot to the top band, then plastic
				const f32 print_foot = c_case_h / 2 - c_case_r;
				const f32 print_top = -c_case_h / 2 + c_insert_top;
				add_patch(nullptr, whole, pose, 1, 1, 0.f, 1.f, plastic(shade, alpha), wall(p0.y, print_foot));
				add_patch(nullptr, whole, pose, 1, 1, 0.f, 1.f, plastic(shade, alpha), wall(print_top, p1.y));
				if (const image_info_base* print = game.spine())
				{
					// The print's top is at print_top: v runs down it; u runs
					// across from the back (0) to the front (1)
					const auto strip = [&](f32 a, f32 b)
					{
						const f32 y = print_top + (print_foot - print_top) * a;
						return vec3{-c_case_w / 2 - 0.4f, y, half_d - 2.f * half_d * b};
					};
					// Grid: a down the spine, b across it; the texture's u is
					// across (b) and v down (a), so swap them in the uv
					compiled_resource::command cmd;
					cmd.config.set_image_resource(image_resource_id::raw_image);
					cmd.config.external_data_ref = print;
					cmd.config.color = ink(shade, alpha);
					cmd.config.primitives = primitive_type::triangle_strip;
					cmd.config.disable_vertex_snap = true;
					constexpr int rows = 10;
					for (int r = 0; r <= rows; r++)
					{
						const f32 a = static_cast<f32>(r) / rows;
						for (const f32 b : {0.f, 1.f})
						{
							f32 x, y;
							project(to_world(pose, strip(a, b)), x, y);
							vertex v;
							v.vec4(x, y, game.spine_crop[0] + (game.spine_crop[1] - game.spine_crop[0]) * b, game.spine_crop[2] + (game.spine_crop[3] - game.spine_crop[2]) * a);
							cmd.verts.push_back(v);
						}
					}
					compiled_resource part;
					part.append(cmd);
					result.add(part);
				}
				else
				{
					add_patch(nullptr, whole, pose, 1, 1, 0.f, 1.f, plastic(shade, alpha), wall(print_foot, print_top));
				}
			}
		};

		// Far ones first, the selected one last
		std::vector<s32> order;
		for (s32 i = 0; i < static_cast<s32>(m_games.size()); i++)
		{
			if (std::abs(i - m_flow_pos) < 5.f)
			{
				order.push_back(i);
			}
		}
		std::sort(order.begin(), order.end(), [&](s32 a, s32 b)
		{
			if (a == m_selected && m_detail_t > 0.f) return false;
			if (b == m_selected && m_detail_t > 0.f) return true;
			return std::abs(a - m_flow_pos) > std::abs(b - m_flow_pos);
		});

		for (const s32 i : order)
		{
			const ps5_launcher_game& game = m_games[i];
			const case_pose pose = pose_of(i);
			if (pose.alpha <= 0.f)
			{
				continue;
			}
			const bool near_middle = std::abs(i - m_flow_pos) < 1.5f;
			const int grid = near_middle ? 16 : 8;
			const int steps = near_middle ? 4 : 2;

			// Its reflection: the side facing us, mirrored in the floor and
			// fading downward in bands (only the case's foot shows)
			case_pose mirror = pose;
			mirror.mirrored = true;
			for (const f32 side : {-1.f, 1.f})
			{
				if (!facing(mirror, vec3{0.f, 0.f, side}, vec3{0.f, 0.f, side * c_case_d / 2.f}))
				{
					continue;
				}
				constexpr int bands = 5;
				for (int b = 0; b < bands; b++)
				{
					const f32 f0 = 1.f - 0.3f * b / bands;
					const f32 f1 = 1.f - 0.3f * (b + 1) / bands;
					add_side(game, mirror, side, 1, 0.24f * (1.f - (b + 0.5f) / bands) * pose.alpha, f1, f0);
				}
			}

			// The case: its rim, then the side facing us
			add_rim(game, pose, steps, pose.alpha);
			for (const f32 side : {-1.f, 1.f})
			{
				if (facing(pose, vec3{0.f, 0.f, side}, vec3{0.f, 0.f, side * c_case_d / 2.f}))
				{
					add_side(game, pose, side, grid, pose.alpha, 0.f, 1.f);
				}
			}
		}

		// Under the row: the selected game's name and what it is (leaving as
		// the menu comes in)
		if (m_selected >= 0 && static_cast<usz>(m_selected) < m_games.size())
		{
			const big_picture_game_info& info = m_games[m_selected].info;

			std::vector<std::string> facts;
			if (!info.serial.empty()) facts.push_back(info.serial);
			if (const std::string region = region_of(info.serial); !region.empty()) facts.push_back(region);
			if (info.category == "DG") facts.push_back("Disc");
			else if (info.category == "HG") facts.push_back("Digital");
			if (!info.app_ver.empty() && info.app_ver != "Unknown") facts.push_back("v" + info.app_ver);
			const bool own_settings = !info.serial.empty() && fs::is_file(rpcs3::utils::get_custom_config_path(info.serial));
			const std::string name_text = info.name.empty() ? info.serial : info.name;

			if (detail < 1.f)
			{
				compiled_resource caption;

				label name;
				style_label(name, "", 22, f_semibold, c_text);
				fit_text(name, name_text, 1000);
				place(name, static_cast<s16>(640 - name.w / 2), 596.f);
				caption.add(name.get_compiled());

				std::vector<std::unique_ptr<label>> parts;
				for (usz i = 0; i < facts.size(); i++)
				{
					if (i) parts.push_back(make_label("·", 12, f_medium, c_text_dim));
					parts.push_back(make_label(facts[i], 12, f_medium, c_text_dim));
				}
				std::unique_ptr<label> chip_text;
				f32 total = 0.f;
				for (usz i = 0; i < parts.size(); i++) total += parts[i]->w + (i ? 10 : 0);
				if (own_settings)
				{
					chip_text = make_label("Own settings", 10, f_semibold, c_accent);
					total += 16 + chip_text->w + 24;
				}
				f32 x = 640.f - total / 2.f;
				for (usz i = 0; i < parts.size(); i++)
				{
					place(*parts[i], static_cast<s16>(std::lround(x)), 630.f);
					caption.add(parts[i]->get_compiled());
					x += parts[i]->w + (i + 1 < parts.size() ? 10 : 0);
				}
				if (chip_text)
				{
					x += 16;
					rounded_rect chip;
					chip.set_pos(static_cast<s16>(std::lround(x)), 618);
					chip.set_size(static_cast<u16>(chip_text->w + 24), 24);
					chip.border_radius = 12;
					chip.back_color = color4f(c_accent.r, c_accent.g, c_accent.b, 0.16f);
					caption.add(chip.get_compiled());
					place(*chip_text, static_cast<s16>(std::lround(x + 12)), 630.f);
					caption.add(chip_text->get_compiled());
				}

				label count;
				style_label(count, fmt::format("%d / %d", m_selected + 1, m_games.size()), 11, f_medium, c_text_dim);
				place(count, c_margin, c_hints_y);
				caption.add(count.get_compiled());

				add_animated(result, caption, 1.f - detail, 0.f, 20.f * detail);
			}

			// The game's own menu, on the right: a card of glass with the game's
			// name over its options, each a pill with its icon; the chosen one
			// lit in cyan
			if (detail > 0.f)
			{
				compiled_resource menu;
				constexpr s16 card_x = 676;
				constexpr s16 card_y = 112;
				constexpr u16 card_w = 540;
				constexpr s16 left = card_x + 40;
				constexpr u16 inner = card_w - 80;

				// The card, its cyan edge light along the top, and a glow behind it
				glow(card_x + card_w / 2, card_y + 250, 900, 700, color4f(c_accent.r, c_accent.g, c_accent.b, 0.08f));
				rounded_rect card;
				card.set_pos(card_x, card_y);
				card.border_radius = 28;
				card.back_color = color4f(0.02f, 0.04f, 0.16f, 0.78f);
				rounded_rect edge;
				edge.set_pos(card_x + 28, card_y);
				edge.set_size(card_w - 56, 3);
				edge.border_radius = 2;
				edge.back_color = c_accent;

				auto kicker = make_label(spaced(fmt::format("%s%s", region_of(info.serial).empty() ? "PS3" : region_of(info.serial), info.category == "HG" ? "  DIGITAL" : info.category == "DG" ? "  DISC" : "")), 9, f_semibold, c_accent);
				place(*kicker, left, card_y + 44.f);

				label title;
				title.set_font(28, f_bold);
				title.fore_color = c_text;
				title.back_color.a = 0.f;
				title.set_padding(0);
				title.set_wrap_text(true);
				title.set_text(name_text);
				title.set_pos(left - 1, card_y + 58);
				title.set_size(inner, 100);
				title.auto_resize(false, inner, 100);

				f32 y = title.y + title.h + 18.f;
				std::vector<std::unique_ptr<overlay_element>> chips;
				{
					std::vector<std::string> texts;
					if (!info.serial.empty()) texts.push_back(info.serial);
					if (!info.app_ver.empty() && info.app_ver != "Unknown") texts.push_back("Version " + info.app_ver);
					if (own_settings) texts.push_back("Own settings");
					f32 x = left;
					for (const std::string& text : texts)
					{
						const bool accent = text == "Own settings";
						auto chip_label = make_label(text, 10, f_medium, accent ? c_accent : c_text_dim);
						auto chip = std::make_unique<rounded_rect>();
						chip->set_pos(static_cast<s16>(x), static_cast<s16>(y));
						chip->set_size(static_cast<u16>(chip_label->w + 26), 24);
						chip->border_radius = 12;
						chip->back_color = accent ? color4f(c_accent.r, c_accent.g, c_accent.b, 0.16f) : color4f(1.f, 1.f, 1.f, 0.08f);
						place(*chip_label, static_cast<s16>(x + 13), y + 12.f);
						x += chip->w + 8;
						chips.push_back(std::move(chip));
						chips.push_back(std::move(chip_label));
					}
				}
				y += 24.f + 26.f;

				struct option
				{
					const char* text;
					const char* note;
					image_info* icon;
				};
				const option options[] =
				{
					{"Play", "Start the game", m_play_icon_data.get()},
					{"Game settings", "Its own settings, over the global ones", m_settings_icon_data.get()},
					{"Patches", "RPCS3's patches for it: more frames, fixes", m_patches_icon_data.get()},
					{m_flipped ? "Look at the front" : "Look at the back", "Turn the case round (R3)", m_flip_icon_data.get()},
					{"Delete game", "Remove its files from the console", m_delete_icon_data.get()},
					{"Back to library", "", m_back_icon_data.get()},
				};
				constexpr f32 pill_h = 48.f;
				constexpr f32 pill_gap = 6.f;
				std::vector<std::unique_ptr<overlay_element>> rows;
				constexpr int option_count = static_cast<int>(std::size(options));
				for (int o = 0; o < option_count; o++)
				{
					const bool selected = o == m_detail_option;
					const f32 row_y = y + o * (pill_h + pill_gap);
					const color4f ink = selected ? c_button_text : c_text;

					auto pill = std::make_unique<rounded_rect>();
					pill->set_pos(left - 12, static_cast<s16>(row_y));
					pill->set_size(inner + 24, static_cast<u16>(pill_h));
					pill->border_radius = static_cast<u16>(pill_h / 2);
					pill->back_color = selected ? c_accent : color4f(1.f, 1.f, 1.f, 0.06f);
					rows.push_back(std::move(pill));

					// The icon in a disc of its own
					auto disc = std::make_unique<ellipse>();
					disc->set_size(36, 36);
					disc->set_pos(left - 4, static_cast<s16>(row_y + 6));
					disc->back_color = selected ? color4f(1.f, 1.f, 1.f, 0.35f) : color4f(1.f, 1.f, 1.f, 0.08f);
					rows.push_back(std::move(disc));
					if (options[o].icon)
					{
						auto icon = std::make_unique<image_view>();
						icon->set_raw_image(options[o].icon);
						icon->set_size(18, 18);
						icon->set_pos(left + 5, static_cast<s16>(row_y + 15));
						icon->back_color.a = 0.f;
						icon->fore_color = ink;
						rows.push_back(std::move(icon));
					}

					const bool noted = selected && options[o].note[0];
					auto text = make_label(options[o].text, 14, selected ? f_bold : f_medium, ink);
					place(*text, left + 46, row_y + (noted ? 17.f : pill_h / 2.f));
					rows.push_back(std::move(text));
					if (noted)
					{
						auto note = make_label(options[o].note, 10, f_medium, color4f(c_button_text.r, c_button_text.g, c_button_text.b, 0.7f));
						place(*note, left + 46, row_y + 33.f);
						rows.push_back(std::move(note));
					}
					if (selected)
					{
						auto chevron = make_label("\u203a", 20, f_bold, ink);
						place(*chevron, static_cast<s16>(left + inner - 14), row_y + pill_h / 2.f);
						rows.push_back(std::move(chevron));
					}
				}

				const f32 bottom = y + option_count * (pill_h + pill_gap) + 18.f;
				auto tip = make_label("Right stick turns the case  \u00b7  R3 turns it round", 10, f_medium, c_text_dim);
				place(*tip, left, bottom + 8.f);

				card.set_size(card_w, static_cast<u16>(bottom + 36 - card_y));
				menu.add(card.get_compiled());
				menu.add(edge.get_compiled());
				menu.add(kicker->get_compiled());
				menu.add(title.get_compiled());
				for (const auto& chip : chips) menu.add(chip->get_compiled());
				for (const auto& row : rows) menu.add(row->get_compiled());
				menu.add(tip->get_compiled());

				add_animated(result, menu, detail, 40.f * (1.f - detail), 0.f);
			}
		}
	}

	void ps5_launcher_dialog::update(u64 timestamp_us)
	{
		{
			std::lock_guard lock(m_mutex);
			m_now_us = timestamp_us;
			if (!m_intro_start_us)
			{
				m_intro_start_us = timestamp_us;
			}
			if (m_play_intro && !std::exchange(m_intro_sound_started, true))
			{
				ps5_play_sound_file("/app0/assets/launcher/intro.wav");
			}
			if (!m_content_start_us && !m_loading)
			{
				m_content_start_us = timestamp_us;
			}
			if (m_play_intro && intro_seconds() >= c_intro_end + 1.f)
			{
				m_play_intro = false;
			}

			// An installation that has ended: logged, and the Library read again
			if (m_installer && m_installer->done() && !std::exchange(m_in_reported, true))
			{
				usz installed = 0, failed = 0, stopped = 0;
				for (usz i = 0; i < m_installer->files().size(); i++)
				{
					switch (m_installer->state_of(i))
					{
					case ps5_packages::installer::state::installed: installed++; break;
					case ps5_packages::installer::state::failed: failed++; break;
					default: stopped++; break;
					}
				}
				launcher_log.notice("Install: ended, %u installed, %u failed, %u not installed", installed, failed, stopped);
				m_reload_requested = true;
				layout_hints();
			}

			// The game's patches, once read
			std::unique_ptr<ps5_patches::game> patches;
			{
				std::lock_guard result_lock(m_pt_result_mutex);
				if (m_pt_result && m_pt_result_generation == m_pt_generation)
				{
					patches = std::move(m_pt_result);
				}
			}
			// The trophies, or the open set's icons, once read
			take_trophies();

			if (patches && m_pt_open)
			{
				launcher_log.notice("Patches for %s v%s: %u listed, %u on, of %u in %u files%s%s", patches->serial, patches->app_version, patches->patches.size(),
					std::count_if(patches->patches.begin(), patches->patches.end(), [](const ps5_patches::patch& p) { return p.enabled; }), patches->total, patches->files,
					patches->broken.empty() ? "" : " (errors in " + fmt::merge(patches->broken, ", ") + ")", patches->unpacked.empty() ? "" : "; " + patches->unpacked);
				m_pt = std::move(patches);
				build_patch_rows();
				layout_patches();
				layout_hints();
			}
		}

		if (m_reload_requested.exchange(false))
		{
			m_enumeration_thread.reset();
			start_reload();
		}

		if (m_fade_animation.active)
		{
			m_fade_animation.update(timestamp_us);
		}

		// The new game's art fading in over the last one's
		{
			std::lock_guard lock(m_mutex);
			if (m_background_fading)
			{
				if (!m_background_fade_start)
				{
					m_background_fade_start = timestamp_us;
				}
				const f32 t = std::min(1.f, static_cast<f32>(timestamp_us - m_background_fade_start) / c_background_fade_us);
				m_background.fore_color.a = t * t * (3.f - 2.f * t);
				m_background.refresh();
				m_background_fading = t < 1.f;
			}
		}

		// The right stick, as it is now: from the pad handler's own copy, not
		// the pad thread's pads under their lock. A game's boot tears the pad
		// thread down while this still runs, and the Library froze the app
		// when a game was started from it (build 88, on my console)
		f32 stick_x = 0.f, stick_y = 0.f;
		if (m_tab == tab::library)
		{
			ps5_pad_handler::right_stick(stick_x, stick_y);
			stick_x = std::abs(stick_x) < 0.12f ? 0.f : stick_x;
			stick_y = std::abs(stick_y) < 0.12f ? 0.f : stick_y;
		}

		{
			// The Library's row eases toward the selection
			std::lock_guard lock(m_mutex);
			const f32 dt = m_last_update_us ? std::min(0.1f, (timestamp_us - m_last_update_us) / 1'000'000.f) : 0.f;
			m_last_update_us = timestamp_us;

			// The game's menu opens and closes over 0.45 s
			m_detail_t = std::clamp(m_detail_t + (m_detail ? dt : -dt) / 0.45f, 0.f, 1.f);

			// The selected case: the stick turns it (up to 50 degrees across,
			// 28 up and down); left alone it sways a little
			m_sway_time += dt;
			constexpr f32 deg = 3.14159265f / 180.f;
			const f32 target_yaw = stick_x * 50.f * deg + std::sin(m_sway_time * 0.7f) * 7.f * deg * (1.f - std::abs(stick_x));
			const f32 target_pitch = stick_y * 28.f * deg + std::sin(m_sway_time * 0.5f) * 2.5f * deg * (1.f - std::abs(stick_y));
			const f32 follow = 1.f - std::exp(-dt * 9.f);
			m_cover_yaw += (target_yaw - m_cover_yaw) * follow;
			const f32 flip_target = m_flipped ? 3.14159265f : 0.f;
			m_flip_angle += (flip_target - m_flip_angle) * (1.f - std::exp(-dt * 7.f));
			// A spin's turn left to make: fast at first, settling over about a second
			m_spin -= m_spin * (1.f - std::exp(-dt * 5.f));
			if (m_spin < 0.002f)
			{
				m_spin = 0.f;
			}
			m_cover_pitch += (target_pitch - m_cover_pitch) * follow;
			const f32 target = static_cast<f32>(m_selected);
			m_flow_pos += (target - m_flow_pos) * (1.f - std::exp(-dt * 11.f));
			if (std::abs(target - m_flow_pos) < 0.001f)
			{
				m_flow_pos = target;
			}

			// The Trophies tab's cards, the same way
			const f32 cards = trophy_row_target();
			m_tr_flow += (cards - m_tr_flow) * (1.f - std::exp(-dt * 11.f));
			if (std::abs(cards - m_tr_flow) < 0.001f)
			{
				m_tr_flow = cards;
			}
		}

	}

	void ps5_launcher_dialog::on_button_pressed(pad_button button_press, bool /*is_auto_repeat*/)
	{
		if (m_fade_animation.active)
		{
			return;
		}

		std::lock_guard lock(m_mutex);

		// Any button during the opening ends it
		if (intro_running())
		{
			skip_intro();
			return;
		}

		// The delete confirmation, or its result, takes every button
		if (m_deleting)
		{
			return;
		}
		if (m_confirm_delete || !m_delete_result.empty())
		{
			if (button_press == pad_button::cross && m_confirm_delete)
			{
				delete_selected();
			}
			else if (button_press == pad_button::cross || button_press == pad_button::circle)
			{
				m_confirm_delete = false;
				m_delete_result.clear();
				play_sound(sound_effect::cancel);
			}
			return;
		}

		if (m_gs_open)
		{
			handle_game_settings(button_press);
			return;
		}
		if (m_pt_open)
		{
			handle_patches(button_press);
			return;
		}
		if (m_tr_open)
		{
			// The trophy box takes L1 and R1 too, for the set before and after
			handle_trophies(button_press);
			return;
		}

		// Tabs change from anywhere at the top level, except while packages install
		if ((button_press == pad_button::L1 || button_press == pad_button::R1) && !installing())
		{
			const s32 step = button_press == pad_button::L1 ? -1 : 1;
			set_tab(static_cast<tab>(std::clamp(static_cast<s32>(m_tab) + step, 0, static_cast<s32>(tab::install))));
			return;
		}

		if (m_tab == tab::install)
		{
			handle_install(button_press);
			return;
		}

		if (m_tab == tab::trophies)
		{
			handle_trophies(button_press);
			return;
		}

		if (m_tab == tab::library)
		{
			handle_library(button_press);
			return;
		}

		if (m_tab == tab::settings)
		{
			handle_settings(button_press);
			return;
		}

		if (m_games.empty())
		{
			return;
		}

		const bool up = button_press == pad_button::dpad_up || button_press == pad_button::ls_up;
		const bool down = button_press == pad_button::dpad_down || button_press == pad_button::ls_down;
		const bool left = button_press == pad_button::dpad_left || button_press == pad_button::ls_left;
		const bool right = button_press == pad_button::dpad_right || button_press == pad_button::ls_right;

		if (button_press == pad_button::triangle)
		{
			open_game_settings();
			return;
		}
		if (button_press == pad_button::square)
		{
			ask_delete();
			return;
		}

		if (m_focus == focus::tiles)
		{
			if (left) select_game(m_selected - 1);
			else if (right) select_game(m_selected + 1);
			else if (up) set_focus(focus::play);
			else if (button_press == pad_button::cross) boot_selected();
			return;
		}

		// The buttons' row
		constexpr focus order[] = {focus::play, focus::settings, focus::patches, focus::remove};
		const s32 at = static_cast<s32>(std::find(std::begin(order), std::end(order), m_focus) - std::begin(order));

		if (left && at > 0) set_focus(order[at - 1]);
		else if (right && at + 1 < static_cast<s32>(std::size(order))) set_focus(order[at + 1]);
		else if (down || button_press == pad_button::circle) set_focus(focus::tiles);
		else if (button_press == pad_button::cross)
		{
			switch (m_focus)
			{
			case focus::play: boot_selected(); break;
			case focus::settings: open_game_settings(); break;
			case focus::patches: open_patches(); break;
			case focus::remove: ask_delete(); break;
			default: break;
			}
		}
	}

	compiled_resource ps5_launcher_dialog::get_compiled()
	{
		if (!visible)
		{
			return {};
		}

		std::lock_guard lock(m_mutex);

		const f32 intro = intro_seconds();
		const f32 content = content_seconds();

		// A step of the hero or the row: rising `rise` pixels into place
		const auto step = [&](f32 start, f32 length = 0.5f) { return ease_out(progress(content, start, length)); };

		compiled_resource result;
		result.add(m_backdrop.get_compiled());
		if (m_tab == tab::home || m_gs_open || m_pt_open)
		{
			compiled_resource art;
			if (m_background_fading)
			{
				art.add(m_background_prev.get_compiled());
			}
			if (m_background_image)
			{
				art.add(m_background.get_compiled());
			}
			add_animated(result, art, step(0.f, 0.7f));
		}
		result.add(m_wash.get_compiled());
		if (m_tab == tab::home || m_gs_open || m_pt_open)
		{
			result.add(m_fade_left.get_compiled());
		}
		result.add(m_fade_top.get_compiled());
		result.add(m_fade_bottom.get_compiled());

		if (m_gs_open)
		{
			const f32 page = ease_out(progress((m_now_us - m_gs_open_us) / 1'000'000.f, 0.f, 0.3f));
			compiled_resource items;
			for (const auto& item : m_gs_items)
			{
				items.add(item->get_compiled());
			}
			add_animated(result, items, page, 0.f, 14.f * (1.f - page));
		}
		else if (m_pt_open)
		{
			const f32 page = ease_out(progress((m_now_us - m_pt_open_us) / 1'000'000.f, 0.f, 0.3f));
			compiled_resource items;
			for (const auto& item : m_pt_items)
			{
				items.add(item->get_compiled());
			}
			add_animated(result, items, page, 0.f, 14.f * (1.f - page));
		}
		else if (m_tab == tab::home)
		{
			const f32 welcome = step(0.05f);
			const f32 title = step(0.12f);
			const f32 chips = step(0.2f);
			const f32 buttons = step(0.28f);
			add_animated(result, m_welcome.get_compiled(), welcome, 0.f, 18.f * (1.f - welcome));
			add_animated(result, m_title.get_compiled(), title, 0.f, 18.f * (1.f - title));
			for (usz i = 0; i < m_chips.size(); i++)
			{
				compiled_resource chip;
				chip.add(m_chips[i]->get_compiled());
				chip.add(m_chip_labels[i]->get_compiled());
				add_animated(result, chip, chips, 0.f, 18.f * (1.f - chips));
			}

			if (!m_games.empty())
			{
				compiled_resource row;
				for (overlay_element* element : std::initializer_list<overlay_element*>{&m_play_button, &m_play_icon, &m_play_label, &m_settings_button,
						&m_settings_icon, &m_settings_label, &m_patches_button, &m_patches_icon, &m_patches_label, &m_delete_button, &m_delete_icon, &m_delete_label})
				{
					row.add(element->get_compiled());
				}
				add_animated(result, row, buttons, 0.f, 18.f * (1.f - buttons));
			}

			{
				const f32 header = step(0.3f);
				compiled_resource heading;
				heading.add(m_row_title.get_compiled());
				heading.add(m_row_rule.get_compiled());
				add_animated(result, heading, header);
			}

			if (m_games.empty())
			{
				add_animated(result, m_placeholder.get_compiled(), ease_out(progress(intro, c_intro_bar, 0.5f)));
			}
			else
			{
				// The tiles one after another, left to right
				for (usz i = 0; i < m_tiles.size(); i++)
				{
					const f32 tile = step(0.36f + 0.06f * i, 0.55f);
					compiled_resource part;
					if (m_first_visible + static_cast<s32>(i) == m_selected)
					{
						part.add(m_highlight.get_compiled());
					}
					part.add(m_tiles[i]->get_compiled());
					part.add(m_tile_labels[i]->get_compiled());
					add_animated(result, part, tile, 0.f, 26.f * (1.f - tile));
				}
			}
		}
		else if (m_tab == tab::library)
		{
			compile_library(result);
		}
		else if (m_tab == tab::settings)
		{
			compile_settings(result);
		}
		else if (m_tab == tab::trophies)
		{
			compile_trophies(result);
		}
		else
		{
			compile_install(result);
		}

		// The top bar comes down after the logo has arrived
		{
			const f32 bar = ease_out(progress(intro, c_intro_bar, 0.5f));
			compiled_resource top;
			top.add(m_bar_divider.get_compiled());
			for (const auto& tab_label : m_tab_labels)
			{
				top.add(tab_label->get_compiled());
			}
			top.add(m_tab_underline.get_compiled());
			top.add(m_avatar.get_compiled());
			top.add(m_avatar_letter.get_compiled());
			top.add(m_user_name.get_compiled());
			if (!ps5_title_build().empty())
			{
				top.add(m_build.get_compiled());
			}
			add_animated(result, top, bar, 0.f, -10.f * (1.f - bar));

			compiled_resource hints;
			for (const auto& entry : m_hints)
			{
				hints.add(entry->icon.get_compiled());
				hints.add(entry->text.get_compiled());
			}
			add_animated(result, hints, std::min(bar, step(0.6f)));
		}

		// The splash: black over everything until it makes way, the white
		// screen over that, and the logo, which ends as the top bar's own
		if (intro < c_intro_move + c_intro_move_length + 0.1f)
		{
			overlay_element cover;
			cover.set_size(virtual_width, virtual_height);
			cover.back_color = color4f(0.f, 0.f, 0.f, 1.f);
			add_animated(result, cover.get_compiled(), 1.f - ease_in_out(progress(intro, c_intro_reveal, 0.6f)));

			if (intro < c_intro_white_out + 0.6f)
			{
				compile_intro_white(result, intro);
			}

			const f32 print_alpha = 1.f - progress(intro, c_intro_move - 0.1f, 0.25f);
			const f32 move = ease_in_out(progress(intro, c_intro_move, c_intro_move_length));
			const f32 appear = ease_out(progress(intro, c_intro_logo_in, 0.6f));

			if (m_logo_data && appear > 0.f)
			{
				// From 2.5 times its size in the middle (growing a little as it
				// fades in) to its place in the bar
				const f32 big = 2.4f + 0.1f * appear;
				const f32 scale = big + (1.f - big) * move;
				const f32 w = m_logo.w * scale;
				const f32 h = m_logo.h * scale;
				const f32 from_x = (virtual_width - m_logo.w * big) / 2.f;
				const f32 from_y = (virtual_height - m_logo.h * big) / 2.f - 20.f;
				image_view logo;
				logo.set_raw_image(m_logo_data.get());
				logo.back_color.a = 0.f;
				logo.set_pos(static_cast<s16>(std::lround(from_x + (m_logo.x - from_x) * move)), static_cast<s16>(std::lround(from_y + (m_logo.y - from_y) * move)));
				logo.set_size(static_cast<u16>(std::lround(w)), static_cast<u16>(std::lround(h)));
				add_animated(result, logo.get_compiled(), appear);

				// The fine print under it, as a PS1's licence line
				place(m_intro_print, m_intro_print.x, from_y + m_logo.h * big + 46.f);
				add_animated(result, m_intro_print.get_compiled(), print_alpha * ease_out(progress(intro, c_intro_print, 0.5f)));
			}
		}
		else
		{
			result.add(m_logo_data ? m_logo.get_compiled() : m_logo_text.get_compiled());
		}

		if (m_confirm_delete || m_deleting || !m_delete_result.empty())
		{
			result.add(m_confirm_dim.get_compiled());
			result.add(m_confirm_panel.get_compiled());
			result.add(m_confirm_title.get_compiled());
			result.add(m_confirm_body.get_compiled());
			if (!m_deleting)
			{
				for (hint* entry : {&m_confirm_yes, &m_confirm_no})
				{
					result.add(entry->icon.get_compiled());
					result.add(entry->text.get_compiled());
				}
			}
		}

		m_fade_animation.apply(result);
		return result;
	}

	void ps5_launcher_dialog::show()
	{
		// The first opening has its splash instead of the quick fade
		m_fade_animation.current = color4f(0.f);
		m_fade_animation.end = color4f(1.f);
		m_fade_animation.active = !m_play_intro;

		visible = true;

		auto& overlayman = g_fxo->get<display_manager>();
		overlayman.attach_thread_input(uid, "PS5 Launcher");
	}

	void open_ps5_launcher()
	{
		auto& overlayman = g_fxo->get<display_manager>();
		const auto dialog = overlayman.create<ps5_launcher_dialog>();
		dialog->show();

		// Both ways out (a game's boot, or leaving) tear the shell down from the
		// main thread, and this dialog with it: wait for that, as Big Picture
		// Mode's own dialog does
		while (!Emu.IsStopped())
		{
			thread_ctrl::wait_for(50'000);
		}
	}
}
