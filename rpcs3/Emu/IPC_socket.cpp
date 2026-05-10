#include "stdafx.h"
#include "System.h"
#include "Emu/IPC_config.h"
#include "IPC_socket.h"
#include "rpcs3_version.h"


namespace IPC_socket
{
	const u8& IPC_impl::read8(u32 addr)
	{
		return vm::read8(addr);
	}

	void IPC_impl::write8(u32 addr, u8 value)
	{
		vm::write8(addr, value);
	}

	const be_t<u16>& IPC_impl::read16(u32 addr)
	{
		return vm::read16(addr);
	}

	void IPC_impl::write16(u32 addr, be_t<u16> value)
	{
		vm::write16(addr, value);
	}

	const be_t<u32>& IPC_impl::read32(u32 addr)
	{
		return vm::read32(addr);
	}

	void IPC_impl::write32(u32 addr, be_t<u32> value)
	{
		vm::write32(addr, value);
	}

	const be_t<u64>& IPC_impl::read64(u32 addr)
	{
		return vm::read64(addr);
	}

	void IPC_impl::write64(u32 addr, be_t<u64> value)
	{
		vm::write64(addr, value);
	}

	int IPC_impl::get_port()
	{
		return g_cfg_ipc.get_port();
	}

	pine::EmuStatus IPC_impl::get_status()
	{
		switch (Emu.GetStatus())
		{
		case system_state::running:
			return pine::EmuStatus::Running;
		case system_state::paused:
			return pine::EmuStatus::Paused;
		default:
			return pine::EmuStatus::Shutdown;
		}
	}

	const std::string& IPC_impl::get_title()
	{
		return Emu.GetTitle();
	}

	const std::string& IPC_impl::get_title_ID()
	{
		return Emu.GetTitleID();
	}

	const std::string& IPC_impl::get_executable_hash()
	{
		return Emu.GetExecutableHash();
	}

	const std::string& IPC_impl::get_app_version()
	{
		return Emu.GetAppVersion();
	}

	std::string IPC_impl::get_version_and_branch()
	{
		return rpcs3::get_version_and_branch();
	}

	IPC_impl& IPC_impl::operator=(thread_state)
	{
		return *this;
	}

	IPC_server_manager::IPC_server_manager(bool enabled)
	{
		// Enable IPC if needed
		set_server_enabled(enabled);
	}

	void IPC_server_manager::set_server_enabled(bool enabled)
	{
		if (enabled)
		{
			int port = g_cfg_ipc.get_port();
			if (!m_ipc_server || port != m_old_port)
			{
				IPC.notice("Starting server with port %d", port);
				m_ipc_server = std::make_unique<IPC_server>();
				m_old_port = port;
			}
		}
		else if (m_ipc_server)
		{
			IPC.notice("Stopping server");
			m_ipc_server.reset();
		}
	}
}
