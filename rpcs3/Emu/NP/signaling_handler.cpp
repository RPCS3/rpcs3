#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "signaling_handler.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "np_handler.h"
#include "Emu/NP/vport0.h"
#include "Emu/NP/np_helpers.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

LOG_CHANNEL(sign_log, "Signaling");

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
	this->sig_cb = sig_cb;
	this->sig_cb_arg = sig_cb_arg;
}

void signaling_handler::set_ext_sig_cb(u32 sig_ext_cb_ctx, vm::ptr<SceNpSignalingHandler> sig_ext_cb, vm::ptr<void> sig_ext_cb_arg)
{
	std::lock_guard lock(data_mutex);
	this->sig_ext_cb_ctx = sig_ext_cb_ctx;
	this->sig_ext_cb = sig_ext_cb;
	this->sig_ext_cb_arg = sig_ext_cb_arg;
}

void signaling_handler::set_sig2_cb(u16 sig2_cb_ctx, vm::ptr<SceNpMatching2SignalingCallback> sig2_cb, vm::ptr<void> sig2_cb_arg)
{
	std::lock_guard lock(data_mutex);
	this->sig2_cb_ctx = sig2_cb_ctx;
	this->sig2_cb = sig2_cb;
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
		sign_log.notice("Called EXT sig CB: 0x%x (conn_id: %d)", event, conn_id);
	}
}

void signaling_handler::signal_sig2_callback(u64 room_id, u16 member_id, SceNpMatching2Event event) const
{
	if (room_id && sig2_cb)
	{
		sysutil_register_cb([sig2_cb = this->sig2_cb, sig2_cb_ctx = this->sig2_cb_ctx, room_id, member_id, event, sig2_cb_arg = this->sig2_cb_arg](ppu_thread& cb_ppu) -> s32
			{
				sig2_cb(cb_ppu, sig2_cb_ctx, room_id, member_id, event, 0, sig2_cb_arg);
				return 0;
			});
		sign_log.notice("Called sig2 CB: 0x%x (room_id: %d, member_id: %d)", event, room_id, member_id);
	}
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
			auto new_queue = qpackets.extract(it);
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

	if (sp->version != SIGNALING_VERSION)
	{
		sign_log.error("Invalid version in signaling packet: %d, expected: %d", sp->version, SIGNALING_VERSION);

		if (sp->version > SIGNALING_VERSION)
			sign_log.error("You are most likely using an outdated version of RPCS3");
		else
			sign_log.error("The other user is most likely using an outdated version of RPCS3");

		return false;
	}

	if (!np::is_valid_npid(sp->npid))
	{
		sign_log.error("Invalid npid in signaling packet");
		return false;
	}

	return true;
}

u64 signaling_handler::get_micro_timestamp(const std::chrono::steady_clock::time_point& time_point)
{
	return std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch()).count();
}

void signaling_handler::process_incoming_messages()
{
	auto msgs = get_sign_msgs();

	for (const auto& msg : msgs)
	{
		if (msg.data.size() != sizeof(signaling_packet))
		{
			sign_log.error("Received an invalid signaling packet");
			continue;
		}

		auto op_addr = msg.src_addr;
		auto op_port = msg.src_port;
		const auto* sp = reinterpret_cast<const signaling_packet*>(msg.data.data());

		if (!validate_signaling_packet(sp))
			continue;

		if (sign_log.trace)
		{
			in_addr addr{};
			addr.s_addr = op_addr;
			char ip_str[16];
			inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));
			std::string_view npid(sp->npid.handle.data);

			sign_log.trace("SP %s from %s:%d(npid: %s)", sp->command, ip_str, op_port, npid);
		}

		bool reply = false, schedule_repeat = false;
		auto& sent_packet = sig_packet;

		// Get signaling info for user to know if we should even bother looking further
		auto si = get_signaling_ptr(sp);

		if (!si && sp->command == signal_connect)
		{
			// Connection can be remotely established and not mutual
			const u32 conn_id = get_always_conn_id(sp->npid);
			si = ::at32(sig_peers, conn_id);
		}

		if (!si && sp->command != signal_finished)
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
			sent_packet.timestamp_sender = get_micro_timestamp(now);
			send_signaling_packet(sent_packet, si->addr, si->port);
			queue_signaling_packet(sent_packet, si, now + REPEAT_PING_DELAY);
		};

		const auto update_rtt = [&](u64 rtt_timestamp)
		{
			u64 timestamp_now = get_micro_timestamp(now);
			u64 rtt = timestamp_now - rtt_timestamp;
			si->last_rtts[(si->rtt_counters % 6)] = rtt;
			si->rtt_counters++;

			std::size_t num_rtts = std::min(static_cast<std::size_t>(6), si->rtt_counters);
			u64 sum = 0;
			for (std::size_t index = 0; index < num_rtts; index++)
			{
				sum += si->last_rtts[index];
			}

			si->rtt = (sum / num_rtts);
		};

		switch (sp->command)
		{
		case signal_ping:
			reply = true;
			schedule_repeat = false;
			sent_packet.command = signal_pong;
			sent_packet.timestamp_sender = sp->timestamp_sender;
			break;
		case signal_pong:
			update_rtt(sp->timestamp_sender);
			reply = false;
			schedule_repeat = false;
			reschedule_packet(si, signal_ping, now + 10s);
			break;
		case signal_connect:
			reply = true;
			schedule_repeat = true;
			sent_packet.command = signal_connect_ack;
			sent_packet.timestamp_sender = sp->timestamp_sender;
			sent_packet.timestamp_receiver = get_micro_timestamp(now);
			update_si_addr(si, op_addr, op_port);
			break;
		case signal_connect_ack:
			update_rtt(sp->timestamp_sender);
			reply = true;
			schedule_repeat = false;
			setup_ping();
			sent_packet.command = signal_confirm;
			sent_packet.timestamp_receiver = sp->timestamp_receiver;
			retire_packet(si, signal_connect);
			update_si_addr(si, op_addr, op_port);
			update_si_mapped_addr(si, sp->sent_addr, sp->sent_port);
			update_si_status(si, SCE_NP_SIGNALING_CONN_STATUS_ACTIVE);
			break;
		case signal_confirm:
			update_rtt(sp->timestamp_receiver);
			reply = false;
			schedule_repeat = false;
			setup_ping();
			retire_packet(si, signal_connect_ack);
			update_si_addr(si, op_addr, op_port);
			update_si_mapped_addr(si, sp->sent_addr, sp->sent_port);
			update_ext_si_status(si, true);
			break;
		case signal_finished:
			reply = true;
			schedule_repeat = false;
			sent_packet.command = signal_finished_ack;
			update_ext_si_status(si, false);
			break;
		case signal_finished_ack:
			reply = false;
			schedule_repeat = false;
			update_si_status(si, SCE_NP_SIGNALING_CONN_STATUS_INACTIVE);
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
			auto& [timestamp, sig] = *it;

			if (timestamp > now)
				break;

			if (sig.sig_info->time_last_msg_recvd < now - 60s)
			{
				// We had no connection to opponent for 60 seconds, consider the connection dead
				sign_log.notice("Timeout disconnection");
				update_si_status(sig.sig_info, SCE_NP_SIGNALING_CONN_STATUS_INACTIVE);
				break; // qpackets will be emptied of all packets for this user so we're requeuing
			}

			// Update the timestamp if necessary
			switch (sig.packet.command)
			{
			case signal_connect:
			case signal_ping:
				sig.packet.timestamp_sender = get_micro_timestamp(now);
				break;
			case signal_connect_ack:
				sig.packet.timestamp_receiver = get_micro_timestamp(now);
				break;
			default:
				break;
			}

			// Resend the packet
			send_signaling_packet(sig.packet, sig.sig_info->addr, sig.sig_info->port);

			// Reschedule another packet
			SignalingCommand cmd = sig.packet.command;
			auto& si = sig.sig_info;

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
		if (sign_log.trace)
		{
			in_addr addr_old, addr_new;
			addr_old.s_addr = si->addr;
			addr_new.s_addr = new_addr;

			char ip_str_old[16];
			char ip_str_new[16];
			inet_ntop(AF_INET, &addr_old, ip_str_old, sizeof(ip_str_old));
			inet_ntop(AF_INET, &addr_new, ip_str_new, sizeof(ip_str_new));

			sign_log.trace("Updated Address from %s:%d to %s:%d", ip_str_old, si->port, ip_str_new, new_port);
		}

		si->addr = new_addr;
		si->port = new_port;
	}
}

void signaling_handler::update_si_mapped_addr(std::shared_ptr<signaling_info>& si, u32 new_addr, u16 new_port)
{
	ensure(si);

	if (si->mapped_addr != new_addr || si->mapped_port != new_port)
	{
		if (sign_log.trace)
		{
			in_addr addr_old, addr_new;
			addr_old.s_addr = si->mapped_addr;
			addr_new.s_addr = new_addr;

			char ip_str_old[16];
			char ip_str_new[16];
			inet_ntop(AF_INET, &addr_old, ip_str_old, sizeof(ip_str_old));
			inet_ntop(AF_INET, &addr_new, ip_str_new, sizeof(ip_str_new));

			sign_log.trace("Updated Mapped Address from %s:%d to %s:%d", ip_str_old, si->mapped_port, ip_str_new, new_port);
		}

		si->mapped_addr = new_addr;
		si->mapped_port = new_port;
	}
}

void signaling_handler::update_si_status(std::shared_ptr<signaling_info>& si, s32 new_status)
{
	if (!si)
		return;

	if (si->conn_status == SCE_NP_SIGNALING_CONN_STATUS_PENDING && new_status == SCE_NP_SIGNALING_CONN_STATUS_ACTIVE)
	{
		si->conn_status = SCE_NP_SIGNALING_CONN_STATUS_ACTIVE;

		signal_sig_callback(si->conn_id, SCE_NP_SIGNALING_EVENT_ESTABLISHED);
		signal_sig2_callback(si->room_id, si->member_id, SCE_NP_MATCHING2_SIGNALING_EVENT_Established);

		if (si->op_activated)
			signal_ext_sig_callback(si->conn_id, SCE_NP_SIGNALING_EVENT_EXT_MUTUAL_ACTIVATED);
	}
	else if ((si->conn_status == SCE_NP_SIGNALING_CONN_STATUS_PENDING || si->conn_status == SCE_NP_SIGNALING_CONN_STATUS_ACTIVE) && new_status == SCE_NP_SIGNALING_CONN_STATUS_INACTIVE)
	{
		si->conn_status = SCE_NP_SIGNALING_CONN_STATUS_INACTIVE;
		signal_sig_callback(si->conn_id, SCE_NP_SIGNALING_EVENT_DEAD);
		signal_sig2_callback(si->room_id, si->member_id, SCE_NP_MATCHING2_SIGNALING_EVENT_Dead);
		retire_all_packets(si);
	}
}

void signaling_handler::update_ext_si_status(std::shared_ptr<signaling_info>& si, bool op_activated)
{
	if (op_activated && !si->op_activated)
	{
		si->op_activated = true;

		if (si->conn_status != SCE_NP_SIGNALING_CONN_STATUS_ACTIVE)
			signal_ext_sig_callback(si->conn_id, SCE_NP_SIGNALING_EVENT_EXT_PEER_ACTIVATED);
		else
			signal_ext_sig_callback(si->conn_id, SCE_NP_SIGNALING_EVENT_EXT_MUTUAL_ACTIVATED);
	}
	else if (!op_activated && si->op_activated)
	{
		si->op_activated = false;

		signal_ext_sig_callback(si->conn_id, SCE_NP_SIGNALING_EVENT_EXT_PEER_DEACTIVATED);
	}
}

void signaling_handler::set_self_sig_info(SceNpId& npid)
{
	std::lock_guard lock(data_mutex);
	sig_packet.npid = npid;
}

void signaling_handler::send_signaling_packet(signaling_packet& sp, u32 addr, u16 port) const
{
	std::vector<u8> packet(sizeof(signaling_packet) + VPORT_0_HEADER_SIZE);
	reinterpret_cast<le_t<u16>&>(packet[0]) = 0; // VPort 0
	packet[2] = SUBSET_SIGNALING;
	sp.sent_addr = addr;
	sp.sent_port = port;
	memcpy(packet.data() + VPORT_0_HEADER_SIZE, &sp, sizeof(signaling_packet));

	sockaddr_in dest;
	memset(&dest, 0, sizeof(sockaddr_in));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = addr;
	dest.sin_port = std::bit_cast<u16, be_t<u16>>(port);

	char ip_str[16];
	inet_ntop(AF_INET, &dest.sin_addr, ip_str, sizeof(ip_str));

	sign_log.trace("Sending %s packet to %s:%d", sp.command, ip_str, port);

	if (send_packet_from_p2p_port(packet, dest) == -1)
	{
		sign_log.error("Failed to send signaling packet to %s:%d", ip_str, port);
	}
}

void signaling_handler::queue_signaling_packet(signaling_packet& sp, std::shared_ptr<signaling_info> si, steady_clock::time_point wakeup_time)
{
	queued_packet qp;
	qp.sig_info = std::move(si);
	qp.packet = sp;
	qpackets.emplace(wakeup_time, std::move(qp));
}

std::shared_ptr<signaling_info> signaling_handler::get_signaling_ptr(const signaling_packet* sp)
{
	u32 conn_id;

	char npid_buf[17]{};
	memcpy(npid_buf, sp->npid.handle.data, 16);
	std::string npid(npid_buf);

	if (!npid_to_conn_id.contains(npid))
		return nullptr;

	conn_id = ::at32(npid_to_conn_id, npid);

	if (!sig_peers.contains(conn_id))
	{
		sign_log.error("Discrepancy in signaling data");
		return nullptr;
	}

	return ::at32(sig_peers, conn_id);
}

void signaling_handler::start_sig(u32 conn_id, u32 addr, u16 port)
{
	std::lock_guard lock(data_mutex);
	auto& sent_packet = sig_packet;
	sent_packet.command = signal_connect;
	sent_packet.timestamp_sender = get_micro_timestamp(steady_clock::now());

	ensure(sig_peers.contains(conn_id));
	std::shared_ptr<signaling_info> si = ::at32(sig_peers, conn_id);

	si->addr = addr;
	si->port = port;

	send_signaling_packet(sent_packet, si->addr, si->port);
	queue_signaling_packet(sent_packet, si, steady_clock::now() + REPEAT_CONNECT_DELAY);
	wake_up();
}

void signaling_handler::stop_sig_nl(u32 conn_id)
{
	if (!sig_peers.contains(conn_id))
		return;

	std::shared_ptr<signaling_info> si = ::at32(sig_peers, conn_id);

	// Do not queue packets for an already dead connection
	if (si->conn_status == SCE_NP_SIGNALING_CONN_STATUS_INACTIVE)
		return;

	auto& sent_packet = sig_packet;
	sent_packet.command = signal_finished;

	send_signaling_packet(sent_packet, si->addr, si->port);
	queue_signaling_packet(sent_packet, std::move(si), steady_clock::now() + REPEAT_FINISHED_DELAY);
}

void signaling_handler::stop_sig(u32 conn_id)
{
	std::lock_guard lock(data_mutex);

	stop_sig_nl(conn_id);
}

void signaling_handler::disconnect_sig2_users(u64 room_id)
{
	std::lock_guard lock(data_mutex);

	for (auto& [conn_id, si] : sig_peers)
	{
		if (si->room_id == room_id)
		{
			stop_sig_nl(conn_id);
		}
	}
}

u32 signaling_handler::get_always_conn_id(const SceNpId& npid)
{
	std::string npid_str(reinterpret_cast<const char*>(npid.handle.data));
	if (npid_to_conn_id.contains(npid_str))
		return ::at32(npid_to_conn_id, npid_str);

	const u32 conn_id = cur_conn_id++;
	npid_to_conn_id.emplace(std::move(npid_str), conn_id);
	sig_peers.emplace(conn_id, std::make_shared<signaling_info>());
	auto& si = ::at32(sig_peers, conn_id);
	si->conn_id = conn_id;
	si->npid = npid;

	return conn_id;
}

u32 signaling_handler::init_sig1(const SceNpId& npid)
{
	std::lock_guard lock(data_mutex);

	const u32 conn_id = get_always_conn_id(npid);

	if (sig_peers[conn_id]->conn_status == SCE_NP_SIGNALING_CONN_STATUS_INACTIVE)
	{
		sign_log.trace("Creating new sig1 connection and requesting infos from RPCN");
		sig_peers[conn_id]->conn_status = SCE_NP_SIGNALING_CONN_STATUS_PENDING;

		// Request peer infos from RPCN
		std::string npid_str(reinterpret_cast<const char*>(npid.handle.data));
		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		nph.req_sign_infos(npid_str, conn_id);
	}

	return conn_id;
}

u32 signaling_handler::init_sig2(const SceNpId& npid, u64 room_id, u16 member_id)
{
	std::lock_guard lock(data_mutex);
	u32 conn_id = get_always_conn_id(npid);
	auto& si = ::at32(sig_peers, conn_id);
	si->room_id = room_id;
	si->member_id = member_id;
	si->conn_status = SCE_NP_SIGNALING_CONN_STATUS_PENDING;

	return conn_id;
}

std::optional<u32> signaling_handler::get_conn_id_from_npid(const SceNpId& npid)
{
	std::lock_guard lock(data_mutex);

	std::string npid_str(reinterpret_cast<const char*>(npid.handle.data));
	if (npid_to_conn_id.contains(npid_str))
		return ::at32(npid_to_conn_id, npid_str);

	return std::nullopt;
}

std::optional<signaling_info> signaling_handler::get_sig_infos(u32 conn_id)
{
	std::lock_guard lock(data_mutex);
	if (sig_peers.contains(conn_id))
		return *::at32(sig_peers, conn_id);

	return std::nullopt;
}

std::optional<u32> signaling_handler::get_conn_id_from_addr(u32 addr, u16 port)
{
	std::lock_guard lock(data_mutex);

	for (const auto& [conn_id, conn_info] : sig_peers)
	{
		if (conn_info && std::bit_cast<u32, be_t<u32>>(conn_info->addr) == addr && conn_info->port == port)
		{
			return conn_id;
		}
	}

	return std::nullopt;
}
