#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/NP/np_handler.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/cellNetCtl.h"
#include "Utilities/StrUtil.h"
#include "Emu/IdManager.h"
#include "Emu/NP/np_structs_extra.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_config.h"
#include "Emu/NP/np_contexts.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Emu/Cell/lv2/sys_net/network_context.h"
#include "Emu/Cell/lv2/sys_net/sys_net_helpers.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#else
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif

#include "util/asm.hpp"

#include <span>

LOG_CHANNEL(sys_net);
LOG_CHANNEL(sceNp2);
LOG_CHANNEL(sceNp);

LOG_CHANNEL(rpcn_log, "rpcn");
LOG_CHANNEL(nph_log, "NPHandler");
LOG_CHANNEL(ticket_log, "Ticket");

namespace np
{
	ticket::ticket(std::vector<u8>&& raw_data)
		: raw_data(raw_data)
	{
		parse();
	}

	std::size_t ticket::size() const
	{
		return raw_data.size();
	}

	const u8* ticket::data() const
	{
		return raw_data.data();
	}

	bool ticket::empty() const
	{
		return raw_data.empty();
	}

	bool ticket::get_value(s32 param_id, vm::ptr<SceNpTicketParam> param) const
	{
		if (!parse_success)
		{
			return false;
		}

		switch (param_id)
		{
		case SCE_NP_TICKET_PARAM_SERIAL_ID:
		{
			const auto& node = nodes[0].data.data_nodes[0];
			if (node.len != SCE_NP_TICKET_SERIAL_ID_SIZE)
			{
				return false;
			}

			memcpy(param->data, node.data.data_vec.data(), SCE_NP_TICKET_SERIAL_ID_SIZE);
			break;
		}
		case SCE_NP_TICKET_PARAM_ISSUER_ID:
		{
			const auto& node = nodes[0].data.data_nodes[1];
			param->ui32      = node.data.data_u32;
			break;
		}
		case SCE_NP_TICKET_PARAM_ISSUED_DATE:
		{
			const auto& node = nodes[0].data.data_nodes[2];
			param->ui64      = node.data.data_u64;
			break;
		}
		case SCE_NP_TICKET_PARAM_EXPIRE_DATE:
		{
			const auto& node = nodes[0].data.data_nodes[3];
			param->ui64      = node.data.data_u64;
			break;
		}
		case SCE_NP_TICKET_PARAM_SUBJECT_ACCOUNT_ID:
		{
			const auto& node = nodes[0].data.data_nodes[4];
			param->ui64      = node.data.data_u64;
			break;
		}
		case SCE_NP_TICKET_PARAM_SUBJECT_ONLINE_ID:
		{
			const auto& node = nodes[0].data.data_nodes[5];
			if (node.len != 0x20)
			{
				return false;
			}

			memcpy(param->data, node.data.data_vec.data(), 0x20);
			break;
		}
		case SCE_NP_TICKET_PARAM_SUBJECT_REGION:
		{
			const auto& node = nodes[0].data.data_nodes[6];
			if (node.len != SCE_NP_SUBJECT_REGION_SIZE)
			{
				return false;
			}

			memcpy(param->data, node.data.data_vec.data(), SCE_NP_SUBJECT_REGION_SIZE);
			break;
		}
		case SCE_NP_TICKET_PARAM_SUBJECT_DOMAIN:
		{
			const auto& node = nodes[0].data.data_nodes[7];
			if (node.len != SCE_NP_SUBJECT_DOMAIN_SIZE)
			{
				return false;
			}

			memcpy(param->data, node.data.data_vec.data(), SCE_NP_SUBJECT_DOMAIN_SIZE);
			break;
		}
		case SCE_NP_TICKET_PARAM_SERVICE_ID:
		{
			const auto& node = nodes[0].data.data_nodes[8];
			if (node.len != SCE_NP_SERVICE_ID_SIZE)
			{
				return false;
			}

			memcpy(param->data, node.data.data_vec.data(), SCE_NP_SERVICE_ID_SIZE);
			break;
		}
		case SCE_NP_TICKET_PARAM_SUBJECT_STATUS:
		{
			const auto& node = nodes[0].data.data_nodes[9];
			param->ui32      = node.data.data_u32;
			break;
		}
		case SCE_NP_TICKET_PARAM_STATUS_DURATION:
		case SCE_NP_TICKET_PARAM_SUBJECT_DOB:
		{
			param->ui64 = 0;
			break;
		}
		default:
			sceNp.fatal("Invalid ticket param id requested!");
			return false;
		}

		return true;
	}

	std::optional<ticket_data> ticket::parse_node(std::size_t index) const
	{
		if ((index + MIN_TICKET_DATA_SIZE) > size())
		{
			ticket_log.error("node didn't meet minimum size requirements");
			return std::nullopt;
		}

		ticket_data tdata{};
		const auto* ptr = data() + index;
		tdata.id = read_from_ptr<be_t<u16>>(ptr);
		tdata.len = read_from_ptr<be_t<u16>>(ptr + 2);
		const auto* data_ptr = data() + index + 4;

		auto check_size = [&](std::size_t expected) -> bool
		{
			if ((index + MIN_TICKET_DATA_SIZE + expected) > size())
			{
				return false;
			}
			return true;
		};

		switch (tdata.id)
		{
		case 0:
			if (tdata.len != 0)
			{
				return std::nullopt;
			}
			break;
		case 1:
			if (tdata.len != 4 || !check_size(4))
			{
				return std::nullopt;
			}
			tdata.data.data_u32 = read_from_ptr<be_t<u32>>(data_ptr);
			break;
		case 2:
		case 7:
			if (tdata.len != 8 || !check_size(8))
			{
				return std::nullopt;
			}
			tdata.data.data_u64 = read_from_ptr<be_t<u64>>(data_ptr);
			break;
		case 4:
		case 8:
			if (!check_size(tdata.len))
			{
				return std::nullopt;
			}
			tdata.data.data_vec = std::vector<u8>(tdata.len);
			memcpy(tdata.data.data_vec.data(), data_ptr, tdata.len);
			break;
		default:
			if ((tdata.id & 0x3000) == 0x3000)
			{
				if (!check_size(tdata.len))
				{
					return std::nullopt;
				}

				std::size_t sub_index = 0;
				tdata.data.data_nodes = {};
				while (sub_index < tdata.len)
				{
					auto sub_node = parse_node(sub_index + index + 4);
					if (!sub_node)
					{
						ticket_log.error("Failed to parse subnode at %d", sub_index + index + 4);
						return std::nullopt;
					}
					sub_index += sub_node->len + MIN_TICKET_DATA_SIZE;
					tdata.data.data_nodes.push_back(std::move(*sub_node));
				}
				break;
			}
			return std::nullopt;
		}

		return tdata;
	}

	void ticket::parse()
	{
		nodes.clear();
		parse_success = false;

		if (size() < (sizeof(u32) * 2))
		{
			return;
		}

		version = read_from_ptr<be_t<u32>>(data());
		if (version != 0x21010000)
		{
			ticket_log.error("Invalid version: 0x%08x", version);
			return;
		}

		u32 given_size = read_from_ptr<be_t<u32>>(data() + 4);
		if ((given_size + 8) != size())
		{
			ticket_log.error("Size mismatch (gs: %d vs s: %d)", given_size, size());
			return;
		}

		std::size_t index = 8;
		while (index < size())
		{
			auto node = parse_node(index);
			if (!node)
			{
				ticket_log.error("Failed to parse node at index %d", index);
				return;
			}

			index += (node->len + MIN_TICKET_DATA_SIZE);
			nodes.push_back(std::move(*node));
		}

		// Check that everything expected is there
		if (nodes.size() != 2)
		{
			ticket_log.error("Expected 2 blobs, found %d", nodes.size());
			return;
		}

		if (nodes[0].id != 0x3000 && nodes[1].id != 0x3002)
		{
			ticket_log.error("The 2 blobs ids are incorrect");
			return;
		}

		if (nodes[0].data.data_nodes.size() < 12)
		{
			ticket_log.error("Expected at least 12 sub-nodes, found %d", nodes[0].data.data_nodes.size());
			return;
		}

		const auto& subnodes = nodes[0].data.data_nodes;

		if (subnodes[0].id != 8 || subnodes[1].id != 1 || subnodes[2].id != 7 || subnodes[3].id != 7 ||
			subnodes[4].id != 2 || subnodes[5].id != 4 || subnodes[6].id != 8 || subnodes[7].id != 4 ||
			subnodes[8].id != 8 || subnodes[9].id != 1)
		{
			ticket_log.error("Mismatched node");
			return;
		}

		parse_success = true;
		return;
	}

	np_handler::np_handler()
	{
		g_fxo->need<named_thread<signaling_handler>>();

		is_connected  = (g_cfg.net.net_active == np_internet_status::enabled);
		is_psn_active = (g_cfg.net.psn_status >= np_psn_status::psn_fake) && is_connected;

		if (get_net_status() == CELL_NET_CTL_STATE_IPObtained)
		{

			if (!discover_ether_address() || !discover_ip_address())
			{
				nph_log.error("Failed to discover ethernet or ip address!");
				is_connected  = false;
				is_psn_active = false;
				return;
			}

			// Convert dns address
			// TODO: recover actual user dns through OS specific API
			in_addr conv{};
			if (!inet_pton(AF_INET, g_cfg.net.dns.to_string().c_str(), &conv))
			{
				// Do not set to disconnected on invalid IP just error and continue using default(google's 8.8.8.8)
				nph_log.error("Provided IP(%s) address for DNS is invalid!", g_cfg.net.dns.to_string());
			}
			else
			{
				dns_ip = conv.s_addr;
			}

			// Convert bind address
			conv = {};
			if (!inet_pton(AF_INET, g_cfg.net.bind_address.to_string().c_str(), &conv))
			{
				// Do not set to disconnected on invalid IP just error and continue using default (0.0.0.0)
				nph_log.error("Provided IP(%s) address for bind is invalid!", g_cfg.net.bind_address.to_string());
			}
			else
			{
				bind_ip = conv.s_addr;

				if (bind_ip)
					local_ip_addr = bind_ip;
			}

			if (g_cfg.net.upnp_enabled)
				upnp.upnp_enable();
		}

		if (is_psn_active && g_cfg.net.psn_status == np_psn_status::psn_rpcn)
		{
			g_fxo->need<network_context>();
			auto& nc = g_fxo->get<network_context>();
			nc.bind_sce_np_port();

			std::lock_guard lock(mutex_rpcn);
			rpcn = rpcn::rpcn_client::get_instance();
		}
	}

	np_handler::np_handler(utils::serial& ar)
		: np_handler()
	{
		ar(is_netctl_init, is_NP_init);

		if (!is_NP_init)
		{
			return;
		}

		ar(is_NP_Lookup_init, is_NP_Score_init, is_NP2_init, is_NP2_Match2_init, is_NP_Auth_init, manager_cb, manager_cb_arg, std::as_bytes(std::span(&basic_handler, 1)), is_connected, is_psn_active, hostname, ether_address, local_ip_addr, public_ip_addr, dns_ip);

		// Call init func if needed (np_memory is unaffected when an empty pool is provided)
		init_NP(0, vm::null);

		np_memory.save(ar);

		// TODO: IDM-tied objects are not yet saved
	}

	np_handler::~np_handler()
	{
		std::unordered_map<u32, std::shared_ptr<score_transaction_ctx>> moved_trans;
		{
			std::lock_guard lock(mutex_score_transactions);
			moved_trans = std::move(score_transactions);
			score_transactions.clear();
		}

		for (auto& [trans_id, trans] : moved_trans)
		{
			trans->abort_score_transaction();
		}

		for (auto& [trans_id, trans] : moved_trans)
		{
			if (trans->thread.joinable())
				trans->thread.join();
		}
	}

	std::shared_ptr<rpcn::rpcn_client> np_handler::get_rpcn()
	{
		if (!rpcn) [[unlikely]]
		{
			ensure(g_cfg.net.psn_status == np_psn_status::psn_fake);
			fmt::throw_exception("RPCN is required to use PSN online features.");
		}
		return rpcn;
	}

	void np_handler::save(utils::serial& ar)
	{
		// TODO: See ctor
		ar(is_netctl_init, is_NP_init);

		if (!is_NP_init)
		{
			return;
		}

		USING_SERIALIZATION_VERSION(sceNp);

		ar(is_NP_Lookup_init, is_NP_Score_init, is_NP2_init, is_NP2_Match2_init, is_NP_Auth_init, manager_cb, manager_cb_arg, std::as_bytes(std::span(&basic_handler, 1)), is_connected, is_psn_active, hostname, ether_address, local_ip_addr, public_ip_addr, dns_ip);

		np_memory.save(ar);
	}

	void memory_allocator::save(utils::serial& ar)
	{
		ar(m_pool, m_size, m_allocs, m_avail);
	}

	bool np_handler::discover_ip_address()
	{
		auto sockfd = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
		if (sockfd == INVALID_SOCKET)
#else
		if (sockfd == -1)
#endif
		{
			nph_log.error("Creating socket to discover local ip failed: %d", get_native_error());
			return false;
		}

		auto close_socket = [&]()
		{
#ifdef _WIN32
			closesocket(sockfd);
#else
			close(sockfd);
#endif
		};

		::sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = 53;
		addr.sin_addr.s_addr = 0x08080808;
		if (connect(sockfd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) != 0)
		{
			// If connect fails a route to the internet is not available
			nph_log.error("connect to discover local ip failed: %d", get_native_error());
			close_socket();
			return false; // offline
		}

		sockaddr_in client_addr{};
		socklen_t client_addr_size = sizeof(client_addr);
		if (getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_size) != 0)
		{
			rpcn_log.error("getsockname to discover local ip failed: %d", get_native_error());
			close_socket();
			return true; // still assume online
		}

		local_ip_addr = client_addr.sin_addr.s_addr;
		if (nph_log.trace)
			nph_log.trace("discover_ip_address: IP was determined to be %s", ip_to_string(local_ip_addr));
		close_socket();
		return true;
	}

	bool np_handler::discover_ether_address()
	{
#if defined(__FreeBSD__) || defined(__APPLE__)
		ifaddrs* ifap;

		if (getifaddrs(&ifap) == 0)
		{
			ifaddrs* p;
			for (p = ifap; p; p = p->ifa_next)
			{
				if (p->ifa_addr->sa_family == AF_LINK)
				{
					sockaddr_dl* sdp = reinterpret_cast<sockaddr_dl*>(p->ifa_addr);
					memcpy(ether_address.data(), sdp->sdl_data + sdp->sdl_nlen, 6);
					freeifaddrs(ifap);
					// nph_log.notice("Determined Ethernet address to be %s", ether_to_string(ether_address));
					return true;
				}
			}
			freeifaddrs(ifap);
		}
#elif defined(_WIN32)
		std::vector<u8> adapter_infos(sizeof(IP_ADAPTER_INFO));
		ULONG size_infos = sizeof(IP_ADAPTER_INFO);

		if (GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapter_infos.data()), &size_infos) == ERROR_BUFFER_OVERFLOW)
			adapter_infos.resize(size_infos);

		if (GetAdaptersInfo(reinterpret_cast<PIP_ADAPTER_INFO>(adapter_infos.data()), &size_infos) == NO_ERROR && size_infos)
		{
			PIP_ADAPTER_INFO info = reinterpret_cast<PIP_ADAPTER_INFO>(adapter_infos.data());
			memcpy(ether_address.data(), info[0].Address, 6);
			// nph_log.notice("Determined Ethernet address to be %s", ether_to_string(ether_address));
			return true;
		}
#else
		ifreq ifr;
		ifconf ifc;
		char buf[1024];
		int success = 0;

		int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
		if (sock == -1)
			return false;

		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = buf;
		if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
			return false;

		ifreq* it              = ifc.ifc_req;
		const ifreq* const end = it + (ifc.ifc_len / sizeof(ifreq));

		for (; it != end; ++it)
		{
			strcpy(ifr.ifr_name, it->ifr_name);
			if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
			{
				if (!(ifr.ifr_flags & IFF_LOOPBACK))
				{
					if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
					{
						success = 1;
						break;
					}
				}
			}
		}

		if (success)
		{
			memcpy(ether_address.data(), ifr.ifr_hwaddr.sa_data, 6);
			// nph_log.notice("Determined Ethernet address to be %s", ether_to_string(ether_address));

			return true;
		}
#endif

		return false;
	}

	const std::array<u8, 6>& np_handler::get_ether_addr() const
	{
		return ether_address;
	}

	const std::string& np_handler::get_hostname() const
	{
		return hostname;
	}

	u32 np_handler::get_local_ip_addr() const
	{
		return local_ip_addr;
	}

	u32 np_handler::get_public_ip_addr() const
	{
		return public_ip_addr;
	}

	u32 np_handler::get_dns_ip() const
	{
		return dns_ip;
	}

	u32 np_handler::get_bind_ip() const
	{
		return bind_ip;
	}

	s32 np_handler::get_net_status() const
	{
		return is_connected ? CELL_NET_CTL_STATE_IPObtained : CELL_NET_CTL_STATE_Disconnected;
	}

	s32 np_handler::get_psn_status() const
	{
		return is_psn_active ? SCE_NP_MANAGER_STATUS_ONLINE : SCE_NP_MANAGER_STATUS_OFFLINE;
	}

	s32 np_handler::get_upnp_status() const
	{
		if (upnp.is_active())
			return SCE_NP_SIGNALING_NETINFO_UPNP_STATUS_VALID;

		return SCE_NP_SIGNALING_NETINFO_UPNP_STATUS_INVALID;
	}

	const SceNpId& np_handler::get_npid() const
	{
		return npid;
	}

	const SceNpOnlineId& np_handler::get_online_id() const
	{
		return npid.handle;
	}

	const SceNpOnlineName& np_handler::get_online_name() const
	{
		return online_name;
	}

	const SceNpAvatarUrl& np_handler::get_avatar_url() const
	{
		return avatar_url;
	}

	void np_handler::init_NP(u32 poolsize, vm::ptr<void> poolptr)
	{
		if (poolsize)
		{
			// Init memory pool (zero arg is reserved for savestate's use)
			np_memory.setup(poolptr, poolsize);
		}

		memset(&npid, 0, sizeof(npid));
		memset(&online_name, 0, sizeof(online_name));
		memset(&avatar_url, 0, sizeof(avatar_url));

		if (g_cfg.net.psn_status >= np_psn_status::psn_fake)
		{
			g_cfg_rpcn.load(); // Ensures config is loaded even if rpcn is not running for simulated
			std::string s_npid = g_cfg_rpcn.get_npid();
			ensure(!s_npid.empty()); // It should have been generated before this

			string_to_npid(s_npid, npid);
			auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
			sigh.set_self_sig_info(npid);
		}

		switch (g_cfg.net.psn_status)
		{
		case np_psn_status::disabled:
			break;
		case np_psn_status::psn_fake:
		{
			string_to_online_name("RPCS3's user", online_name);
			string_to_avatar_url("https://rpcs3.net/cdn/netplay/DefaultAvatar.png", avatar_url);
			break;
		}
		case np_psn_status::psn_rpcn:
		{
			if (!is_psn_active)
				break;

			std::lock_guard lock(mutex_rpcn);
			rpcn = rpcn::rpcn_client::get_instance();

			// Make sure we're connected

			if (auto state = rpcn->wait_for_connection(); state != rpcn::rpcn_state::failure_no_failure)
			{
				rsx::overlays::queue_message(rpcn::rpcn_state_to_localized_string_id(state));
				rpcn_log.error("Connection to RPCN Failed!");
				is_psn_active = false;
				return;
			}

			if (auto state = rpcn->wait_for_authentified(); state != rpcn::rpcn_state::failure_no_failure)
			{
				rsx::overlays::queue_message(rpcn::rpcn_state_to_localized_string_id(state));
				rpcn_log.error("RPCN login attempt failed!");
				is_psn_active = false;
				return;
			}

			rsx::overlays::queue_message(localized_string_id::RPCN_SUCCESS_LOGGED_ON);

			string_to_online_name(rpcn->get_online_name(), online_name);
			string_to_avatar_url(rpcn->get_avatar_url(), avatar_url);
			public_ip_addr = rpcn->get_addr_sig();
			local_ip_addr  = std::bit_cast<u32, be_t<u32>>(rpcn->get_addr_local());

			break;
		}
		default:
			break;
		}
	}

	void np_handler::terminate_NP()
	{
		np_memory.release();

		if (g_cfg.net.psn_status == np_psn_status::psn_rpcn)
		{
			rpcn_log.notice("Disconnecting from RPCN!");
			std::lock_guard lock(mutex_rpcn);
			rpcn.reset();
		}
	}

	u32 np_handler::get_match2_event(SceNpMatching2EventKey event_key, u32 dest_addr, u32 size)
	{
		std::lock_guard lock(mutex_match2_req_results);

		if (!match2_req_results.contains(event_key))
			return 0;

		auto& data = ::at32(match2_req_results, event_key);
		data.apply_relocations(dest_addr);

		vm::ptr<void> dest = vm::cast(dest_addr);

		u32 size_copied = std::min(size, data.size());
		memcpy(dest.get_ptr(), data.data(), size_copied);

		np_memory.free(data.addr());
		match2_req_results.erase(event_key);

		return size_copied;
	}

	bool np_handler::send_basic_event(s32 event, s32 retCode, u32 reqId)
	{
		if (basic_handler.registered)
		{
			sysutil_register_cb([handler_func = this->basic_handler.handler_func, handler_arg = this->basic_handler.handler_arg, event, retCode, reqId](ppu_thread& cb_ppu) -> s32
				{
					handler_func(cb_ppu, event, retCode, reqId, handler_arg);
					return 0;
				});

			return true;
		}

		return false;
	}

	void np_handler::queue_basic_event(basic_event to_queue)
	{
		std::lock_guard lock(mutex_queue_basic_events);
		queue_basic_events.push(std::move(to_queue));
	}

	error_code np_handler::get_basic_event(vm::ptr<s32> event, vm::ptr<SceNpUserInfo> from, vm::ptr<u8> data, vm::ptr<u32> size)
	{
		basic_event cur_event;
		{
			std::lock_guard lock(mutex_queue_basic_events);
			if (queue_basic_events.empty())
			{
				return not_an_error(SCE_NP_BASIC_ERROR_NO_EVENT);
			}

			cur_event = std::move(queue_basic_events.front());
			queue_basic_events.pop();
		}

		const u32 size_avail = *size;
		u32 res_size         = std::min(static_cast<u32>(cur_event.data.size()), size_avail);

		*event = cur_event.event;
		memcpy(from.get_ptr(), &cur_event.from, sizeof(cur_event.from));
		memcpy(data.get_ptr(), cur_event.data.data(), res_size);
		*size = res_size;

		if (res_size < cur_event.data.size())
		{
			return SCE_NP_BASIC_ERROR_DATA_LOST;
		}

		nph_log.notice("basic_event: event:%d, from:%s(%s), size:%d", *event, static_cast<char*>(from->userId.handle.data), static_cast<char*>(from->name.data), *size);

		return CELL_OK;
	}

	std::optional<std::shared_ptr<std::pair<std::string, message_data>>> np_handler::get_message(u64 id)
	{
		return get_rpcn()->get_message(id);
	}

	void np_handler::set_message_selected(SceNpBasicAttachmentDataId id, u64 msg_id)
	{
		switch (id)
		{
		case SCE_NP_BASIC_SELECTED_INVITATION_DATA:
			selected_invite_id = msg_id;
			break;
		case SCE_NP_BASIC_SELECTED_MESSAGE_DATA:
			selected_message_id = msg_id;
			break;
		default:
			fmt::throw_exception("set_message_selected with id %d", id);
		}
	}

	std::optional<std::shared_ptr<std::pair<std::string, message_data>>> np_handler::get_message_selected(SceNpBasicAttachmentDataId id)
	{
		switch (id)
		{
		case SCE_NP_BASIC_SELECTED_INVITATION_DATA:
			if (!selected_invite_id)
				return std::nullopt;

			return get_message(*selected_invite_id);
		case SCE_NP_BASIC_SELECTED_MESSAGE_DATA:
			if (!selected_message_id)
				return std::nullopt;

			return get_message(*selected_message_id);
		default:
			fmt::throw_exception("get_message_selected with id %d", id);
		}
	}

	void np_handler::clear_message_selected(SceNpBasicAttachmentDataId id)
	{
		switch (id)
		{
		case SCE_NP_BASIC_SELECTED_INVITATION_DATA:
			selected_invite_id = std::nullopt;
			break;
		case SCE_NP_BASIC_SELECTED_MESSAGE_DATA:
			selected_message_id = std::nullopt;
			break;
		default:
			fmt::throw_exception("clear_message_selected with id %d", id);
		}
	}

	void np_handler::operator()()
	{
		if (g_cfg.net.psn_status != np_psn_status::psn_rpcn)
			return;

		while (thread_ctrl::state() != thread_state::aborting && !Emu.IsStopped())
		{
			bool sleep = true;
			if (rpcn)
			{
				std::lock_guard lock(mutex_rpcn);
				if (!rpcn)
				{
					continue;
				}

				auto replies = rpcn->get_replies();
				for (auto& reply : replies)
				{
					const u16 command     = reply.second.first;
					const u32 req_id      = reply.first;
					std::vector<u8>& data = reply.second.second;

					// Every reply should at least contain a return value/error code
					ensure(data.size() >= 1);

					switch (command)
					{
					case rpcn::CommandType::GetWorldList: reply_get_world_list(req_id, data); break;
					case rpcn::CommandType::CreateRoom: reply_create_join_room(req_id, data); break;
					case rpcn::CommandType::JoinRoom: reply_join_room(req_id, data); break;
					case rpcn::CommandType::LeaveRoom: reply_leave_room(req_id, data); break;
					case rpcn::CommandType::SearchRoom: reply_search_room(req_id, data); break;
					case rpcn::CommandType::GetRoomDataExternalList: reply_get_roomdata_external_list(req_id, data); break;
					case rpcn::CommandType::SetRoomDataExternal: reply_set_roomdata_external(req_id, data); break;
					case rpcn::CommandType::GetRoomDataInternal: reply_get_roomdata_internal(req_id, data); break;
					case rpcn::CommandType::SetRoomDataInternal: reply_set_roomdata_internal(req_id, data); break;
					case rpcn::CommandType::SetRoomMemberDataInternal: reply_set_roommemberdata_internal(req_id, data); break;
					case rpcn::CommandType::PingRoomOwner: reply_get_ping_info(req_id, data); break;
					case rpcn::CommandType::SendRoomMessage: reply_send_room_message(req_id, data); break;
					case rpcn::CommandType::RequestSignalingInfos: reply_req_sign_infos(req_id, data); break;
					case rpcn::CommandType::RequestTicket: reply_req_ticket(req_id, data); break;
					case rpcn::CommandType::GetBoardInfos: reply_get_board_infos(req_id, data); break;
					case rpcn::CommandType::RecordScore: reply_record_score(req_id, data); break;
					case rpcn::CommandType::RecordScoreData: reply_record_score_data(req_id, data); break;
					case rpcn::CommandType::GetScoreData: reply_get_score_data(req_id, data); break;
					case rpcn::CommandType::GetScoreRange: reply_get_score_range(req_id, data); break;
					case rpcn::CommandType::GetScoreFriends: reply_get_score_friends(req_id, data); break;
					case rpcn::CommandType::GetScoreNpid: reply_get_score_npid(req_id, data); break;
					default: rpcn_log.error("Unknown reply(%d) received!", command); break;
					}
				}

				auto notifications = rpcn->get_notifications();
				for (auto& notif : notifications)
				{
					switch (notif.first)
					{
					case rpcn::NotificationType::UserJoinedRoom: notif_user_joined_room(notif.second); break;
					case rpcn::NotificationType::UserLeftRoom: notif_user_left_room(notif.second); break;
					case rpcn::NotificationType::RoomDestroyed: notif_room_destroyed(notif.second); break;
					case rpcn::NotificationType::UpdatedRoomDataInternal: notif_updated_room_data_internal(notif.second); break;
					case rpcn::NotificationType::UpdatedRoomMemberDataInternal: notif_updated_room_member_data_internal(notif.second); break;
					case rpcn::NotificationType::SignalP2PConnect: notif_p2p_connect(notif.second); break;
					case rpcn::NotificationType::RoomMessageReceived: notif_room_message_received(notif.second); break;
					default: rpcn_log.error("Unknown notification(%d) received!", notif.first); break;
					}
				}

				auto messages = rpcn->get_new_messages();
				if (basic_handler.registered)
				{
					for (const auto msg_id : messages)
					{
						const auto opt_msg = rpcn->get_message(msg_id);
						if (!opt_msg)
						{
							continue;
						}
						const auto& msg = opt_msg.value();
						if (strncmp(msg->second.commId.data, basic_handler.context.data, sizeof(basic_handler.context.data) - 1) == 0)
						{
							u32 event;
							switch (msg->second.mainType)
							{
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_DATA_ATTACHMENT:
								event = SCE_NP_BASIC_EVENT_INCOMING_ATTACHMENT;
								break;
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE:
								event = (msg->second.msgFeatures & SCE_NP_BASIC_MESSAGE_FEATURES_BOOTABLE) ? SCE_NP_BASIC_EVENT_INCOMING_BOOTABLE_INVITATION : SCE_NP_BASIC_EVENT_INCOMING_INVITATION;
								break;
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_CUSTOM_DATA:
								event = (msg->second.msgFeatures & SCE_NP_BASIC_MESSAGE_FEATURES_BOOTABLE) ? SCE_NP_BASIC_EVENT_INCOMING_BOOTABLE_CUSTOM_DATA_MESSAGE : SCE_NP_BASIC_EVENT_INCOMING_CUSTOM_DATA_MESSAGE;
								break;
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_GENERAL:
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_ADD_FRIEND:
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_URL_ATTACHMENT:
								event = SCE_NP_BASIC_EVENT_MESSAGE;
							default:
								continue;
							}

							basic_event to_add{};
							to_add.event = event;
							strcpy_trunc(to_add.from.userId.handle.data, msg->first);
							strcpy_trunc(to_add.from.name.data, msg->first);

							queue_basic_event(std::move(to_add));
							send_basic_event(event, 0, 0);
						}
					}
				}

				if (!replies.empty() || !notifications.empty())
				{
					sleep = false;
				}
			}

			// TODO: replace with an appropriate semaphore
			if (sleep)
			{
				thread_ctrl::wait_for(200'000);
				continue;
			}
		}
	}

	bool np_handler::error_and_disconnect(const std::string& error_msg)
	{
		rpcn_log.error("%s", error_msg);
		rpcn.reset();

		return false;
	}

	u32 np_handler::generate_callback_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam, SceNpMatching2Event event_type)
	{
		callback_info ret;

		const auto ctx = get_match2_context(ctx_id);
		ensure(ctx);

		const u32 req_id = get_req_id(optParam ? optParam->appReqId : ctx->default_match2_optparam.appReqId);

		ret.ctx_id = ctx_id;
		ret.cb_arg = (optParam && optParam->cbFuncArg) ? optParam->cbFuncArg : ctx->default_match2_optparam.cbFuncArg;
		ret.cb     = (optParam && optParam->cbFunc) ? optParam->cbFunc : ctx->default_match2_optparam.cbFunc;
		ret.event_type = event_type;

		nph_log.warning("Callback used is 0x%x", ret.cb);

		{
			std::lock_guard lock(mutex_pending_requests);
			pending_requests[req_id] = std::move(ret);
		}

		return req_id;
	}

	std::optional<np_handler::callback_info> np_handler::take_pending_request(u32 req_id)
	{
		std::lock_guard lock(mutex_pending_requests);

		if (!pending_requests.contains(req_id))
			return std::nullopt;

		const auto cb_info = std::move(::at32(pending_requests, req_id));
		pending_requests.erase(req_id);

		return cb_info;
	}

	bool np_handler::abort_request(u32 req_id)
	{
		auto cb_info_opt = take_pending_request(req_id);

		if (!cb_info_opt)
			return false;

		cb_info_opt->queue_callback(req_id, 0, SCE_NP_MATCHING2_ERROR_ABORTED, 0);

		return true;
	}

	event_data& np_handler::allocate_req_result(u32 event_key, u32 max_size, u32 initial_size)
	{
		std::lock_guard lock(mutex_match2_req_results);
		match2_req_results.emplace(std::piecewise_construct, std::forward_as_tuple(event_key), std::forward_as_tuple(np_memory.allocate(max_size), initial_size, max_size));
		return ::at32(match2_req_results, event_key);
	}

	u32 np_handler::add_players_to_history(vm::cptr<SceNpId> /*npids*/, u32 /*count*/)
	{
		const u32 req_id = get_req_id(0);

		if (basic_handler.handler_func)
		{
			sysutil_register_cb([req_id, cb = basic_handler.handler_func, cb_arg = basic_handler.handler_arg](ppu_thread& cb_ppu) -> s32
			{
				cb(cb_ppu, SCE_NP_BASIC_EVENT_ADD_PLAYERS_HISTORY_RESULT, 0, req_id, cb_arg);
				return 0;
			});
		}

		return req_id;
	}

	u32 np_handler::get_num_friends()
	{
		return get_rpcn()->get_num_friends();
	}

	u32 np_handler::get_num_blocks()
	{
		return get_rpcn()->get_num_blocks();
	}

	std::pair<error_code, std::optional<SceNpId>> np_handler::get_friend_by_index(u32 index)
	{
		auto str_friend = get_rpcn()->get_friend_by_index(index);

		if (!str_friend)
		{
			return {SCE_NP_ERROR_ID_NOT_FOUND, {}};
		}

		SceNpId npid_friend;
		string_to_npid(str_friend.value(), npid_friend);

		return {CELL_OK, npid_friend};
	}

	std::pair<error_code, std::optional<SceNpId>> np_handler::local_get_npid(u64 room_id, u16 member_id)
	{
		return np_cache.get_npid(room_id, member_id);
	}

	std::pair<error_code, std::optional<SceNpMatching2SessionPassword>> np_handler::local_get_room_password(SceNpMatching2RoomId room_id)
	{
		return np_cache.get_password(room_id);
	}

	std::pair<error_code, std::optional<SceNpMatching2RoomSlotInfo>> np_handler::local_get_room_slots(SceNpMatching2RoomId room_id)
	{
		return np_cache.get_slots(room_id);
	}

	std::pair<error_code, std::vector<SceNpMatching2RoomMemberId>> np_handler::local_get_room_memberids(SceNpMatching2RoomId room_id, s32 sort_method)
	{
		return np_cache.get_memberids(room_id, sort_method);
	}

	error_code np_handler::local_get_room_member_data(SceNpMatching2RoomId room_id, SceNpMatching2RoomMemberId member_id, const std::vector<SceNpMatching2AttributeId>& binattrs_list, SceNpMatching2RoomMemberDataInternal* ptr_member, u32 addr_data, u32 size_data)
	{
		return np_cache.get_member_and_attrs(room_id, member_id, binattrs_list, ptr_member, addr_data, size_data);
	}

	void np_handler::upnp_add_port_mapping(u16 internal_port, std::string_view protocol)
	{
		upnp.add_port_redir(np::ip_to_string(get_local_ip_addr()), internal_port, protocol);
	}

	void np_handler::upnp_remove_port_mapping(u16 internal_port, std::string_view protocol)
	{
		upnp.remove_port_redir(internal_port, protocol);
	}
} // namespace np
