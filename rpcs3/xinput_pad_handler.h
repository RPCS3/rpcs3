#ifndef X_INPUT_PAD_HANDLER
#define X_INPUT_PAD_HANDLER

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>

struct xinput_config final : cfg::node
{
	const std::string cfg_name = fs::get_config_dir() + "/config_xinput.yml";

	cfg::_int<0, 1000000> lstickdeadzone{ this, "Left Stick Deadzone", 7849 };
	cfg::_int<0, 1000000> rstickdeadzone{ this, "Right Stick Deadzone", 8689 };
	cfg::_int<0, 1000000> padsquircling{ this, "Pad Squircling Factor", 8000 };

	bool load()
	{
		if (fs::file cfg_file{ cfg_name, fs::read })
		{
			return from_string(cfg_file.to_string());
		}

		return false;
	}

	void save()
	{
		fs::file(cfg_name, fs::rewrite).write(to_string());
	}

	bool exist()
	{
		return fs::is_file(cfg_name);
	}
};

class xinput_pad_handler final : public PadHandlerBase
{
public:
	xinput_pad_handler();
	~xinput_pad_handler();

	bool Init() override;
	void Close();

	std::vector<std::string> ListDevices() override;
	bool bindPadToDevice(std::shared_ptr<Pad> pad, const std::string& device) override;
	void ThreadProc() override;

private:
	typedef void (WINAPI * PFN_XINPUTENABLE)(BOOL);
	typedef DWORD (WINAPI * PFN_XINPUTGETSTATE)(DWORD, XINPUT_STATE *);
	typedef DWORD (WINAPI * PFN_XINPUTSETSTATE)(DWORD, XINPUT_VIBRATION *);

private:
	std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY);

private:
	bool is_init;
	float squircle_factor;
	u32 left_stick_deadzone, right_stick_deadzone;
	HMODULE library;
	PFN_XINPUTGETSTATE xinputGetState;
	PFN_XINPUTSETSTATE xinputSetState;
	PFN_XINPUTENABLE xinputEnable;

	std::vector<std::pair<u32, std::shared_ptr<Pad>>> bindings;
	std::array<bool, 7> last_connection_status = {};

	// holds internal controller state change
	XINPUT_STATE state;
	DWORD result;
	DWORD online = 0;
};

extern xinput_config xinput_cfg;

#endif
