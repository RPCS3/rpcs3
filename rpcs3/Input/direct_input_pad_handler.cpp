#ifdef _WIN32
#pragma optimize( "", off )
#include "direct_input_pad_handler.h";
#include "direct_input_constants.h";
#include <tchar.h>

LOG_CHANNEL(dinput_log, "DirectInput");

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }

LPDIRECTINPUT8 g_pDI = nullptr;

static const size_t di_btn_count = 32;
static const int di_axis_min = -32768;
static const int di_axis_max = 32767;
static const int di_threshold = 100;

static std::string wstring(const WCHAR wstr[MAX_PATH])
{
	char ch[MAX_PATH];
	char def_char = ' ';
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, ch, MAX_PATH, &def_char, NULL);
	return std::string(ch);
}

static std::string get_device_name(const LPDIRECTINPUTDEVICE8 device, const GUID& type)
{
	DIPROPSTRING di_str = {};
	di_str.diph.dwSize = sizeof(di_str);
	di_str.diph.dwHeaderSize = sizeof(di_str.diph);
	di_str.diph.dwHow = DIPH_DEVICE;

	if (device->GetProperty(type, &di_str.diph) >= 0)
	{
		return wstring(di_str.wsz);
	}

	dinput_log.error("DirectInput driver failed to get device name");
	return "";
}

static std::string get_guid_and_path(const LPDIRECTINPUTDEVICE8 device)
{
	DIPROPGUIDANDPATH guid_and_path;
	guid_and_path.diph.dwSize = sizeof(guid_and_path);
	guid_and_path.diph.dwHeaderSize = sizeof(guid_and_path.diph);
	guid_and_path.diph.dwObj = 0;
	guid_and_path.diph.dwHow = DIPH_DEVICE;

	if (device->GetProperty(DIPROP_GUIDANDPATH, &guid_and_path.diph) >= 0)
	{
		return wstring(guid_and_path.wszPath);
	}

	dinput_log.error("DirectInput driver failed to get device name");
	return "";
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
	for (auto& binding : bindings)
	{
		if (DirectInputDevice* device = static_cast<DirectInputDevice*>(binding.first.get()))
		{
			if (device->device)
				device->device->Unacquire();

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
	//cfg->ps.def       = button_list.at(DInputKeyCodes::Guide); // I haven't figured this one out yet
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
	cfg->lstickdeadzone.def = di_threshold;
	cfg->rstickdeadzone.def = di_threshold;
	cfg->ltriggerthreshold.def = di_threshold;
	cfg->rtriggerthreshold.def = di_threshold;
	cfg->lpadsquircling.def = 8000;
	cfg->rpadsquircling.def = 8000;

	// apply defaults
	cfg->from_default();
}

BOOL CALLBACK EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext)
{
	auto device = reinterpret_cast<LPDIRECTINPUTDEVICE8*>(pContext);
	if (!device)
		return DIENUM_CONTINUE;

	// For axes that are returned, set the DIPROP_RANGE property for the
	// enumerated axis in order to scale min/max values.
	if (pdidoi->dwType & DIDFT_AXIS)
	{
		// Set the range for the axis
		DIPROPRANGE range;
		range.diph.dwSize = sizeof(DIPROPRANGE);
		range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		range.diph.dwHow = DIPH_BYID;
		range.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
		range.lMin = di_axis_min;
		range.lMax = di_axis_max;

		if ((*device)->SetProperty(DIPROP_RANGE, &range.diph) < 0)
			return DIENUM_STOP;
	
		// Set the dead zone for the axis
		DIPROPDWORD dead_zone;
		dead_zone.dwData = di_threshold;
		dead_zone.diph.dwSize = sizeof(DIPROPDWORD);
		dead_zone.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dead_zone.diph.dwHow = DIPH_BYID;
		dead_zone.diph.dwObj = pdidoi->dwType;
	
		if ((*device)->SetProperty(DIPROP_DEADZONE, &dead_zone.diph) < 0)
			return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
	auto devices = reinterpret_cast<std::vector<std::shared_ptr<direct_input_pad_handler::DirectInputDevice>>*>(pContext);
	if (!devices)
		return DIENUM_CONTINUE;

	LPDIRECTINPUTDEVICE8 device;

	if (g_pDI->CreateDevice(pdidInstance->guidInstance, &device, nullptr) < 0)
		return DIENUM_CONTINUE;

	dinput_log.error("tszInstanceName = %s", wstring(pdidInstance->tszInstanceName));
	dinput_log.error("tszProductName = %s", wstring(pdidInstance->tszProductName));

	if (device->SetDataFormat(&c_dfDIJoystick2) < 0)
		return DIENUM_CONTINUE;

	//if (device->SetCooperativeLevel(hDlg, DISCL_EXCLUSIVE | DISCL_FOREGROUND) < 0)
	//	return DIENUM_CONTINUE;

	if (device->EnumObjects(EnumObjectsCallback, &device, DI8DEVCLASS_ALL) < 0)
		return DIENUM_CONTINUE;

	DIDEVCAPS capabilities;
	capabilities.dwSize = sizeof(DIDEVCAPS);
	if (device->GetCapabilities(&capabilities) < 0)
		return DIENUM_CONTINUE;

	auto dev = std::make_shared<direct_input_pad_handler::DirectInputDevice>();
	dev->device = device;
	dev->capabilities = capabilities;
	dev->product_name = get_device_name(device, DIPROP_PRODUCTNAME);
	dev->instance_name = get_device_name(device, DIPROP_INSTANCENAME);
	dev->guid_and_path = get_guid_and_path(device);
	
	dinput_log.error("product_name = %s", dev->product_name);
	dinput_log.error("instance_name = %s", dev->instance_name);
	dinput_log.error("guid_and_path = %s", dev->guid_and_path);

	devices->emplace_back(dev);

	return DIENUM_CONTINUE;
}

bool direct_input_pad_handler::Init()
{
	if (is_init)
		return true;

	m_devices.clear();

	if (DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID * *)& g_pDI, nullptr) < 0)
	{
		dinput_log.error("Failed to initialize DirectInput");
		return false;
	}

	if (g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, &m_devices, DIEDFL_ATTACHEDONLY) < 0)
	{
		dinput_log.error("Failed to enumerate DirectInput devices");
		return false;
	}

	if (m_devices.size() <= 0)
	{
		dinput_log.error("No DirectInput devices found");
		return false;
	}

	dinput_log.notice("DirectInput driver supports %d devices", m_devices.size());

	is_init = true;
	return true;
}

PadHandlerBase::connection direct_input_pad_handler::update_connection(const std::shared_ptr<PadDevice>& device)
{
	DirectInputDevice* dev = static_cast<DirectInputDevice*>(device.get());
	if (!dev || !dev->device)
		return connection::disconnected;

	if (dev->device->Acquire() < 0)
		return connection::disconnected;

	if (dev->device->GetDeviceState(sizeof(DIJOYSTATE2), &dev->state) < 0)
		return connection::disconnected;

	return connection::connected;
}

std::unordered_map<u64, u16> direct_input_pad_handler::get_button_values(const std::shared_ptr<PadDevice>& device)
{
	std::unordered_map<u64, u16> values;

	DirectInputDevice* dev = static_cast<DirectInputDevice*>(device.get());
	if (!dev || !dev->device)
		return values;

	if (dev->device->Poll() < 0)
		return values;

	// Get POV values
	for (size_t i = 0; i < 4; i++)
	{
		const auto offset = i * 4;

		values[offset + DInputKeyCodes::POV_1_UP]    = dev->state.rgdwPOV[i] == 0 ? 255 : 0;
		values[offset + DInputKeyCodes::POV_1_RIGHT] = dev->state.rgdwPOV[i] == 9000 ? 255 : 0;
		values[offset + DInputKeyCodes::POV_1_DOWN]  = dev->state.rgdwPOV[i] == 18000 ? 255 : 0;
		values[offset + DInputKeyCodes::POV_1_LEFT]  = dev->state.rgdwPOV[i] == 27000 ? 255 : 0;
	}

	// Get Button values
	for (size_t i = 0; i < 128 && i < di_btn_count; i++)
	{
		values[i + DInputKeyCodes::BUTTON_1] = (dev->state.rgbButtons[i] & 0x80) ? 255 : 0;
	}

	// Get Axis values
	values[DInputKeyCodes::AXIS_X_NEG] = static_cast<u16>(dev->state.lX < 0 ? abs(dev->state.lX) - 1 : 0);
	values[DInputKeyCodes::AXIS_X_POS] = static_cast<u16>(dev->state.lX > 0 ? dev->state.lX : 0);
	values[DInputKeyCodes::AXIS_Y_NEG] = static_cast<u16>(dev->state.lY < 0 ? abs(dev->state.lY) - 1 : 0);
	values[DInputKeyCodes::AXIS_Y_POS] = static_cast<u16>(dev->state.lY > 0 ? dev->state.lY : 0);
	values[DInputKeyCodes::AXIS_Z_NEG] = static_cast<u16>(dev->state.lZ < 0 ? abs(dev->state.lZ) - 1 : 0);
	values[DInputKeyCodes::AXIS_Z_POS] = static_cast<u16>(dev->state.lZ > 0 ? dev->state.lZ : 0);

	values[DInputKeyCodes::AXIS_X_ROT_NEG] = static_cast<u16>(dev->state.lRx < 0 ? abs(dev->state.lRx) - 1 : 0);
	values[DInputKeyCodes::AXIS_X_ROT_POS] = static_cast<u16>(dev->state.lRx > 0 ? dev->state.lRx : 0);
	values[DInputKeyCodes::AXIS_Y_ROT_NEG] = static_cast<u16>(dev->state.lRy < 0 ? abs(dev->state.lRy) - 1 : 0);
	values[DInputKeyCodes::AXIS_Y_ROT_POS] = static_cast<u16>(dev->state.lRy > 0 ? dev->state.lRy : 0);
	values[DInputKeyCodes::AXIS_Z_ROT_NEG] = static_cast<u16>(dev->state.lRz < 0 ? abs(dev->state.lRz) - 1 : 0);
	values[DInputKeyCodes::AXIS_Z_ROT_POS] = static_cast<u16>(dev->state.lRz > 0 ? dev->state.lRz : 0);

	// Get Slider values
	values[DInputKeyCodes::SLIDER_1_NEG] = static_cast<u16>(dev->state.rglSlider[0] < 0 ? abs(dev->state.rglSlider[0]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_1_POS] = static_cast<u16>(dev->state.rglSlider[0] > 0 ? dev->state.rglSlider[0] : 0);
	values[DInputKeyCodes::SLIDER_2_NEG] = static_cast<u16>(dev->state.rglSlider[1] < 0 ? abs(dev->state.rglSlider[1]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_2_POS] = static_cast<u16>(dev->state.rglSlider[1] > 0 ? dev->state.rglSlider[1] : 0);

	values[DInputKeyCodes::SLIDER_1_VEL_NEG] = static_cast<u16>(dev->state.rglVSlider[0] < 0 ? abs(dev->state.rglVSlider[0]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_1_VEL_POS] = static_cast<u16>(dev->state.rglVSlider[0] > 0 ? dev->state.rglVSlider[0] : 0);
	values[DInputKeyCodes::SLIDER_2_VEL_NEG] = static_cast<u16>(dev->state.rglVSlider[1] < 0 ? abs(dev->state.rglVSlider[1]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_2_VEL_POS] = static_cast<u16>(dev->state.rglVSlider[1] > 0 ? dev->state.rglVSlider[1] : 0);

	values[DInputKeyCodes::SLIDER_1_ACC_NEG] = static_cast<u16>(dev->state.rglASlider[0] < 0 ? abs(dev->state.rglASlider[0]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_1_ACC_POS] = static_cast<u16>(dev->state.rglASlider[0] > 0 ? dev->state.rglASlider[0] : 0);
	values[DInputKeyCodes::SLIDER_2_ACC_NEG] = static_cast<u16>(dev->state.rglASlider[1] < 0 ? abs(dev->state.rglASlider[1]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_2_ACC_POS] = static_cast<u16>(dev->state.rglASlider[1] > 0 ? dev->state.rglASlider[1] : 0);

	values[DInputKeyCodes::SLIDER_1_FOR_NEG] = static_cast<u16>(dev->state.rglFSlider[0] < 0 ? abs(dev->state.rglFSlider[0]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_1_FOR_POS] = static_cast<u16>(dev->state.rglFSlider[0] > 0 ? dev->state.rglFSlider[0] : 0);
	values[DInputKeyCodes::SLIDER_2_FOR_NEG] = static_cast<u16>(dev->state.rglFSlider[1] < 0 ? abs(dev->state.rglFSlider[1]) - 1 : 0);
	values[DInputKeyCodes::SLIDER_2_FOR_POS] = static_cast<u16>(dev->state.rglFSlider[1] > 0 ? dev->state.rglFSlider[1] : 0);

	//dinput_log.error("S Val %d %d", dev->state.rglSlider[0], dev->state.rglSlider[1]);
	//dinput_log.error("S Vel %d %d", dev->state.rglVSlider[0], dev->state.rglVSlider[1]);
	//dinput_log.error("S Acc %d %d", dev->state.rglASlider[0], dev->state.rglASlider[1]);
	//dinput_log.error("S For %d %d", dev->state.rglFSlider[0], dev->state.rglFSlider[1]);
	//dinput_log.error("A Val %d %d %d", dev->state.lX, dev->state.lY, dev->state.lZ);
	//dinput_log.error("A Rot %d %d %d", dev->state.lRx, dev->state.lRy, dev->state.lRz);
	//dinput_log.error("A Vel %d %d %d", dev->state.lVX, dev->state.lVY, dev->state.lVZ);
	//dinput_log.error("A AVe %d %d %d", dev->state.lVRx, dev->state.lVRy, dev->state.lVRz);
	//dinput_log.error("A Acc %d %d %d", dev->state.lAX, dev->state.lAY, dev->state.lAZ);
	//dinput_log.error("A Acc %d %d %d", dev->state.lARx, dev->state.lARy, dev->state.lARz);
	//dinput_log.error("A For %d %d %d", dev->state.lFX, dev->state.lFY, dev->state.lFZ);
	//dinput_log.error("A Tor %d %d %d", dev->state.lFRx, dev->state.lFRy, dev->state.lFRz);

	return values;
}

pad_preview_values direct_input_pad_handler::get_preview_values(const std::unordered_map<u64, u16>& data, const std::vector<std::string>& buttons)
{
	pad_preview_values preview_values = { 0, 0, 0, 0, 0, 0 };

	if (buttons.size() == 10)
	{
		preview_values[0] = data.at(FindKeyCodeByString(button_list, buttons[0]));
		preview_values[1] = data.at(FindKeyCodeByString(button_list, buttons[1]));
		preview_values[2] = data.at(FindKeyCodeByString(button_list, buttons[3])) - data.at(FindKeyCodeByString(button_list, buttons[2]));
		preview_values[3] = data.at(FindKeyCodeByString(button_list, buttons[5])) - data.at(FindKeyCodeByString(button_list, buttons[4]));
		preview_values[4] = data.at(FindKeyCodeByString(button_list, buttons[7])) - data.at(FindKeyCodeByString(button_list, buttons[6]));
		preview_values[5] = data.at(FindKeyCodeByString(button_list, buttons[9])) - data.at(FindKeyCodeByString(button_list, buttons[8]));
	}

	return preview_values;
}

std::shared_ptr<PadDevice> direct_input_pad_handler::get_device(const std::string& device)
{
	if (!Init())
		return nullptr;

	for (size_t i = 0; i < m_devices.size(); ++i)
	{
		if (m_devices[i]->product_name == device)
		{
			return m_devices[i];
		}
	}

	return nullptr;
}

bool direct_input_pad_handler::get_is_left_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	return device && device->trigger_left == keyCode;
}

bool direct_input_pad_handler::get_is_right_trigger(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	return device && device->trigger_right == keyCode;
}

bool direct_input_pad_handler::get_is_left_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	return device && std::find(device->axis_left.begin(), device->axis_left.end(), keyCode) != device->axis_left.end();
}

bool direct_input_pad_handler::get_is_right_stick(const std::shared_ptr<PadDevice>& device, u64 keyCode)
{
	return device && std::find(device->axis_right.begin(), device->axis_right.end(), keyCode) != device->axis_right.end();
}

std::vector<std::string> direct_input_pad_handler::ListDevices()
{
	std::vector<std::string> list;

	if (!Init())
		return list;

	for (size_t i = 0; i < m_devices.size(); ++i)
	{
		list.emplace_back(m_devices[i]->product_name);
	}

	return list;
}
#pragma optimize( "", on )

#endif
