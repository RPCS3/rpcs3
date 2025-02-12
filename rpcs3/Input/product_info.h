#pragma once

#include <vector>
#include "util/types.hpp"

namespace input
{
	enum class product_type
	{
		playstation_3_controller,
		red_octane_gh_guitar,
		red_octane_gh_drum_kit,
		dance_dance_revolution_mat,
		dj_hero_turntable,
		harmonix_rockband_guitar,
		harmonix_rockband_drum_kit,
		harmonix_rockband_drum_kit_2,
		rock_revolution_drum_kit,
		ps_move_navigation,
		ride_skateboard,
		guncon_3,
		top_shot_elite,
		top_shot_fearmaster,
		udraw_gametablet,
	};

	enum vendor_id
	{
		sony_corp = 0x054C, // Sony Corp.
		namco     = 0x0B9A, // Namco
		sony_cea  = 0x12BA, // Sony Computer Entertainment America
		konami_de = 0x1CCF, // Konami Digital Entertainment
		bda       = 0x20D6, // Bensussen Deutsch & Associates
	};

	enum product_id
	{
		red_octane_gh_guitar         = 0x0100, // RedOctane Guitar (Guitar Hero 4 Guitar Controller)
		red_octane_gh_drum_kit       = 0x0120, // RedOctane Drum Kit (Guitar Hero 4 Drum Controller)
		dj_hero_turntable            = 0x0140, // DJ Hero Turntable Controller
		harmonix_rockband_guitar     = 0x0200, // Harmonix Guitar (Rock Band II Guitar Controller)
		harmonix_rockband_drum_kit   = 0x0210, // Harmonix Drum Kit (Rock Band II Drum Controller)
		harmonix_rockband_drum_kit_2 = 0x0218, // Harmonix Pro-Drum Kit (Rock Band III Pro-Drum Controller)
		playstation_3_controller     = 0x0268, // PlayStation 3 Controller
		rock_revolution_drum_kit     = 0x0300, // Rock Revolution Drum Controller
		ps_move_navigation           = 0x042F, // PlayStation Move navigation controller
		dance_dance_revolution_mat   = 0x1010, // Dance Dance Revolution Dance Mat Controller
		ride_skateboard              = 0x0400, // Tony Hawk RIDE Skateboard Controller
		top_shot_elite               = 0x04A0, // Top Shot Elite Controller
		top_shot_fearmaster          = 0x04A1, // Top Shot Fearmaster Controller
		guncon_3                     = 0x0800, // GunCon 3 Controller
		udraw_gametablet             = 0xCB17, // uDraw GameTablet Controller
	};

	struct product_info
	{
		product_type type;
		u16 class_id;
		u16 vendor_id;
		u16 product_id;
		u32 pclass_profile; // See CELL_PAD_PCLASS_PROFILE flags
		u32 capabilites; // See CELL_PAD_CAPABILITY flags
	};

	product_info get_product_info(product_type type);
	product_type get_product_by_vid_pid(u16 vid, u16 pid);
	std::vector<product_info> get_products_by_class(int class_id);
}
