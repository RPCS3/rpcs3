#include "Input/product_info.h"

#include <map>

#include "Emu/Io/pad_types.h"

namespace input
{
	static const std::map<product_type, product_info> input_products = {
	{
		product_type::playstation_3_controller,
		{
			.type = product_type::playstation_3_controller,
			.class_id = CELL_PAD_PCLASS_TYPE_STANDARD,
			.vendor_id = vendor_id::sony_corp,
			.product_id = product_id::playstation_3_controller,
			.pclass_profile = 0x0,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::red_octane_gh_guitar,
		{
			.type = product_type::red_octane_gh_guitar,
			.class_id = CELL_PAD_PCLASS_TYPE_GUITAR,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::red_octane_gh_guitar,
			.pclass_profile =
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_1 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_2 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_3 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_4 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_FRET_5 |
				CELL_PAD_PCLASS_PROFILE_GUITAR_STRUM_UP |
				CELL_PAD_PCLASS_PROFILE_GUITAR_STRUM_DOWN |
				CELL_PAD_PCLASS_PROFILE_GUITAR_WHAMMYBAR,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::harmonix_rockband_guitar,
		{
			.type = product_type::harmonix_rockband_guitar,
			.class_id = CELL_PAD_PCLASS_TYPE_GUITAR,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::harmonix_rockband_guitar,
			.pclass_profile =
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
				CELL_PAD_PCLASS_PROFILE_GUITAR_TILT_SENS,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::red_octane_gh_drum_kit,
		{
			.type = product_type::red_octane_gh_drum_kit,
			.class_id = CELL_PAD_PCLASS_TYPE_DRUM,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::red_octane_gh_drum_kit,
			.pclass_profile =
				CELL_PAD_PCLASS_PROFILE_DRUM_SNARE |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM_FLOOR |
				CELL_PAD_PCLASS_PROFILE_DRUM_KICK |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_HiHAT |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_RIDE,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::harmonix_rockband_drum_kit,
		{
			.type = product_type::harmonix_rockband_drum_kit,
			.class_id = CELL_PAD_PCLASS_TYPE_DRUM,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::harmonix_rockband_drum_kit,
			.pclass_profile = 
				CELL_PAD_PCLASS_PROFILE_DRUM_SNARE |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM2 |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM_FLOOR |
				CELL_PAD_PCLASS_PROFILE_DRUM_KICK |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_HiHAT |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_CRASH |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_RIDE,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::harmonix_rockband_drum_kit_2,
		{
			.type = product_type::harmonix_rockband_drum_kit_2,
			.class_id = CELL_PAD_PCLASS_TYPE_DRUM,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::harmonix_rockband_drum_kit_2,
			.pclass_profile =
				CELL_PAD_PCLASS_PROFILE_DRUM_SNARE |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM2 |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM_FLOOR |
				CELL_PAD_PCLASS_PROFILE_DRUM_KICK |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_HiHAT |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_RIDE,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::rock_revolution_drum_kit,
		{
			.type = product_type::rock_revolution_drum_kit,
			.class_id = CELL_PAD_PCLASS_TYPE_DRUM,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::rock_revolution_drum_kit,
			.pclass_profile =
				CELL_PAD_PCLASS_PROFILE_DRUM_SNARE |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM |
				CELL_PAD_PCLASS_PROFILE_DRUM_TOM_FLOOR |
				CELL_PAD_PCLASS_PROFILE_DRUM_KICK |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_HiHAT |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_CRASH |
				CELL_PAD_PCLASS_PROFILE_DRUM_CYM_RIDE,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::dj_hero_turntable,
		{
			.type = product_type::dj_hero_turntable,
			.class_id = CELL_PAD_PCLASS_TYPE_DJ,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::dj_hero_turntable,
			.pclass_profile =
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
				CELL_PAD_PCLASS_PROFILE_DJ_DECK2_PLATTER,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::dance_dance_revolution_mat,
		{
			.type = product_type::dance_dance_revolution_mat,
			.class_id = CELL_PAD_PCLASS_TYPE_DANCEMAT,
			.vendor_id = vendor_id::konami_de,
			.product_id = product_id::dance_dance_revolution_mat,
			.pclass_profile =
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_CIRCLE |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_CROSS |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_TRIANGLE |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_SQUARE |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_RIGHT |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_LEFT |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_UP |
				CELL_PAD_PCLASS_PROFILE_DANCEMAT_DOWN,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::ps_move_navigation,
		{
			.type = product_type::ps_move_navigation,
			.class_id = CELL_PAD_PCLASS_TYPE_NAVIGATION,
			.vendor_id = vendor_id::sony_corp,
			.product_id = product_id::ps_move_navigation,
			.pclass_profile = 0x0,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_HP_ANALOG_STICK | CELL_PAD_CAPABILITY_ACTUATOR | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::ride_skateboard,
		{
			.type = product_type::ride_skateboard,
			.class_id = CELL_PAD_PCLASS_TYPE_SKATEBOARD,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::ride_skateboard,
			.pclass_profile = 0x0,
			.capabilites = CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE | CELL_PAD_CAPABILITY_SENSOR_MODE
		}
	},
	{
		product_type::guncon_3,
		{
			.type = product_type::guncon_3,
			.class_id = CELL_PAD_FAKE_TYPE_GUNCON3,
			.vendor_id = vendor_id::namco,
			.product_id = product_id::guncon_3,
			.pclass_profile = 0x0,
			.capabilites = 0x0
		}
	},
	{
		product_type::top_shot_elite,
		{
			.type = product_type::top_shot_elite,
			.class_id = CELL_PAD_FAKE_TYPE_TOP_SHOT_ELITE,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::top_shot_elite,
			.pclass_profile = 0x0,
			.capabilites = 0x0
		}
	},
	{
		product_type::top_shot_fearmaster,
		{
			.type = product_type::top_shot_fearmaster,
			.class_id = CELL_PAD_FAKE_TYPE_TOP_SHOT_FEARMASTER,
			.vendor_id = vendor_id::sony_cea,
			.product_id = product_id::top_shot_fearmaster,
			.pclass_profile = 0x0,
			.capabilites = 0x0
		}
	},
	{
		product_type::udraw_gametablet,
		{
			.type = product_type::udraw_gametablet,
			.class_id = CELL_PAD_FAKE_TYPE_GAMETABLET,
			.vendor_id = vendor_id::bda,
			.product_id = product_id::udraw_gametablet,
			.pclass_profile = 0x0,
			.capabilites = 0x0
		}
	}
	};

	product_info get_product_info(product_type type)
	{
		return input_products.at(type);
	}

	product_type get_product_by_vid_pid(u16 vendor_id, u16 product_id)
	{
		for (const auto& [type, product] : input_products)
		{
			if (product.vendor_id == vendor_id && product.product_id == product_id)
				return type;
		}
		return product_type::playstation_3_controller;
	}

	std::vector<product_info> get_products_by_class(int class_id)
	{
		if (class_id >= CELL_PAD_FAKE_TYPE_COPILOT_1 && class_id <= CELL_PAD_FAKE_TYPE_COPILOT_7)
		{
			class_id = CELL_PAD_PCLASS_TYPE_STANDARD;
		}

		std::vector<product_info> ret;
		for (const auto& [type, product] : input_products)
		{
			if (product.class_id == class_id)
				ret.push_back(product);
		}
		return ret;
	}
}
