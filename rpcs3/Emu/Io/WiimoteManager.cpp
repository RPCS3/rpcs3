#include "stdafx.h"
#include "WiimoteManager.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Utilities/File.h"
#include <algorithm>
#include <sstream>

// Nintendo
static constexpr u16 VID_NINTENDO = 0x057e;
static constexpr u16 PID_WIIMOTE = 0x0306;
static constexpr u16 PID_WIIMOTE_PLUS = 0x0330;

// Mayflash DolphinBar
static constexpr u16 VID_MAYFLASH = 0x0079;
static constexpr u16 PID_DOLPHINBAR_START = 0x1800;
static constexpr u16 PID_DOLPHINBAR_END = 0x1803;

WiimoteDevice::WiimoteDevice(hid_device_info* info)
	: m_path(info->path)
	, m_serial(info->serial_number ? info->serial_number : L"")
{
	m_handle = hid_open_path(info->path);
	if (m_handle)
	{
		// 1. Connectivity Test (Matching wiimote_test)
		u8 status_req[] = { 0x15, 0x00 };
		if (hid_write(m_handle, status_req, sizeof(status_req)) < 0)
		{
			hid_close(m_handle);
			m_handle = nullptr;
			return;
		}

		// 2. Full Initialization
		if (initialize_ir())
		{
			m_state.connected = true;
		}
		else
		{
			hid_close(m_handle);
			m_handle = nullptr;
		}
	}
}

WiimoteDevice::~WiimoteDevice()
{
	if (m_handle) hid_close(m_handle);
}

bool WiimoteDevice::initialize_ir()
{
	auto write_reg = [&](u32 addr, const std::vector<u8>& data) {
		u8 buf[22] = {0};
		buf[0] = 0x16; // Write register
		buf[1] = 0x04;
		buf[2] = (addr >> 16) & 0xFF;
		buf[3] = (addr >> 8) & 0xFF;
		buf[4] = addr & 0xFF;
		buf[5] = static_cast<u8>(data.size());
		std::copy(data.begin(), data.end(), &buf[6]);
		if (hid_write(m_handle, buf, sizeof(buf)) < 0) return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		return true;
	};

	// 1. Enable IR logic / Pixel Clock
	u8 ir_on1[] = { 0x13, 0x04 };
	hid_write(m_handle, ir_on1, 2);
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	u8 ir_on2[] = { 0x1a, 0x04 };
	hid_write(m_handle, ir_on2, 2);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// 2. Enable IR Camera (Matching wiimote_test order)
	if (!write_reg(0xb00030, {0x08})) return false;

	// 3. Sensitivity Level 3 (Exactly matching wiimote_test)
	if (!write_reg(0xb00000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x41})) return false;
	if (!write_reg(0xb0001a, {0x40, 0x00})) return false;

	// 4. IR Mode: Extended (3 bytes per point)
	if (!write_reg(0xb00033, {0x03})) return false;

	// 5. Reporting mode: Buttons + Accel + IR
	u8 mode[] = { 0x12, 0x00, 0x33 };
	if (hid_write(m_handle, mode, sizeof(mode)) < 0) return false;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	return true;
}

bool WiimoteDevice::update()
{
	if (!m_handle) return false;

	u8 buf[22];
	int res;

	// Fully drain the buffer until empty to ensure we have the most recent data.
	// This avoids getting stuck behind a backlog of old reports (e.g. from before IR was enabled).
	while ((res = hid_read_timeout(m_handle, buf, sizeof(buf), 0)) > 0)
	{
		// All data reports (0x30-0x3F) carry buttons in the same location (first 2 bytes).
		// We mask out accelerometer LSBs (bits 5,6 of both bytes).
		if ((buf[0] & 0xF0) == 0x30)
		{
			m_state.buttons = (buf[1] | (buf[2] << 8)) & 0x9F1F;
		}

		// Mode 0x33: Buttons + Accel + IR (Extended Format)
		if (buf[0] == 0x33)
		{
			// Parse Accelerometer (byte 1: bits 5,6 are X LSBs; byte 2: bit 5 Y LSB, bit 6 Z LSB)
			m_state.acc_x = (buf[3] << 2) | ((buf[1] >> 5) & 3);
			m_state.acc_y = (buf[4] << 2) | ((buf[2] >> 5) & 1);
			m_state.acc_z = (buf[5] << 2) | ((buf[2] >> 6) & 1);

			// Each IR point is 3 bytes in Extended report 0x33.
			for (int j = 0; j < 4; j++)
			{
				u8* ir = &buf[6 + j * 3];
				m_state.ir[j].x = (ir[0] | ((ir[2] & 0x30) << 4));
				m_state.ir[j].y = (ir[1] | ((ir[2] & 0xC0) << 2));
				m_state.ir[j].size = ir[2] & 0x0f;
			}
		}
	}

	// hid_read_timeout returns -1 on error (e.g. device disconnected).
	if (res < 0) return false;

	return true;
}

static WiimoteManager* s_instance = nullptr;

static std::string get_config_path()
{
	return fs::get_config_dir(true) + "wiimote.yml";
}

void WiimoteManager::load_config()
{
	fs::file f(get_config_path(), fs::read);
	if (!f) return;

	std::string line;
	std::stringstream ss(f.to_string());
	WiimoteGunConMapping map;

	auto parse_btn = [](const std::string& val) -> WiimoteButton {
		return static_cast<WiimoteButton>(std::strtoul(val.c_str(), nullptr, 0));
	};

	while (std::getline(ss, line))
	{
		auto pos = line.find(':');
		if (pos == std::string::npos) continue;

		std::string key = line.substr(0, pos);
		std::string val = line.substr(pos + 1);

		// Trim whitespace
		key.erase(0, key.find_first_not_of(" \t"));
		key.erase(key.find_last_not_of(" \t") + 1);
		val.erase(0, val.find_first_not_of(" \t"));
		val.erase(val.find_last_not_of(" \t") + 1);

		if (key == "trigger") map.trigger = parse_btn(val);
		else if (key == "a1") map.a1 = parse_btn(val);
		else if (key == "a2") map.a2 = parse_btn(val);
		else if (key == "a3") map.a3 = parse_btn(val);
		else if (key == "b1") map.b1 = parse_btn(val);
		else if (key == "b2") map.b2 = parse_btn(val);
		else if (key == "b3") map.b3 = parse_btn(val);
		else if (key == "c1") map.c1 = parse_btn(val);
		else if (key == "c2") map.c2 = parse_btn(val);
	}

	std::unique_lock lock(m_mutex);
	m_mapping = map;
}

void WiimoteManager::save_config()
{
	fs::file f(get_config_path(), fs::write + fs::create + fs::trunc);
	if (!f) return;

	std::stringstream ss;
	// Helper to write lines
	auto write_line = [&](const char* key, WiimoteButton btn) {
		ss << key << ": " << static_cast<u16>(btn) << "\n";
	};

	{
		std::shared_lock lock(m_mutex);
		write_line("trigger", m_mapping.trigger);
		write_line("a1", m_mapping.a1);
		write_line("a2", m_mapping.a2);
		write_line("a3", m_mapping.a3);
		write_line("b1", m_mapping.b1);
		write_line("b2", m_mapping.b2);
		write_line("b3", m_mapping.b3);
		write_line("c1", m_mapping.c1);
		write_line("c2", m_mapping.c2);
	}

	f.write(ss.str());
}

WiimoteManager::WiimoteManager()
{
	if (!s_instance)
		s_instance = this;

	// Set default mapping explicitly to match user preference: C1=Plus, A3=Left
	// (Struct default constructor might have different values if I didn't edit header defaults)
	// Let's force it here before loading config.
	m_mapping.c1 = WiimoteButton::Plus;
	m_mapping.a3 = WiimoteButton::Left;
	// Defaults for others from struct:
	// a1=A, a2=Minus, etc.

	load_config();
}

WiimoteManager::~WiimoteManager()
{
	stop();
	if (s_instance == this)
		s_instance = nullptr;
}

WiimoteManager* WiimoteManager::get_instance()
{
	return s_instance;
}

void WiimoteManager::start()
{
	if (m_running) return;

	// Note: hid_init() is not thread-safe. ideally should be called once at app startup.
	if (hid_init() != 0) return;

	m_running = true;
	m_thread = std::thread(&WiimoteManager::thread_proc, this);
}

void WiimoteManager::stop()
{
	m_running = false;
	if (m_thread.joinable()) m_thread.join();
	hid_exit();
}

size_t WiimoteManager::get_device_count()
{
	std::shared_lock lock(m_mutex);
	return m_devices.size();
}

void WiimoteManager::set_mapping(const WiimoteGunConMapping& mapping)
{
	{
		std::unique_lock lock(m_mutex);
		m_mapping = mapping;
	}
	save_config();
}

WiimoteGunConMapping WiimoteManager::get_mapping() const
{
	// shared_lock not strictly needed for trivial copy but good practice if it becomes complex
	return m_mapping;
}

std::vector<WiimoteState> WiimoteManager::get_states()
{
	std::shared_lock lock(m_mutex);
	std::vector<WiimoteState> states;
	states.reserve(m_devices.size());

	for (const auto& dev : m_devices)
	{
		states.push_back(dev->get_state());
	}
	return states;
}


void WiimoteManager::thread_proc()
{
	u32 counter = 0;
	while (m_running)
	{
		// Scan every 2 seconds
		if (counter++ % 200 == 0)
		{
			auto scan_and_add = [&](u16 vid, u16 pid_start, u16 pid_end)
			{
				hid_device_info* devs = hid_enumerate(vid, 0);
				hid_device_info* cur = devs;

				while (cur)
				{
					if (cur->product_id >= pid_start && cur->product_id <= pid_end)
					{
						bool already_owned = false;
						{
							std::shared_lock lock(m_mutex);
							for (const auto& d : m_devices)
							{
								if (d->get_path() == cur->path)
								{
									already_owned = true;
									break;
								}
							}
						}

						if (!already_owned)
						{
							auto dev = std::make_unique<WiimoteDevice>(cur);
							if (dev->get_state().connected)
							{
								std::unique_lock lock(m_mutex);
								m_devices.push_back(std::move(dev));
							}
						}
					}
					cur = cur->next;
				}
				hid_free_enumeration(devs);
			};

			// Generic Wiimote / DolphinBar Mode 4 (Normal)
			scan_and_add(VID_NINTENDO, PID_WIIMOTE, PID_WIIMOTE);
			// Wiimote Plus
			scan_and_add(VID_NINTENDO, PID_WIIMOTE_PLUS, PID_WIIMOTE_PLUS);
			// Mayflash DolphinBar Mode 4 (Custom VID/PIDs)
			// Supports up to 4 players (1800, 1801, 1802, 1803)
			scan_and_add(VID_MAYFLASH, PID_DOLPHINBAR_START, PID_DOLPHINBAR_END);
		}

		// Update all devices at 100Hz
		{
			std::unique_lock lock(m_mutex);
			m_devices.erase(std::remove_if(m_devices.begin(), m_devices.end(), [](const auto& d)
			{
				return !const_cast<WiimoteDevice&>(*d).update();
			}), m_devices.end());
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
