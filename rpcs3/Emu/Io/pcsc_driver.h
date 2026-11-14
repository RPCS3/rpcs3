#pragma once

#include "util/types.hpp"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Information about a detected card in a PC/SC reader
struct pcsc_card_info
{
	bool present = false;
	std::vector<u8> atr;
	std::vector<u8> uid;
	std::vector<u8> pmm;
	u16 system_code = 0xFFFF;
	std::vector<u8> historical_bytes;
	bool is_felica = false;
	bool is_mifare = false;
	bool is_iso14443_4 = false;
	u16 atqa = 0x0000;
	u8 sak = 0x00;
	bool sak_valid = false;
};

// Cross-platform PC/SC smartcard / NFC reader driver
class pcsc_driver
{
public:
	pcsc_driver();
	~pcsc_driver();

	pcsc_driver(const pcsc_driver&) = delete;
	pcsc_driver& operator=(const pcsc_driver&) = delete;
	pcsc_driver(pcsc_driver&&) noexcept;
	pcsc_driver& operator=(pcsc_driver&&) noexcept;

	// Enumerates all available PC/SC smartcard / NFC readers on the system
	static std::vector<std::string> list_readers();

	// Opens connection to a specific reader by name
	bool open(std::string_view reader_name);

	// Closes the reader and releases handles
	void close();

	// Returns true if open on a valid reader
	bool is_open() const;

	// Returns the current reader name
	std::string get_reader_name() const;

	// Polls reader status; checks whether a contactless card is currently present in the RF field
	bool is_card_present();

	// Connects to the card if present (using shared mode with T0/T1 protocols)
	bool connect_card();

	// Disconnects from the current card
	void disconnect_card(bool reset = false);

	// Clears cached card identity information
	void invalidate_card_cache();

	// Returns card presence and identity information (UID, ATR, FeliCa/MIFARE type)
	pcsc_card_info get_card_info();

	// Sends APDU command via SCardTransmit and receives response (including SW1/SW2)
	bool transmit_apdu(std::span<const u8> apdu, std::vector<u8>& response);

	// Sends CCID Escape command via SCardControl (IOCTL_CCID_ESCAPE)
	bool control(u32 control_code, std::span<const u8> in_data, std::vector<u8>& out_data);

	// Sends CCID Escape command (tries SCardControl first; falls back to PAPDU_ESCAPE_CMD if blocked by driver)
	bool send_escape(std::span<const u8> escape_data, std::vector<u8>& response);

	// High-level NFC operations translated from PN532 / RC-S620/S:

	// FeliCa raw RF frame pass-through (used for CommunicateThruEX 0xA0)
	// Uses Identiv CNTLESS_FELICA_PASS_THRU (Escape 0xF3) or PAPDU fallback
	bool felica_pass_thru(std::span<const u8> felica_cmd, std::vector<u8>& felica_resp);

	// Mifare Classic Authentication (Load Keys FF 82 + Authenticate FF 86 / FF 88)
	bool mifare_authenticate(u8 block, bool key_a, std::span<const u8, 6> key);

	// Mifare Classic / Ultralight Read Binary (PAPDU_MIFARE_READ_BINARY: FF B0)
	bool mifare_read(u8 block, std::vector<u8>& block_data);

	// Mifare Classic / Ultralight Update Binary (PAPDU_MIFARE_UPDATE_BINARY: FF D6)
	bool mifare_write(u8 block, std::span<const u8> block_data);

	// ISO14443-4 Part 3 Pass-Through (PAPDU_ISO14443_PART3_PASS_THRU: FF EF 01 00 Lc ...)
	bool part3_pass_thru(std::span<const u8> data, std::vector<u8>& response);

	// ISO14443-4 T=CL Pass-Through (PAPDU_TCL_PASS_THRU: FF FE 00 00 Lc ...)
	// Sends APDU/raw data to card via T=CL and receives card response (including card SW1/SW2)
	bool tcl_pass_thru(std::span<const u8> data, std::vector<u8>& response);

	// ISO14443-4 Part 4 / Part 3 Mode Switch (PAPDU_ISO14443_PART4_PART3_SWITCH: FF F8 P1 00 00)
	// P1 = 0x00: MIFARE mode to TCL mode; P1 = 0x01: TCL mode to MIFARE mode (uTrust Section 6.2.12)
	bool part4_switch(bool to_tcl);

private:
	struct impl;
	std::unique_ptr<impl> m_impl;

	// Configures reader hardware parameters (e.g. FeliCa polling on Identiv uTrust 370xF)
	void configure_reader_if_needed();
};
