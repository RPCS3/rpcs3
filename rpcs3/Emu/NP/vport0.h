#pragma once

#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "Emu/Cell/lv2/sys_net/nt_p2p_port.h"

bool send_packet_from_p2p_port_ipv4(const std::vector<u8>& data, const sockaddr_in& addr);
bool send_packet_from_p2p_port_ipv6(const std::vector<u8>& data, const sockaddr_in6& addr);
std::vector<signaling_message> get_sign_msgs();
std::vector<std::vector<u8>> get_rpcn_msgs();

constexpr s32 VPORT_0_HEADER_SIZE = sizeof(u16) + sizeof(u8);

// VPort 0 is invalid for sys_net so we use it for:
// Subset 0: Messages from RPCN server, IP retrieval / UDP hole punching
// Subset 1: Signaling
enum VPORT_0_SUBSET : u8
{
	SUBSET_RPCN      = 0,
	SUBSET_SIGNALING = 1,
};
