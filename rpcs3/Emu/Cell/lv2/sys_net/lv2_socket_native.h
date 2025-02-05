
#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#endif

#include "lv2_socket.h"

class lv2_socket_native final : public lv2_socket
{
public:
	static constexpr u32 id_type = 1;

	lv2_socket_native(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol);
	lv2_socket_native(utils::serial& ar, lv2_socket_type type);
	~lv2_socket_native() noexcept override;
	void save(utils::serial& ar);
	s32 create_socket();

	std::tuple<bool, s32, shared_ptr<lv2_socket>, sys_net_sockaddr> accept(bool is_lock = true) override;
	s32 bind(const sys_net_sockaddr& addr) override;

	std::optional<s32> connect(const sys_net_sockaddr& addr) override;
	s32 connect_followup() override;

	std::pair<s32, sys_net_sockaddr> getpeername() override;
	std::pair<s32, sys_net_sockaddr> getsockname() override;
	std::tuple<s32, sockopt_data, u32> getsockopt(s32 level, s32 optname, u32 len) override;
	s32 setsockopt(s32 level, s32 optname, const std::vector<u8>& optval) override;
	std::optional<std::tuple<s32, std::vector<u8>, sys_net_sockaddr>> recvfrom(s32 flags, u32 len, bool is_lock = true) override;
	std::optional<s32> sendto(s32 flags, const std::vector<u8>& buf, std::optional<sys_net_sockaddr> opt_sn_addr, bool is_lock = true) override;
	std::optional<s32> sendmsg(s32 flags, const sys_net_msghdr& msg, bool is_lock = true) override;

	s32 poll(sys_net_pollfd& sn_pfd, pollfd& native_pfd) override;
	std::tuple<bool, bool, bool> select(bs_t<poll_t> selected, pollfd& native_pfd) override;

	bool is_socket_connected();

	s32 listen(s32 backlog) override;
	void close() override;
	s32 shutdown(s32 how) override;

private:
	void set_socket(socket_type native_socket, lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol);
	void set_default_buffers();
	void set_non_blocking();

private:
	// Value keepers
#ifdef _WIN32
	s32 so_reuseaddr = 0;
	s32 so_reuseport = 0;
#endif
	u16 bound_port = 0;
	bool feign_tcp_conn_failure = false; // Savestate load related
};
