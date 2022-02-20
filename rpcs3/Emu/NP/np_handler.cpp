#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/NP/np_handler.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Emu/Cell/Modules/cellNetCtl.h"
#include "Utilities/StrUtil.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/IdManager.h"
#include "Emu/NP/np_structs_extra.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_config.h"
#include "Emu/NP/np_contexts.h"
#include "Emu/NP/np_helpers.h"

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

LOG_CHANNEL(sys_net);
LOG_CHANNEL(sceNp2);
LOG_CHANNEL(sceNp);

LOG_CHANNEL(rpcn_log, "rpcn");
LOG_CHANNEL(nph_log, "NPHandler");

namespace np
{
	np_handler::np_handler()
	{
		g_fxo->need<named_thread<signaling_handler>>();

		std::lock_guard lock(mutex_rpcn);
		rpcn = rpcn::rpcn_client::get_instance();

		is_connected  = (g_cfg.net.net_active == np_internet_status::enabled);
		is_psn_active = (g_cfg.net.psn_status >= np_psn_status::psn_fake);

		if (get_net_status() == CELL_NET_CTL_STATE_IPObtained)
		{
			discover_ip_address();

			if (!discover_ether_address())
			{
				nph_log.error("Failed to discover ethernet address!");
				is_connected  = false;
				is_psn_active = false;
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
		}
	}

	void np_handler::discover_ip_address()
	{
		hostname.clear();
		hostname.resize(1024);

		const auto use_default_ip_addr = [this](const std::string_view error_msg)
		{
			nph_log.error("discover_ip_address: %s", error_msg);
			nph_log.error("discover_ip_address: Defaulting to 127.0.0.1!");
			local_ip_addr  = 0x0100007f;
			public_ip_addr = local_ip_addr;
		};

		if (gethostname(hostname.data(), hostname.size()) == -1)
		{
			use_default_ip_addr("gethostname failed!");
			return;
		}

		nph_log.notice("discover_ip_address: Hostname was determined to be %s", hostname.c_str());

		hostent* host = gethostbyname(hostname.data());
		if (!host)
		{
			use_default_ip_addr("gethostbyname failed!");
			return;
		}

		if (host->h_addrtype != AF_INET)
		{
			use_default_ip_addr("Could only find IPv6 addresses for current host!");
			return;
		}

		// First address is used for now, (TODO combobox with possible local addresses to use?)
		local_ip_addr = *reinterpret_cast<u32*>(host->h_addr_list[0]);

		// Set public address to local discovered address for now, may be updated later;
		public_ip_addr = local_ip_addr;

		nph_log.notice("discover_ip_address: IP was determined to be %s", ip_to_string(local_ip_addr));
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
					nph_log.notice("Determined Ethernet address to be %s", ether_to_string(ether_address));
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
			nph_log.notice("Determined Ethernet address to be %s", ether_to_string(ether_address));
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
			nph_log.notice("Determined Ethernet address to be %s", ether_to_string(ether_address));

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

	s32 np_handler::get_net_status() const
	{
		return is_connected ? CELL_NET_CTL_STATE_IPObtained : CELL_NET_CTL_STATE_Disconnected;
	}

	s32 np_handler::get_psn_status() const
	{
		return is_psn_active ? SCE_NP_MANAGER_STATUS_ONLINE : SCE_NP_MANAGER_STATUS_OFFLINE;
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
		// Init memory pool
		np_memory.setup(poolptr, poolsize);

		memset(&npid, 0, sizeof(npid));
		memset(&online_name, 0, sizeof(online_name));
		memset(&avatar_url, 0, sizeof(avatar_url));

		if (g_cfg.net.psn_status >= np_psn_status::psn_fake)
		{
			std::string s_npid = g_cfg_rpcn.get_npid();
			ensure(!s_npid.empty()); // It should have been generated before this

			string_to_npid(s_npid, &npid);
			auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
			sigh.set_self_sig_info(npid);
		}

		switch (g_cfg.net.psn_status)
		{
		case np_psn_status::disabled:
			break;
		case np_psn_status::psn_fake:
		{
			string_to_online_name("RPCS3's user", &online_name);
			string_to_avatar_url("https://rpcs3.net/cdn/netplay/DefaultAvatar.png", &avatar_url);
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
				rpcn_log.error("Connection to RPCN Failed!");
				is_psn_active = false;
				return;
			}

			if (auto state = rpcn->wait_for_authentified(); state != rpcn::rpcn_state::failure_no_failure)
			{
				rpcn_log.error("RPCN login attempt failed!");
				is_psn_active = false;
				return;
			}

			string_to_online_name(rpcn->get_online_name(), &online_name);
			string_to_avatar_url(rpcn->get_avatar_url(), &avatar_url);
			public_ip_addr = rpcn->get_addr_sig();

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

		auto& data = match2_req_results.at(event_key);
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

	error_code np_handler::get_basic_event(vm::ptr<s32> event, vm::ptr<SceNpUserInfo> from, vm::ptr<s32> data, vm::ptr<u32> size)
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

		return CELL_OK;
	}

	std::optional<std::shared_ptr<std::pair<std::string, message_data>>> np_handler::get_message(u64 id)
	{
		return rpcn->get_message(id);
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

	u32 np_handler::generate_callback_info(SceNpMatching2ContextId ctx_id, vm::cptr<SceNpMatching2RequestOptParam> optParam)
	{
		callback_info ret;

		const auto ctx = get_match2_context(ctx_id);
		ensure(ctx);

		const u32 req_id = get_req_id(optParam ? optParam->appReqId : ctx->default_match2_optparam.appReqId);

		ret.ctx_id = ctx_id;
		ret.cb_arg = optParam ? optParam->cbFuncArg : ctx->default_match2_optparam.cbFuncArg;

		if (optParam && optParam->cbFunc)
		{
			ret.cb = optParam->cbFunc;
		}
		else
		{
			ret.cb = ctx->default_match2_optparam.cbFunc;
		}

		nph_log.warning("Callback used is 0x%x", ret.cb);

		{
			std::lock_guard lock(mutex_pending_requests);
			pending_requests[req_id] = std::move(ret);
		}

		return req_id;
	}

	np_handler::callback_info np_handler::take_pending_request(u32 req_id)
	{
		std::lock_guard lock(mutex_pending_requests);

		const auto cb_info = std::move(pending_requests.at(req_id));
		pending_requests.erase(req_id);

		return cb_info;
	}

	event_data& np_handler::allocate_req_result(u32 event_key, u32 max_size, u32 initial_size)
	{
		std::lock_guard lock(mutex_match2_req_results);
		match2_req_results.emplace(std::piecewise_construct, std::forward_as_tuple(event_key), std::forward_as_tuple(np_memory.allocate(max_size), initial_size, max_size));
		return match2_req_results.at(event_key);
	}

	u32 np_handler::add_players_to_history(vm::cptr<SceNpId> /*npids*/, u32 /*count*/)
	{
		const u32 req_id = get_req_id(0);

		// if (basic_handler)
		// {
		// 	sysutil_register_cb([basic_handler = this->basic_handler, req_id, basic_handler_arg = this->basic_handler_arg](ppu_thread& cb_ppu) -> s32
		// 	{
		// 		basic_handler(cb_ppu, SCE_NP_BASIC_EVENT_ADD_PLAYERS_HISTORY_RESULT, 0, req_id, basic_handler_arg);
		// 		return 0;
		// 	});
		// }

		return req_id;
	}

	u32 np_handler::get_num_friends()
	{
		return rpcn->get_num_friends();
	}

	u32 np_handler::get_num_blocks()
	{
		return rpcn->get_num_blocks();
	}

	std::pair<error_code, std::optional<SceNpId>> np_handler::get_friend_by_index(u32 index)
	{
		auto str_friend = rpcn->get_friend_by_index(index);

		if (!str_friend)
		{
			return {SCE_NP_ERROR_ID_NOT_FOUND, {}};
		}

		SceNpId npid_friend;
		string_to_npid(str_friend.value(), &npid_friend);

		return {CELL_OK, npid_friend};
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
} // namespace np
