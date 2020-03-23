#include "stdafx.h"
#include "cellGem.h"
#include "cellCamera.h"

#include "Emu/Cell/PPUModule.h"
#include "Emu/Io/MouseHandler.h"
#include "Emu/RSX/GSRender.h"
#include "Utilities/Timer.h"
#include "Input/pad_thread.h"

LOG_CHANNEL(cellGem);

template <>
void fmt_class_string<CellGemError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_GEM_ERROR_RESOURCE_ALLOCATION_FAILED);
			STR_CASE(CELL_GEM_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_GEM_ERROR_UNINITIALIZED);
			STR_CASE(CELL_GEM_ERROR_INVALID_PARAMETER);
			STR_CASE(CELL_GEM_ERROR_INVALID_ALIGNMENT);
			STR_CASE(CELL_GEM_ERROR_UPDATE_NOT_FINISHED);
			STR_CASE(CELL_GEM_ERROR_UPDATE_NOT_STARTED);
			STR_CASE(CELL_GEM_ERROR_CONVERT_NOT_FINISHED);
			STR_CASE(CELL_GEM_ERROR_CONVERT_NOT_STARTED);
			STR_CASE(CELL_GEM_ERROR_WRITE_NOT_FINISHED);
			STR_CASE(CELL_GEM_ERROR_NOT_A_HUE);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellGemStatus>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_GEM_NOT_CONNECTED);
			STR_CASE(CELL_GEM_SPHERE_NOT_CALIBRATED);
			STR_CASE(CELL_GEM_SPHERE_CALIBRATING);
			STR_CASE(CELL_GEM_COMPUTING_AVAILABLE_COLORS);
			STR_CASE(CELL_GEM_HUE_NOT_SET);
			STR_CASE(CELL_GEM_NO_VIDEO);
			STR_CASE(CELL_GEM_TIME_OUT_OF_RANGE);
			STR_CASE(CELL_GEM_NOT_CALIBRATED);
			STR_CASE(CELL_GEM_NO_EXTERNAL_PORT_DEVICE);
		}

		return unknown;
	});
}

// **********************
// * HLE helper structs *
// **********************

struct gem_config
{
	atomic_t<u32> state = 0;

	struct gem_color
	{
		float r, g, b;

		gem_color() : r(0.0f), g(0.0f), b(0.0f) {}
		gem_color(float r_, float g_, float b_)
		{
			r = std::clamp(r_, 0.0f, 1.0f);
			g = std::clamp(g_, 0.0f, 1.0f);
			b = std::clamp(b_, 0.0f, 1.0f);
		}
	};

	struct gem_controller
	{
		u32 status = CELL_GEM_STATUS_DISCONNECTED;         // Connection status (CELL_GEM_STATUS_DISCONNECTED or CELL_GEM_STATUS_READY)
		u32 ext_status = CELL_GEM_NO_EXTERNAL_PORT_DEVICE; // External port connection status
		u32 port = 0;                                      // Assigned port
		bool enabled_magnetometer = false;                 // Whether the magnetometer is enabled (probably used for additional rotational precision)
		bool calibrated_magnetometer = false;              // Whether the magnetometer is calibrated
		bool enabled_filtering = false;                    // Whether filtering is enabled
		bool enabled_tracking = false;                     // Whether tracking is enabled
		bool enabled_LED = false;                          // Whether the LED is enabled
		u8 rumble = 0;                                     // Rumble intensity
		gem_color sphere_rgb = {};                         // RGB color of the sphere LED
		u32 hue = 0;                                       // Tracking hue of the motion controller
	};

	CellGemAttribute attribute = {};
	CellGemVideoConvertAttribute vc_attribute = {};
	u64 status_flags = CELL_GEM_NOT_CALIBRATED;
	bool enable_pitch_correction = false;
	u32 inertial_counter = 0;

	std::array<gem_controller, CELL_GEM_MAX_NUM> controllers;
	u32 connected_controllers = 0;
	bool update_started{};
	u32 camera_frame{};
	u32 memory_ptr{};

	shared_mutex mtx;

	Timer timer;

	// helper functions
	bool is_controller_ready(u32 gem_num) const
	{
		return controllers[gem_num].status == CELL_GEM_STATUS_READY;
	}

	void reset_controller(u32 gem_num)
	{
		switch (g_cfg.io.move)
		{
		case move_handler::fake:
		case move_handler::mouse:
		{
			connected_controllers = 1;
			break;
		}
		default: break;
		}

		// Assign status and port number
		if (gem_num < connected_controllers)
		{
			controllers[gem_num].status = CELL_GEM_STATUS_READY;
			controllers[gem_num].port = 7u - gem_num;
		}
	}
};

// ************************
// * HLE helper functions *
// ************************

template <>
void fmt_class_string<move_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case move_handler::null: return "Null";
		case move_handler::fake: return "Fake";
		case move_handler::mouse: return "Mouse";
		}

		return unknown;
	});
}

/**
 * \brief Verifies that a Move controller id is valid
 * \param gem_num Move controler ID to verify
 * \return True if the ID is valid, false otherwise
 */
static bool check_gem_num(const u32 gem_num)
{
	return gem_num >= 0 && gem_num < CELL_GEM_MAX_NUM;
}

/**
 * \brief Maps Move controller data (digital buttons, and analog Trigger data) to DS3 pad input.
 *        Unavoidably buttons conflict with DS3 mappings, which is problematic for some games.
 * \param port_no DS3 port number to use
 * \param digital_buttons Bitmask filled with CELL_GEM_CTRL_* values
 * \param analog_t Analog value of Move's Trigger. Currently mapped to R2.
 * \return true on success, false if port_no controller is invalid
 */
static bool ds3_input_to_pad(const u32 port_no, be_t<u16>& digital_buttons, be_t<u16>& analog_t)
{
	std::scoped_lock lock(pad::g_pad_mutex);

	const auto handler = pad::get_current_handler();

	const PadInfo& rinfo = handler->GetInfo();

	auto& pads = handler->GetPads();
	auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return false;

	for (Button& button : pad->m_buttons)
	{
		// here we check btns, and set pad accordingly
		if (button.m_offset == CELL_PAD_BTN_OFFSET_DIGITAL2)
		{
			if (button.m_pressed) pad->m_digital_2 |= button.m_outKeyCode;
			else pad->m_digital_2 &= ~button.m_outKeyCode;

			switch (button.m_outKeyCode)
			{
			case CELL_PAD_CTRL_SQUARE:
				pad->m_press_square = button.m_value;
				break;
			case CELL_PAD_CTRL_CROSS:
				pad->m_press_cross = button.m_value;
				break;
			case CELL_PAD_CTRL_CIRCLE:
				pad->m_press_circle = button.m_value;
				break;
			case CELL_PAD_CTRL_TRIANGLE:
				pad->m_press_triangle = button.m_value;
				break;
			case CELL_PAD_CTRL_R1:
				pad->m_press_R1 = button.m_value;
				break;
			case CELL_PAD_CTRL_L1:
				pad->m_press_L1 = button.m_value;
				break;
			case CELL_PAD_CTRL_R2:
				pad->m_press_R2 = button.m_value;
				break;
			case CELL_PAD_CTRL_L2:
				pad->m_press_L2 = button.m_value;
				break;
			default: break;
			}
		}
	}

	digital_buttons = 0;

	// map the Move key to R1 and the Trigger to R2

	if (pad->m_press_R1)
		digital_buttons |= CELL_GEM_CTRL_MOVE;
	if (pad->m_press_R2)
		digital_buttons |= CELL_GEM_CTRL_T;

	if (pad->m_press_cross)
		digital_buttons |= CELL_GEM_CTRL_CROSS;
	if (pad->m_press_circle)
		digital_buttons |= CELL_GEM_CTRL_CIRCLE;
	if (pad->m_press_square)
		digital_buttons |= CELL_GEM_CTRL_SQUARE;
	if (pad->m_press_triangle)
		digital_buttons |= CELL_GEM_CTRL_TRIANGLE;
	if (pad->m_digital_1)
		digital_buttons |= CELL_GEM_CTRL_SELECT;
	if (pad->m_digital_2)
		digital_buttons |= CELL_GEM_CTRL_START;

	analog_t = pad->m_press_R2;

	return true;
}

/**
 * \brief Maps external Move controller data to DS3 input
 *        Implementation detail: CellGemExtPortData's digital/analog fields map the same way as
 *        libPad, so no translation is needed.
 * \param port_no DS3 port number to use
 * \param ext External data to modify
 * \return true on success, false if port_no controller is invalid
 */
static bool ds3_input_to_ext(const u32 port_no, CellGemExtPortData& ext)
{
	std::scoped_lock lock(pad::g_pad_mutex);

	const auto handler = pad::get_current_handler();

	auto& pads = handler->GetPads();

	const PadInfo& rinfo = handler->GetInfo();

	auto pad = pads[port_no];

	if (!(pad->m_port_status & CELL_PAD_STATUS_CONNECTED))
		return false;

	ext.status = 0; // CELL_GEM_EXT_CONNECTED | CELL_GEM_EXT_EXT0 | CELL_GEM_EXT_EXT1
	ext.analog_left_x = pad->m_analog_left_x;
	ext.analog_left_y = pad->m_analog_left_y;
	ext.analog_right_x = pad->m_analog_right_x;
	ext.analog_right_y = pad->m_analog_right_y;
	ext.digital1 = pad->m_digital_1;
	ext.digital2 = pad->m_digital_2;

	return true;
}

/**
 * \brief Maps Move controller data (digital buttons, and analog Trigger data) to mouse input.
 *        Move Button: Mouse1
 *        Trigger:     Mouse2
 * \param mouse_no Mouse index number to use
 * \param digital_buttons Bitmask filled with CELL_GEM_CTRL_* values
 * \param analog_t Analog value of Move's Trigger.
 * \return true on success, false if mouse_no is invalid
 */
static bool mouse_input_to_pad(const u32 mouse_no, be_t<u16>& digital_buttons, be_t<u16>& analog_t)
{
	const auto handler = g_fxo->get<MouseHandlerBase>();

	std::scoped_lock lock(handler->mutex);

	if (!handler || mouse_no >= handler->GetMice().size())
	{
		return false;
	}

	const auto& mouse_data = handler->GetMice().at(0);

	digital_buttons = 0;

	if ((mouse_data.buttons & CELL_MOUSE_BUTTON_1) && (mouse_data.buttons & CELL_MOUSE_BUTTON_2))
		digital_buttons |= CELL_GEM_CTRL_CIRCLE;
	if (mouse_data.buttons & CELL_MOUSE_BUTTON_3)
		digital_buttons |= CELL_GEM_CTRL_CROSS;
	if (mouse_data.buttons & CELL_MOUSE_BUTTON_2)
		digital_buttons |= CELL_GEM_CTRL_MOVE;
	if ((mouse_data.buttons & CELL_MOUSE_BUTTON_1) && (mouse_data.buttons & CELL_MOUSE_BUTTON_3))
		digital_buttons |= CELL_GEM_CTRL_START;
	if (mouse_data.buttons & CELL_MOUSE_BUTTON_1)
		digital_buttons |= CELL_GEM_CTRL_T;
	if ((mouse_data.buttons & CELL_MOUSE_BUTTON_2) && (mouse_data.buttons & CELL_MOUSE_BUTTON_3))
		digital_buttons |= CELL_GEM_CTRL_TRIANGLE;

	analog_t = (mouse_data.buttons & CELL_MOUSE_BUTTON_1) ? 0xFFFF : 0;

	return true;
}

static bool mouse_pos_to_gem_image_state(const u32 mouse_no, vm::ptr<CellGemImageState>& gem_image_state)
{
	const auto handler = g_fxo->get<MouseHandlerBase>();

	std::scoped_lock lock(handler->mutex);

	if (!gem_image_state || !handler || mouse_no >= handler->GetMice().size())
	{
		return false;
	}

	const auto& mouse = handler->GetMice().at(0);

	const auto renderer = static_cast<GSRender*>(rsx::get_current_renderer());
	const auto width = renderer->get_frame()->client_width();
	const auto hight = renderer->get_frame()->client_height();
	const f32 scaling_width = width / 640.f;
	const f32 scaling_hight = hight / 480.f;

	const f32 x = static_cast<f32>(mouse.x_pos) / scaling_width;
	const f32 y = static_cast<f32>(mouse.y_pos) / scaling_hight;

	gem_image_state->u = 133.f + (x / 1.50f);
	gem_image_state->v = 160.f + (y / 1.67f);
	gem_image_state->projectionx = x - 320.f;
	gem_image_state->projectiony = 240.f - y;

	return true;
}

static bool mouse_pos_to_gem_state(const u32 mouse_no, vm::ptr<CellGemState>& gem_state)
{
	const auto handler = g_fxo->get<MouseHandlerBase>();

	std::scoped_lock lock(handler->mutex);

	if (!gem_state || !handler || mouse_no >= handler->GetMice().size())
	{
		return false;
	}

	const auto& mouse = handler->GetMice().at(0);

	const auto renderer = static_cast<GSRender*>(rsx::get_current_renderer());
	const auto width = renderer->get_frame()->client_width();
	const auto hight = renderer->get_frame()->client_height();

	const f32 scaling_width = width / 640.f;
	const f32 scaling_hight = hight / 480.f;
	const f32 x = static_cast<f32>(mouse.x_pos) / scaling_width;
	const f32 y = static_cast<f32>(mouse.y_pos) / scaling_hight;

	gem_state->pos[0] = x;
	gem_state->pos[1] = -y;
	gem_state->pos[2] = 1500.f;
	gem_state->pos[3] = 0.f;

	gem_state->quat[0] = 320.f - x;
	gem_state->quat[1] = (mouse.y_pos / scaling_width) - 180.f;
	gem_state->quat[2] = 1200.f;

	gem_state->handle_pos[0] = x;
	gem_state->handle_pos[1] = y;
	gem_state->handle_pos[2] = 1500.f;
	gem_state->handle_pos[3] = 0.f;

	return true;
}

// *********************
// * cellGem functions *
// *********************

error_code cellGemCalibrate(u32 gem_num)
{
	cellGem.todo("cellGemCalibrate(gem_num=%d)", gem_num);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (g_cfg.io.move == move_handler::fake || g_cfg.io.move == move_handler::mouse)
	{
		gem->controllers[gem_num].calibrated_magnetometer = true;
		gem->controllers[gem_num].enabled_tracking = true;
		gem->controllers[gem_num].hue = 1;
		gem->status_flags = CELL_GEM_FLAG_CALIBRATION_OCCURRED | CELL_GEM_FLAG_CALIBRATION_SUCCEEDED;
	}

	return CELL_OK;
}

error_code cellGemClearStatusFlags(u32 gem_num, u64 mask)
{
	cellGem.todo("cellGemClearStatusFlags(gem_num=%d, mask=0x%x)", gem_num, mask);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	gem->status_flags &= ~mask;

	return CELL_OK;
}

error_code cellGemConvertVideoFinish()
{
	cellGem.todo("cellGemConvertVideoFinish()");

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	return CELL_OK;
}

error_code cellGemConvertVideoStart(vm::cptr<void> video_frame)
{
	cellGem.todo("cellGemConvertVideoStart(video_frame=*0x%x)", video_frame);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	return CELL_OK;
}

error_code cellGemEnableCameraPitchAngleCorrection(u32 enable_flag)
{
	cellGem.todo("cellGemEnableCameraPitchAngleCorrection(enable_flag=%d)", enable_flag);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	gem->enable_pitch_correction = !!enable_flag;

	return CELL_OK;
}

error_code cellGemEnableMagnetometer(u32 gem_num, u32 enable)
{
	cellGem.todo("cellGemEnableMagnetometer(gem_num=%d, enable=0x%x)", gem_num, enable);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!gem->is_controller_ready(gem_num))
	{
		return CELL_GEM_NOT_CONNECTED;
	}

	gem->controllers[gem_num].enabled_magnetometer = !!enable;

	return CELL_OK;
}

error_code cellGemEnableMagnetometer2()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

error_code cellGemEnd()
{
	cellGem.warning("cellGemEnd()");

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (gem->state.compare_and_swap_test(1, 0))
	{
		if (u32 addr = gem->memory_ptr)
		{
			sys_memory_free(addr);
		}

		return CELL_OK;
	}

	return CELL_GEM_ERROR_UNINITIALIZED;
}

error_code cellGemFilterState(u32 gem_num, u32 enable)
{
	cellGem.warning("cellGemFilterState(gem_num=%d, enable=%d)", gem_num, enable);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	gem->controllers[gem_num].enabled_filtering = !!enable;

	return CELL_OK;
}

error_code cellGemForceRGB(u32 gem_num, float r, float g, float b)
{
	cellGem.todo("cellGemForceRGB(gem_num=%d, r=%f, g=%f, b=%f)", gem_num, r, g, b);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	gem->controllers[gem_num].sphere_rgb = gem_config::gem_color(r, g, b);

	return CELL_OK;
}

error_code cellGemGetAccelerometerPositionInDevice()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

error_code cellGemGetAllTrackableHues(vm::ptr<u8> hues)
{
	cellGem.todo("cellGemGetAllTrackableHues(hues=*0x%x)");

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	for (u32 i = 0; i < 360; i++)
	{
		hues[i] = true;
	}

	return CELL_OK;
}

error_code cellGemGetCameraState(vm::ptr<CellGemCameraState> camera_state)
{
	cellGem.todo("cellGemGetCameraState(camera_state=0x%x)", camera_state);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!camera_state)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	camera_state->exposure_time = 1.0f / 60.0f; // TODO: use correct framerate
	camera_state->gain = 1.0;

	return CELL_OK;
}

error_code cellGemGetEnvironmentLightingColor(vm::ptr<f32> r, vm::ptr<f32> g, vm::ptr<f32> b)
{
	cellGem.todo("cellGemGetEnvironmentLightingColor(r=*0x%x, g=*0x%x, b=*0x%x)", r, g, b);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!r || !g || !b)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	// default to 128
	*r = 128;
	*g = 128;
	*b = 128;

	return CELL_OK;
}

error_code cellGemGetHuePixels(vm::cptr<void> camera_frame, u32 hue, vm::ptr<u8> pixels)
{
	cellGem.todo("cellGemGetHuePixels(camera_frame=*0x%x, hue=%d, pixels=*0x%x)", camera_frame, hue, pixels);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!camera_frame || !pixels || hue > 359)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	return CELL_OK;
}

error_code cellGemGetImageState(u32 gem_num, vm::ptr<CellGemImageState> gem_image_state)
{
	cellGem.todo("cellGemGetImageState(gem_num=%d, image_state=&0x%x)", gem_num, gem_image_state);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !gem_image_state)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	auto shared_data = g_fxo->get<gem_camera_shared>();

	if (g_cfg.io.move == move_handler::fake)
	{
		gem_image_state->u = 0;
		gem_image_state->v = 0;
		gem_image_state->projectionx = 1;
		gem_image_state->projectiony = 1;
	}
	else if (g_cfg.io.move == move_handler::mouse)
	{
		mouse_pos_to_gem_image_state(gem_num, gem_image_state);
	}

	if (g_cfg.io.move == move_handler::fake || g_cfg.io.move == move_handler::mouse)
	{
		gem_image_state->frame_timestamp = shared_data->frame_timestamp.load();
		gem_image_state->timestamp = gem_image_state->frame_timestamp + 10;
		gem_image_state->r = 10;
		gem_image_state->distance = 2 * 1000; // 2 meters away from camera
		gem_image_state->visible = true;
		gem_image_state->r_valid = true;
	}

	return CELL_OK;
}

error_code cellGemGetInertialState(u32 gem_num, u32 state_flag, u64 timestamp, vm::ptr<CellGemInertialState> inertial_state)
{
	cellGem.warning("cellGemGetInertialState(gem_num=%d, state_flag=%d, timestamp=0x%x, inertial_state=0x%x)", gem_num, state_flag, timestamp, inertial_state);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || state_flag > CELL_GEM_INERTIAL_STATE_FLAG_NEXT || !inertial_state || !gem->is_controller_ready(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (g_cfg.io.move == move_handler::fake)
	{
		ds3_input_to_pad(gem_num, inertial_state->pad.digitalbuttons, inertial_state->pad.analog_T);
	}
	else if (g_cfg.io.move == move_handler::mouse)
	{
		mouse_input_to_pad(gem_num, inertial_state->pad.digitalbuttons, inertial_state->pad.analog_T);
	}

	if (g_cfg.io.move == move_handler::fake || g_cfg.io.move == move_handler::mouse)
	{
		ds3_input_to_ext(gem_num, inertial_state->ext);

		inertial_state->timestamp = gem->timer.GetElapsedTimeInMicroSec();
		inertial_state->counter = gem->inertial_counter++;
		inertial_state->accelerometer[0] = 10;
	}

	return CELL_OK;
}

error_code cellGemGetInfo(vm::ptr<CellGemInfo> info)
{
	cellGem.warning("cellGemGetInfo(info=*0x%x)", info);

	const auto gem = g_fxo->get<gem_config>();

	std::shared_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!info)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	// TODO: Support connecting PlayStation Move controllers
	info->max_connect = gem->attribute.max_connect;
	info->now_connect = gem->connected_controllers;

	for (int i = 0; i < CELL_GEM_MAX_NUM; i++)
	{
		info->status[i] = gem->controllers[i].status;
		info->port[i] = gem->controllers[i].port;
	}

	return CELL_OK;
}

u32 GemGetMemorySize(s32 max_connect)
{
	return max_connect <= 2 ? 0x120000 : 0x140000;
}

error_code cellGemGetMemorySize(s32 max_connect)
{
	cellGem.warning("cellGemGetMemorySize(max_connect=%d)", max_connect);

	if (max_connect > CELL_GEM_MAX_NUM || max_connect <= 0)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	return not_an_error(GemGetMemorySize(max_connect));
}

error_code cellGemGetRGB(u32 gem_num, vm::ptr<float> r, vm::ptr<float> g, vm::ptr<float> b)
{
	cellGem.todo("cellGemGetRGB(gem_num=%d, r=*0x%x, g=*0x%x, b=*0x%x)", gem_num, r, g, b);

	const auto gem = g_fxo->get<gem_config>();

	std::shared_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !r || !g || !b)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	auto& sphere_color = gem->controllers[gem_num].sphere_rgb;
	*r = sphere_color.r;
	*g = sphere_color.g;
	*b = sphere_color.b;

	return CELL_OK;
}

error_code cellGemGetRumble(u32 gem_num, vm::ptr<u8> rumble)
{
	cellGem.todo("cellGemGetRumble(gem_num=%d, rumble=*0x%x)", gem_num, rumble);

	const auto gem = g_fxo->get<gem_config>();

	std::shared_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !rumble)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	*rumble = gem->controllers[gem_num].rumble;

	return CELL_OK;
}

error_code cellGemGetState(u32 gem_num, u32 flag, u64 time_parameter, vm::ptr<CellGemState> gem_state)
{
	cellGem.warning("cellGemGetState(gem_num=%d, flag=0x%x, time=0x%llx, gem_state=*0x%x)", gem_num, flag, time_parameter, gem_state);

	const auto gem = g_fxo->get<gem_config>();

	std::shared_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || flag > CELL_GEM_STATE_FLAG_TIMESTAMP || !gem_state)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (g_cfg.io.move == move_handler::fake)
	{
		ds3_input_to_pad(gem_num, gem_state->pad.digitalbuttons, gem_state->pad.analog_T);
	}
	else if (g_cfg.io.move == move_handler::mouse)
	{
		mouse_input_to_pad(gem_num, gem_state->pad.digitalbuttons, gem_state->pad.analog_T);
		mouse_pos_to_gem_state(gem_num, gem_state);
	}

	if (g_cfg.io.move == move_handler::fake || g_cfg.io.move == move_handler::mouse)
	{
		ds3_input_to_ext(gem_num, gem_state->ext);

		gem_state->tracking_flags = CELL_GEM_TRACKING_FLAG_POSITION_TRACKED | CELL_GEM_TRACKING_FLAG_VISIBLE;
		gem_state->timestamp = gem->timer.GetElapsedTimeInMicroSec();
		gem_state->quat[3] = 1.f;

		return CELL_OK;
	}

	return CELL_GEM_NOT_CONNECTED;
}

error_code cellGemGetStatusFlags(u32 gem_num, vm::ptr<u64> flags)
{
	cellGem.todo("cellGemGetStatusFlags(gem_num=%d, flags=*0x%x)", gem_num, flags);

	const auto gem = g_fxo->get<gem_config>();

	std::shared_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !flags)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	*flags = gem->status_flags;

	return CELL_OK;
}

error_code cellGemGetTrackerHue(u32 gem_num, vm::ptr<u32> hue)
{
	cellGem.warning("cellGemGetTrackerHue(gem_num=%d, hue=*0x%x)", gem_num, hue);

	const auto gem = g_fxo->get<gem_config>();

	std::shared_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !hue)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (!gem->controllers[gem_num].enabled_tracking || gem->controllers[gem_num].hue > 359)
	{
		return CELL_GEM_ERROR_NOT_A_HUE;
	}

	*hue = gem->controllers[gem_num].hue;

	return CELL_OK;
}

error_code cellGemHSVtoRGB(f32 h, f32 s, f32 v, vm::ptr<f32> r, vm::ptr<f32> g, vm::ptr<f32> b)
{
	cellGem.todo("cellGemHSVtoRGB(h=%f, s=%f, v=%f, r=*0x%x, g=*0x%x, b=*0x%x)", h, s, v, r, g, b);

	if (s < 0.0f || s > 1.0f || v < 0.0f || v > 1.0f || !r || !g || !b)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	h = std::clamp(h, 0.0f, 360.0f);

	// TODO: convert

	return CELL_OK;
}

error_code cellGemInit(vm::cptr<CellGemAttribute> attribute)
{
	cellGem.warning("cellGemInit(attribute=*0x%x)", attribute);

	const auto gem = g_fxo->get<gem_config>();

	if (!attribute || !attribute->spurs_addr || !attribute->max_connect || attribute->max_connect > CELL_GEM_MAX_NUM)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	std::scoped_lock lock(gem->mtx);

	if (!gem->state.compare_and_swap_test(0, 1))
	{
		return CELL_GEM_ERROR_ALREADY_INITIALIZED;
	}

	if (!attribute->memory_ptr)
	{
		vm::var<u32> addr(0);

		// Decrease memory stats
		if (sys_memory_allocate(GemGetMemorySize(attribute->max_connect), SYS_MEMORY_PAGE_SIZE_64K, +addr) != CELL_OK)
		{
			return CELL_GEM_ERROR_RESOURCE_ALLOCATION_FAILED;
		}

		gem->memory_ptr = *addr;
	}
	else
	{
		gem->memory_ptr = 0;
	}

	gem->update_started = false;
	gem->camera_frame = 0;
	gem->status_flags = 0;
	gem->attribute = *attribute;

	if (g_cfg.io.move == move_handler::mouse)
	{
		// init mouse handler
		const auto handler = g_fxo->get<MouseHandlerBase>();

		handler->Init(std::min<u32>(attribute->max_connect, CELL_GEM_MAX_NUM));
	}

	for (int gem_num = 0; gem_num < CELL_GEM_MAX_NUM; gem_num++)
	{
		gem->reset_controller(gem_num);
	}

	// TODO: is this correct?
	gem->timer.Start();

	return CELL_OK;
}

error_code cellGemInvalidateCalibration(s32 gem_num)
{
	cellGem.todo("cellGemInvalidateCalibration(gem_num=%d)", gem_num);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (g_cfg.io.move == move_handler::fake || g_cfg.io.move == move_handler::mouse)
	{
		gem->controllers[gem_num].calibrated_magnetometer = false;
		// TODO: gem->status_flags
	}

	return CELL_OK;
}

s32 cellGemIsTrackableHue(u32 hue)
{
	cellGem.todo("cellGemIsTrackableHue(hue=%d)", hue);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state || hue > 359)
	{
		return false;
	}

	return true;
}

error_code cellGemPrepareCamera(s32 max_exposure, f32 image_quality)
{
	cellGem.todo("cellGemPrepareCamera(max_exposure=%d, image_quality=%f)", max_exposure, image_quality);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	max_exposure = std::clamp(max_exposure, static_cast<s32>(CELL_GEM_MIN_CAMERA_EXPOSURE), static_cast<s32>(CELL_GEM_MAX_CAMERA_EXPOSURE));
	image_quality = std::clamp(image_quality, 0.0f, 1.0f);

	// TODO: prepare camera

	return CELL_OK;
}

error_code cellGemPrepareVideoConvert(vm::cptr<CellGemVideoConvertAttribute> vc_attribute)
{
	cellGem.todo("cellGemPrepareVideoConvert(vc_attribute=*0x%x)", vc_attribute);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!vc_attribute)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	const auto vc = *vc_attribute;

	if (!vc_attribute || vc.version == 0 || vc.output_format == 0 ||
		(vc.conversion_flags & CELL_GEM_COMBINE_PREVIOUS_INPUT_FRAME && !vc.buffer_memory))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (vc.video_data_out & 0x1f || vc.buffer_memory & 0xff)
	{
		return CELL_GEM_ERROR_INVALID_ALIGNMENT;
	}

	gem->vc_attribute = vc;

	return CELL_OK;
}

error_code cellGemReadExternalPortDeviceInfo(u32 gem_num, vm::ptr<u32> ext_id, vm::ptr<u8[CELL_GEM_EXTERNAL_PORT_DEVICE_INFO_SIZE]> ext_info)
{
	cellGem.todo("cellGemReadExternalPortDeviceInfo(gem_num=%d, ext_id=*0x%x, ext_info=%s)", gem_num, ext_id, ext_info);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num) || !ext_id)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	if (gem->controllers[gem_num].status & CELL_GEM_STATUS_DISCONNECTED)
	{
		return CELL_GEM_NOT_CONNECTED;
	}

	if (!(gem->controllers[gem_num].ext_status & CELL_GEM_EXT_CONNECTED))
	{
		return CELL_GEM_NO_EXTERNAL_PORT_DEVICE;
	}

	return CELL_OK;
}

error_code cellGemReset(u32 gem_num)
{
	cellGem.todo("cellGemReset(gem_num=%d)", gem_num);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	gem->reset_controller(gem_num);

	// TODO: is this correct?
	gem->timer.Start();

	return CELL_OK;
}

error_code cellGemSetRumble(u32 gem_num, u8 rumble)
{
	cellGem.todo("cellGemSetRumble(gem_num=%d, rumble=0x%x)", gem_num, rumble);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	gem->controllers[gem_num].rumble = rumble;

	return CELL_OK;
}

error_code cellGemSetYaw()
{
	UNIMPLEMENTED_FUNC(cellGem);
	return CELL_OK;
}

error_code cellGemTrackHues(vm::cptr<u32> req_hues, vm::ptr<u32> res_hues)
{
	cellGem.todo("cellGemTrackHues(req_hues=*0x%x, res_hues=*0x%x)", req_hues, res_hues);

	const auto gem = g_fxo->get<gem_config>();

	std::scoped_lock lock(gem->mtx);

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!req_hues)
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	for (u32 i = 0; i < CELL_GEM_MAX_NUM; i++)
	{
		if (req_hues[i] == u32{CELL_GEM_DONT_CARE_HUE})
		{

		}
		else if (req_hues[i] == u32{CELL_GEM_DONT_TRACK_HUE})
		{
			gem->controllers[i].enabled_tracking = false;
			gem->controllers[i].enabled_LED = false;
		}
		else
		{
			if (req_hues[i] > 359)
			{
				cellGem.warning("cellGemTrackHues: req_hues[%d]=%d -> this can lead to unexpected behavior", i, req_hues[i]);
			}
		}
	}

	return CELL_OK;
}

error_code cellGemUpdateFinish()
{
	cellGem.warning("cellGemUpdateFinish()");

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	std::scoped_lock lock(gem->mtx);

	if (!std::exchange(gem->update_started, false))
	{
		return CELL_GEM_ERROR_UPDATE_NOT_STARTED;
	}

	if (!gem->camera_frame)
	{
		return not_an_error(CELL_GEM_NO_VIDEO);
	}

	return CELL_OK;
}

error_code cellGemUpdateStart(vm::cptr<void> camera_frame, u64 timestamp)
{
	cellGem.warning("cellGemUpdateStart(camera_frame=*0x%x, timestamp=%d)", camera_frame, timestamp);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	std::scoped_lock lock(gem->mtx);

	// Update is starting even when camera_frame is null
	if (std::exchange(gem->update_started, true))
	{
		return CELL_GEM_ERROR_UPDATE_NOT_FINISHED;
	}

	gem->camera_frame = camera_frame.addr();
	if (!camera_frame)
	{
		return not_an_error(CELL_GEM_NO_VIDEO);
	}

	return CELL_OK;
}

error_code cellGemWriteExternalPort(u32 gem_num, vm::ptr<u8[CELL_GEM_EXTERNAL_PORT_OUTPUT_SIZE]> data)
{
	cellGem.todo("cellGemWriteExternalPort(gem_num=%d, data=%s)", gem_num, data);

	const auto gem = g_fxo->get<gem_config>();

	if (!gem->state)
	{
		return CELL_GEM_ERROR_UNINITIALIZED;
	}

	if (!check_gem_num(gem_num))
	{
		return CELL_GEM_ERROR_INVALID_PARAMETER;
	}

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellGem)("libgem", []()
{
	REG_FUNC(libgem, cellGemCalibrate);
	REG_FUNC(libgem, cellGemClearStatusFlags);
	REG_FUNC(libgem, cellGemConvertVideoFinish);
	REG_FUNC(libgem, cellGemConvertVideoStart);
	REG_FUNC(libgem, cellGemEnableCameraPitchAngleCorrection);
	REG_FUNC(libgem, cellGemEnableMagnetometer);
	REG_FUNC(libgem, cellGemEnableMagnetometer2);
	REG_FUNC(libgem, cellGemEnd);
	REG_FUNC(libgem, cellGemFilterState);
	REG_FUNC(libgem, cellGemForceRGB);
	REG_FUNC(libgem, cellGemGetAccelerometerPositionInDevice);
	REG_FUNC(libgem, cellGemGetAllTrackableHues);
	REG_FUNC(libgem, cellGemGetCameraState);
	REG_FUNC(libgem, cellGemGetEnvironmentLightingColor);
	REG_FUNC(libgem, cellGemGetHuePixels);
	REG_FUNC(libgem, cellGemGetImageState);
	REG_FUNC(libgem, cellGemGetInertialState);
	REG_FUNC(libgem, cellGemGetInfo);
	REG_FUNC(libgem, cellGemGetMemorySize);
	REG_FUNC(libgem, cellGemGetRGB);
	REG_FUNC(libgem, cellGemGetRumble);
	REG_FUNC(libgem, cellGemGetState);
	REG_FUNC(libgem, cellGemGetStatusFlags);
	REG_FUNC(libgem, cellGemGetTrackerHue);
	REG_FUNC(libgem, cellGemHSVtoRGB);
	REG_FUNC(libgem, cellGemInit);
	REG_FUNC(libgem, cellGemInvalidateCalibration);
	REG_FUNC(libgem, cellGemIsTrackableHue);
	REG_FUNC(libgem, cellGemPrepareCamera);
	REG_FUNC(libgem, cellGemPrepareVideoConvert);
	REG_FUNC(libgem, cellGemReadExternalPortDeviceInfo);
	REG_FUNC(libgem, cellGemReset);
	REG_FUNC(libgem, cellGemSetRumble);
	REG_FUNC(libgem, cellGemSetYaw);
	REG_FUNC(libgem, cellGemTrackHues);
	REG_FUNC(libgem, cellGemUpdateFinish);
	REG_FUNC(libgem, cellGemUpdateStart);
	REG_FUNC(libgem, cellGemWriteExternalPort);
});
