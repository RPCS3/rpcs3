#pragma once

// PS5: what the launcher (ps5_launcher.cpp) and the pause menu
// (ps5_pause_menu.cpp) share: the design's palette, fonts and text, its
// curves, and the settings the desktop's dialog offers, with how a game's
// own are read and written.

#include "Emu/RSX/Overlays/overlays.h"
#include "rpcs3qt/emu_settings_type.h" // which config entry each of the desktop dialog's settings is; no Qt

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct cfg_root;

namespace cfg
{
	class _base;
	class node;
}

namespace rsx::overlays::ps5ui
{
	// The design's palette
	inline const color4f c_text{1.f, 1.f, 1.f, 1.f};
	inline const color4f c_text_dim{1.f, 1.f, 1.f, 0.68f};
	inline const color4f c_accent{0.40f, 0.87f, 0.95f, 1.f};
	inline const color4f c_button{0.96f, 0.93f, 0.87f, 1.f};
	inline const color4f c_button_text{0.06f, 0.06f, 0.08f, 1.f};
	inline const color4f c_glass{0.02f, 0.03f, 0.07f, 0.42f};
	inline const color4f c_glass_border{1.f, 1.f, 1.f, 0.22f};
	inline const color4f c_backdrop{0.02f, 0.03f, 0.08f, 1.f};
	inline const color4f c_library_blue{0.003f, 0.006f, 0.135f, 1.f}; // #010222, a shade darker than the first #01022F

	// A trophy grade's colour: 0 platinum, 1 gold, 2 silver, 3 bronze, else unknown
	const color4f& trophy_grade_colour(u8 grade);

	// The title's fonts (/app0/assets/fonts, the frontend's font folder);
	// characters they lack come from the PS3's own font
	inline constexpr std::string_view f_regular = "Inter-Regular";
	inline constexpr std::string_view f_medium = "Inter-Medium";
	inline constexpr std::string_view f_semibold = "Inter-SemiBold";
	inline constexpr std::string_view f_bold = "Inter-Bold";

	// An image with rounded corners: the overlays' rounded-box SDF, which the
	// shader applies to sampled images as to flat colour
	struct rounded_image : public image_view
	{
		u16 border_radius = 0;

		compiled_resource& get_compiled() override
		{
			if (is_compiled())
			{
				return compiled_resources;
			}

			image_view::get_compiled();
			if (!compiled_resources.draw_commands.empty())
			{
				auto& config = compiled_resources.draw_commands.front().config;
				configure_sdf(config, sdf_function::rounded_box);
				config.sdf_config.br = std::min({static_cast<f32>(border_radius), config.sdf_config.hx, config.sdf_config.hy});
			}
			return compiled_resources;
		}
	};

	void style_label(label& target, std::string_view text, u16 font_size, std::string_view font_name, const color4f& color);
	std::unique_ptr<label> make_label(std::string_view text, u16 font_size, std::string_view font_name, const color4f& color);

	// The extents of what a text draws, from its baseline at 0 (y up is negative)
	struct ink
	{
		f32 left = 0.f, right = 0.f, top = 0.f, bottom = 0.f;
	};

	ink measure_ink(font* renderer, std::u32string_view text);

	// A label's top for its capitals to sit centred on mid_y
	s16 cap_centred_y(const label& target, f32 mid_y);
	void place(label& target, s16 x, f32 mid_y);

	// Shortened with an ellipsis to fit max_w
	void fit_text(label& target, std::string_view text, u16 max_w);

	// "WELCOME BACK" with the design's wide letter spacing
	std::string spaced(std::string_view text);

	std::unique_ptr<image_info> load_image(const std::string& path);

	// A soft white disc, its alpha falling to 0 at its edge: tinted and
	// stretched, a glow
	std::unique_ptr<memory_image_info> make_glow(std::vector<u8>& pixels);

	// The deep blue's texture (the Library, Trophies, Settings and Install
	// tabs): the PS3's wave, a sheet of fine lines and a soft ribbon under it
	// drifting apart so its shape keeps changing, and a fine grain
	struct backdrop_texture
	{
		std::vector<u8> lines_pixels;
		std::vector<u8> ribbon_pixels;
		std::vector<u8> grain_pixels;
		std::unique_ptr<memory_image_info> lines;
		std::unique_ptr<memory_image_info> ribbon;
		std::unique_ptr<memory_image_info> grain;

		void build();

		// Over the blue, at `seconds` into the page's life
		void add(compiled_resource& out, f32 seconds, f32 alpha = 1.f) const;
	};

	// A five-pointed star, white on clear, its edges smoothed: a favourite's
	std::unique_ptr<memory_image_info> make_star(std::vector<u8>& pixels);

	// The curves: how far a step is, at `now` seconds, for a step from `start` lasting `length`
	f32 progress(f32 now, f32 start, f32 length);
	f32 ease_out(f32 t);
	f32 ease_in_out(f32 t);

	// Part of the screen drawn faded and moved
	void add_animated(compiled_resource& out, const compiled_resource& part, f32 alpha, f32 dx = 0.f, f32 dy = 0.f);

	// Every setting the desktop's settings dialog offers, by its tabs, with
	// the tooltips it shows there (ps5_settings_table.inc, written from
	// rpcs3qt by settings_table.py). The Settings tab shows them a tab at a
	// time, a game's settings all of them, and the pause menu a tab each
	struct settings_tab
	{
		const char* name;
		const char* icon;
		const char* note;
	};

	inline constexpr settings_tab c_settings_tabs[]
	{
		{"CPU", "home/32/gauge-solid.png", "How the PS3's processors are run."},
		{"GPU", "home/32/display-solid.png", "How games are drawn."},
		{"Audio", "home/32/headphones-solid.png", "How games sound."},
		{"I/O", "home/32/gamepad-solid.png", "Controllers, as games see them."},
		{"System", "home/32/settings.png", "The PS3 the games see."},
		{"Network", "home/32/user-group-solid.png", "The PS3's network, as games see it."},
		{"Advanced", "home/32/sliders-solid.png", "Accuracy and timing, for the games that need them."},
		{"Emulator", "home/32/maximize-solid.png", "Overlays, notices and compiling."},
		{"Debug", "home/32/bug-solid.png", "For tracking down a fault. Most of these slow games down."},
	};

	struct settings_entry
	{
		const char* tab;
		emu_settings_type type;
		const char* label;
		const char* help;
	};

	std::span<const settings_entry> settings_table();

	// A setting as a page shows it: where it is in the config, its words, the
	// values to choose from, the value it would have without this page (the
	// global config's, or the default) and the one it has; or a tab's heading
	struct setting_row
	{
		std::string section;
		std::string key;
		std::string label;
		std::string help;
		std::vector<std::string> options;
		std::string global;
		std::string value;
		bool heading = false;
	};

	// An entry's section in the config ("Video/Performance Overlay") and name
	std::pair<std::string, std::string> setting_path(emu_settings_type type);

	// A setting's values to choose from
	std::vector<std::string> setting_options(const cfg::_base& setting, std::string_view key);

	// A setting by its section ("Video", or "Video/Performance Overlay") and name
	cfg::_base* find_setting(cfg::node& root, std::string_view section, std::string_view key);

	// A value as a page shows it ("On", "100%")
	std::string setting_text(const std::string& key, const std::string& value);

	// The global config, as a game's boot starts from: the defaults, then config.yml
	std::unique_ptr<cfg_root> load_global_config();

	// The entries of `config` that differ from `base`, as YAML
	bool write_differences(std::string& out, const cfg::node& config, const cfg::node& base, int depth);

	// A setting's default as this title has it
	std::string usable_default(const cfg::_base& setting, const std::vector<std::string>& options);

	// A game's settings, every one the desktop dialog offers (with a heading
	// for each tab, if `headings`): the global config's value and what the
	// game gets, the global config with its custom config over it
	std::vector<setting_row> read_game_settings(const std::string& serial, bool headings);

	// The game's custom config (config/custom_configs/config_<serial>.yml):
	// what differs from the global config, among these settings and any the
	// file held already; removed when nothing differs
	void write_game_settings(const std::string& serial, const std::vector<setting_row>& rows);
}
