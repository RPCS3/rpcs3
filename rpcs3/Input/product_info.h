#pragma once

#include <vector>
#include "Emu/Io/pad_types.h"

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
	};

	enum vendor_id
	{
		sony_corp = 0x054C, // Sony Corp.
		namco     = 0x0B9A, // Namco
		sony_cea  = 0x12BA, // Sony Computer Entertainment America
		konami_de = 0x1CCF, // Konami Digital Entertainment
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
		guncon_3                     = 0x0800, // GunCon 3 Controller
	};

	struct product_info
	{
		product_type type;
		u16 vendor_id;
		u16 product_id;
		u32 pclass_profile; // See CELL_PAD_PCLASS_PROFILE flags
		u32 capabilites; // See CELL_PAD_CAPABILITY flags
	};

	inline product_info get_product_info(product_type type)
	{
		switch (type)
		{
		case product_type::dance_dance_revolution_mat:
		{
			constexpr u32 profile =
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_CIRCLE |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_CROSS |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_TRIANGLE |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_SQUARE |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_RIGHT |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_LEFT |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_UP |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_DOWN;
			return product_info{
				.type = type,
				.vendor_id = vendor_id::konami_de,
				.product_id = product_id::dance_dance_revolution_mat,
				.pclass_profile = profile,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::dj_hero_turntable:
		{
			constexpr u32 profile =
				CELL_PAD_PCLASS_PROFILE_DJ_MIXER_ATTACK |
				CELL_PAD_PCLASS_PROFILE_DJ_MIXER_CROSSFADER |
				CELL_PAD_PCLASS_PROFILE_DJ_MIXER_DSP_DIAL |
				CELL_PAD_PCLASS_PROFILE_DJ_DECK1_STREAM1 |
				CELL_PAD_PCLASS_PROFILE_DJ_DECK1_STREAM2 |
				CELL_PAD_PCLASS_PROFILE_DJ_DECK1_STREAM3 |
				CELL_PAD_PCLASS_PROFILE_DJ_DECK1_PLATTER |
				CELL_PAD_PCLASS_PROFILE_DJ_DECK2_STREAM1 |
				CELL_PAD_PCLASS_PROFILE_DJ_DECK2_STREAM2 |
				CELL_PAD_PCLASS_PROFILE_DJ_DECK2_STREAM3 |
				CELL_PAD_PCLASS_PROFILE_DJ_DECK2_PLATTER;
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_cea,
				.product_id = product_id::dj_hero_turntable,
				.pclass_profile = profile,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::harmonix_rockband_drum_kit:
		{
			constexpr u32 profile =
				CELL_PAD_PCLASS_PROFILE_DRUM_SNARE |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM2 |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM_FLOOR |
				CELL_PAD_PCLASS_PROFILE_DRUM_KICK |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_HiHAT |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_CRASH |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_RIDE;
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_cea,
				.product_id = product_id::harmonix_rockband_drum_kit,
				.pclass_profile = profile,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::harmonix_rockband_drum_kit_2:
		{
			constexpr u32 profile =
				CELL_PAD_PCLASS_PROFILE_DRUM_SNARE |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM2 |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM_FLOOR |
				CELL_PAD_PCLASS_PROFILE_DRUM_KICK |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_HiHAT |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_RIDE;
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_cea,
				.product_id = product_id::harmonix_rockband_drum_kit_2,
				.pclass_profile = profile,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::harmonix_rockband_guitar:
		{
			constexpr u32 profile =
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_1 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_2 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_3 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_4 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_5 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_STRUM_UP |
				CELL_PAD_PCLASS_PROFILE_GUITAR_STRUM_DOWN |
				CELL_PAD_PCLASS_PROFILE_GUITAR_WHAMMYBAR |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H1 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H2 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H3 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H4 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_H5 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_5WAY_EFFECT |
				CELL_PAD_PCLASS_PROFILE_GUITAR_TILT_SENS;
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_cea,
				.product_id = product_id::harmonix_rockband_guitar,
				.pclass_profile = profile,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::red_octane_gh_drum_kit:
		{
			constexpr u32 profile =
				CELL_PAD_PCLASS_PROFILE_DRUM_SNARE |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM_FLOOR |
				CELL_PAD_PCLASS_PROFILE_DRUM_KICK |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_HiHAT |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_RIDE;
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_cea,
				.product_id = product_id::red_octane_gh_drum_kit,
				.pclass_profile = profile,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::red_octane_gh_guitar:
		{
			constexpr u32 profile =
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_1 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_2 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_3 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_4 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_5 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_STRUM_UP |
				CELL_PAD_PCLASS_PROFILE_GUITAR_STRUM_DOWN |
				CELL_PAD_PCLASS_PROFILE_GUITAR_WHAMMYBAR;
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_cea,
				.product_id = product_id::red_octane_gh_guitar,
				.pclass_profile = profile,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::rock_revolution_drum_kit:
		{
			constexpr u32 profile =
				CELL_PAD_PCLASS_PROFILE_DRUM_SNARE |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM_FLOOR |
				CELL_PAD_PCLASS_PROFILE_DRUM_KICK |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_HiHAT |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_CRASH |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_RIDE;
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_cea,
				.product_id = product_id::rock_revolution_drum_kit,
				.pclass_profile = profile,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::ps_move_navigation:
		{
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_corp,
				.product_id = product_id::ps_move_navigation,
				.pclass_profile = 0x0,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::ride_skateboard:
		{
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_cea,
				.product_id = product_id::ride_skateboard,
				.pclass_profile = 0x0,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		case product_type::guncon_3:
		{
			return product_info{
				.type = type,
				.vendor_id = vendor_id::namco,
				.product_id = product_id::guncon_3,
				.pclass_profile = 0x0,
				.capabilites = 0x0
			};
		}
		case product_type::playstation_3_controller:
		default: // GCC doesn't like it when there is no return value if if all enum values are covered
		{
			return product_info{
				.type = type,
				.vendor_id = vendor_id::sony_corp,
				.product_id = product_id::playstation_3_controller,
				.pclass_profile = 0x0,
				.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
			};
		}
		}
	}

	inline product_type get_product_by_vid_pid(u16 vid, u16 pid)
	{
		if (vid == vendor_id::konami_de && pid == product_id::dance_dance_revolution_mat)
			return product_type::dance_dance_revolution_mat;
		if (vid == vendor_id::sony_cea && pid == product_id::dj_hero_turntable)
			return product_type::dj_hero_turntable;
		if (vid == vendor_id::sony_cea && pid == product_id::harmonix_rockband_drum_kit)
			return product_type::harmonix_rockband_drum_kit;
		if (vid == vendor_id::sony_cea && pid == product_id::harmonix_rockband_drum_kit_2)
			return product_type::harmonix_rockband_drum_kit_2;
		if (vid == vendor_id::sony_cea && pid == product_id::harmonix_rockband_guitar)
			return product_type::harmonix_rockband_guitar;
		if (vid == vendor_id::sony_cea && pid == product_id::red_octane_gh_drum_kit)
			return product_type::red_octane_gh_drum_kit;
		if (vid == vendor_id::sony_cea && pid == product_id::red_octane_gh_guitar)
			return product_type::red_octane_gh_guitar;
		if (vid == vendor_id::sony_cea && pid == product_id::rock_revolution_drum_kit)
			return product_type::rock_revolution_drum_kit;
		if (vid == vendor_id::sony_corp && pid == product_id::ps_move_navigation)
			return product_type::ps_move_navigation;
		if (vid == vendor_id::sony_cea && pid == product_id::ride_skateboard)
			return product_type::ride_skateboard;
		if (vid == vendor_id::namco && pid == product_id::guncon_3)
			return product_type::guncon_3;
		if (vid == vendor_id::sony_corp && pid == product_id::playstation_3_controller)
			return product_type::playstation_3_controller;
		return product_type::playstation_3_controller;
	}

	inline std::vector<product_info> get_products_by_class(int class_id)
	{
		switch (class_id)
		{
		default:
		case CELL_PAD_PCLASS_TYPE_STANDARD:
		{
			return
			{
				get_product_info(product_type::playstation_3_controller)
			};
		}
		case CELL_PAD_PCLASS_TYPE_GUITAR:
		{
			return
			{
				get_product_info(product_type::red_octane_gh_guitar),
				get_product_info(product_type::harmonix_rockband_guitar)
			};
		}
		case CELL_PAD_PCLASS_TYPE_DRUM:
		{
			return
			{
				get_product_info(product_type::red_octane_gh_drum_kit),
				get_product_info(product_type::harmonix_rockband_drum_kit),
				get_product_info(product_type::harmonix_rockband_drum_kit_2),
				get_product_info(product_type::rock_revolution_drum_kit)
			};
		}
		case CELL_PAD_PCLASS_TYPE_DJ:
		{
			return
			{
				get_product_info(product_type::dj_hero_turntable)
			};
		}
		case CELL_PAD_PCLASS_TYPE_DANCEMAT:
		{
			return
			{
				get_product_info(product_type::dance_dance_revolution_mat)
			};
		}
		case CELL_PAD_PCLASS_TYPE_NAVIGATION:
		{
			return
			{
				get_product_info(product_type::ps_move_navigation)
			};
		}
		case CELL_PAD_PCLASS_TYPE_SKATEBOARD:
		{
			return
			{
				get_product_info(product_type::ride_skateboard)
			};
		}
		case CELL_PAD_FAKE_TYPE_GUNCON3:
		{
			return
			{
				get_product_info(product_type::guncon_3)
			};
		}
		}
	}
};
