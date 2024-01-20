#include "stdafx.h"
#include "lv2_socket_raw.h"
#include "Emu/NP/vport0.h"

LOG_CHANNEL(sys_net);

lv2_socket_raw::lv2_socket_raw(lv2_socket_family family, lv2_socket_type type, lv2_ip_protocol protocol)
	: lv2_socket(family, type, protocol)
{
}

lv2_socket_raw::lv2_socket_raw(utils::serial& ar, lv2_socket_type type)
	: lv2_socket(stx::make_exact(ar), type)
{
}

void lv2_socket_raw::save(utils::serial& ar)
{
	lv2_socket::save(ar, true);
}

std::tuple<bool, s32, std::shared_ptr<lv2_socket>, sys_net_sockaddr> lv2_socket_raw::accept([[maybe_unused]] bool is_lock)
{
	sys_net.fatal("[RAW] accept() called on a RAW socket");
	return {};
}

std::optional<s32> lv2_socket_raw::connect([[maybe_unused]] const sys_net_sockaddr& addr)
{
	sys_net.fatal("[RAW] connect() called on a RAW socket");
	return CELL_OK;
}

s32 lv2_socket_raw::connect_followup()
{
	sys_net.fatal("[RAW] connect_followup() called on a RAW socket");
	return CELL_OK;
}

std::pair<s32, sys_net_sockaddr> lv2_socket_raw::getpeername()
{
	sys_net.todo("[RAW] getpeername() called on a RAW socket");
	return {};
}

s32 lv2_socket_raw::listen([[maybe_unused]] s32 backlog)
{
	sys_net.todo("[RAW] listen() called on a RAW socket");
	return {};
}

s32 lv2_socket_raw::bind([[maybe_unused]] const sys_net_sockaddr& addr)
{
	sys_net.todo("lv2_socket_raw::bind");
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

s32 lv2_socket_raw::setsockopt(s32 level, s32 optname, const std::vector<u8>& optval)
{
	sys_net.todo("lv2_socket_raw::setsockopt");

	// TODO
	int native_int = *reinterpret_cast<const be_t<s32>*>(optval.data());

	if (level == SYS_NET_SOL_SOCKET && optname == SYS_NET_SO_NBIO)
	{
		so_nbio = native_int;
	}

	return {};
}

std::optional<std::tuple<s32, std::vector<u8>, sys_net_sockaddr>> lv2_socket_raw::recvfrom(s32 flags, [[maybe_unused]] u32 len, [[maybe_unused]] bool is_lock)
{
	sys_net.todo("lv2_socket_raw::recvfrom");

	if (so_nbio || (flags & SYS_NET_MSG_DONTWAIT))
	{
		return {{-SYS_NET_EWOULDBLOCK, {}, {}}};
	}

	return {};
}

std::optional<s32> lv2_socket_raw::sendto([[maybe_unused]] s32 flags, [[maybe_unused]] const std::vector<u8>& buf, [[maybe_unused]] std::optional<sys_net_sockaddr> opt_sn_addr, [[maybe_unused]] bool is_lock)
{
	sys_net.todo("lv2_socket_raw::sendto");
	return ::size32(buf);
}

std::optional<s32> lv2_socket_raw::sendmsg([[maybe_unused]] s32 flags, [[maybe_unused]] const sys_net_msghdr& msg, [[maybe_unused]] bool is_lock)
{
	sys_net.todo("lv2_socket_raw::sendmsg");
	return {};
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
