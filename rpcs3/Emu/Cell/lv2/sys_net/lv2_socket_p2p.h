#pragma once

#include "lv2_socket.h"

class lv2_socket_p2p : public lv2_socket
{
public:
	lv2_socket_p2p(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol);
	lv2_socket_p2p(utils::serial& ar, lv2_socket_type type);
	void save(utils::serial& ar);

	std::tuple<bool, s32, shared_ptr<lv2_socket>, sys_net_sockaddr> accept(bool is_lock = true) override;
	s32 bind(const sys_net_sockaddr& addr) override;

	std::optional<s32> connect(const sys_net_sockaddr& addr) override;
	s32 connect_followup() override;

	std::pair<s32, sys_net_sockaddr> getpeername() override;
	std::pair<s32, sys_net_sockaddr> getsockname() override;

	std::tuple<s32, sockopt_data, u32> getsockopt(s32 level, s32 optname, u32 len) override;
	s32 setsockopt(s32 level, s32 optname, const std::vector<u8>& optval) override;

	s32 listen(s32 backlog) override;

	std::optional<std::tuple<s32, std::vector<u8>, sys_net_sockaddr>> recvfrom(s32 flags, u32 len, bool is_lock = true) override;
	std::optional<s32> sendto(s32 flags, const std::vector<u8>& buf, std::optional<sys_net_sockaddr> opt_sn_addr, bool is_lock = true) override;
	std::optional<s32> sendmsg(s32 flags, const sys_net_msghdr& msg, bool is_lock = true) override;

	void close() override;
	s32 shutdown(s32 how) override;

	s32 poll(sys_net_pollfd& sn_pfd, pollfd& native_pfd) override;
	std::tuple<bool, bool, bool> select(bs_t<poll_t> selected, pollfd& native_pfd) override;

	void handle_new_data(sys_net_sockaddr_in_p2p p2p_addr, std::vector<u8> p2p_data);

protected:
	// Port(actual bound port) and Virtual Port(indicated by u16 at the start of the packet)
	u16 port = 3658, vport = 0;
	u32 bound_addr = 0;
	// Queue containing received packets from network_thread for SYS_NET_SOCK_DGRAM_P2P sockets
	std::queue<std::pair<sys_net_sockaddr_in_p2p, std::vector<u8>>> data{};
	// List of sock options
	std::map<u64, sockopt_cache> sockopts;
};
