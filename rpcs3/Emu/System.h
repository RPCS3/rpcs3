#pragma once

#include "VFS.h"
#include "DbgCommand.h"

enum class frame_type;

struct EmuCallbacks
{
	std::function<void(std::function<void()>)> call_after;
	std::function<void()> process_events;
	std::function<void()> exit;
	std::function<void(DbgCommand, class cpu_thread*)> send_dbg_command;
	std::function<std::shared_ptr<class KeyboardHandlerBase>()> get_kb_handler;
	std::function<std::shared_ptr<class MouseHandlerBase>()> get_mouse_handler;
	std::function<std::shared_ptr<class PadHandlerBase>()> get_pad_handler;
	std::function<std::unique_ptr<class GSFrameBase>(frame_type, int, int)> get_gs_frame;
	std::function<std::shared_ptr<class GSRender>()> get_gs_render;
	std::function<std::shared_ptr<class AudioThread>()> get_audio;
	std::function<std::shared_ptr<class MsgDialogBase>()> get_msg_dialog;
	std::function<std::unique_ptr<class SaveDialogBase>()> get_save_dialog;
};

enum Status : u32
{
	Running,
	Paused,
	Stopped,
	Ready,
};

// Emulation Stopped exception event
class EmulationStopped {};

class Emulator final
{
	atomic_t<u32> m_status;

	EmuCallbacks m_cb;

	atomic_t<u64> m_pause_start_time; // set when paused
	atomic_t<u64> m_pause_amend_time; // increased when resumed

	u32 m_cpu_thr_stop;

	std::string m_path;
	std::string m_elf_path;
	std::string m_title_id;
	std::string m_title;

public:
	Emulator();

	void SetCallbacks(EmuCallbacks&& cb)
	{
		m_cb = std::move(cb);
	}

	const auto& GetCallbacks() const
	{
		return m_cb;
	}

	void SendDbgCommand(DbgCommand cmd, class cpu_thread* thread = nullptr)
	{
		if (m_cb.send_dbg_command) m_cb.send_dbg_command(cmd, thread);
	}

	// Call from the GUI thread
	void CallAfter(std::function<void()>&& func) const
	{
		return m_cb.call_after(std::move(func));
	}

	/** Set emulator mode to running unconditionnaly.
	 * Required to execute various part (PPUInterpreter, memory manager...) outside of rpcs3.
	 */
	void SetTestMode()
	{
		m_status = Running;
	}

	void Init();
	void SetPath(const std::string& path, const std::string& elf_path = {});

	const std::string& GetPath() const
	{
		return m_elf_path;
	}

	const std::string& GetTitleID() const
	{
		return m_title_id;
	}

	const std::string& GetTitle() const
	{
		return m_title;
	}

	u64 GetPauseTime()
	{
		return m_pause_amend_time;
	}

	void SetCPUThreadStop(u32 addr)
	{
		m_cpu_thr_stop = addr;
	}

	u32 GetCPUThreadStop() const { return m_cpu_thr_stop; }

	bool BootGame(const std::string& path, bool direct = false);

	static std::string GetGameDir();
	static std::string GetLibDir();

	void Load();
	void Run();
	bool Pause();
	void Resume();
	void Stop();

	force_inline bool IsRunning() const { return m_status == Running; }
	force_inline bool IsPaused()  const { return m_status == Paused; }
	force_inline bool IsStopped() const { return m_status == Stopped; }
	force_inline bool IsReady()   const { return m_status == Ready; }
};

extern Emulator Emu;

#define CHECK_EMU_STATUS if (Emu.IsStopped()) throw EmulationStopped{}
