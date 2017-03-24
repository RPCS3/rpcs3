#pragma once

#include "Emu/Io/PadHandler.h"
#include "Utilities/Thread.h"
#include "hidapi.h"

const u32 MAX_GAMEPADS = 7;

class DS4Thread final : public named_thread
{
private:
	struct DS4Device
	{
		hid_device* hidDevice;
		std::string path;
		bool btCon;
		u8 largeVibrate;
		u8 smallVibrate;
	};

	const u16 DS4_VID = 0x054C;

	// pid's of connected ds4
	const std::array<u16, 3> ds4Pids = {{0xBA0, 0x5C4, 0x09CC }};

	// pseudo 'controller id' to keep track of unique controllers
	std::unordered_map<std::string, DS4Device> controllers;

	std::array<std::array<u8, 64>, MAX_GAMEPADS> padData{};

	void on_task() override;

	std::string get_name() const override { return "DS4 Thread"; }

	semaphore<> mutex;

public:
	void on_init(const std::shared_ptr<void>&) override;

	std::array<bool, MAX_GAMEPADS> GetConnectedControllers();

	std::array<std::array<u8, 64>, MAX_GAMEPADS> GetControllerData();

	void SetRumbleData(u32 port, u8 largeVibrate, u8 smallVibrate);

	DS4Thread() = default;

	~DS4Thread();
};

class DS4PadHandler final : public PadHandlerBase
{
public:
	DS4PadHandler() {}
	~DS4PadHandler();

	void Init(const u32 max_connect) override;
	void Close();

	PadInfo& GetInfo() override;
	std::vector<Pad>& GetPads() override;
	void SetRumble(const u32 pad, u8 largeMotor, bool smallMotor) override;

private:
	void ProcessData();

	// holds internal controller state change
	std::array<bool, MAX_GAMEPADS> last_connection_status = {};

	std::shared_ptr<DS4Thread> ds4Thread;
};
