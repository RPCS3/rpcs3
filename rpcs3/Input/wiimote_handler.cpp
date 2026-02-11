#include "stdafx.h"
#include "wiimote_handler.h"
#include "Input/hid_instance.h"
#include "Emu/System.h"
#include "Emu/Io/wiimote_config.h"
#include "util/logs.hpp"
#include <algorithm>
#include <initializer_list>
#include <vector>

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

	m_handle = hid_instance::open_path(info->path);

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
	hid_instance::close(m_handle);
	m_handle = nullptr;

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

static wiimote_handler* s_instance = nullptr;

void wiimote_handler::load_config()
{
	auto& cfg = get_wiimote_config();
	if (cfg.load())
	{
		std::unique_lock lock(m_mutex);
		m_mapping.trigger = static_cast<wiimote_button>(cfg.mapping.trigger.get());
		m_mapping.a1 = static_cast<wiimote_button>(cfg.mapping.a1.get());
		m_mapping.a2 = static_cast<wiimote_button>(cfg.mapping.a2.get());
		m_mapping.a3 = static_cast<wiimote_button>(cfg.mapping.a3.get());
		m_mapping.b1 = static_cast<wiimote_button>(cfg.mapping.b1.get());
		m_mapping.b2 = static_cast<wiimote_button>(cfg.mapping.b2.get());
		m_mapping.b3 = static_cast<wiimote_button>(cfg.mapping.b3.get());
		m_mapping.c1 = static_cast<wiimote_button>(cfg.mapping.c1.get());
		m_mapping.c2 = static_cast<wiimote_button>(cfg.mapping.c2.get());
		m_mapping.b1_alt = static_cast<wiimote_button>(cfg.mapping.b1_alt.get());
		m_mapping.b2_alt = static_cast<wiimote_button>(cfg.mapping.b2_alt.get());
	}
}

void wiimote_handler::save_config()
{
	{
		std::shared_lock lock(m_mutex);
		auto& cfg = get_wiimote_config();
		cfg.mapping.trigger.set(static_cast<u16>(m_mapping.trigger));
		cfg.mapping.a1.set(static_cast<u16>(m_mapping.a1));
		cfg.mapping.a2.set(static_cast<u16>(m_mapping.a2));
		cfg.mapping.a3.set(static_cast<u16>(m_mapping.a3));
		cfg.mapping.b1.set(static_cast<u16>(m_mapping.b1));
		cfg.mapping.b2.set(static_cast<u16>(m_mapping.b2));
		cfg.mapping.b3.set(static_cast<u16>(m_mapping.b3));
		cfg.mapping.c1.set(static_cast<u16>(m_mapping.c1));
		cfg.mapping.c2.set(static_cast<u16>(m_mapping.c2));
		cfg.mapping.b1_alt.set(static_cast<u16>(m_mapping.b1_alt));
		cfg.mapping.b2_alt.set(static_cast<u16>(m_mapping.b2_alt));
	}
	get_wiimote_config().save();
}

wiimote_handler::wiimote_handler()
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

wiimote_handler::~wiimote_handler()
{
	stop();
	if (s_instance == this)
		s_instance = nullptr;
}

wiimote_handler* wiimote_handler::get_instance()
{
	static std::mutex mtx;
	std::lock_guard lock(mtx);

	if (!s_instance)
	{
		s_instance = new wiimote_handler();
		s_instance->start();
	}
	return s_instance;
}

void wiimote_handler::start()
{
	if (m_running) return;

	// Note: initialize() is thread-safe and handles multiple calls.
	if (!hid_instance::get_instance().initialize()) return;

	m_running = true;
	m_thread = std::make_unique<named_thread<std::function<void()>>>("WiiMoteManager", [this]() { thread_proc(); });
}

void wiimote_handler::stop()
{
	m_running = false;
	m_thread.reset();
}

usz wiimote_handler::get_device_count()
{
	std::shared_lock lock(m_mutex);
	return m_devices.size();
}

void wiimote_handler::set_mapping(const wiimote_guncon_mapping& mapping)
{
	{
		std::unique_lock lock(m_mutex);
		m_mapping = mapping;
	}
	save_config();
}

wiimote_guncon_mapping wiimote_handler::get_mapping() const
{
	// shared_lock not strictly needed for trivial copy but good practice if it becomes complex
	return m_mapping;
}

std::vector<wiimote_state> wiimote_handler::get_states()
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


void wiimote_handler::thread_proc()
{
	u32 counter = 0;
	while (m_running && thread_ctrl::state() != thread_state::aborting)
	{
		// Scan every 2 seconds
		if (counter++ % 200 == 0)
		{
			const auto scan_and_add = [&](u16 vid, std::initializer_list<std::pair<u16, u16>> ranges)
			{
				struct info_t
				{
					std::string path;
					u16 product_id;
					std::wstring serial;
				};
				std::vector<info_t> candidates;

				hid_device_info* devs = hid_instance::enumerate(vid, 0);
				for (hid_device_info* cur = devs; cur; cur = cur->next)
				{
					for (const auto& range : ranges)
					{
						if (cur->product_id >= range.first && cur->product_id <= range.second)
						{
							candidates.push_back({cur->path, cur->product_id, cur->serial_number ? cur->serial_number : L""});
							break;
						}
					}
				}
				hid_instance::free_enumeration(devs);

				for (const auto& candidate : candidates)
				{
					// 1. Check if this physical device is already connected to any slot
					{
						std::shared_lock lock(m_mutex);
						bool already_connected = false;
						for (const auto& d : m_devices)
						{
							if (d->get_state().connected && d->get_path() == candidate.path)
							{
								already_connected = true;
								break;
							}
						}
						if (already_connected) continue;
					}

					// 2. Determine target slot
					int slot_idx = -1;
					if (vid == VID_MAYFLASH)
					{
						// DolphinBar Mode 4: PIDs 0x1800-0x1803 correspond to Players 1-4
						slot_idx = candidate.product_id - PID_DOLPHINBAR_START;
					}
					else
					{
						// Generic Wiimote: Find first available slot
						std::shared_lock lock(m_mutex);
						for (usz i = 0; i < m_devices.size(); i++)
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
						bool already_connected = false;
						{
							std::shared_lock lock(m_mutex);
							already_connected = m_devices[slot_idx]->get_state().connected;
						}

						if (!already_connected)
						{
							// Re-create a temporary info struct for open()
							hid_device_info temp_info = {};
							temp_info.path = const_cast<char*>(candidate.path.c_str());
							temp_info.serial_number = const_cast<wchar_t*>(candidate.serial.c_str());
							temp_info.product_id = candidate.product_id;

							// We call open() without holding m_mutex to avoid deadlock with main thread
							// wiimote_device::open() internally handles m_handle and connectivity state
							m_devices[slot_idx]->open(&temp_info);
						}
					}
				}
			};

			// Generic Wiimote / Wiimote Plus / DolphinBar Mode 4 (Normal)
			scan_and_add(VID_NINTENDO, {{PID_WIIMOTE, PID_WIIMOTE}, {PID_WIIMOTE_PLUS, PID_WIIMOTE_PLUS}});
			// Mayflash DolphinBar Mode 4 (Custom VID/PIDs)
			scan_and_add(VID_MAYFLASH, {{PID_DOLPHINBAR_START, PID_DOLPHINBAR_END}});
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

		thread_ctrl::wait_for(10'000);
	}
}
