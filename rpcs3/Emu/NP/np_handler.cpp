#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/NP/np_handler.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/cellNetCtl.h"
#include "Emu/Cell/timers.hpp"
#include "Utilities/StrUtil.h"
#include "Emu/IdManager.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_config.h"
#include "Emu/NP/np_contexts.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/NP/signaling_handler.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Emu/Cell/lv2/sys_net/network_context.h"
#include "Emu/Cell/lv2/sys_net/sys_net_helpers.h"

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#else
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <ifaddrs.h>
#include <net/if_dl.h>
#endif

#include "util/yaml.hpp"

#include <span>

LOG_CHANNEL(sys_net);
LOG_CHANNEL(sceNp2);
LOG_CHANNEL(sceNp);

LOG_CHANNEL(rpcn_log, "rpcn");
LOG_CHANNEL(nph_log, "NPHandler");
LOG_CHANNEL(ticket_log, "Ticket");

namespace np
{
	std::string get_players_history_path()
	{
		return fs::get_config_dir(true) + "players_history.yml";
	}

	std::map<std::string, player_history> load_players_history()
	{
		const auto parsing_error = [](std::string_view error) -> std::map<std::string, player_history>
		{
			nph_log.error("Error parsing %s: %s", get_players_history_path(), error);
			return {};
		};

		std::map<std::string, player_history> history;

		if (fs::file history_file{get_players_history_path(), fs::read + fs::create})
		{
			auto [yml_players_history, error] = yaml_load(history_file.to_string());

			if (!error.empty())
				return parsing_error(error);

			for (const auto& player : yml_players_history)
			{
				std::string username = player.first.Scalar();
				const auto& seq = player.second;

				if (!seq.IsSequence() || seq.size() != 3)
					return parsing_error("Player history is not a proper sequence!");

				const u64 timestamp = get_yaml_node_value<u64>(seq[0], error);
				if (!error.empty())
					return parsing_error(error);

				std::string description = seq[1].Scalar();

				if (!seq[2].IsSequence())
					return parsing_error("Expected communication ids sequence");

				std::set<std::string> com_ids;

				for (usz i = 0; i < seq[2].size(); i++)
				{
					com_ids.insert(seq[2][i].Scalar());
				}

				history.insert(std::make_pair(std::move(username), player_history{.timestamp = timestamp, .communication_ids = std::move(com_ids), .description = std::move(description)}));
			}
		}

		return history;
	}

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

	extern void init_np_handler_dependencies()
	{
		if (auto handler = g_fxo->try_get<named_thread<np_handler>>())
		{
			handler->init_np_handler_dependencies();
		}
	}

	void np_handler::init_np_handler_dependencies()
	{
		if (is_psn_active && g_cfg.net.psn_status == np_psn_status::psn_rpcn && g_fxo->is_init<p2p_context>() && !m_inited_np_handler_dependencies)
		{
			m_inited_np_handler_dependencies = true;

			auto& nc = g_fxo->get<p2p_context>();
			nc.bind_sce_np_port();

			std::lock_guard lock(mutex_rpcn);
			rpcn = rpcn::rpcn_client::get_instance(bind_ip);
		}
	}

	np_handler::np_handler()
	{
		{
			auto history = load_players_history();

			std::lock_guard lock(mutex_history);
			players_history = std::move(history);
		}

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
			bind_ip = resolve_binding_ip();

			if (bind_ip)
				local_ip_addr = bind_ip;

			if (g_cfg.net.upnp_enabled)
				upnp.upnp_enable();
		}

		init_np_handler_dependencies();
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
		std::unordered_map<u32, shared_ptr<generic_async_transaction_context>> moved_trans;
		{
			std::lock_guard lock(mutex_async_transactions);
			moved_trans = std::move(async_transactions);
			async_transactions.clear();
		}

		for (auto& [trans_id, trans] : moved_trans)
		{
			trans->abort_transaction();
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

		std::memset(&npid, 0, sizeof(npid));
		std::memset(&online_name, 0, sizeof(online_name));
		std::memset(&avatar_url, 0, sizeof(avatar_url));

		if (g_cfg.net.psn_status >= np_psn_status::psn_fake)
		{
			g_cfg_rpcn.load(); // Ensures config is loaded even if rpcn is not running for simulated
			const std::string s_npid = g_cfg_rpcn.get_npid();
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

			bool was_already_started = true;

			if (!rpcn)
			{
				rpcn = rpcn::rpcn_client::get_instance(bind_ip);
				was_already_started = false;
			}

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

			if (!was_already_started)
				rsx::overlays::queue_message(localized_string_id::RPCN_SUCCESS_LOGGED_ON);

			string_to_online_name(rpcn->get_online_name(), online_name);
			string_to_avatar_url(rpcn->get_avatar_url(), avatar_url);
			public_ip_addr = rpcn->get_addr_sig();

			if (!public_ip_addr)
			{
				rsx::overlays::queue_message(rpcn::rpcn_state_to_localized_string_id(rpcn::rpcn_state::failure_other));
				rpcn_log.error("Failed to get a reply from RPCN signaling!");
				is_psn_active = false;
				rpcn->terminate_connection();
				return;
			}

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

		manager_cb = {};
		manager_cb_arg = {};
		basic_handler_registered = false;
		room_event_cb = {};
		room_event_cb_ctx = 0;
		room_event_cb_arg = {};
		room_msg_cb = {};
		room_msg_cb_ctx = 0;
		room_msg_cb_arg = {};

		presence_self.pr_status = {};
		presence_self.pr_data = {};
		presence_self.advertised = false;

		if (is_connected && is_psn_active && rpcn)
		{
			rpcn_log.notice("Setting RPCN state to disconnected!");
			rpcn->reset_state();
		}
	}

	u32 np_handler::get_match2_event(SceNpMatching2EventKey event_key, u32 dest_addr, u32 size)
	{
		std::lock_guard lock(mutex_match2_req_results);

		if (!match2_req_results.contains(event_key))
			return 0;

		auto& data = ::at32(match2_req_results, event_key);
		data.apply_relocations(dest_addr);

		const u32 size_copied = std::min(size, data.size());

		if (dest_addr && size_copied)
		{
			vm::ptr<void> dest = vm::cast(dest_addr);
			memcpy(dest.get_ptr(), data.data(), size_copied);
		}

		np_memory.free(data.addr());
		match2_req_results.erase(event_key);

		return size_copied;
	}

	void np_handler::register_basic_handler(vm::cptr<SceNpCommunicationId> context, vm::ptr<SceNpBasicEventHandler> handler, vm::ptr<void> arg, bool context_sensitive)
	{
		{
			std::lock_guard lock(basic_handler.mutex);
			memcpy(&basic_handler.context, context.get_ptr(), sizeof(basic_handler.context));
			basic_handler.handler_func = handler;
			basic_handler.handler_arg = arg;
			basic_handler.context_sensitive = context_sensitive;
			basic_handler_registered = true;

			presence_self.pr_com_id = *context;
			presence_self.pr_title = fmt::truncate(Emu.GetTitle(), SCE_NP_BASIC_PRESENCE_TITLE_SIZE_MAX - 1);
		}

		if (g_cfg.net.psn_status != np_psn_status::psn_rpcn || !is_psn_active)
		{
			return;
		}

		std::lock_guard lock(mutex_rpcn);

		if (!rpcn)
		{
			return;
		}

		auto current_presences = rpcn->get_presence_states();

		for (auto& [npid, pr_info] : current_presences)
		{
			basic_event to_add{};
			strcpy_trunc(to_add.from.userId.handle.data, npid);
			strcpy_trunc(to_add.from.name.data, npid);

			if (pr_info.online)
			{
				if (std::memcmp(pr_info.pr_com_id.data, basic_handler.context.data, sizeof(pr_info.pr_com_id.data)) == 0)
				{
					to_add.event = SCE_NP_BASIC_EVENT_PRESENCE;
					to_add.data = std::move(pr_info.pr_data);
				}
				else
				{
					if (basic_handler.context_sensitive)
					{
						to_add.event = SCE_NP_BASIC_EVENT_OUT_OF_CONTEXT;
					}
					else
					{
						to_add.event = SCE_NP_BASIC_EVENT_OFFLINE;
					}
				}
			}
			else
			{
				to_add.event = SCE_NP_BASIC_EVENT_OFFLINE;
			}

			queue_basic_event(to_add);
			send_basic_event(to_add.event, 0, 0);
		}

		send_basic_event(SCE_NP_BASIC_EVENT_END_OF_INITIAL_PRESENCE, 0, 0);
	}

	SceNpCommunicationId np_handler::get_basic_handler_context()
	{
		std::lock_guard lock(basic_handler.mutex);
		return basic_handler.context;
	}

	bool np_handler::send_basic_event(s32 event, s32 retCode, u32 reqId)
	{
		if (basic_handler_registered)
		{
			std::lock_guard lock(basic_handler.mutex);

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

		*event = cur_event.event;
		memcpy(from.get_ptr(), &cur_event.from, sizeof(cur_event.from));
		if (cur_event.event != SCE_NP_BASIC_EVENT_OFFLINE)
		{
			const u32 size_avail = *size;
			const u32 res_size   = std::min(static_cast<u32>(cur_event.data.size()), size_avail);

			memcpy(data.get_ptr(), cur_event.data.data(), res_size);
			*size = res_size;

			if (res_size < cur_event.data.size())
			{
				return SCE_NP_BASIC_ERROR_DATA_LOST;
			}
		}

		nph_log.notice("basic_event: event:%d, from:%s(%s), size:%d", *event, static_cast<char*>(from->userId.handle.data), static_cast<char*>(from->name.data), *size);

		return CELL_OK;
	}

	std::optional<shared_ptr<std::pair<std::string, message_data>>> np_handler::get_message(u64 id)
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

	std::optional<shared_ptr<std::pair<std::string, message_data>>> np_handler::get_message_selected(SceNpBasicAttachmentDataId id)
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

	void np_handler::send_message(const message_data& msg_data, const std::set<std::string>& npids)
	{
		get_rpcn()->send_message(msg_data, npids);
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
					const rpcn::CommandType command = static_cast<rpcn::CommandType>(reply.second.first);
					const u32 req_id      = reply.first;
					std::vector<u8>& data = reply.second.second;

					// Every reply should at least contain a return value/error code
					ensure(data.size() >= 1);
					const auto error = static_cast<rpcn::ErrorType>(data[0]);
					vec_stream reply_data(data, 1);

					switch (command)
					{
					case rpcn::CommandType::GetWorldList: reply_get_world_list(req_id, error, reply_data); break;
					case rpcn::CommandType::CreateRoom: reply_create_join_room(req_id, error, reply_data); break;
					case rpcn::CommandType::JoinRoom: reply_join_room(req_id, error, reply_data); break;
					case rpcn::CommandType::LeaveRoom: reply_leave_room(req_id, error, reply_data); break;
					case rpcn::CommandType::SearchRoom: reply_search_room(req_id, error, reply_data); break;
					case rpcn::CommandType::GetRoomDataExternalList: reply_get_roomdata_external_list(req_id, error, reply_data); break;
					case rpcn::CommandType::SetRoomDataExternal: reply_set_roomdata_external(req_id, error); break;
					case rpcn::CommandType::GetRoomDataInternal: reply_get_roomdata_internal(req_id, error, reply_data); break;
					case rpcn::CommandType::SetRoomDataInternal: reply_set_roomdata_internal(req_id, error); break;
					case rpcn::CommandType::GetRoomMemberDataInternal: reply_get_roommemberdata_internal(req_id, error, reply_data); break;
					case rpcn::CommandType::SetRoomMemberDataInternal: reply_set_roommemberdata_internal(req_id, error); break;
					case rpcn::CommandType::SetUserInfo: reply_set_userinfo(req_id, error); break;
					case rpcn::CommandType::PingRoomOwner: reply_get_ping_info(req_id, error, reply_data); break;
					case rpcn::CommandType::SendRoomMessage: reply_send_room_message(req_id, error); break;
					case rpcn::CommandType::RequestSignalingInfos: reply_req_sign_infos(req_id, error, reply_data); break;
					case rpcn::CommandType::RequestTicket: reply_req_ticket(req_id, error, reply_data); break;
					case rpcn::CommandType::GetBoardInfos: reply_get_board_infos(req_id, error, reply_data); break;
					case rpcn::CommandType::RecordScore: reply_record_score(req_id, error, reply_data); break;
					case rpcn::CommandType::RecordScoreData: reply_record_score_data(req_id, error); break;
					case rpcn::CommandType::GetScoreData: reply_get_score_data(req_id, error, reply_data); break;
					case rpcn::CommandType::GetScoreRange: reply_get_score_range(req_id, error, reply_data); break;
					case rpcn::CommandType::GetScoreFriends: reply_get_score_friends(req_id, error, reply_data); break;
					case rpcn::CommandType::GetScoreNpid: reply_get_score_npid(req_id, error, reply_data); break;
					case rpcn::CommandType::TusSetMultiSlotVariable: reply_tus_set_multislot_variable(req_id, error); break;
					case rpcn::CommandType::TusGetMultiSlotVariable: reply_tus_get_multislot_variable(req_id, error, reply_data); break;
					case rpcn::CommandType::TusGetMultiUserVariable: reply_tus_get_multiuser_variable(req_id, error, reply_data); break;
					case rpcn::CommandType::TusGetFriendsVariable: reply_tus_get_friends_variable(req_id, error, reply_data); break;
					case rpcn::CommandType::TusAddAndGetVariable: reply_tus_add_and_get_variable(req_id, error, reply_data); break;
					case rpcn::CommandType::TusTryAndSetVariable: reply_tus_try_and_set_variable(req_id, error, reply_data); break;
					case rpcn::CommandType::TusDeleteMultiSlotVariable: reply_tus_delete_multislot_variable(req_id, error); break;
					case rpcn::CommandType::TusSetData: reply_tus_set_data(req_id, error); break;
					case rpcn::CommandType::TusGetData: reply_tus_get_data(req_id, error, reply_data); break;
					case rpcn::CommandType::TusGetMultiSlotDataStatus: reply_tus_get_multislot_data_status(req_id, error, reply_data); break;
					case rpcn::CommandType::TusGetMultiUserDataStatus: reply_tus_get_multiuser_data_status(req_id, error, reply_data); break;
					case rpcn::CommandType::TusGetFriendsDataStatus: reply_tus_get_friends_data_status(req_id, error, reply_data); break;
					case rpcn::CommandType::TusDeleteMultiSlotData: reply_tus_delete_multislot_data(req_id, error); break;
					case rpcn::CommandType::CreateRoomGUI: reply_create_room_gui(req_id, error, reply_data); break;
					case rpcn::CommandType::JoinRoomGUI: reply_join_room_gui(req_id, error, reply_data); break;
					case rpcn::CommandType::LeaveRoomGUI: reply_leave_room_gui(req_id, error, reply_data); break;
					case rpcn::CommandType::GetRoomListGUI: reply_get_room_list_gui(req_id, error, reply_data); break;
					case rpcn::CommandType::SetRoomSearchFlagGUI: reply_set_room_search_flag_gui(req_id, error); break;
					case rpcn::CommandType::GetRoomSearchFlagGUI: reply_get_room_search_flag_gui(req_id, error, reply_data); break;
					case rpcn::CommandType::SetRoomInfoGUI: reply_set_room_info_gui(req_id, error); break;
					case rpcn::CommandType::GetRoomInfoGUI: reply_get_room_info_gui(req_id, error, reply_data); break;
					case rpcn::CommandType::QuickMatchGUI: reply_quickmatch_gui(req_id, error, reply_data); break;
					case rpcn::CommandType::SearchJoinRoomGUI: reply_searchjoin_gui(req_id, error, reply_data); break;
					default: fmt::throw_exception("Unknown reply(%d) received!", command); break;
					}
				}

				auto notifications = rpcn->get_notifications();
				for (auto& notif : notifications)
				{
					vec_stream noti_data(notif.second);

					switch (notif.first)
					{
					case rpcn::NotificationType::UserJoinedRoom: notif_user_joined_room(noti_data); break;
					case rpcn::NotificationType::UserLeftRoom: notif_user_left_room(noti_data); break;
					case rpcn::NotificationType::RoomDestroyed: notif_room_destroyed(noti_data); break;
					case rpcn::NotificationType::UpdatedRoomDataInternal: notif_updated_room_data_internal(noti_data); break;
					case rpcn::NotificationType::UpdatedRoomMemberDataInternal: notif_updated_room_member_data_internal(noti_data); break;
					case rpcn::NotificationType::RoomMessageReceived: notif_room_message_received(noti_data); break;
					case rpcn::NotificationType::SignalingHelper: notif_signaling_helper(noti_data); break;
					case rpcn::NotificationType::MemberJoinedRoomGUI: notif_member_joined_room_gui(noti_data); break;
					case rpcn::NotificationType::MemberLeftRoomGUI: notif_member_left_room_gui(noti_data); break;
					case rpcn::NotificationType::RoomDisappearedGUI: notif_room_disappeared_gui(noti_data); break;
					case rpcn::NotificationType::RoomOwnerChangedGUI: notif_room_owner_changed_gui(noti_data); break;
					case rpcn::NotificationType::UserKickedGUI: notif_user_kicked_gui(noti_data); break;
					case rpcn::NotificationType::QuickMatchCompleteGUI: notif_quickmatch_complete_gui(noti_data); break;
					default: fmt::throw_exception("Unknown notification(%d) received!", notif.first); break;
					}
				}

				auto presence_updates = rpcn->get_presence_updates();
				if (basic_handler_registered)
				{
					for (auto& [npid, pr_info] : presence_updates)
					{
						basic_event to_add{};
						strcpy_trunc(to_add.from.userId.handle.data, npid);
						strcpy_trunc(to_add.from.name.data, npid);

						if (std::memcmp(pr_info.pr_com_id.data, basic_handler.context.data, sizeof(pr_info.pr_com_id.data)) == 0)
						{
							to_add.event = SCE_NP_BASIC_EVENT_PRESENCE;
							to_add.data = std::move(pr_info.pr_data);
						}
						else
						{
							if (basic_handler.context_sensitive && pr_info.online)
							{
								to_add.event = SCE_NP_BASIC_EVENT_OUT_OF_CONTEXT;
							}
							else
							{
								to_add.event = SCE_NP_BASIC_EVENT_OFFLINE;
							}
						}

						queue_basic_event(to_add);
						send_basic_event(to_add.event, 0, 0);
					}
				}

				auto messages = rpcn->get_new_messages();
				if (basic_handler_registered)
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
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_URL_ATTACHMENT:
								event = SCE_NP_BASIC_EVENT_INCOMING_ATTACHMENT;
								break;
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE:
								event = (msg->second.msgFeatures & SCE_NP_BASIC_MESSAGE_FEATURES_BOOTABLE) ? SCE_NP_BASIC_EVENT_INCOMING_BOOTABLE_INVITATION : SCE_NP_BASIC_EVENT_INCOMING_INVITATION;
								break;
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_CUSTOM_DATA:
								event = (msg->second.msgFeatures & SCE_NP_BASIC_MESSAGE_FEATURES_BOOTABLE) ? SCE_NP_BASIC_EVENT_INCOMING_BOOTABLE_CUSTOM_DATA_MESSAGE : SCE_NP_BASIC_EVENT_INCOMING_CUSTOM_DATA_MESSAGE;
								break;
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_GENERAL:
								event = SCE_NP_BASIC_EVENT_MESSAGE;
								break;
							case SCE_NP_BASIC_MESSAGE_MAIN_TYPE_ADD_FRIEND:
							default:
								continue;
							}

							basic_event to_add{};
							to_add.event = event;
							to_add.data = msg->second.data;
							strcpy_trunc(to_add.from.userId.handle.data, msg->first);
							strcpy_trunc(to_add.from.name.data, msg->first);

							queue_basic_event(std::move(to_add));
							send_basic_event(event, 0, 0);
						}
					}
				}

				if (!replies.empty() || !notifications.empty() || !presence_updates.empty())
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

		nph_log.trace("Callback used is 0x%x with req_id %d", ret.cb, req_id);

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

	player_history& np_handler::get_player_and_set_timestamp(const SceNpId& npid, u64 timestamp)
	{
		std::string npid_str = std::string(npid.handle.data);

		if (!players_history.contains(npid_str))
		{
			auto [it, success] = players_history.insert(std::make_pair(std::move(npid_str), player_history{.timestamp = timestamp}));
			ensure(success);
			return it->second;
		}

		auto& history = ::at32(players_history, npid_str);
		history.timestamp = timestamp;
		return history;
	}

	constexpr usz MAX_HISTORY_ENTRIES = 200;

	void np_handler::add_player_to_history(const SceNpId* npid, const char* description)
	{
		std::lock_guard lock(mutex_history);
		auto& history = get_player_and_set_timestamp(*npid, get_system_time());

		if (description)
			history.description = description;

		while (players_history.size() > MAX_HISTORY_ENTRIES)
		{
			auto it = std::min_element(players_history.begin(), players_history.end(), [](const auto& a, const auto& b) { return a.second.timestamp < b.second.timestamp; } );
			players_history.erase(it);
		}

		save_players_history();
	}

	u32 np_handler::add_players_to_history(const SceNpId* npids, const char* description, u32 count)
	{
		std::lock_guard lock(mutex_history);

		const std::string communication_id_str = std::string(basic_handler.context.data);

		for (u32 i = 0; i < count; i++)
		{
			auto& history = get_player_and_set_timestamp(npids[i], get_system_time());

			if (description)
				history.description = description;

			history.communication_ids.insert(communication_id_str);
		}

		while (players_history.size() > MAX_HISTORY_ENTRIES)
		{
			auto it = std::min_element(players_history.begin(), players_history.end(), [](const auto& a, const auto& b) { return a.second.timestamp < b.second.timestamp; } );
			players_history.erase(it);
		}

		save_players_history();

		const u32 req_id = get_req_id(REQUEST_ID_HIGH::MISC);
		send_basic_event(SCE_NP_BASIC_EVENT_ADD_PLAYERS_HISTORY_RESULT, 0, req_id);
		return req_id;
	}

	u32 np_handler::get_players_history_count(u32 options)
	{
		const bool all_history = (options == SCE_NP_BASIC_PLAYERS_HISTORY_OPTIONS_ALL);

		std::lock_guard lock(mutex_history);

		if (all_history)
		{
			return ::size32(players_history);
		}

		const std::string communication_id_str = std::string(basic_handler.context.data);
		return static_cast<u32>(std::count_if(players_history.begin(), players_history.end(), [&](const auto& entry)
			{
				return entry.second.communication_ids.contains(communication_id_str);
			}));
	}

	bool np_handler::get_player_history_entry(u32 options, u32 index, SceNpId* npid)
	{
		const bool all_history = (options == SCE_NP_BASIC_PLAYERS_HISTORY_OPTIONS_ALL);

		std::lock_guard lock(mutex_history);

		if (all_history)
		{
			auto it = players_history.begin();
			std::advance(it, index);

			if (it != players_history.end())
			{
				string_to_npid(it->first, *npid);
				return true;
			}
		}
		else
		{
			const std::string communication_id_str = std::string(basic_handler.context.data);

			// Get the nth element that contains the current communication_id
			for (auto it = players_history.begin(); it != players_history.end(); it++)
			{
				if (it->second.communication_ids.contains(communication_id_str))
				{
					if (index == 0)
					{
						string_to_npid(it->first, *npid);
						return true;
					}

					index--;
				}
			}
		}

		return false;
	}

	void np_handler::save_players_history()
	{
#ifdef _WIN32
		const std::string path_to_cfg = fs::get_config_dir(true);
		if (!fs::create_path(path_to_cfg))
		{
			nph_log.error("Could not create path: %s", path_to_cfg);
		}
#endif
		fs::file history_file(get_players_history_path(), fs::rewrite);
		if (!history_file)
			return;

		YAML::Emitter out;

		out << YAML::BeginMap;
		for (const auto& [player_npid, player_info] : players_history)
		{
			out << player_npid;
			out << YAML::BeginSeq;
			out << player_info.timestamp;
			out << player_info.description;
			out << YAML::BeginSeq;
			for (const auto& com_id : player_info.communication_ids)
			{
				out << com_id;
			}
			out << YAML::EndSeq;
			out << YAML::EndSeq;
		}
		out << YAML::EndMap;

		history_file.write(out.c_str(), out.size());
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

	void np_handler::set_presence(std::optional<std::string> status, std::optional<std::vector<u8>> data)
	{
		bool send_update = false;

		if (status)
		{
			if (status != presence_self.pr_status)
			{
				presence_self.pr_status = *status;
				send_update = true;
			}
		}

		if (data)
		{
			if (data != presence_self.pr_data)
			{
				presence_self.pr_data = *data;
				send_update = true;
			}
		}

		if (is_psn_active && (!presence_self.advertised || send_update))
		{
			std::lock_guard lock(mutex_rpcn);

			if (!rpcn)
			{
				return;
			}

			presence_self.advertised = true;
			rpcn->send_presence(presence_self.pr_com_id, presence_self.pr_title, presence_self.pr_status, presence_self.pr_comment, presence_self.pr_data);
		}
	}

	template <typename T>
	error_code np_handler::get_friend_presence_by_index(u32 index, SceNpUserInfo* user, T* pres)
	{
		if (!is_psn_active || g_cfg.net.psn_status != np_psn_status::psn_rpcn)
		{
			return SCE_NP_BASIC_ERROR_NOT_CONNECTED;
		}

		std::lock_guard lock(mutex_rpcn);

		if (!rpcn)
		{
			return SCE_NP_BASIC_ERROR_NOT_CONNECTED;
		}

		auto friend_infos = rpcn->get_friend_presence_by_index(index);
		if (!friend_infos)
		{
			return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
		}

		const bool same_context = (std::memcmp(friend_infos->second.pr_com_id.data, basic_handler.context.data, sizeof(friend_infos->second.pr_com_id.data)) == 0);
		strings_to_userinfo(friend_infos->first, friend_infos->first, "", *user);
		onlinedata_to_presencedetails(friend_infos->second, same_context, *pres);

		return CELL_OK;
	}

	template error_code np_handler::get_friend_presence_by_index<SceNpBasicPresenceDetails>(u32 index, SceNpUserInfo* user, SceNpBasicPresenceDetails* pres);
	template error_code np_handler::get_friend_presence_by_index<SceNpBasicPresenceDetails2>(u32 index, SceNpUserInfo* user, SceNpBasicPresenceDetails2* pres);

	template <typename T>
	error_code np_handler::get_friend_presence_by_npid(const SceNpId& npid, T* pres)
	{
		if (!is_psn_active || g_cfg.net.psn_status != np_psn_status::psn_rpcn)
		{
			return SCE_NP_BASIC_ERROR_NOT_CONNECTED;
		}

		std::lock_guard lock(mutex_rpcn);
		
		if (!rpcn)
		{
			return SCE_NP_BASIC_ERROR_NOT_CONNECTED;
		}

		auto friend_infos = rpcn->get_friend_presence_by_npid(std::string(npid.handle.data));
		if (!friend_infos)
		{
			return SCE_NP_BASIC_ERROR_INVALID_ARGUMENT;
		}

		const bool same_context = (std::memcmp(friend_infos->second.pr_com_id.data, basic_handler.context.data, sizeof(friend_infos->second.pr_com_id.data)) == 0);
		onlinedata_to_presencedetails(friend_infos->second, same_context, *pres);

		return CELL_OK;
	}

	template error_code np_handler::get_friend_presence_by_npid<SceNpBasicPresenceDetails>(const SceNpId& npid, SceNpBasicPresenceDetails* pres);
	template error_code np_handler::get_friend_presence_by_npid<SceNpBasicPresenceDetails2>(const SceNpId& npid, SceNpBasicPresenceDetails2* pres);

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

	error_code np_handler::local_get_room_member_data(SceNpMatching2RoomId room_id, SceNpMatching2RoomMemberId member_id, const std::vector<SceNpMatching2AttributeId>& binattrs_list, SceNpMatching2RoomMemberDataInternal* ptr_member, u32 addr_data, u32 size_data, u32 ctx_id)
	{
		auto [include_onlinename, include_avatarurl] = get_match2_context_options(ctx_id);
		return np_cache.get_member_and_attrs(room_id, member_id, binattrs_list, ptr_member, addr_data, size_data, include_onlinename, include_avatarurl);
	}

	void np_handler::upnp_add_port_mapping(u16 internal_port, std::string_view protocol)
	{
		upnp.add_port_redir(np::ip_to_string(get_local_ip_addr()), internal_port, protocol);
	}

	void np_handler::upnp_remove_port_mapping(u16 internal_port, std::string_view protocol)
	{
		upnp.remove_port_redir(internal_port, protocol);
	}

	std::pair<bool, bool> np_handler::get_match2_context_options(u32 ctx_id)
	{
		bool include_onlinename = false, include_avatarurl = false;

		if (auto ctx = get_match2_context(ctx_id))
		{
			include_onlinename = ctx->include_onlinename;
			include_avatarurl = ctx->include_avatarurl;
		}

		return {include_onlinename, include_avatarurl};
	}

	void np_handler::add_gui_request(u32 req_id, u32 ctx_id)
	{
		std::lock_guard lock(gui_requests.mutex);
		ensure(gui_requests.list.insert({req_id, ctx_id}).second);
	}

	void np_handler::remove_gui_request(u32 req_id)
	{
		std::lock_guard lock(gui_requests.mutex);
		if (gui_requests.list.erase(req_id) != 1)
		{
			rpcn_log.error("Failed to erase gui request %d", req_id);
		}
	}

	u32 np_handler::take_gui_request(u32 req_id)
	{
		std::lock_guard lock(gui_requests.mutex);

		if (!gui_requests.list.contains(req_id))
		{
			return 0;
		}

		const u32 ctx_id = ::at32(gui_requests.list, req_id);
		gui_requests.list.erase(req_id);

		return ctx_id;
	}

	shared_ptr<matching_ctx> np_handler::take_pending_gui_request(u32 req_id)
	{
		const u32 ctx_id = take_gui_request(req_id);

		if (!ctx_id)
			return {};

		auto ctx = get_matching_context(ctx_id);

		if (!ctx)
			return {};

		return ctx;
	}

} // namespace np
