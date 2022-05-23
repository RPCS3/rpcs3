#include "stdafx.h"
#include "lv2_socket_raw.h"

LOG_CHANNEL(sys_net);

lv2_socket_raw::lv2_socket_raw(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol)
	: lv2_socket(family, type, protocol)
{
}

std::tuple<bool, s32, std::shared_ptr<lv2_socket>, sys_net_sockaddr> lv2_socket_raw::accept([[maybe_unused]] bool is_lock)
{
	sys_net.todo("lv2_socket_raw::accept");
	return {};
}

s32 lv2_socket_raw::bind([[maybe_unused]] const sys_net_sockaddr& addr, [[maybe_unused]] s32 ps3_id)
{
	sys_net.todo("lv2_socket_raw::bind");
	return {};
}

std::optional<s32> lv2_socket_raw::connect([[maybe_unused]] const sys_net_sockaddr& addr)
{
	sys_net.todo("lv2_socket_raw::connect");
	return CELL_OK;
}

s32 lv2_socket_raw::connect_followup()
{
	sys_net.todo("lv2_socket_raw::connect_followup");
	return CELL_OK;
}

std::pair<s32, sys_net_sockaddr> lv2_socket_raw::getpeername()
{
	sys_net.todo("lv2_socket_raw::getpeername");
	return {};
}

std::pair<s32, sys_net_sockaddr> lv2_socket_raw::getsockname()
{
	sys_net.todo("lv2_socket_raw::getsockname");
	return {};
}

std::tuple<s32, lv2_socket::sockopt_data, u32> lv2_socket_raw::getsockopt([[maybe_unused]] s32 level, [[maybe_unused]] s32 optname, [[maybe_unused]] u32 len)
{
	sys_net.todo("lv2_socket_raw::getsockopt");
	return {};
}

s32 lv2_socket_raw::setsockopt([[maybe_unused]] s32 level, [[maybe_unused]] s32 optname, [[maybe_unused]] const std::vector<u8>& optval)
{
	sys_net.todo("lv2_socket_raw::setsockopt");
	return {};
}

s32 lv2_socket_raw::listen([[maybe_unused]] s32 backlog)
{
	sys_net.todo("lv2_socket_raw::listen");
	return {};
}

std::optional<std::tuple<s32, std::vector<u8>, sys_net_sockaddr>> lv2_socket_raw::recvfrom([[maybe_unused]] s32 flags, [[maybe_unused]] u32 len, [[maybe_unused]] bool is_lock)
{
	sys_net.todo("lv2_socket_raw::recvfrom");
	return {{{}, {}, {}}};
}

std::optional<s32> lv2_socket_raw::sendto([[maybe_unused]] s32 flags, [[maybe_unused]] const std::vector<u8>& buf, [[maybe_unused]] std::optional<sys_net_sockaddr> opt_sn_addr, [[maybe_unused]] bool is_lock)
{
	sys_net.todo("lv2_socket_raw::sendto");
	return buf.size();
}

void lv2_socket_raw::close()
{
	sys_net.todo("lv2_socket_raw::close");
}

s32 lv2_socket_raw::shutdown([[maybe_unused]] s32 how)
{
	sys_net.todo("lv2_socket_raw::shutdown");
	return {};
}

s32 lv2_socket_raw::poll([[maybe_unused]] sys_net_pollfd& sn_pfd, [[maybe_unused]] pollfd& native_pfd)
{
	sys_net.todo("lv2_socket_raw::poll");
	return {};
}

std::tuple<bool, bool, bool> lv2_socket_raw::select([[maybe_unused]] bs_t<lv2_socket::poll_t> selected, [[maybe_unused]] pollfd& native_pfd)
{
	sys_net.todo("lv2_socket_raw::select");
	return {};
}
