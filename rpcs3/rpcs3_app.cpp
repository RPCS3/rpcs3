#include "rpcs3_app.h"

#include "Emu/System.h"

// For now, a trivial constructor/destructor.  May add command line usage later.
rpcs3_app::rpcs3_app(int& argc, char** argv) : QCoreApplication(argc, argv)
{
}

void rpcs3_app::Init()
{
	// Force init the emulator
	InitializeEmulator("1", true); // TODO: get user from cli args if possible

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();
}

void rpcs3_app::InitializeConnects()
{
	qRegisterMetaType<std::function<void()>>("std::function<void()>");
	connect(this, &rpcs3_app::RequestCallAfter, this, &rpcs3_app::HandleCallAfter);
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here. */
void rpcs3_app::InitializeCallbacks()
{
	EmuCallbacks callbacks = CreateCallbacks();

	callbacks.exit = [this]()
	{
		quit();
	};
	callbacks.call_after = [=](std::function<void()> func)
	{
		RequestCallAfter(std::move(func));
	};

	callbacks.get_gs_frame                   = []() -> std::unique_ptr<GSFrameBase> { return std::unique_ptr<GSFrameBase>(); };
	callbacks.get_msg_dialog                 = []() -> std::shared_ptr<MsgDialogBase> { return std::shared_ptr<MsgDialogBase>(); };
	callbacks.get_osk_dialog                 = []() -> std::shared_ptr<OskDialogBase> { return std::shared_ptr<OskDialogBase>(); };
	callbacks.get_save_dialog                = []() -> std::unique_ptr<SaveDialogBase> { return std::unique_ptr<SaveDialogBase>(); };
	callbacks.get_trophy_notification_dialog = []() -> std::unique_ptr<TrophyNotificationBase> { return std::unique_ptr<TrophyNotificationBase>(); };

	callbacks.on_run    = []() {};
	callbacks.on_pause  = []() {};
	callbacks.on_resume = []() {};
	callbacks.on_stop   = []() {};
	callbacks.on_ready  = []() {};

	Emu.SetCallbacks(std::move(callbacks));
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread. So, even if this looks stupid to just call func, it's succinct.
 */
void rpcs3_app::HandleCallAfter(const std::function<void()>& func)
{
	func();
}
