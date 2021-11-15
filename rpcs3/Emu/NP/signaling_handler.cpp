#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "signaling_handler.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "np_handler.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

LOG_CHANNEL(sign_log, "Signaling");

std::vector<std::pair<std::pair<u32, u16>, std::vector<u8>>> get_sign_msgs();
s32 send_packet_from_p2p_port(const std::vector<u8>& data, const sockaddr_in& addr);
void need_network();

template <>
void fmt_class_string<SignalingCommand>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case signal_ping: return "PING";
		case signal_pong: return "PONG";
		case signal_connect: return "CONNECT";
		case signal_connect_ack: return "CONNECT_ACK";
		case signal_confirm: return "CONFIRM";
		case signal_finished: return "FINISHED";
		case signal_finished_ack: return "FINISHED_ACK";
		}

		return unknown;
	});
}

signaling_handler::signaling_handler()
{
	need_network();
}

/////////////////////////////
//// SIGNALING CALLBACKS ////
/////////////////////////////

void signaling_handler::set_sig_cb(u32 sig_cb_ctx, vm::ptr<SceNpSignalingHandler> sig_cb, vm::ptr<void> sig_cb_arg)
{
	std::lock_guard lock(data_mutex);
	this->sig_cb_ctx = sig_cb_ctx;
	this->sig_cb     = sig_cb;
	this->sig_cb_arg = sig_cb_arg;
}

void signaling_handler::set_ext_sig_cb(u32 sig_ext_cb_ctx, vm::ptr<SceNpSignalingHandler> sig_ext_cb, vm::ptr<void> sig_ext_cb_arg)
{
	std::lock_guard lock(data_mutex);
	this->sig_ext_cb_ctx = sig_ext_cb_ctx;
	this->sig_ext_cb     = sig_ext_cb;
	this->sig_ext_cb_arg = sig_ext_cb_arg;
}

void signaling_handler::set_sig2_cb(u16 sig2_cb_ctx, vm::ptr<SceNpMatching2SignalingCallback> sig2_cb, vm::ptr<void> sig2_cb_arg)
{
	std::lock_guard lock(data_mutex);
	this->sig2_cb_ctx = sig2_cb_ctx;
	this->sig2_cb     = sig2_cb;
	this->sig2_cb_arg = sig2_cb_arg;
}

void signaling_handler::signal_sig_callback(u32 conn_id, int event)
{
	if (sig_cb)
	{
		sysutil_register_cb([sig_cb = this->sig_cb, sig_cb_ctx = this->sig_cb_ctx, conn_id, event, sig_cb_arg = this->sig_cb_arg](ppu_thread& cb_ppu) -> s32
		{
			sig_cb(cb_ppu, sig_cb_ctx, conn_id, event, 0, sig_cb_arg);
			return 0;
		});
		sign_log.notice("Called sig CB: 0x%x (conn_id: %d)", event, conn_id);
	}

	// extended callback also receives normal events
	signal_ext_sig_callback(conn_id, event);
}

void signaling_handler::signal_ext_sig_callback(u32 conn_id, int event) const
{
	if (sig_ext_cb)
	{
		sysutil_register_cb([sig_ext_cb = this->sig_ext_cb, sig_ext_cb_ctx = this->sig_ext_cb_ctx, conn_id, event, sig_ext_cb_arg = this->sig_ext_cb_arg](ppu_thread& cb_ppu) -> s32
		{
			sig_ext_cb(cb_ppu, sig_ext_cb_ctx, conn_id, event, 0, sig_ext_cb_arg);
			return 0;
		});
		sign_log.notice("Called EXT sig CB: 0x%x (conn_id: %d, member_id: %d)", event, conn_id);
	}
}

void signaling_handler::signal_sig2_callback(u64 room_id, u16 member_id, SceNpMatching2Event event) const
{
	// Signal the callback
	if (sig2_cb)
	{
		sysutil_register_cb([sig2_cb = this->sig2_cb, sig2_cb_ctx = this->sig2_cb_ctx, room_id, member_id, event, sig2_cb_arg = this->sig2_cb_arg](ppu_thread& cb_ppu) -> s32
		{
			sig2_cb(cb_ppu, sig2_cb_ctx, room_id, member_id, event, 0, sig2_cb_arg);
			return 0;
		});
	}

	sign_log.notice("Called sig2 CB: 0x%x (room_id: %d, member_id: %d)", event, room_id, member_id);
}

///////////////////////////////////
//// SIGNALING MSGS PROCESSING ////
///////////////////////////////////

void signaling_handler::reschedule_packet(std::shared_ptr<signaling_info>& si, SignalingCommand cmd, steady_clock::time_point new_timepoint)
{
	for (auto it = qpackets.begin(); it != qpackets.end(); it++)
	{
		if (it->second.packet.command == cmd && it->second.sig_info == si)
		{
			auto new_queue  = qpackets.extract(it);
			new_queue.key() = new_timepoint;
			qpackets.insert(std::move(new_queue));
			return;
		}
	}
}

void signaling_handler::retire_packet(std::shared_ptr<signaling_info>& si, SignalingCommand cmd)
{
	for (auto it = qpackets.begin(); it != qpackets.end(); it++)
	{
		if (it->second.packet.command == cmd && it->second.sig_info == si)
		{
			qpackets.erase(it);
			return;
		}
	}
}

void signaling_handler::retire_all_packets(std::shared_ptr<signaling_info>& si)
{
	for (auto it = qpackets.begin(); it != qpackets.end();)
	{
		if (it->second.sig_info == si)
			it = qpackets.erase(it);
		else
			it++;
	}
}

bool signaling_handler::validate_signaling_packet(const signaling_packet* sp)
{
	if (sp->signature != SIGNALING_SIGNATURE)
	{
		sign_log.error("Received a signaling packet with an invalid signature");
		return false;
	}

	if (sp->version != 1u && sp->version != 2u)
	{
		sign_log.error("Invalid version in signaling packet");
		return false;
	}

	return true;
}

void signaling_handler::process_incoming_messages()
{
	auto msgs = get_sign_msgs();

	for (const auto& msg : msgs)
	{
		if (msg.second.size() != sizeof(signaling_packet))
		{
			sign_log.error("Received an invalid signaling packet");
			continue;
		}

		auto op_addr = msg.first.first;
		auto op_port = msg.first.second;
		auto* sp     = reinterpret_cast<const signaling_packet*>(msg.second.data());

		if (!validate_signaling_packet(sp))
			continue;

		if (sign_log.enabled == logs::level::trace)
		{
			in_addr addr;
			addr.s_addr = op_addr;

			if (sp->version == 1u)
			{
				char npid_buf[17]{};
				memcpy(npid_buf, sp->V1.npid.handle.data, 16);
				std::string npid(npid_buf);

				char ip_str[16];
				inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));

				sign_log.trace("sig1 %s from %s:%d(%s)", sp->command, ip_str, op_port, npid);
			}
			else
			{
				char inet_addr[16];
				const char* inet_addr_string = inet_ntop(AF_INET, &addr, inet_addr, sizeof(inet_addr));

				sign_log.trace("sig2 %s from %s:%d(%d:%d)", sp->command, inet_addr_string, op_port, sp->V2.room_id, sp->V2.member_id);
			}
		}

		bool reply = false, schedule_repeat = false;
		auto& sent_packet = sp->version == 1u ? sig1_packet : sig2_packet;

		// Get signaling info for user to know if we should even bother looking further
		auto si = get_signaling_ptr(sp);

		if (!si && sp->version == 1u && sp->command == signal_connect)
		{
			// Connection can be remotely established and not mutual
			const u32 conn_id = create_sig_infos(&sp->V1.npid);
			si = sig1_peers.at(conn_id);
			// Activate the connection without triggering the main CB
			si->connStatus = SCE_NP_SIGNALING_CONN_STATUS_ACTIVE;
			si->addr = op_addr;
			si->port = op_port;
			si->ext_status = ext_sign_peer;
			// Notify extended callback that peer activated
			signal_ext_sig_callback(conn_id, SCE_NP_SIGNALING_EVENT_EXT_PEER_ACTIVATED);
		}

		if (!si || (si->connStatus == SCE_NP_SIGNALING_CONN_STATUS_INACTIVE && sp->command != signal_finished))
		{
			// User is unknown to us or the connection is inactive
			// Ignore packet unless it's a finished packet in case the finished_ack wasn't received by opponent
			continue;
		}

		const auto now = steady_clock::now();
		if (si)
			si->time_last_msg_recvd = now;

		const auto setup_ping = [&]()
		{
			for (auto it = qpackets.begin(); it != qpackets.end(); it++)
			{
				if (it->second.packet.command == signal_ping && it->second.sig_info == si)
				{
					return;
				}
			}

			sent_packet.command = signal_ping;
			queue_signaling_packet(sent_packet, si, now + REPEAT_PING_DELAY);
		};

		switch (sp->command)
		{
		case signal_ping:
			reply               = true;
			schedule_repeat     = false;
			sent_packet.command = signal_pong;
			break;
		case signal_pong:
			reply           = false;
			schedule_repeat = false;
			reschedule_packet(si, signal_ping, now + 15s);
			break;
		case signal_connect:
			reply               = true;
			schedule_repeat     = true;
			sent_packet.command = signal_connect_ack;
			// connection is established
			// TODO: notify extended callback!
			break;
		case signal_connect_ack:
			reply           = true;
			schedule_repeat = false;
			setup_ping();
			sent_packet.command = signal_confirm;
			retire_packet(si, signal_connect);
			// connection is active
			update_si_addr(si, op_addr, op_port);
			update_si_mapped_addr(si, sp->sent_addr, sp->sent_port);
			update_si_status(si, SCE_NP_SIGNALING_CONN_STATUS_ACTIVE);
			break;
		case signal_confirm:
			reply           = false;
			schedule_repeat = false;
			setup_ping();
			retire_packet(si, signal_connect_ack);
			// connection is active
			update_si_addr(si, op_addr, op_port);
			update_si_mapped_addr(si, sp->sent_addr, sp->sent_port);
			update_si_status(si, SCE_NP_SIGNALING_CONN_STATUS_ACTIVE, true);
			break;
		case signal_finished:
			reply               = true;
			schedule_repeat     = false;
			sent_packet.command = signal_finished_ack;
			// terminate connection
			update_si_status(si, SCE_NP_SIGNALING_CONN_STATUS_INACTIVE);
			break;
		case signal_finished_ack:
			reply           = false;
			schedule_repeat = false;
			retire_packet(si, signal_finished);
			break;
		default: sign_log.error("Invalid signaling command received"); continue;
		}

		if (reply)
		{
			send_signaling_packet(sent_packet, op_addr, op_port);
			if (schedule_repeat)
				queue_signaling_packet(sent_packet, si, now + REPEAT_CONNECT_DELAY);
		}
	}
}

void signaling_handler::operator()()
{
	while (thread_ctrl::state() != thread_state::aborting)
	{
		std::unique_lock<std::mutex> lock(data_mutex);
		if (!qpackets.empty())
			wakey.wait_until(lock, qpackets.begin()->first);
		else
			wakey.wait(lock);

		if (thread_ctrl::state() == thread_state::aborting)
			return;

		process_incoming_messages();

		const auto now = steady_clock::now();

		for (auto it = qpackets.begin(); it != qpackets.end();)
		{
			if (it->first > now)
				break;

			if (it->second.sig_info->time_last_msg_recvd < now - 60s)
			{
				// We had no connection to opponent for 60 seconds, consider the connection dead
				sign_log.trace("Timeout disconnection");
				update_si_status(it->second.sig_info, SCE_NP_SIGNALING_CONN_STATUS_INACTIVE);
				break; // qpackets will be emptied of all packets from this user so we're requeuing
			}

			// Resend the packet
			send_signaling_packet(it->second.packet, it->second.sig_info->addr, it->second.sig_info->port);

			// Reschedule another packet
			SignalingCommand cmd = it->second.packet.command;
			auto& si             = it->second.sig_info;

			std::chrono::milliseconds delay(500);
			switch (cmd)
			{
			case signal_ping:
			case signal_pong:
				delay = REPEAT_PING_DELAY;
				break;
			case signal_connect:
			case signal_connect_ack:
			case signal_confirm:
				delay = REPEAT_CONNECT_DELAY;
				break;
			case signal_finished:
			case signal_finished_ack:
				delay = REPEAT_FINISHED_DELAY;
				break;
			}

			it++;
			reschedule_packet(si, cmd, now + delay);
		}
	}
}

void signaling_handler::wake_up()
{
	wakey.notify_one();
}

signaling_handler& signaling_handler::operator=(thread_state)
{
	wakey.notify_one();
	return *this;
}

void signaling_handler::update_si_addr(std::shared_ptr<signaling_info>& si, u32 new_addr, u16 new_port)
{
	ensure(si);

	if (si->addr != new_addr || si->port != new_port)
	{
		in_addr addr_old, addr_new;
		addr_old.s_addr = si->addr;
		addr_new.s_addr = new_addr;

		char ip_str_old[16];
		char ip_str_new[16];
		inet_ntop(AF_INET, &addr_old, ip_str_old, sizeof(ip_str_old));
		inet_ntop(AF_INET, &addr_new, ip_str_new, sizeof(ip_str_new));

		sign_log.trace("Updated Address from %s:%d to %s:%d", ip_str_old, si->port, ip_str_new, new_port);
		si->addr = new_addr;
		si->port = new_port;
	}
}

void signaling_handler::update_si_mapped_addr(std::shared_ptr<signaling_info>& si, u32 new_addr, u16 new_port)
{
	ensure(si);

	if (si->addr != new_addr || si->port != new_port)
	{
		in_addr addr_old, addr_new;
		addr_old.s_addr = si->addr;
		addr_new.s_addr = new_addr;

		char ip_str_old[16];
		char ip_str_new[16];
		inet_ntop(AF_INET, &addr_old, ip_str_old, sizeof(ip_str_old));
		inet_ntop(AF_INET, &addr_new, ip_str_new, sizeof(ip_str_new));

		sign_log.trace("Updated Mapped Address from %s:%d to %s:%d", ip_str_old, si->port, ip_str_new, new_port);
		si->mapped_addr = new_addr;
		si->mapped_port = new_port;
	}
}

void signaling_handler::update_si_status(std::shared_ptr<signaling_info>& si, s32 new_status, bool confirm_packet)
{
	if (!si)
		return;

	if (si->connStatus == SCE_NP_SIGNALING_CONN_STATUS_PENDING && new_status == SCE_NP_SIGNALING_CONN_STATUS_ACTIVE)
	{
		si->connStatus = SCE_NP_SIGNALING_CONN_STATUS_ACTIVE;
		ensure(si->version == 1u || si->version == 2u);
		if (si->version == 1u)
			signal_sig_callback(si->conn_id, SCE_NP_SIGNALING_EVENT_ESTABLISHED);
		else
			signal_sig2_callback(si->room_id, si->member_id, SCE_NP_MATCHING2_SIGNALING_EVENT_Established);

		return;
	}

	if ((si->connStatus == SCE_NP_SIGNALING_CONN_STATUS_PENDING || si->connStatus == SCE_NP_SIGNALING_CONN_STATUS_ACTIVE) && new_status == SCE_NP_SIGNALING_CONN_STATUS_INACTIVE)
	{
		si->connStatus = SCE_NP_SIGNALING_CONN_STATUS_INACTIVE;
		ensure(si->version == 1u || si->version == 2u);
		if (si->version == 1u)
			signal_sig_callback(si->conn_id, SCE_NP_SIGNALING_EVENT_DEAD);
		else
			signal_sig2_callback(si->room_id, si->member_id, SCE_NP_MATCHING2_SIGNALING_EVENT_Dead);

		retire_all_packets(si);
		return;
	}

	if (confirm_packet && si->version == 1u && si->ext_status == ext_sign_none)
	{
		si->ext_status = ext_sign_mutual;
		signal_ext_sig_callback(si->conn_id, SCE_NP_SIGNALING_EVENT_EXT_MUTUAL_ACTIVATED);
	}
}

void signaling_handler::set_self_sig_info(SceNpId& npid)
{
	std::lock_guard lock(data_mutex);
	sig1_packet.V1.npid = npid;
}

void signaling_handler::set_self_sig2_info(u64 room_id, u16 member_id)
{
	std::lock_guard lock(data_mutex);
	sig2_packet.V2.room_id   = room_id;
	sig2_packet.V2.member_id = member_id;
}

void signaling_handler::send_signaling_packet(signaling_packet& sp, u32 addr, u16 port) const
{
	std::vector<u8> packet(sizeof(signaling_packet) + sizeof(u16));
	reinterpret_cast<le_t<u16>&>(packet[0]) = 65535;
	memcpy(packet.data() + sizeof(u16), &sp, sizeof(signaling_packet));

	sockaddr_in dest;
	memset(&dest, 0, sizeof(sockaddr_in));
	dest.sin_family      = AF_INET;
	dest.sin_addr.s_addr = addr;
	dest.sin_port        = std::bit_cast<u16, be_t<u16>>(port);

	char ip_str[16];
	inet_ntop(AF_INET, &dest.sin_addr, ip_str, sizeof(ip_str));

	sign_log.trace("Sending %s packet to %s:%d", sp.command, ip_str, port);

	sp.sent_addr = addr;
	sp.sent_port = port;

	if (send_packet_from_p2p_port(packet, dest) == -1)
	{
		sign_log.error("Failed to send signaling packet to %s:%d", ip_str, port);
	}
}

void signaling_handler::queue_signaling_packet(signaling_packet& sp, std::shared_ptr<signaling_info> si, steady_clock::time_point wakeup_time)
{
	queued_packet qp;
	qp.sig_info = std::move(si);
	qp.packet   = sp;
	qpackets.emplace(wakeup_time, std::move(qp));
}

std::shared_ptr<signaling_info> signaling_handler::get_signaling_ptr(const signaling_packet* sp)
{
	// V1
	if (sp->version == 1u)
	{
		char npid_buf[17]{};
		memcpy(npid_buf, sp->V1.npid.handle.data, 16);
		std::string npid(npid_buf);

		if (!npid_to_conn_id.count(npid))
			return nullptr;

		const u32 conn_id = npid_to_conn_id.at(npid);
		if (!sig1_peers.contains(conn_id))
		{
			sign_log.error("Discrepancy in signaling 1 data");
			return nullptr;
		}

		return sig1_peers.at(conn_id);
	}

	// V2
	auto room_id   = sp->V2.room_id;
	auto member_id = sp->V2.member_id;
	if (!sig2_peers.contains(room_id) || !sig2_peers.at(room_id).contains(member_id))
		return nullptr;

	return sig2_peers.at(room_id).at(member_id);
}

void signaling_handler::start_sig(u32 conn_id, u32 addr, u16 port)
{
	std::lock_guard lock(data_mutex);
	start_sig_nl(conn_id, addr, port);
}

void signaling_handler::start_sig_nl(u32 conn_id, u32 addr, u16 port)
{
	auto& sent_packet   = sig1_packet;
	sent_packet.command = signal_connect;

	ensure(sig1_peers.contains(conn_id));
	std::shared_ptr<signaling_info> si = sig1_peers.at(conn_id);

	si->addr = addr;
	si->port = port;

	send_signaling_packet(sent_packet, si->addr, si->port);
	queue_signaling_packet(sent_packet, si, steady_clock::now() + REPEAT_CONNECT_DELAY);
	wake_up();
}

void signaling_handler::start_sig2(u64 room_id, u16 member_id)
{
	std::lock_guard lock(data_mutex);

	auto& sent_packet   = sig2_packet;
	sent_packet.command = signal_connect;

	ensure(sig2_peers.contains(room_id));
	const auto& sp = sig2_peers.at(room_id);

	ensure(sp.contains(member_id));
	std::shared_ptr<signaling_info> si = sp.at(member_id);

	send_signaling_packet(sent_packet, si->addr, si->port);
	queue_signaling_packet(sent_packet, si, steady_clock::now() + REPEAT_CONNECT_DELAY);
	wake_up();
}

void signaling_handler::disconnect_sig2_users(u64 room_id)
{
	std::lock_guard lock(data_mutex);

	if (!sig2_peers.contains(room_id))
		return;

	auto& sent_packet = sig2_packet;

	sent_packet.command = signal_finished;

	for (const auto& member : sig2_peers.at(room_id))
	{
		auto& si = member.second;
		if (si->connStatus != SCE_NP_SIGNALING_CONN_STATUS_INACTIVE && !si->self)
		{
			send_signaling_packet(sent_packet, si->addr, si->port);
			queue_signaling_packet(sent_packet, si, steady_clock::now() + REPEAT_FINISHED_DELAY);
		}
	}
}

u32 signaling_handler::create_sig_infos(const SceNpId* npid)
{
	ensure(npid->handle.data[16] == 0);
	std::string npid_str(reinterpret_cast<const char*>(npid->handle.data));

	if (npid_to_conn_id.contains(npid_str))
	{
		return npid_to_conn_id.at(npid_str);
	}

	const u32 conn_id = cur_conn_id++;
	npid_to_conn_id.emplace(npid_str, conn_id);
	sig1_peers.emplace(conn_id, std::make_shared<signaling_info>());
	sig1_peers.at(conn_id)->version = 1;
	sig1_peers.at(conn_id)->conn_id = conn_id;

	return conn_id;
}

u32 signaling_handler::init_sig_infos(const SceNpId* npid)
{
	std::lock_guard lock(data_mutex);

	const u32 conn_id = create_sig_infos(npid);

	if (sig1_peers[conn_id]->connStatus == SCE_NP_SIGNALING_CONN_STATUS_INACTIVE)
	{
		sign_log.trace("Creating new sig1 connection and requesting infos from RPCN");
		sig1_peers[conn_id]->connStatus = SCE_NP_SIGNALING_CONN_STATUS_PENDING;

		// Request peer infos from RPCN
		std::string npid_str(reinterpret_cast<const char*>(npid->handle.data));
		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		nph.req_sign_infos(npid_str, conn_id);
	}
	else
	{
		// Connection already exists(from passive connection)
		if (sig1_peers[conn_id]->connStatus == SCE_NP_SIGNALING_CONN_STATUS_ACTIVE && sig1_peers[conn_id]->ext_status == ext_sign_peer)
		{
			sign_log.trace("Activating already peer activated connection");
			sig1_peers[conn_id]->ext_status = ext_sign_mutual;
			start_sig_nl(conn_id, sig1_peers[conn_id]->addr, sig1_peers[conn_id]->port);
			signal_sig_callback(conn_id, SCE_NP_SIGNALING_EVENT_ESTABLISHED);
			signal_ext_sig_callback(conn_id, SCE_NP_SIGNALING_EVENT_EXT_MUTUAL_ACTIVATED);
		}
	}

	return conn_id;
}

signaling_info signaling_handler::get_sig_infos(u32 conn_id)
{
	return *sig1_peers[conn_id];
}

void signaling_handler::set_sig2_infos(u64 room_id, u16 member_id, s32 status, u32 addr, u16 port, bool self)
{
	std::lock_guard lock(data_mutex);
	if (!sig2_peers[room_id][member_id])
		sig2_peers[room_id][member_id] = std::make_shared<signaling_info>();

	auto& peer       = sig2_peers[room_id][member_id];
	peer->connStatus = status;
	peer->addr       = addr;
	peer->port       = port;
	peer->self       = self;
	peer->version    = 2;
	peer->room_id    = room_id;
	peer->member_id  = member_id;
}

signaling_info signaling_handler::get_sig2_infos(u64 room_id, u16 member_id)
{
	std::lock_guard lock(data_mutex);

	if (!sig2_peers[room_id][member_id])
	{
		sig2_peers[room_id][member_id] = std::make_shared<signaling_info>();
		auto& peer = sig2_peers[room_id][member_id];
		peer->room_id = room_id;
		peer->member_id = member_id;
		peer->version = 2;
	}

	return *sig2_peers[room_id][member_id];
}
