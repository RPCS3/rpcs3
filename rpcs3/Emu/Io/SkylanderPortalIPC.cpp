#include "stdafx.h"
#include "SkylanderPortalIPC.h"
#include "SkylanderPortalIPC_config.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
using sky_socket_t = SOCKET;
static constexpr sky_socket_t sky_invalid_socket = INVALID_SOCKET;
static int sky_recv(sky_socket_t s, char* buf, int len) { return recv(s, buf, len, 0); }
static int sky_send_some(sky_socket_t s, const char* buf, int len) { return send(s, buf, len, 0); }
static void sky_close(sky_socket_t s) { closesocket(s); }
static bool sky_interrupted() { return WSAGetLastError() == WSAEINTR; }

static bool sky_set_recv_timeout(sky_socket_t s, u32 timeout_ms)
{
	const DWORD timeout = timeout_ms;
	return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0;
}
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
using sky_socket_t = int;
static constexpr sky_socket_t sky_invalid_socket = -1;
static int sky_recv(sky_socket_t s, char* buf, int len) { return static_cast<int>(read(s, buf, len)); }
static int sky_send_some(sky_socket_t s, const char* buf, int len) { return static_cast<int>(write(s, buf, len)); }
static void sky_close(sky_socket_t s) { close(s); }
static bool sky_interrupted() { return errno == EINTR; }

static bool sky_set_recv_timeout(sky_socket_t s, u32 timeout_ms)
{
	timeval timeout{};
	timeout.tv_sec  = static_cast<time_t>(timeout_ms / 1000);
	timeout.tv_usec = static_cast<suseconds_t>(timeout_ms % 1000 * 1000);
	return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0;
}
#endif

#include "Skylander.h"
#include "3rdparty/pine/pine_server.h"

#include <chrono>

LOG_CHANNEL(skylander_ipc_log, "SkylanderIPC");

static constexpr usz max_command_size = 4096;
static constexpr u32 client_timeout_ms = 1000;

static bool is_absolute_local_path(std::string_view path)
{
#ifdef _WIN32
	// Requiring a drive letter also rules out network (\\server\share) and device (\\.\) paths
	const bool has_drive_letter = path.size() >= 3 && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && path[1] == ':';
	return has_drive_letter && (path[2] == '\\' || path[2] == '/');
#else
	return path.starts_with('/');
#endif
}

static std::string handle_load(int slot, const std::string& path)
{
	if (slot < -1 || slot > 7)
		return "error invalid slot\n";

	if (!is_absolute_local_path(path))
		return "error path must be absolute\n";

	std::array<u8, 0x40 * 0x10> buf{};

	// Checked before opening, so that devices and pipes are rejected without ever being opened
	fs::stat_t info{};
	if (!fs::get_stat(path, info) || info.is_directory || info.size != buf.size())
		return "error not a skylander file\n";

	fs::file sky_file(path, fs::read + fs::write + fs::lock);
	if (!sky_file)
		return "error cannot open file\n";

	if (sky_file.read(buf.data(), buf.size()) != buf.size())
		return "error file too small\n";

	if (read_from_ptr<le_t<u16>>(buf, 0x1E) != skylander_crc16(0xFFFF, buf.data(), 0x1E))
		return "error not a skylander file\n";

	const u8 result = g_skyportal.load_skylander(buf, std::move(sky_file), slot);
	if (result == 0xFF)
		return "error no free slot\n";

	return "ok " + std::to_string(result) + "\n";
}

static std::string handle_remove(int slot)
{
	if (slot < 0 || slot > 7)
		return "error invalid slot\n";

	if (!g_skyportal.remove_skylander(static_cast<u8>(slot)))
		return "error slot empty\n";

	return "ok\n";
}

static std::string handle_status()
{
	std::string result;
	for (u8 i = 0; i < 8; i++)
	{
		u8 status;
		u16 id, variant;
		g_skyportal.get_figure_info(i, status, id, variant);
		if (status & 1)
			result += "slot" + std::to_string(i) + " loaded " + std::to_string(id) + " " + std::to_string(variant) + "\n";
		else
			result += "slot" + std::to_string(i) + " empty\n";
	}
	result += "ok\n";
	return result;
}

static std::string handle_clear()
{
	for (u8 i = 0; i < 8; i++)
		g_skyportal.remove_skylander(i);
	return "ok\n";
}

static bool parse_int(const std::string& s, int& out)
{
	if (s.empty())
		return false;
	char* end;
	errno = 0;
	const long val = std::strtol(s.c_str(), &end, 10);
	if (end == s.c_str() || *end != '\0' || errno != 0)
		return false;
	out = static_cast<int>(val);
	return true;
}

static std::string process_command(const std::string& line)
{
	if (line.empty())
		return "error empty command\n";

	const usz first_space = line.find(' ');
	const std::string cmd  = line.substr(0, first_space);

	if (cmd == "status")
		return handle_status();

	if (cmd == "clear")
		return handle_clear();

	if (cmd == "remove")
	{
		if (first_space == std::string::npos)
			return "error missing slot\n";
		int slot;
		if (!parse_int(line.substr(first_space + 1), slot))
			return "error invalid slot\n";
		return handle_remove(slot);
	}

	if (cmd == "load")
	{
		if (first_space == std::string::npos)
			return "error missing arguments\n";
		const std::string rest  = line.substr(first_space + 1);
		const usz second_space  = rest.find(' ');
		if (second_space == std::string::npos)
			return "error missing path\n";
		int slot;
		if (!parse_int(rest.substr(0, second_space), slot))
			return "error invalid arguments\n";
		const std::string path = rest.substr(second_space + 1);
		return handle_load(slot, path);
	}

	return "error unknown command\n";
}

static bool sky_send(sky_socket_t s, std::string_view data)
{
	while (!data.empty())
	{
		const int sent = sky_send_some(s, data.data(), ::narrow<int>(data.size()));

		if (sent > 0)
			data.remove_prefix(sent);
		else if (sent < 0 && sky_interrupted())
			continue;
		else
			return false;
	}

	return true;
}

static bool read_command(sky_socket_t client_sock, std::string& line)
{
	if (!sky_set_recv_timeout(client_sock, client_timeout_ms))
		return false;

	const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(client_timeout_ms);

	while (line.size() < max_command_size && std::chrono::steady_clock::now() < deadline)
	{
		char ch;
		const int received = sky_recv(client_sock, &ch, 1);

		if (received == 0)
			return true;

		if (received < 0)
		{
			if (sky_interrupted())
				continue;

			return false;
		}

		if (ch == '\n' || ch == '\r')
			return true;

		line += ch;
	}

	return false;
}

void SkylanderPortalIPCServer::operator()()
{
	const int port = g_cfg_sky_ipc.get_port();

#ifdef _WIN32
	WSADATA wsa{};
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		skylander_ipc_log.error("Cannot initialize winsock");
		return;
	}

	const sky_socket_t listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == sky_invalid_socket)
	{
		skylander_ipc_log.error("Cannot create socket");
		WSACleanup();
		return;
	}

	if (const char* failure = pine::bind_and_listen(listen_sock, static_cast<u16>(port)))
	{
		skylander_ipc_log.error("Cannot start server on port %d: %s", port, failure);
		sky_close(listen_sock);
		WSACleanup();
		return;
	}

	skylander_ipc_log.notice("Server started on port %d", port);
#else
	const std::string socket_path = pine::get_socket_path("rpcs3.skylanders.sock", port, g_cfg_sky_ipc.sky_ipc_port.def);

	const sky_socket_t listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listen_sock == sky_invalid_socket)
	{
		skylander_ipc_log.error("Cannot create socket");
		return;
	}

	if (const char* failure = pine::bind_and_listen(listen_sock, socket_path))
	{
		skylander_ipc_log.error("Cannot start server at %s: %s", socket_path, failure);
		sky_close(listen_sock);
		return;
	}

	skylander_ipc_log.notice("Server started at %s", socket_path);
#endif

	while (thread_ctrl::state() != thread_state::aborting)
	{
		pollfd pfd{};
		pfd.fd     = listen_sock;
		pfd.events = POLLIN;

#ifdef _WIN32
		const int poll_result = WSAPoll(&pfd, 1, 10);
#else
		const int poll_result = poll(&pfd, 1, 10);
#endif
		if (poll_result < 0 && !sky_interrupted())
		{
			skylander_ipc_log.error("Waiting for connections failed, stopping server");
			break;
		}

		if (poll_result <= 0)
			continue;

		const sky_socket_t client_sock = accept(listen_sock, nullptr, nullptr);
		if (client_sock == sky_invalid_socket)
			continue;

		std::string line;
		if (read_command(client_sock, line) && !line.empty())
		{
			if (!sky_send(client_sock, process_command(line)))
				skylander_ipc_log.warning("Failed to send the response");
		}

		sky_close(client_sock);
	}

	sky_close(listen_sock);
#ifdef _WIN32
	WSACleanup();
#else
	unlink(socket_path.c_str());
#endif

	skylander_ipc_log.notice("Server stopped");
}

SkylanderPortalIPCServer& SkylanderPortalIPCServer::operator=(thread_state)
{
	return *this;
}

SkylanderPortalIPCServerManager::SkylanderPortalIPCServerManager(bool enabled)
{
	set_server_enabled(enabled);
}

void SkylanderPortalIPCServerManager::set_server_enabled(bool enabled)
{
	if (enabled)
	{
		const int port = g_cfg_sky_ipc.get_port();
		if (!m_server || port != m_old_port)
		{
			skylander_ipc_log.notice("Starting server with port %d", port);
			m_server = std::make_unique<server_thread>();
			m_old_port = port;
		}
	}
	else if (m_server)
	{
		skylander_ipc_log.notice("Stopping server");
		m_server.reset();
	}
}
