#pragma once

#include "Emu/Io/usb_device.h"
#include "Utilities/mutex.h"
#include <array>
#include <optional>
#include <queue>

struct kamen_rider_figure
{
	fs::file kamen_file;
	std::array<u8, 0x14 * 0x10> data{};
	std::array<u8, 7> uid{};
	bool present = false;
	void save();
};

class rider_gate
{
public:
	void get_blank_response(u8 command, u8 sequence, std::array<u8, 64>& reply_buf);
	void wake_rider_gate(std::array<u8, 64>& replyBuf, u8 command, u8 sequence);
	void get_list_tags(std::array<u8, 64>& replyBuf, u8 command, u8 sequence);
	void query_block(std::array<u8, 64>& replyBuf, u8 command, u8 sequence, const u8* uid, u8 sector, u8 block);
	void write_block(std::array<u8, 64>& replyBuf, u8 command, u8 sequence, const u8* uid, u8 sector, u8 block, const u8* to_write_buf);
	std::optional<std::array<u8, 64>> pop_added_removed_response();

	bool remove_figure(u8 position);
	u8 load_figure(const std::array<u8, 0x14 * 0x10>& buf, fs::file in_file);

protected:
	shared_mutex kamen_mutex;
	std::array<kamen_rider_figure, 8> figures{};

private:
	u8 generate_checksum(const std::array<u8, 64>& data, u32 num_of_bytes) const;
	kamen_rider_figure& get_figure_by_uid(const std::array<u8, 7> uid);

	std::queue<std::array<u8, 64>> m_figure_added_removed_responses;

	bool m_is_awake = false;
};

extern rider_gate g_ridergate;

class usb_device_kamen_rider : public usb_device_emulated
{
public:
	usb_device_kamen_rider(const std::array<u8, 7>& location);
	~usb_device_kamen_rider();

	static std::shared_ptr<usb_device> make_instance(u32 controller_index, const std::array<u8, 7>& location);
	static u16 get_num_emu_devices();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;

protected:
	std::queue<std::array<u8, 64>> m_queries;
};
