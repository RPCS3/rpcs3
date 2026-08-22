#include "stdafx.h"
#include "SkylanderPortalIPC.h"
#include "SkylanderPortalIPC_config.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
using sky_socket_t = SOCKET;
static constexpr sky_socket_t sky_invalid_socket = INVALID_SOCKET;
static int sky_recv(sky_socket_t s, char* buf, int len) { return recv(s, buf, len, 0); }
static int sky_send(sky_socket_t s, const char* buf, int len) { return send(s, buf, len, 0); }
static void sky_close(sky_socket_t s) { closesocket(s); }
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
using sky_socket_t = int;
static constexpr sky_socket_t sky_invalid_socket = -1;
static int sky_recv(sky_socket_t s, char* buf, int len) { return static_cast<int>(read(s, buf, len)); }
static int sky_send(sky_socket_t s, const char* buf, int len) { return static_cast<int>(write(s, buf, len)); }
static void sky_close(sky_socket_t s) { close(s); }
#endif

#include "Skylander.h"

LOG_CHANNEL(skylander_ipc_log, "SkylanderIPC");

static std::string handle_load(int slot, const std::string& path)
{
	if (slot < -1 || slot > 7)
		return "error invalid slot\n";

	fs::file sky_file(path, fs::read + fs::write + fs::lock);
	if (!sky_file)
		return "error cannot open file\n";

	std::array<u8, 0x40 * 0x10> buf{};
	if (sky_file.read(buf.data(), buf.size()) != buf.size())
		return "error file too small\n";

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

void SkylanderPortalIPCServer::operator()()
{
	sky_socket_t listen_sock = sky_invalid_socket;
	const int port = g_cfg_sky_ipc.get_port();

#ifdef _WIN32
	WSADATA wsa{};
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		skylander_ipc_log.error("Cannot initialize winsock");
		return;
	}

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == sky_invalid_socket)
	{
		skylander_ipc_log.error("Cannot create socket");
		WSACleanup();
		return;
	}

	sockaddr_in server{};
	server.sin_family = AF_INET;
	if (!inet_pton(AF_INET, "127.0.0.1", &server.sin_addr.s_addr))
	{
		skylander_ipc_log.error("inet_pton failed");
		sky_close(listen_sock);
		WSACleanup();
		return;
	}
	server.sin_port = htons(static_cast<u_short>(port));

	if (bind(listen_sock, reinterpret_cast<sockaddr*>(&server), sizeof(server)) == SOCKET_ERROR)
	{
		skylander_ipc_log.error("Cannot bind on port %d (already in use?)", port);
		sky_close(listen_sock);
		WSACleanup();
		return;
	}
#else
	const std::string socket_path = "/tmp/rpcs3.skylanders.sock";
	unlink(socket_path.c_str());

	listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listen_sock == sky_invalid_socket)
	{
		skylander_ipc_log.error("Cannot create socket");
		return;
	}

	sockaddr_un server{};
	server.sun_family = AF_UNIX;
	strncpy(server.sun_path, socket_path.c_str(), sizeof(server.sun_path) - 1);

	if (bind(listen_sock, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0)
	{
		skylander_ipc_log.error("Cannot bind socket at %s", socket_path);
		sky_close(listen_sock);
		return;
	}
#endif

	// Maximum queue before refusing; SOMAXCONN is unreliable on Windows
	listen(listen_sock, 4096);
	skylander_ipc_log.notice("Server started (port %d)", port);

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
		if (poll_result <= 0)
			continue;

		const sky_socket_t client_sock = accept(listen_sock, nullptr, nullptr);
		if (client_sock == sky_invalid_socket)
			continue;

		// Read one newline-terminated command (max 4 KB)
		std::string line;
		char ch;
		while (line.size() < 4096)
		{
			if (sky_recv(client_sock, &ch, 1) <= 0)
				break;
			if (ch == '\n' || ch == '\r')
				break;
			line += ch;
		}

		if (!line.empty())
		{
			const std::string response = process_command(line);
			sky_send(client_sock, response.c_str(), static_cast<int>(response.size()));
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
