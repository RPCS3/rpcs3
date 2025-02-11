#pragma once
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/Cell/Modules/sceNp2.h"
#include "Utilities/Thread.h"
#include "Utilities/mutex.h"
#include <unordered_map>
#include <chrono>
#include <optional>

struct signaling_info
{
	s32 conn_status = SCE_NP_SIGNALING_CONN_STATUS_INACTIVE;
	u32 addr = 0;
	u16 port = 0;

	// User seen from that peer
	u32 mapped_addr = 0;
	u16 mapped_port = 0;

	// For handler
	steady_clock::time_point time_last_msg_recvd = steady_clock::now();
	bool self = false;
	SceNpId npid{};

	// Signaling
	u32 conn_id = 0;
	bool op_activated = false;
	u32 info_counter = 10;

	// Matching2
	u64 room_id = 0;
	u16 member_id = 0;

	// Stats
	u64 last_rtts[6] = {};
	usz rtt_counters = 0;
	u32 rtt = 0;
	u32 pings_sent = 1, lost_pings = 0;
	u32 packet_loss = 0;
};

enum SignalingCommand : u32
{
	signal_ping,
	signal_pong,
	signal_connect,
	signal_connect_ack,
	signal_confirm,
	signal_finished,
	signal_finished_ack,
	signal_info,
};

class signaling_handler
{
public:
	signaling_handler();
	void operator()();
	void wake_up();
	signaling_handler& operator=(thread_state);

	void set_self_sig_info(SceNpId& npid);

	u32 init_sig1(const SceNpId& npid);
	u32 init_sig2(const SceNpId& npid, u64 room_id, u16 member_id);
	std::optional<signaling_info> get_sig_infos(u32 conn_id);
	std::optional<u32> get_conn_id_from_npid(const SceNpId& npid);
	std::optional<u32> get_conn_id_from_addr(u32 addr, u16 port);

	void add_sig_ctx(u32 ctx_id);
	void remove_sig_ctx(u32 ctx_id);
	void clear_sig_ctx();
	void add_match2_ctx(u16 ctx_id);
	void remove_match2_ctx(u16 ctx_id);
	void clear_match2_ctx();

	void start_sig(u32 conn_id, u32 addr, u16 port);
	void stop_sig(u32 conn_id, bool forceful);
	void send_information_packets(u32 addr, u16 port, const SceNpId& npid);

	void disconnect_sig2_users(u64 room_id);

	static constexpr auto thread_name = "Signaling Manager Thread"sv;

private:
	static constexpr auto REPEAT_CONNECT_DELAY = std::chrono::milliseconds(200);
	static constexpr auto REPEAT_PING_DELAY = std::chrono::milliseconds(500);
	static constexpr auto REPEAT_FINISHED_DELAY = std::chrono::milliseconds(500);
	static constexpr auto REPEAT_INFO_DELAY = std::chrono::milliseconds(200);
	static constexpr be_t<u32> SIGNALING_SIGNATURE = (static_cast<u32>('S') << 24 | static_cast<u32>('I') << 16 | static_cast<u32>('G') << 8 | static_cast<u32>('N'));
	static constexpr le_t<u32> SIGNALING_VERSION = 3;

	struct signaling_packet
	{
		be_t<u32> signature = SIGNALING_SIGNATURE;
		le_t<u32> version = SIGNALING_VERSION;
		le_t<u64> timestamp_sender;
		le_t<u64> timestamp_receiver;
		le_t<SignalingCommand> command;
		le_t<u32> sent_addr;
		le_t<u16> sent_port;
		SceNpId npid;
	};

	struct queued_packet
	{
		signaling_packet packet{};
		std::shared_ptr<signaling_info> sig_info;
	};

	std::set<u32> sig_ctx_lst;
	std::set<u16> match2_ctx_lst;

	static u64 get_micro_timestamp(const std::chrono::steady_clock::time_point& time_point);

	u32 get_always_conn_id(const SceNpId& npid);
	static void update_si_addr(std::shared_ptr<signaling_info>& si, u32 new_addr, u16 new_port);
	static void update_si_mapped_addr(std::shared_ptr<signaling_info>& si, u32 new_addr, u16 new_port);
	void update_si_status(std::shared_ptr<signaling_info>& si, s32 new_status, s32 error_code);
	void update_ext_si_status(std::shared_ptr<signaling_info>& si, bool op_activated);
	void signal_sig_callback(u32 conn_id, s32 event, s32 error_code);
	void signal_ext_sig_callback(u32 conn_id, s32 event, s32 error_code) const;
	void signal_sig2_callback(u64 room_id, u16 member_id, SceNpMatching2Event event, s32 error_code) const;

	static bool validate_signaling_packet(const signaling_packet* sp);
	void reschedule_packet(std::shared_ptr<signaling_info>& si, SignalingCommand cmd, steady_clock::time_point new_timepoint);
	void retire_packet(std::shared_ptr<signaling_info>& si, SignalingCommand cmd);
	void retire_all_packets(std::shared_ptr<signaling_info>& si);
	void stop_sig_nl(u32 conn_id, bool forceful);

	shared_mutex data_mutex;
	atomic_t<u32> wakey = 0;

	signaling_packet sig_packet{};

	std::map<steady_clock::time_point, queued_packet> qpackets; // (wakeup time, packet)

	u32 cur_conn_id = 1;
	std::unordered_map<std::string, u32> npid_to_conn_id;               // (npid, conn_id)
	std::unordered_map<u32, std::shared_ptr<signaling_info>> sig_peers; // (conn_id, sig_info)

	void process_incoming_messages();
	std::shared_ptr<signaling_info> get_signaling_ptr(const signaling_packet* sp);
	void send_signaling_packet(signaling_packet& sp, u32 addr, u16 port) const;
	void queue_signaling_packet(signaling_packet& sp, std::shared_ptr<signaling_info> si, steady_clock::time_point wakeup_time);
};
