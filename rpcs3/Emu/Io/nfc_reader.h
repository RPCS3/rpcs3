#pragma once

#include "util/types.hpp"
#include "util/endian.hpp"
#include "mifare_types.h"
#include "pcsc_driver.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// NFC controller host protocol
class nfc_reader
{
public:
	nfc_reader();
	~nfc_reader();

	// Sets the reader mode / PC/SC device name ("Fake" or empty for internal emulation, or PC/SC device name)
	void set_device_name(const std::string& name);
	const std::string& get_device_name() const;

	// Get the buffer size required by write_buffer
	usz get_buffer_size() const;

	// Writes ACK and response to an external buffer
	void write_buffer(std::vector<u8>& out);

	// Processes a request received from the host (accepts arbitrary card size, e.g. 0x40 sector 0 or 0x400 full 1K)
	void process_request(const std::vector<u8>& in, std::span<u8> card);

	static constexpr usz mifare_1k_size   = 1024;
	static constexpr usz mifare_1k_blocks = 64;
	static constexpr usz mifare_4k_blocks = 256;

private:
	// PC/SC Reader integration
	std::string m_device_name = "Fake";
	std::unique_ptr<pcsc_driver> m_pcsc;

	// Mifare Classic authentication key type
	enum class auth_key : u8
	{
		none  = 0,
		key_a = 1,
		key_b = 2,
	};

	// Variables for retaining internal status
	bool m_remove_pre_and_postamble = false;
	bool m_send_ack = false;
	bool m_send_response = false;
	std::vector<u8> m_response = {};

	// Mifare Classic internal authentication state
	int m_authenticated_sector = -1;
	auth_key m_authenticated_key = auth_key::none;
	std::array<u8, mifare_block_size> m_transfer_buffer = {};
	bool m_transfer_buffer_valid = false;

	// Internal register storage & GPIO / Target mode state
	void init_registers();
	u8 read_register_value(u16 address) const;
	void write_register_value(u16 address, u8 value);

	std::unordered_map<u16, u8> m_registers = {};
	u8 m_gpio_p3 = 0x3F;
	u8 m_gpio_p7 = 0x06;
	u8 m_gpio_i0i1 = 0x00;
	u8 m_target_state = 0x00;
	u8 m_parameters_flags = 0x34; // UM0701-02 Table 15 default: fAutomaticATR_RES(bit 2) | fAutomaticRATS(bit 4) | fISO14443-4_PICC(bit 5)
	u8 m_last_error = 0x00;
	u8 m_sam_mode = 0x01;
	u8 m_atr_res_timeout = 0x0B;
	u8 m_retry_timeout = 0x0A;
	u8 m_max_rty_com = 0x00;
	u8 m_mx_rty_atr = 0xFF;
	u8 m_mx_rty_psl = 0x01;
	u8 m_mx_rty_passive_activation = 0xFF;
	u8 m_rf_on_wait_time = 0x00;        // RFConfiguration CfgItem 0x81 (Sony RC-S620/S extension)
	u8 m_dep_atr_res_to = 0x0E;         // RFConfiguration CfgItem 0x82: gbyAtrResTo (Sony RC-S620/S extension)
	u8 m_dep_rtox = 0x07;               // RFConfiguration CfgItem 0x82: gbyRtox (Sony RC-S620/S extension)
	u8 m_dep_target_sto_to = 0x0E;      // RFConfiguration CfgItem 0x82: gbyTargetStoTo (Sony RC-S620/S extension)

	// Target activation state in Initiator mode
	enum class initiator_target_state : u8
	{
		released = 0,
		selected = 1,
		deselected = 2,
	};
	initiator_target_state m_initiator_state = initiator_target_state::released;

	// Frames are wrapped in a preamble and a postamble byte, which are optional when receiving
	static constexpr u8 preamble = 0x00;
	static constexpr u8 postamble = 0x00;

	// Start code of every frame
	static constexpr std::array<u8, 2> start_code = {0x00, 0xFF};

	// ACK frame (without preamble and postamble)
	static constexpr std::array<u8, 4> ack_frame = {0x00, 0xFF, 0x00, 0xFF};

	// NACK frame (without preamble and postamble)
	static constexpr std::array<u8, 4> nack_frame = {0x00, 0xFF, 0xFF, 0x00};

	// Error frame (application level error, e.g. syntax error) (without preamble and postamble)
	static constexpr std::array<u8, 6> error_frame = {0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x81};

	// Frame identifier (direction of an information frame)
	enum class frame_identifier : u8
	{
		to_device = 0xD4,
		to_host = 0xD5,
	};

	// Command codes (the response code is always the command code + 1)
	enum class command_code : u8
	{
		// Miscellaneous commands (UM0701-02 Section 7.2)
		diagnose                 = 0x00,
		get_firmware_version     = 0x02,
		get_general_status       = 0x04,
		read_register            = 0x06,
		write_register           = 0x08,
		read_gpio                = 0x0C,
		write_gpio               = 0x0E,
		set_serial_baud_rate     = 0x10,
		set_parameters           = 0x12,
		sam_configuration        = 0x14,
		power_down               = 0x16,

		// RF configuration (UM0701-02 Section 7.3.1)
		rf_configuration         = 0x32,

		// Initiator commands (UM0701-02 Section 7.3.3 - 7.3.13)
		in_data_exchange         = 0x40,
		in_communicate_thru      = 0x42,
		in_deselect              = 0x44,
		in_jump_for_psl          = 0x46,
		in_list_passive_target   = 0x4A,
		in_psl                   = 0x4E,
		in_atr                   = 0x50,
		in_release               = 0x52,
		in_select                = 0x54,
		in_jump_for_dep          = 0x56,
		rf_regulation_test       = 0x58,
		in_auto_poll             = 0x60,

		// Target commands (UM0701-02 Section 7.3.14 - 7.3.21)
		tg_get_data              = 0x86,
		tg_get_initiator_command = 0x88,
		tg_get_target_status     = 0x8A,
		tg_init_as_target        = 0x8C,
		tg_set_data              = 0x8E,
		tg_response_to_initiator = 0x90,
		tg_set_general_bytes     = 0x92,
		tg_set_meta_data         = 0x94,

		// Sony RC-S620/S extension commands (RC-S620/S User's Manual Section 8.2.5 - 8.2.11)
		reset                    = 0x18,
		communicate_thru_ex      = 0xA0,
	};

	// Status byte of InDataExchange, InDeselect, InRelease, InSelect and PowerDown responses
	enum class status : u8
	{
		success           = 0x00,
		timeout           = 0x01,
		crc_error         = 0x02,
		parity_error      = 0x03,
		mifare_auth_error = 0x14,
		context_error     = 0x27,
	};

	static constexpr u8 response_code(command_code command)
	{
		return static_cast<u8>(static_cast<u8>(command) + 1);
	}

	// Normal information frame header
	struct frame_header
	{
		//u8 preamble = nfc_reader::preamble; // Removed for separate handling of preamble and postamble
		std::array<u8, 2> start = start_code;
		u8 len = 0; // Length of TFI + data
		u8 lcs = 0; // Length checksum (len + lcs == 0)
		frame_identifier tfi = frame_identifier::to_host;
	};

	// Extended information frame header (len and lcs are 0xFF in the normal header position)
	struct extended_frame_header
	{
		//u8 preamble = nfc_reader::preamble; // Removed for separate handling of preamble and postamble
		std::array<u8, 2> start = start_code;
		std::array<u8, 2> marker = {0xFF, 0xFF};
		be_t<u16, 1> len = 0; // Length of TFI + data
		u8 lcs = 0;           // Length checksum (len_m + len_l + lcs == 0)
		frame_identifier tfi = frame_identifier::to_host;
	};

	// Frame footer
	struct frame_footer
	{
		u8 dcs = 0; // Data checksum (tfi + data + dcs == 0)
		//u8 postamble = nfc_reader::postamble; // Removed for separate handling of preamble and postamble
	};

	// InDataExchange request carrying a MIFARE command
	struct in_data_exchange_request
	{
		command_code cmd = command_code::in_data_exchange;
		u8 tg = 0;
		mifare_cmd mifare = mifare_cmd::read_16_bytes;
		u8 block = 0;
	};

	// InDataExchange request carrying a MIFARE authentication command (14 bytes)
	struct mifare_auth_request
	{
		command_code cmd = command_code::in_data_exchange;
		u8 tg = 0;
		mifare_cmd mifare = mifare_cmd::auth_key_a;
		u8 block = 0;
		std::array<u8, 6> key{};
		std::array<u8, mifare_uid_size> uid{};
	};

	// Card types and contactless protocols supported or referenced by NFC reader commands
	enum class card_type : u8
	{
		unknown           = 0,
		mifare_classic    = 1, // MIFARE Classic 1K / 4K (ISO/IEC 14443-3 Type A)
		mifare_ultralight = 2, // MIFARE Ultralight / Ultralight C (ISO/IEC 14443-3 Type A)
		felica            = 3, // FeliCa (JIS X 6319-4 / ISO/IEC 18092 212/424 kbps)
		iso14443_4        = 4, // ISO/IEC 14443-4 Type A/B (T=CL, e.g. MIFARE DESFire)
		iso14443_3b       = 5, // ISO/IEC 14443-3 Type B
		jewel             = 6, // Innovision Jewel / Topaz
		nfcip_dep         = 7, // ISO/IEC 18092 NFCIP-1 Data Exchange Protocol (DEP)
	};

	static card_type get_card_type(std::span<const u8> card);

	static inline bool is_4k_card(std::span<const u8> card)
	{
		return card.size() > mifare_1k_size;
	}

	// Mifare block geometric information (supporting asymmetric 1K / 4K layout)
	struct mifare_block_info
	{
		u8 sector = 0;
		u8 block_rel = 0;
		u8 trailer_block = 0;
		bool is_trailer = false;
		u8 cond_index = 0;
	};

	static mifare_block_info get_mifare_block_info(u8 block);

	// Mifare Classic block access conditions
	struct access_bits
	{
		u8 c1 = 0;
		u8 c2 = 0;
		u8 c3 = 0;
		bool valid = false;
	};

	// Helpers for Mifare Classic 1K / 4K card emulation
	std::array<u8, mifare_uid_size> get_card_uid(std::span<const u8> card) const;
	std::vector<u8> get_card_uid_bytes(std::span<const u8> card) const;
	std::array<u8, mifare_block_size> get_block_data(std::span<const u8> card, u8 block) const;
	static access_bits get_access_bits(const std::array<u8, mifare_block_size>& trailer, u8 cond_index);
	static std::array<u8, mifare_block_size> format_value_block(s32 value, u8 addr);
	static bool get_value_from_block(std::span<const u8> block, s32& out_value, u8& out_addr);

	struct mifare_check_result
	{
		mifare_block_info info{};
		access_bits bits{};
		u8 c = 0;
		usz offset = 0;
		status error = status::success;

		bool ok() const { return error == status::success; }
	};

	void reset_card_session();
	void reset_target_state();
	mifare_check_result check_mifare_block_access(std::span<const u8> card, u8 block, bool allow_trailer, bool allow_block_0 = false) const;

	// ReadGPIO response
	struct read_gpio_response
	{
		u8 cmd = response_code(command_code::read_gpio);
		u8 p3 = 0x00;
		u8 p7 = 0x06;
		u8 i0i1 = 0x00;
	};

	// InDataExchange response (MIFARE READ of one 16-byte block)
	struct in_data_exchange_response
	{
		u8 cmd = response_code(command_code::in_data_exchange);
		u8 status = 0x00;
		std::array<u8, mifare_block_size> block{};
	};

	// InListPassiveTarget response (one ISO/IEC 14443 Type A target)
	struct in_list_passive_target_response
	{
		u8 cmd = response_code(command_code::in_list_passive_target);
		u8 nb_tg = 0x01;
		u8 tg = 0x01;
		std::array<u8, 2> sens_res = {0x04, 0x00}; // ATQA: Mifare Classic 1K on-the-wire order (UM0701-02 §7.3.5 Page 120)
		u8 sel_res = 0x08;              // SAK: MIFARE Classic 1K
		u8 nfcid_length = mifare_uid_size;
		std::array<u8, mifare_uid_size> nfcid{};
	};

	// GetFirmwareVersion response
	struct get_firmware_version_response
	{
		u8 cmd = response_code(command_code::get_firmware_version);
		u8 ic = 0x32;
		u8 ver = 0x01;
		u8 rev = 0x06;
		u8 support = 0x07;
	};

	// TgGetTargetStatus response
	struct tg_get_target_status_response
	{
		u8 cmd = response_code(command_code::tg_get_target_status);
		u8 state = 0x00;
		u8 brit = 0x00;
	};

	// Serialize a struct into bytes
	template <typename T> requires std::is_trivially_copyable_v<T>
	static inline std::vector<u8> to_bytes(const T& value)
	{
		const auto bytes = std::bit_cast<std::array<u8, sizeof(T)>>(value);
		return {bytes.begin(), bytes.end()};
	}

	// Skip the preamble (any bytes before the start code) and return the frame starting at the start code
	// Returns an empty span if there is no start code
	static inline std::span<const u8> find_frame(const std::vector<u8>& in)
	{
		const auto it = std::search(in.begin(), in.end(), start_code.begin(), start_code.end());
		return {it, in.end()};
	}

	// Requires at least sizeof(frame_header) bytes
	static inline bool is_extended_frame(std::span<const u8> frame)
	{
		const auto header = read_from_ptr<frame_header>(frame);
		return header.len == 0xFF && header.lcs == 0xFF;
	}

	// Writes an ACK frame (with preamble and postamble, unless removed with m_remove_pre_and_postamble = true) to the out buffer
	inline void write_ack(std::vector<u8>& out)
	{
		if (!m_remove_pre_and_postamble)
			out.push_back(preamble);

		out.insert(out.end(), ack_frame.begin(), ack_frame.end());

		if (!m_remove_pre_and_postamble)
			out.push_back(postamble);
	}

	// Expects a frame starting at the start code (see find_frame), the postamble is optional
	static bool verify_request(std::span<const u8> in);

	// Builds a valid response frame with the given data from the in buffer; if no data is given, builds an error frame
	void build_response(const std::vector<u8>& in = {});

	// Asynchronous worker for time-consuming PC/SC operations (avoids blocking USIO thread)
	void start_worker();
	void stop_worker();
	void worker_loop();
	void dispatch_async_command(std::span<const u8> frame, std::span<u8> card);
	void execute_command(std::span<const u8> frame, std::span<u8> card);

	struct worker_task
	{
		u64 id = 0;
		std::vector<u8> frame;
		std::span<u8> card;
	};

	mutable std::mutex m_exec_mutex;
	mutable std::mutex m_buffer_mutex;
	std::mutex m_task_mutex;
	std::condition_variable m_task_cv;
	std::thread m_worker_thread;
	std::atomic<bool> m_worker_running{false};
	std::atomic<bool> m_worker_stop{false};
	u64 m_task_seq = 0;
	std::deque<worker_task> m_task_queue;
	std::vector<u8> m_exec_response;

	static_assert(sizeof(frame_header) == 5);
	static_assert(sizeof(extended_frame_header) == 8);
	static_assert(sizeof(frame_footer) == 1);
	static_assert(sizeof(in_data_exchange_request) == 4);
	static_assert(sizeof(mifare_auth_request) == 14);
	static_assert(sizeof(read_gpio_response) == 4);
	static_assert(sizeof(in_data_exchange_response) == 18);
	static_assert(sizeof(in_list_passive_target_response) == 11);
	static_assert(sizeof(get_firmware_version_response) == 5);
	static_assert(sizeof(tg_get_target_status_response) == 3);
};
