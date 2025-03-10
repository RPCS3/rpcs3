#pragma once

#include "Utilities/Thread.h"
#include "util/logs.hpp"
#include "Emu/Memory/vm.h"
#include "3rdparty/pine/pine_server.h"

LOG_CHANNEL(IPC);

namespace IPC_socket
{
	class IPC_impl
	{
	protected:
		template <u32 Size = 1>
		static bool check_addr(u32 addr, u8 flags = vm::page_readable)
		{
			return vm::check_addr<Size>(addr, flags);
		}

		static const u8& read8(u32 addr);
		static void write8(u32 addr, u8 value);
		static const be_t<u16>& read16(u32 addr);
		static void write16(u32 addr, be_t<u16> value);
		static const be_t<u32>& read32(u32 addr);
		static void write32(u32 addr, be_t<u32> value);
		static const be_t<u64>& read64(u32 addr);
		static void write64(u32 addr, be_t<u64> value);

		template<typename... Args>
		static void error(const const_str& fmt, Args&&... args)
		{
			IPC.error(fmt, std::forward<Args>(args)...);
		}

		static int get_port();
		static pine::EmuStatus get_status();
		static const std::string& get_title();
		static const std::string& get_title_ID();
		static const std::string& get_executable_hash();
		static const std::string& get_app_version();
		static std::string get_version_and_branch();

	public:
		static auto constexpr thread_name = "IPC Server"sv;
		IPC_impl& operator=(thread_state);
	};

	class IPC_server_manager
	{
		using IPC_server = named_thread<pine::pine_server<IPC_socket::IPC_impl>>;

		std::unique_ptr<IPC_server> m_ipc_server;
		int m_old_port = 0;

	public:
		explicit IPC_server_manager(bool enabled);
		void set_server_enabled(bool enabled);
	};
}
