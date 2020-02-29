#include "headless_application.h"

#include "Emu/RSX/GSRender.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/Cell/Modules/cellOskDialog.h"
#include "Emu/Cell/Modules/cellSaveData.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"

#include <clocale>

// For now, a trivial constructor/destructor. May add command line usage later.
headless_application::headless_application(int& argc, char** argv) : QCoreApplication(argc, argv)
{
}

void headless_application::Init()
{
	// Force init the emulator
	InitializeEmulator("1", true, false); // TODO: get user from cli args if possible

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	// As per QT recommendations to avoid conflicts for POSIX functions
	std::setlocale(LC_NUMERIC, "C");
}

void headless_application::InitializeConnects()
{
	qRegisterMetaType<std::function<void()>>("std::function<void()>");
	connect(this, &headless_application::RequestCallAfter, this, &headless_application::HandleCallAfter);
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here. */
void headless_application::InitializeCallbacks()
{
	EmuCallbacks callbacks = CreateCallbacks();

	callbacks.exit = [this](bool force_quit)
	{
		if (force_quit)
		{
			quit();
		}
	};
	callbacks.call_after = [=, this](std::function<void()> func)
	{
		RequestCallAfter(std::move(func));
	};

	callbacks.get_gs_frame                   = []() -> std::unique_ptr<GSFrameBase> { return std::unique_ptr<GSFrameBase>(); };
	callbacks.get_msg_dialog                 = []() -> std::shared_ptr<MsgDialogBase> { return std::shared_ptr<MsgDialogBase>(); };
	callbacks.get_osk_dialog                 = []() -> std::shared_ptr<OskDialogBase> { return std::shared_ptr<OskDialogBase>(); };
	callbacks.get_save_dialog                = []() -> std::unique_ptr<SaveDialogBase> { return std::unique_ptr<SaveDialogBase>(); };
	callbacks.get_trophy_notification_dialog = []() -> std::unique_ptr<TrophyNotificationBase> { return std::unique_ptr<TrophyNotificationBase>(); };

	callbacks.on_run    = [](bool /*start_playtime*/) {};
	callbacks.on_pause  = []() {};
	callbacks.on_resume = []() {};
	callbacks.on_stop   = []() {};
	callbacks.on_ready  = []() {};

	Emu.SetCallbacks(std::move(callbacks));
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread. So, even if this looks stupid to just call func, it's succinct.
 */
void headless_application::HandleCallAfter(const std::function<void()>& func)
{
	func();
}
