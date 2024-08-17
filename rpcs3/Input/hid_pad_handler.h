#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/CRC.h"
#include "Utilities/Thread.h"

#include "hidapi.h"

struct CalibData
{
	s16 bias = 0;
	s32 sens_numer = 0;
	s32 sens_denom = 0;
};

enum CalibIndex
{
	// gyro
	PITCH = 0,
	YAW,
	ROLL,

	// accel
	X,
	Y,
	Z,
	COUNT
};

class HidDevice : public PadDevice
{
public:
	void close();

	hid_device* hidDevice{nullptr};
	std::string path;
	bool enable_player_leds{false};
	u8 led_delay_on{0};
	u8 led_delay_off{0};
	u8 battery_level{0};
	u8 last_battery_level{0};
	u8 cable_state{0};
};

struct id_pair
{
	u16 m_vid = 0;
	u16 m_pid = 0;
};

template <class Device>
class hid_pad_handler : public PadHandlerBase
{
public:
	hid_pad_handler(pad_handler type, std::vector<id_pair> ids);
	~hid_pad_handler();

	bool Init() override;
	void process() override;
	std::vector<pad_list_entry> list_devices() override;

protected:
	enum class DataStatus
	{
		NewData,
		NoNewData,
		ReadError,
	};

	CRCPP::CRC::Table<u32, 32> crcTable{CRCPP::CRC::CRC_32()};

	std::vector<id_pair> m_ids;

	// pseudo 'controller id' to keep track of unique controllers
	std::map<std::string, std::shared_ptr<Device>> m_controllers;

	std::set<std::string> m_last_enumerated_devices;
	std::set<std::string> m_new_enumerated_devices;
	std::map<std::string, std::wstring> m_enumerated_serials;
	std::mutex m_enumeration_mutex;
	std::unique_ptr<named_thread<std::function<void()>>> m_enumeration_thread;

	void enumerate_devices();
	void update_devices();
	std::shared_ptr<Device> get_hid_device(const std::string& padId);

	virtual void check_add_device(hid_device* hidDevice, std::string_view path, std::wstring_view serial) = 0;
	virtual int send_output_report(Device* device) = 0;
	virtual DataStatus get_data(Device* device) = 0;

	static s16 apply_calibration(s32 raw_value, const CalibData& calib_data)
	{
		const s32 biased = raw_value - calib_data.bias;
		const s32 quot   = calib_data.sens_numer / calib_data.sens_denom;
		const s32 rem    = calib_data.sens_numer % calib_data.sens_denom;
		const s32 output = (quot * biased) + ((rem * biased) / calib_data.sens_denom);

		return static_cast<s16>(std::clamp<s32>(output, s16{smin}, s16{smax}));
	}

	static s16 read_s16(const void* buf)
	{
		return *static_cast<const s16*>(buf);
	}

	static u32 read_u32(const void* buf)
	{
		return *static_cast<const u32*>(buf);
	}

	static u32 get_battery_color(u8 battery_level, u32 brightness);

private:
	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
};
