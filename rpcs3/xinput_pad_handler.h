#pragma once

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#include <chrono>

class xinput_pad_handler final : public PadHandlerBase
{
	// These are all the possible buttons on a standard xbox 360 or xbox one controller
	enum XInputKeyCodes
	{
		A,
		B,
		X,
		Y,
		Left,
		Right,
		Up,
		Down,
		LB,
		RB,
		LS,
		RS,
		Start,
		Back,
		Guide,

		LT,
		RT,

		LSXNeg,
		LSXPos,
		LSYNeg,
		LSYPos,
		RSXNeg,
		RSXPos,
		RSYNeg,
		RSYPos,

		KeyCodeCount
	};


	struct XInputDevice
	{
		u32 deviceNumber{ 0 };
		bool newVibrateData{ true };
		u16 largeVibrate{ 0 };
		u16 smallVibrate{ 0 };
		std::chrono::high_resolution_clock::time_point last_vibration;
		pad_config* config{ nullptr };
	};

	union XInputState; // Defined in .cpp

	friend class xinput_pad_processor_base; // Processors need access to XInputKeyCodes
	friend class xinput_pad_processor_xi;
	friend class xinput_pad_processor_scp;

public:
	xinput_pad_handler(pad_handler handler);
	~xinput_pad_handler() override;

	bool Init() override;

	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;
	void GetNextButtonPress(const std::string& padId, const std::function<void(u16, std::string, std::string, int[])>& callback, const std::function<void(std::string)>& fail_callback, bool get_blacklist = false, const std::vector<std::string>& buttons = {}) override;
	void SetPadData(const std::string& padId, u32 largeMotor, u32 smallMotor, s32 r, s32 g, s32 b) override;
	void init_config(pad_config* cfg, const std::string& name) override;


private:
	static std::unique_ptr<class xinput_pad_processor_base> makeProcessorFromType(pad_handler type);

	int GetDeviceNumber(const std::string& padId);
	void TranslateButtonPress(u64 keyCode, bool& pressed, u16& val, bool ignore_threshold = false) override;

	std::unique_ptr<class xinput_pad_processor_base> m_processor;

	std::vector<u32> blacklist;
	std::vector<std::pair<std::shared_ptr<XInputDevice>, std::shared_ptr<Pad>>> bindings;
	std::shared_ptr<XInputDevice> m_dev;

	// Unique names for the config files and our pad settings dialog
	const std::unordered_map<u32, std::string> button_list;
};
