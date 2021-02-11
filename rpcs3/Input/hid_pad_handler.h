#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/CRC.h"

#include "hidapi.h"

struct CalibData
{
	s16 bias;
	s32 sens_numer;
	s32 sens_denom;
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
	hid_device* hidDevice{nullptr};
	std::string path{""};
	u8 largeVibrate{0};
	u8 smallVibrate{0};
	u8 led_delay_on{0};
	u8 led_delay_off{0};
};

template <class Device>
class hid_pad_handler : public PadHandlerBase
{
public:
	hid_pad_handler(pad_handler type, u16 vid, std::vector<u16> pids);
	~hid_pad_handler();

	bool Init() override;
	void ThreadProc() override;
	std::vector<std::string> ListDevices() override;

protected:
	enum class DataStatus
	{
		NewData,
		NoNewData,
		ReadError,
	};

	CRCPP::CRC::Table<u32, 32> crcTable{CRCPP::CRC::CRC_32()};

	u16 m_vid;
	std::vector<u16> m_pids;

	// pseudo 'controller id' to keep track of unique controllers
	std::map<std::string, std::shared_ptr<Device>> m_controllers;

	bool m_is_init = false;
	std::chrono::system_clock::time_point m_last_enumeration;
	std::set<std::string> m_last_enumerated_devices;

	void enumerate_devices();
	std::shared_ptr<Device> get_hid_device(const std::string& padId);

	virtual void check_add_device(hid_device* hidDevice, std::string_view path, std::wstring_view serial) = 0;

	inline s16 apply_calibration(s32 rawValue, const CalibData& calibData)
	{
		const s32 biased = rawValue - calibData.bias;
		const s32 quot   = calibData.sens_numer / calibData.sens_denom;
		const s32 rem    = calibData.sens_numer % calibData.sens_denom;
		const s32 output = (quot * biased) + ((rem * biased) / calibData.sens_denom);

		if (output > INT16_MAX)
			return INT16_MAX;
		else if (output < INT16_MIN)
			return INT16_MIN;
		else
			return static_cast<s16>(output);
	}

	inline s16 read_s16(const void* buf)
	{
		return *reinterpret_cast<const s16*>(buf);
	}

	inline u32 read_u32(const void* buf)
	{
		return *reinterpret_cast<const u32*>(buf);
	}

private:
	std::shared_ptr<PadDevice> get_device(const std::string& device) override;
};
