#include "stdafx.h"
#include "WiimoteManager.h"
#include "Input/hid_instance.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Utilities/File.h"
#include "util/yaml.hpp"
#include <algorithm>
#include <sstream>

LOG_CHANNEL(wiimote_log, "Wiimote");

// Nintendo
static constexpr u16 VID_NINTENDO = 0x057e;
static constexpr u16 PID_WIIMOTE = 0x0306;
static constexpr u16 PID_WIIMOTE_PLUS = 0x0330;

// Mayflash DolphinBar
static constexpr u16 VID_MAYFLASH = 0x0079;
static constexpr u16 PID_DOLPHINBAR_START = 0x1800;
static constexpr u16 PID_DOLPHINBAR_END = 0x1803;

wiimote_device::wiimote_device()
{
	m_state.connected = false;
}

wiimote_device::~wiimote_device()
{
	close();
}

bool wiimote_device::open(hid_device_info* info)
{
	if (m_handle) return false;

	m_path = info->path;
	m_serial = info->serial_number ? info->serial_number : L"";
	m_handle = hid_open_path(info->path);

	if (m_handle)
	{
		// 1. Connectivity Test (Matching wiimote_test)
		constexpr std::array<u8, 2> status_req = { 0x15, 0x00 };
		if (hid_write(m_handle, status_req.data(), status_req.size()) < 0)
		{
			close();
			return false;
		}

		// 2. Full Initialization
		if (initialize_ir())
		{
			m_state.connected = true;
			m_last_ir_check = std::chrono::steady_clock::now();
			return true;
		}

		close();
	}
	return false;
}

void wiimote_device::close()
{
	if (m_handle)
	{
#if defined(__APPLE__)
		Emu.BlockingCallFromMainThread([this]()
		{
			if (m_handle)
			{
				hid_close(m_handle);
			}
		}, false);
#else
		hid_close(m_handle);
#endif
		m_handle = nullptr;
	}
	m_state = {}; // Reset state including connected = false
	m_path.clear();
	m_serial.clear();
}

bool wiimote_device::write_reg(u32 addr, const std::vector<u8>& data)
{
	u8 buf[22] = {0};
	buf[0] = 0x16; // Write register
	buf[1] = 0x06; // Register Space + Request Acknowledgement
	buf[2] = (addr >> 16) & 0xFF;
	buf[3] = (addr >> 8) & 0xFF;
	buf[4] = addr & 0xFF;
	buf[5] = static_cast<u8>(data.size());
	std::copy(data.begin(), data.end(), &buf[6]);
	if (hid_write(m_handle, buf, sizeof(buf)) < 0) return false;
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	return true;
}

bool wiimote_device::initialize_ir()
{
	// 1. Enable IR logic / Pixel Clock (Requesting Acknowledgement for stability)
	constexpr std::array<u8, 2> ir_on1 = { 0x13, 0x06 };
	hid_write(m_handle, ir_on1.data(), ir_on1.size());
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	constexpr std::array<u8, 2> ir_on2 = { 0x1a, 0x06 };
	hid_write(m_handle, ir_on2.data(), ir_on2.size());
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// 2. Enable IR Camera (Wii-style sequence)
	if (!write_reg(0xb00030, {0x01})) return false;

	// 3. Sensitivity Level 3 (Exactly matching wiimote_test / official levels)
	if (!write_reg(0xb00000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x41})) return false;
	if (!write_reg(0xb0001a, {0x40, 0x00})) return false;

	// 4. IR Mode: Extended (3 bytes per point)
	if (!write_reg(0xb00033, {0x03})) return false;

	// 5. Finalize IR Enable
	if (!write_reg(0xb00030, {0x08})) return false;

	// 6. Reporting mode: Buttons + Accel + IR (Continuous)
	constexpr std::array<u8, 3> mode = { 0x12, 0x04, 0x33 };
	if (hid_write(m_handle, mode.data(), mode.size()) < 0) return false;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	return true;
}

bool wiimote_device::verify_ir_health()
{
	if (!m_handle) return false;

	// Request device status to verify communication
	constexpr std::array<u8, 2> status_req = { 0x15, 0x00 };
	if (hid_write(m_handle, status_req.data(), status_req.size()) < 0)
	{
		return false;
	}

	// Try to read a response within reasonable time
	u8 buf[22];
	const int res = hid_read_timeout(m_handle, buf, sizeof(buf), 100);

	// If we got a response, device is alive. If timeout or error, it's dead.
	return res > 0;
}

bool wiimote_device::update()
{
	if (!m_handle) return false;

	// Every 3 seconds, verify IR is still working
	const auto now = std::chrono::steady_clock::now();
	if (now - m_last_ir_check > std::chrono::seconds(3))
	{
		m_last_ir_check = now;
		if (!verify_ir_health())
		{
			// Device not responding - attempt to reinitialize IR
			if (!initialize_ir())
			{
				// Failed to reinitialize - close and mark disconnected
				close();
				return false;
			}
		}
	}

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
				const u8* ir = &buf[6 + j * 3];
				m_state.ir[j].x = (ir[0] | ((ir[2] & 0x30) << 4));
				m_state.ir[j].y = (ir[1] | ((ir[2] & 0xC0) << 2));
				m_state.ir[j].size = ir[2] & 0x0f;
			}
		}
	}

	// hid_read_timeout returns -1 on error (e.g. device disconnected).
	if (res < 0)
	{
		// Device error - try to recover once before giving up
		if (!initialize_ir())
		{
			close();
			return false;
		}
	}

	return true;
}

static wiimote_manager* s_instance = nullptr;

static std::string get_config_path()
{
	return fs::get_config_dir(true) + "wiimote.yml";
}

void wiimote_manager::load_config()
{
	const std::string path = get_config_path();
	if (!fs::exists(path)) return;

	auto [root, error] = yaml_load(fs::read_all_text(path));
	if (!error.empty())
	{
		wiimote_log.error("Failed to load wiimote config: %s", error);
		return;
	}

	wiimote_guncon_mapping map;
	auto parse_btn = [&](const char* key, wiimote_button& btn)
	{
		if (root[key])
		{
			try
			{
				btn = static_cast<wiimote_button>(root[key].as<u16>());
			}
			catch (const YAML::Exception&)
			{
				wiimote_log.error("Invalid value for %s in wiimote config", key);
			}
		}
	};

	parse_btn("trigger", map.trigger);
	parse_btn("a1", map.a1);
	parse_btn("a2", map.a2);
	parse_btn("a3", map.a3);
	parse_btn("b1", map.b1);
	parse_btn("b2", map.b2);
	parse_btn("b3", map.b3);
	parse_btn("c1", map.c1);
	parse_btn("c2", map.c2);

	std::unique_lock lock(m_mutex);
	m_mapping = map;
}

void wiimote_manager::save_config()
{
	YAML::Node root;
	{
		std::shared_lock lock(m_mutex);
		root["trigger"] = static_cast<u16>(m_mapping.trigger);
		root["a1"] = static_cast<u16>(m_mapping.a1);
		root["a2"] = static_cast<u16>(m_mapping.a2);
		root["a3"] = static_cast<u16>(m_mapping.a3);
		root["b1"] = static_cast<u16>(m_mapping.b1);
		root["b2"] = static_cast<u16>(m_mapping.b2);
		root["b3"] = static_cast<u16>(m_mapping.b3);
		root["c1"] = static_cast<u16>(m_mapping.c1);
		root["c2"] = static_cast<u16>(m_mapping.c2);
	}

	YAML::Emitter emitter;
	emitter << root;
	fs::write_all_text(get_config_path(), emitter.c_str());
}

wiimote_manager::wiimote_manager()
{
	if (!s_instance)
		s_instance = this;

	// Pre-initialize 4 Wiimote slots (standard for DolphinBar and typical local multiplayer)
	for (int i = 0; i < 4; i++)
	{
		m_devices.push_back(std::make_unique<wiimote_device>());
	}

	load_config();
}

wiimote_manager::~wiimote_manager()
{
	stop();
	if (s_instance == this)
		s_instance = nullptr;
}

wiimote_manager* wiimote_manager::get_instance()
{
	return s_instance;
}

void wiimote_manager::start()
{
	if (m_running) return;

	// Note: initialize() is thread-safe and handles multiple calls.
	if (!hid_instance::get_instance().initialize()) return;

	m_running = true;
	m_thread = std::thread(&wiimote_manager::thread_proc, this);
}

void wiimote_manager::stop()
{
	m_running = false;
	if (m_thread.joinable()) m_thread.join();
}

size_t wiimote_manager::get_device_count()
{
	std::shared_lock lock(m_mutex);
	return m_devices.size();
}

void wiimote_manager::set_mapping(const wiimote_guncon_mapping& mapping)
{
	{
		std::unique_lock lock(m_mutex);
		m_mapping = mapping;
	}
	save_config();
}

wiimote_guncon_mapping wiimote_manager::get_mapping() const
{
	// shared_lock not strictly needed for trivial copy but good practice if it becomes complex
	return m_mapping;
}

std::vector<wiimote_state> wiimote_manager::get_states()
{
	std::shared_lock lock(m_mutex);
	std::vector<wiimote_state> states;
	states.reserve(m_devices.size());

	for (const auto& dev : m_devices)
	{
		states.push_back(dev->get_state());
	}
	return states;
}


void wiimote_manager::thread_proc()
{
	thread_ctrl::set_name("WiiMoteManager");

	u32 counter = 0;
	while (m_running)
	{
		// Scan every 2 seconds
		if (counter++ % 200 == 0)
		{
			const auto scan_and_add = [&](u16 vid, u16 pid_start, u16 pid_end)
			{
				std::lock_guard lock(g_hid_mutex);

				hid_device_info* devs;
#if defined(__APPLE__)
				Emu.BlockingCallFromMainThread([&]()
				{
					devs = hid_enumerate(vid, 0);
				}, false);
#else
				devs = hid_enumerate(vid, 0);
#endif
				for (hid_device_info* cur = devs; cur; cur = cur->next)
				{
					if (cur->product_id >= pid_start && cur->product_id <= pid_end)
					{
						std::unique_lock lock(m_mutex);

						// 1. Check if this physical device is already connected to any slot
						bool already_connected = false;
						for (const auto& d : m_devices)
						{
							if (d->get_state().connected && d->get_path() == cur->path)
							{
								already_connected = true;
								break;
							}
						}
						if (already_connected) continue;

						// 2. Determine target slot
						int slot_idx = -1;
						if (vid == VID_MAYFLASH)
						{
							// DolphinBar Mode 4: PIDs 0x1800-0x1803 correspond to Players 1-4
							slot_idx = cur->product_id - PID_DOLPHINBAR_START;
						}
						else
						{
							// Generic Wiimote: Find first available slot
							for (size_t i = 0; i < m_devices.size(); i++)
							{
								if (!m_devices[i]->get_state().connected)
								{
									slot_idx = static_cast<int>(i);
									break;
								}
							}
						}

						// 3. Connect to slot
						if (slot_idx >= 0 && slot_idx < static_cast<int>(m_devices.size()))
						{
							if (!m_devices[slot_idx]->get_state().connected)
							{
								m_devices[slot_idx]->open(cur);
							}
						}
					}
				}
				hid_free_enumeration(devs);
			};

			// Generic Wiimote / DolphinBar Mode 4 (Normal)
			scan_and_add(VID_NINTENDO, PID_WIIMOTE, PID_WIIMOTE);
			// Wiimote Plus
			scan_and_add(VID_NINTENDO, PID_WIIMOTE_PLUS, PID_WIIMOTE_PLUS);
			// Mayflash DolphinBar Mode 4 (Custom VID/PIDs)
			scan_and_add(VID_MAYFLASH, PID_DOLPHINBAR_START, PID_DOLPHINBAR_END);
		}

		// Update all devices at 100Hz
		{
			std::unique_lock lock(m_mutex);
			for (auto& d : m_devices)
			{
				if (d->get_state().connected)
				{
					d->update();
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
