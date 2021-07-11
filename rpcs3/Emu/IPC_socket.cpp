/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "IPC_socket.h"
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <sys/types.h>
#if _WIN32
#define read_portable(a, b, c) (recv(a, b, c, 0))
#define write_portable(a, b, c) (send(a, b, c, 0))
#define close_portable(a) (closesocket(a))
#define bzero(b, len) (memset((b), '\0', (len)), (void)0)
#include <WinSock2.h>
#include <windows.h>
#else
#define read_portable(a, b, c) (read(a, b, c))
#define write_portable(a, b, c) (write(a, b, c))
#define close_portable(a) (close(a))
#include <sys/socket.h>
#include <sys/un.h>
#if defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <unistd.h>
#endif
#endif

#include "util/logs.hpp"
#include "Emu/Memory/vm.h"
#include "System.h"
#include "Emu/system_config.h"
#include "rpcs3_version.h"

LOG_CHANNEL(IPC);

SocketIPC::SocketIPC() noexcept
{
#ifdef _WIN32
	WSADATA wsa;
	struct sockaddr_in server;
	m_sock = INVALID_SOCKET;
	m_msgsock = INVALID_SOCKET;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		IPC.error("IPC: Cannot initialize winsock! Shutting down...");
		return;
	}

	if ((m_sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		IPC.error("IPC: Cannot open socket! Shutting down...");
		return;
	}

	// yes very good windows s/sun/sin/g sure is fine
	server.sin_family = AF_INET;
	// localhost only
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(PORT);

	if (bind(m_sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		IPC.error("IPC: Error while binding to socket! Shutting down...");
		return;
	}

#else
	char* runtime_dir = nullptr;
#ifdef __APPLE__
	runtime_dir = std::getenv("TMPDIR");
#else
	runtime_dir = std::getenv("XDG_RUNTIME_DIR");
#endif
	// fallback in case macOS or other OSes don't implement the XDG base
	// spec
	if (runtime_dir == nullptr)
		m_socket_name = "/tmp/rpcs3.sock";
	else
	{
		m_socket_name = runtime_dir;
		m_socket_name += "/rpcs3.sock";
	}

	m_socket_name = fmt::format("%s.%d", m_socket_name, PORT);

	struct sockaddr_un server;

	m_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (m_sock < 0)
	{
		IPC.error("IPC: Cannot open socket! Shutting down...");
		return;
	}
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, m_socket_name.c_str());

	// we unlink the socket so that when releasing this thread the socket gets
	// freed even if we didn't close correctly the loop
	unlink(m_socket_name.c_str());
	if (bind(m_sock, std::bit_cast<struct sockaddr*>(&server), sizeof(struct sockaddr_un)))
	{
		IPC.error("IPC: Error while binding to socket! Shutting down...");
		return;
	}
#endif

	// maximum queue of 4096 commands before refusing, approximated to the
	// nearest legal value. We do not use SOMAXCONN as windows have this idea
	// that a "reasonable" value is 5, which is not.
	listen(m_sock, 4096);
}

char* SocketIPC::MakeOkIPC(char* ret_buffer, uint32_t size = 5)
{
	ToArray<uint32_t>(ret_buffer, size, 0);
	ret_buffer[4] = IPC_OK;
	return ret_buffer;
}

char* SocketIPC::MakeFailIPC(char* ret_buffer, uint32_t size = 5)
{
	ToArray<uint32_t>(ret_buffer, size, 0);
	ret_buffer[4] = IPC_FAIL;
	return ret_buffer;
}

int SocketIPC::StartSocket()
{
	m_msgsock = accept(m_sock, 0, 0);

	if (m_msgsock == -1)
	{
		// everything else is non recoverable in our scope
		// we also mark as recoverable socket errors where it would block a
		// non blocking socket, even though our socket is blocking, in case
		// we ever have to implement a non blocking socket.
#ifdef _WIN32
		int errno_w = WSAGetLastError();
		if (!(errno_w == WSAECONNRESET || errno_w == WSAEINTR || errno_w == WSAEINPROGRESS || errno_w == WSAEMFILE || errno_w == WSAEWOULDBLOCK))
		{
#else
		if (!(errno == ECONNABORTED || errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
		{
#endif
			IPC.error("IPC: An unrecoverable error happened! Shutting down...\n");
			return -1;
		}
	}
	return 0;
}

void SocketIPC::operator()()
{
	// we allocate once buffers to not have to do mallocs for each IPC
	// request, as malloc is expansive when we optimize for µs.
	m_ret_buffer = new char[MAX_IPC_RETURN_SIZE];
	m_ipc_buffer = new char[MAX_IPC_SIZE];

	if (StartSocket() < 0)
		return;

	while (thread_ctrl::state() != thread_state::aborting)
	{
		// either int or ssize_t depending on the platform, so we have to
		// use a bunch of auto
		auto receive_length = 0;
		auto end_length = 4;

		// while we haven't received the entire packet, maybe due to
		// socket datagram splittage, we continue to read
		while (receive_length < end_length)
		{
			auto tmp_length = read_portable(m_msgsock, &m_ipc_buffer[receive_length], MAX_IPC_SIZE - receive_length);

			// we recreate the socket if an error happens
			if (tmp_length <= 0)
			{
				receive_length = 0;
				if (StartSocket() < 0)
					return;
				break;
			}

			receive_length += tmp_length;

			// if we got at least the final size then update
			if (end_length == 4 && receive_length >= 4)
			{
				end_length = FromArray<u32>(m_ipc_buffer, 0);
				// we'd like to avoid a client trying to do OOB
				if (end_length > MAX_IPC_SIZE || end_length < 4)
				{
					receive_length = 0;
					break;
				}
			}
		}
		SocketIPC::IPCBuffer res;

		// we remove 4 bytes to get the message size out of the IPC command
		// size in ParseCommand.
		// also, if we got a failed command, let's reset the state so we don't
		// end up deadlocking by getting out of sync, eg when a client
		// disconnects
		if (receive_length != 0)
		{
			res = ParseCommand(&m_ipc_buffer[4], m_ret_buffer, static_cast<u32>(end_length) - 4);

			// if we cannot send back our answer restart the socket
			if (write_portable(m_msgsock, res.buffer, res.size) < 0)
			{
				if (StartSocket() < 0)
					return;
			}
		}
	}
	return;
}

void SocketIPC::Cleanup()
{
#ifdef _WIN32
	WSACleanup();
#else
	unlink(m_socket_name.c_str());
#endif
	close_portable(m_sock);
	close_portable(m_msgsock);
}

SocketIPC& SocketIPC::operator=(thread_state)
{
	Cleanup();
	return *this;
}

SocketIPC::~SocketIPC()
{
	Cleanup();
	delete[] m_ret_buffer;
	delete[] m_ipc_buffer;
}

SocketIPC::IPCBuffer SocketIPC::ParseCommand(char* buf, char* ret_buffer, u32 buf_size)
{
	u32 ret_cnt = 5;
	u32 buf_cnt = 0;

	while (buf_cnt < buf_size)
	{
		if (!SafetyChecks(buf_cnt, 1, ret_cnt, 0, buf_size))
			return IPCBuffer{ 5, MakeFailIPC(ret_buffer) };
		buf_cnt++;
		// example IPC messages: MsgRead/Write
		// refer to the client doc for more info on the format
		//         IPC Message event (1 byte)
		//         |  Memory address (4 byte)
		//         |  |           argument (VLE)
		//         |  |           |
		// format: XX YY YY YY YY ZZ ZZ ZZ ZZ
		//        reply code: 00 = OK, FF = NOT OK
		//        |  return value (VLE)
		//        |  |
		// reply: XX ZZ ZZ ZZ ZZ
		IPCCommand command = std::bit_cast<IPCCommand>(buf[buf_cnt - 1]);
		switch (command)
		{
		case MsgRead8:
		{
			if (!SafetyChecks(buf_cnt, 4, ret_cnt, 1, buf_size))
				goto error;
			const u32 a = FromArray<u32>(&buf[buf_cnt], 0);
			if (!vm::check_addr(a))
				goto error;
			const u8 res = vm::read8(a);
			ToArray(ret_buffer, res, ret_cnt);
			ret_cnt += 1;
			buf_cnt += 4;
			break;
		}
		case MsgRead16:
		{
			if (!SafetyChecks(buf_cnt, 4, ret_cnt, 2, buf_size))
				goto error;
			const u32 a = FromArray<u32>(&buf[buf_cnt], 0);
			if (!vm::check_addr<2>(a))
				goto error;
			const u16 res = vm::read16(a);
			ToArray(ret_buffer, res, ret_cnt);
			ret_cnt += 2;
			buf_cnt += 4;
			break;
		}
		case MsgRead32:
		{
			if (!SafetyChecks(buf_cnt, 4, ret_cnt, 4, buf_size))
				goto error;
			const u32 a = FromArray<u32>(&buf[buf_cnt], 0);
			if (!vm::check_addr<4>(a))
				goto error;
			const u32 res = vm::read32(a);
			ToArray(ret_buffer, res, ret_cnt);
			ret_cnt += 4;
			buf_cnt += 4;
			break;
		}
		case MsgRead64:
		{
			if (!SafetyChecks(buf_cnt, 4, ret_cnt, 8, buf_size))
				goto error;
			const u32 a = FromArray<u32>(&buf[buf_cnt], 0);
			if (!vm::check_addr<8>(a))
				goto error;
			u64 res = vm::read64(a);
			ToArray(ret_buffer, res, ret_cnt);
			ret_cnt += 8;
			buf_cnt += 4;
			break;
		}
		case MsgWrite8:
		{
			if (!SafetyChecks(buf_cnt, 1 + 4, ret_cnt, 0, buf_size))
				goto error;
			const u32 a = FromArray<u32>(&buf[buf_cnt], 0);
			if (!vm::check_addr(a, vm::page_writable))
				goto error;
			vm::write8(a, FromArray<u8>(&buf[buf_cnt], 4));
			buf_cnt += 5;
			break;
		}
		case MsgWrite16:
		{
			if (!SafetyChecks(buf_cnt, 2 + 4, ret_cnt, 0, buf_size))
				goto error;
			const u32 a = FromArray<u32>(&buf[buf_cnt], 0);
			if (!vm::check_addr<2>(a, vm::page_writable))
				goto error;
			vm::write16(a, FromArray<u16>(&buf[buf_cnt], 4));
			buf_cnt += 6;
			break;
		}
		case MsgWrite32:
		{
			if (!SafetyChecks(buf_cnt, 4 + 4, ret_cnt, 0, buf_size))
				goto error;
			const u32 a = FromArray<u32>(&buf[buf_cnt], 0);
			if (!vm::check_addr<4>(a, vm::page_writable))
				goto error;
			vm::write32(a, FromArray<u32>(&buf[buf_cnt], 4));
			buf_cnt += 8;
			break;
		}
		case MsgWrite64:
		{
			if (!SafetyChecks(buf_cnt, 8 + 4, ret_cnt, 0, buf_size))
				goto error;
			const u32 a = FromArray<u32>(&buf[buf_cnt], 0);
			if (!vm::check_addr<8>(a, vm::page_writable))
				goto error;
			vm::write64(a, FromArray<u64>(&buf[buf_cnt], 4));
			buf_cnt += 12;
			break;
		}
		case MsgVersion:
		{
			char version[256] = {};
			sprintf(version, "RPCS3 %s", rpcs3::get_version_and_branch().c_str());
			const u32 size = strlen(version) + 1;
			version[size] = 0x00;
			if (!SafetyChecks(buf_cnt, 0, ret_cnt, size + 4, buf_size))
				goto error;
			ToArray(ret_buffer, size, ret_cnt);
			ret_cnt += 4;
			memcpy(&ret_buffer[ret_cnt], version, size);
			ret_cnt += size;
			break;
		}
		case MsgStatus:
		{
			if (!SafetyChecks(buf_cnt, 0, ret_cnt, 4, buf_size))
				goto error;
			EmuStatus res;
			switch (Emu.GetStatus())
			{
			case system_state::running:
				res = EmuStatus::Running;
				break;
			case system_state::paused:
				res = EmuStatus::Paused;
				break;
			default:
				res = EmuStatus::Shutdown;
				break;
			}
			ToArray(ret_buffer, res, ret_cnt);
			ret_cnt += 4;
			buf_cnt += 4;
			break;
		}
		case MsgTitle:
		{
			const auto title_string = Emu.GetTitle();
			const auto size = title_string.size() + 1;
			char* title = new char[size];
			sprintf(title, "%s", title_string.c_str());
			title[size] = 0x00;
			if (!SafetyChecks(buf_cnt, 0, ret_cnt, size + 4, buf_size))
				goto error;
			ToArray(ret_buffer, size, ret_cnt);
			ret_cnt += 4;
			memcpy(&ret_buffer[ret_cnt], title, size);
			ret_cnt += size;
			delete[] title;
			break;
		}
		case MsgID:
		{
			const auto title_id_string = Emu.GetTitleID();
			const auto size = title_id_string.size() + 1;
			char* title_id = new char[size];
			sprintf(title_id, "%s", title_id_string.c_str());
			title_id[size] = 0x00;
			if (!SafetyChecks(buf_cnt, 0, ret_cnt, size + 4, buf_size))
				goto error;
			ToArray(ret_buffer, size, ret_cnt);
			ret_cnt += 4;
			memcpy(&ret_buffer[ret_cnt], title_id, size);
			ret_cnt += size;
			delete[] title_id;
			break;
		}
		case MsgUUID:
		{
			const auto hash_string = Emu.GetExecutableHash();
			const auto size = hash_string.size() + 1;
			char* hash = new char[size];
			sprintf(hash, "%s", hash_string.c_str());
			hash[size] = 0x00;
			if (!SafetyChecks(buf_cnt, 0, ret_cnt, size + 4, buf_size))
				goto error;
			ToArray(ret_buffer, size, ret_cnt);
			ret_cnt += 4;
			memcpy(&ret_buffer[ret_cnt], hash, size);
			ret_cnt += size;
			delete[] hash;
			break;
		}
		case MsgGameVersion:
		{
			const auto game_version_string = Emu.GetAppVersion();
			const auto size = game_version_string.size() + 1;
			char* game_version = new char[size];
			sprintf(game_version, "%s", game_version_string.c_str());
			game_version[size] = 0x00;
			if (!SafetyChecks(buf_cnt, 0, ret_cnt, size, buf_size))
				goto error;
			ToArray(ret_buffer, size, ret_cnt);
			ret_cnt += 4;
			memcpy(&ret_buffer[ret_cnt], game_version, size);
			ret_cnt += size;
			delete[] game_version;
			break;
		}
		default:
		{
		error:
			return IPCBuffer{ 5, MakeFailIPC(ret_buffer) };
		}
		}
	}
	return IPCBuffer{ static_cast<int>(ret_cnt), MakeOkIPC(ret_buffer, ret_cnt) };
}
