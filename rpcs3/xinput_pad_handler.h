#ifndef X_INPUT_PAD_HANDLER
#define X_INPUT_PAD_HANDLER

#include "Utilities/Config.h"
#include "Emu/Io/PadHandler.h"
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

	void Init(const u32 max_connect) override;
	void SetRumble(const u32 pad, u8 largeMotor, bool smallMotor) override;
	void Close();

private:
	typedef void (WINAPI * PFN_XINPUTENABLE)(BOOL);
	typedef DWORD (WINAPI * PFN_XINPUTGETSTATE)(DWORD, XINPUT_STATE *);
	typedef DWORD (WINAPI * PFN_XINPUTSETSTATE)(DWORD, XINPUT_VIBRATION *);

private:
	std::tuple<u16, u16> ConvertToSquirclePoint(u16 inX, u16 inY);
	DWORD ThreadProcedure();
	static DWORD WINAPI ThreadProcProxy(LPVOID parameter);

private:
	mutable bool active;
	float squircle_factor;
	u32 left_stick_deadzone, right_stick_deadzone;
	HANDLE thread;
	HMODULE library;
	PFN_XINPUTGETSTATE xinputGetState;
	PFN_XINPUTSETSTATE xinputSetState;
	PFN_XINPUTENABLE xinputEnable;

};

extern xinput_config xinput_cfg;

#endif
