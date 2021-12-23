#ifdef _WIN32

#include "direct_input_pad_handler.h"
#include "direct_input_constants.h"
#include <tchar.h>

LOG_CHANNEL(dinput_log, "DirectInput");

#define SAFE_RELEASE(p) { if (p) { (p)->Release(); (p) = nullptr; } }

LPDIRECTINPUT8 g_pDI = nullptr;

static constexpr size_t di_btn_count = 32;
static constexpr int di_axis_min = -32768;
static constexpr int di_axis_max = 32767;
static constexpr int di_threshold_01 = 100; // 1%

static std::string to_wstring(const WCHAR wstr[MAX_PATH])
{
	char ch[MAX_PATH]{};
	constexpr char def_char = ' ';
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, ch, MAX_PATH, &def_char, NULL);
	return ch;
}

static std::string get_device_name(const LPDIRECTINPUTDEVICE8 device, const GUID& type)
{
	if (!device)
	{
		dinput_log.error("Failed to get device name: device is null");
		return {};
	}

	DIPROPSTRING di_str{};
	di_str.diph.dwSize = sizeof(di_str);
	di_str.diph.dwHeaderSize = sizeof(di_str.diph);
	di_str.diph.dwHow = DIPH_DEVICE;

	if (const HRESULT hr = device->GetProperty(type, &di_str.diph); hr < 0)
	{
		dinput_log.error("Driver failed to get device name: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return {};
	}

	return to_wstring(di_str.wsz);
}

static std::string get_guid_and_path(const LPDIRECTINPUTDEVICE8 device)
{
	if (!device)
	{
		dinput_log.error("Failed to get GUID and path: device is null");
		return {};
	}

	DIPROPGUIDANDPATH guid_and_path{};
	guid_and_path.diph.dwSize = sizeof(guid_and_path);
	guid_and_path.diph.dwHeaderSize = sizeof(guid_and_path.diph);
	guid_and_path.diph.dwObj = 0;
	guid_and_path.diph.dwHow = DIPH_DEVICE;

	if (const HRESULT hr = device->GetProperty(DIPROP_GUIDANDPATH, &guid_and_path.diph); hr < 0)
	{
		dinput_log.error("Driver failed to get GUID and path: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return {};
	}

	return to_wstring(guid_and_path.wszPath);
}

direct_input_pad_handler::direct_input_pad_handler() : PadHandlerBase(pad_handler::direct_input)
{
	button_list = input_names;

	init_configs();

	// Define border values
	thumb_max = di_axis_max;
	trigger_min = 0;
	trigger_max = di_axis_max;

	// Set capabilities
	b_has_config = true;
	b_has_deadzones = true;

	m_trigger_threshold = trigger_max / 2;
	m_thumb_threshold = thumb_max / 2;
}

direct_input_pad_handler::~direct_input_pad_handler()
{
	for (pad_ensemble& binding : m_bindings)
	{
		if (DirectInputDevice* device = static_cast<DirectInputDevice*>(binding.device.get()))
		{
			if (device->device)
			{
				device->device->Unacquire();
			}

			SAFE_RELEASE(device->device);
		}
	}

	SAFE_RELEASE(g_pDI);
}

void direct_input_pad_handler::init_config(cfg_pad* cfg)
{
	if (!cfg) return;

	// Set default button mapping (based on xbox one controller)
	cfg->ls_left.def  = button_list.at(DInputKeyCodes::AXIS_X_NEG);
	cfg->ls_down.def  = button_list.at(DInputKeyCodes::AXIS_Y_POS);
	cfg->ls_right.def = button_list.at(DInputKeyCodes::AXIS_X_POS);
	cfg->ls_up.def    = button_list.at(DInputKeyCodes::AXIS_Y_NEG);
	cfg->rs_left.def  = button_list.at(DInputKeyCodes::AXIS_X_ROT_NEG);
	cfg->rs_down.def  = button_list.at(DInputKeyCodes::AXIS_Y_ROT_POS);
	cfg->rs_right.def = button_list.at(DInputKeyCodes::AXIS_X_ROT_POS);
	cfg->rs_up.def    = button_list.at(DInputKeyCodes::AXIS_Y_ROT_NEG);
	cfg->start.def    = button_list.at(DInputKeyCodes::BUTTON_8);
	cfg->select.def   = button_list.at(DInputKeyCodes::BUTTON_7);
	cfg->ps.def       = button_list.at(DInputKeyCodes::BUTTON_12);
	cfg->square.def   = button_list.at(DInputKeyCodes::BUTTON_3);
	cfg->cross.def    = button_list.at(DInputKeyCodes::BUTTON_1);
	cfg->circle.def   = button_list.at(DInputKeyCodes::BUTTON_2);
	cfg->triangle.def = button_list.at(DInputKeyCodes::BUTTON_4);
	cfg->left.def     = button_list.at(DInputKeyCodes::POV_1_LEFT);
	cfg->down.def     = button_list.at(DInputKeyCodes::POV_1_DOWN);
	cfg->right.def    = button_list.at(DInputKeyCodes::POV_1_RIGHT);
	cfg->up.def       = button_list.at(DInputKeyCodes::POV_1_UP);
	cfg->r1.def       = button_list.at(DInputKeyCodes::BUTTON_6);
	cfg->r2.def       = button_list.at(DInputKeyCodes::AXIS_Z_NEG);
	cfg->r3.def       = button_list.at(DInputKeyCodes::BUTTON_10);
	cfg->l1.def       = button_list.at(DInputKeyCodes::BUTTON_5);
	cfg->l2.def       = button_list.at(DInputKeyCodes::AXIS_Z_POS);
	cfg->l3.def       = button_list.at(DInputKeyCodes::BUTTON_9);

	cfg->pressure_intensity_button.def = button_list.at(DInputKeyCodes::NONE);

	// Set default misc variables
	cfg->lstickdeadzone.def = di_threshold_01;
	cfg->rstickdeadzone.def = di_threshold_01;
	cfg->ltriggerthreshold.def = di_threshold_01;
	cfg->rtriggerthreshold.def = di_threshold_01;
	cfg->lpadsquircling.def = 8000;
	cfg->rpadsquircling.def = 8000;

	// apply defaults
	cfg->from_default();
}

BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext)
{
	LPDIRECTINPUTDEVICE8 device = reinterpret_cast<LPDIRECTINPUTDEVICE8>(pContext);
	if (!device || !pdidoi)
	{
		return DIENUM_CONTINUE;
	}

	// For axes that are returned, set the DIPROP_RANGE property for the
	// enumerated axis in order to scale min/max values.
	if (pdidoi->dwType & DIDFT_AXIS)
	{
		dinput_log.notice("Found axis %s", wchar_to_utf8(pdidoi->tszName));

		// Set the range for the axis
		DIPROPRANGE range{};
		range.diph.dwSize = sizeof(DIPROPRANGE);
		range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		range.diph.dwHow = DIPH_BYID;
		range.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
		range.lMin = di_axis_min;
		range.lMax = di_axis_max;

		if (const HRESULT hr = device->SetProperty(DIPROP_RANGE, &range.diph); hr < 0)
		{
			dinput_log.error("EnumObjectsCallback failed to set property DIPROP_RANGE: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
			return DIENUM_STOP;
		}

		// Set the dead zone for the axis
		DIPROPDWORD dead_zone{};
		dead_zone.dwData = di_threshold_01;
		dead_zone.diph.dwSize = sizeof(DIPROPDWORD);
		dead_zone.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dead_zone.diph.dwHow = DIPH_BYID;
		dead_zone.diph.dwObj = pdidoi->dwType;

		if (const HRESULT hr = device->SetProperty(DIPROP_DEADZONE, &dead_zone.diph); hr < 0)
		{
			dinput_log.error("EnumObjectsCallback failed to set property DIPROP_DEADZONE: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
			return DIENUM_STOP;
		}
	}

	return DIENUM_CONTINUE;
}

BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
	auto devices = reinterpret_cast<std::vector<std::shared_ptr<direct_input_pad_handler::DirectInputDevice>>*>(pContext);
	if (!devices || !pdidInstance)
	{
		return DIENUM_CONTINUE;
	}

	LPDIRECTINPUTDEVICE8 device{};

	if (const HRESULT hr = g_pDI->CreateDevice(pdidInstance->guidInstance, &device, nullptr); hr < 0)
	{
		dinput_log.error("EnumJoysticksCallback failed to create device: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return DIENUM_CONTINUE;
	}

	dinput_log.trace("tszInstanceName='%s', tszProductName='%s'", to_wstring(pdidInstance->tszInstanceName), to_wstring(pdidInstance->tszProductName));

	if (const HRESULT hr = device->SetDataFormat(&c_dfDIJoystick2); hr < 0)
	{
		dinput_log.error("EnumJoysticksCallback failed to set data format: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return DIENUM_CONTINUE;
	}

	//if (const HRESULT hr = device->SetCooperativeLevel(hDlg, DISCL_EXCLUSIVE | DISCL_FOREGROUND); hr < 0)
	//{
	//	dinput_log.error("EnumJoysticksCallback failed to set cooperative level: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
	//	return DIENUM_CONTINUE;
	//}

	if (const HRESULT hr = device->EnumObjects(EnumObjectsCallback, device, DI8DEVCLASS_ALL); hr < 0)
	{
		dinput_log.error("EnumJoysticksCallback failed to enum objects: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return DIENUM_CONTINUE;
	}

	DIDEVCAPS capabilities{};
	capabilities.dwSize = sizeof(DIDEVCAPS);
	if (const HRESULT hr = device->GetCapabilities(&capabilities); hr < 0)
	{
		dinput_log.error("EnumJoysticksCallback failed to get capabilities: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return DIENUM_CONTINUE;
	}

	auto dev = std::make_shared<direct_input_pad_handler::DirectInputDevice>();
	dev->device = device;
	dev->capabilities = capabilities;
	dev->product_name = get_device_name(device, DIPROP_PRODUCTNAME);
	dev->instance_name = get_device_name(device, DIPROP_INSTANCENAME);
	dev->guid_and_path = get_guid_and_path(device);

	dinput_log.trace("product_name='%s', instance_name='%s', guid_and_path='%s'", dev->product_name, dev->instance_name, dev->guid_and_path);

	devices->emplace_back(dev);

	return DIENUM_CONTINUE;
}

bool direct_input_pad_handler::Init()
{
	if (is_init)
	{
		return true;
	}

	m_devices.clear();

	if (const HRESULT hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID * *)& g_pDI, nullptr); hr < 0)
	{
		dinput_log.error("Failed to initialize DirectInput: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return false;
	}

	if (const HRESULT hr = g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, &m_devices, DIEDFL_ATTACHEDONLY); hr < 0)
	{
		dinput_log.error("Failed to enumerate DirectInput devices: %s (0x%08x)", std::system_category().message(hr), static_cast<u32>(hr));
		return false;
	}

	if (m_devices.empty())
	{
		dinput_log.error("No DirectInput devices found");
		return false;
	}

	dinput_log.notice("Driver supports %d devices", m_devices.size());

	is_init = true;
	return true;
}

PadHandlerBase::connection direct_input_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	DirectInputDevice* dev = static_cast<DirectInputDevice*>(device.get());
	if (!dev || !dev->device)
	{
		return connection::disconnected;
	}

	if (const HRESULT hr = dev->device->Acquire(); hr < 0)
	{
		dinput_log.trace("Acquire failed for device %d: %s (0x%08x)", device->player_id, std::system_category().message(hr), static_cast<u32>(hr));
		return connection::disconnected;
	}

	if (const HRESULT hr = dev->device->GetDeviceState(sizeof(DIJOYSTATE2), &dev->state); hr < 0)
	{
		dinput_log.trace("GetDeviceState failed for device %d: %s (0x%08x)", device->player_id, std::system_category().message(hr), static_cast<u32>(hr));
		return connection::disconnected;
	}

	return connection::connected;
}

std::unordered_map<u64, u16> direct_input_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> values;

	DirectInputDevice* dev = static_cast<DirectInputDevice*>(device.get());
	if (!dev || !dev->device)
	{
		return values;
	}

	if (const HRESULT hr = dev->device->Poll(); hr < 0)
	{
		dinput_log.error("Poll failed for device %d: %s (0x%08x)", device->player_id, std::system_category().message(hr), static_cast<u32>(hr));
		return values;
	}

	DIJOYSTATE2& state = dev->state;

	// Get POV values
	for (usz i = 0; i < 4; i++)
	{
		const usz offset = i * 4;

		values[offset + DInputKeyCodes::POV_1_UP]    = state.rgdwPOV[i] == 0 ? 255 : 0;
		values[offset + DInputKeyCodes::POV_1_RIGHT] = state.rgdwPOV[i] == 9000 ? 255 : 0;
		values[offset + DInputKeyCodes::POV_1_DOWN]  = state.rgdwPOV[i] == 18000 ? 255 : 0;
		values[offset + DInputKeyCodes::POV_1_LEFT]  = state.rgdwPOV[i] == 27000 ? 255 : 0;
	}

	// Get Button values
	for (usz i = 0; i < std::min(128ULL, di_btn_count); i++)
	{
		values[i + DInputKeyCodes::BUTTON_1] = (state.rgbButtons[i] & 0x80) ? 255 : 0;
	}

	// Get Axis values
	values[DInputKeyCodes::AXIS_X_NEG] = static_cast<u16>(state.lX < 0 ? abs(state.lX) - 1 : 0);
	values[DInputKeyCodes::AXIS_X_POS] = static_cast<u16>(state.lX > 0 ? state.lX : 0);
	values[DInputKeyCodes::AXIS_Y_NEG] = static_cast<u16>(state.lY < 0 ? abs(state.lY) - 1 : 0);
	values[DInputKeyCodes::AXIS_Y_POS] = static_cast<u16>(state.lY > 0 ? state.lY : 0);
	values[DInputKeyCodes::AXIS_Z_NEG] = static_cast<u16>(state.lZ < 0 ? abs(state.lZ) - 1 : 0);
	values[DInputKeyCodes::AXIS_Z_POS] = static_cast<u16>(state.lZ > 0 ? state.lZ : 0);

	values[DInputKeyCodes::AXIS_X_ROT_NEG] = static_cast<u16>(state.lRx < 0 ? abs(state.lRx) - 1 : 0);
	values[DInputKeyCodes::AXIS_X_ROT_POS] = static_cast<u16>(state.lRx > 0 ? state.lRx : 0);
	values[DInputKeyCodes::AXIS_Y_ROT_NEG] = static_cast<u16>(state.lRy < 0 ? abs(state.lRy) - 1 : 0);
	values[DInputKeyCodes::AXIS_Y_ROT_POS] = static_cast<u16>(state.lRy > 0 ? state.lRy : 0);
	values[DInputKeyCodes::AXIS_Z_ROT_NEG] = static_cast<u16>(state.lRz < 0 ? abs(state.lRz) - 1 : 0);
	values[DInputKeyCodes::AXIS_Z_ROT_POS] = static_cast<u16>(state.lRz > 0 ? state.lRz : 0);

	// Get Slider values
	values[DInputKeyCodes::SLIDER_1_NEG] = static_cast<u16>(state.rglSlider[0] < 0 ? abs(state.rglSlider[0]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_1_POS] = static_cast<u16>(state.rglSlider[0] > 0 ? state.rglSlider[0] : 0);
	values[DInputKeyCodes::SLIDER_2_NEG] = static_cast<u16>(state.rglSlider[1] < 0 ? abs(state.rglSlider[1]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_2_POS] = static_cast<u16>(state.rglSlider[1] > 0 ? state.rglSlider[1] : 0);

	values[DInputKeyCodes::SLIDER_1_VEL_NEG] = static_cast<u16>(state.rglVSlider[0] < 0 ? abs(state.rglVSlider[0]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_1_VEL_POS] = static_cast<u16>(state.rglVSlider[0] > 0 ? state.rglVSlider[0] : 0);
	values[DInputKeyCodes::SLIDER_2_VEL_NEG] = static_cast<u16>(state.rglVSlider[1] < 0 ? abs(state.rglVSlider[1]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_2_VEL_POS] = static_cast<u16>(state.rglVSlider[1] > 0 ? state.rglVSlider[1] : 0);

	values[DInputKeyCodes::SLIDER_1_ACC_NEG] = static_cast<u16>(state.rglASlider[0] < 0 ? abs(state.rglASlider[0]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_1_ACC_POS] = static_cast<u16>(state.rglASlider[0] > 0 ? state.rglASlider[0] : 0);
	values[DInputKeyCodes::SLIDER_2_ACC_NEG] = static_cast<u16>(state.rglASlider[1] < 0 ? abs(state.rglASlider[1]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_2_ACC_POS] = static_cast<u16>(state.rglASlider[1] > 0 ? state.rglASlider[1] : 0);

	values[DInputKeyCodes::SLIDER_1_FOR_NEG] = static_cast<u16>(state.rglFSlider[0] < 0 ? abs(state.rglFSlider[0]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_1_FOR_POS] = static_cast<u16>(state.rglFSlider[0] > 0 ? state.rglFSlider[0] : 0);
	values[DInputKeyCodes::SLIDER_2_FOR_NEG] = static_cast<u16>(state.rglFSlider[1] < 0 ? abs(state.rglFSlider[1]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_2_FOR_POS] = static_cast<u16>(state.rglFSlider[1] > 0 ? state.rglFSlider[1] : 0);

	// Debug log
	if constexpr (false)
	{
		static DIJOYSTATE2 old_state{};
		if (std::memcmp(&state, &old_state, sizeof(DIJOYSTATE2)) != 0)
		{
			old_state = state;
			dinput_log.error("Slider Val %d %d", state.rglSlider[0], state.rglSlider[1]);
			dinput_log.error("Slider Vel %d %d", state.rglVSlider[0], state.rglVSlider[1]);
			dinput_log.error("Slider Acc %d %d", state.rglASlider[0], state.rglASlider[1]);
			dinput_log.error("Slider For %d %d", state.rglFSlider[0], state.rglFSlider[1]);
			dinput_log.error("Axis Val %d %d %d", state.lX, state.lY, state.lZ);
			dinput_log.error("Axis Rot %d %d %d", state.lRx, state.lRy, state.lRz);
			dinput_log.error("Axis Vel %d %d %d", state.lVX, state.lVY, state.lVZ);
			dinput_log.error("Axis AVe %d %d %d", state.lVRx, state.lVRy, state.lVRz);
			dinput_log.error("Axis Acc %d %d %d", state.lAX, state.lAY, state.lAZ);
			dinput_log.error("Axis Acc %d %d %d", state.lARx, state.lARy, state.lARz);
			dinput_log.error("Axis For %d %d %d", state.lFX, state.lFY, state.lFZ);
			dinput_log.error("Axis Tor %d %d %d", state.lFRx, state.lFRy, state.lFRz);
			dinput_log.error("POV %d %d %d %d", state.rgdwPOV[0], state.rgdwPOV[1], state.rgdwPOV[2], state.rgdwPOV[3]);

			std::string btn_str;
			for (int i = 0; i < 128; i++)
			{
				if (i > 0) btn_str += " ";
				fmt::append(btn_str, "%d", static_cast<int>(state.rgbButtons[i]));
			}
			dinput_log.error("Buttons: %s", btn_str);
		}
	}

	return values;
}

pad_preview_values direct_input_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data, const pad_buttons& buttons)
{
	pad_preview_values preview_values{};
	preview_values[0] = data.at(FindKeyCodeByString(button_list, buttons[0]));
	preview_values[1] = data.at(FindKeyCodeByString(button_list, buttons[1]));
	preview_values[2] = data.at(FindKeyCodeByString(button_list, buttons[3])) - data.at(FindKeyCodeByString(button_list, buttons[2]));
	preview_values[3] = data.at(FindKeyCodeByString(button_list, buttons[5])) - data.at(FindKeyCodeByString(button_list, buttons[4]));
	preview_values[4] = data.at(FindKeyCodeByString(button_list, buttons[7])) - data.at(FindKeyCodeByString(button_list, buttons[6]));
	preview_values[5] = data.at(FindKeyCodeByString(button_list, buttons[9])) - data.at(FindKeyCodeByString(button_list, buttons[8]));
	return preview_values;
}

std::shared_ptr<PadDevice> direct_input_pad_handler::get_device(const std::string& device)
{
	if (!Init())
	{
		return nullptr;
	}

	for (size_t i = 0; i < m_devices.size(); ++i)
	{
		auto& dev = m_devices.at(i);
		ensure(!!dev);

		if (dev->product_name == device)
		{
			return dev;
		}
	}

	return nullptr;
}

bool direct_input_pad_handler::get_is_stick(const std::shared_ptr<PadDevice>& /*device*/, u64 keycode)
{
	switch (keycode)
	{
	case AXIS_X_NEG:
	case AXIS_X_POS:
	case AXIS_Y_NEG:
	case AXIS_Y_POS:
	case AXIS_Z_NEG:
	case AXIS_Z_POS:
	case AXIS_X_ROT_NEG:
	case AXIS_X_ROT_POS:
	case AXIS_Y_ROT_NEG:
	case AXIS_Y_ROT_POS:
	case AXIS_Z_ROT_NEG:
	case AXIS_Z_ROT_POS:
		return true;
	default:
		break;
	}

	return false;
}

bool direct_input_pad_handler::get_is_trigger(const std::shared_ptr<PadDevice>& /*device*/, u64 keycode)
{
	switch (keycode)
	{
	case SLIDER_1_NEG:
	case SLIDER_1_POS:
	case SLIDER_2_NEG:
	case SLIDER_2_POS:
	case SLIDER_1_VEL_NEG:
	case SLIDER_1_VEL_POS:
	case SLIDER_2_VEL_NEG:
	case SLIDER_2_VEL_POS:
	case SLIDER_1_ACC_NEG:
	case SLIDER_1_ACC_POS:
	case SLIDER_2_ACC_NEG:
	case SLIDER_2_ACC_POS:
	case SLIDER_1_FOR_NEG:
	case SLIDER_1_FOR_POS:
	case SLIDER_2_FOR_NEG:
	case SLIDER_2_FOR_POS:
		return true;
	default:
		break;
	}

	return false;
}

bool direct_input_pad_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	DirectInputDevice* dev = static_cast<DirectInputDevice*>(device.get());
	return dev && dev->trigger_code_left == keyCode;
}

bool direct_input_pad_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	DirectInputDevice* dev = static_cast<DirectInputDevice*>(device.get());
	return dev && dev->trigger_code_right == keyCode;
}

bool direct_input_pad_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	DirectInputDevice* dev = static_cast<DirectInputDevice*>(device.get());
	return dev && std::find(dev->axis_code_left.cbegin(), dev->axis_code_left.cend(), keyCode) != dev->axis_code_left.cend();
}

bool direct_input_pad_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	DirectInputDevice* dev = static_cast<DirectInputDevice*>(device.get());
	return dev && std::find(dev->axis_code_right.cbegin(), dev->axis_code_right.cend(), keyCode) != dev->axis_code_right.cend();
}

std::vector<pad_list_entry> direct_input_pad_handler::list_devices()
{
	std::vector<pad_list_entry> list;

	if (!Init())
	{
		return list;
	}

	for (size_t i = 0; i < m_devices.size(); ++i)
	{
		auto& dev = m_devices.at(i);
		ensure(!!dev);

		list.emplace_back(dev->product_name, false);
	}

	return list;
}

#endif
